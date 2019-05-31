/*
 * DCTCP RMT PS
 *
 *    Matej Gregr <igregr@fit.vutbr.cz>
 *
 * This program is free software; you can dummyistribute it and/or modify
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
#include <linux/module.h>
#include <linux/string.h>

#define RINA_PREFIX "dctcp-rmt-ps"

#include "logs.h"
#include "rds/rmem.h"
#include "rmt-ps.h"
#include "policies.h"

#define DEFAULT_Q_THRESHOLD 20
#define DEFAULT_Q_MAX 200

struct dctcp_rmt_ps_data {
	/* Threshold after that we will begin to mark packets. */
	unsigned int q_threshold;
	/* Max length of a queue. */
	unsigned int q_max;
};

struct dctcp_rmt_queue {
	struct rfifo*  queue;
	port_id_t      port_id;
};

static struct dctcp_rmt_queue* dctcp_queue_create(port_id_t port_id)
{
	struct dctcp_rmt_queue* tmp;

	tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
	if (!tmp) {
		return NULL;
	}

	tmp->queue = rfifo_create_ni();
	if (!tmp->queue) {
		rkfree(tmp);
		return NULL;
	}
	tmp->port_id = port_id;

	return tmp;
}

static int dctcp_rmt_queue_destroy(struct dctcp_rmt_queue *q)
{
	if (!q) {
		LOG_ERR("No DCTCP RMT Key-queue to destroy...");
		return -1;
	}
	if (q->queue) rfifo_destroy(q->queue, (void (*)(void *)) du_destroy);

	rkfree(q);

	return 0;
}

static void * dctcp_rmt_q_create_policy(struct rmt_ps *ps,
		struct rmt_n1_port *port)
{
	struct dctcp_rmt_queue     *q;
	struct dctcp_rmt_ps_data   *data;

	if (!ps || !port || !ps->priv) {
		LOG_ERR("DCTCP RMT: Wrong input parameters");
		return NULL;
	}

	data = ps->priv;

	q = dctcp_queue_create(port->port_id);
	if (!q) {
		LOG_ERR("DCTCP RMT: Could not create queue for n1_port %u",
				port->port_id);
		return NULL;
	}

	LOG_DBG("DCTCP RMT: Structures for scheduling policies created...");
	return q;
}

static int dctcp_rmt_q_destroy_policy(struct rmt_ps *ps, struct rmt_n1_port *port)
{
	struct dctcp_rmt_queue    *q;

	if (!ps || !port) {
		LOG_ERR("DCTCP RMT: Wrong input parameters for dctcp_rmt_scheduling_destroy_policy");
		return -1;
	}

	q = port->rmt_ps_queues;
	if (q) {
		return dctcp_rmt_queue_destroy(q);
	}

	return -1;
}

static int dctcp_rmt_enqueue_policy(struct rmt_ps *ps,
				    struct rmt_n1_port *port,
				    struct du * du,
				    bool must_enqueue)
{
	struct dctcp_rmt_queue   * q;
	struct dctcp_rmt_ps_data * data = ps->priv;
	unsigned int 		   qlen;
	pdu_flags_t     	   pci_flags;

	if (!ps || !port || !du || !data) {
		LOG_ERR("DCTCP RMT: Wrong input parameters"
				" for dctcp_enqueu_scheduling_policy_tx");
		return RMT_PS_ENQ_ERR;
	}

	q = port->rmt_ps_queues;
	if (!q) {
		LOG_ERR("DCTCP RMT: Could not find queue for n1_port %u",
			port->port_id);
		du_destroy(du);
		return RMT_PS_ENQ_ERR;
	}

	qlen = rfifo_length(q->queue);
	if (qlen >= data->q_threshold && qlen < data->q_max) {
		pci_flags = pci_flags_get(&du->pci);
		pci_flags_set(&du->pci, pci_flags |= PDU_FLAGS_EXPLICIT_CONGESTION);
		LOG_DBG("Queue length is %u, marked PDU with ECN", qlen);
	} else if (qlen >= data->q_max) {
		if (pci_type(&du->pci) != PDU_TYPE_MGMT) {
			du_destroy(du);
			LOG_INFO("DCTCP RMT: PDU dropped, q_max (%u) reached...",
					data->q_max);
			return RMT_PS_ENQ_DROP;
		}
	}

	if (!must_enqueue && rfifo_is_empty(q->queue))
		return RMT_PS_ENQ_SEND;

	rfifo_push_ni(q->queue, du);
	return RMT_PS_ENQ_SCHED;
}

static struct du * dctcp_rmt_dequeue_policy(struct rmt_ps *ps,
					    struct rmt_n1_port *port)
{
	struct dctcp_rmt_queue    *q;
	struct dctcp_rmt_ps_data  *data = ps->priv;
	struct du *ret_pdu;

	if (!ps || !port || !data) {
		LOG_ERR("Wrong input parameters for red_rmt_dequeue_policy");
		return NULL;
	}

	q = port->rmt_ps_queues;
	if (!q) {
		LOG_ERR("Could not find queue for n1_port %u", port->port_id);
		return NULL;
	}

	ret_pdu = rfifo_pop(q->queue);
	LOG_DBG("DCTCP RMT: PDU dequeued...");

	if (!ret_pdu) {
		LOG_ERR("Could not dequeue scheduled pdu");
	}

	return ret_pdu;
}

static int dctcp_rmt_ps_set_policy_set_param(struct ps_base *bps,
		const char *name,
		const char *value)
{
	struct rmt_ps *ps = container_of(bps, struct rmt_ps, base);
	struct dctcp_rmt_ps_data *data = ps->priv;
	int ival;
	int ret;

	(void) ps;

	if (!name) {
		LOG_ERR("Null parameter name");
		return -1;
	}

	if (!value) {
		LOG_ERR("Null parameter value");
		return -1;
	}
	if (strcmp(name, "q_threshold") == 0) {
		ret = kstrtoint(value, 10, &ival);
		if (!ret) {
			data->q_threshold = ival;
			LOG_INFO("Queue marking threshold is %d", ival);
		}
	}

	if (strcmp(name, "q_max") == 0) {
		ret = kstrtoint(value, 10, &ival);
		if (!ret) {
			data->q_max = ival;
			LOG_INFO("Queue max occupancy is %d", ival);
		}
	}

	return 0;
}

static int rmt_ps_load_param(struct rmt_ps *ps, const char *param_name)
{
	struct rmt_config * rmt_cfg;
	struct policy_parm * ps_param;

	rmt_cfg = rmt_config_get(ps->dm);

	if (rmt_cfg) {
		ps_param = policy_param_find(rmt_cfg->policy_set, param_name);
	} else {
		ps_param = NULL;
	}

	if (!ps_param) {
		LOG_WARN("DCTCP RMT: No PS param %s specified", param_name);
	} else {
		dctcp_rmt_ps_set_policy_set_param(&ps->base,
				policy_param_name(ps_param),
				policy_param_value(ps_param));
	}

	return 0;
}
static struct ps_base * rmt_ps_dctcp_create(struct rina_component *component)
{
	struct rmt *rmt = rmt_from_component(component);
	struct rmt_ps *ps = rkzalloc(sizeof(*ps), GFP_KERNEL);
	struct dctcp_rmt_ps_data *data;

	if (!ps) {
		return NULL;
	}

	data = rkzalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		kfree(ps);
		return NULL;
	}
	ps->base.set_policy_set_param = dctcp_rmt_ps_set_policy_set_param;
	ps->dm = rmt;
	ps->priv = data;

	// set defaults
	data->q_max = DEFAULT_Q_MAX;
	data->q_threshold = DEFAULT_Q_THRESHOLD;

	//load configuration if available
	rmt_ps_load_param(ps, "q_threshold");
	rmt_ps_load_param(ps, "q_max");

	ps->rmt_q_create_policy = dctcp_rmt_q_create_policy;
	ps->rmt_q_destroy_policy = dctcp_rmt_q_destroy_policy;
	ps->rmt_enqueue_policy = dctcp_rmt_enqueue_policy;
	ps->rmt_dequeue_policy = dctcp_rmt_dequeue_policy;

	LOG_INFO("DCTCP RMT: PS loaded, q_max = %d, q_threshold = %d",
			data->q_max, data->q_threshold);

	return &ps->base;
}

static void rmt_ps_dctcp_destroy(struct ps_base *bps)
{
	struct rmt_ps *ps = container_of(bps, struct rmt_ps, base);

	if (bps) {
		rkfree(ps);
	}
}

struct ps_factory rmt_factory = {
		.owner     = THIS_MODULE,
		.create  = rmt_ps_dctcp_create,
		.destroy = rmt_ps_dctcp_destroy,
};
