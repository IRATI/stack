/*
 * Dummy Kernel plugin with whole set of policy sets (RMT, PFT, DTP, DTCP)
 *
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

#define RINA_PREFIX "pff-multipath"
#define RINA_PFF_MULTIPATH_NAME "multipath"

#include "logs.h"
#include "rds/rmem.h"
#include "pff-ps.h"

extern struct ps_factory pff_factory;

static int __init mod_init(void)
{
        int ret;

	strcpy(pff_factory.name, RINA_PFF_MULTIPATH_NAME);

        ret = pff_ps_publish(&pff_factory);
        if (ret) {
                LOG_ERR("Failed to publish PFT policy set factory");
                return -1;
        }

        LOG_INFO("PFF multipath policy set loaded successfully");

        return 0;
}

static void __exit mod_exit(void)
{
        int ret;

        ret = pff_ps_unpublish(RINA_PFF_MULTIPATH_NAME);
        if (ret) {
                LOG_ERR("Failed to unpublish Dummy PFT policy set factory");
                return;
        }

        LOG_INFO("PFF multipath policy set unloaded successfully");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_AUTHOR("Javier García <javier.garcial.external@atos.net>");
MODULE_LICENSE ("GPL");
MODULE_DESCRIPTION("Multipath routing policy sets");

