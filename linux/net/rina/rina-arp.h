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

#ifndef RINA_ARP_H
#define RINA_ARP_H

struct arp_reply_ops {
	struct list_head    list;
	__be16              ar_pro; 
	struct net_device * dev; 
	unsigned char *     src_netw_addr;
	unsigned char *     dest_netw_addr;
	void                (*handle)(struct sk_buff *skb);
};

int             rinarp_register_netaddr(__be16 ar_pro, 
					struct net_device *dev, 
					unsigned char *netw_addr);
int             rinarp_unregister_netaddr(__be16 ar_pro, 
					  struct net_device *dev, 
					  unsigned char *netw_addr);
unsigned char * rinarp_lookup_netaddr(__be16 ar_pro, 
				      struct net_device *dev, 
				      unsigned char *netw_addr);
int             rinarp_send_request(struct arp_reply_ops *ops);
int             rinarp_remove_reply_handler(struct arp_reply_ops *ops);

#if 0


/*
 * Generic use:
 *   Create a netaddr_handle by registering a network address
 *   Get a filter by calling netaddr_filter_set
 *   This filter can be used to lookup ARP entries and send ARP requests
 */

struct netaddr_handle;
struct netaddr_filter;

struct netaddr_handle * rinarp_netaddr_register(__be16              ar_pro, 
						__be16              ar_pro_len,
                                                struct net_device * dev, 
                                                unsigned char *     netw_addr);
int                     rinarp_netaddr_unregister(struct netaddr_handle * h);

struct netaddr_filter * netaddr_filter_set(
	struct netaddr_handle * net_handle,
	void (*arp_req_handle)(struct sk_buff *skb),
	void (*arp_rep_handle)(struct sk_buff *skb));

void (*netaddr_filter_get_req(struct netaddr_handle * net_handle))
(struct sk_buff *skb);
void (*netaddr_filter_get_rep(struct netaddr_handle * net_handle))
(struct sk_buff *skb);

/* 
 *  Checks for a network address in the ARP cache 
 *  Returns the network address if present
 *  NULL if not present
 */
unsigned char * rinarp_get_netaddr(struct netaddr_filter * filter, 
				   unsigned char         * netw_addr);

int rinarp_send_request(struct netaddr_filter * filter, 
			unsigned char         * netw_addr);
#endif

#endif
