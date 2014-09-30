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

static void default_max_q_policy_tx(struct rmt_ps * ps,
                                    struct pdu *    pdu,
                                    struct rfifo *  queue)
{ }

static void default_max_q_policy_rx(struct rmt_ps * ps,
                                    struct sdu *    sdu,
                                    struct rfifo *  queue)
{ }

static struct rmt_ps * rmt_ps_default_create(struct rmt *rmt)
{
        struct rmt_ps * ps = rkzalloc(sizeof(*ps), GFP_KERNEL);
        if (!ps) {
             return ps;
        }

        ps->dm              = rmt;
        ps->max_q           = 256;
        ps->priv            = NULL;
        ps->max_q_policy_tx = default_max_q_policy_tx;
        ps->max_q_policy_rx = default_max_q_policy_rx;

        return ps;
}

static void rmt_ps_default_destroy(struct rmt_ps * ps)
{
        if (ps)
                rkfree(ps);
}

static struct rmt_ps_factory factory = {
        .base = {
                .parameters     = NULL,
                .num_parameters = 0,
        },
        .create  = rmt_ps_default_create,
        .destroy = rmt_ps_default_destroy,
};

static int __init mod_init(void)
{
        int ret;

        strcpy(factory.base.name, DEFAULT_NAME);

        ret = rmt_ps_publish(&factory);
        if (ret) {
                LOG_ERR("Failed to publish policy set factory");
                return -1;
        }

        LOG_INFO("RMT default policy set loaded successfully");

        return 0;
}

static void __exit mod_exit(void)
{
        int ret = rmt_ps_unpublish(DEFAULT_NAME);

        if (ret) {
                LOG_ERR("Failed to unpublish policy set factory");
                return;
        }

        LOG_INFO("RMT default policy set unloaded successfully");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RMT default policy set");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Vincenzo Maffione <v.maffione@nextworks.it>");
MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
