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

#define RINA_PREFIX "rmt-ps-default"

#include "logs.h"
#include "rds/rmem.h"
#include "rmt-ps.h"
#include "rmt.h"

static void default_max_q_policy_tx(struct rmt_ps * ps,
                                    struct pdu *    pdu,
                                    struct rfifo *  queue)
{ }

static void default_max_q_policy_rx(struct rmt_ps * ps,
                                    struct sdu *    sdu,
                                    struct rfifo *  queue)
{ }

static void default_rmt_q_monitor_policy_tx(struct rmt_ps * ps,
                                    struct pdu *    pdu,
                                    struct rfifo *  queue)
{ }

static void default_rmt_q_monitor_policy_rx(struct rmt_ps * ps,
                                    struct sdu *    sdu,
                                    struct rfifo *  queue)
{ }

static int rmt_ps_set_policy_set_param(struct ps_base * bps,
                                       const char    * name,
                                       const char    * value)
{
        struct rmt_ps *ps = container_of(bps, struct rmt_ps, base);

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
rmt_ps_default_create(struct rina_component * component)
{
        struct rmt * rmt = rmt_from_component(component);
        struct rmt_ps * ps = rkzalloc(sizeof(*ps), GFP_KERNEL);

        if (!ps) {
                return NULL;
        }

        ps->base.set_policy_set_param = rmt_ps_set_policy_set_param;
        ps->dm              = rmt;
        ps->max_q           = 256;
        ps->priv            = NULL;
        ps->max_q_policy_tx = default_max_q_policy_tx;
        ps->max_q_policy_rx = default_max_q_policy_rx;
        ps->rmt_q_monitor_policy_tx = default_rmt_q_monitor_policy_tx;
        ps->rmt_q_monitor_policy_rx = default_rmt_q_monitor_policy_rx;

        return &ps->base;
}

static void rmt_ps_default_destroy(struct ps_base * bps)
{
        struct rmt_ps *ps = container_of(bps, struct rmt_ps, base);

        if (bps) {
                rkfree(ps);
        }
}

struct ps_factory rmt_factory = {
        .owner          = THIS_MODULE,
        .create  = rmt_ps_default_create,
        .destroy = rmt_ps_default_destroy,
};
