/*
 * Default plugins for kernel components
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

#define RINA_PREFIX "rina-default-plugin"

#include "logs.h"
#include "rds/rmem.h"
#include "rmt-ps.h"

extern struct rmt_ps_factory rmt_factory;

static int __init mod_init(void)
{
        int ret;

        strcpy(rmt_factory.base.name, RINA_PS_DEFAULT_NAME);

        ret = rmt_ps_publish(&rmt_factory);
        if (ret) {
                LOG_ERR("Failed to publish RMT policy set factory");
                return -1;
        }

        LOG_INFO("RMT default policy set loaded successfully");

        return 0;
}

static void __exit mod_exit(void)
{
        int ret = rmt_ps_unpublish(RINA_PS_DEFAULT_NAME);

        if (ret) {
                LOG_ERR("Failed to unpublish RMT policy set factory");
                return;
        }

        LOG_INFO("RMT default policy set unloaded successfully");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("Default policy sets");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Vincenzo Maffione <v.maffione@nextworks.it>");
MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
