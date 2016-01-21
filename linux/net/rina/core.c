/*
 * Core
 *
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
#include <linux/init.h>
#include <linux/sysfs.h>

#define RINA_PREFIX "core"

#include "logs.h"
#include "debug.h"
#include "rnl.h"
#include "kipcm.h"
#include "utils.h"
#include "rds/robjects.h"

#define MK_RINA_VERSION(MAJOR, MINOR, MICRO)                            \
        (((MAJOR & 0xFF) << 24) | ((MINOR & 0xFF) << 16) | (MICRO & 0xFFFF))

#define RINA_VERSION_MAJOR(V) ((V >> 24) & 0xFF)
#define RINA_VERSION_MINOR(V) ((V >> 16) & 0xFF)
#define RINA_VERSION_MICRO(V) ((V      ) & 0xFFFF)

static struct robject core_object;
static uint32_t version = MK_RINA_VERSION(1, 4, 1);

static ssize_t core_attr_show(struct robject *        robj,
                              struct robj_attribute * attr,
                              char *                  buff)
{
	if (strcmp(robject_attr_name(attr), "version") == 0)
        	return sprintf(buff, "%d.%d.%d\n",
                       RINA_VERSION_MAJOR(version),
                       RINA_VERSION_MINOR(version),
                       RINA_VERSION_MICRO(version));
	return 0;
}

RINA_SYSFS_OPS(core);
RINA_ATTRS(core, version);
RINA_KTYPE(core);

static int __init rina_core_init(void)
{
        LOG_DBG("RINA stack initializing");

        if (rina_debug_init())
                return -1;

        LOG_DBG("Creating root rset");
        if (robject_init_and_add(&core_object, &core_rtype, NULL, "rina")) {
                LOG_ERR("Cannot initialize root rset, bailing out");
                rina_debug_exit();
                return -1;
	}
        LOG_DBG("Initializing RNL");
        if (rnl_init()) {
		robject_del(&core_object);
                rina_debug_exit();
                return -1;
        }

	LOG_DBG("Initializing KIPCM");
        if (kipcm_init(&core_object)) {
                rnl_exit();
		robject_del(&core_object);
                rina_debug_exit();
                return -1;
        }

        LOG_INFO("RINA stack v%d.%d.%d initialized",
                 RINA_VERSION_MAJOR(version),
                 RINA_VERSION_MINOR(version),
                 RINA_VERSION_MICRO(version));

        LOG_INFO("Don't panic ...");

        return 0;
}

__initcall(rina_core_init);
