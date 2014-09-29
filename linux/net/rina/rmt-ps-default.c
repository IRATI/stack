/*
 * Default policy set for RMT
 *
 *    Vincenzo Maffione <v.maffione@nextworks.it>
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
#include "rmt-ps.h"


#define DEFAULT_NAME    "default"

static struct rmt_ps *
rmt_ps_default_create(struct rmt *rmt)
{
        return NULL;
}

static void
rmt_ps_default_destroy(struct rmt_ps *ps)
{
}

static struct rmt_ps_factory factory = {
        .base = {
                .parameters = NULL,
                .num_parameters = 0,
        },
        .create = rmt_ps_default_create,
        .destroy = rmt_ps_default_destroy,
};

static int __init mod_init(void)
{
        int ret;

        strcpy(factory.base.name, DEFAULT_NAME);

        ret = publish_rmt_ps(&factory);
        if (ret) {
                LOG_ERR("Failed to publish policy set factory");
                return -1;
        }

        LOG_INFO("RMT default policy set loaded successfully");

        return 0;
}

static void __exit mod_exit(void)
{
        int ret = unpublish_rmt_ps(DEFAULT_NAME);

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
