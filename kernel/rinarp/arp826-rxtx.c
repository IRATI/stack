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
#include <linux/if_ether.h>

/* FIXME: The following dependencies have to be removed */
#define RINA_PREFIX "arp826-rxtx"

#include "logs.h"
#include "debug.h"
#include "utils.h"
/* FIXME: End of dependencies ... */

#include "rinarp/arp826.h"
#include "rinarp/arp826-utils.h"
#include "rinarp/arp826-tables.h"
#include "rinarp/arp826-arm.h"
#include "rinarp/arp826-rxtx.h"

#if defined(CONFIG_RINARP) || defined(CONFIG_RINARP_MODULE)
#define HAVE_RINARP 1
#else
#define HAVE_RINARP 0
#endif

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
        if (skb == NULL) {
                LOG_ERR("Couldn't allocate skb");
                return NULL;
        }

        skb_reserve(skb, hlen);
        skb_reset_network_header(skb);
        arp = (struct arp_header *) skb_put(skb, length);

        skb->dev      = dev;
        skb->protocol = htons(RINARP_ETH_PROTO);

        /* Fill the device header for the ARP frame */
        if (dev_hard_header(skb, dev,
                            RINARP_ETH_PROTO,
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

int arp_send_reply(struct net_device * dev,
                   uint16_t            ptype,
                   const struct gpa *  spa,
                   const struct gha *  sha,
                   const struct gpa *  tpa,
                   const struct gha *  tha)
{
#if HAVE_RINARP
        struct gpa *        tmp_spa;
        struct gpa *        tmp_tpa;
        size_t              max_len;
#endif
        struct sk_buff *    skb;

        LOG_DBG("Sending ARP reply");

        if (!gpa_is_ok(spa) || !gha_is_ok(sha) ||
            !gpa_is_ok(tpa) || !gha_is_ok(tha)) {
                LOG_ERR("Wrong input parameters, cannot send ARP reply");
                return -1;
        }

#if HAVE_RINARP
        max_len = max(gpa_address_length(spa), gpa_address_length(tpa));
        LOG_DBG("Growing addresses to %zd", max_len);
        tmp_spa = gpa_dup_gfp(GFP_ATOMIC, spa);
        if (gpa_address_grow_gfp(GFP_ATOMIC, tmp_spa, max_len, 0x00)) {
                LOG_ERR("Failed to grow SPA");
                return -1;
        }
        tmp_tpa = gpa_dup_gfp(GFP_ATOMIC, tpa);
        if (gpa_address_grow_gfp(GFP_ATOMIC, tmp_tpa, max_len, 0x00)) {
                LOG_ERR("Failed to grow TPA");
                gpa_destroy(tmp_spa);
                return -1;
        }

        LOG_DBG("Creating an ARP packet");
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
                LOG_ERR("Skb was NULL");
                return -1;
        }

        dev_queue_xmit(skb);

        LOG_DBG("ARP packet sent successfully");

        return 0;
}

/* Fills the packet fields, sets THA to unkown */
int arp_send_request(struct net_device * dev,
                     uint16_t            ptype,
                     const struct gpa *  spa,
                     const struct gha *  sha,
                     const struct gpa *  tpa)
{
#if HAVE_RINARP
        struct gpa *        tmp_spa;
        struct gpa *        tmp_tpa;
        size_t              max_len;
#endif
        struct sk_buff *    skb;
        struct gha *        tha;

        LOG_DBG("Sending ARP request");

        if (!gpa_is_ok(spa) || !gha_is_ok(sha) || !gpa_is_ok(tpa)) {
                LOG_ERR("Wrong input parameters, cannot send ARP request");
                return -1;
        }

        tha = gha_create_unknown(gha_type(sha));
        if (!tha) {
                LOG_ERR("Cannot create broadcast GHA");
                return -1;
        }

#if HAVE_RINARP
        max_len = max(gpa_address_length(spa), gpa_address_length(tpa));
        LOG_DBG("Growing addresses to %zd", max_len);
        tmp_spa = gpa_dup(spa);
        if (gpa_address_grow(tmp_spa, max_len, 0x00)) {
                LOG_ERR("Failed to grow SPA");
                return -1;
        }
        tmp_tpa = gpa_dup(tpa);
        if (gpa_address_grow(tmp_tpa, max_len, 0x00)) {
                LOG_ERR("Failed to grow TPA");
                gpa_destroy(tmp_spa);
                return -1;
        }

        LOG_DBG("Creating an ARP packet");
        skb = arp_create(dev,
                         ARP_REQUEST, ptype,
                         tmp_spa, sha, tmp_tpa, tha);

        gpa_destroy(tmp_spa);
        gpa_destroy(tmp_tpa);
#else
        skb = arp_create(dev,
                         ARP_REQUEST, ptype,
                         spa, sha, tpa, tha);
#endif

        if (skb == NULL) {
                LOG_ERR("Sk_buff was null");
                gha_destroy(tha);
                return -1;
        }

        dev_queue_xmit(skb);

        LOG_DBG("ARP packet sent successfully");

        gha_destroy(tha);

        return 0;
}

static struct arp_header * header_get(const struct sk_buff * skb)
{ return (struct arp_header *) skb_network_header(skb); }

static int process(const struct sk_buff * skb,
                   struct table *         cl,
                   struct net_device *    dev)
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
        LOG_DBG("  Hardware type           = 0x%04X", htype);
        LOG_DBG("  Protocol type           = 0x%04X", ptype);
        LOG_DBG("  Hardware address length = %d",     hlen);
        LOG_DBG("  Protocol address length = %d",     plen);
        LOG_DBG("  Operation               = 0x%04X", oper);

        if (header->htype != htons(HW_TYPE_ETHER)) {
                LOG_ERR("Unhandled ARP hardware type 0x%04X", header->htype);
                return -1;
        }
        if (hlen != 6) {
                LOG_ERR("Unhandled ARP hardware address length (%d)", hlen);
                return -1;
        }

        operation = ntohs(header->oper);
        if (operation != ARP_REPLY && operation != ARP_REQUEST) {
                LOG_ERR("Unhandled ARP operation 0x%04X", operation);
                return -1;
        }

        LOG_DBG("We can handle this ARP");

        ptr = (uint8_t *) header + 8;

        sha = ptr; ptr += header->hlen;
        spa = ptr; ptr += header->plen;
        tha = ptr; ptr += header->hlen;
        tpa = ptr; ptr += header->plen;

        LOG_DBG("Fetching addresses");

        /* FIXME: Add some dumps of these addresses */
        tmp_spa = gpa_create_gfp(GFP_ATOMIC, spa, plen);
        tmp_sha = gha_create_gfp(GFP_ATOMIC, MAC_ADDR_802_3, sha);
        tmp_tpa = gpa_create_gfp(GFP_ATOMIC, tpa, plen);
        tmp_tha = gha_create_gfp(GFP_ATOMIC, MAC_ADDR_802_3, tha);

#if HAVE_RINARP
        LOG_DBG("Shrinking as needed");
        if (gpa_address_shrink_gfp(GFP_ATOMIC, tmp_spa, 0x00)) {
                LOG_ERR("Problems parsing the source GPA");
                gpa_destroy(tmp_spa);
                gpa_destroy(tmp_tpa);
                gha_destroy(tmp_sha);
                gha_destroy(tmp_tha);
                return -1;
        }
        if (gpa_address_shrink_gfp(GFP_ATOMIC, tmp_tpa, 0x00)) {
                LOG_ERR("Got problems parsing the target GPA");
                gpa_destroy(tmp_spa);
                gpa_destroy(tmp_tpa);
                gha_destroy(tmp_sha);
                gha_destroy(tmp_tha);
                return -1;
        }
#endif

        /* Finally process the entry */
        switch (operation) {
        case ARP_REQUEST: {
                struct table *             tbl       = NULL;
                const struct table_entry * entry     = NULL;
                const struct table_entry * req_addr  = NULL;
                const struct gha *         target_ha = NULL;

                /* FIXME: Should we add all ARP Requests? */

                /* Do we have it in the cache ? */
                tbl = tbls_find(dev, ptype);
                if (!tbl) {
                        LOG_ERR("I don't have a table for ptype 0x%04X",
                                ptype);
                        gpa_destroy(tmp_spa);
                        gpa_destroy(tmp_tpa);
                        gha_destroy(tmp_sha);
                        gha_destroy(tmp_tha);
                        return -1;
                }

                /*
                 * FIXME: We do a double lookup here. Please remove it, all
                 *        the CPUs (and trees) out there will apreciate that...
                 */
                entry = tbl_find_by_gpa(tbl, tmp_spa);
                if (!entry) {
                        struct table_entry * tmp;

                        LOG_DBG("Adding new entry to the table");

                        tmp = tble_create_ni(tmp_spa, tmp_sha);
                        if (!tmp)
                                return -1;

                        if (tbl_add(tbl, tmp)) {
                                LOG_ERR("Can't add in table");
                                tble_destroy(tmp);
                                gpa_destroy(tmp_spa);
                                gpa_destroy(tmp_tpa);
                                gha_destroy(tmp_sha);
                                gha_destroy(tmp_tha);
                                return -1;
                        }
                } else {
                        LOG_DBG("Updating old entry %pK into the table",
                                entry);

                        if (tbl_update_by_gpa(tbl, tmp_spa, tmp_sha,
                                              GFP_ATOMIC)) {
                                LOG_ERR("Failed to update table");
                                gpa_destroy(tmp_spa);
                                gpa_destroy(tmp_tpa);
                                gha_destroy(tmp_sha);
                                gha_destroy(tmp_tha);
                                return -1;
                        }
                }

                req_addr = tbl_find_by_gpa(tbl, tmp_tpa);
                if (!req_addr) {
                        LOG_DBG("Cannot find this TPA in my tables, "
                                "bailing out");
                        gpa_destroy(tmp_spa);
                        gpa_destroy(tmp_tpa);
                        gha_destroy(tmp_sha);
                        gha_destroy(tmp_tha);
                        return -1;
                }

                target_ha = tble_ha(req_addr);
                if (!target_ha) {
                        LOG_ERR("Cannot get a good target HA");
                        gpa_destroy(tmp_spa);
                        gpa_destroy(tmp_tpa);
                        gha_destroy(tmp_sha);
                        gha_destroy(tmp_tha);
                        return -1;
                }

                LOG_DBG("Showing the target ha");
                gha_dump(target_ha);

                if (arp_send_reply(dev,
                                   ptype,
                                   tmp_tpa, target_ha,
                                   tmp_spa, tmp_sha)) {
                        LOG_ERR("Couldn't send reply");
                        gpa_destroy(tmp_spa);
                        gpa_destroy(tmp_tpa);
                        gha_destroy(tmp_sha);
                        gha_destroy(tmp_tha);
                        return -1;
                }
                gpa_destroy(tmp_spa);
                gpa_destroy(tmp_tpa);
                gha_destroy(tmp_sha);
                gha_destroy(tmp_tha);

                LOG_DBG("Request replied successfully");
        }
                break;

        case ARP_REPLY: {
                if (arm_resolve(dev, ptype,
                                tmp_spa, tmp_sha, tmp_tpa, tmp_tha)) {
                        LOG_ERR("Cannot resolve with this reply ...");
                        return -1;
                }

                LOG_DBG("Resolution in progress, please wait ...");
        }
                break;

        default:
                BUG();
        }

        LOG_DBG("Processing completed successfully");

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

        if (!skb) {
                LOG_ERR("No skb passed ?");
                return 0;
        }

        if (!dev) {
                LOG_ERR("No device passed ?");
                kfree_skb(skb);
                return 0;
        }

        if (dev->flags & IFF_NOARP            ||
            skb->pkt_type == PACKET_OTHERHOST ||
            skb->pkt_type == PACKET_LOOPBACK) {
                LOG_DBG("This ARP is not for us "
                        "(no arp, other-host or loopback)");
                kfree_skb(skb);
                return 0;
        }

        /* We only receive type-1 headers (this handler could be reused) */
        if (skb->dev->type != HW_TYPE_ETHER) {
                LOG_DBG("Unhandled device type %d", skb->dev->type);
                kfree_skb(skb);
                return 0;
        }

        /* FIXME: We should move pre-checks from arp826_process() here ... */

        skb = skb_share_check(skb, GFP_ATOMIC);
        if (!skb) {
                LOG_ERR("This ARP cannot be shared!");
                kfree_skb(skb);
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
        cl = tbls_find(dev, ntohs(header->ptype));
        if (!cl) {
#if 0
                /* This log is too noisy ... but necessary for now :) */
                LOG_DBG("I don't have a table to handle this ARP "
                        "(ptype = 0x%04X)", ntohs(header->ptype));
#endif
                kfree_skb(skb);
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

        if (process(skb, cl, dev)) {
                LOG_DBG("Cannot process this ARP");
                kfree_skb(skb);
                return 0;
        }

        consume_skb(skb);

        return 0;
}
