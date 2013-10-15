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
#include "arp826-arm.h"
#include "arp826-tables.h"

static struct arp_header * header_get(const struct sk_buff * skb)
{ return (struct arp_header *) skb_network_header(skb); }

struct net_device * device_from_gha(const struct gha * ha) 
{
	LOG_MISSING;
	return NULL;
}

struct sk_buff * create_arp_packet(int op, int ptype,
				  struct net_device * dev,
				  const struct gpa * spa,
				  const struct gpa * tpa,
				  const struct gha * tha)
{
#if 0
	struct sk_buff * skb;
	struct arp_hdr * arp;
	unsigned char * arp_ptr;
	int hlen = LL_RESERVED_SPACE(dev);
	int tlen = dev->needed_tailroom;
	const unsigned char *src_hw;

/*
 * Allocate a buffer
 */
	int length = sizeof(struct arp_hdr) + (dev->addr_len + plen) * 2;

	skb = alloc_skb(length + hlen + tlen, GFP_ATOMIC);
	if (skb == NULL)
		return NULL;

	skb_reserve(skb, hlen);
	skb_reset_network_header(skb);
	arp = (struct arp_hdr *) skb_put(skb, length);
	skb->dev = dev;
	skb->protocol = htons(ETH_P_ARP);
	src_hw = dev->dev_addr;
	if (dest_hw == NULL)
		dest_hw = dev->broadcast;

/*
 * Fill the device header for the ARP frame
 */
	if (dev_hard_header(skb, dev, ptype, dest_hw, src_hw, skb->len) < 0)
		goto out;

/*
 * Fill out the arp protocol part.
 */

	switch (dev->type) {
	default:
		arp->ar_hrd = htons(dev->type);
		arp->ar_pro = htons(ptype);
		break;
	}

	arp->ar_hln = dev->addr_len;
	arp->ar_pln = plen;
	arp->ar_op = htons(op);

	arp_ptr = (unsigned char *)(arp + 1);

	memcpy(arp_ptr, src_hw, dev->addr_len);
	arp_ptr += dev->addr_len;
	memcpy(arp_ptr, src_nwaddr, plen);
	arp_ptr += plen;

	switch (dev->type) {
	default:
		if (dest_hw != NULL)
			memcpy(arp_ptr, dest_hw, dev->addr_len);
		else
			memset(arp_ptr, 0, dev->addr_len);
		arp_ptr += dev->addr_len;
	}
	memcpy(arp_ptr, dest_nwaddr, plen);

	return skb;

out:
	kfree_skb(skb);
#endif
	LOG_MISSING;
	return NULL;
}

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

        /* Finally process the entry */
        switch (operation) {
        case ARP_REQUEST: {
		struct table *       tbl;
		const struct table_entry * entry;
		const struct table_entry * req_addr;
		const struct gha *   target_ha;
		struct net_device *  dev;

		/* FIXME: Should we add all ARP Requests? */
                /* Do we have it in the cache ? */
		tbl = tbls_find(ptype);
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
          
		req_addr = tbl_find_by_gpa(tbl, tmp_tpa);
		target_ha = tble_ha(req_addr);
		/* FIXME: Get lock here */
		dev = device_from_gha(target_ha);
		if (dev) {
			struct sk_buff * skb;
			/* This is our gpa and gha. Send reply */
			skb = create_arp_packet(ARP_REPLY, ptype, 
						dev, tmp_tpa, 
						tmp_spa, tmp_sha);
			if (skb == NULL) 
				return -1;
			dev_queue_xmit(skb);
		}
         }
                break;

        case ARP_REPLY: {
                if (arm_resolve(ptype, tmp_spa, tmp_sha, tmp_tpa, tmp_tha)) {
                        LOG_ERR("Canot resolve with this reply ...");
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
static int receive(struct sk_buff *     skb,
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

/* FIXME : This has to be managed dinamically ... */
static struct packet_type arp_packet_type __read_mostly = {
        .type = cpu_to_be16(ETH_P_ARP),
        .func = receive,
};

static int __init mod_init(void)
{
        if (tbls_init())
                return -1;

        if (arm_init()) {
                tbls_fini();
                return -1;
        }

        /* FIXME: Pack these two lines together */
        if (tbls_create(ETH_P_RINA, 6)) {
                tbls_fini();
                arm_fini();
                return -1;
        }

        dev_add_pack(&arp_packet_type);

        LOG_DBG("Initialized successfully");

        return 0;
}

static void __exit mod_exit(void)
{
        arm_fini();

        /* FIXME: Pack these two lines together */
        dev_remove_pack(&arp_packet_type);
        tbls_destroy(ETH_P_RINA);

        tbls_fini();

        LOG_DBG("Destroyed successfully");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("Basic RFC 826 compliant ARP implementation");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Sander Vrijders <sander.vrijders@intec.ugent.be>");
MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
