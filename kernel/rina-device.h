/*
 * RINA virtual network device (rina_device)
 * At the moment only for IP support
 *
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* For net_device related code */

#ifndef RINA_IP_DEV_H
#define RINA_IP_DEV_H

#include <linux/if_arp.h>
#include "common.h"

#define ipaddr_t __be32

struct ipcp_instance;

struct rina_device;

struct rina_device* rina_dev_create(string_t *name,
				    struct ipcp_instance* kfa_ipcp,
			  	    port_id_t port);
int		    rina_dev_destroy(struct rina_device *rina_dev);
int		    rina_dev_rcv(struct sk_buff *skb,
			  	 struct rina_device *rina_dev);

#endif
