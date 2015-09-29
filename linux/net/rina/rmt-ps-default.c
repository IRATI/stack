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

#define rmap_hash(T, K) hash_min(K, HASH_BITS(T))

/* The key in this struct is used to filter by cep_ids, qos_id, address... */
struct rmt_kqueue {
	struct rfifo *queue;
	unsigned int key;
	unsigned int max_q;
	struct hlist_node hlist;
};

struct rmt_qgroup {
	port_id_t pid;
	struct hlist_node hlist;
	DECLARE_HASHTABLE(queues, RMT_PS_HASHSIZE);
};

struct rmt_queue_set {
	DECLARE_HASHTABLE(qgroups, RMT_PS_HASHSIZE);
};

struct rmt_ps_default_data {
	struct rmt_queue_set *outqs;
};

static struct rmt_kqueue *rmt_kqueue_create(unsigned int key)
{
	struct rmt_kqueue *tmp;

	tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
	if (!tmp)
		return NULL;

	tmp->queue = rfifo_create_ni();
	if (!tmp->queue) {
		rkfree(tmp);
		return NULL;
	}

	tmp->key = key;
	INIT_HLIST_NODE(&tmp->hlist);

	return tmp;
}

static int rmt_kqueue_destroy(struct rmt_kqueue *q)
{
	if (!q) {
		LOG_ERR("No RMT Key-queue to destroy...");
		return -1;
	}

	hash_del(&q->hlist);

	if (q->queue)
		rfifo_destroy(q->queue, (void (*)(void *)) pdu_destroy);

	rkfree(q);

	return 0;
}

static struct rmt_kqueue *rmt_kqueue_find(struct rmt_qgroup *g,
					  unsigned int	     key)
{
	struct rmt_kqueue *entry;
	const struct hlist_head *head;

	ASSERT(g);

	head = &g->queues[rmap_hash(g->queues, key)];
	hlist_for_each_entry(entry, head, hlist)
		if (entry->key == key)
			return entry;

	return NULL;
}

static struct rmt_qgroup *rmt_qgroup_create(port_id_t pid)
{
	struct rmt_qgroup *tmp;

	if (!is_port_id_ok(pid)) {
		LOG_ERR("Could not create qgroup, wrong port id");
		return NULL;
	}

	tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
	if (!tmp)
		return NULL;

	tmp->pid = pid;
	hash_init(tmp->queues);

	return tmp;
}

static int rmt_qgroup_destroy(struct rmt_qgroup *g)
{
	struct rmt_kqueue *entry;
	struct hlist_node *tmp;
	int		   bucket;

	ASSERT(g);
	hash_del(&g->hlist);

	hash_for_each_safe(g->queues, bucket, tmp, entry, hlist)
		if (rmt_kqueue_destroy(entry)) {
			LOG_ERR("Could not destroy entry %pK", entry);
			return -1;
		}

	LOG_DBG("Qgroup for port: %u destroyed...", g->pid);
	rkfree(g);

	return 0;
}

static struct rmt_qgroup *rmt_qgroup_find(struct rmt_queue_set *qs,
					  port_id_t		pid)
{
	struct rmt_qgroup *entry;
	const struct hlist_head *head;

	ASSERT(qs);

	head = &qs->qgroups[rmap_hash(qs->qgroups, pid)];

	hlist_for_each_entry(entry, head, hlist)
		if (entry->pid == pid)
			return entry;

	return NULL;
}

static struct rmt_queue_set *rmt_queue_set_create(void)
{
	struct rmt_queue_set *tmp;

	tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
	if (!tmp)
		return NULL;

	hash_init(tmp->qgroups);

	return tmp;
}

static int rmt_queue_set_destroy(struct rmt_queue_set *qs)
{
	struct rmt_qgroup *entry;
	struct hlist_node *tmp;
	int bucket;

	ASSERT(qs);

	hash_for_each_safe(qs->qgroups, bucket, tmp, entry, hlist)
		if (rmt_qgroup_destroy(entry)) {
			LOG_ERR("Could not destroy entry %pK", entry);
			return -1;
		}

	rkfree(qs);

	return 0;
}

int default_rmt_scheduling_create_policy_tx(struct rmt_ps      *ps,
					    struct rmt_n1_port *n1_port)
{
	struct rmt_qgroup *qgroup;
	struct rmt_kqueue *kqueue;
	struct rmt_ps_default_data *data;

	if (!ps || !n1_port || !ps->priv) {
		LOG_ERR("Wrong input parameters");
		return -1;
	}

	data = ps->priv;

	qgroup = rmt_qgroup_create(n1_port->port_id);
	if (!qgroup) {
		LOG_ERR("Could not create queues group struct for n1_port %u",
			n1_port->port_id);
		return -1;
	}
	hash_add(data->outqs->qgroups, &qgroup->hlist, qgroup->pid);

	kqueue = rmt_kqueue_create(0);
	if (!kqueue) {
		LOG_ERR("Could not create kqueue for n1_port %u and key %u",
			n1_port->port_id, 0);
		rmt_qgroup_destroy(qgroup);
		return -1;
	}

	/* FIXME this is not used in this implementation so far */
	kqueue->max_q	= 256;
	hash_add(qgroup->queues, &kqueue->hlist, 0);

	LOG_DBG("Structures for scheduling policies created...");
	return 0;
}

int default_rmt_scheduling_destroy_policy_tx(struct rmt_ps *ps,
					    struct rmt_n1_port *n1_port)
{
	struct rmt_qgroup *qgroup;
	struct rmt_ps_default_data *data;

	if (!ps || !n1_port || !ps->priv) {
		LOG_ERR("Wrong input parameters");
		return -1;
	}

	data = ps->priv;

	qgroup = rmt_qgroup_find(data->outqs, n1_port->port_id);
	if (!qgroup) {
		LOG_ERR("Could not find queues group for n1_port %u",
			n1_port->port_id);
		return -1;
	}

	hash_del(&qgroup->hlist);
	rmt_qgroup_destroy(qgroup);

	return 0;
}

int default_rmt_enqueue_scheduling_policy_tx(struct rmt_ps *ps,
					    struct rmt_n1_port *n1_port,
					    struct pdu *pdu)
{
	struct rmt_kqueue *q;
	struct rmt_qgroup *qg;
	struct rmt_ps_default_data *data = ps->priv;

	if (!ps || !n1_port || !pdu) {
		LOG_ERR("Wrong input parameters");
		return -1;
	}

	qg = rmt_qgroup_find(data->outqs, n1_port->port_id);
	if (!qg) {
		LOG_ERR("Could not find queue group for n1_port %u",
			n1_port->port_id);
		pdu_destroy(pdu);
		return -1;
	}
	q = rmt_kqueue_find(qg, 0);
	if (!q) {
		LOG_ERR("Could not find queue in the group for n1_port %u",
			n1_port->port_id);
		pdu_destroy(pdu);
		return -1;
	}

	rfifo_push_ni(q->queue, pdu);

	return 0;
}

int default_rmt_requeue_scheduling_policy_tx(struct rmt_ps *ps,
					    struct rmt_n1_port *n1_port,
					    struct pdu *pdu)
{
	struct rmt_kqueue *q;
	struct rmt_qgroup *qg;
	struct rmt_ps_default_data *data = ps->priv;

	if (!ps || !n1_port || !pdu) {
		LOG_ERR("Wrong input parameters");
		return -1;
	}

	qg = rmt_qgroup_find(data->outqs, n1_port->port_id);
	if (!qg) {
		LOG_ERR("Could not find queue group for n1_port %u",
			n1_port->port_id);
		pdu_destroy(pdu);
		return -1;
	}

	q = rmt_kqueue_find(qg, 0);
	if (!qg) {
		LOG_ERR("Could not find queue in the group for n1_port %u",
			n1_port->port_id);
		pdu_destroy(pdu);
		return -1;
	}

	rfifo_head_push_ni(q->queue, pdu);
	return 0;
}

struct pdu *default_rmt_next_scheduled_policy_tx(struct rmt_ps *ps,
						struct rmt_n1_port *n1_port)
{
	struct rmt_kqueue *q;
	struct rmt_qgroup *qg;
	struct rmt_ps_default_data *data = ps->priv;
	struct pdu *ret_pdu;

	if (!ps || !n1_port) {
		LOG_ERR("Wrong input parameters");
		return NULL;
	}

	/* NOTE: The policy is called with the n1_port lock taken */
	qg = rmt_qgroup_find(data->outqs, n1_port->port_id);
	if (!qg) {
		LOG_ERR("Could not find queue group for n1_port %u",
			n1_port->port_id);
		return NULL;
	}

	q = rmt_kqueue_find(qg, 0);
	if (!q) {
		LOG_ERR("Could not find queue in the group for n1_port %u",
			n1_port->port_id);
		return NULL;
	}

	ret_pdu = rfifo_pop(q->queue);
	if (!ret_pdu) {
		LOG_ERR("Could not dequeue scheduled pdu");
		return NULL;
	}

	return ret_pdu;
}

int default_rmt_scheduling_policy_rx(struct rmt_ps *ps,
				    struct rmt_n1_port *n1_port,
				    struct sdu *sdu)
{
/*
	struct rmt_ps_default_data * data = ps->priv;
	atomic_inc(&in_n1_port->n_sdus);

	tasklet_hi_schedule(&instance->ingress.ingress_tasklet);

	LOG_DBG("RMT tasklet scheduled");
*/
	return 0;
}

int rmt_ps_default_set_policy_set_param(struct ps_base *bps,
				       const char *name,
				       const char *value)
{
	struct rmt_ps *ps = container_of(bps, struct rmt_ps, base);

	(void) ps;

	if (!name) {
		LOG_ERR("Null parameter name");
		return -1;
	}

	if (!value) {
		LOG_ERR("Null parameter value");
		return -1;
	}

	LOG_ERR("No such parameter to set");

	return -1;
}

>>>>>>> kernel:rmt: rmt-ps-common removed
static struct ps_base *rmt_ps_default_create(struct rina_component *component)
{
	struct rmt *rmt;
	struct rmt_ps *ps;
	struct rmt_ps_default_data *data;

	rmt = rmt_from_component(component);
	ps = rkmalloc(sizeof(*ps), GFP_KERNEL);
	if (!ps)
		return NULL;

	data = rkmalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		rkfree(ps);
		return NULL;
	}

	/* Allocate policy-set private data. */
	data->outqs = rmt_queue_set_create();
	if (!data->outqs) {
		rkfree(ps);
		rkfree(data);
		return NULL;
	}

	ps->priv = data;

	ps->base.set_policy_set_param = NULL; /* default */
	ps->dm = rmt;

	ps->max_q_policy_tx = NULL;
	ps->max_q_policy_rx = NULL;
	ps->rmt_q_monitor_policy_tx_enq = NULL;
	ps->rmt_q_monitor_policy_tx_deq = NULL;
	ps->rmt_q_monitor_policy_rx = NULL;
	ps->rmt_next_scheduled_policy_tx =
		default_rmt_next_scheduled_policy_tx;
	ps->rmt_enqueue_scheduling_policy_tx =
		default_rmt_enqueue_scheduling_policy_tx;
	ps->rmt_requeue_scheduling_policy_tx =
		default_rmt_requeue_scheduling_policy_tx;
	ps->rmt_scheduling_policy_rx =
		default_rmt_scheduling_policy_rx;
	ps->rmt_scheduling_create_policy_tx  =
		default_rmt_scheduling_create_policy_tx;
	ps->rmt_scheduling_destroy_policy_tx =
		default_rmt_scheduling_destroy_policy_tx;

	return &ps->base;
}

static void rmt_ps_default_destroy(struct ps_base *bps)
{
	struct rmt_ps *ps;
	struct rmt_ps_default_data *data;

	ps = container_of(bps, struct rmt_ps, base);
	data = ps->priv;

	if (bps) {
		if (data) {
			if (data->outqs)
				rmt_queue_set_destroy(data->outqs);
			rkfree(data);
		}
		rkfree(ps);
	}
}

struct ps_factory default_rmt_ps_factory = {
	.owner = THIS_MODULE,
	.create = rmt_ps_default_create,
	.destroy = rmt_ps_default_destroy,
};
EXPORT_SYMBOL(default_rmt_ps_factory );
