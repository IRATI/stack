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

#include <linux/if_arp.h>
#include <linux/ip.h>
#include <linux/if.h>
#include <linux/version.h>

#define RINA_PREFIX "rina-device"

#include "logs.h"
#include "debug.h"
#include "kfa.h"
#include "du.h"
#include "rds/rmem.h"
#include "rina-device.h"

#define RINA_EXTRA_HEADER_LENGTH 50

struct rina_device {
	struct net_device_stats stats;
	struct ipcp_instance* kfa_ipcp;
	port_id_t port;
	struct net_device* dev;
};

static int rina_dev_open(struct net_device *dev)
{
	netif_tx_start_all_queues(dev);
	LOG_DBG("RINA IP device %s opened...", dev->name);

	return 0;
}

static int rina_dev_close(struct net_device *dev)
{
	netif_tx_stop_all_queues(dev);
	LOG_DBG("RINA IP device %s closed...", dev->name);

	return 0;
}

static struct net_device_stats *rina_dev_get_stats(struct net_device *dev)
{
	struct rina_device* rina_dev = netdev_priv(dev);
	if(!rina_dev)
		return NULL;
	
	return &rina_dev->stats;
}

int rina_dev_rcv(struct sk_buff *skb, struct rina_device *rina_dev)
{
	ssize_t len;

	if(!(skb->data[0] & 0xf0)) {
		LOG_INFO("RINA IP device %s rcv a non IP packet, dropping...",
			  rina_dev->dev->name);
		rina_dev->stats.rx_dropped++;
		kfree_skb(skb);
		return -1;
	}

	skb->protocol = htons(ETH_P_IP);
	skb->dev = rina_dev->dev;
	len = skb->len;

	if(likely(netif_rx(skb) == NET_RX_SUCCESS)) {
		rina_dev->stats.rx_packets++;
		rina_dev->stats.rx_bytes += len;
	} else {
		rina_dev->stats.rx_dropped++;
	}

	LOG_DBG("RINA IP device %s rcv a IP packet...",
		rina_dev->dev->name);

	return 0;
}

static int rina_dev_start_xmit(struct sk_buff * skb, struct net_device *dev)
{
	struct iphdr* iph = NULL;
	struct rina_device* rina_dev = netdev_priv(dev);
	ssize_t data_sent;
	ASSERT(rina_dev);

	skb_orphan(skb);
	//skb_dst_force(skb);

	iph = ip_hdr(skb);
	ASSERT(iph);

	LOG_DBG("Device %s about to send a packet of length %u via port %d",
		dev->name, skb->len, rina_dev->port);

	data_sent = kfa_flow_skb_write(rina_dev->kfa_ipcp->data,
				       rina_dev->port, skb, skb->len, false);
	if (data_sent < 0) {
		rina_dev->stats.tx_dropped++;
		LOG_ERR("Could not xmit IP packet, unable to send to KFA...");
		return NET_XMIT_DROP;
	}

	rina_dev->stats.tx_packets++;
	rina_dev->stats.tx_bytes += data_sent;

	LOG_DBG("RINA IP device %s sent a packet of %zd bytes via port %d",
		dev->name, data_sent, rina_dev->port);

	return NETDEV_TX_OK;
}

static void rina_dev_free(struct net_device *dev)
{ return free_netdev(dev); }

static const struct net_device_ops rina_dev_ops = {
	.ndo_start_xmit	= rina_dev_start_xmit,
	.ndo_get_stats	= rina_dev_get_stats,
	.ndo_open	= rina_dev_open,
	.ndo_stop	= rina_dev_close,
};

static void rina_dev_setup(struct net_device *dev)
{
	/* This should be set according to the N-1 DIF properties,
         * for the moment an upper bound is provided */
	dev->needed_headroom += RINA_EXTRA_HEADER_LENGTH;
	dev->needed_tailroom += RINA_EXTRA_HEADER_LENGTH;
	/* This should be set depending on supporting DIF */
	dev->mtu = 1400;
	dev->hard_header_len = 0;
	dev->addr_len = 0;
	dev->type = ARPHRD_NONE;
	dev->flags = IFF_POINTOPOINT | IFF_NOARP | IFF_MULTICAST;
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,3,0)
	dev->priv_flags |= IFF_LIVE_ADDR_CHANGE
#else
	dev->priv_flags	|= IFF_LIVE_ADDR_CHANGE | IFF_NO_QUEUE
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,6,0)
		| IFF_DONT_BRIDGE;
#else
		| IFF_DONT_BRIDGE | IFF_PHONY_HEADROOM;
#endif
	netif_keep_dst(dev);
	dev->features = NETIF_F_HW_CSUM;
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,11,9)
	dev->destructor	= rina_dev_free;
#else
	dev->priv_destructor = rina_dev_free;
#endif
	dev->netdev_ops	= &rina_dev_ops;

	return;
}

struct rina_device* rina_dev_create(string_t* name,
				    struct ipcp_instance* kfa_ipcp,
				    port_id_t port)
{
	int rv;
	struct net_device *dev;
	struct rina_device* rina_dev;

	if (!kfa_ipcp || !name)
		return NULL;

	if (strlen(name) > IFNAMSIZ) {
		LOG_ERR("Could not allocate RINA IP network device %s, "
				"name too long", name);
		return NULL;
	}

	dev = alloc_netdev(sizeof(struct rina_device), name,
			   NET_NAME_UNKNOWN, rina_dev_setup);
	if (!dev) {
		LOG_ERR("Could not allocate RINA IP network device %s", name);
		return NULL;
	}

	rina_dev = netdev_priv(dev);
	rina_dev->dev = dev;
	rina_dev->kfa_ipcp = kfa_ipcp;
	rina_dev->port = port;
	memset(&rina_dev->stats, 0x00, sizeof(struct net_device_stats));

	rv = register_netdev(dev);
	if(rv) {
		LOG_ERR("Could not register RINA IP device %s: %d", name, rv);
		free_netdev(dev);
		return NULL;
	}

	LOG_DBG("RINA IP device %s (%pk) created with dev %p",
		name, rina_dev, dev);

	return rina_dev;
}

int rina_dev_destroy(struct rina_device *rina_dev)
{
	if(!rina_dev)
		return -1;

	unregister_netdev(rina_dev->dev);
	return 0;
}
