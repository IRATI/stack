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

#include "rds/rmem.h"
#include "dtp-ps.h"
#include "dtp.h"
#include "dtp-ps-common.h"

static int
default_transmission_control(struct dtp_ps * ps, struct pdu * pdu)
{ return common_transmission_control(ps, pdu); }

static int
default_closed_window(struct dtp_ps * ps, struct pdu * pdu)
{ return common_closed_window(ps, pdu); }

static int
default_flow_control_overrun(struct dtp_ps * ps,
                             struct pdu *    pdu)
{ return common_flow_control_overrun(ps, pdu); }

static int
default_initial_sequence_number(struct dtp_ps * ps)
{ return common_initial_sequence_number(ps); }


static int
default_receiver_inactivity_timer(struct dtp_ps * ps)
{ return common_receiver_inactivity_timer(ps); }

static int
default_sender_inactivity_timer(struct dtp_ps * ps)
{ return common_sender_inactivity_timer(ps); }

static int dtp_ps_set_policy_set_param(struct ps_base * bps,
                                       const char    * name,
                                       const char    * value)
{ return dtp_ps_common_set_policy_set_param(bps, name, value); }

static struct ps_base *
dtp_ps_default_create(struct rina_component * component)
{
        struct dtp * dtp = dtp_from_component(component);
        struct dtp_ps * ps = rkzalloc(sizeof(*ps), GFP_KERNEL);

        if (!ps) {
                return NULL;
        }

        ps->base.set_policy_set_param = dtp_ps_set_policy_set_param;
        ps->dm              = dtp;
        ps->priv            = NULL;

        ps->transmission_control        = default_transmission_control;
        ps->closed_window               = default_closed_window;
        ps->flow_control_overrun        = default_flow_control_overrun;
        ps->initial_sequence_number     = default_initial_sequence_number;
        ps->receiver_inactivity_timer   = default_receiver_inactivity_timer;
        ps->sender_inactivity_timer     = default_sender_inactivity_timer;

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

static void dtp_ps_default_destroy(struct ps_base * bps)
{
        struct dtp_ps *ps = container_of(bps, struct dtp_ps, base);

        if (bps) {
                rkfree(ps);
        }
}

struct ps_factory dtp_factory = {
        .owner          = THIS_MODULE,
        .create  = dtp_ps_default_create,
        .destroy = dtp_ps_default_destroy,
};
