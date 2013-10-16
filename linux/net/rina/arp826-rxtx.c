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

/* FIXME: The following dependencies have to be removed */
#define RINA_PREFIX "arp826-rxtx"

#include "logs.h"
#include "debug.h"
#include "utils.h"
/* FIXME: End of dependencies ... */

#include "arp826-utils.h"

#if 0
static struct sk_buff * arp_create(struct net_device * dev,
                                   uint16_t            oper,
                                   uint16_t            ptype,
                                   const struct gpa *  spa,
                                   const struct gpa *  tpa,
                                   const struct gha *  tha)
{
        ASSERT(gpa_is_ok(spa) && gpa_is_ok(tpa) && gha_is_ok(tha));

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
#endif

int arp_send_reply(uint16_t            ptype,
                   const struct gpa *  spa,
                   const struct gpa *  tpa,
                   const struct gha *  tha)
{
#if 0
        struct net_device * dev;
        struct sk_buff *    skb;

        if (!gpa_is_ok(spa) || !gpa_is_ok(tpa) || !gha_is_ok(tha)) {
                LOG_ERR("Wrong input parameters, cannot send ARP reply");
                return -1;
        }

        dev = gha_to_device(target_ha);
        if (!dev) {
                LOG_ERR("Cannot get the device for this GHA");
                return -1;
        }

        skb = arp_create(dev,
                         ARP_REPLY, ptype, 
                         tmp_tpa, tmp_spa, tmp_sha);
        if (skb == NULL)
                return -1;

        dev_queue_xmit(skb);
#endif

        return 0;
}

int arp_send_request(uint16_t            ptype,
                     const struct gpa *  spa,
                     const struct gha *  sha,
                     const struct gpa *  tpa)
{
#if 0
        struct net_device * dev;
        struct sk_buff *    skb;

        if (!gpa_is_ok(spa) || !gha_is_ok(sha) || !gpa_is_ok(tpa)) {
                LOG_ERR("Wrong input parameters, cannot send ARP request");
                return -1;
        }

        dev = gha_to_device(target_ha);
        if (!dev) {
                LOG_ERR("Cannot get the device for this GHA");
                return -1;
        }

        skb = arp_create(dev,
                         ARP_REQUEST, ptype, 
                         tmp_tpa, tmp_spa, tmp_sha);
        if (skb == NULL)
                return -1;

        dev_queue_xmit(skb);
#endif

        return 0;
}
