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
struct arp_reply_ops {
	__be16              ar_pro; 
	unsigned char *     src_netw_addr;
	unsigned char *     dest_netw_addr;
	void                (*handle)(struct sk_buff *skb);
};

struct netaddr_handle;

struct netaddr_handle * rinarp_netaddr_register(__be16              ar_pro, 
                                                struct net_device * dev, 
                                                unsigned char *     netw_addr);
int                     rinarp_netaddr_unregister(struct netaddr_handle * h);

unsigned char *         rinarp_lookup_netaddr(__be16 ar_pro, 
                                              struct net_device *dev, 
                                              unsigned char *netw_addr);
int             rinarp_send_request(struct arp_reply_ops *ops);
int             rinarp_remove_reply_handler(struct arp_reply_ops *ops);
#endif

#endif
