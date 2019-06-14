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
#include "dtp-utils.h"
#include "debug.h"

int default_transmission_control(struct dtp_ps * ps, struct du * du)
{
        struct dtp *  dtp;
        struct dtcp * dtcp;

        dtp = ps->dm;
        if (!dtp) {
                LOG_ERR("No instance passed, cannot run policy");
                du_destroy(du);
                return -1;
        }
        dtcp = dtp->dtcp;

        /* Post SDU to RMT */
        LOG_DBG("defaultTxPolicy - sending to rmt");

        spin_lock_bh(&dtp->sv_lock);
        dtp->sv->max_seq_nr_sent = pci_sequence_number_get(&du->pci);
        dtcp->sv->snd_lft_win = dtp->sv->max_seq_nr_sent;
        spin_unlock_bh(&dtp->sv_lock);

        LOG_DBG("local_soft_irq_pending: %d", local_softirq_pending());

        return dtp_pdu_send(dtp, dtp->rmt, du);
}

int default_closed_window(struct dtp_ps * ps, struct du * du)
{
        struct dtp * dtp = ps->dm;
        struct dtcp * dtcp;
        struct cwq * cwq;
        uint_t       max_len;

        if (!dtp) {
                LOG_ERR("No instance passed, cannot run policy");
                du_destroy(du);
                return -1;
        }

        dtcp = dtp->dtcp;
        cwq = dtp->cwq;

        LOG_DBG("Closed Window Queue");

	max_len = dtcp_max_closed_winq_length(dtcp->cfg);

	if (max_len != 0 && (cwq_size(cwq) < max_len - 1)) {
		if (cwq_push(cwq, du)) {
			LOG_ERR("Failed to push into cwq");
			return -1;
		}

		return 0;
	}

	ASSERT(ps->snd_flow_control_overrun);

	if (ps->snd_flow_control_overrun(ps, du)) {
		LOG_ERR("Failed Flow Control Overrun");
		return -1;
	}

	return 0;
}

int default_snd_flow_control_overrun(struct dtp_ps * ps, struct du * du)
{
        struct dtp * dtp = ps->dm;

        if (!dtp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }

        LOG_DBG("Default Flow Control");

        if (cwq_push(dtp->cwq, du)) {
                LOG_ERR("Failed to push into cwq");
                return -1;
        }

        LOG_DBG("rbfc Disabling the write on port...");

        if (efcp_disable_write(dtp->efcp))
                return -1;

        return 0;
}

static void initial_seq_num(struct dtp * dtp)
{
        seq_num_t    seq_num;

        get_random_bytes(&seq_num, sizeof(seq_num_t));
        if (seq_num == 0)
                seq_num = 1;

        spin_lock_bh(&dtp->sv_lock);
        dtp->sv->seq_nr_to_send = seq_num;
        spin_unlock_bh(&dtp->sv_lock);

        LOG_DBG("initial_seq_number reset");
}

int default_initial_sequence_number(struct dtp_ps * ps)
{
        struct dtp * dtp = ps->dm;

        if (!dtp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }

        initial_seq_num(dtp);

        return 0;
}

int default_receiver_inactivity_timer(struct dtp_ps * ps)
{
        struct dtp * dtp = ps->dm;
        struct dtcp *        dtcp;

        LOG_DBG("default_receiver_inactivity launched");

        if (!dtp) return 0;

        dtcp = dtp->dtcp;
        if (!dtcp)
                return -1;

        spin_lock_bh(&dtp->sv_lock);
        dtcp->sv->rcvr_rt_wind_edge = 0;
        dtp->sv->rcv_left_window_edge = 0;
        dtp_squeue_flush(dtp);
        dtcp->sv->rendezvous_rcvr = false;
        dtp->sv->drf_required = true;
        spin_unlock_bh(&dtp->sv_lock);

        return 0;
}

int default_sender_inactivity_timer(struct dtp_ps * ps)
{
        struct dtp *         dtp = ps->dm;
        struct dtcp *        dtcp;
        struct dtcp_ps *     dtcp_ps;
        seq_num_t            max_sent, snd_rt_win, init_credit, next_send;

        if (!dtp) return 0;

        LOG_DBG("DTP %pK, STime launched", dtp);

        dtcp = dtp->dtcp;
        if (!dtcp) {
        	LOG_ERR("No DTCP to work with");
                return -1;
        }
        if (!dtcp->cfg) {
        	LOG_ERR("No configuration in DTCP");
        	return -1;
        }

        initial_seq_num(dtp);

        spin_lock_bh(&dtp->sv_lock);

        dtp->sv->drf_flag = true;
        dtp->sv->window_closed = false;
        init_credit = dtcp_initial_credit(dtcp->cfg);
        max_sent    = dtp->sv->max_seq_nr_sent;
        snd_rt_win  = dtcp->sv->snd_rt_wind_edge;
        next_send   = dtp->sv->seq_nr_to_send;
        dtcp->sv->snd_rt_wind_edge = next_send + init_credit;
        dtcp->sv->rendezvous_sndr = false;
        if (dtp->rttq) {
        	rttq_flush(dtp->rttq);
        }

        LOG_DBG("Current values:\n\tinit_credit: %u "
                "max_sent: %u snd_rt_win: %u next_send: %u",
                init_credit, max_sent, snd_rt_win, next_send);

        LOG_DBG("Resulting snd_rt_win_edge: %u", dtcp->sv->snd_rt_wind_edge);

        spin_unlock_bh(&dtp->sv_lock);

        rcu_read_lock();
        dtcp_ps = dtcp_ps_get(dtcp);

        if (dtcp_ps->rtx_ctrl) {
                if (!dtp->rtxq) {
                        rcu_read_unlock();
                        LOG_ERR("Couldn't find the Retransmission queue");
                        return -1;
                }
                rtxq_flush(dtp->rtxq);
        }

        if (dtcp_ps->flow_ctrl) {
                if (cwq_flush(dtp->cwq)) {
                        rcu_read_unlock();
                        LOG_ERR("Coudln't flush cwq");
                        return -1;
                }
        }
        rcu_read_unlock();

        /*FIXME: Missing sending the control ack pdu */
        return 0;
}

bool default_reconcile_flow_conflict(struct dtp_ps * ps)
{
	LOG_DBG("Reconciling window and rate flow controls...");
        return true;
}

struct ps_base * dtp_ps_default_create(struct rina_component * component)
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
