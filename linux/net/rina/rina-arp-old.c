/*
 * Implementation of RFC 826 because current implementation is too
 * intertwined with IP version 4.
 *
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
 *
 *    Code reused from:
 *      net/ipv4/arp.c
 *      include/linux/if_arp.h
 *      include/uapi/linux/if_arp.h
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

#include "rina-arp-old.h"

#define RINA_TEST 0

/* Taken from include/uapi/linux/if_arp.h */
#define RINARP_ETHER     1              /* Ethernet    */

#define RINARP_REQUEST   1              /* ARP request */
#define RINARP_REPLY     2              /* ARP reply   */

struct network_mapping {
        struct list_head list;
        unsigned char *  netaddr; /* The network address advertised */
	unsigned char *  hw_addr; /* The Hardware address */
};

struct arp_data {
	struct list_head    list;
	struct list_head    entries;
	__be16              ar_pro;
	struct net_device * dev;
	struct list_head    arp_handlers;
};

static struct list_head data;

/* 
 *  Taken from include/uapi/linux/if_arp.h 
 *  Original name: struct arphdr
 */
struct arp_hdr {
	__be16          ar_hrd;         /* format of hardware address   */
	__be16          ar_pro;         /* format of protocol address   */
	__u8            ar_hln;         /* length of hardware address   */
        __u8            ar_pln;         /* length of protocol address   */
	__be16          ar_op;          /* ARP opcode (command)         */
 
#if 0
	/*
	 *      This bit is variable sized however...
	 *      This is an example
	 */
	unsigned char    ar_sha;     /* sender hardware address   */
	unsigned char    ar_spa;     /* sender protocol address   */
	unsigned char    ar_tha;     /* target hardware address   */
	unsigned char    ar_tpa;     /* target protocol address   */
#endif
 
};

/* Searches for an arp_data or creates one if not found */
static struct arp_data * find_arp_data(__be16              ar_pro, 
				       struct net_device * dev)
{
	struct arp_data * arp_d;

	list_for_each_entry(arp_d, &data, list) {
		if (arp_d->ar_pro == ar_pro && arp_d->dev == dev)
			return arp_d;
        }

	arp_d = kzalloc(sizeof(*arp_d), GFP_KERNEL);
        if (!arp_d) {
		printk("Out of memory");
                return NULL;
	}

	arp_d->dev    = dev;
	arp_d->ar_pro = ar_pro;

	INIT_LIST_HEAD(&arp_d->list);
	INIT_LIST_HEAD(&arp_d->entries);

	list_add(&arp_d->list, &data);
	
	return arp_d;
}

static struct network_mapping * find_netw_map(struct arp_data * arp_d,
					      unsigned char   * netw_addr)
{
	struct network_mapping * netw_map;

	list_for_each_entry(netw_map, &arp_d->entries, list) {
		if (!strcmp(netw_map->netaddr, netw_addr))
			return netw_map;
        }

	return NULL;
}

static struct arp_reply_ops * find_reply_ops(struct arp_data * arp_d,
			  		     unsigned char *   src_netw_addr,
					     unsigned char *   dest_netw_addr)
{
	struct arp_reply_ops * ops;

	list_for_each_entry(ops, &arp_d->arp_handlers, list) {
		if (!strcmp(ops->src_netw_addr,  src_netw_addr) &&
                    !strcmp(ops->dest_netw_addr, dest_netw_addr)) {
			return ops;
		}
        }

	return NULL;
}

int rinarp_old_register_netaddr(__be16              ar_pro, 
                                struct net_device * dev, 
                                unsigned char *     netw_addr)
{ 
	struct network_mapping * net_map;
	struct arp_data * arp_d;

	/* First get the right arp_data */
	arp_d = find_arp_data(ar_pro, dev);
	if (!arp_d)
		return -1;

        if (find_netw_map(arp_d, netw_addr)) {
                printk("Network address %s has been already registered", 
		       netw_addr);
                return -1;
        }

        net_map = kzalloc(sizeof(*net_map), GFP_KERNEL);
        if (!net_map) {
		printk("Out of memory");
                return -1;
	}

        INIT_LIST_HEAD(&net_map->list);

	strcpy(net_map->netaddr, netw_addr);
	strcpy(net_map->hw_addr, dev->dev_addr);
        list_add(&net_map->list, &arp_d->entries);

	return 0;
}
EXPORT_SYMBOL(rinarp_old_register_netaddr);

int rinarp_old_unregister_netaddr(__be16              ar_pro, 
                                  struct net_device * dev, 
                                  unsigned char *     netw_addr)
{ 
	struct network_mapping * net_map;
	struct arp_data *        arp_d;

	/* First get the right arp_data */
	arp_d = find_arp_data(ar_pro, dev);
	if (!arp_d)
		return -1;

	net_map = find_netw_map(arp_d, netw_addr);
        if (!net_map) {
                printk("No such network address: %s", netw_addr);
                return -1;
        }

	list_del(&net_map->list);
	kfree(net_map);
	
	return 0;
}
EXPORT_SYMBOL(rinarp_old_unregister_netaddr);

unsigned char * rinarp_old_lookup_netaddr(__be16              ar_pro, 
                                          struct net_device * dev, 
                                          unsigned char *     netw_addr)
{
	struct network_mapping * net_map;
	struct arp_data * arp_d;

	/* First get the right arp_data */
	arp_d = find_arp_data(ar_pro, dev);
	if (!arp_d)
		return NULL;
	
	net_map = find_netw_map(arp_d, netw_addr);

	return net_map->hw_addr;
}
EXPORT_SYMBOL(rinarp_old_lookup_netaddr);

int rinarp_old_remove_reply_handler(struct arp_reply_ops * ops)
{ 
	struct arp_reply_ops * reply_ops;
	struct arp_data * arp_d;

	/* First get the right arp_data */
	arp_d = find_arp_data(ops->ar_pro, ops->dev);
	if (!arp_d)
		return -1;

	reply_ops = find_reply_ops(arp_d,
                                   ops->src_netw_addr,
                                   ops->dest_netw_addr);
        if (!reply_ops) {
                printk("No such reply ops");
                return -1;
        }

	list_del(&reply_ops->list);
	kfree(reply_ops);
	
	return 0;
}
EXPORT_SYMBOL(rinarp_old_remove_reply_handler);

/* Taken from include/linux/if_arp.h */
static struct arp_hdr * arphdr(const struct sk_buff * skb)
{ return (struct arp_hdr *)skb_network_header(skb); }

/*
 *	Create an arp packet. If (dest_hw == NULL), we create a broadcast
 *	message.
 *      Taken from net/ipv4/arp.c
 */
static struct sk_buff *arp_create(int op, int ptype, int plen,
                                  struct net_device *   dev, 
				  const unsigned char * src_nwaddr,
				  const unsigned char * dest_nwaddr,
                                  const unsigned char * dest_hw)
{
	struct sk_buff *     skb;
	struct arp_hdr *     arp;
	unsigned char *      arp_ptr;
	int                  hlen = LL_RESERVED_SPACE(dev);
	int                  tlen = dev->needed_tailroom;
	const unsigned char *src_hw;

	/*
	 *	Allocate a buffer
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
	 *	Fill the device header for the ARP frame
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
	arp->ar_op  = htons(op);

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
	return NULL;
}

/*
 *	Create and send an arp packet.
 *      Taken from net/ipv4/arp.c
 *      Original name arp_send
 */

int rinarp_old_send_request(struct arp_reply_ops * ops)
{
	struct sk_buff *  skb;
	struct arp_data * arp_d;

	/*
	 *	No arp on this interface.
	 */

	if (ops->dev->flags&IFF_NOARP)
		return -1;
	
	/* Store into list of ARP response handlers */
	arp_d = find_arp_data(ops->ar_pro, ops->dev);
	if (!arp_d)
		return -1;
	list_add(&ops->list, &arp_d->arp_handlers);
	

	/* FIXME: Call arp_create with correct length params */
	skb = arp_create(RINARP_REQUEST,
                         ops->ar_pro, 32, ops->dev,
			 ops->src_netw_addr, ops->dest_netw_addr, NULL);

	if (skb == NULL)
		return -1;
	
        /* Actually send it */
	dev_queue_xmit(skb);
	return 0;
}
EXPORT_SYMBOL(rinarp_old_send_request);

/*
 *	Process an arp request.
 *      Taken from net/ipv4/arp.c
 */
#if RINA_TEST
static int arp_process(struct sk_buff *skb)
{
	struct net_device *dev = skb->dev;
	struct in_device *in_dev = __in_dev_get_rcu(dev);
	struct arp_hdr *arp;
	unsigned char  *arp_ptr;

	unsigned char *sha;
	unsigned char *s_netaddr;
	unsigned char *d_netaddr;
	unsigned char *s_hwaddr;
	unsigned char *d_hwaddr;
	u16 dev_type = dev->type;
	struct net *net = dev_net(dev);

	/* arp_rcv below verifies the ARP header and verifies the device
	 * is ARP'able.
	 */

	if (in_dev == NULL)
		goto out;

	arp = arphdr(skb);

	/* 
	 * Check if we know the protocol specified
	 * Only accept RINA packets in this implementation 
	 * Others could be added 
	 */

	if (arp->ar_pro != htons(ETH_P_RINA) ||
	    htons(dev_type) != arp->ar_hrd)
		goto out;

	switch (dev_type) {
                /* 
                 * Only accept Ethernet packets in this implementation
                 * Other could be added
                 */
	case ARPHRD_ETHER:
		if ((arp->ar_hrd != htons(RINARP_ETHER))
                    goto out;
                    break;
                    }

                /* Understand only these message types */
                if (arp->ar_op != htons(RINARP_REPLY) &&
                    arp->ar_op != htons(RINARP_REQUEST))
                        goto out;

                /*
                 *	Extract addresses
                 */
                arp_ptr = (unsigned char *)(arp + 1);
                sha	= arp_ptr;
                arp_ptr += dev->addr_len;
                memcpy(s_netaddr, arp_ptr, arp->ar_pln);
                arp_ptr += arp->ar_pln;
                memcpy(s_hwaddr, arp_ptr, arp->ar_hln);
                arp_ptr += dev->addr_len;
                memcpy(d_netaddr, arp_ptr, arp->ar_pln);
                arp_ptr += arp->ar_pln;
                memcpy(d_hwaddr, arp_ptr, arp->ar_hln);

                /*
                 *  Process entry. The idea here is we want to send a reply if it is a
                 *  request for us. We want to add an entry to our cache if it is a reply
                 *  to us or if it is a request for one of our addresses.
                 *
                 *  Putting this another way, we only care about replies if they are to
                 *  us, in which case we add them to the cache.  For requests, we care
                 *  about those for us. We add the requester to the arp cache.
                 *
                 */

                /* FIXME: The following part, first complete top part ARP PM */
                if (arp->ar_op == htons(RINARP_REQUEST)) {
                        /* Are we advertising this network address? */
	
	
                } else if (arp->arp_op == htons(RINARP_REPLY)) {
                        /* Is the reply for one of our network addresses? */

		
                } else {
                        printk("Unknown operation code");
                        goto out;
                }

                /* Update our ARP tables */
                
        out:
                consume_skb(skb);
                return 0;
        }
}
#endif 

/* Function taken from net/ipv4/arp.c */
static int arp_rcv(struct sk_buff *skb, struct net_device *dev,
                   struct packet_type *pt, struct net_device *orig_dev)
{

        const struct arp_hdr * arp;
        int                    total_length;

        if (dev->flags & IFF_NOARP ||
            skb->pkt_type == PACKET_OTHERHOST ||
            skb->pkt_type == PACKET_LOOPBACK)
                goto freeskb;

        skb = skb_share_check(skb, GFP_ATOMIC);
        if (!skb)
                goto out_of_mem;

        /* ARP header, without 2 device and 2 network addresses.  */
        if (!pskb_may_pull(skb, sizeof(struct arp_hdr)))
                goto freeskb;

        arp = arphdr(skb);
        if (arp->ar_hln != dev->addr_len)
                goto freeskb;

        total_length = sizeof(struct arp_hdr) 
                + (dev->addr_len + arp->ar_pln) * 2;

        /* ARP header, with 2 device and 2 network addresses.  */
        if (!pskb_may_pull(skb, total_length))
                goto freeskb;
#if RINA_TEST	
        arp_process(skb);

        return 0;
#else
        return -1;
#endif

 freeskb:
        kfree_skb(skb);
 out_of_mem:
        return 0;

}

/* Taken from net/ipv4/arp.c */
static struct packet_type arp_packet_type __read_mostly = {
        .type =	cpu_to_be16(ETH_P_ARP),
        .func =	arp_rcv,
};

static int __init mod_init(void)
{
        INIT_LIST_HEAD(&data);

        dev_add_pack(&arp_packet_type);

        return 0;
}

static void __exit mod_exit(void)
{ /* FIXME: Destroy everything (e.g. the contents of 'data' */ }

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("Basic implementation of RFC 826");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Sander Vrijders <sander.vrijders@intec.ugent.be>");
