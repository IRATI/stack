/*
 * Implementation of RFC 826 because current implementation is too
 * intertwined with IP version 4.
 *
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

#ifdef RINA_ARP

#include "rina-arp.h"



int add_arp_reply_handler(struct arp_reply_ops *ops)
{



}
EXPORT_SYMBOL(add_arp_reply_handler);

int remove_arp_reply_handler(struct arp_reply_ops *ops)
{



}
EXPORT_SYMBOL(remove_arp_reply_handler);

int register_network_address(__be16 ar_pro, 
			     struct net_device *dev, 
			     unsigned char *netw_addr)
{



}
EXPORT_SYMBOL(register_network_address);

int unregister_network_address(__be16 ar_pro, 
			       struct net_device *dev, 
			       unsigned char *netw_addr)
{



}
EXPORT_SYMBOL(unregister_network_address);

unsigned char * lookup_network_address(__be16 ar_pro, 
				       struct net_device *dev, 
				       unsigned char *netw_addr)
{



}
EXPORT_SYMBOL(lookup_network_address);

/*
 *	Create an arp packet. If (dest_hw == NULL), we create a broadcast
 *	message.
 */
static struct sk_buff *arp_create(int type, int ptype, __be32 dest_ip,
			   struct net_device *dev, __be32 src_ip,
			   const unsigned char *dest_hw,
			   const unsigned char *src_hw,
			   const unsigned char *target_hw)
{
	struct sk_buff *skb;
	struct arphdr *arp;
	unsigned char *arp_ptr;
	int hlen = LL_RESERVED_SPACE(dev);
	int tlen = dev->needed_tailroom;

	/*
	 *	Allocate a buffer
	 */

	skb = alloc_skb(arp_hdr_len(dev) + hlen + tlen, GFP_ATOMIC);
	if (skb == NULL)
		return NULL;

	skb_reserve(skb, hlen);
	skb_reset_network_header(skb);
	arp = (struct arphdr *) skb_put(skb, arp_hdr_len(dev));
	skb->dev = dev;
	skb->protocol = htons(ETH_P_ARP);
	if (src_hw == NULL)
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
	 *
	 * The arp hardware type should match the device type, except for FDDI,
	 * which (according to RFC 1390) should always equal 1 (Ethernet).
	 */
	/*
	 *	Exceptions everywhere. AX.25 uses the AX.25 PID value not the
	 *	DIX code for the protocol. Make these device structure fields.
	 */
	switch (dev->type) {
	default:
		arp->ar_hrd = htons(dev->type);
		arp->ar_pro = htons(ETH_P_IP);
		break;
	}

	arp->ar_hln = dev->addr_len;
	arp->ar_pln = 4;
	arp->ar_op = htons(type);

	arp_ptr = (unsigned char *)(arp + 1);

	memcpy(arp_ptr, src_hw, dev->addr_len);
	arp_ptr += dev->addr_len;
	memcpy(arp_ptr, &src_ip, 4);
	arp_ptr += 4;

	switch (dev->type) {
	default:
		if (target_hw != NULL)
			memcpy(arp_ptr, target_hw, dev->addr_len);
		else
			memset(arp_ptr, 0, dev->addr_len);
		arp_ptr += dev->addr_len;
	}
	memcpy(arp_ptr, &dest_ip, 4);

	return skb;

out:
	kfree_skb(skb);
	return NULL;
}


/*
 *	Create and send an arp packet.
 */
void arp_send_request(__be16 ar_pro, 
		      struct net_device *dev, 
		      unsigned char *src_netw_addr,
		      unsigned char *dest_netw_addr)
{
	struct sk_buff *skb;

	/*
	 *	No arp on this interface.
	 */

	if (dev->flags&IFF_NOARP)
		return;

	skb = arp_create(type, ptype, dest_ip, dev, src_ip,
			 dest_hw, src_hw, target_hw);
	if (skb == NULL)
		return;

	NF_HOOK(NFPROTO_ARP, NF_ARP_OUT, skb, NULL, skb->dev, dev_queue_xmit);
}
EXPORT_SYMBOL(arp_send_request);

/*
 *	Process an arp request.
 */

static int arp_process(struct sk_buff *skb)
{
	struct net_device *dev = skb->dev;
	struct in_device *in_dev = __in_dev_get_rcu(dev);
	struct arphdr *arp;
	unsigned char *arp_ptr;

	unsigned char *sha;
	__be32 sip, tip;
	u16 dev_type = dev->type;
	int addr_type;
	struct neighbour *n;
	struct net *net = dev_net(dev);

	/* arp_rcv below verifies the ARP header and verifies the device
	 * is ARP'able.
	 */

	if (in_dev == NULL)
		goto out;

	arp = arp_hdr(skb);

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
		if ((arp->ar_hrd != htons(ARPHRD_ETHER)
			goto out;
		break;
	}

	/* Understand only these message types */
	if (arp->ar_op != htons(ARPOP_REPLY) &&
	    arp->ar_op != htons(ARPOP_REQUEST))
		goto out;

/*
 *	Extract protocol addresses
 */
	arp_ptr = (unsigned char *)(arp + 1);
	sha	= arp_ptr;
	arp_ptr += dev->addr_len;
	memcpy(&sip, arp_ptr, arp->ar_pln);
	arp_ptr += arp->ar_pln;
	switch (dev_type) {
	default:
		arp_ptr += dev->addr_len;
	}
	memcpy(&tip, arp_ptr, arp->ar_pln);

/*
 *  Process entry. The idea here is we want to send a reply if it is a
 *  request for us. We want to add an entry to our cache if it is a reply
 *  to us or if it is a request for our address.
 *
 *  Putting this another way, we only care about replies if they are to
 *  us, in which case we add them to the cache.  For requests, we care
 *  about those for us. We add the requester to the arp cache.
 */

	if (arp->ar_op == htons(ARPOP_REQUEST) &&
	    ip_route_input_noref(skb, tip, sip, 0, dev) == 0) {

		rt = skb_rtable(skb);
		addr_type = rt->rt_type;

		if (addr_type == RTN_LOCAL) {
			int dont_send;

			dont_send = arp_ignore(in_dev, sip, tip);
			if (!dont_send && IN_DEV_ARPFILTER(in_dev))
				dont_send = arp_filter(sip, tip, dev);
			if (!dont_send) {
				n = neigh_event_ns(&arp_tbl, sha, &sip, dev);
				if (n) {
					arp_send(ARPOP_REPLY, ETH_P_ARP, sip,
						 dev, tip, sha, dev->dev_addr,
						 sha);
					neigh_release(n);
				}
			}
			goto out;
		}
	}

	/* Update our ARP tables */

	n = __neigh_lookup(&arp_tbl, &sip, dev, 0);

	if (IN_DEV_ARP_ACCEPT(in_dev)) {
		/* Unsolicited ARP is not accepted by default.
		   It is possible, that this option should be enabled for some
		   devices (strip is candidate)
		 */
		if (n == NULL &&
		    (arp->ar_op == htons(ARPOP_REPLY) ||
		     (arp->ar_op == htons(ARPOP_REQUEST) && tip == sip)) &&
		    inet_addr_type(net, sip) == RTN_UNICAST)
			n = __neigh_lookup(&arp_tbl, &sip, dev, 1);
	}

	if (n) {
		int state = NUD_REACHABLE;
		int override;

		/* If several different ARP replies follows back-to-back,
		   use the FIRST one. It is possible, if several proxy
		   agents are active. Taking the first reply prevents
		   arp trashing and chooses the fastest router.
		 */
		override = time_after(jiffies, n->updated + n->parms->locktime);

		/* Broadcast replies and request packets
		   do not assert neighbour reachability.
		 */
		if (arp->ar_op != htons(ARPOP_REPLY) ||
		    skb->pkt_type != PACKET_HOST)
			state = NUD_STALE;
		neigh_update(n, sha, state,
			     override ? NEIGH_UPDATE_F_OVERRIDE : 0);
		neigh_release(n);
	}

out:
	consume_skb(skb);
	return 0;
}

static int arp_rcv(struct sk_buff *skb, struct net_device *dev,
		   struct packet_type *pt, struct net_device *orig_dev)
{
	const struct arphdr *arp;
	int total_length;

	if (dev->flags & IFF_NOARP ||
	    skb->pkt_type == PACKET_OTHERHOST ||
	    skb->pkt_type == PACKET_LOOPBACK)
		goto freeskb;

	skb = skb_share_check(skb, GFP_ATOMIC);
	if (!skb)
		goto out_of_mem;

	/* ARP header, without 2 device and 2 network addresses.  */
	if (!pskb_may_pull(skb, sizeof(struct arphdr)))
		goto freeskb;

	arp = arp_hdr(skb);
	if (arp->ar_hln != dev->addr_len)
		goto freeskb;

	total_length = sizeof(struct arphdr) 
		+ (dev->addr_len + arp->ar_pln) * 2;

        /* ARP header, with 2 device and 2 network addresses.  */
	if (!pskb_may_pull(skb, total_length))
		goto freeskb;
	
	arp_process(skb);

	return 0;

freeskb:
	kfree_skb(skb);
out_of_mem:
	return 0;
}

static struct packet_type arp_packet_type __read_mostly = {
	.type =	cpu_to_be16(ETH_P_ARP),
	.func =	arp_rcv,
};


static int __init mod_init(void)
{
	dev_add_pack(&arp_packet_type);

        return 0;
}

static void __exit mod_exit(void)
{
	
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("Basic implementation of RFC 826");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Sander Vrijders <sander.vrijders@intec.ugent.be>");

#endif
