/*
 * Default policy set for RMT
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
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

#include <linux/export.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/hashtable.h>

#define RINA_PREFIX "rmt-ps-default"

#include "logs.h"
#include "rds/rmem.h"
#include "rmt-ps.h"
#include "rmt.h"
#include "debug.h"
#include "policies.h"
#include "rmt-ps-default.h"
#include "rds/robjects.h"

#define DEFAULT_Q_MAX 1000
#define rmap_hash(T, K) hash_min(K, HASH_BITS(T))

struct rmt_queue {
	struct rfifo *dt_queue;
	struct rfifo *mgt_queue;
	port_id_t    pid;
};

struct rmt_ps_default_data {
	unsigned int q_max;
	struct robject robj;
};

static ssize_t rmt_ps_attr_show(struct robject *        robj,
                         	struct robj_attribute * attr,
                                char *                  buf)
{
	struct rmt_ps_default_data * data;

	data = container_of(robj, struct rmt_ps_default_data, robj);
	if (!data)
		return 0;

	if (strcmp(robject_attr_name(attr), "q_max") == 0) {
		return sprintf(buf, "%u\n", data->q_max);
	}
	return 0;
}
RINA_SYSFS_OPS(rmt_ps);
RINA_ATTRS(rmt_ps, q_max);
RINA_KTYPE(rmt_ps);

static struct rmt_queue *rmt_queue_create(port_id_t port)
{
	struct rmt_queue *tmp;

	tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
	if (!tmp)
		return NULL;

	tmp->dt_queue = rfifo_create_ni();
	if (!tmp->dt_queue) {
		rkfree(tmp);
		return NULL;
	}

	tmp->mgt_queue = rfifo_create_ni();
	if (!tmp->mgt_queue) {
		rfifo_destroy(tmp->dt_queue, (void (*)(void *)) du_destroy);
		rkfree(tmp);
		return NULL;
	}

	tmp->pid = port;

	return tmp;
}

static int rmt_queue_destroy(struct rmt_queue *q)
{
	if (!q) {
		LOG_ERR("No RMT Key-queue to destroy...");
		return -1;
	}

	if (q->dt_queue)
		rfifo_destroy(q->dt_queue, (void (*)(void *)) du_destroy);

	if (q->mgt_queue)
		rfifo_destroy(q->mgt_queue, (void (*)(void *)) du_destroy);

	rkfree(q);

	return 0;
}

void * default_rmt_q_create_policy(struct rmt_ps      *ps,
			        struct rmt_n1_port *n1_port)
{
	struct rmt_queue *queue;
	struct rmt_ps_default_data *data;

	if (!ps || !n1_port || !ps->priv) {
		LOG_ERR("Wrong input parameters");
		return NULL;
	}

	data = ps->priv;

	queue = rmt_queue_create(n1_port->port_id);
	if (!queue) {
		LOG_ERR("Could not create queue for n1_port %u",
			n1_port->port_id);
		return NULL;
	}

	LOG_DBG("Structures for scheduling policies created...");
	return queue;
}
EXPORT_SYMBOL(default_rmt_q_create_policy);

int default_rmt_q_destroy_policy(struct rmt_ps      *ps,
				 struct rmt_n1_port *n1_port)
{
	struct rmt_queue *queue;
	struct rmt_ps_default_data *data;

	if (!ps || !n1_port || !ps->priv) {
		LOG_ERR("Wrong input parameters");
		return -1;
	}

	data = ps->priv;

	queue = n1_port->rmt_ps_queues;
	if (!queue) {
		LOG_ERR("Could not find queue for n1_port %u",
			n1_port->port_id);
		return -1;
	}

	rmt_queue_destroy(queue);

	return 0;
}
EXPORT_SYMBOL(default_rmt_q_destroy_policy);

int default_rmt_enqueue_policy(struct rmt_ps *      ps,
			       struct rmt_n1_port * n1_port,
			       struct du *          du,
			       bool 		    must_enqueue)
{
	struct rmt_queue *q;
	struct rmt_ps_default_data *data = ps->priv;
	pdu_type_t pdu_type;

	if (!ps || !n1_port || !du) {
		LOG_ERR("Wrong input parameters");
		return RMT_PS_ENQ_ERR;
	}

	q = n1_port->rmt_ps_queues;
	if (!q) {
		LOG_ERR("Could not find queue for n1_port %u",
			n1_port->port_id);
		du_destroy(du);
		return RMT_PS_ENQ_ERR;
	}

	pdu_type = pci_type(&du->pci);
	if (pdu_type == PDU_TYPE_MGMT) {
		if (!must_enqueue && rfifo_is_empty(q->mgt_queue)){
			return RMT_PS_ENQ_SEND;
		}

		rfifo_push_ni(q->mgt_queue, du);
		return RMT_PS_ENQ_SCHED;
	}

	if (!must_enqueue && rfifo_is_empty(q->dt_queue))
		return RMT_PS_ENQ_SEND;

	if (rfifo_length(q->dt_queue) >= data->q_max) {
		du_destroy(du);
		return RMT_PS_ENQ_DROP;
	}

	rfifo_push_ni(q->dt_queue, du);
	return RMT_PS_ENQ_SCHED;
}
EXPORT_SYMBOL(default_rmt_enqueue_policy);

struct du *default_rmt_dequeue_policy(struct rmt_ps	  *ps,
				      struct rmt_n1_port *n1_port)
{
	struct rmt_queue *q;
	struct du *ret_du;

	if (!ps || !n1_port) {
		LOG_ERR("Wrong input parameters");
		return NULL;
	}

	q = n1_port->rmt_ps_queues;
	if (!q) {
		LOG_ERR("Could not find queue for n1_port %u",
			n1_port->port_id);
		return NULL;
	}

	if (!rfifo_is_empty(q->mgt_queue))
		ret_du = rfifo_pop(q->mgt_queue);
	else
		ret_du = rfifo_pop(q->dt_queue);

	if (!ret_du) {
		LOG_ERR("Could not dequeue scheduled pdu");
		return NULL;
	}

	return ret_du;
}
EXPORT_SYMBOL(default_rmt_dequeue_policy);

static int rmt_ps_default_set_policy_set_param(struct ps_base *bps,
					       const char *name,
					       const char *value)
{
	struct rmt_ps *ps = container_of(bps, struct rmt_ps, base);
	struct rmt_ps_default_data *data = ps->priv;
	int bool_value;
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

	if (strcmp(name, "q_max") == 0) {
		ret = kstrtoint(value, 10, &bool_value);
		if (!ret)
			data->q_max = bool_value;
	}

	return 0;
}

struct ps_base *rmt_ps_default_create(struct rina_component *component)
{
	struct rmt *rmt;
	struct rmt_ps *ps;
	struct rmt_ps_default_data *data;
	const struct rmt_config *rmt_cfg;
	struct policy_parm *parm = NULL;

	rmt = rmt_from_component(component);
	if(!rmt)
		return NULL;

	ps = rkmalloc(sizeof(*ps), GFP_KERNEL);
	if (!ps)
		return NULL;

	data = rkzalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		rkfree(ps);
		return NULL;
	}
	if (robject_init_and_add(&data->robj, &rmt_ps_rtype,
				rmt_robject(rmt), "ps")) {
		rkfree(data);
		rkfree(ps);
		return NULL;
	}

	ps->base.set_policy_set_param = rmt_ps_default_set_policy_set_param;
	ps->dm = rmt;
	ps->priv = data;

	rmt_cfg = rmt_config_get(rmt);
	if (rmt_cfg) {
		/* RMT config is available at assign-to-dif time, but
		 * not available at set-policy-set time. */
		parm = policy_param_find(rmt_cfg->policy_set, "q_max");
	}

	if (!parm) {
		LOG_WARN("No PS param q_max, setting default");
		data->q_max = DEFAULT_Q_MAX;
	} else {
		rmt_ps_default_set_policy_set_param(&ps->base,
						    policy_param_name(parm),
						    policy_param_value(parm));
        }

	ps->rmt_dequeue_policy = default_rmt_dequeue_policy;
	ps->rmt_enqueue_policy = default_rmt_enqueue_policy;
	ps->rmt_q_create_policy = default_rmt_q_create_policy;
	ps->rmt_q_destroy_policy = default_rmt_q_destroy_policy;

	return &ps->base;
}
EXPORT_SYMBOL(rmt_ps_default_create);

void rmt_ps_default_destroy(struct ps_base *bps)
{
	struct rmt_ps *ps;
	struct rmt_ps_default_data *data;

	ps = container_of(bps, struct rmt_ps, base);
	data = ps->priv;

	if (bps) {
		if (data) {
			robject_del(&data->robj);
			rkfree(data);
                }
		rkfree(ps);
	}
}
EXPORT_SYMBOL(rmt_ps_default_destroy);
