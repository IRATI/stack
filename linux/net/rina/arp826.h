/*
 * An ARP RFC-826 (wonnabe) compliant implementation
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

#ifndef ARP_826_H
#define ARP_826_H

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/if_ether.h>

struct naddr_handle;
struct naddr_filter;

struct paddr {
	void * buf;
	size_t length;
};

enum rinarp_mac_addr_type {
        MAC_ADDR_802_3
};

struct rinarp_mac_addr {
        enum rinarp_mac_addr_type type;
        union {
                uint8_t mac_802_3[6];
        } data;
};

/* FIXME: All these rinarp_* thingies have to be removed */

struct naddr_handle * rinarp_paddr_register(__be16              proto_name,
					    __be16              proto_len,
                                            struct net_device * device,
                                            struct paddr        address);
int                   rinarp_paddr_unregister(struct naddr_handle * h);

typedef void (* arp_handler_t)(void *                         opaque,
			       const struct paddr *           dest_paddr,
                               const struct rinarp_mac_addr * dest_hw_addr);

struct naddr_filter * naddr_filter_create(struct naddr_handle * handle);
int                   naddr_filter_set(struct naddr_filter * filter,
				       void *                opaque,
                                       arp_handler_t         request,
                                       arp_handler_t         reply);
int                   naddr_filter_destroy(struct naddr_filter * filter);

int                   rinarp_hwaddr_get(struct naddr_filter *    filter, 
                                        struct paddr             in_address,
                                        struct rinarp_mac_addr * out_addr);
int                   rinarp_paddr_get(struct naddr_filter *  filter, 
				       struct rinarp_mac_addr in_address,
				       struct paddr         * out_addr);

int                   rinarp_send_request(struct naddr_filter * filter, 
                                          struct paddr          address);

#endif

#if 0
struct naddr_handle * rinarp_paddr_register(__be16              proto_name,
					    __be16              proto_len,
                                            struct net_device * device,
                                            struct paddr        address);
int                   rinarp_paddr_unregister(struct naddr_handle * h);

typedef void (* arp_handler_t)(void *                         opaque,
			       const struct paddr *           dest_paddr,
                               const struct rinarp_mac_addr * dest_hw_addr);

int                   rinarp_hwaddr_get(struct naddr_handle *    handle, 
                                        struct paddr             in_address,
                                        arp_handler_t            handler);

#endif
