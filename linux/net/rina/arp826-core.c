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
#include "arp826-cache.h"

enum arp826_optypes {
        ARP_REQUEST = 1,
        ARP_REPLY   = 2,
};

enum arp826_htypes {
        HW_TYPE_ETHER = 1,
        HW_TYPE_MAX,
};

struct cache_line * cache_lines[HW_TYPE_MAX - 1] = { NULL };

static bool is_line_id_ok(int line)
{ return (line < HW_TYPE_ETHER - 1 || line >= HW_TYPE_MAX - 1) ? 0 : 1; }

static struct arp_header * arp826_header(const struct sk_buff * skb)
{ return (struct arp_header *) skb_network_header(skb); }

static int arp826_process(struct sk_buff *    skb,
                          struct cache_line * cl)
{
        struct arp_header * header;
        uint16_t            operation;

        uint16_t            htype;
        uint16_t            ptype;
        uint8_t             hlen;
        uint8_t             plen;
        uint16_t            oper;

        uint8_t *           ptr;

        uint8_t *           spa; /* Source protocol address pointer */
        uint8_t *           tpa; /* Target protocol address pointer */
        uint8_t *           sha; /* Source protocol address pointer */
        uint8_t *           tha; /* Target protocol address pointer */
        
        ASSERT(skb);
        ASSERT(cl);

        LOG_DBG("Processing ARP skb %pK", skb);
        
        header = arp826_header(skb);
        if (!header) {
                LOG_ERR("Cannot get the header");
                return 0;
        }

        htype = ntohs(header->htype);
        ptype = ntohs(header->ptype);
        hlen  = header->hlen;
        plen  = header->plen;
        oper  = ntohs(header->oper);

        LOG_DBG("ARP header:");
        LOG_DBG("  Hardware type           = 0x%02x", htype);
        LOG_DBG("  Protocol type           = 0x%02x", ptype);
        LOG_DBG("  Hardware address length = %d",     hlen);
        LOG_DBG("  Protocol address length = %d",     plen);
        LOG_DBG("  Operation               = 0x%02x", oper);

        /*
         * FIXME: Check if we know the protocol specified. Only accept RINA
         *        packets for the time being. Others will be added later ...
         */
#if 0
        if (header->ptype != htons(ETH_P_RINA)) {
                LOG_ERR("Unknown protocol type 0x%02x", header->ptype);
                return 0;
        }
#endif

        if (header->htype != htons(HW_TYPE_ETHER)) {
                LOG_ERR("Unhandled ARP hardware type 0x%02x", header->htype);
                return 0;
        }

        operation = ntohs(header->oper);
        if (operation != ARP_REPLY && operation != ARP_REQUEST) {
                LOG_ERR("Unhandled ARP operation 0x%02x", operation);
                return 0;
        }

        /* Hooray, we can handle this ARP (probably ...) */

        ptr = (uint8_t *) header + 8;

        sha = ptr; ptr += header->hlen;
        spa = ptr; ptr += header->plen;
        tha = ptr; ptr += header->hlen;
        tpa = ptr; ptr += header->plen;

        if (cl_add(cl, gpa_create(spa, header->plen), sha)) {
                LOG_ERR("Could not add this entry to its cache-line");
                return -1;
        }

        /*
         *  And finally process the entry ...
         *
         *  The idea is that we want to send a reply if the request is for us.
         *  We want to add an entry to our cache if it is a reply to us or if
         *  it is a request for one of our addresses.
         *
         *  Putting this another way, we only care about replies if
         *  they are to us, in which case we add them to the cache.
         *  For requests, we care about those for us. We add the
         *  requester to the arp cache.
         */

        switch (operation) {
        case ARP_REQUEST:
                /* Are we advertising this network address? */

                LOG_MISSING;
                break;
        case ARP_REPLY:
                /* Is the reply for one of our network addresses? */

                LOG_MISSING;
                break;
        default:
                BUG();
        }

        /* Finally, update the ARP cache */
                
        return 0;
}

/* NOTE: The following function uses a different mapping for return values */
static int arp826_receive(struct sk_buff *     skb,
                          struct net_device *  dev,
                          struct packet_type * pkt,
                          struct net_device *  orig_dev)
{
        const struct arp_header * header;
        int                       total_length;
        struct cache_line *       cl;
        int                       line_id;

        if (!dev || !skb) {
                LOG_ERR("Wrong device or skb");
                return 0;
        }

        if (dev->flags & IFF_NOARP            ||
            skb->pkt_type == PACKET_OTHERHOST ||
            skb->pkt_type == PACKET_LOOPBACK) {
                kfree_skb(skb);
                LOG_DBG("This ARP is not for us "
                        "(no arp, other-host or loopback)");
                return 0;
        }

        /* We only receive type-1 headers (this handler could be reused) */
        if (skb->dev->type != HW_TYPE_ETHER) {
                LOG_DBG("Unhandled device type %d", skb->dev->type);
                return 0;
        }
        line_id = skb->dev->type - 1;
        if (!is_line_id_ok(line_id)) {
                LOG_ERR("Wrong cache line %d", line_id);
                return 0;
        }

        cl = cache_lines[line_id];
        if (!cl) {
                LOG_ERR("I don't have a CL to handle this ARP");
                return 0;
        }

        /* FIXME: We should move pre-checks from arp826_process() here ... */

        skb = skb_share_check(skb, GFP_ATOMIC);
        if (!skb) {
                LOG_ERR("This ARP cannot be shared!");
                return 0;
        }

        /* ARP header, without 2 device and 2 network addresses */
        if (!pskb_may_pull(skb, sizeof(struct arp_header))) {
                LOG_WARN("Got an ARP header "
                         "without 2 devices and 2 network addresses "
                         "(step #1)");
                kfree_skb(skb);
                return 0;
        }

        header = arp826_header(skb);
        if (header->hlen != dev->addr_len) {
                LOG_WARN("Cannot process this ARP");
                kfree_skb(skb);
                return 0;
        }

        total_length = sizeof(struct arp_header) +
                (dev->addr_len + header->plen) * 2;

        /* ARP header, with 2 device and 2 network addresses */
        if (!pskb_may_pull(skb, total_length)) {
                LOG_WARN("Got an ARP header "
                         "without 2 devices and 2 network addresses "
                         "(step #2)");
                kfree_skb(skb);
                return 0;
        }

        if (arp826_process(skb, cl)) {
                LOG_ERR("Cannot process this ARP");
                return 0;
        }
        consume_skb(skb);

        return 0;
}

static struct packet_type arp_packet_type __read_mostly = {
        .type = cpu_to_be16(ETH_P_ARP),
        .func = arp826_receive,
};

static int cache_line_create(int idx, size_t hwlen)
{
        cache_lines[idx] = cl_create(hwlen);
        if (!cache_lines[idx]) {
                LOG_ERR("Cannot intialize CL on index %d, bailing out", idx);
                return -1;
        }
        LOG_DBG("CL on index %d created successfully", idx);

        return 0;
}

static void cache_line_destroy(int idx)
{
        if (cache_lines[idx])
                cl_destroy(cache_lines[idx]);
}

static int __init mod_init(void)
{
        /* MAC */
        if (cache_line_create(HW_TYPE_ETHER - 1, 6))
                return -1;

        /* That's all */
        dev_add_pack(&arp_packet_type);

        LOG_DBG("Initialized successfully");

        return 0;
}

static void __exit mod_exit(void)
{
        dev_remove_pack(&arp_packet_type);

        cache_line_destroy(HW_TYPE_ETHER - 1);

        LOG_DBG("Destroyed successfully");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("Basic RFC 826 compliant ARP implementation");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Sander Vrijders <sander.vrijders@intec.ugent.be>");
MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
