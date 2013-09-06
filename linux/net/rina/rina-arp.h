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
struct naddr_handle;
struct naddr_filter;

typedef unsigned char * naddr_t;

struct naddr_handle * rinarp_naddr_register(__be16              proto, 
                                            __be16              proto_length,
                                            struct net_device * device,
                                            naddr_t             address);
int                   rinarp_naddr_unregister(struct naddr_handle * h);

typedef void (* arp_handler_t)(struct sk_buff * skb);

struct naddr_filter * naddr_filter_create(struct naddr_handle * handle);
int                   naddr_filter_set(struct naddr_filter * filter,
                                       arp_handler_t         request,
                                       arp_handler_t         reply);
int                   naddr_filter_destroy(struct naddr_filter * filter);
arp_handler_t         naddr_filter_req_handler_get(struct naddr_handle * h);
arp_handler_t         naddr_filter_rep_handler_get(struct naddr_handle * h);
           
/* 
 * Checks for a network address in the ARP cache. Returns the network address
 * if present NULL if not present
 */
naddr_t               rinarp_naddr_get(struct naddr_filter * filter, 
                                       naddr_t               address);

int                   rinarp_send_request(struct naddr_filter * filter, 
                                          naddr_t               address);
#endif

#endif
