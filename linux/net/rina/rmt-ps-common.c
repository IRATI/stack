/*
 * Common parts of the common policy set for RMT.
 * This is envisioned for reurilization in other custom policy sets
 *
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
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

#define RINA_PREFIX "rmt-ps-common"

#include "logs.h"
#include "rds/rmem.h"
#include "rmt-ps.h"
#include "rmt-ps-common.h"

int common_rmt_scheduling_create_policy_tx(struct rmt_ps *ps,
					   struct rmt_n1_port *n1_port)
{
	struct rmt_qgroup *qgroup;
	struct rmt_kqueue *kqueue;
	struct rmt_ps_common_data *data;

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
	kqueue->min_qth = 0;
	kqueue->max_qth = 0;
	hash_add(qgroup->queues, &kqueue->hlist, 0);

	LOG_DBG("Structures for scheduling policies created...");
	return 0;
}
EXPORT_SYMBOL(common_rmt_scheduling_create_policy_tx);

int common_rmt_scheduling_destroy_policy_tx(struct rmt_ps *ps,
					    struct rmt_n1_port *n1_port)
{
	struct rmt_qgroup *qgroup;
	struct rmt_ps_common_data *data;

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
EXPORT_SYMBOL(common_rmt_scheduling_destroy_policy_tx);

int common_rmt_enqueue_scheduling_policy_tx(struct rmt_ps *ps,
					    struct rmt_n1_port *n1_port,
					    struct pdu *pdu)
{
	struct rmt_kqueue *q;
	struct rmt_qgroup *qg;
	struct rmt_ps_common_data *data = ps->priv;

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
EXPORT_SYMBOL(common_rmt_enqueue_scheduling_policy_tx);

int common_rmt_requeue_scheduling_policy_tx(struct rmt_ps *ps,
					    struct rmt_n1_port *n1_port,
					    struct pdu *pdu)
{
	struct rmt_kqueue *q;
	struct rmt_qgroup *qg;
	struct rmt_ps_common_data *data = ps->priv;

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
EXPORT_SYMBOL(common_rmt_requeue_scheduling_policy_tx);

struct pdu *common_rmt_next_scheduled_policy_tx(struct rmt_ps *ps,
						struct rmt_n1_port *n1_port)
{
	struct rmt_kqueue *q;
	struct rmt_qgroup *qg;
	struct rmt_ps_common_data *data = ps->priv;
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
EXPORT_SYMBOL(common_rmt_next_scheduled_policy_tx);

int common_rmt_scheduling_policy_rx(struct rmt_ps *ps,
				    struct rmt_n1_port *n1_port,
				    struct sdu *sdu)
{
/*
	struct rmt_ps_common_data * data = ps->priv;
	atomic_inc(&in_n1_port->n_sdus);

	tasklet_hi_schedule(&instance->ingress.ingress_tasklet);

	LOG_DBG("RMT tasklet scheduled");
*/
	return 0;
}
EXPORT_SYMBOL(common_rmt_scheduling_policy_rx);

int rmt_ps_common_set_policy_set_param(struct ps_base *bps,
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
EXPORT_SYMBOL(rmt_ps_common_set_policy_set_param);
