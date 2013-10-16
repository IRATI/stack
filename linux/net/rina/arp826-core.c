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

struct protocol {
        struct packet_type * packet;
        struct list_head     next;
};

static struct protocol *
protocol_create(uint16_t ptype,
                size_t   hlen,
                int   (* receiver)(struct sk_buff *     skb,
                                   struct net_device *  dev,
                                   struct packet_type * pkt,
                                   struct net_device *  orig_dev))
{
        struct protocol * p;

        if (!receiver) {
                LOG_ERR("Bad input parameters, "
                        "cannot create protocol %d", ptype);
                return NULL;
        }

        p = rkzalloc(sizeof(*p), GFP_KERNEL);
        if (!p)
                return NULL;
        p->packet = rkzalloc(sizeof(*p->packet), GFP_KERNEL);
        if (!p->packet) {
                rkfree(p);
                return NULL;
        }
        
        p->packet->type = cpu_to_be16(ptype);
        p->packet->func = receiver;
        INIT_LIST_HEAD(&p->next);

        return p;
}

static void protocol_destroy(struct protocol * p)
{
        ASSERT(p);
        ASSERT(p->packet);

        rkfree(p->packet);
        rkfree(p);
}

static spinlock_t       protocols_lock;
static struct list_head protocols;

static int protocol_add(uint16_t ptype,
                        size_t   hlen)
{
        struct protocol * p = protocol_create(ptype, hlen, arp_receive);

        if (!p) {
                LOG_ERR("Cannot add protocol type %d", ptype);
                return -1;
        }

        if (tbls_create(ptype, hlen)) {
                protocol_destroy(p);
                return -1;
        }

        dev_add_pack(p->packet);

        spin_lock(&protocols_lock);
        list_add(&protocols, &p->next); 
        spin_unlock(&protocols_lock);

        LOG_DBG("Protocol type %d added successfully", ptype);

        return 0;
}

static void protocol_remove(uint16_t ptype)
{
        struct protocol * pos, * q;
        struct protocol * p;

        p = NULL;
        
        spin_lock(&protocols_lock);
        list_for_each_entry_safe(pos, q, &protocols, next) {
                ASSERT(pos);
                ASSERT(pos->packet);

                if (be16_to_cpu(pos->packet->type) == ptype) {
                        p = pos;
                        list_del(&pos->next);
                        break;
                }
        }
        spin_unlock(&protocols_lock);

        if (!p) {
                LOG_ERR("Cannot remove protocol type %d, it's unknown", ptype);
                return;
        }

        ASSERT(p);
        ASSERT(p->packet);

        dev_remove_pack(p->packet);
        tbls_destroy(ptype);

        protocol_destroy(p);
}

static int __init mod_init(void)
{
        LOG_DBG("Initializing");

        spin_lock_init(&protocols_lock);

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
