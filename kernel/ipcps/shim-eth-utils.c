/*
 * Utilities for shim IPCPs over Ethernet
 *
 *    Eduard Grasa <eduard.grasa@i2cat.net>
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

#include <linux/export.h>
#include <linux/types.h>
#include <linux/rtnetlink.h>
#include <net/pkt_sched.h>
#include <net/sch_generic.h>

#define RINA_PREFIX "shim-eth-utils"
#define MAX_NOTIFICATIONS 5

#include "logs.h"
#include "debug.h"
#include "shim-eth.h"
#include "utils.h"

/*
 * Private data for a rina_shim_eth scheduler containing:
 * 	- queue max_size
 * 	- queue enable_thres (size at which the qdisc will enable N+1 ports)
 */
struct shim_eth_qdisc_priv {
	uint16_t q_max_size;
	uint16_t q_enable_thres;
	uint16_t notifications;
	bool 	 started_notifying;
};

static int shim_eth_qdisc_enqueue(struct sk_buff *skb, struct Qdisc *qdisc)
{
	struct shim_eth_qdisc_priv *priv;

	if (!skb) {
		LOG_ERR("Bogus skb passed, bailing out");
		return -1;
	}

	if (!qdisc) {
		LOG_ERR("Bogus qdisc passed, bailing out");
		return -1;
	}

	priv = qdisc_priv(qdisc);

	LOG_DBG("shim-eth-enqueue called; current size is %u", qdisc->q.qlen);
	if (skb_queue_len(&qdisc->q) < priv->q_max_size)
		return __qdisc_enqueue_tail(skb, qdisc, &qdisc->q);

	priv->notifications = MAX_NOTIFICATIONS;
	priv->started_notifying = false;

	return qdisc_drop(skb, qdisc);
}

static struct sk_buff * shim_eth_qdisc_dequeue(struct Qdisc *qdisc)
{
	struct shim_eth_qdisc_priv *priv;

	if (!qdisc) {
		LOG_ERR("Bogus qdisc passed, bailing out");
		return NULL;
	}

	priv = qdisc_priv(qdisc);

	if (skb_queue_len(&qdisc->q) > 0) {
		struct sk_buff *skb = __qdisc_dequeue_head(qdisc, &qdisc->q);
		if (skb_queue_len(&qdisc->q) == priv->q_enable_thres &&
				priv->notifications == MAX_NOTIFICATIONS)
			priv->started_notifying = true;

		if (priv->started_notifying && priv->notifications > 0) {
			enable_write_all(qdisc->dev_queue->dev);
			priv->notifications--;
		}

		return skb;
	}

	return NULL;
}

static struct sk_buff * shim_eth_qdisc_peek(struct Qdisc *qdisc)
{
	if (!qdisc) {
		LOG_ERR("Bogus qdisc passed, bailing out");
		return NULL;
	}

	return skb_peek(&qdisc->q);
}

static int shim_eth_qdisc_init(struct Qdisc *qdisc, struct nlattr *opt)
{
	struct shim_eth_qdisc_priv * priv;

	if (!qdisc) {
		LOG_ERR("Bogus qdisc passed, bailing out");
		return -1;
	}

	if (!opt)
		return 0;

	priv = qdisc_priv(qdisc);
	priv->q_max_size = opt->nla_len;
	priv->q_enable_thres = opt->nla_type;
	priv->notifications = 0;
	priv->started_notifying = false;
	skb_queue_head_init(&qdisc->q);

	LOG_INFO("shim-eth-qdisc-init: max size: %u, enable thres: %u",
		 priv->q_max_size, priv->q_enable_thres);

	return 0;
}

static void shim_eth_qdisc_reset(struct Qdisc *qdisc)
{
	if (!qdisc) {
		LOG_ERR("Bogus qdisc passed, bailing out");
		return;
	}

	__qdisc_reset_queue(qdisc, &qdisc->q);

	qdisc->qstats.backlog = 0;
	qdisc->q.qlen = 0;
}

static struct Qdisc_ops shim_eth_qdisc_ops __read_mostly = {
	.id	   = "rina_shim_eth",
	.priv_size = sizeof(struct shim_eth_qdisc_priv),
	.enqueue   = shim_eth_qdisc_enqueue,
	.dequeue   = shim_eth_qdisc_dequeue,
	.peek	   = shim_eth_qdisc_peek,
	.init	   = shim_eth_qdisc_init,
	.reset	   = shim_eth_qdisc_reset,
	.dump	   = NULL,
	.owner	   = THIS_MODULE,
};

void restore_qdisc(struct net_device * dev)
{
	struct Qdisc * 		    qdisc;
	unsigned int   		    q_index;

	ASSERT(dev);

	rtnl_lock();

	if (dev->flags & IFF_UP)
		dev_deactivate(dev);

	atomic_dec(&dev->qdisc->refcnt);

	/* Destroy all qdiscs in TX queues, point them to no-op (so that
	 * the kernel will create default ones when activating the device)
	 */
	for (q_index = 0; q_index < dev->num_tx_queues; q_index++) {
		qdisc = dev_graft_qdisc(&dev->_tx[q_index], NULL);
		if (qdisc && qdisc != &noop_qdisc)
			qdisc_destroy(qdisc);
	}
	dev->qdisc = &noop_qdisc;

	if (dev->flags & IFF_UP)
		dev_activate(dev);

	rtnl_unlock();
}
EXPORT_SYMBOL(restore_qdisc);

int  update_qdisc(struct net_device * dev,
		  uint16_t qdisc_max_size,
		  uint16_t qdisc_enable_size)
{
	struct Qdisc *        qdisc;
	struct Qdisc *        oqdisc;
	struct nlattr         attr;
	unsigned int   	      q_index;

	ASSERT(dev);

	if (!dev->qdisc) {
		LOG_ERR("qdisc not found on device %s", dev->name);
		return -1;
	}

	if (string_cmp(dev->qdisc->ops->id,
		       shim_eth_qdisc_ops.id) == 0) {
		LOG_INFO("Shim-eth-qdisc already set on this device");
		return 0;
	}

	rtnl_lock();
	/* For each TX queue, create a new shim-eth-qdisc instance, attach
	 * it to the queue and then destroy the old qdisc
	 */
	for (q_index = 0; q_index < dev->num_tx_queues; q_index++) {
		qdisc = qdisc_create_dflt(&dev->_tx[q_index],
					  &shim_eth_qdisc_ops,
					  0);
		if (!qdisc) {
			LOG_ERR("Problems creating shim-eth-qdisc");
			rtnl_unlock();
			restore_qdisc(dev);
			return -1;
		}

		attr.nla_len = qdisc_max_size;
		attr.nla_type = qdisc_enable_size;
		if (shim_eth_qdisc_init(qdisc, &attr)) {
			LOG_ERR("Problems initializing shim-eth-qdisc");
			qdisc_destroy(qdisc);
			rtnl_unlock();
			restore_qdisc(dev);
			return -1;
		}

		oqdisc = dev_graft_qdisc(&dev->_tx[q_index], qdisc);
		if (oqdisc && oqdisc != &noop_qdisc)
			qdisc_destroy(oqdisc);
	}

	if (dev->flags & IFF_UP)
		dev_deactivate(dev);

	dev->qdisc = netdev_get_tx_queue(dev, 0)->qdisc_sleeping;
	atomic_inc(&dev->qdisc->refcnt);

	if (dev->flags & IFF_UP)
		dev_activate(dev);

	rtnl_unlock();
	return 0;
}
EXPORT_SYMBOL(update_qdisc);
