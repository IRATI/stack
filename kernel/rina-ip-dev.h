/*
 * RINA IP virtual network device (rina_ip_dev)
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

struct rcache_entry {
	ipaddr_t ip;
	ipaddr_t mask;
	port_id_t port;
        struct list_head next;
};

struct rcache {
        struct list_head head;
	spinlock_t lock;
};

struct rina_ip_dev;

int				rina_ip_dev_open(struct net_device *dev);
int				rina_ip_dev_close(struct net_device *dev);
struct net_device_stats*	rina_ip_dev_get_stats(struct net_device *dev);
int				rina_ip_dev_start_xmit(struct sk_buff *skb,
							struct net_device *dev);
void				rina_ip_dev_setup(struct net_device *dev);
struct rina_ip_dev*		rina_ip_dev_create(struct ipcp_instance*
								kfa_ipcp);
int				rina_ip_dev_destroy(struct rina_ip_dev *ip_dev);

int				rcache_entry_add(ipaddr_t ip, ipaddr_t mask,
						port_id_t port,
						struct rcache* rcache);
port_id_t			rcache_get_port(ipaddr_t ip,
							struct rcache* rcache);
int				rcache_entry_remove(ipaddr_t ip, ipaddr_t mask,
							struct rcache* rcache);
int				rcache_flush(struct rcache * rcache);
#endif
