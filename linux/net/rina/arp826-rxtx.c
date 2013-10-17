/*
 * ARP 826 (wonnabe) RX/TX functions
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
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

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/if_arp.h>

/* FIXME: The following dependencies have to be removed */
#define RINA_PREFIX "arp826-rxtx"

#include "logs.h"
#include "debug.h"
#include "utils.h"
/* FIXME: End of dependencies ... */

#include "arp826-utils.h"
#include "arp826-tables.h"
#include "arp826-arm.h"

static struct sk_buff * arp_create(struct net_device * dev,
                                   uint16_t            oper,
                                   uint16_t            ptype,
                                   const struct gpa *  spa,
                                   const struct gha *  sha,
                                   const struct gpa *  tpa,
                                   const struct gha *  tha)
{
	struct sk_buff *    skb;
	struct arp_header * arp;
	unsigned char *     arp_ptr;
        size_t              hlen;
        size_t              tlen;
	size_t              length;

        ASSERT(dev &&
               gpa_is_ok(spa) && gha_is_ok(sha) &&
               gpa_is_ok(tpa) && gha_is_ok(tha));

        if (gpa_address_length(spa) != gpa_address_length(tpa)) {
                LOG_ERR("Mismatched source and target GPA lengths, "
                        "cannot create ARP");
                return NULL;
        }
        if (gha_type(sha) != gha_type(tha)) {
                LOG_ERR("Mismatched source and target GHA types, "
                        "cannot create ARP");
                return NULL;
        }

        ASSERT(dev);
        ASSERT(gpa_address_length(spa) == gpa_address_length(tpa));
        ASSERT(gha_type(sha)           == gha_type(tha));

        if (gha_type(sha) != MAC_ADDR_802_3) {
                LOG_ERR("Cannot process GHAs != 802.3");
                return NULL;
        }

        if (dev->type != ARPHRD_ETHER) {
                LOG_ERR("Wrong device, cannot use the passed GHAs");
                return NULL;
        }

	hlen   = LL_RESERVED_SPACE(dev);
	tlen   = dev->needed_tailroom;
        length = sizeof(struct arp_header) +
                (gpa_address_length(spa) + gha_address_length(sha)) * 2;

	skb = alloc_skb(length + hlen + tlen, GFP_ATOMIC);
	if (skb == NULL)
		return NULL;

	skb_reserve(skb, hlen);
	skb_reset_network_header(skb);
	arp = (struct arp_header *) skb_put(skb, length);

	skb->dev      = dev;
	skb->protocol = htons(ptype);

        /* Fill the device header for the ARP frame */
	if (dev_hard_header(skb, dev,
                            ptype,
                            gha_address(tha),
                            gha_address(sha),
                            skb->len) < 0) {
                LOG_ERR("Cannot create hard-header");
                kfree_skb(skb);
                return NULL;
        }

        /* Fill out the packet, finally */

        arp->htype = htons(dev->type);
        arp->ptype = htons(ptype);
	arp->hlen  = gha_address_length(sha);
	arp->plen  = gpa_address_length(spa);
	arp->oper  = htons(oper);

	arp_ptr = (unsigned char *)(arp + 1);

        /* SHA */
	memcpy(arp_ptr, gha_address(sha), gha_address_length(sha));
	arp_ptr += gha_address_length(sha);

        /* SPA */
	memcpy(arp_ptr, gpa_address_value(spa), gpa_address_length(spa));
	arp_ptr += gpa_address_length(spa);

        /* THA */
	memcpy(arp_ptr, gha_address(tha), gha_address_length(tha));
	arp_ptr += gha_address_length(tha);

        /* TPA */
	memcpy(arp_ptr, gpa_address_value(tpa), gpa_address_length(tpa));
	arp_ptr += gpa_address_length(tpa);

	return skb;
}

int arp_send_reply(uint16_t            ptype,
                   const struct gpa *  spa,
                   const struct gha *  sha,
                   const struct gpa *  tpa,
                   const struct gha *  tha)
{
#ifdef CONFIG_RINARP
        struct gpa *        tmp_spa;
        struct gpa *        tmp_tpa;
        uint8_t             filler;
        size_t              max_len;
#endif
        struct net_device * dev;
        struct sk_buff *    skb;

        if (!gpa_is_ok(spa) || !gha_is_ok(sha) ||
            !gpa_is_ok(tpa) || !gha_is_ok(tha)) {
                LOG_ERR("Wrong input parameters, cannot send ARP reply");
                return -1;
        }

        dev = gha_to_device(sha);
        if (!dev) {
                LOG_ERR("Cannot get the device for this GHA");
                return -1;
        }

#ifdef CONFIG_RINARP
        max_len = max(gpa_address_length(spa), gpa_address_length(tpa));
        LOG_DBG("Growing addresses to %zd", max_len);
        tmp_spa = gpa_dup(spa);
        if (gpa_address_grow(tmp_spa, max_len, 0x00))
                return -1;
        tmp_tpa = gpa_dup(tpa);
        if (gpa_address_grow(tmp_tpa, max_len, 0x00)) {
                gpa_destroy(tmp_spa);
                return -1;
        }

        skb = arp_create(dev,
                         ARP_REPLY, ptype, 
                         tmp_spa, sha, tmp_tpa, tha);

        gpa_destroy(tmp_spa);
        gpa_destroy(tmp_tpa);
#else
        skb = arp_create(dev,
                         ARP_REPLY, ptype, 
                         spa, sha, tpa, tha);
#endif
        if (skb == NULL)
                return -1;

        dev_queue_xmit(skb);

        return 0;
}

/* Fills the packet fields, sets TPA to broadcast */
int arp_send_request(uint16_t            ptype,
                     const struct gpa *  spa,
                     const struct gha *  sha,
                     const struct gpa *  tpa)
{
#ifdef CONFIG_RINARP
        struct gpa *        tmp_spa;
        struct gpa *        tmp_tpa;
        uint8_t             filler;
        size_t              max_len;
#endif
        struct net_device * dev;
        struct sk_buff *    skb;
        struct gha *        tha;

        if (!gpa_is_ok(spa) || !gha_is_ok(sha) || !gpa_is_ok(tpa)) {
                LOG_ERR("Wrong input parameters, cannot send ARP request");
                return -1;
        }
       
        tha = gha_create_broadcast(gha_type(sha));
        if (!tha) {
                LOG_ERR("Cannot create broadcast GHA");
                return -1;
        }

        dev = gha_to_device(sha);
        if (!dev) {
                LOG_ERR("Cannot get the device for this GHA");
                gha_destroy(tha);
                return -1;
        }

#ifdef CONFIG_RINARP
        max_len = max(gpa_address_length(spa), gpa_address_length(tpa));
        LOG_DBG("Growing addresses to %zd", max_len);
        tmp_spa = gpa_dup(spa);
        if (gpa_address_grow(tmp_spa, max_len, 0x00))
                return -1;
        tmp_tpa = gpa_dup(tpa);
        if (gpa_address_grow(tmp_tpa, max_len, 0x00)) {
                gpa_destroy(tmp_spa);
                return -1;
        }

        skb = arp_create(dev,
                         ARP_REPLY, ptype, 
                         tmp_spa, sha, tmp_tpa, tha);

        gpa_destroy(tmp_spa);
        gpa_destroy(tmp_tpa);
#else
        skb = arp_create(dev,
                         ARP_REPLY, ptype, 
                         spa, sha, tpa, tha);
#endif

        if (skb == NULL) {
                gha_destroy(tha);
                return -1;
        }

        dev_queue_xmit(skb);

        gha_destroy(tha);

        return 0;
}

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

        LOG_DBG("We can handle this ARP");

        ptr = (uint8_t *) header + 8;

        sha = ptr; ptr += header->hlen;
        spa = ptr; ptr += header->plen;
        tha = ptr; ptr += header->hlen;
        tpa = ptr; ptr += header->plen;

        LOG_DBG("Fetching addresses");
	tmp_spa = gpa_create_gfp(GFP_ATOMIC, spa, plen);
	tmp_sha = gha_create_gfp(GFP_ATOMIC, MAC_ADDR_802_3, sha);
	tmp_tpa = gpa_create_gfp(GFP_ATOMIC, tpa, plen);
	tmp_tha = gha_create_gfp(GFP_ATOMIC, MAC_ADDR_802_3, tha);

#ifdef CONFIG_RINARP
        LOG_DBG("Shrinking as needed");
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
int arp_receive(struct sk_buff *     skb,
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
