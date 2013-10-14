/*
 *  Dummy Shim IPC Process
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
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
#include <linux/list.h>

#define IPCP_NAME   "normal-ipc"

#define RINA_PREFIX IPCP_NAME

#include "logs.h"
#include "common.h"
#include "debug.h"
#include "utils.h"
#include "kipcm.h"
#include "ipcp-utils.h"
#include "ipcp-factories.h"
#include "du.h"
#include "kfa.h"
#include "rnl-utils.h"

struct ipcp_factory_data {
        struct list_head instances;
};

static struct ipcp_factory_data normal_data;

/* FIXME: To be removed ABSOLUTELY */
extern struct kipcm * default_kipcm;

static struct ipcp_factory_ops normal_ops = {
        .init      = NULL,
        .fini      = NULL,
        .create    = NULL,
        .destroy   = NULL,
};

struct ipcp_factory * normal = NULL;

static int __init mod_init(void)
{
        normal = kipcm_ipcp_factory_register(default_kipcm,
                                           IPCP_NAME,
                                           &normal_data,
                                           &normal_ops);
        if (!normal) {
                LOG_CRIT("Cannot register %s factory", IPCP_NAME);
                return -1;
        }

        return 0;
}

static void __exit mod_exit(void)
{
        ASSERT(normal);

        if (kipcm_ipcp_factory_unregister(default_kipcm, normal)) {
                LOG_CRIT("Cannot unregister %s factory", IPCP_NAME);
                return;
        }
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RINA Dummy Shim IPC");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
MODULE_AUTHOR("Miquel Tarzan <miquel.tarzan@i2cat.net>");
MODULE_AUTHOR("Sander Vrijders <sander.vrijders@intec.ugent.be>");
MODULE_AUTHOR("Leonardo Bergesio     <leonardo.bergesio@i2cat.net>");
