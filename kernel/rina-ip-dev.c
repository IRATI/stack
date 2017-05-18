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

#include <linux/if_arp.h>

#define RINA_PREFIX "rina-ip-dev"

#include "logs.h"
#include "rds/rmem.h"
#include "rina-ip-dev.h"


struct rina_ip_dev {
	struct rcache rcache;
	struct net_device_stats stats;
	struct net_device* dev;
};

int rina_ip_dev_open(struct net_device *dev)
{
	LOG_DBG("RINA IP device opened...");
	return 0;
}

int rina_ip_dev_close(struct net_device *dev)
{
	LOG_DBG("RINA IP device closed...");
	return 0;
}

struct net_device_stats *rina_ip_dev_get_stats(struct net_device *dev)
{
    struct rina_ip_dev* ip_dev = netdev_priv(dev);
    if(!ip_dev)
	    return NULL;
    return &ip_dev->stats;
}

int rina_ip_dev_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
    return 0;
}

static const struct net_device_ops rina_ip_dev_ops = {
	.ndo_start_xmit	= rina_ip_dev_start_xmit,
	.ndo_get_stats	= rina_ip_dev_get_stats,
	.ndo_open	= rina_ip_dev_open,
	.ndo_stop	= rina_ip_dev_close,
};

/* as ether_setup to set internal dev fields */
void rina_ip_dev_setup(struct net_device *dev)
{
	/*Mix of properties from lo and tun ifaces */
	/* This should be set depending on supporting DIF */
	dev->mtu = 64 * 1024;
	dev->hard_header_len = 0;
	dev->addr_len = 0;
	dev->type = ARPHRD_NONE;
	dev->flags = IFF_POINTOPOINT | IFF_NOARP | IFF_MULTICAST;
	dev->priv_flags	|= IFF_LIVE_ADDR_CHANGE | IFF_NO_QUEUE
		| IFF_DONT_BRIDGE
		| IFF_PHONY_HEADROOM;
	netif_keep_dst(dev);
	dev->hw_features = NETIF_F_GSO_SOFTWARE;
	dev->features = NETIF_F_SG | NETIF_F_FRAGLIST
		| NETIF_F_GSO_SOFTWARE
		| NETIF_F_HW_CSUM
		| NETIF_F_RXCSUM
		| NETIF_F_SCTP_CRC
		| NETIF_F_HIGHDMA
		| NETIF_F_LLTX
		| NETIF_F_NETNS_LOCAL
		| NETIF_F_VLAN_CHALLENGED
		| NETIF_F_LOOPBACK;
	/*
	dev->ethtool_ops	= &loopback_ethtool_ops;
	dev->header_ops		= &eth_header_ops;
	dev->destructor		= loopback_dev_free;
	*/
	dev->netdev_ops	= &rina_ip_dev_ops;
	//dev->tx_queue_len = 0;

	return;
}

struct rina_ip_dev* rina_ip_dev_create(void)
{
	int rv;
	struct net_device *dev;
	struct rina_ip_dev* rina_dev;

	dev = alloc_netdev(sizeof(struct rina_ip_dev), "rina_ip",
							NET_NAME_UNKNOWN,
			      				rina_ip_dev_setup);
	if (!dev) {
		LOG_ERR("Could not allocate RINA IP network device");
		return NULL;
	}

	//SET_NETDEV_DEV(dev, parent);

	rina_dev = netdev_priv(dev);
	rina_dev->dev = dev;

        INIT_LIST_HEAD(&rina_dev->rcache.head);
	spin_lock_init(&rina_dev->rcache.lock);

	rv = register_netdev(dev);
	if(rv) {
		LOG_ERR("Could not register RINA IP device: %d", rv);
		free_netdev(dev);
		return NULL;
	}

	LOG_DBG("RINA IP device %p created with dev %p", rina_dev, dev);

	return rina_dev;
}

int rina_ip_dev_destroy(struct rina_ip_dev *ip_dev)
{
	if(!ip_dev)
		return -1;

	rcache_flush(&ip_dev->rcache);
	unregister_netdev(ip_dev->dev);
	free_netdev(ip_dev->dev);
	return 0;
}

int rcache_entry_add(ipaddr_t ip, ipaddr_t mask,
                                                struct ipcp_instance_data* data,
					        port_id_t port,
						struct rcache* rcache)
{
        struct rcache_entry * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
        if (!tmp)
                return -1;

        tmp->ip = ip;
        tmp->mask = mask;
	tmp->flow.data = data;
	tmp->flow.port = port;

        INIT_LIST_HEAD(&tmp->next);

	spin_lock(&rcache->lock);
	list_add(&tmp->next, &rcache->head);
	spin_unlock(&rcache->lock);

        return 0;
}

struct flow_info* rcache_get_flow_info(ipaddr_t ip, struct rcache* rcache)
{
        struct rcache_entry * cur;
	struct flow_info * flow;

        if (!rcache)
                return NULL;

	spin_lock(&rcache->lock);
        list_for_each_entry(cur, &rcache->head, next) {
		if ((ip&cur->mask) & (cur->ip&cur->mask)) {
                	flow = &cur->flow;
			spin_unlock(&rcache->lock);
			return flow;
		}
        }
	spin_unlock(&rcache->lock);

        return NULL;
}

int rcache_entry_remove(ipaddr_t ip, ipaddr_t mask,
                                            struct rcache* rcache)
{
        struct rcache_entry * cur, * n;
        if (!rcache)
                return -1;

	spin_lock(&rcache->lock);
        list_for_each_entry_safe(cur, n, &rcache->head, next) {
		if (ip == cur->ip && mask == cur->mask) {
        		list_del(&cur->next);
        		rkfree(cur);
			spin_unlock(&rcache->lock);
			return 0;
		}
        }
	spin_unlock(&rcache->lock);

        return 0;
}

int rcache_flush(struct rcache * rcache)
{
        struct rcache_entry * cur, * n;
        if (!rcache)
                return -1;

	spin_lock(&rcache->lock);
        list_for_each_entry_safe(cur, n, &rcache->head, next) {
        	list_del(&cur->next);
        	rkfree(cur);
		spin_unlock(&rcache->lock);
        }
	spin_unlock(&rcache->lock);

        return 0;
}
