/*
 * Default policy set for RMT
 *
 *    Vincenzo Maffione <v.maffione@nextworks.it>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#define RINA_PREFIX "dtp-ps-default"

#include "logs.h"
#include "rds/rmem.h"
#include "dtp-ps.h"
#include "dtp.h"

static int
default_transmission_control(struct dtp_ps * ps, struct pdu * pdu)
{
        return 0;
}

static int
default_closed_window(struct dtp * instance, struct pdu * pdu)
{
        return 0;
}

static int
default_flow_control_overrun(struct dtp_ps * ps,
                             struct pdu * pdu)
{
        return 0;
}

static int
default_initial_sequence_number(struct dtp_ps * ps)
{
        return 0;
}


static int
default_receiver_inactivity_timer(struct dtp_ps * ps)
{
        return 0;
}

static int
default_sender_inactivity_timer(struct dtp_ps * ps)
{
        return 0;
}

static int dtp_ps_set_policy_set_param(struct ps_base * bps,
                                       const char    * name,
                                       const char    * value)
{
        struct dtp_ps *ps = container_of(bps, struct dtp_ps, base);

        (void) ps;

        if (!name) {
                LOG_ERR("Null parameter name");
                return -1;
        }

        if (!value) {
                LOG_ERR("Null parameter value");
                return -1;
        }

        LOG_ERR("No such parameter to set");

        return -1;
}

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
        ps->transmission_control = default_transmission_control;
        ps->closed_window = default_closed_window;
        ps->flow_control_overrun = default_flow_control_overrun;
        ps->initial_sequence_number = default_initial_sequence_number;
        ps->receiver_inactivity_timer = default_receiver_inactivity_timer;
        ps->sender_inactivity_timer = default_sender_inactivity_timer;

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
