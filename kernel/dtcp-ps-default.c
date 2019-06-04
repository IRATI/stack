/*
 * Default policy set for DTCP
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

#define RINA_PREFIX "dtcp-ps-default"

#include "rds/rmem.h"
#include "dtp.h"
#include "dtcp-ps.h"
#include "dtcp-ps-default.h"
#include "dtp-utils.h"
#include "du.h"
#include "logs.h"
#include "rds/rtimer.h"

int default_lost_control_pdu(struct dtcp_ps * ps)
{
        struct dtcp * dtcp = ps->dm;

        if (!dtcp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }

#if 0
        struct du * du_ctrl;
        seq_num_t last_rcv_ctrl, snd_lft, snd_rt;

        spin_lock_bh(&dtcp->parent->sv_lock);
        last_rcv_ctrl = dtcp->sv->last_rcv_ctl_seq;
        snd_lft       = dtcp->sv->snd_lft_win;
        snd_rt        = dtcp->sv->snd_rt_wind_edge;
        spin_unlock_bh(&dtcp->parent->sv_lock);

        du_ctrl      = pdu_ctrl_ack_create(dtcp,
                                           last_rcv_ctrl,
                                           snd_lft,
                                           snd_rt);
        if (!du_ctrl) {
                LOG_ERR("Failed Lost Control PDU policy");
                return -1;
        }

        if (dt_pdu_send(dtcp->parent, du_ctrl))
                return -1;
#endif

        LOG_DBG("Default lost control pdu policy");

        return 0;
}

#ifdef CONFIG_RINA_DTCP_RCVR_ACK
int default_rcvr_ack(struct dtcp_ps * ps, const struct pci * pci)
{
        struct dtcp * dtcp = ps->dm;
        seq_num_t     seq;

        if (!pci) {
                LOG_ERR("No PCI passed, cannot run policy");
                return -1;
        }
        seq = pci_sequence_number_get(pci);

        return dtcp_ack_flow_control_pdu_send(dtcp, seq);
}
#endif

#ifdef CONFIG_RINA_DTCP_RCVR_ACK_ATIMER
int default_rcvr_ack_atimer(struct dtcp_ps * ps, const struct pci * pci)
{ return 0; }
#endif

int default_sender_ack(struct dtcp_ps * ps, seq_num_t seq_num)
{
        struct dtcp * dtcp = ps->dm;
        timeout_t tr;

        if (!dtcp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }

        if (ps->rtx_ctrl) {
                if (!dtcp->parent->rtxq) {
                        LOG_ERR("Couldn't find the Retransmission queue");
                        return -1;
                }

                spin_lock_bh(&dtcp->parent->sv_lock);

                tr = dtcp->parent->sv->tr;

                rtxq_ack(dtcp->parent->rtxq, seq_num, tr);

                /* Update LWE */
                dtcp->sv->snd_lft_win = seq_num + 1;

                spin_unlock_bh(&dtcp->parent->sv_lock);
        }

        return 0;
}

int default_sending_ack(struct dtcp_ps * ps, seq_num_t seq)
{
        struct pci * pci;
        int ret;

        struct dtcp * dtcp = ps->dm;

        if (!dtcp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }

        if (!dtcp->parent) {
                LOG_ERR("No DTP from dtcp->parent");
                return -1;
        }

        /* Invoke delimiting and update left window edge */

        pci = process_A_expiration(dtcp->parent, dtcp);
        if (!pci)
                return 0;

        ret = dtcp_sv_update(ps->dm, pci);
        pci_release(pci);

        return ret;
}

int default_receiving_flow_control(struct dtcp_ps * ps, const struct pci * pci)
{
        struct dtcp * dtcp = ps->dm;
        struct du * du;

        if (!dtcp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }
        if (!pci) {
                LOG_ERR("No PCI passed, cannot run policy");
                return -1;
        }

        du = pdu_ctrl_generate(dtcp, PDU_TYPE_FC);
        if (!du)
                return -1;

        LOG_DBG("DTCP Sending FC (CPU: %d)", smp_processor_id());

        if (dtcp_pdu_send(dtcp, du))
               return -1;

        return 0;
}

int default_rcvr_flow_control(struct dtcp_ps * ps, const struct pci * pci)
{
        struct dtcp * dtcp = ps->dm;
        seq_num_t LWE;
        seq_num_t RWE;
        seq_num_t lwe_p;
        seq_num_t rwe_p;

        if (!dtcp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }
        if (!pci) {
                LOG_ERR("No PCI passed, cannot run policy");
                return -1;
        }
        lwe_p = pci_control_new_left_wind_edge(pci);
        rwe_p = pci_control_new_rt_wind_edge(pci);

        spin_lock_bh(&dtcp->parent->sv_lock);
        LWE = dtcp->parent->sv->rcv_left_window_edge;
        dtcp->sv->rcvr_rt_wind_edge = LWE + dtcp->sv->rcvr_credit;
        RWE = dtcp->sv->rcvr_rt_wind_edge;
        spin_unlock_bh(&dtcp->parent->sv_lock);

        if (dtcp->sv->rendezvous_rcvr) {
                LOG_DBG("DTCP: LWE: %u  RWE: %u -- PCI: lwe: %u, rwe: %u",
                	LWE, RWE, lwe_p, rwe_p);
        }

        return 0;
}

int default_rate_reduction(struct dtcp_ps * ps, const struct pci * pci)
{
	struct dtcp * dtcp = ps->dm;
	u_int32_t rt;
	u_int32_t tf;

	if (!dtcp) {
	       LOG_ERR("No instance passed, cannot run policy");
	       return -1;
	}

	rt = pci_control_sndr_rate(pci);
	tf = pci_control_time_frame(pci);

	if(rt == 0 || tf == 0) {
	       return 0;
	}

	spin_lock_bh(&dtcp->parent->sv_lock);
	dtcp->sv->sndr_rate = rt;
	dtcp->sv->rcvr_rate = rt;
	dtcp->sv->time_unit = tf;

	LOG_DBG("DTCP: %pK", dtcp);
	LOG_DBG("    Rate: %u, Time: %u",
		dtcp->sv->sndr_rate,
		dtcp->sv->time_unit);

	spin_unlock_bh(&dtcp->parent->sv_lock);

	return 0;
}

static int rtt_calculation(struct dtcp_ps * ps, timeout_t start_time)
{
	struct dtcp *       dtcp;
	uint_t              rtt, new_sample, srtt, rttvar, trmsecs;
	timeout_t           a;
	int                 abs;

	if (!ps)
		return -1;
	dtcp = ps->dm;
	if (!dtcp)
		return -1;

	new_sample = jiffies_to_msecs(jiffies - start_time);

	spin_lock_bh(&dtcp->parent->sv_lock);

	rtt    = dtcp->sv->rtt;
	srtt   = dtcp->sv->srtt;
	rttvar = dtcp->sv->rttvar;
	a      = dtcp->parent->sv->A;

	if (!rtt) {
		rtt    = new_sample;
		rttvar = new_sample >> 1;
		srtt   = new_sample;
	} else {
		/* RTT <== RTT * (112/128) + SAMPLE * (16/128)*/
		rtt = (rtt * 112 + (new_sample << 4)) >> 7;
		abs = srtt - new_sample;
		abs = abs < 0 ? -abs : abs;
		rttvar = ((3 * rttvar) >> 2) + (((uint_t)abs) >> 2);
	}

	/* FIXME: k, G, alpha and betha should be parameters passed to the
	 * policy set. Probably moving them to ps->priv */

	/* k * rttvar = 4 * rttvar */
	trmsecs  = rttvar << 2;
	/* G is 0.1s according to RFC6298, then 100ms */
	trmsecs  = 100 > trmsecs ? 100 : trmsecs;
	trmsecs += srtt + jiffies_to_msecs(a);
	/* RTO (tr) less than 1s? (not for the common policy) */
	/*trmsecs  = trmsecs < 1000 ? 1000 : trmsecs;*/

	dtcp->sv->rtt = rtt;
	dtcp->sv->rttvar = rttvar;
	dtcp->sv->srtt = srtt;
	dtcp->parent->sv->tr = msecs_to_jiffies(trmsecs);

	LOG_DBG("RTT estimated at %u", rtt);

	spin_unlock_bh(&dtcp->parent->sv_lock);

	return 0;
}

int default_rtt_estimator(struct dtcp_ps * ps, seq_num_t sn)
{
	struct dtcp *       dtcp;
	timeout_t           start_time;

	if (!ps)
		return -1;
	dtcp = ps->dm;
	if (!dtcp)
		return -1;

	LOG_DBG("RTT Estimator...");

	start_time = rtxq_entry_timestamp(dtcp->parent->rtxq, sn);
	if (start_time == 0) {
		LOG_DBG("RTTestimator: PDU %u has been retransmitted", sn);
		return 0;
	} else if (start_time == -1) {
		/* The PDU being ACKed is no longer in the RTX queue */
		return -1;
	}

	rtt_calculation(ps, start_time);

	return 0;
}

int default_rtt_estimator_nortx(struct dtcp_ps * ps, seq_num_t sn)
{
	struct dtcp *       dtcp;
	timeout_t           start_time;

	if (!ps)
		return -1;
	dtcp = ps->dm;
	if (!dtcp)
		return -1;

	LOG_DBG("RTT Estimator with only flow control...");

	start_time = rttq_entry_timestamp(dtcp->parent->rttq, sn);
	if (start_time == 0) {
		return 0;
	}

	rtt_calculation(ps, start_time);

	return 0;
}

int default_rcvr_rendezvous(struct dtcp_ps * ps, const struct pci * pci)
{
	struct dtcp *       dtcp;
	seq_num_t rcv_lft, rcv_rt, snd_lft, snd_rt;
	timeout_t rv;

	if (!ps)
		return -1;
	dtcp = ps->dm;
	if (!dtcp)
		return -1;

	spin_lock_bh(&dtcp->parent->sv_lock);
	/* TODO: check if retransmission control enabled */

	if (dtcp->parent->sv->window_based) {
		rcv_lft = pci_control_new_left_wind_edge(pci);
		rcv_rt = pci_control_new_rt_wind_edge(pci);
		snd_lft = pci_control_my_left_wind_edge(pci);
		snd_rt = pci_control_my_rt_wind_edge(pci);

		/* Check Consistency of the Receiving Window values with the
		 * values in the PDU.
		 */
		if (dtcp->sv->snd_lft_win != rcv_lft) {
			/* TODO what to do? */
		}

		if (dtcp->sv->snd_rt_wind_edge != rcv_rt) {
			/* TODO what to do? */
		}
		LOG_DBG("RCVR rendezvous. RCV LWE: %u | RCV RWE: %u || "
				"SND LWE: %u | SND RWE: %u",
			dtcp->parent->sv->rcv_left_window_edge,
			dtcp->sv->rcvr_rt_wind_edge, snd_lft, snd_rt);

		dtcp->sv->rcvr_rt_wind_edge = snd_lft + dtcp->sv->rcvr_credit;
	}

	if (dtcp->sv->flow_ctl && dtcp->parent->sv->rate_based) {
		/* TODO implement */
	}

	/* TODO Receiver is in the Rendezvous-at-the-receiver state. The next PDU is
	 * expected to have DRF bit set to true
	 */

	dtcp->sv->rendezvous_rcvr = true;

	spin_unlock_bh(&dtcp->parent->sv_lock);

	/* Send a ControlAck PDU to confirm reception of RendezvousPDU via
	 * lastControlPDU value or send any other control PDU with Flow Control
	 * information opening the window.
	 */
	if (dtcp->sv->rcvr_credit != 0) {
		rv = jiffies_to_msecs(dtcp->parent->sv->tr);
		rtimer_start(&dtcp->rendezvous_rcv, rv);
	}

	atomic_inc(&dtcp->cpdus_in_transit);
	LOG_DBG("DTCP 1st Sending FC to stop Rendezvous (CPU: %d)",
		smp_processor_id());

	return ctrl_pdu_send(dtcp, PDU_TYPE_FC, true);
}

struct ps_base * dtcp_ps_default_create(struct rina_component * component)
{
        struct dtcp * dtcp = dtcp_from_component(component);
        struct dtcp_ps * ps = rkzalloc(sizeof(*ps), GFP_KERNEL);

        if (!ps) {
                return NULL;
        }

        ps->base.set_policy_set_param   = NULL;
        ps->dm                          = dtcp;
        ps->priv                        = NULL;
        ps->flow_init                   = NULL;
        ps->lost_control_pdu            = default_lost_control_pdu;
        if (ps->rtx_ctrl) {
        	ps->rtt_estimator = default_rtt_estimator;
        } else if (ps->flow_ctrl) {
        	ps->rtt_estimator = default_rtt_estimator_nortx;
        }
        ps->retransmission_timer_expiry = NULL;
        ps->received_retransmission     = NULL;
        ps->sender_ack                  = default_sender_ack;
        ps->sending_ack                 = default_sending_ack;
        ps->receiving_ack_list          = NULL;
        ps->initial_rate                = NULL;
        ps->receiving_flow_control      = default_receiving_flow_control;
        ps->update_credit               = NULL;
#ifdef CONFIG_RINA_DTCP_RCVR_ACK
        ps->rcvr_ack                    = default_rcvr_ack,
#endif
#ifdef CONFIG_RINA_DTCP_RCVR_ACK_ATIMER
        ps->rcvr_ack                    = default_rcvr_ack_atimer,
#endif
        ps->rcvr_flow_control           = default_rcvr_flow_control;
        ps->rate_reduction              = default_rate_reduction;
        ps->rcvr_control_ack            = NULL;
        ps->no_rate_slow_down           = NULL;
        ps->no_override_default_peak    = NULL;
        ps->rcvr_rendezvous             = default_rcvr_rendezvous;

        return &ps->base;
}
EXPORT_SYMBOL(dtcp_ps_default_create);

void dtcp_ps_default_destroy(struct ps_base * bps)
{
        struct dtcp_ps *ps = container_of(bps, struct dtcp_ps, base);

        if (bps) {
                rkfree(ps);
        }
}
EXPORT_SYMBOL(dtcp_ps_default_destroy);
