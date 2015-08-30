/*
 * TCP-like RED Congestion avoidance policy set
 *
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
 *
 * This program is free software; you can casistribute it and/or modify
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
#include <linux/bitmap.h>

#define RINA_PREFIX "red-dtcp-ps"

#include "rds/rmem.h"
#include "dtcp-ps.h"
#include "dtcp-conf-utils.h"
#include "dtcp-ps-common.h"
#include "policies.h"
#include "logs.h"
#include "dtcp-ps-debug.h"


#define DEC_PRECISION 1000000

enum tx_state {
	SLOW_START,
	CONG_AVOID
};

struct red_dtcp_ps_data {
	spinlock_t    lock;
        uint_t        new_ecn_burst;
	uint_t        init_credit;
	uint_t        sshtresh;
	enum tx_state state;
	uint_t        dec_credit;
	/* to allocate debug information and display by procfs
	 * it is cleaned up in dtcp-ps-debug.c */
	struct red_dtcp_debug *debug;
};

static int
red_lost_control_pdu(struct dtcp_ps * ps)
{ return common_lost_control_pdu(ps); }

static int red_rcvr_ack(struct dtcp_ps * ps, const struct pci * pci)
{ return common_rcvr_ack(ps, pci); }

static int
red_sender_ack(struct dtcp_ps * ps, seq_num_t seq_num)
{ return common_sender_ack(ps, seq_num); }

static int
red_sending_ack(struct dtcp_ps * ps, seq_num_t seq)
{ return common_sending_ack(ps, seq); }

static int
red_receiving_flow_control(struct dtcp_ps * ps, const struct pci * pci)
{ return common_receiving_flow_control(ps, pci); }

static int
red_rcvr_flow_control(struct dtcp_ps * ps, const struct pci * pci)
{
        struct dtcp * dtcp = ps->dm;
        struct red_dtcp_ps_data * data = ps->priv;
        seq_num_t new_credit;
	unsigned long flags;

	spin_lock_irqsave(&data->lock, flags);
        new_credit = dtcp_rcvr_credit(dtcp);
	if (data->state == SLOW_START) {
		new_credit++;
		if (new_credit >= data->sshtresh) {
			data->state = CONG_AVOID;
		}
	} else {
		data->dec_credit += DEC_PRECISION/new_credit;
		if (data->dec_credit >= DEC_PRECISION) {
			new_credit++;
			data->dec_credit -= DEC_PRECISION;
		}
	}

        if ((pci_flags_get(pci) & PDU_FLAGS_EXPLICIT_CONGESTION) && !data->new_ecn_burst) {
		new_credit = (2 >= (new_credit >> 1) ? 2 : (new_credit >> 1));
		data->sshtresh = new_credit;
		data->state = CONG_AVOID;
		data->new_ecn_burst = new_credit;
		LOG_INFO("change enc_burst to %u", data->new_ecn_burst);
	} else if (data->new_ecn_burst != 0)
		data->new_ecn_burst--;

        /* set new credit */
        dtcp_rcvr_credit_set(dtcp, new_credit);
	spin_unlock_irqrestore(&data->lock, flags);
        //update_rt_wind_edge(dtcp);
	update_credit_and_rt_wind_edge(dtcp, new_credit);

	/* Debug register */
	if (data->debug->ws_index < DEBUG_SIZE) {
        	data->debug->ws_log[data->debug->ws_index++] = new_credit;
	}

        return 0;
}

static int
red_rate_reduction(struct dtcp_ps * ps)
{ return common_rate_reduction(ps); }

static int dtcp_ps_red_set_policy_set_param(struct ps_base * bps,
                                            const char    * name,
                                            const char    * value)
{
        struct dtcp_ps *ps = container_of(bps, struct dtcp_ps, base);
        struct red_dtcp_ps_data * data = ps->priv;

        if (!ps || ! data) {
                LOG_ERR("Wrong PS or parameters to set");
                return -1;
        }

        if (!name) {
                LOG_ERR("Null parameter name");
                return -1;
        }

        if (!value) {
                LOG_ERR("Null parameter value");
                return -1;
        }

        return 0;
}

static int red_rtt_estimator(struct dtcp_ps * ps, seq_num_t sn)
{ return common_rtt_estimator(ps, sn); }

static struct ps_base *
dtcp_ps_red_create(struct rina_component * component)
{
        struct dtcp *             dtcp  = dtcp_from_component(component);
        struct dtcp_ps *          ps    = rkzalloc(sizeof(*ps), GFP_KERNEL);
        struct red_dtcp_ps_data * data = rkzalloc(sizeof(*data), GFP_KERNEL);

        if (!ps || !data || !dtcp) {
                return NULL;
        }

        data->new_ecn_burst = 0;
	data->state = SLOW_START;
	data->init_credit = 3;
	data->dec_credit = 0;
	data->sshtresh = 0XFFFFFFFF;
	dtcp_rcvr_credit_set(dtcp, data->init_credit);
	data->debug = red_dtcp_debug_create();

	spin_lock_init(&data->lock);

        ps->base.set_policy_set_param   = dtcp_ps_red_set_policy_set_param;
        ps->dm                          = dtcp;
        ps->priv                        = data;

        ps->flow_init                   = NULL;
        ps->lost_control_pdu            = red_lost_control_pdu;
        ps->rtt_estimator               = red_rtt_estimator;
        ps->retransmission_timer_expiry = NULL;
        ps->received_retransmission     = NULL;
        ps->sender_ack                  = red_sender_ack;
        ps->sending_ack                 = red_sending_ack;
        ps->receiving_ack_list          = NULL;
        ps->initial_rate                = NULL;
        ps->receiving_flow_control      = red_receiving_flow_control;
        ps->update_credit               = NULL;
        ps->reconcile_flow_conflict     = NULL;
        ps->rcvr_ack                    = red_rcvr_ack,
        ps->rcvr_flow_control           = red_rcvr_flow_control;
        ps->rate_reduction              = red_rate_reduction;
        ps->rcvr_control_ack            = NULL;
        ps->no_rate_slow_down           = NULL;
        ps->no_override_default_peak    = NULL;

        return &ps->base;
}

static void dtcp_ps_red_destroy(struct ps_base * bps)
{
        struct dtcp_ps *ps = container_of(bps, struct dtcp_ps, base);
	struct red_dtcp_ps_data * data;
        if (bps) {
                if (ps->priv) {
			data = ps->priv;
                        rkfree((struct red_dtcp_ps_data *) ps->priv);
                }
                rkfree(ps);
        }
}

struct ps_factory dtcp_factory = {
        .owner   = THIS_MODULE,
        .create  = dtcp_ps_red_create,
        .destroy = dtcp_ps_red_destroy,
};
