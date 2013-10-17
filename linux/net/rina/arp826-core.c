/*
 * ARP 826 (wonnabe) core
 *
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
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
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/if_ether.h>

/* FIXME: The following dependencies have to be removed */
#define RINA_PREFIX "arp826-core"

#include "logs.h"
#include "debug.h"
#include "utils.h"
/* FIXME: End of dependencies ... */

#include "arp826.h"
#include "arp826-utils.h"
#include "arp826-rxtx.h"
#include "arp826-arm.h"
#include "arp826-tables.h"

static struct packet_type arp826_packet_type __read_mostly = {
        .type = cpu_to_be16(ETH_P_ARP),
        .func = arp_receive,
};

static int protocol_add(uint16_t ptype,
                        size_t   hlen)
{
        struct protocol * p;

        LOG_DBG("Adding protocol 0x%02x, hlen = %zd", ptype, hlen);
      
        if (tbls_create(ptype, hlen)) {
                return -1;
        }

        LOG_DBG("Protocol type 0x%02x added successfully", ptype);

        return 0;
}

static void protocol_remove(uint16_t ptype)
{
        tbls_destroy(ptype);
}

static int __init mod_init(void)
{
        LOG_DBG("Initializing");

	dev_add_pack(&arp826_packet_type);

        if (tbls_init())
                return -1;

        if (arm_init()) {
                tbls_fini();
                return -1;
        }

        if (!protocol_add(ETH_P_RINA, 6)) {
                tbls_fini();
                arm_fini();
                return -1;
        }

        LOG_DBG("Initialized successfully");

        return 0;
}

static void __exit mod_exit(void)
{
        LOG_DBG("Finalizing");

	dev_remove_pack(&arp826_packet_type);

        protocol_remove(ETH_P_RINA);

        arm_fini();
        tbls_fini();

        LOG_DBG("Finalized successfully");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("Basic RFC 826 compliant ARP implementation");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Sander Vrijders <sander.vrijders@intec.ugent.be>");
MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
