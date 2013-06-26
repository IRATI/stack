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
#include <linux/kobject.h>
#include <linux/sysfs.h>

#define RINA_PREFIX "core"

#include "logs.h"
#include "debug.h"
#include "rina.h"
#include "netlink.h"
#include "personality.h"

static uint32_t      version   = MK_RINA_VERSION(0, 0, 0);
static struct kset * root_kset = NULL;

uint32_t rina_version(void)
{ return version; }
EXPORT_SYMBOL(rina_version);

#if CONFIG_RINA_SYSFS
#if 0
static ssize_t version_show(struct kobject *        kobj,
                            struct kobj_attribute * attr,
                            char *                  buff)
{
        return sprintf(buff, "%d.%d.%d\n",
                       RINA_VERSION_MAJOR(version),
                       RINA_VERSION_MINOR(version),
                       RINA_VERSION_MICRO(version));
}

static struct kobj_attribute version_attr = __ATTR_RO(version);
#endif
#endif

static int __init rina_core_init(void)
{
        LOG_FBEGN;

        LOG_INFO("RINA stack initializing");

        if (rina_debug_init())
                return -1;

        /* FIXME: Move the set path from kernel to rina (subsys) */
        root_kset = kset_create_and_add("rina", NULL, kernel_kobj);
        if (!root_kset) {
                LOG_ERR("Cannot initialize root kset, bailing out");
                rina_debug_exit();
                return -1;
        }

        /*
         * FIXME: Move to default personality OR add multiplexer based on
         * personality identifiers ...
         */
        if (rina_netlink_init()) {
                kset_unregister(root_kset);
                rina_debug_exit();
                return -1;
        }

        if (rina_personality_init(&root_kset->kobj)) {
                rina_netlink_exit();
                kset_unregister(root_kset);
                rina_debug_exit();
                return -1;
        }

        LOG_INFO("RINA stack v%d.%d.%d initialized",
                 RINA_VERSION_MAJOR(version),
                 RINA_VERSION_MINOR(version),
                 RINA_VERSION_MICRO(version));

        LOG_FEXIT;

        return 0;
}

__initcall(rina_core_init);
