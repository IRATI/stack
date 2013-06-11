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

#include <linux/module.h>
#include <linux/init.h>

#define RINA_PREFIX "core"

#include "logs.h"
#include "rina.h"
#include "sysfs.h"
#include "netlink.h"
#include "personality.h"

static uint32_t version = MK_RINA_VERSION(0, 0, 0);

uint32_t rina_version(void)
{ return version; }

static int __init rina_core_init(void)
{
        LOG_FBEGN;

        LOG_INFO("RINA stack v%d.%d.%d initializing",
                 RINA_VERSION_MAJOR(version),
                 RINA_VERSION_MINOR(version),
                 RINA_VERSION_MICRO(version));

        if (!rina_personality_init()) {
                LOG_CRIT("Could not initialize personality");
                return -1;
        }
#ifdef CONFIG_RINA_SYSFS
        if (!rina_sysfs_init()) {
                LOG_CRIT("Could not initialize sysfs");
                rina_personality_exit();
        }
#endif
        if (!rina_netlink_init()) {
                LOG_CRIT("Could not initialize netlink");
                rina_sysfs_exit();
                rina_personality_exit();                
        }

        LOG_FEXIT;

        return 0;
}

__initcall(rina_core_init);
