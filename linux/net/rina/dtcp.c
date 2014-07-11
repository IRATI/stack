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
         * RetransmissionQ.
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
        int (* sending_ack)(struct dtcp * instance, seq_num_t seq);
        int (* receiving_ack_list)(struct dtcp * instance);
        int (* initial_rate)(struct dtcp * instance);
        int (* receiving_flow_control)(struct dtcp * instance, seq_num_t seq);
        int (* update_credit)(struct dtcp * instance);
        int (* flow_control_overrun)(struct dtcp * instance);
        int (* reconcile_flow_conflict)(struct dtcp * instance);
        int (* rcvr_flow_control)(struct dtcp * instance, seq_num_t seq);
        int (* rate_reduction)(struct dtcp * instance);
        int (* rcvr_control_ack)(struct dtcp * instance);
        int (* no_rate_slow_down)(struct dtcp * instance);
        int (* no_override_default_peak)(struct dtcp * instance);
        int (* receiver_inactivity_timer)(struct dtcp * instance);
        int (* sender_inactivity_timer)(struct dtcp * instance);
};

struct dtcp {
        struct dt *            parent;

        /*
         * NOTE: The DTCP State Vector can be discarded during long periods of
         *       no traffic
         */
        struct dtcp_sv *       sv; /* The state-vector */
        struct dtcp_policies * policies;
        struct connection *    conn;
        struct rmt *           rmt;

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

static void snd_lft_win_set(struct dtcp * dtcp, seq_num_t seq_num)
{
        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock(&dtcp->sv->lock);
        dtcp->sv->snd_lft_win = seq_num;
        spin_unlock(&dtcp->sv->lock);
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

static int push_pdus_rmt(struct dtcp * dtcp)
{
        struct cwq *  q;
        struct rtxq * rtxq;

        ASSERT(dtcp);

        q = dt_cwq(dtcp->parent);
        if (!q) {
                LOG_ERR("No Closed Window Queue");
                return -1;
        }

        while (!cwq_is_empty(q) &&
               (snd_lft_win(dtcp) < snd_rt_wind_edge(dtcp))) {
                struct pdu * pdu;

                pdu = cwq_pop(q);
                if (!pdu)
                        return 0;

                if (dtcp_rtx_ctrl(dtcp_config_get(dtcp))) {
                        rtxq = dt_rtxq(dtcp->parent);
                        if (!rtxq) {
                                LOG_ERR("Couldn't find the "
                                        "Retransmission queue");
                                return -1;
                        }
                        rtxq_push(rtxq, pdu);
                }
                /* FIXME: We must update the last seq num sent */
                if (pdu_send(dtcp, pdu))
                        LOG_ERR("Problems sending PDU");

        }

        return 0;
}

static struct pdu * pdu_ctrl_create_ni(struct dtcp * dtcp, pdu_type_t type)
{
        struct pdu *    pdu;
        struct pci *    pci;
        struct buffer * buffer;
        seq_num_t       seq;

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

/* This is 0x8803 PDU type */
static struct pdu * pdu_ctrl_ack_create(struct dtcp * dtcp,
                                        seq_num_t     last_ctrl_seq_rcvd,
                                        seq_num_t     snd_left_wind_edge,
                                        seq_num_t     snd_rt_wind_edge)
{
        struct pdu * pdu;
        struct pci * pci;

        pdu = pdu_ctrl_create_ni(dtcp, PDU_TYPE_ACK_AND_FC);
        if (!pdu)
                return NULL;

        pci = pdu_pci_get_rw(pdu);
        if (!pci)
                return NULL;

        if (pci_control_last_seq_num_rcvd_set(pci, last_ctrl_seq_rcvd)) {
                pdu_destroy(pdu);
                return NULL;
        }

        return pdu;
}

/* This is 880C */
static struct pdu * pdu_ctrl_ack_flow(struct dtcp * dtcp,
                                      seq_num_t     ctrl_seq,
                                      seq_num_t     ack_nack_seq,
                                      seq_num_t     new_rt_wind_edge,
                                      seq_num_t     left_wind_edge)
{
        struct pdu * pdu;
        struct pci * pci;
        seq_num_t    my_lf_win_edge;
        seq_num_t    my_rt_win_edge;

        pdu = pdu_ctrl_create_ni(dtcp, PDU_TYPE_ACK_AND_FC);
        if (!pdu)
                return NULL;

        pci = pdu_pci_get_rw(pdu);
        if (!pci) {
                pdu_destroy(pdu);
                return NULL;
        }

        if (pci_control_ack_seq_num_set(pci, ack_nack_seq)) {
                pdu_destroy(pdu);
                return NULL;
        }

        if (pci_control_last_seq_num_rcvd_set(pci, ctrl_seq)) {
                pdu_destroy(pdu);
                return NULL;
        }

        if (pci_control_new_left_wind_edge_set(pci, left_wind_edge)) {
                pdu_destroy(pdu);
                return NULL;
        }

        if (pci_control_new_rt_wind_edge_set(pci, new_rt_wind_edge)) {
                pdu_destroy(pdu);
                return NULL;
        }

        my_lf_win_edge = dt_sv_rcv_lft_win(dtcp->parent);
        if (pci_control_left_wind_edge_set(pci,my_lf_win_edge)) {
                pdu_destroy(pdu);
                return NULL;
        }

        my_rt_win_edge = rcvr_rt_wind_edge(dtcp);
        if (pci_control_rt_wind_edge_set(pci, my_rt_win_edge)) {
                pdu_destroy(pdu);
                return NULL;
        }

        return pdu;
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

static int rcv_flow_ctl(struct dtcp * dtcp,
                        struct pci *  pci,
                        struct pdu *  pdu)
{
        struct cwq * q;

        ASSERT(dtcp);
        ASSERT(pci);
        ASSERT(pdu);

        snd_rt_wind_edge_set(dtcp, pci_control_new_rt_wind_edge(pci));
        push_pdus_rmt(dtcp);

        q = dt_cwq(dtcp->parent);
        if (!q) {
                LOG_ERR("No Closed Window Queue");
                return -1;
        }
        if (cwq_is_empty(q) &&
            (dt_sv_last_seq_num_sent(dtcp->parent) < snd_rt_wind_edge(dtcp))) {
                dt_sv_window_closed_set(dtcp->parent, false);
        }

        return 0;
}

static int rcv_ack_and_flow_ctl(struct dtcp * dtcp,
                                struct pci *  pci,
                                seq_num_t     seq_num,
                                struct pdu *  pdu)
{
        ASSERT(dtcp);
        ASSERT(pci);
        ASSERT(pdu);

        LOG_DBG("Updating Window Edges");
        snd_rt_wind_edge_set(dtcp, pci_control_new_rt_wind_edge(pci));
        snd_lft_win_set(dtcp, seq_num);
        LOG_DBG("Right Window Edge: %d", snd_rt_wind_edge(dtcp));
        LOG_DBG("Left Window Edge: %d", snd_lft_win(dtcp));

        /* FIXME: Verify values for the receiver side */

        return 0;
}

int dtcp_common_rcv_control(struct dtcp * dtcp, struct pdu * pdu)
{
        struct pci * pci;
        pdu_type_t   type;
        seq_num_t    seq_num;
        seq_num_t    seq;
        seq_num_t    last_ctrl;

        if (!pdu_is_ok(pdu)) {
                LOG_ERR("PDU is not ok");
                pdu_destroy(pdu);
                return -1;
        }

        if (!dtcp) {
                LOG_ERR("DTCP instance bogus");
                pdu_destroy(pdu);
                return -1;
        }

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

        } else if (seq_num > (last_ctrl + 1)) {
                if (dtcp->policies->lost_control_pdu(dtcp))
                        LOG_ERR("Failed lost control PDU policy");
        }
        last_rcv_ctrl_seq_set(dtcp, seq_num);
        last_ctrl = last_rcv_ctrl_seq(dtcp);

        /*
         * FIXME: Missing step described in the specs: retrieve the time
         *        of this Ack and calculate the RTT with RTTEstimator policy
         */

        switch (type) {
        case PDU_TYPE_ACK:
                seq = pci_control_ack_seq_num(pdu_pci_get_ro(pdu));

                return dtcp->policies->sender_ack(dtcp,
                                                  seq);
        case PDU_TYPE_NACK:
                seq = pci_control_ack_seq_num(pdu_pci_get_ro(pdu));

                return rcv_nack_ctl(dtcp,
                                    seq);
        case PDU_TYPE_FC:
                return rcv_flow_ctl(dtcp, pci, pdu);
        case PDU_TYPE_ACK_AND_FC:
                return rcv_ack_and_flow_ctl(dtcp, pci, seq_num, pdu);
        default:
                return -1;
        }
}

static int default_lost_control_pdu(struct dtcp * dtcp)
{
        if (!dtcp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }

#if 0
        struct pdu * pdu_ctrl;
        seq_num_t last_rcv_ctrl, snd_lft, snd_rt;

        last_rcv_ctrl = last_rcv_ctrl_seq(dtcp);
        snd_lft       = snd_lft_win(dtcp);
        snd_rt        = snd_rt_wind_edge(dtcp);
        pdu_ctrl      = pdu_ctrl_ack_create(dtcp,
                                            last_rcv_ctrl,
                                            snd_lft,
                                            snd_rt);
        if (!pdu_ctrl) {
                LOG_ERR("Failed Lost Control PDU policy");
                return -1;
        }

        if (pdu_send(dtcp, pdu_ctrl))
                return -1;
#endif

        LOG_DBG("Default lost control pdu policy");

        return 0;

}

static int default_sending_ack(struct dtcp * dtcp, seq_num_t seq)
{
        struct pdu * pdu_ctrl;
        seq_num_t    last_rcv_ctrl, snd_lft, snd_rt;

        if (!dtcp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }

        last_rcv_ctrl = last_rcv_ctrl_seq(dtcp);
        snd_lft       = snd_lft_win(dtcp);
        snd_rt        = snd_rt_wind_edge(dtcp);
        pdu_ctrl      = pdu_ctrl_ack_create(dtcp,
                                            last_rcv_ctrl,
                                            snd_lft,
                                            snd_rt);
        if (!pdu_ctrl)
                return -1;

        if (pdu_send(dtcp, pdu_ctrl))
                return -1;

        return 0;
}

static int default_rcvr_ack(struct dtcp * dtcp, seq_num_t seq)
{
        if (!dtcp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }

        if (!dt_sv_a(dtcp->parent))
                return dtcp->policies->sending_ack(dtcp, seq);

        return 0;
}

static int default_receiving_flow_control(struct dtcp * dtcp, seq_num_t seq)
{
        if (!dtcp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }

        LOG_MISSING;

        return 0;
}

static seq_num_t update_rt_wind_edge(struct dtcp * dtcp)
{
        seq_num_t seq;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock(&dtcp->sv->lock);
        seq = dtcp->sv->rcvr_credit + dtcp->sv->rcvr_rt_wind_edge;
        dtcp->sv->rcvr_rt_wind_edge = seq;
        spin_unlock(&dtcp->sv->lock);

        return seq;
}

static int default_rcvr_flow_control(struct dtcp * dtcp, seq_num_t seq)
{
        struct pdu * pdu_ctrl;
        seq_num_t    seq_ctl;
        seq_num_t    rt_wind_edge;
        seq_num_t    lf_wind_edge;

        if (!dtcp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }

        /* FIXME: Missing update of right window edge */

        seq_ctl = next_snd_ctl_seq(dtcp);
        rt_wind_edge = update_rt_wind_edge(dtcp);
        lf_wind_edge = dt_sv_rcv_lft_win(dtcp->parent);
        pdu_ctrl = pdu_ctrl_ack_flow(dtcp,
                                     seq_ctl,
                                     seq,
                                     rt_wind_edge,
                                     lf_wind_edge);
        if (!pdu_ctrl) {
                LOG_ERR("ERROR creating PDU for default rcvr flow control");
                return -1;
        }

        if (pdu_send(dtcp, pdu_ctrl)) {
                LOG_ERR("ERROR sending PDU for default rcvr flow control");
                return -1;
        }

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

static int default_sv_update(struct dtcp * dtcp, seq_num_t seq)
{
        int                  retval = 0;
        struct dtcp_config * dtcp_cfg;

        if (!dtcp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }

        dtcp_cfg = dtcp_config_get(dtcp);
        if (!dtcp_cfg)
                return -1;

        if (dtcp_flow_ctrl(dtcp_cfg)) {
                if (dtcp_window_based_fctrl(dtcp_cfg))
                        if (dtcp->policies->rcvr_flow_control(dtcp, seq)) {
                                LOG_ERR("Failed Rcvr Flow Control policy");
                                retval = -1;
                        }

                if (dtcp_rate_based_fctrl(dtcp_cfg)) {
                        LOG_DBG("Rate based fctrl invoked");
                        if (dtcp->policies->rate_reduction(dtcp)) {
                                LOG_ERR("Failed Rate Reduction policy");
                                retval = -1;
                        }
                }
        }

        if (dtcp_rtx_ctrl(dtcp_cfg)) {
                LOG_DBG("Retransmission ctrl invoked");
                if (dtcp->policies->rcvr_ack(dtcp, seq)) {
                        LOG_ERR("Failed Rcvr Ack policy");
                        retval = -1;
                }
        }

        if (dtcp_flow_ctrl(dtcp_cfg) && !dtcp_rtx_ctrl(dtcp_cfg)) {
                LOG_DBG("Receiving flow ctrl invoked");
                if (dtcp->policies->receiving_flow_control(dtcp, seq)) {
                        LOG_ERR("Failed Receiving Flow Control policy");
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
        .flow_control_overrun        = NULL,
        .reconcile_flow_conflict     = NULL,
        .rcvr_ack                    = default_rcvr_ack,
        .rcvr_flow_control           = default_rcvr_flow_control,
        .rate_reduction              = default_rate_reduction,
        .rcvr_control_ack            = NULL,
        .no_rate_slow_down           = NULL,
        .no_override_default_peak    = NULL,
        .receiver_inactivity_timer   = NULL,
        .sender_inactivity_timer     = NULL,
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

struct dtcp * dtcp_create(struct dt *         dt,
                          struct connection * conn,
                          struct rmt *        rmt)
{
        struct dtcp * tmp;

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
        rkfree(instance);

        LOG_DBG("Instance %pK destroyed successfully", instance);

        return 0;
}

int dtcp_send(struct dtcp * instance,
              struct sdu *  sdu)
{
        LOG_MISSING;

        /* Takes the pdu and enqueue in its internal queues */

        return -1;
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
