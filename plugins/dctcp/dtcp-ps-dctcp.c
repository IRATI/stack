/*
 * DCTCP Policy Set for DTCP
 *
 *    Matej Gregr <igregr@fit.vutbr.cz>
 *
 * This program is free software; you can dummyistribute it and/or modify
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

#define RINA_PREFIX "dctcp-dtcp-ps"

#include "rds/rmem.h"
#include "dtcp-ps.h"
#include "logs.h"

#define DEC_PRECISION 1000000
#define DCTCP_MAX_ALPHA 1024U

enum tx_state {
	SLOW_START,
	CONG_AVOID
};

struct dctcp_dtcp_ps_data {
	uint_t        init_credit;
	uint_t        sshtresh;
	enum tx_state state;
	uint_t        dec_credit;
	uint_t        shift_g;
	uint_t        sent_total;
	uint_t        ecn_total;
	uint_t        dctcp_alpha;
	uint_t	      cycle_start_jiffies;
	uint_t	      current_rtt;
};

static int dctcp_rcvr_flow_control(struct dtcp_ps * ps, const struct pci * pci)
{
	struct dtcp * dtcp = ps->dm;
	struct dctcp_dtcp_ps_data * data = ps->priv;
	seq_num_t new_credit;
	uint_t alpha_old = 0;
	uint_t elapsed_time = 0;

	spin_lock_bh(&dtcp->parent->sv_lock);
	new_credit = dtcp->sv->rcvr_credit;

	// Check if we must abandon Slow Start state
	if (new_credit >= data->sshtresh) {
		data->state = CONG_AVOID;
	}

	// Update congestion window
	if ((pci_flags_get(pci) & PDU_FLAGS_EXPLICIT_CONGESTION)) {
		// PDU is ECN-marked, decrease cwnd value
		data->ecn_total++;
		new_credit = max(new_credit -
			  ((new_credit * data->dctcp_alpha) >> 11U), 2U);

		// TODO check if this is ok
		data->sshtresh = new_credit;
		data->state = CONG_AVOID;
	} else if (data->state == SLOW_START) {
		// Increase credit by one
		new_credit++;
	} else {
		// CA state, increase by 1/cwnd
		data->dec_credit += DEC_PRECISION/new_credit;
		if (data->dec_credit >= DEC_PRECISION) {
			new_credit++;
			data->dec_credit -= DEC_PRECISION;
		}
	}

	data->sent_total++;
	if ((pci_flags_get(pci) & PDU_FLAGS_EXPLICIT_CONGESTION)) {
		data->ecn_total++;
	}

	/* Update alpha once every RTT. Since we don't have an RTT timeout */
	/* we update RTT every credit packets received */
	elapsed_time = jiffies_to_msecs(jiffies - data->cycle_start_jiffies);
	if (elapsed_time >= data->current_rtt) {
		LOG_DBG("Received %u PDUs, with %u marked PDUs in %u ms",
			data->sent_total, data->ecn_total, elapsed_time);
		/* alpha = (1-g) * alpha + g * F according DCTCP kernel patch */
		alpha_old = data->dctcp_alpha;
		data->dctcp_alpha = alpha_old - (alpha_old >> data->shift_g) +
				(data->ecn_total << (10 - data->shift_g)) / data->sent_total;

		data->sent_total = 0;
		data->ecn_total = 0;
		data->cycle_start_jiffies = jiffies;
		data->current_rtt = dtcp->sv->rtt;
	}

	/* Update credit and right window edge */
	dtcp->sv->rcvr_credit = new_credit;

	/* applying the TCP rule of not shrinking the window */
	if (dtcp->parent->sv->rcv_left_window_edge + new_credit >
		dtcp->sv->rcvr_rt_wind_edge)
		dtcp->sv->rcvr_rt_wind_edge =
			dtcp->parent->sv->rcv_left_window_edge + new_credit;

	LOG_DBG("New credit is %u, Alpha is %u",
		new_credit, data->dctcp_alpha);

	spin_unlock_bh(&dtcp->parent->sv_lock);

	return 0;
}

static int dtcp_ps_set_policy_set_param(struct ps_base * bps, const char * name,
					const char * value)
{
	struct dtcp_ps *ps = container_of(bps, struct dtcp_ps, base);
	struct dctcp_dtcp_ps_data *data = ps->priv;
	int ival = 0;
	int ret = 0;

	(void) ps;

	if (!name) {
		LOG_ERR("Null parameter name");
		return -1;
	}

	if (!value) {
		LOG_ERR("Null parameter value");
		return -1;
	}

	if (strcmp(name, "shift_g") == 0) {
		ret = kstrtoint(value, 10, &ival);
		if (!ret) {
			data->shift_g = ival;
		}
	}

	return 0;
}
static struct ps_base * dtcp_ps_dctcp_create(struct rina_component * component)
{
	struct dtcp * dtcp = dtcp_from_component(component);
	struct dtcp_ps * ps = rkzalloc(sizeof(*ps), GFP_KERNEL);
	struct dctcp_dtcp_ps_data * data = rkzalloc(sizeof(*data), GFP_KERNEL);

	if (!ps || !data || !dtcp) {
		return NULL;
	}

	data->state = SLOW_START;
	data->init_credit = 3;
	data->dec_credit = 0;
	data->sshtresh = 0XFFFFFFFF;
	data->shift_g = 4;
	data->sent_total = 0;
	data->ecn_total = 0;
	data->current_rtt = dtcp->sv->rtt;
	data->cycle_start_jiffies = jiffies;
	dtcp->sv->rcvr_credit = data->init_credit;

	ps->base.set_policy_set_param   = dtcp_ps_set_policy_set_param;
	ps->dm                          = dtcp;
	ps->priv                        = data;
	ps->flow_init                   = NULL;
	ps->lost_control_pdu            = NULL;
	ps->rtt_estimator               = NULL;
	ps->retransmission_timer_expiry = NULL;
	ps->received_retransmission     = NULL;
	ps->sender_ack                  = NULL;
	ps->sending_ack                 = NULL;
	ps->receiving_ack_list          = NULL;
	ps->initial_rate                = NULL;
	ps->receiving_flow_control      = NULL;
	ps->update_credit               = NULL;
	ps->rcvr_ack                    = NULL;
	ps->rcvr_flow_control           = dctcp_rcvr_flow_control;
	ps->rate_reduction              = NULL;
	ps->rcvr_control_ack            = NULL;
	ps->no_rate_slow_down           = NULL;
	ps->no_override_default_peak    = NULL;

	LOG_INFO("DCTCP DTCP policy created, shift_g value is %d",
		 data->shift_g);

	return &ps->base;
}

static void dtcp_ps_dctcp_destroy(struct ps_base * bps)
{
	struct dtcp_ps *ps = container_of(bps, struct dtcp_ps, base);

	if (bps) {
		rkfree(ps);
	}
}

struct ps_factory dtcp_factory = {
		.owner   = THIS_MODULE,
		.create  = dtcp_ps_dctcp_create,
		.destroy = dtcp_ps_dctcp_destroy,
};
