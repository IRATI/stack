/*
 * Qdisc for the shim-eth-vlan (will enable all ports the ipcp uses)
 *
 *   Eduard Grasa <eduard.grasa@i2cat.net>
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

#include <linux/skbuff.h>
#include <net/pkt_sched.h>
#include <net/sch_generic.h>

#define RINA_PREFIX "shim-eth-qdisc"

#include "logs.h"
#include "shim-eth-qdisc.h"
#include "shim-eth-vlan.h"

/*
 * Private data for a rina_shim_eth scheduler containing:
 * (only supports a single flow for the moment)
 *	- ipcp_id id of the shim-eth IPCP that is using this qdisc
 * 	- queue max_size
 * 	- queue enable_thres (size at which the qdisc will enable N+1 ports)
 * 	- the queue
 */
struct shim_eth_qdisc_priv {
	ipc_process_id_t    ipcp_id;
	uint16_t            q_max_size;
	uint16_t            q_enable_thres;
};

static int shim_eth_qdisc_enqueue(struct sk_buff *skb, struct Qdisc *qdisc)
{
	struct shim_eth_qdisc_priv *priv = qdisc_priv(qdisc);

	LOG_INFO("shim-eth-enqueue called; current size is %u", qdisc->q.qlen);
	if (skb_queue_len(&qdisc->q) < priv->q_max_size) {
		qdisc->q.qlen++;
		return __qdisc_enqueue_tail(skb, qdisc, &qdisc->q);
	}

	return qdisc_drop(skb, qdisc);
}

static struct sk_buff *shim_eth_qdisc_dequeue(struct Qdisc *qdisc)
{
	struct shim_eth_qdisc_priv *priv = qdisc_priv(qdisc);

	LOG_INFO("shim-eth-dequeue called; current size is %u", qdisc->q.qlen);

	if (skb_queue_len(&qdisc->q) > 0) {
		struct sk_buff *skb = __qdisc_dequeue_head(qdisc, &qdisc->q);
		qdisc->q.qlen--;
		if (skb_queue_len(&qdisc->q) == priv->q_enable_thres) {
			LOG_INFO("Enabling all ports");
			eth_vlan_enable_write_all(priv->ipcp_id);
		}

		return skb;
	}

	return NULL;
}

static struct sk_buff * shim_eth_qdisc_peek(struct Qdisc *qdisc)
{
	return skb_peek(&qdisc->q);
}

static int shim_eth_qdisc_init(struct Qdisc *qdisc, struct nlattr *opt)
{
	struct shim_eth_qdisc_priv * priv;

	if (!opt) {
		return 0;
	}

	priv = qdisc_priv(qdisc);
	priv->ipcp_id = opt->nla_len;
	priv->q_max_size = 20;
	priv->q_enable_thres = 5;
	skb_queue_head_init(&qdisc->q);

	LOG_INFO("shim-eth-init: ipcp id %u, max size: %u, enable thres: %u",
		 priv->ipcp_id, priv->q_max_size, priv->q_enable_thres);

	return 0;
}

static void shim_eth_qdisc_reset(struct Qdisc *qdisc)
{
	__qdisc_reset_queue(qdisc, &qdisc->q);

	qdisc->qstats.backlog = 0;
	qdisc->q.qlen = 0;
}

static struct Qdisc_ops shim_eth_qdisc_ops __read_mostly = {
	.id		=	"rina_shim_eth",
	.priv_size	=	sizeof(struct shim_eth_qdisc_priv),
	.enqueue	=	shim_eth_qdisc_enqueue,
	.dequeue	=	shim_eth_qdisc_dequeue,
	.peek		=	shim_eth_qdisc_peek,
	.init		=	shim_eth_qdisc_init,
	.reset		=	shim_eth_qdisc_reset,
	.dump		=	NULL,
	.owner		=	THIS_MODULE,
};

struct Qdisc * shim_eth_qdisc_alloc(ipc_process_id_t id,
			            struct net_device * dev)
{
	struct Qdisc * sch;
	struct nlattr  arg;

	sch = qdisc_create_dflt(dev->_tx, &shim_eth_qdisc_ops, 0);
	if (!sch) {
		LOG_ERR("Problems creating shim-eth-qdisc");
		return NULL;
	}

	arg.nla_len = id;
	if (shim_eth_qdisc_ops.init(sch, &arg) == 0) {
		return sch;
	}

	LOG_ERR("Problems initializing shim-eth-qdisc");
	qdisc_destroy(sch);

	return NULL;
}
EXPORT_SYMBOL(shim_eth_qdisc_alloc);

void shim_eth_qdisc_destroy(struct net_device * dev)
{
	qdisc_destroy(dev->qdisc);
}
EXPORT_SYMBOL(shim_eth_qdisc_destroy);
