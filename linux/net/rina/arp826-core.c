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

static struct arp_header * header_get(const struct sk_buff * skb)
{ return (struct arp_header *) skb_network_header(skb); }

static int process(const struct sk_buff * skb,
                   struct table *         cl)
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

	struct gpa *        tmp_spa;
	struct gha *        tmp_sha;
	struct gpa *        tmp_tpa;
	struct gha *        tmp_tha;

        ASSERT(skb);
        ASSERT(cl);

        LOG_DBG("Processing ARP skb %pK", skb);
        
        header = header_get(skb);
        if (!header) {
                LOG_ERR("Cannot get the header");
                return -1;
        }

        htype = ntohs(header->htype);
        ptype = ntohs(header->ptype);
        hlen  = header->hlen;
        plen  = header->plen;
        oper  = ntohs(header->oper);

        LOG_DBG("Decoded ARP header:");
        LOG_DBG("  Hardware type           = 0x%02x", htype);
        LOG_DBG("  Protocol type           = 0x%02x", ptype);
        LOG_DBG("  Hardware address length = %d",     hlen);
        LOG_DBG("  Protocol address length = %d",     plen);
        LOG_DBG("  Operation               = 0x%02x", oper);

        if (header->htype != htons(HW_TYPE_ETHER)) {
                LOG_ERR("Unhandled ARP hardware type 0x%02x", header->htype);
                return -1;
        }
        if (hlen != 6) {
                LOG_ERR("Unhandled ARP hardware address length (%d)", hlen);
                return -1;
        }

        operation = ntohs(header->oper);
        if (operation != ARP_REPLY && operation != ARP_REQUEST) {
                LOG_ERR("Unhandled ARP operation 0x%02x", operation);
                return -1;
        }

        /* Hooray, we can handle this ARP (probably ...) */

        ptr = (uint8_t *) header + 8;

        sha = ptr; ptr += header->hlen;
        spa = ptr; ptr += header->plen;
        tha = ptr; ptr += header->hlen;
        tpa = ptr; ptr += header->plen;

	tmp_spa = gpa_create(spa, plen);
	tmp_sha = gha_create(MAC_ADDR_802_3, sha);
	tmp_tpa = gpa_create(tpa, plen);
	tmp_tha = gha_create(MAC_ADDR_802_3, tha);

#ifdef CONFIG_RINARP
        if (gpa_address_shrink(tmp_spa, 0x00)) {
                LOG_ERR("Problems parsing the source GPA");
                return -1;
        }
        if (gpa_address_shrink(tmp_tpa, 0x00)) {
                LOG_ERR("Got problems parsing the target GPA");
                return -1;
        }
#endif

        /* Finally process the entry */
        switch (operation) {
        case ARP_REQUEST: {
		struct table *             tbl;
		const struct table_entry * entry;
		const struct table_entry * req_addr;
		const struct gha *         target_ha;

		/* FIXME: Should we add all ARP Requests? */
                /* Do we have it in the cache ? */
		tbl   = tbls_find(ptype);
		entry = tbl_find_by_gpa(tbl, tmp_spa);
		
                if (!entry) {
			if (tbl_add(tbl, tmp_spa, tmp_sha)) {
				LOG_ERR("Bollocks. Can't add in table.");
				return -1;
			}
		} else {
			if (tbl_update_by_gpa(tbl, tmp_spa, tmp_sha))
				LOG_ERR("Failed to update table");
				return -1;
		}
          
		req_addr  = tbl_find_by_gpa(tbl, tmp_tpa);
		target_ha = tble_ha(req_addr);

                if (arp_send_reply(ptype,
                                   tmp_tpa, tmp_tha, tmp_spa, tmp_sha)) {
                        /* FIXME: Couldn't send reply ... */
                        return -1;
                }
         }
                break;

        case ARP_REPLY: {
                if (arm_resolve(ptype, tmp_spa, tmp_sha, tmp_tpa, tmp_tha)) {
                        LOG_ERR("Cannot resolve with this reply ...");
                        return -1;
                }
        }
                break;

        default:
                BUG();
        }

        return 0;
}

/* NOTE: The following function uses a different mapping for return values */
static int arp_receive(struct sk_buff *     skb,
                       struct net_device *  dev,
                       struct packet_type * pkt,
                       struct net_device *  orig_dev)
{
        const struct arp_header * header;
        int                       total_length;
        struct table *            cl;

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

        /* FIXME: We should move pre-checks from arp826_process() here ... */

        skb = skb_share_check(skb, GFP_ATOMIC);
        if (!skb) {
                LOG_ERR("This ARP cannot be shared!");
                return 0;
        }

        /* ARP header, without 2 device and 2 network addresses (???) */
        if (!pskb_may_pull(skb, sizeof(struct arp_header))) {
                LOG_WARN("Got an ARP header "
                         "without 2 devices and 2 network addresses "
                         "(step #1)");
                kfree_skb(skb);
                return 0;
        }

        header = header_get(skb);
        if (header->hlen != dev->addr_len) {
                LOG_WARN("Cannot process this ARP");
                kfree_skb(skb);
                return 0;
        }

        /* FIXME: There's no need to lookup it here ... */
        cl = tbls_find(header->ptype);
        if (!cl) {
                LOG_ERR("I don't have a CL to handle this ARP");
                return 0;
        }

        total_length = sizeof(struct arp_header) +
                (dev->addr_len + header->plen) * 2;

        /* ARP header, with 2 device and 2 network addresses (???) */
        if (!pskb_may_pull(skb, total_length)) {
                LOG_WARN("Got an ARP header "
                         "without 2 devices and 2 network addresses "
                         "(step #2)");
                kfree_skb(skb);
                return 0;
        }

        if (process(skb, cl)) {
                LOG_ERR("Cannot process this ARP");
                return 0;
        }
        consume_skb(skb);

        return 0;
}

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
                        size_t   hlen,
                        int   (* receiver)(struct sk_buff *     skb,
                                           struct net_device *  dev,
                                           struct packet_type * pkt,
                                           struct net_device *  orig_dev))
{
        struct protocol * p = protocol_create(ptype, hlen, receiver);

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
        spin_lock_init(&protocols_lock);

        if (tbls_init())
                return -1;

        if (arm_init()) {
                tbls_fini();
                return -1;
        }

        if (!protocol_add(ETH_P_RINA, 6, arp_receive)) {
                tbls_fini();
                arm_fini();
                return -1;
        }

        LOG_DBG("Initialized successfully");

        return 0;
}

static void __exit mod_exit(void)
{
        protocol_remove(ETH_P_RINA);

        arm_fini();
        tbls_fini();

        LOG_DBG("Destroyed successfully");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("Basic RFC 826 compliant ARP implementation");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Sander Vrijders <sander.vrijders@intec.ugent.be>");
MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
