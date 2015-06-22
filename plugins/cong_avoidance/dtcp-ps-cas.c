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
#include "dtcp-utils.h"
#include "policies.h"
#include "logs.h"

#define W_INC_A_P_DEFAULT     1
#define W_DEC_B_NUM_P_DEFAULT 7
#define W_DEC_B_DEN_P_DEFAULT 3

struct cas_dtcp_ps_data {
        seq_num_t    wc;
        seq_num_t    wp;
        seq_num_t    wc_lwe;
        unsigned int w_inc_a_p;
        unsigned int w_dec_b_num_p;
        unsigned int w_dec_b_den_p;
        unsigned int rcv_count;
        unsigned int ecn_count;
};

static int
cas_lost_control_pdu(struct dtcp_ps * ps)
{
        printk("%s: called()\n", __func__);
        return 0;
}

static int cas_rcvr_ack(struct dtcp_ps * ps, const struct pci * pci)
{
        printk("%s: called()\n", __func__);
        return 0;
}

static int
cas_sender_ack(struct dtcp_ps * ps, seq_num_t seq_num)
{
        printk("%s: called()\n", __func__);
        return 0;
}

static int
cas_sending_ack(struct dtcp_ps * ps, seq_num_t seq)
{
        printk("%s: called()\n", __func__);
        return 0;
}

static int
cas_receiving_flow_control(struct dtcp_ps * ps, const struct pci * pci)
{
        printk("%s: called()\n", __func__);
        return 0;
}

static int
cas_rcvr_flow_control(struct dtcp_ps * ps, const struct pci * pci)
{
        struct dtcp *             dtcp = ps->dm;
        struct cas_dtcp_ps_data * data = ps->priv;
        seq_num_t                 c_seq;

        if (!dtcp || !data) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }
        if (!pci) {
                LOG_ERR("No PCI passed, cannot run policy");
                return -1;
        }

        data->rcv_count++;
        c_seq   = pci_sequence_number_get(pci);

        /* add ecn bit */
        if (data->rcv_count > (data->wc_lwe + data->wp)) {
                data->ecn_count += (unsigned int) (pci_flags_get(pci) && PDU_FLAGS_EXPLICIT_CONGESTION);

                if (c_seq - data->wc_lwe <= 0) {
                        LOG_ERR("Seq_number %u was not expected, current window's LWE"
                                 " will be used instead",
                                c_seq);
                        c_seq = data->wc_lwe;
                }
        }

        /* it is time to update the window's size */
        if (data->rcv_count == data->wc + data->wp) {
                data->wp = data->wc;
                data->wc_lwe = dt_sv_rcv_lft_win(dtcp_dt(dtcp));

                /*check number of ecn bits set */
                if (data->ecn_count >= (data->wc >> 1)) {
                        /* decrease window's size*/
                        data->wc = (data->wc * data->w_dec_b_num_p) >> data->w_dec_b_den_p;
                } else {
                        /*increment window's size */
                        data->wc += data->w_inc_a_p;
                }
                dtcp_rcvr_credit_set(dtcp, data->wc);
        }

        update_rt_wind_edge(dtcp);

        LOG_DBG("Credit and RWE set: %u, %u", data->wc, rcvr_rt_wind_edge(dtcp));

        return 0;
}

static int
cas_rate_reduction(struct dtcp_ps * ps)
{
        printk("%s: called()\n", __func__);
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
        if (strcmp(name, "w_dec_b_num_p") == 0) {
                ret = kstrtoint(value, 10, &bool_value);
                if (!ret)
                        data->w_dec_b_num_p = bool_value;
        }
        if (strcmp(name, "w_dec_b_den_p") == 0) {
                ret = kstrtoint(value, 10, &bool_value);
                if (!ret)
                        data->w_dec_b_den_p = bool_value;
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

        dtcp_cfg                        = dtcp_config_get(dtcp);

        ps->base.set_policy_set_param   = dtcp_ps_cas_set_policy_set_param;
        ps->dm                          = dtcp;

        data->w_inc_a_p                 = W_INC_A_P_DEFAULT;
        data->w_dec_b_num_p             = W_DEC_B_NUM_P_DEFAULT;
        data->w_dec_b_den_p             = W_DEC_B_DEN_P_DEFAULT;
        data->wc                        = ps->flowctrl.window.initial_credit;
        data->wp                        = 0;
        data->wc_lwe                    = 0;
        data->rcv_count                 = 0;
        data->ecn_count                 = 0;
        ps->priv                        = data;

        ps_conf = dtcp_ps(dtcp_cfg);
        ps_param = policy_param_find(ps_conf, "w_inc_a_p");
        dtcp_ps_cas_set_policy_set_param(&ps->base,
                                         policy_param_name(ps_param),
                                         policy_param_value(ps_param));
        ps_param = policy_param_find(ps_conf, "w_dec_b_num_p");
        dtcp_ps_cas_set_policy_set_param(&ps->base,
                                        policy_param_name(ps_param),
                                        policy_param_value(ps_param));
        ps_param = policy_param_find(ps_conf, "w_dec_b_den_p");
        dtcp_ps_cas_set_policy_set_param(&ps->base,
                                        policy_param_name(ps_param),
                                        policy_param_value(ps_param));


        ps->flow_init                   = NULL;
        ps->lost_control_pdu            = cas_lost_control_pdu;
        ps->rtt_estimator               = NULL;
        ps->retransmission_timer_expiry = NULL;
        ps->received_retransmission     = NULL;
        ps->sender_ack                  = cas_sender_ack;
        ps->sending_ack                 = cas_sending_ack;
        ps->receiving_ack_list          = NULL;
        ps->initial_rate                = NULL;
        ps->receiving_flow_control      = cas_receiving_flow_control;
        ps->update_credit               = NULL;
        ps->reconcile_flow_conflict     = NULL;
        ps->rcvr_ack                    = cas_rcvr_ack,
        ps->rcvr_flow_control           = cas_rcvr_flow_control;
        ps->rate_reduction              = cas_rate_reduction;
        ps->rcvr_control_ack            = NULL;
        ps->no_rate_slow_down           = NULL;
        ps->no_override_default_peak    = NULL;

        return &ps->base;
}

static void dtcp_ps_cas_destroy(struct ps_base * bps)
{
        struct dtcp_ps *ps = container_of(bps, struct dtcp_ps, base);

        if (bps) {
                rkfree(ps);
        }
}

struct ps_factory dtcp_factory = {
        .owner          = THIS_MODULE,
        .create  = dtcp_ps_cas_create,
        .destroy = dtcp_ps_cas_destroy,
};
