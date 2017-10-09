/*
 * Congestion avoidance Scheme based on Jain's report DTCP PS
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

#define RINA_PREFIX "cas-dtcp-ps"

#include "rds/rmem.h"
#include "dtcp-ps.h"
#include "dtcp-conf-utils.h"
#include "policies.h"
#include "logs.h"

#define W_INC_A_P_DEFAULT     1
#define DEBUG_ENABLED         0

struct cas_dtcp_ps_data {
        seq_num_t    wc;
        seq_num_t    wp;
        unsigned int w_inc_a_p;
        unsigned int ecn_count;
        unsigned int rcv_count;

        /* Value of window in fixed point, most significant 16 bits are the
         * integer part, less significant 16 bits are the decimal part (i.e.
         * 0000 0000 0000 0000 0000 0000 0000 0001 represents 0.00001525878).
         * With that we can get window values up to 65536 PDUs, and a
         * resolution of almost 5 decimals. Window values must be computed
         * using real numbers, otherwise a fair allocation of windows between
         * competing flows is not guaranteed.
         */
        unsigned int real_window;

#if DEBUG_ENABLED
	/* Used to debug the evolution of the window size withouth penalizing
	 * the performance of the stack. It should be normally set to 0
	 */
	seq_num_t   ws_log[5000];
	int         ws_index;
#endif
};

static unsigned int round_half_to_even(unsigned int real_window)
{
	unsigned int decimal_part = real_window & 0x0000FFFF;
	unsigned int integer_part = (real_window & 0xFFFF0000) >> 16;

	if (decimal_part > 0x00007FFF)
		return integer_part +1;

	if (decimal_part < 0x00007FFF)
		return integer_part;

	if ((integer_part & 0x00000001) == 0)
		return integer_part +1;

	return integer_part;
}

static int
cas_rcvr_flow_control(struct dtcp_ps * ps, const struct pci * pci)
{
        struct dtcp *             dtcp = ps->dm;
        struct cas_dtcp_ps_data * data = ps->priv;
        uint_t rwe;

        if (!dtcp || !data) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }
        if (!pci) {
                LOG_ERR("No PCI passed, cannot run policy");
                return -1;
        }

	/* FIXME: This has to be considered in the case the sender inactivity
	 * timers is triggered */
/*
        if (!data->first_run && (pci_flags_get(pci) & PDU_FLAGS_DATA_RUN)) {
                LOG_DBG("DRF Flag, reseting...");
                data->ecn_count = 0;
                data->rcv_count = 0;
        }
*/

        spin_lock_bh(&dtcp->parent->sv_lock);
        /* if we passed the wp bits, consider ecn bit */
        if ((++data->rcv_count > data->wp) &&
             ((int) (pci_flags_get(pci) & PDU_FLAGS_EXPLICIT_CONGESTION))) {
                data->ecn_count++;
                LOG_DBG("ECN bit marked, total %u", data->ecn_count);
        }

        /* it is time to update the window's size & reset */
        if (data->rcv_count == data->wc + data->wp) {
                LOG_DBG("Updating window size...");
                data->wp = data->wc;
                /*check number of ecn bits set */
                LOG_DBG("ECN COUNT: %d, Wc: %u", data->ecn_count, data->wc);
                if (data->ecn_count > (data->wc >> 1)) {
                	if (data->wc != 1) {
                		/* decrease window's size, multiplying by 0,875
                		 * X*0,875 = x(1-0.125) = x -x/8
                		 */
                		data->real_window = data->real_window - (data->real_window >>3);
                		data->wc = round_half_to_even(data->real_window);
                		LOG_DBG("Window size decreased, new values are Wp: %u, Wc: %u",
                				data->wp, data->wc);
                	}
                } else {
                        /*increment window's size */
                	data->real_window += data->w_inc_a_p << 16;
                	data->wc = round_half_to_even(data->real_window);
                        LOG_DBG("Window size increased, new values are Wp: %u, Wc: %u",
                        	data->wp, data->wc);
                }
#if DEBUG_ENABLED
                data->ws_log[data->ws_index++] = data->wc;
#endif
                data->rcv_count = 0;
                data->ecn_count = 0;
                dtcp->sv->rcvr_credit = data->wc;
        }

        dtcp->sv->rcvr_rt_wind_edge = dtcp->sv->rcvr_credit +
        		dtcp->parent->sv->rcv_left_window_edge;
        rwe = dtcp->sv->rcvr_rt_wind_edge;
	spin_unlock_bh(&dtcp->parent->sv_lock);

        LOG_DBG("Credit and RWE set: %u, %u", data->wc, rwe);

        return 0;
}

static int dtcp_ps_cas_set_policy_set_param(struct ps_base * bps,
                                            const char    * name,
                                            const char    * value)
{
        struct dtcp_ps *ps = container_of(bps, struct dtcp_ps, base);
        struct cas_dtcp_ps_data * data = ps->priv;
        int bool_value;
        int ret;

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

        if (strcmp(name, "w_inc_a_p") == 0) {
                ret = kstrtoint(value, 10, &bool_value);
                if (!ret)
                        data->w_inc_a_p = bool_value;
        }
        return 0;
}

static struct ps_base *
dtcp_ps_cas_create(struct rina_component * component)
{
        struct dtcp *             dtcp  = dtcp_from_component(component);
        struct dtcp_ps *          ps    = rkzalloc(sizeof(*ps), GFP_KERNEL);
        struct cas_dtcp_ps_data * data  = rkzalloc(sizeof(*data), GFP_KERNEL);
        struct policy *           ps_conf;
        struct policy_parm *      ps_param;
        struct dtcp_config *      dtcp_cfg;

        if (!ps || !data || !dtcp) {
                return NULL;
        }

        dtcp_cfg                        = dtcp->cfg;

        ps->base.set_policy_set_param   = dtcp_ps_cas_set_policy_set_param;
        ps->dm                          = dtcp;

#if DEBUG_ENABLED
	data->ws_index = 0;
#endif

        data->w_inc_a_p                 = W_INC_A_P_DEFAULT;
        /* Cannot use this because it is initialized later on in
         * dtcp_select_policy_set */
        /*data->wc                        = ps->flowctrl.window.initial_credit;*/
        data->wc                        = dtcp_initial_credit(dtcp_cfg);
        data->real_window		= data->wc << 16;
        data->wp                        = 0;
        data->ecn_count                 = 0;
        data->rcv_count                 = 0;

        ps->priv                        = data;

        //ps_conf = dtcp_ps(dtcp_cfg);
        ps_conf = dtcp_cfg->dtcp_ps;
        if (!ps_conf) {
                LOG_ERR("No PS conf struct");
                return NULL;
        }
        ps_param = policy_param_find(ps_conf, "w_inc_a_p");
        if (!ps_param) {
                LOG_WARN("No PS param w_inc_a_p");
        }
        dtcp_ps_cas_set_policy_set_param(&ps->base,
                                         policy_param_name(ps_param),
                                         policy_param_value(ps_param));

        ps->flow_init                   = NULL;
        ps->lost_control_pdu            = NULL; /* default */
        ps->rtt_estimator               = NULL; /* default */
        ps->retransmission_timer_expiry = NULL;
        ps->received_retransmission     = NULL;
        ps->sender_ack                  = NULL; /* default */
        ps->sending_ack                 = NULL; /* default */
        ps->receiving_ack_list          = NULL;
        ps->initial_rate                = NULL;
        ps->receiving_flow_control      = NULL; /* default */
        ps->update_credit               = NULL;
        ps->rcvr_ack                    = NULL; /* default */
        ps->rcvr_flow_control           = cas_rcvr_flow_control;
        ps->rate_reduction              = NULL; /* default */
        ps->rcvr_control_ack            = NULL;
        ps->no_rate_slow_down           = NULL;
        ps->no_override_default_peak    = NULL;

        return &ps->base;
}

static void dtcp_ps_cas_destroy(struct ps_base * bps)
{
        struct dtcp_ps *ps = container_of(bps, struct dtcp_ps, base);
        struct cas_dtcp_ps_data * data;
        if (bps) {
                if (ps->priv) {
			data = ps->priv;
#if DEBUG_ENABLED
			if (data->ws_index > 0) {
				int i;
				for (i=0; i< data->ws_index; i++) {
					LOG_INFO("(%p) = %u", data, data->ws_log[i] );
				}
			}
#endif
                        rkfree(data);
                }
                rkfree(ps);
        }
}

struct ps_factory dtcp_factory = {
        .owner          = THIS_MODULE,
        .create  = dtcp_ps_cas_create,
        .destroy = dtcp_ps_cas_destroy,
};
