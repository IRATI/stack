/*
 * Debugging facilities
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

#include <linux/debugfs.h>

#define RINA_PREFIX "debug"

#include "logs.h"

#if CONFIG_RINA_DEBUG

#if CONFIG_DEBUG_FS
static struct dentry * dfs_root = NULL;
#endif

int rina_debug_init()
{
#if CONFIG_DEBUG_FS
        if (dfs_root) {
                LOG_ERR("Debugging facilities already initialized, "
                        "bailing out");
                return -1;
        }

        dfs_root = debugfs_create_dir("rina", NULL);
        if (!dfs_root) {
                LOG_DBG("Cannot setup debugfs facilities");
                return -1;
        }
#endif

        LOG_DBG("Debugging facilities initialized successfully");

        return 0;
}

void rina_debug_exit()
{
        LOG_DBG("Debugging facilities finalizing");

#if CONFIG_DEBUG_FS
        if (!dfs_root) {
                LOG_DBG("Debugging facilities not correctly initialized, "
                        "bailing out");
                return;
        }

        debugfs_remove_recursive(dfs_root);
#endif

        LOG_DBG("Debugging facilities finalized successfully");
}

#else

int rina_debug_init()
{ return 0; }

void rina_debug_exit()
{ }

#endif
