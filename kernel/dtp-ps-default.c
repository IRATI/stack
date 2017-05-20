/*
 * Default policy set for DTP
 *
 *    Vincenzo Maffione <v.maffione@nextworks.it>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
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

#include <linux/export.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/random.h>

#define RINA_PREFIX "dtp-ps-default"

#include "logs.h"
#include "rds/rmem.h"
#include "rds/rtimer.h"
#include "dtp-ps.h"
#include "dtp.h"
#include "dtcp.h"
#include "dtcp-ps.h"
#include "dtcp-conf-utils.h"
#include "dt-utils.h"
#include "debug.h"

int
default_transmission_control(struct dtp_ps * ps, struct pdu * pdu)
{
        struct dtp *  dtp = ps->dm;
        struct dt  *  dt;

        if (!dtp) {
                LOG_ERR("No instance passed, cannot run policy");
                pdu_destroy(pdu);
                return -1;
        }

        dt = dtp_dt(dtp);
        if (!dt) {
                LOG_ERR("Passed instance has no parent, cannot run policy");
                pdu_destroy(pdu);
                return -1;
        }

        /* Post SDU to RMT */
        LOG_DBG("defaultTxPolicy - sending to rmt");
        if (dtp_sv_max_seq_nr_set(dtp,
                                  pci_sequence_number_get(pdu_pci_get_ro(
                                                          pdu))))
                LOG_ERR("Problems setting max sequence number received "
                        "in default_transmission");

        LOG_DBG("local_soft_irq_pending: %d", local_softirq_pending());

        return dt_pdu_send(dt, dtp_rmt(dtp), pdu);
}

int
default_closed_window(struct dtp_ps * ps, struct pdu * pdu)
{
        struct dtp * dtp = ps->dm;
        struct dtcp * dtcp;
        struct cwq * cwq;
        struct dt *  dt;
        uint_t       max_len;

        if (!dtp) {
                LOG_ERR("No instance passed, cannot run policy");
                pdu_destroy(pdu);
                return -1;
        }
        if (!pdu_is_ok(pdu)) {
                LOG_ERR("PDU is not ok, cannot run policy");
                pdu_destroy(pdu);
                return -1;
        }

        dt = dtp_dt(dtp);
        ASSERT(dt);

        dtcp = dt_dtcp(dt);
        ASSERT(dtcp);

        cwq = dt_cwq(dt);
        if (!cwq) {
                LOG_ERR("Failed to get cwq");
                pdu_destroy(pdu);
                return -1;
        }

        LOG_DBG("Closed Window Queue");

        ASSERT(dtp);

	max_len = dtcp_max_closed_winq_length(dtcp_config_get(dtcp));

	if (max_len != 0 && (cwq_size(cwq) < max_len - 1)) {
		if (cwq_push(cwq, pdu)) {
			LOG_ERR("Failed to push into cwq");
			return -1;
		}

		return 0;
	}

	ASSERT(ps->snd_flow_control_overrun);

	if (ps->snd_flow_control_overrun(ps, pdu)) {
		LOG_ERR("Failed Flow Control Overrun");
		return -1;
	}

	return 0;
}

int
default_snd_flow_control_overrun(struct dtp_ps * ps,
                            struct pdu *    pdu)
{
        struct cwq * cwq;
        struct dt *  dt;
        struct dtp * dtp = ps->dm;

        if (!dtp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }


        dt = dtp_dt(dtp);
        ASSERT(dt);

        cwq = dt_cwq(dt);
        if (!cwq) {
                LOG_ERR("Failed to get cwq");
                pdu_destroy(pdu);
                return -1;
        }

        LOG_DBG("Default Flow Control");

        if (!pdu_is_ok(pdu)) {
                LOG_ERR("PDU is not ok, cannot run policy");
                return -1;
        }

        if (cwq_push(cwq, pdu)) {
                LOG_ERR("Failed to push into cwq");
                return -1;
        }

        LOG_DBG("rbfc Disabling the write on port...");

        if (efcp_disable_write(dt_efcp(dt)))
                return -1;

        return 0;
}

int
default_initial_sequence_number(struct dtp_ps * ps)
{
        struct dtp * dtp = ps->dm;
        seq_num_t    seq_num;

        if (!dtp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }

        get_random_bytes(&seq_num, sizeof(seq_num_t));
        if (seq_num == 0)
                seq_num = 1;
        if (nxt_seq_reset(dtp_dtp_sv(dtp), seq_num))
                return -1;

        LOG_DBG("initial_seq_number reset");
        return 0;
}


int
default_receiver_inactivity_timer(struct dtp_ps * ps)
{
        struct dtp * dtp = ps->dm;
        struct dt *          dt;
        struct dtcp *        dtcp;

        LOG_DBG("default_receiver_inactivity launched");

        if (!dtp) return 0;

        dt = dtp_dt(dtp);
        if (!dt)
                return -1;

        dtcp = dt_dtcp(dt);
        if (!dtcp)
                return -1;

        dtcp_rcv_rt_win_set(dtcp, 0);
        dt_sv_rcv_lft_win_set(dt,0);
        dtp_squeue_flush(dtp);
        dtp_drf_required_set(dtp);

        return 0;
}

int
default_sender_inactivity_timer(struct dtp_ps * ps)
{
        struct dtp *         dtp = ps->dm;
        struct dt *          dt;
        struct dtcp *        dtcp;
        struct dtcp_ps *     dtcp_ps;
        struct dtcp_config * cfg;
        seq_num_t            max_sent, snd_rt_win, init_credit, next_send;

        LOG_DBG("default_sender_inactivity launched");

        if (!dtp) return 0;

        dt = dtp_dt(dtp);
        if (!dt)
                return -1;

        dtcp = dt_dtcp(dt);
        if (!dtp)
                return -1;

        dt_sv_drf_flag_set(dt, true);
        dtp_initial_sequence_number(dtp);

        cfg = dtcp_config_get(dtcp);
        if (!cfg)
                return -1;

        rcu_read_lock();
        dtcp_ps = dtcp_ps_get(dtcp);

        if (dtcp_ps->rtx_ctrl) {
                struct rtxq * q;

                q = dt_rtxq(dt);
                if (!q) {
                        rcu_read_unlock();
                        LOG_ERR("Couldn't find the Retransmission queue");
                        return -1;
                }
                rtxq_flush(q);
        }
        if (dtcp_ps->flow_ctrl) {
                struct cwq * cwq;

                cwq = dt_cwq(dt);
                ASSERT(cwq);
                if (cwq_flush(cwq)) {
                        rcu_read_unlock();
                        LOG_ERR("Coudln't flush cwq");
                        return -1;
                }
                dt_sv_window_closed_set(dt, false);
        }
        rcu_read_unlock();

        init_credit = dtcp_initial_credit(cfg);
        max_sent    = dtp_sv_max_seq_nr_sent(dtp);
        snd_rt_win  = dtcp_snd_rt_win(dtcp);
        next_send   = dtp_sv_last_nxt_seq_nr(dtp);

        LOG_DBG("Current values:\n\tinit_credit: %u "
                "max_sent: %u snd_rt_win: %u next_send: %u",
                init_credit, max_sent, snd_rt_win, next_send);

        dtcp_snd_rt_win_set(dtcp, next_send + init_credit);
        LOG_DBG("Resulting snd_rt_win_edge: %u", dtcp_snd_rt_win(dtcp));

        /*FIXME: Missing sending the control ack pdu */
        return 0;
}

bool default_reconcile_flow_conflict(struct dtp_ps * ps)
{
	LOG_DBG("Reconciling window and rate flow controls...");
        return true;
}

struct ps_base *
dtp_ps_default_create(struct rina_component * component)
{
        struct dtp * dtp = dtp_from_component(component);
        struct dtp_ps * ps = rkzalloc(sizeof(*ps), GFP_KERNEL);

        if (!ps) {
                return NULL;
        }

        ps->base.set_policy_set_param = NULL;
        ps->dm              = dtp;
        ps->priv            = NULL;

        ps->transmission_control        = default_transmission_control;
        ps->closed_window               = default_closed_window;
        ps->snd_flow_control_overrun    = default_snd_flow_control_overrun;
        ps->rcv_flow_control_overrun    = NULL;
        ps->initial_sequence_number     = default_initial_sequence_number;
        ps->receiver_inactivity_timer   = default_receiver_inactivity_timer;
        ps->sender_inactivity_timer     = default_sender_inactivity_timer;
        ps->reconcile_flow_conflict     = default_reconcile_flow_conflict;

        /* Just zero here. These fields are really initialized by
         * dtp_select_policy_set. */
        ps->dtcp_present = 0;
        ps->seq_num_ro_th = 0;
        ps->initial_a_timer = 0;
        ps->partial_delivery = 0;
        ps->incomplete_delivery = 0;
        ps->in_order_delivery = 0;
        ps->max_sdu_gap = 0;

        return &ps->base;
}
EXPORT_SYMBOL(dtp_ps_default_create);

void dtp_ps_default_destroy(struct ps_base * bps)
{
        struct dtp_ps *ps = container_of(bps, struct dtp_ps, base);

        if (bps) {
                rkfree(ps);
        }
}
EXPORT_SYMBOL(dtp_ps_default_destroy);
