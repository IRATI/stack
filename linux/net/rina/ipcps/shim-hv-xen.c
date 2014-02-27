/*
 *  Shim IPC Process for Hypervisors (using XEN)
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
#include <linux/kernel.h>
#include <linux/if_ether.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/netdevice.h>
#include <linux/if_packet.h>
#include <linux/workqueue.h>

#define SHIM_NAME   "shim-hv-xen"

#define RINA_PREFIX SHIM_NAME

#include "logs.h"
#include "common.h"
#include "kipcm.h"
#include "debug.h"
#include "utils.h"
#include "du.h"
#include "ipcp-utils.h"
#include "ipcp-factories.h"

/* FIXME: To be removed ABSOLUTELY */
extern struct kipcm * default_kipcm;

static int shim_init(struct ipcp_factory_data * data)
{
        LOG_MISSING;

        return -1;
}

static int shim_fini(struct ipcp_factory_data * data)
{
        LOG_MISSING;

        return 0;
}

static struct ipcp_instance * shim_create(struct ipcp_factory_data * data,
                                          const struct name *        name,
                                          ipc_process_id_t           id)
{
        LOG_MISSING;

        return NULL;
}

static int shim_destroy(struct ipcp_factory_data * data,
                        struct ipcp_instance *     instance)
{
        LOG_MISSING;

        return -1;
}

static struct ipcp_factory_ops shim_ops = {
        .init      = shim_init,
        .fini      = shim_fini,
        .create    = shim_create,
        .destroy   = shim_destroy,
};

static struct ipcp_factory_data {
        int empty;
} shim_data;

static struct ipcp_factory * shim = NULL;

static int __init mod_init(void)
{
        shim = kipcm_ipcp_factory_register(default_kipcm,
                                           SHIM_NAME,
                                           &shim_data,
                                           &shim_ops);
        if (!shim)
                return -1;

        return 0;
}

static void __exit mod_exit(void)
{
        ASSERT(shim);

        kipcm_ipcp_factory_unregister(default_kipcm, shim);
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RINA Shim IPC for Hypervisors (using XEN)");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
