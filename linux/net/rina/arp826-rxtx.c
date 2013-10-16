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
