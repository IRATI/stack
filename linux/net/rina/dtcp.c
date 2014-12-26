/*
 * DTCP (Data Transfer Control Protocol)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define RINA_PREFIX "dtcp"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "dtcp.h"
#include "rmt.h"
#include "connection.h"
#include "dtp.h"
#include "dt-utils.h"
#include "dtcp-utils.h"

/* This is the DT-SV part maintained by DTCP */
struct dtcp_sv {
        /* SV lock */
        spinlock_t   lock;

        /* TimeOuts */
        /*
         * When flow control is rate based this timeout may be
         * used to pace number of PDUs sent in TimeUnit
         */
        uint_t       pdus_per_time_unit;

        /* Sequencing */

        /*
         * Outbound: NextSndCtlSeq contains the Sequence Number to
         * be assigned to a control PDU
         */
        seq_num_t    next_snd_ctl_seq;

        /*
         * Inbound: LastRcvCtlSeq - Sequence number of the next
         * expected // Transfer(? seems an error in the spec’s
         * doc should be Control) PDU received on this connection
         */
        seq_num_t    last_rcv_ctl_seq;

        /*
         * Retransmission: There’s no retransmission queue,
         * when a lost PDU is detected a new one is generated
         */

        /* Outbound */
        seq_num_t    last_snd_data_ack;

        /*
         * Seq number of the lowest seq number expected to be
         * Acked. Seq number of the first PDU on the
         * RetransmissionQ. My LWE thus.
         */
        seq_num_t    snd_lft_win;

        /*
         * Maximum number of retransmissions of PDUs without a
         * positive ack before declaring an error
         */
        uint_t       data_retransmit_max;

        /* Inbound */
        seq_num_t    last_rcv_data_ack;

        /* Time (ms) over which the rate is computed */
        uint_t       time_unit;

        /* Flow Control State */

        /* Outbound */
        uint_t       sndr_credit;

        /* snd_rt_wind_edge = LastSendDataAck + PDU(credit) */
        seq_num_t    snd_rt_wind_edge;

        /* PDUs per TimeUnit */
        uint_t       sndr_rate;

        /* PDUs already sent in this time unit */
        uint_t       pdus_sent_in_time_unit;

        /* Inbound */

        /*
         * PDUs receiver believes sender may send before extending
         * credit or stopping the flow on the connection
         */
        uint_t       rcvr_credit;

        /* Value of credit in this flow */
        seq_num_t    rcvr_rt_wind_edge;

        /*
         * Current rate receiver has told sender it may send PDUs
         * at.
         */
        uint_t       rcvr_rate;

        /*
         * PDUs received in this time unit. When it equals
         * rcvr_rate, receiver is allowed to discard any PDUs
         * received until a new time unit begins
         */
        uint_t       pdus_rcvd_in_time_unit;

        /*
         * Control of duplicated control PDUs
         * */
        uint_t       acks;
        uint_t       flow_ctl;
};

struct dtcp_policies {
        int (* flow_init)(struct dtcp * instance);
        int (* sv_update)(struct dtcp * instance, seq_num_t seq);
        int (* lost_control_pdu)(struct dtcp * instance);
        int (* rtt_estimator)(struct dtcp * instance);
        int (* retransmission_timer_expiry)(struct dtcp * instance);
        int (* received_retransmission)(struct dtcp * instance);
        int (* rcvr_ack)(struct dtcp * instance, seq_num_t seq);
        int (* sender_ack)(struct dtcp * instance, seq_num_t seq);
        int (* sending_ack)(struct dtcp * instance);
        int (* receiving_ack_list)(struct dtcp * instance);
        int (* initial_rate)(struct dtcp * instance);
        int (* receiving_flow_control)(struct dtcp * instance, seq_num_t seq);
        int (* update_credit)(struct dtcp * instance);
        int (* flow_control_overrun)(struct dtcp * instance, struct pdu * pdu);
        int (* reconcile_flow_conflict)(struct dtcp * instance);
        int (* rcvr_flow_control)(struct dtcp * instance, seq_num_t seq);
        int (* rate_reduction)(struct dtcp * instance);
        int (* rcvr_control_ack)(struct dtcp * instance);
        int (* no_rate_slow_down)(struct dtcp * instance);
        int (* no_override_default_peak)(struct dtcp * instance);
};

struct dtcp {
        struct dt *               parent;

        /*
         * NOTE: The DTCP State Vector can be discarded during long periods of
         *       no traffic
         */
        struct dtcp_sv *          sv; /* The state-vector */
        struct dtcp_policies *    policies;
        struct connection *       conn;
        struct rmt *              rmt;
        struct workqueue_struct * rcv_wq;

        /* FIXME: Add QUEUE(flow_control_queue, pdu) */
        /* FIXME: Add QUEUE(closed_window_queue, pdu) */
        /* FIXME: Add QUEUE(rx_control_queue, ...) */
};

struct dtcp_config * dtcp_config_get(struct dtcp * dtcp)
{
        if (!dtcp)
                return NULL;
        if (!dtcp->conn)
                return NULL;
        if (!dtcp->conn->policies_params)
                return NULL;
        return dtcp->conn->policies_params->dtcp_cfg;
}
EXPORT_SYMBOL(dtcp_config_get);

static int pdu_send(struct dtcp * dtcp, struct pdu * pdu)
{
        ASSERT(dtcp);
        ASSERT(pdu);

        if (rmt_send(dtcp->rmt,
                     dtcp->conn->destination_address,
                     dtcp->conn->qos_id,
                     pdu))
                return -1;

        return 0;
}

static int last_rcv_ctrl_seq_set(struct dtcp * dtcp,
                                 seq_num_t     last_rcv_ctrl_seq)
{
        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock(&dtcp->sv->lock);
        dtcp->sv->last_rcv_ctl_seq = last_rcv_ctrl_seq;
        spin_unlock(&dtcp->sv->lock);

        return 0;
}

static seq_num_t last_rcv_ctrl_seq(struct dtcp * dtcp)
{
        seq_num_t tmp;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock(&dtcp->sv->lock);
        tmp = dtcp->sv->last_rcv_ctl_seq;
        spin_unlock(&dtcp->sv->lock);

        return tmp;
}

static void flow_ctrl_inc(struct dtcp * dtcp)
{
        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock(&dtcp->sv->lock);
        dtcp->sv->flow_ctl++;
        spin_unlock(&dtcp->sv->lock);
}

static void acks_inc(struct dtcp * dtcp)
{
        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock(&dtcp->sv->lock);
        dtcp->sv->acks++;
        spin_unlock(&dtcp->sv->lock);
}

static int snd_rt_wind_edge_set(struct dtcp * dtcp, seq_num_t new_rt_win)
{
        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock(&dtcp->sv->lock);
        dtcp->sv->snd_rt_wind_edge = new_rt_win;
        spin_unlock(&dtcp->sv->lock);

        return 0;
}

static seq_num_t snd_rt_wind_edge(struct dtcp * dtcp)
{
        seq_num_t tmp;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock(&dtcp->sv->lock);
        tmp = dtcp->sv->snd_rt_wind_edge;
        spin_unlock(&dtcp->sv->lock);

        return tmp;
}

static seq_num_t snd_lft_win(struct dtcp * dtcp)
{
        seq_num_t tmp;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock(&dtcp->sv->lock);
        tmp = dtcp->sv->snd_lft_win;
        spin_unlock(&dtcp->sv->lock);

        return tmp;
}

static seq_num_t rcvr_rt_wind_edge(struct dtcp * dtcp)
{
        seq_num_t tmp;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock(&dtcp->sv->lock);
        tmp = dtcp->sv->rcvr_rt_wind_edge;
        spin_unlock(&dtcp->sv->lock);

        return tmp;
}

static seq_num_t next_snd_ctl_seq(struct dtcp * dtcp)
{
        seq_num_t tmp;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock(&dtcp->sv->lock);
        tmp = ++dtcp->sv->next_snd_ctl_seq;
        spin_unlock(&dtcp->sv->lock);

        return tmp;
}

static seq_num_t last_snd_data_ack(struct dtcp * dtcp)
{
        seq_num_t tmp;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock(&dtcp->sv->lock);
        tmp = dtcp->sv->last_snd_data_ack;
        spin_unlock(&dtcp->sv->lock);

        return tmp;
}

static void last_snd_data_ack_set(struct dtcp * dtcp, seq_num_t seq_num)
{
        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock(&dtcp->sv->lock);
        dtcp->sv->last_snd_data_ack = seq_num;
        spin_unlock(&dtcp->sv->lock);
}

static int push_pdus_rmt(struct dtcp * dtcp)
{
        struct cwq *  q;

        ASSERT(dtcp);

        q = dt_cwq(dtcp->parent);
        if (!q) {
                LOG_ERR("No Closed Window Queue");
                return -1;
        }

        cwq_deliver(q,
                    dtcp->parent,
                    dtcp->rmt,
                    dtcp->conn->destination_address,
                    dtcp->conn->qos_id);

        return 0;
}

static struct pdu * pdu_ctrl_create_ni(struct dtcp * dtcp, pdu_type_t type)
{
        struct pdu *    pdu;
        struct pci *    pci;
        struct buffer * buffer;
        seq_num_t       seq;

        if (!pdu_type_is_control(type))
                return NULL;

        buffer = buffer_create_ni(1);
        if (!buffer)
                return NULL;

        pdu = pdu_create_ni();
        if (!pdu) {
                buffer_destroy(buffer);
                return NULL;
        }

        pci = pci_create_ni();
        if (!pci) {
                pdu_destroy(pdu);
                return NULL;
        }

        seq = next_snd_ctl_seq(dtcp);
        if (pci_format(pci,
                       dtcp->conn->source_cep_id,
                       dtcp->conn->destination_cep_id,
                       dtcp->conn->source_address,
                       dtcp->conn->destination_address,
                       seq,
                       dtcp->conn->qos_id,
                       type)) {
                pdu_destroy(pdu);
                pci_destroy(pci);
                return NULL;
        }

        if (pci_control_last_seq_num_rcvd_set(pci,last_rcv_ctrl_seq(dtcp))) {
                pci_destroy(pci);
                pdu_destroy(pdu);
                return NULL;
        }

        if (pdu_pci_set(pdu, pci)) {
                pdu_destroy(pdu);
                pci_destroy(pci);
                return NULL;
        }

        if (pdu_buffer_set(pdu, buffer)) {
                pdu_destroy(pdu);
                return NULL;
        }

        return pdu;
}

static int populate_ctrl_pci(struct pci *  pci,
                             struct dtcp * dtcp,
                             seq_num_t     LWE)
{
        struct dtcp_config * dtcp_cfg;
        seq_num_t snd_lft;
        seq_num_t snd_rt;

        dtcp_cfg = dtcp_config_get(dtcp);
        if (!dtcp_cfg) {
                LOG_ERR("No dtcp cfg...");
                return -1;
        }

        /*
         * FIXME: Shouldn't we check if PDU_TYPE_ACK_AND_FC or
         * PDU_TYPE_NACK_AND_FC ?
         */
        if (dtcp_flow_ctrl(dtcp_cfg)) {
                if (dtcp_window_based_fctrl(dtcp_cfg)) {
                        snd_lft = snd_lft_win(dtcp);
                        snd_rt  = snd_rt_wind_edge(dtcp);

                        pci_control_new_left_wind_edge_set(pci, LWE);
                        pci_control_new_rt_wind_edge_set(pci,
                                                         rcvr_rt_wind_edge(dtcp));
                        pci_control_my_left_wind_edge_set(pci, snd_lft);
                        pci_control_my_rt_wind_edge_set(pci, snd_rt);
                }

                if (dtcp_rate_based_fctrl(dtcp_cfg)) {
                        LOG_MISSING;
                }
        }

        switch (pci_type(pci)) {
        case PDU_TYPE_ACK_AND_FC:
        case PDU_TYPE_ACK:
                if (pci_control_ack_seq_num_set(pci, LWE)) {
                        LOG_ERR("Could not set sn to ACK");
                        return -1;
                }
                return 0;
        case PDU_TYPE_NACK_AND_FC:
        case PDU_TYPE_NACK:
                if (pci_control_ack_seq_num_set(pci, LWE + 1)) {
                        LOG_ERR("Could not set sn to NACK");
                        return -1;
                }
                return 0;
        default:
                break;
        }

        return 0;
}

static pdu_type_t pdu_ctrl_type_get(struct dtcp * dtcp, seq_num_t seq)
{
        struct dtcp_config * dtcp_cfg;
        seq_num_t    LWE;
        timeout_t    a;

        ASSERT(dtcp);
        ASSERT(dtcp->parent);

        dtcp_cfg = dtcp_config_get(dtcp);
        ASSERT(dtcp_cfg);

        a = dt_sv_a(dtcp->parent);

        /*FIXME: pdu_ctrl_type_get should not be controlling if the seq_num was
         * already acked, I would move this out of here, probably to
         * default_rcvr_ack and default_sending_ack policies */
        LWE = dt_sv_rcv_lft_win(dtcp->parent);
        if (last_snd_data_ack(dtcp) < LWE) {
                last_snd_data_ack_set(dtcp, LWE);
                if (!a) {
#if 0
                        if (seq > LWE) {
                                LOG_DBG("This is a NACK, "
                                        "LWE couldn't be updated");
                                if (dtcp_flow_ctrl(dtcp_cfg)) {
                                        return PDU_TYPE_NACK_AND_FC;
                                }
                                return PDU_TYPE_NACK;
                        }
#endif
                        LOG_DBG("This is an ACK");
                        if (dtcp_flow_ctrl(dtcp_cfg)) {
                                return PDU_TYPE_ACK_AND_FC;
                        }
                        return PDU_TYPE_ACK;
                }
#if 0
                if (seq > LWE) {
                        /* FIXME: This should be a SEL ACK */
                        LOG_DBG("This is a NACK, "
                                "LWE couldn't be updated");
                        if (dtcp_flow_ctrl(dtcp_cfg)) {
                                return PDU_TYPE_NACK_AND_FC;
                        }
                        return PDU_TYPE_NACK;
                }
#endif
                LOG_DBG("This is an ACK");
                if (dtcp_flow_ctrl(dtcp_cfg)) {
                        return PDU_TYPE_ACK_AND_FC;
                }
                return PDU_TYPE_ACK;
        }
        LOG_DBG("LWE already acked");
        return 0;
}

static int default_rcvr_ack_atimer(struct dtcp * dtcp, seq_num_t seq)
{
        LOG_MISSING;
        return 0;
}

static int default_sender_ack(struct dtcp * dtcp, seq_num_t seq_num)
{
        if (!dtcp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }

        if (dtcp_rtx_ctrl(dtcp_config_get(dtcp))) {
                struct rtxq * q;

                q = dt_rtxq(dtcp->parent);
                if (!q) {
                        LOG_ERR("Couldn't find the Retransmission queue");
                        return -1;
                }
                rtxq_ack(q, seq_num, dt_sv_tr(dtcp->parent));
        }

        return 0;
}

/* not a policy according to specs */
static int rcv_nack_ctl(struct dtcp * dtcp, seq_num_t seq_num)
{
        struct rtxq * q;

        if (dtcp_rtx_ctrl(dtcp_config_get(dtcp))) {
                q = dt_rtxq(dtcp->parent);
                if (!q) {
                        LOG_ERR("Couldn't find the Retransmission queue");
                        return -1;
                }
                rtxq_nack(q, seq_num, dt_sv_tr(dtcp->parent));
        }
        return 0;
}

static void dump_we(struct dtcp * dtcp,
                    struct pci *  pci)
{
        struct dtp * dtp;
        seq_num_t    snd_rt_we;
        seq_num_t    snd_lf_we;
        seq_num_t    cwq_lf_we = 0;
        seq_num_t    rcv_rt_we;
        seq_num_t    rcv_lf_we;
        seq_num_t    new_rt_we;
        seq_num_t    new_lf_we;
        seq_num_t    pci_seqn;
        seq_num_t    my_rt_we;
        seq_num_t    my_lf_we;
        seq_num_t    ack;

        ASSERT(dtcp);
        ASSERT(pci);

        dtp = dt_dtp(dtcp->parent);
        ASSERT(dtp);

        snd_rt_we = snd_rt_wind_edge(dtcp);
        snd_lf_we = dtcp_snd_lf_win(dtcp);
        /* commented to avoid doing spin_lock_irqsave */
        /* cwq_lf_we = cwq_peek(dt_cwq(dtcp->parent));*/
        rcv_rt_we = rcvr_rt_wind_edge(dtcp);
        rcv_lf_we = dt_sv_rcv_lft_win(dtcp->parent);
        new_rt_we = pci_control_new_rt_wind_edge(pci);
        new_lf_we = pci_control_new_left_wind_edge(pci);
        my_lf_we  = pci_control_my_left_wind_edge(pci);
        my_rt_we  = pci_control_my_rt_wind_edge(pci);
        pci_seqn  = pci_sequence_number_get(pci);
        ack       = pci_control_ack_seq_num(pci);

        LOG_INFO("SEQN: %u N/Ack: %u SndRWE: %u SndLWE: %u RcvRWE: %u RcvLWE: %u"
                 " newRWE: %u newLWE: %u myRWE: %u myLWE: %u cwqLWE: %u",
                 pci_seqn, ack, snd_rt_we, snd_lf_we, rcv_rt_we, rcv_lf_we,
                 new_rt_we, new_lf_we, my_rt_we, my_lf_we, cwq_lf_we);

}

static int rcv_flow_ctl(struct dtcp * dtcp,
                        struct pci *  pci,
                        struct pdu *  pdu)
{
        struct cwq * q;
        struct dtp * dtp;

        ASSERT(dtcp);
        ASSERT(pci);
        ASSERT(pdu);

        snd_rt_wind_edge_set(dtcp, pci_control_new_rt_wind_edge(pci));
        pdu_destroy(pdu);
        push_pdus_rmt(dtcp);

        dtp = dt_dtp(dtcp->parent);
        if (!dtp) {
                LOG_ERR("No DTP");
                return -1;
        }
        q = dt_cwq(dtcp->parent);
        if (!q) {
                LOG_ERR("No Closed Window Queue");
                return -1;
        }
        if (cwq_is_empty(q) &&
            (dtp_sv_last_seq_nr_sent(dtp) < snd_rt_wind_edge(dtcp))) {
                dt_sv_window_closed_set(dtcp->parent, false);
        }

        return 0;
}

static int rcv_ack_and_flow_ctl(struct dtcp * dtcp,
                                struct pci *  pci,
                                struct pdu *  pdu)
{
        seq_num_t seq;

        ASSERT(dtcp);
        ASSERT(pci);
        ASSERT(pdu);

        LOG_DBG("Updating Window Edges for DTCP: %pK", dtcp);

        seq = pci_control_ack_seq_num(pci);
        LOG_DBG("Ack/Nack SEQ NUM: %u", seq);

        /* This updates sender LWE */
        if (dtcp->policies->sender_ack(dtcp, seq))
                LOG_ERR("Could not update RTXQ and LWE");

        snd_rt_wind_edge_set(dtcp, pci_control_new_rt_wind_edge(pci));
        LOG_DBG("Right Window Edge: %d", snd_rt_wind_edge(dtcp));
        pdu_destroy(pdu);

        LOG_DBG("Calling CWQ_deliver for DTCP: %pK", dtcp);
        push_pdus_rmt(dtcp);

        /* FIXME: Verify values for the receiver side */

        return 0;
}

struct dtcp_rcv_item {
        struct dtcp * dtcp;
        struct pdu *  pdu;
};

static int dtcp_common_rcv_control(void * o)
{
        struct dtcp *          dtcp;
        struct pdu *           pdu;
        struct pci *           pci;
        struct dtcp_rcv_item * ritem;
        pdu_type_t             type;
        seq_num_t              seq_num;
        seq_num_t              seq;
        seq_num_t              last_ctrl;

        /*  VARIABLES FOR SYSTEM TIME DBG MESSAGE BELOW */
        struct timeval te;
        long long milliseconds;

        LOG_DBG("dtcp_common_rcv_control called");

        ritem = (struct dtcp_rcv_item *) o;
        if (!ritem) {
                LOG_ERR("Bogus dtcp rcv_item...");
                return -1;
        }

        pdu = ritem->pdu;
        if (!pdu_is_ok(pdu)) {
                LOG_ERR("Receive_item contained bogus pdu");
                rkfree(ritem);
                return -1;
        }

        dtcp = ritem->dtcp;
        if (!dtcp) {
                LOG_ERR("Bogus instance passed, bailing out");
                rkfree(ritem);
                pdu_destroy(pdu);
                return -1;
        }

        rkfree(ritem);

        pci = pdu_pci_get_rw(pdu);
        if (!pci_is_ok(pci)) {
                LOG_ERR("PCI couldn't be retrieved");
                pdu_destroy(pdu);
                return -1;
        }

        type = pci_type(pci);

        if (!pdu_type_is_control(type)) {
                LOG_ERR("CommonRCVControl policy received a non-control PDU");
                pdu_destroy(pdu);
                return -1;
        }

        seq_num = pci_sequence_number_get(pci);
        last_ctrl = last_rcv_ctrl_seq(dtcp);

        /*  SYSTEM TIME DBG_MESSAGE */
        do_gettimeofday(&te);
        milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
        LOG_DBG("DTCP Received Contrl PDU %d at %lld", seq_num, milliseconds);
        dump_we(dtcp, pci);

        if (seq_num > (last_ctrl + 1))
                dtcp->policies->lost_control_pdu(dtcp);

        if (seq_num <= last_ctrl) {
                switch (type) {
                case PDU_TYPE_FC:
                        flow_ctrl_inc(dtcp);
                        break;
                case PDU_TYPE_ACK:
                        acks_inc(dtcp);
                        break;
                case PDU_TYPE_ACK_AND_FC:
                        acks_inc(dtcp);
                        flow_ctrl_inc(dtcp);
                        break;
                default:
                        break;
                }

                pdu_destroy(pdu);
                return 0;

        }

        /* We are in seq_num == last_ctrl + 1 */

        last_rcv_ctrl_seq_set(dtcp, seq_num);

        /*
         * FIXME: Missing step described in the specs: retrieve the time
         *        of this Ack and calculate the RTT with RTTEstimator policy
         */

        LOG_DBG("dtcp_common_rcv_control sending to proper function...");

        switch (type) {
        case PDU_TYPE_ACK:
                seq = pci_control_ack_seq_num(pci);

                return dtcp->policies->sender_ack(dtcp,
                                                  seq);
        case PDU_TYPE_NACK:
                seq = pci_control_ack_seq_num(pdu_pci_get_ro(pdu));

                return rcv_nack_ctl(dtcp,
                                    seq);
        case PDU_TYPE_FC:
                return rcv_flow_ctl(dtcp, pci, pdu);
        case PDU_TYPE_ACK_AND_FC:
                return rcv_ack_and_flow_ctl(dtcp, pci, pdu);
        default:
                return -1;
        }
}

int dtcp_receive(struct dtcp * instance,
                 struct pdu * pdu)
{
        struct rwq_work_item * item;
        struct dtcp_rcv_item * ritem;

        LOG_DBG("dtcp_receive called");

        if (!pdu_is_ok(pdu)) {
                pdu_destroy(pdu);
                return -1;
        }

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                pdu_destroy(pdu);
                return -1;
        }

        ritem = rkzalloc(sizeof(*ritem), GFP_ATOMIC);
        if (!ritem) {
                pdu_destroy(pdu);
                LOG_ERR("Could not create dtcp receive item");
                return -1;
        }

        ritem->dtcp = instance;
        ritem->pdu = pdu;

        item = rwq_work_create_ni(dtcp_common_rcv_control, ritem);
        if (!item) {
                LOG_ERR("Could not create wwq item");
                pdu_destroy(pdu);
                rkfree(ritem);
                return -1;
        }

        if (rwq_work_post(instance->rcv_wq, item)) {
                LOG_ERR("Could not add dtcp rcv wq item to the wq");
                pdu_destroy(pdu);
                return -1;
        }

        LOG_DBG("dtcp_receive ends...");
        return 0;
}

static int default_lost_control_pdu(struct dtcp * dtcp)
{
        if (!dtcp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }

        LOG_DBG("Default lost control pdu policy");

        return 0;

}

/*FIXME: wrapper to be called by dtp in the post_worker */
int dtcp_sending_ack_policy(struct dtcp * dtcp)
{
        if (!dtcp) {
                LOG_ERR("No DTCP passed...");
                return -1;
        }
        ASSERT(dtcp->policies);
        if (!dtcp->policies->sending_ack) {
                LOG_ERR("No sending_ack policy in dtcp");
                return -1;
        }
        return dtcp->policies->sending_ack(dtcp);
}

static int default_sending_ack(struct dtcp * dtcp)
{
        struct dtp *         dtp;
        seq_num_t            LWE;
        pdu_type_t           type;
        struct pdu *         pdu;
        struct pci *         pci;
        struct dtcp_config * dtcp_cfg;

        dtp = dt_dtp(dtcp->parent);
        if (!dtp) {
                LOG_ERR("No DTP from dtcp->parent");
                return -1;
        }

        dtcp_cfg = dtcp_config_get(dtcp);
        if (!dtcp_cfg)
                return -1;

        /* Invoke delimiting and update left window edge */

        LWE = process_A_expiration(dtp, dtcp);
        LOG_DBG("Seq_num_to_ack: %d", LWE);

        if ((int) LWE < 0) {
                LOG_ERR("LWE is negative");
                return -1;
        }

        /* FIXME: this should use something like pdu_ctrl_type_get, but the
         * last_snd_data_ack check would not work */
        if (last_snd_data_ack(dtcp) < LWE) {
                last_snd_data_ack_set(dtcp, LWE);

                type = PDU_TYPE_FC;
                if (dtcp_rtx_ctrl(dtcp_cfg) && dtcp_flow_ctrl(dtcp_cfg)) {
                        type =  PDU_TYPE_ACK_AND_FC;
                }
                if (dtcp_rtx_ctrl(dtcp_cfg) && !dtcp_flow_ctrl(dtcp_cfg)) {
                        type =  PDU_TYPE_ACK;
                }
                LOG_DBG("Ctrl PDU type: %x", type);

                pdu  = pdu_ctrl_create_ni(dtcp, type);
                if (!pdu) {
                        LOG_ERR("No Ctrl PDU created...");
                        return -1;
                }

                pci = pdu_pci_get_rw(pdu);
                if (populate_ctrl_pci(pci, dtcp, LWE)) {
                        LOG_ERR("Could not populate ctrl PCI");
                        pdu_destroy(pdu);
                        return -1;
                }

                dump_we(dtcp, pci);
                if (pdu_send(dtcp, pdu)) {
                        LOG_ERR("Could not send ctrl PDU");
                        return -1;
                }
        }
        return 0;
}

static int default_rcvr_ack(struct dtcp * dtcp, seq_num_t seq)
{
        struct pdu * pdu;
        struct pci * pci;
        seq_num_t    LWE;
        pdu_type_t   type;

        /* VARIABLES FOR SYSTEM TIMESTAMP DBG MESSAGE BELOW*/
        struct timeval te;
        long long milliseconds;

        if (!dtcp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }

        type = pdu_ctrl_type_get(dtcp, seq);
        if (!type)
                return 0;

        pdu  = pdu_ctrl_create_ni(dtcp, type);
        if (!pdu)
                return -1;


        pci = pdu_pci_get_rw(pdu);
        LWE = dt_sv_rcv_lft_win(dtcp->parent);
        if (populate_ctrl_pci(pci, dtcp, LWE)) {
                pdu_destroy(pdu);
                return -1;
        }

        LOG_DBG("default_rcvr_ack");
        dump_we(dtcp, pci);
        if (pdu_send(dtcp, pdu))
                return -1;

        /* SYSTEM TIMESTAMP DBG MESSAGE */
        do_gettimeofday(&te);
        milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
        LOG_DBG("DTCP Sending ACK %d at %lld", pci_sequence_number_get(pci), milliseconds);

        return 0;
}

static int default_receiving_flow_control(struct dtcp * dtcp, seq_num_t seq)
{
        struct pdu * pdu;
        struct pci * pci;
        seq_num_t    LWE;

        /* VARIABLES FOR SYSTEM TIMESTAMP DBG MESSAGE BELOW*/
        struct timeval te;
        long long milliseconds;

        if (!dtcp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }
        pdu = pdu_ctrl_create_ni(dtcp, PDU_TYPE_FC);
        if (!pdu)
                return -1;

        pci = pdu_pci_get_rw(pdu);
        if (!pci) {
                pdu_destroy(pdu);
                return -1;
        }

        LWE     = dt_sv_rcv_lft_win(dtcp->parent);
        if (populate_ctrl_pci(pci, dtcp, LWE)) {
                pdu_destroy(pdu);
                return -1;
        }

        dump_we(dtcp, pci);
        if (pdu_send(dtcp, pdu))
                return -1;

        /* SYSTEM TIMESTAMP DBG MESSAGE */
        do_gettimeofday(&te);
        milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
        LOG_DBG("DTCP Sending FC %d at %lld", pci_sequence_number_get(pci), milliseconds);

        return 0;
}

static void update_rt_wind_edge(struct dtcp * dtcp)
{
        seq_num_t seq;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        seq = dt_sv_rcv_lft_win(dtcp->parent);
        spin_lock(&dtcp->sv->lock);
        seq += dtcp->sv->rcvr_credit;
        dtcp->sv->rcvr_rt_wind_edge = seq;
        spin_unlock(&dtcp->sv->lock);
}

static int default_rcvr_flow_control(struct dtcp * dtcp, seq_num_t seq)
{
        if (!dtcp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }

        update_rt_wind_edge(dtcp);

        return 0;
}

static int default_rate_reduction(struct dtcp * instance)
{
        if (!instance) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }

        LOG_MISSING;

        return 0;
}

static int default_flow_control_overrun(struct dtcp * instance,
                                        struct pdu * pdu)
{
        pdu_destroy(pdu);

        return 0;
}

static int default_sv_update(struct dtcp * dtcp, seq_num_t seq)
{
        int                  retval = 0;
        struct dtcp_config * dtcp_cfg;

        bool                 flow_ctrl;
        bool                 win_based;
        bool                 rate_based;
        bool                 rtx_ctrl;

        if (!dtcp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }

        dtcp_cfg = dtcp_config_get(dtcp);
        if (!dtcp_cfg)
                return -1;

        flow_ctrl  = dtcp_flow_ctrl(dtcp_cfg);
        win_based  = dtcp_window_based_fctrl(dtcp_cfg);
        rate_based = dtcp_rate_based_fctrl(dtcp_cfg);
        rtx_ctrl   = dtcp_rtx_ctrl(dtcp_cfg);

        LOG_DBG("SV Update Seq Num: %u", seq);

        if (flow_ctrl) {
                if (win_based) {
                        if (dtcp->policies->rcvr_flow_control(dtcp, seq)) {
                                LOG_ERR("Failed Rcvr Flow Control policy");
                                retval = -1;
                        }
                }

                if (rate_based) {
                        LOG_DBG("Rate based fctrl invoked");
                        if (dtcp->policies->rate_reduction(dtcp)) {
                                LOG_ERR("Failed Rate Reduction policy");
                                retval = -1;
                        }
                }

                if (!rtx_ctrl) {
                        LOG_DBG("Receiving flow ctrl invoked");
                        if (dtcp->policies->receiving_flow_control(dtcp,
                                                                   seq)) {
                                LOG_ERR("Failed Receiving Flow Control "
                                        "policy");
                                retval = -1;
                        }

                        return retval;
                }
        }

        if (rtx_ctrl) {
                LOG_DBG("Retransmission ctrl invoked");
                if (dtcp->policies->rcvr_ack(dtcp, seq)) {
                        LOG_ERR("Failed Rcvr Ack policy");
                        retval = -1;
                }
        }

        return retval;
}

static struct dtcp_sv default_sv = {
        .pdus_per_time_unit     = 0,
        .next_snd_ctl_seq       = 0,
        .last_rcv_ctl_seq       = 0,
        .last_snd_data_ack      = 0,
        .snd_lft_win            = 0,
        .data_retransmit_max    = 0,
        .last_rcv_data_ack      = 0,
        .time_unit              = 0,
        .sndr_credit            = 1,
        .snd_rt_wind_edge       = 100,
        .sndr_rate              = 0,
        .pdus_sent_in_time_unit = 0,
        .rcvr_credit            = 1,
        .rcvr_rt_wind_edge      = 100,
        .rcvr_rate              = 0,
        .pdus_rcvd_in_time_unit = 0,
        .acks                   = 0,
        .flow_ctl               = 0,
};

static struct dtcp_policies default_policies = {
        .flow_init                   = NULL,
        .sv_update                   = default_sv_update,
        .lost_control_pdu            = default_lost_control_pdu,
        .rtt_estimator               = NULL,
        .retransmission_timer_expiry = NULL,
        .received_retransmission     = NULL,
        .sender_ack                  = default_sender_ack,
        .sending_ack                 = default_sending_ack,
        .receiving_ack_list          = NULL,
        .initial_rate                = NULL,
        .receiving_flow_control      = default_receiving_flow_control,
        .update_credit               = NULL,
        .flow_control_overrun        = default_flow_control_overrun,
        .reconcile_flow_conflict     = NULL,
        .rcvr_ack                    = default_rcvr_ack,
        .rcvr_flow_control           = default_rcvr_flow_control,
        .rate_reduction              = default_rate_reduction,
        .rcvr_control_ack            = NULL,
        .no_rate_slow_down           = NULL,
        .no_override_default_peak    = NULL,
};

/* FIXME: this should be completed with other parameters from the config */
static int dtcp_sv_init(struct dtcp * instance, struct dtcp_sv sv)
{
        struct dtcp_config * cfg;

        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        if (!instance->sv) {
                LOG_ERR("Bogus sv passed");
                return -1;
        }

        cfg = dtcp_config_get(instance);
        if (!cfg)
                return -1;

        *instance->sv = sv;
        spin_lock_init(&instance->sv->lock);

        if (dtcp_rtx_ctrl(cfg))
                instance->sv->data_retransmit_max =
                        dtcp_data_retransmit_max(cfg);

        instance->sv->sndr_credit         = dtcp_initial_credit(cfg);
        instance->sv->snd_rt_wind_edge    = dtcp_initial_credit(cfg);
        instance->sv->rcvr_credit         = dtcp_initial_credit(cfg);
        instance->sv->rcvr_rt_wind_edge   = dtcp_initial_credit(cfg);

        LOG_DBG("DTCP SV initialized with dtcp_conf:");
        LOG_DBG("  data_retransmit_max: %d",
                instance->sv->data_retransmit_max);
        LOG_DBG("  sndr_credit:         %d",
                instance->sv->sndr_credit);
        LOG_DBG("  snd_rt_wind_edge:    %d",
                instance->sv->snd_rt_wind_edge);
        LOG_DBG("  rcvr_credit:         %d",
                instance->sv->rcvr_credit);
        LOG_DBG("  rcvr_rt_wind_edge:   %d",
                instance->sv->rcvr_rt_wind_edge);

        return 0;
}

#define MAX_NAME_SIZE 128
/* FIXME: This function is not re-entrant */
static const char * rcvwq_name_format(const char *        prefix,
                                      const struct dtcp * instance)
{
        static char name[MAX_NAME_SIZE];

        ASSERT(prefix);
        ASSERT(instance);

        if (snprintf(name, sizeof(name), RINA_PREFIX "-%s-%pK",
                     prefix, instance) >=
            sizeof(name))
                return NULL;

        return name;
}

struct dtcp * dtcp_create(struct dt *         dt,
                          struct connection * conn,
                          struct rmt *        rmt)
{
        struct dtcp * tmp;
        const char *  rwq_name;

        if (!dt) {
                LOG_ERR("No DT passed, bailing out");
                return NULL;
        }
        if (!conn) {
                LOG_ERR("No connection, bailing out");
                return NULL;
        }
        if (!rmt) {
                LOG_ERR("No RMT, bailing out");
                return NULL;
        }

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp) {
                LOG_ERR("Cannot create DTCP state-vector");
                return NULL;
        }

        tmp->parent = dt;

        tmp->sv = rkzalloc(sizeof(*tmp->sv), GFP_KERNEL);
        if (!tmp->sv) {
                LOG_ERR("Cannot create DTCP state-vector");
                dtcp_destroy(tmp);
                return NULL;
        }
        tmp->policies = rkzalloc(sizeof(*tmp->policies), GFP_KERNEL);
        if (!tmp->policies) {
                LOG_ERR("Cannot create DTCP policies");
                dtcp_destroy(tmp);
                return NULL;
        }

	/* FIXME: This function must change */
        rwq_name = rcvwq_name_format("dtcprcvwq", tmp);
        if (!rwq_name) {
                dtcp_destroy(tmp);
                return NULL;
        }
        tmp->rcv_wq = rwq_create(rwq_name);
        if (!tmp->rcv_wq) {
                dtcp_destroy(tmp);
                return NULL;
        }

        tmp->conn = conn;
        tmp->rmt  = rmt;

        if (dtcp_sv_init(tmp, default_sv)) {
                LOG_ERR("Could not load DTCP config in the SV");
                dtcp_destroy(tmp);
                return NULL;
        }
        /* FIXME: fixups to the state-vector should be placed here */

        *tmp->policies = default_policies;
        /* FIXME: fixups to the policies should be placed here */

        LOG_DBG("Instance %pK created successfully", tmp);

        return tmp;
}

int dtcp_destroy(struct dtcp * instance)
{
        if (!instance) {
                LOG_ERR("Bad instance passed, bailing out");
                return -1;
        }

        if (instance->sv)       rkfree(instance->sv);
        if (instance->policies) rkfree(instance->policies);
        if (instance->rcv_wq)   rwq_destroy(instance->rcv_wq);

        rkfree(instance);

        LOG_DBG("Instance %pK destroyed successfully", instance);

        return 0;
}

int dtcp_sv_update(struct dtcp * instance,
                   seq_num_t     seq)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        ASSERT(instance->policies);
        ASSERT(instance->policies->sv_update);

        if (instance->policies->sv_update(instance, seq))
                return -1;

        return 0;
}

int dtcp_ack_flow_control_pdu_send(struct dtcp * dtcp)
{
        if (!dtcp)
                return -1;

        LOG_MISSING;

        return 0;
}

seq_num_t dtcp_snd_rt_win(struct dtcp * dtcp)
{
        if (!dtcp || !dtcp->sv)
                return -1;

        return snd_rt_wind_edge(dtcp);
}

seq_num_t dtcp_snd_lf_win(struct dtcp * dtcp)
{
        if (!dtcp || !dtcp->sv)
                return -1;

        return snd_lft_win(dtcp);
}

int dtcp_snd_lf_win_set(struct dtcp * instance, seq_num_t seq_num)
{
        if (!instance)
                return -1;

        spin_lock(&instance->sv->lock);
        instance->sv->snd_lft_win = seq_num;
        spin_unlock(&instance->sv->lock);

        return 0;
}
