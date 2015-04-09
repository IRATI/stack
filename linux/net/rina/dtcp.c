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

#include <linux/delay.h>

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "dtcp.h"
#include "rmt.h"
#include "connection.h"
#include "dtp.h"
#include "dt-utils.h"
#include "dtcp-utils.h"
#include "ps-factory.h"
#include "dtcp-ps.h"

static struct policy_set_list policy_sets = {
        .head = LIST_HEAD_INIT(policy_sets.head)
};

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

struct dtcp {
        struct dt *            parent;

        /*
         * NOTE: The DTCP State Vector can be discarded during long periods of
         *       no traffic
         */
        struct dtcp_sv *       sv; /* The state-vector */
        struct rina_component  base;
        struct connection *    conn;
        struct rmt *           rmt;

        atomic_t               cpdus_in_transit;
};

struct dt * dtcp_dt(struct dtcp * dtcp)
{
        return dtcp->parent;
}
EXPORT_SYMBOL(dtcp_dt);

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

int dtcp_pdu_send(struct dtcp * dtcp, struct pdu * pdu)
{
        struct efcp * efcp;

        ASSERT(dtcp);
        ASSERT(dtcp->parent);

        efcp = dt_efcp(dtcp->parent);
        if (!efcp) {
                LOG_ERR("Passed instance has no EFCP, dtcp_pdu_send bailing out");
                pdu_destroy(pdu);
                return -1;
        }

        return common_efcp_pdu_send(efcp,
        			    dtcp->rmt,
        			    dtcp->conn->destination_address,
        			    dtcp->conn->qos_id,
        			    pdu);
}
EXPORT_SYMBOL(dtcp_pdu_send);

static int last_rcv_ctrl_seq_set(struct dtcp * dtcp,
                                 seq_num_t     last_rcv_ctrl_seq)
{
        unsigned long flags;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        dtcp->sv->last_rcv_ctl_seq = last_rcv_ctrl_seq;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);

        return 0;
}

seq_num_t last_rcv_ctrl_seq(struct dtcp * dtcp)
{
        seq_num_t     tmp;
        unsigned long flags;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        tmp = dtcp->sv->last_rcv_ctl_seq;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);

        return tmp;
}
EXPORT_SYMBOL(last_rcv_ctrl_seq);

static void flow_ctrl_inc(struct dtcp * dtcp)
{
        unsigned long flags;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        dtcp->sv->flow_ctl++;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);
}

static void acks_inc(struct dtcp * dtcp)
{
        unsigned long flags;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        dtcp->sv->acks++;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);
}

static int snd_rt_wind_edge_set(struct dtcp * dtcp, seq_num_t new_rt_win)
{
        unsigned long flags;
        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        dtcp->sv->snd_rt_wind_edge = new_rt_win;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);

        return 0;
}

seq_num_t snd_rt_wind_edge(struct dtcp * dtcp)
{
        seq_num_t     tmp;
        unsigned long flags;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        tmp = dtcp->sv->snd_rt_wind_edge;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);

        return tmp;
}
EXPORT_SYMBOL(snd_rt_wind_edge);

seq_num_t snd_lft_win(struct dtcp * dtcp)
{
        seq_num_t     tmp;
        unsigned long flags;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        tmp = dtcp->sv->snd_lft_win;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);

        return tmp;
}
EXPORT_SYMBOL(snd_lft_win);

seq_num_t rcvr_rt_wind_edge(struct dtcp * dtcp)
{
        unsigned long flags;
        seq_num_t     tmp;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        tmp = dtcp->sv->rcvr_rt_wind_edge;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);

        return tmp;
}
EXPORT_SYMBOL(rcvr_rt_wind_edge);

static seq_num_t next_snd_ctl_seq(struct dtcp * dtcp)
{
        seq_num_t     tmp;
        unsigned long flags;


        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        tmp = ++dtcp->sv->next_snd_ctl_seq;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);

        return tmp;
}

static seq_num_t last_snd_data_ack(struct dtcp * dtcp)
{
        seq_num_t     tmp;
        unsigned long flags;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        tmp = dtcp->sv->last_snd_data_ack;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);

        return tmp;
}

static void last_snd_data_ack_set(struct dtcp * dtcp, seq_num_t seq_num)
{
        unsigned long flags;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        spin_lock_irqsave(&dtcp->sv->lock, flags);
        dtcp->sv->last_snd_data_ack = seq_num;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);
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

struct pdu * pdu_ctrl_create_ni(struct dtcp * dtcp, pdu_type_t type)
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
EXPORT_SYMBOL(pdu_ctrl_create_ni);

static int populate_ctrl_pci(struct pci *  pci,
                             struct dtcp * dtcp)
{
        struct dtcp_config * dtcp_cfg;
        seq_num_t snd_lft;
        seq_num_t snd_rt;
        seq_num_t LWE;

        dtcp_cfg = dtcp_config_get(dtcp);
        if (!dtcp_cfg) {
                LOG_ERR("No dtcp cfg...");
                return -1;
        }

        /*
         * FIXME: Shouldn't we check if PDU_TYPE_ACK_AND_FC or
         * PDU_TYPE_NACK_AND_FC ?
         */
        LWE = dt_sv_rcv_lft_win(dtcp->parent);
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

struct pdu * pdu_ctrl_generate(struct dtcp * dtcp, pdu_type_t type)
{
        struct pdu * pdu;
        struct pci * pci;

        if (!dtcp || !type) {
                LOG_ERR("wrong parameters, can't generate ctrl PDU...");
                return NULL;
        }
        pdu  = pdu_ctrl_create_ni(dtcp, type);
        if (!pdu) {
                LOG_ERR("No Ctrl PDU created...");
                return NULL;
        }

        pci = pdu_pci_get_rw(pdu);
        if (populate_ctrl_pci(pci, dtcp)) {
                LOG_ERR("Could not populate ctrl PCI");
                pdu_destroy(pdu);
                return NULL;
        }

        return pdu;
}
EXPORT_SYMBOL(pdu_ctrl_generate);

void dump_we(struct dtcp * dtcp, struct pci *  pci)
{
        struct dtp * dtp;
        seq_num_t    snd_rt_we;
        seq_num_t    snd_lf_we;
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
        rcv_rt_we = rcvr_rt_wind_edge(dtcp);
        rcv_lf_we = dt_sv_rcv_lft_win(dtcp->parent);
        new_rt_we = pci_control_new_rt_wind_edge(pci);
        new_lf_we = pci_control_new_left_wind_edge(pci);
        my_lf_we  = pci_control_my_left_wind_edge(pci);
        my_rt_we  = pci_control_my_rt_wind_edge(pci);
        pci_seqn  = pci_sequence_number_get(pci);
        ack       = pci_control_ack_seq_num(pci);

        LOG_DBG("SEQN: %u N/Ack: %u SndRWE: %u SndLWE: %u "
                "RcvRWE: %u RcvLWE: %u "
                "newRWE: %u newLWE: %u "
                "myRWE: %u myLWE: %u",
                pci_seqn, ack, snd_rt_we, snd_lf_we, rcv_rt_we, rcv_lf_we,
                new_rt_we, new_lf_we, my_rt_we, my_lf_we);
}
EXPORT_SYMBOL(dump_we);

/* not a policy according to specs */
static int rcv_nack_ctl(struct dtcp * dtcp,
                        struct pdu *  pdu)
{
        struct rtxq *    q;
        struct dtcp_ps * ps;
        seq_num_t        seq_num;
        struct pci *     pci;

        pci     = pdu_pci_get_rw(pdu);
        seq_num = pci_control_ack_seq_num(pci);

        rcu_read_lock();
        ps = container_of(rcu_dereference(dtcp->base.ps),
                          struct dtcp_ps, base);

        if (ps->rtx_ctrl) {
                q = dt_rtxq(dtcp->parent);
                if (!q) {
                        rcu_read_unlock();
                        LOG_ERR("Couldn't find the Retransmission queue");
                        return -1;
                }
                rtxq_nack(q, seq_num, dt_sv_tr(dtcp->parent));
        }
        rcu_read_unlock();

        LOG_DBG("DTCP received NACK (CPU: %d)", smp_processor_id());
        dump_we(dtcp, pci);

        pdu_destroy(pdu);
        return 0;
}

static int rcv_ack(struct dtcp * dtcp,
                   struct pdu *  pdu)
{
        struct dtcp_ps * ps;
        seq_num_t        seq;
        int              ret;
        struct pci *     pci;

        ASSERT(dtcp);
        ASSERT(pdu);

        pci = pdu_pci_get_rw(pdu);
        ASSERT(pci);

        seq = pci_control_ack_seq_num(pci);

        rcu_read_lock();
        ps = container_of(rcu_dereference(dtcp->base.ps),
                          struct dtcp_ps, base);
        ret = ps->sender_ack(ps, seq);
        rcu_read_unlock();

        LOG_DBG("DTCP received ACK (CPU: %d)", smp_processor_id());
        dump_we(dtcp, pci);

        pdu_destroy(pdu);
        return ret;
}

static int rcv_flow_ctl(struct dtcp * dtcp,
                        struct pdu *  pdu)
{
        struct cwq * q;
        struct dtp * dtp;
        struct pci * pci;

        ASSERT(dtcp);
        ASSERT(pdu);

        pci = pdu_pci_get_rw(pdu);
        ASSERT(pci);

        snd_rt_wind_edge_set(dtcp, pci_control_new_rt_wind_edge(pci));
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
            (dtp_sv_max_seq_nr_sent(dtp) < snd_rt_wind_edge(dtcp))) {
                dt_sv_window_closed_set(dtcp->parent, false);
        }

        LOG_DBG("DTCP received FC (CPU: %d)", smp_processor_id());
        dump_we(dtcp, pci);

        pdu_destroy(pdu);
        return 0;
}

static int rcv_ack_and_flow_ctl(struct dtcp * dtcp,
                                struct pdu *  pdu)
{
        struct dtcp_ps * ps;
        seq_num_t        seq;
        struct pci *     pci;

        ASSERT(dtcp);

        pci = pdu_pci_get_rw(pdu);
        ASSERT(pci);

        LOG_DBG("Updating Window Edges for DTCP: %pK", dtcp);

        seq = pci_control_ack_seq_num(pci);

        rcu_read_lock();
        ps = container_of(rcu_dereference(dtcp->base.ps),
                          struct dtcp_ps, base);
        /* This updates sender LWE */
        if (ps->sender_ack(ps, seq))
                LOG_ERR("Could not update RTXQ and LWE");
        rcu_read_unlock();

        snd_rt_wind_edge_set(dtcp, pci_control_new_rt_wind_edge(pci));
        LOG_DBG("Right Window Edge: %d", snd_rt_wind_edge(dtcp));

        LOG_DBG("Calling CWQ_deliver for DTCP: %pK", dtcp);
        push_pdus_rmt(dtcp);

        /* FIXME: Verify values for the receiver side */
        LOG_DBG("DTCP received ACK-FC (CPU: %d)", smp_processor_id());
        dump_we(dtcp, pci);

        pdu_destroy(pdu);
        return 0;
}

int dtcp_common_rcv_control(struct dtcp * dtcp, struct pdu * pdu)
{
        struct dtcp_ps * ps;
        struct pci *     pci;
        pdu_type_t       type;
        seq_num_t        seq_num;
        seq_num_t        last_ctrl;
        int              ret;

        LOG_DBG("dtcp_common_rcv_control called");

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

        atomic_inc(&dtcp->cpdus_in_transit);

        pci = pdu_pci_get_rw(pdu);
        if (!pci_is_ok(pci)) {
                LOG_ERR("PCI couldn't be retrieved");
                atomic_dec(&dtcp->cpdus_in_transit);
                pdu_destroy(pdu);
                return -1;
        }

        type = pci_type(pci);

        if (!pdu_type_is_control(type)) {
                LOG_ERR("CommonRCVControl policy received a non-control PDU");
                atomic_dec(&dtcp->cpdus_in_transit);
                pdu_destroy(pdu);
                return -1;
        }

        seq_num = pci_sequence_number_get(pci);
        last_ctrl = last_rcv_ctrl_seq(dtcp);

        if (seq_num > (last_ctrl + 1)) {
                rcu_read_lock();
                ps = container_of(rcu_dereference(dtcp->base.ps),
                                  struct dtcp_ps, base);
                ps->lost_control_pdu(ps);
                rcu_read_unlock();
        }

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
                ret = rcv_ack(dtcp, pdu);
                break;
        case PDU_TYPE_NACK:
                ret = rcv_nack_ctl(dtcp, pdu);
                break;
        case PDU_TYPE_FC:
                ret = rcv_flow_ctl(dtcp, pdu);
                break;
        case PDU_TYPE_ACK_AND_FC:
                ret = rcv_ack_and_flow_ctl(dtcp, pdu);
                break;
        default:
                ret = -1;
                break;
        }

        atomic_dec(&dtcp->cpdus_in_transit);
        return ret;
}

/*FIXME: wrapper to be called by dtp in the post_worker */
int dtcp_sending_ack_policy(struct dtcp * dtcp)
{
        struct dtcp_ps *ps;

        if (!dtcp) {
                LOG_ERR("No DTCP passed...");
                return -1;
        }

        rcu_read_lock();
        ps = container_of(rcu_dereference(dtcp->base.ps),
                          struct dtcp_ps, base);
        rcu_read_unlock();

        return ps->sending_ack(ps, 0);
}

pdu_type_t pdu_ctrl_type_get(struct dtcp * dtcp, seq_num_t seq)
{
        struct dtcp_config * dtcp_cfg;
        struct dtcp_ps *ps;
        bool flow_ctrl;
        seq_num_t    LWE;
        timeout_t    a;

        if (!dtcp) {
                LOG_ERR("No DTCP passed...");
                return -1;
        }

        if (!dtcp->parent) {
                LOG_ERR("No DTCP parent passed...");
                return -1;
        }

        dtcp_cfg = dtcp_config_get(dtcp);
        ASSERT(dtcp_cfg);

        rcu_read_lock();
        ps = container_of(rcu_dereference(dtcp->base.ps),
                          struct dtcp_ps, base);
        flow_ctrl = ps->flow_ctrl;
        rcu_read_unlock();

        a = dt_sv_a(dtcp->parent);

        LWE = dt_sv_rcv_lft_win(dtcp->parent);
        if (last_snd_data_ack(dtcp) < LWE) {
                last_snd_data_ack_set(dtcp, LWE);
                if (!a) {
#if 0
                        if (seq > LWE) {
                                LOG_DBG("This is a NACK, "
                                        "LWE couldn't be updated");
                                if (flow_ctrl) {
                                        return PDU_TYPE_NACK_AND_FC;
                                }
                                return PDU_TYPE_NACK;
                        }
#endif
                        LOG_DBG("This is an ACK");
                        if (flow_ctrl) {
                                return PDU_TYPE_ACK_AND_FC;
                        }
                        return PDU_TYPE_ACK;
                }
#if 0
                if (seq > LWE) {
                        /* FIXME: This should be a SEL ACK */
                        LOG_DBG("This is a NACK, "
                                "LWE couldn't be updated");
                        if (flow_ctrl) {
                                return PDU_TYPE_NACK_AND_FC;
                        }
                        return PDU_TYPE_NACK;
                }
#endif
                LOG_DBG("This is an ACK");
                if (flow_ctrl) {
                        return PDU_TYPE_ACK_AND_FC;
                }
                return PDU_TYPE_ACK;
        }

        return 0;
}
EXPORT_SYMBOL(pdu_ctrl_type_get);

int dtcp_ack_flow_control_pdu_send(struct dtcp * dtcp, seq_num_t seq)
{
        struct pdu *   pdu;
        pdu_type_t     type;

        seq_num_t      dbg_seq_num;

        if (!dtcp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }

        atomic_inc(&dtcp->cpdus_in_transit);

        type = pdu_ctrl_type_get(dtcp, seq);
        if (!type) {
                atomic_dec(&dtcp->cpdus_in_transit);
                return 0;
        }

        pdu  = pdu_ctrl_generate(dtcp, type);
        if (!pdu) {
                atomic_dec(&dtcp->cpdus_in_transit);
                return -1;
        }

        dbg_seq_num = pci_sequence_number_get(pdu_pci_get_rw(pdu));

        LOG_DBG("DTCP Sending ACK (CPU: %d)", smp_processor_id());
        dump_we(dtcp, pdu_pci_get_rw(pdu));

        if (dtcp_pdu_send(dtcp, pdu)){
                atomic_dec(&dtcp->cpdus_in_transit);
                return -1;
        }

        atomic_dec(&dtcp->cpdus_in_transit);

        return 0;
}
EXPORT_SYMBOL(dtcp_ack_flow_control_pdu_send);

void update_rt_wind_edge(struct dtcp * dtcp)
{
        seq_num_t     seq;
        unsigned long flags;

        ASSERT(dtcp);
        ASSERT(dtcp->sv);

        seq = dt_sv_rcv_lft_win(dtcp->parent);
        spin_lock_irqsave(&dtcp->sv->lock, flags);
        seq += dtcp->sv->rcvr_credit;
        dtcp->sv->rcvr_rt_wind_edge = seq;
        spin_unlock_irqrestore(&dtcp->sv->lock, flags);
}
EXPORT_SYMBOL(update_rt_wind_edge);

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

/* FIXME: this should be completed with other parameters from the config */
static int dtcp_sv_init(struct dtcp * instance, struct dtcp_sv sv)
{
        struct dtcp_config * cfg;
        struct dtcp_ps * ps;

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

        rcu_read_lock();
        ps = container_of(rcu_dereference(instance->base.ps),
                          struct dtcp_ps, base);
        if (ps->rtx_ctrl)
                instance->sv->data_retransmit_max =
                        ps->rtx.data_retransmit_max;

        instance->sv->sndr_credit         = ps->flowctrl.window.initial_credit;
        instance->sv->snd_rt_wind_edge    = ps->flowctrl.window.initial_credit +
                        dtp_sv_last_nxt_seq_nr(dt_dtp(instance->parent));
        instance->sv->rcvr_credit         = ps->flowctrl.window.initial_credit;
        instance->sv->rcvr_rt_wind_edge   = ps->flowctrl.window.initial_credit;
        rcu_read_unlock();

        LOG_DBG("DTCP SV initialized with dtcp_conf:");
        LOG_DBG("  data_retransmit_max: %d",
                instance->sv->data_retransmit_max);
        LOG_DBG("  sndr_credit:         %u",
                instance->sv->sndr_credit);
        LOG_DBG("  snd_rt_wind_edge:    %u",
                instance->sv->snd_rt_wind_edge);
        LOG_DBG("  rcvr_credit:         %u",
                instance->sv->rcvr_credit);
        LOG_DBG("  rcvr_rt_wind_edge:   %u",
                instance->sv->rcvr_rt_wind_edge);

        return 0;
}

struct dtcp_ps * dtcp_ps_get(struct dtcp * dtcp)
{
        if (!dtcp) {
                LOG_ERR("Could not retrieve DTCP PS, NULL instance passed");
                return NULL;
        }

        return container_of(rcu_dereference(dtcp->base.ps),
                            struct dtcp_ps, base);
}
EXPORT_SYMBOL(dtcp_ps_get);

struct dtcp *
dtcp_from_component(struct rina_component * component)
{
        return container_of(component, struct dtcp, base);
}
EXPORT_SYMBOL(dtcp_from_component);

int dtcp_select_policy_set(struct dtcp * dtcp,
                           const string_t * path,
                           const string_t * name)
{
        struct dtcp_config * cfg = dtcp->conn->policies_params->dtcp_cfg;
        struct dtcp_ps *ps;
        int ret;

        if (path && strcmp(path, "")) {
                LOG_ERR("This component has no selectable subcomponents");
                return -1;
        }

        ret = base_select_policy_set(&dtcp->base, &policy_sets, name);
        if (ret) {
                return ret;
        }

        /* Copy the connection parameter to the policy-set. From now on
         * these connection parameters must be accessed by the DTCP policy set,
         * and not from the struct connection. */
        mutex_lock(&dtcp->base.ps_lock);
        ps = container_of(dtcp->base.ps, struct dtcp_ps, base);
        ps->flow_ctrl                   = dtcp_flow_ctrl(cfg);
        ps->rtx_ctrl                    = dtcp_rtx_ctrl(cfg);
        ps->flowctrl.window_based       = dtcp_window_based_fctrl(cfg);
        ps->flowctrl.rate_based         = dtcp_rate_based_fctrl(cfg);
        ps->flowctrl.sent_bytes_th      = dtcp_sent_bytes_th(cfg);
        ps->flowctrl.sent_bytes_percent_th
                                        = dtcp_sent_bytes_percent_th(cfg);
        ps->flowctrl.sent_buffers_th    = dtcp_sent_buffers_th(cfg);
        ps->flowctrl.rcvd_bytes_th      = dtcp_rcvd_bytes_th(cfg);
        ps->flowctrl.rcvd_bytes_percent_th
                                        = dtcp_rcvd_bytes_percent_th(cfg);
        ps->flowctrl.rcvd_buffers_th    = dtcp_rcvd_buffers_th(cfg);
        ps->rtx.max_time_retry          = dtcp_max_time_retry(cfg);
        ps->rtx.data_retransmit_max     = dtcp_data_retransmit_max(cfg);
        ps->rtx.initial_tr              = dtcp_initial_tr(cfg);
        ps->flowctrl.window.max_closed_winq_length
                                        = dtcp_max_closed_winq_length(cfg);
        ps->flowctrl.window.initial_credit
                                        = dtcp_initial_credit(cfg);
        ps->flowctrl.rate.sending_rate  = dtcp_sending_rate(cfg);
        ps->flowctrl.rate.time_period   = dtcp_time_period(cfg);
        mutex_unlock(&dtcp->base.ps_lock);

        return ret;
}
EXPORT_SYMBOL(dtcp_select_policy_set);

int dtcp_set_policy_set_param(struct dtcp * dtcp,
                              const char * path,
                              const char * name,
                              const char * value)
{
        struct dtcp_ps *ps;
        int ret = -1;

        if (!dtcp|| !path || !name || !value) {
                LOG_ERRF("NULL arguments %p %p %p %p",
                         dtcp, path, name, value);
                return -1;
        }

        LOG_DBG("set-policy-set-param '%s' '%s' '%s'", path, name, value);

        if (strcmp(path, "") == 0) {
                int bool_value;

                /* The request addresses this DTP instance. */
                rcu_read_lock();
                ps = container_of(rcu_dereference(dtcp->base.ps),
                                  struct dtcp_ps,
                                  base);
                if (strcmp(name, "flow_ctrl") == 0) {
                        ret = kstrtoint(value, 10, &bool_value);
                        if (ret == 0) {
                                ps->flow_ctrl = bool_value;
                        }
                } else if (strcmp(name, "rtx_ctrl") == 0) {
                        ret = kstrtoint(value, 10, &bool_value);
                        if (ret == 0) {
                                ps->rtx_ctrl = bool_value;
                        }
                } else if (strcmp(name, "flowctrl.window_based") == 0) {
                        ret = kstrtoint(value, 10, &bool_value);
                        if (ret == 0) {
                                ps->flowctrl.window_based = bool_value;
                        }
                } else if (strcmp(name, "flowctrl.rate_based") == 0) {
                        ret = kstrtoint(value, 10, &bool_value);
                        if (ret == 0) {
                                ps->flowctrl.rate_based = bool_value;
                        }
                } else if (strcmp(name, "flowctrl.sent_bytes_th") == 0) {
                        ret = kstrtouint(value, 10,
                                         &ps->flowctrl.sent_bytes_th);
                } else if (strcmp(name, "flowctrl.sent_bytes_percent_th")
                                == 0) {
                        ret = kstrtouint(value, 10,
                                         &ps->flowctrl.sent_bytes_percent_th);
                } else if (strcmp(name, "flowctrl.sent_buffers_th") == 0) {
                        ret = kstrtouint(value, 10,
                                         &ps->flowctrl.sent_buffers_th);
                } else if (strcmp(name, "flowctrl.rcvd_bytes_th") == 0) {
                        ret = kstrtouint(value, 10,
                                         &ps->flowctrl.rcvd_bytes_th);
                } else if (strcmp(name, "flowctrl.rcvd_bytes_percent_th")
                                == 0) {
                        ret = kstrtouint(value, 10,
                                         &ps->flowctrl.rcvd_bytes_percent_th);
                } else if (strcmp(name, "flowctrl.rcvd_buffers_th") == 0) {
                        ret = kstrtouint(value, 10,
                                         &ps->flowctrl.rcvd_buffers_th);
                } else if (strcmp(name, "rtx.max_time_retry") == 0) {
                        ret = kstrtouint(value, 10,
                                         &ps->rtx.max_time_retry);
                } else if (strcmp(name, "rtx.data_retransmit_max") == 0) {
                        ret = kstrtouint(value, 10,
                                         &ps->rtx.data_retransmit_max);
                } else if (strcmp(name, "rtx.initial_tr") == 0) {
                        ret = kstrtouint(value, 10, &ps->rtx.initial_tr);
                } else if (strcmp(name,
                                "flowctrl.window.max_closed_winq_length")
                                        == 0) {
                        ret = kstrtouint(value, 10,
                                &ps->flowctrl.window.max_closed_winq_length);
                } else if (strcmp(name, "flowctrl.window.initial_credit")
                                == 0) {
                        ret = kstrtouint(value, 10,
                                &ps->flowctrl.window.initial_credit);
                } else if (strcmp(name, "flowctrl.rate.sending_rate") == 0) {
                        ret = kstrtouint(value, 10,
                                         &ps->flowctrl.rate.sending_rate);
                } else if (strcmp(name, "flowctrl.rate.time_period") == 0) {
                        ret = kstrtouint(value, 10,
                                         &ps->flowctrl.rate.time_period);
                } else {
                        LOG_ERR("Unknown DTP parameter policy '%s'", name);
                }
                rcu_read_unlock();
        } else {
                ret = base_set_policy_set_param(&dtcp->base, path, name, value);
        }

        return ret;
}
EXPORT_SYMBOL(dtcp_set_policy_set_param);

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

        tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
        if (!tmp) {
                LOG_ERR("Cannot create DTCP state-vector");
                return NULL;
        }

        tmp->parent = dt;

        tmp->sv = rkzalloc(sizeof(*tmp->sv), GFP_ATOMIC);
        if (!tmp->sv) {
                LOG_ERR("Cannot create DTCP state-vector");
                dtcp_destroy(tmp);
                return NULL;
        }

        tmp->conn = conn;
        tmp->rmt  = rmt;
        atomic_set(&tmp->cpdus_in_transit, 0);

        rina_component_init(&tmp->base);

        /* Try to select the default policy-set. */
        if (dtcp_select_policy_set(tmp, "", RINA_PS_DEFAULT_NAME)) {
                dtcp_destroy(tmp);
                return NULL;
        }

        if (dtcp_sv_init(tmp, default_sv)) {
                LOG_ERR("Could not load DTCP config in the SV");
                dtcp_destroy(tmp);
                return NULL;
        }
        /* FIXME: fixups to the state-vector should be placed here */

        LOG_DBG("Instance %pK created successfully", tmp);

        return tmp;
}

int dtcp_destroy(struct dtcp * instance)
{
        /* FIXME: this is horrible*/
        while(atomic_read(&instance->cpdus_in_transit))
                msleep(20);

        if (!instance) {
                LOG_ERR("Bad instance passed, bailing out");
                return -1;
        }

        if (instance->sv)       rkfree(instance->sv);
        rina_component_fini(&instance->base);
        rkfree(instance);

        LOG_DBG("Instance %pK destroyed successfully", instance);

        return 0;
}

int dtcp_sv_update(struct dtcp * instance,
                   seq_num_t     seq)
{
        struct dtcp_ps * ps;

        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        rcu_read_lock();
        ps = container_of(rcu_dereference(instance->base.ps),
                          struct dtcp_ps, base);

        ASSERT(ps);
        ASSERT(ps->sv_update);

        if (ps->sv_update(ps, seq)) {
                rcu_read_unlock();
                return -1;
        }

        rcu_read_unlock();

        return 0;
}

seq_num_t dtcp_rcv_rt_win(struct dtcp * dtcp)
{
        if (!dtcp || !dtcp->sv)
                return -1;

        return rcvr_rt_wind_edge(dtcp);
}

seq_num_t dtcp_snd_rt_win(struct dtcp * dtcp)
{
        if (!dtcp || !dtcp->sv)
                return -1;

        return snd_rt_wind_edge(dtcp);
}
EXPORT_SYMBOL(dtcp_snd_rt_win);

int dtcp_snd_rt_win_set(struct dtcp * dtcp, seq_num_t rt_win_edge)
{
        if (!dtcp || !dtcp->sv)
                return -1;

        return snd_rt_wind_edge_set(dtcp, rt_win_edge);
}
EXPORT_SYMBOL(dtcp_snd_rt_win_set);

seq_num_t dtcp_snd_lf_win(struct dtcp * dtcp)
{
        if (!dtcp || !dtcp->sv)
                return -1;

        return snd_lft_win(dtcp);
}

int dtcp_snd_lf_win_set(struct dtcp * instance, seq_num_t seq_num)
{
        unsigned long flags;

        if (!instance || !instance->sv)
                return -1;

        spin_lock_irqsave(&instance->sv->lock, flags);
        instance->sv->snd_lft_win = seq_num;
        spin_unlock_irqrestore(&instance->sv->lock, flags);

        return 0;
}
EXPORT_SYMBOL(dtcp_snd_lf_win_set);

int dtcp_rcv_rt_win_set(struct dtcp * instance, seq_num_t seq_num)
{
        unsigned long flags;

        if (!instance || !instance->sv)
                return -1;

        spin_lock_irqsave(&instance->sv->lock, flags);
        instance->sv->rcvr_rt_wind_edge = seq_num;
        spin_unlock_irqrestore(&instance->sv->lock, flags);

        return 0;
}
EXPORT_SYMBOL(dtcp_rcv_rt_win_set);

int dtcp_ps_publish(struct ps_factory * factory)
{
        if (factory == NULL) {
                LOG_ERR("%s: NULL factory", __func__);
                return -1;
        }

        return ps_publish(&policy_sets, factory);
}
EXPORT_SYMBOL(dtcp_ps_publish);

int dtcp_ps_unpublish(const char * name)
{ return ps_unpublish(&policy_sets, name); }
EXPORT_SYMBOL(dtcp_ps_unpublish);

