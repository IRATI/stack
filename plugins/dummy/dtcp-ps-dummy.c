/*
 * Dummy DTCP PS
 *
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
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

#define RINA_PREFIX "dummy-dtcp-ps"

#include "rds/rmem.h"
#include "dtcp-ps.h"

static int
dummy_lost_control_pdu(struct dtcp_ps * ps)
{
        printk("%s: called()\n", __func__);
        return 0;
}

static int dummy_rcvr_ack(struct dtcp_ps * ps, const struct pci * pci)
{
        printk("%s: called()\n", __func__);
        return 0;
}

static int
dummy_sender_ack(struct dtcp_ps * ps, seq_num_t seq_num)
{
        printk("%s: called()\n", __func__);
        return 0;
}

static int
dummy_sending_ack(struct dtcp_ps * ps, seq_num_t seq)
{
        printk("%s: called()\n", __func__);
        return 0;
}

static int
dummy_receiving_flow_control(struct dtcp_ps * ps, const struct pci * pci)
{
        printk("%s: called()\n", __func__);
        return 0;
}

static int
dummy_rcvr_flow_control(struct dtcp_ps * ps, const struct pci * pci)
{
        printk("%s: called()\n", __func__);
        return 0;
}

static int
dummy_rate_reduction(struct dtcp_ps * ps, const struct pci * pci)
{
        printk("%s: called()\n", __func__);
        return 0;
}

static int dtcp_ps_set_policy_set_param(struct ps_base * bps,
                                       const char    * name,
                                       const char    * value)
{
        printk("%s: called()\n", __func__);
        return 0;
}

static struct ps_base *
dtcp_ps_dummy_create(struct rina_component * component)
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
        ps->lost_control_pdu            = dummy_lost_control_pdu;
        ps->rtt_estimator               = NULL;
        ps->retransmission_timer_expiry = NULL;
        ps->received_retransmission     = NULL;
        ps->sender_ack                  = dummy_sender_ack;
        ps->sending_ack                 = dummy_sending_ack;
        ps->receiving_ack_list          = NULL;
        ps->initial_rate                = NULL;
        ps->receiving_flow_control      = dummy_receiving_flow_control;
        ps->update_credit               = NULL;
        ps->rcvr_ack                    = dummy_rcvr_ack,
        ps->rcvr_flow_control           = dummy_rcvr_flow_control;
        ps->rate_reduction              = dummy_rate_reduction;
        ps->rcvr_control_ack            = NULL;
        ps->no_rate_slow_down           = NULL;
        ps->no_override_default_peak    = NULL;

        return &ps->base;
}

static void dtcp_ps_dummy_destroy(struct ps_base * bps)
{
        struct dtcp_ps *ps = container_of(bps, struct dtcp_ps, base);

        if (bps) {
                rkfree(ps);
        }
}

struct ps_factory dtcp_factory = {
        .owner          = THIS_MODULE,
        .create  = dtcp_ps_dummy_create,
        .destroy = dtcp_ps_dummy_destroy,
};
