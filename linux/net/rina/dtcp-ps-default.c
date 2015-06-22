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
#include "dtcp-ps.h"
#include "dtcp-ps-common.h"

static int
default_lost_control_pdu(struct dtcp_ps * ps)
{ return common_lost_control_pdu(ps); }

#ifdef CONFIG_RINA_DTCP_RCVR_ACK
static int default_rcvr_ack(struct dtcp_ps * ps, const struct pci * pci)
{ return common_rcvr_ack(ps, pci); }
#endif

#ifdef CONFIG_RINA_DTCP_RCVR_ACK_ATIMER
static int default_rcvr_ack_atimer(struct dtcp_ps * ps, const struct pci * pci)
{ return common_rcvr_ack_atimer(ps, pci); }
#endif

static int
default_sender_ack(struct dtcp_ps * ps, seq_num_t seq_num)
{ return common_sender_ack(ps, seq_num); }

static int
default_sending_ack(struct dtcp_ps * ps, seq_num_t seq)
{ return common_sending_ack(ps, seq); }

static int
default_receiving_flow_control(struct dtcp_ps * ps, const struct pci * pci)
{ return common_receiving_flow_control(ps, pci); }

static int
default_rcvr_flow_control(struct dtcp_ps * ps, const struct pci * pci)
{ return common_rcvr_flow_control(ps, pci); }

static int
default_rate_reduction(struct dtcp_ps * ps)
{ return common_rate_reduction(ps); }

static int
default_rtt_estimator(struct dtcp_ps * ps, seq_num_t sn)
{ return common_rtt_estimator(ps, sn); }

static int dtcp_ps_set_policy_set_param(struct ps_base * bps,
                                       const char    * name,
                                       const char    * value)
{ return dtcp_ps_common_set_policy_set_param(bps, name, value); }

static struct ps_base *
dtcp_ps_default_create(struct rina_component * component)
{
        struct dtcp * dtcp = dtcp_from_component(component);
        struct dtcp_ps * ps = rkzalloc(sizeof(*ps), GFP_KERNEL);

        if (!ps) {
                return NULL;
        }

        ps->base.set_policy_set_param   = dtcp_ps_set_policy_set_param;
        ps->dm                          = dtcp;
        ps->priv                        = NULL;
        ps->flow_init                   = NULL;
        ps->lost_control_pdu            = default_lost_control_pdu;
        ps->rtt_estimator               = default_rtt_estimator;
        ps->retransmission_timer_expiry = NULL;
        ps->received_retransmission     = NULL;
        ps->sender_ack                  = default_sender_ack;
        ps->sending_ack                 = default_sending_ack;
        ps->receiving_ack_list          = NULL;
        ps->initial_rate                = NULL;
        ps->receiving_flow_control      = default_receiving_flow_control;
        ps->update_credit               = NULL;
        ps->reconcile_flow_conflict     = NULL;
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

        return &ps->base;
}

static void dtcp_ps_default_destroy(struct ps_base * bps)
{
        struct dtcp_ps *ps = container_of(bps, struct dtcp_ps, base);

        if (bps) {
                rkfree(ps);
        }
}

struct ps_factory dtcp_factory = {
        .owner          = THIS_MODULE,
        .create  = dtcp_ps_default_create,
        .destroy = dtcp_ps_default_destroy,
};
