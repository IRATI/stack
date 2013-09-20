/*
 * Implementation of RFC 826 because current implementation is too
 * intertwined with IP version 4.
 *
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#include "rina-arp-new.h"

struct naddr_handle * rinarp_paddr_register(__be16              proto_name,
					    __be16              proto_len,
                                            struct net_device * device,
                                            struct paddr        address)
{ return NULL; }
EXPORT_SYMBOL(rinarp_paddr_register);

int rinarp_paddr_unregister(struct naddr_handle * h)
{ return -1; }
EXPORT_SYMBOL(rinarp_paddr_unregister);

struct naddr_filter * naddr_filter_create(struct naddr_handle * handle)
{ return NULL; }
EXPORT_SYMBOL(naddr_filter_create);

int naddr_filter_set(struct naddr_filter * filter,
		     void *                opaque,
		     arp_handler_t         request,
		     arp_handler_t         reply)
{ return -1; }
EXPORT_SYMBOL(naddr_filter_set);

int naddr_filter_destroy(struct naddr_filter * filter)
{ return -1; }
EXPORT_SYMBOL(naddr_filter_destroy);

int rinarp_hwaddr_get(struct naddr_filter *    filter, 
		      struct paddr             in_address,
		      struct rinarp_mac_addr * out_addr)
{ return -1; }
EXPORT_SYMBOL(rinarp_hwaddr_get);

int rinarp_send_request(struct naddr_filter * filter, 
                        struct paddr          address)
{ return -1; }
EXPORT_SYMBOL(rinarp_send_request);

static int __init mod_init(void)
{ return 0; }

static void __exit mod_exit(void)
{ /* FIXME: Destroy everything (e.g. the contents of 'data' */ }

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("Basic RFC 826 compliant ARP implementation");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Sander Vrijders <sander.vrijders@intec.ugent.be>");
MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
