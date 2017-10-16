/*
 * Dummy DTP PS
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

#define RINA_PREFIX "dummy-dtp-ps"

#include "rds/rmem.h"
#include "dtp-ps.h"

static int
dummy_transmission_control(struct dtp_ps * ps, struct du * du)
{
        printk("%s: called()\n", __func__);
        return 0;
}

static int
dummy_closed_window(struct dtp_ps * ps, struct du * du)
{
        printk("%s: called()\n", __func__);
        return 0;
}

static int
dummy_flow_control_overrun(struct dtp_ps * ps,
                             struct du *   du)
{
        printk("%s: called()\n", __func__);
        return 0;
}

static int
dummy_initial_sequence_number(struct dtp_ps * ps)
{
        printk("%s: called()\n", __func__);
        return 0;
}


static int
dummy_receiver_inactivity_timer(struct dtp_ps * ps)
{
        printk("%s: called()\n", __func__);
        return 0;
}

static int
dummy_sender_inactivity_timer(struct dtp_ps * ps)
{
        printk("%s: called()\n", __func__);
        return 0;
}

static int dtp_ps_set_policy_set_param(struct ps_base * bps,
                                       const char    * name,
                                       const char    * value)
{
        printk("%s: called()\n", __func__);
        return 0;
}

static struct ps_base *
dtp_ps_dummy_create(struct rina_component * component)
{
        struct dtp * dtp = dtp_from_component(component);
        struct dtp_ps * ps = rkzalloc(sizeof(*ps), GFP_KERNEL);

        if (!ps) {
                return NULL;
        }

        ps->base.set_policy_set_param = dtp_ps_set_policy_set_param;
        ps->dm              = dtp;
        ps->priv            = NULL;

        ps->transmission_control        = dummy_transmission_control;
        ps->closed_window               = dummy_closed_window;
        ps->snd_flow_control_overrun    = dummy_flow_control_overrun;
        ps->rcv_flow_control_overrun    = NULL;
        ps->initial_sequence_number     = dummy_initial_sequence_number;
        ps->receiver_inactivity_timer   = dummy_receiver_inactivity_timer;
        ps->sender_inactivity_timer     = dummy_sender_inactivity_timer;
        ps->reconcile_flow_conflict     = NULL;

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

static void dtp_ps_dummy_destroy(struct ps_base * bps)
{
        struct dtp_ps *ps = container_of(bps, struct dtp_ps, base);

        if (bps) {
                rkfree(ps);
        }
}

struct ps_factory dtp_factory = {
        .owner          = THIS_MODULE,
        .create  = dtp_ps_dummy_create,
        .destroy = dtp_ps_dummy_destroy,
};
