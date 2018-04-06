/*
 * QTA MUX plugin
 *
 *    Vincenzo Maffione <v.maffione@nextworks.it>
 *    Miquel Tarzan <miquel.tarzan@i2cat.net>
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
#include <linux/module.h>
#include <linux/string.h>
#include <linux/random.h>

#define RINA_PREFIX "qta-mux-plugin"

#include "logs.h"
#include "rds/rmem.h"
#include "rmt-ps.h"
#include "policies.h"
#include "debug.h"
#include "du.h"

#define RINA_QTA_MUX_PS_NAME "qta-mux-ps"
#define NORM_PROB                100

#define TIMER_T  100

struct pdu_entry {
	struct list_head list;
	struct du *      pdu;
	uint_t           pdu_length;
	uint_t           urgency_level;
	uint_t           cherish_threshold;
};

struct urgency_queue {
	struct list_head list;
	struct list_head queued_pdus;
	uint_t           urgency_level;
	uint_t           length;
	struct robject   robj;
};

struct cu_mux {
	struct list_head   urgency_queues;
	struct list_head   mgmt_queue;
};

struct stream_queue {
	struct list_head list;
	struct list_head queued_pdus;
	qos_id_t         qos_id;
	uint_t		 urgency_level;
	uint_t           cherish_threshold;
	uint_t           max_bytes;
	uint_t           current_bytes;
	uint_t           max_rate;
};

struct stream_queue_conf {
	struct list_head list;
	qos_id_t qos_id;
	uint_t   urgency_level;
	uint_t   cherish_level;
	uint_t   max_burst_size;
	uint_t   max_rate;
};

struct cu_mux_conf {
	uint_t   urgency_levels;
	uint_t   cherish_levels;
	uint_t * cherish_thresholds;
};

struct qta_mux_conf {
	struct cu_mux_conf cu_mux_conf;
	struct list_head stream_queues_conf;
};

struct qta_mux {
	struct list_head list;
	struct list_head stream_queues;
	struct cu_mux    cu_mux;
	port_id_t        port_id;
	uint_t           occupation;
	struct robject   robj;
};

struct qta_mux_set {
	struct list_head      qta_muxes;
	struct qta_mux_conf * qta_mux_conf;
};

/** OLD TYPES **/

struct q_entry {
	struct list_head next;
	struct du * data;
	struct q_qos * qqos;
	int w;
};

struct qta_queue {
	struct list_head list;
	struct list_head list_q_entry;
	struct list_head list_q_qos;
	uint_t key;
	struct rfifo * queue;
};

struct config_pshaper {
	uint_t rate;
	uint_t prb_max_length;
	uint_t abs_max_length;
	uint_t max_backlog;
};

struct config_q_qos {
	uint_t           abs_th;
	uint_t           th;
	uint_t           drop_prob;
	qos_id_t         qos_id;
	uint_t           key;
	uint_t           rate;
	uint_t           prb_max_length;
	uint_t           abs_max_length;
	uint_t           max_backlog;
	struct list_head list;
};

struct qta_config {
	struct list_head list_queues;
};

struct pshaper {
	struct config_pshaper config;
	uint_t length;
	uint_t backlog;
};

struct q_qos {
	uint_t           abs_th;
	uint_t           th;
	uint_t           drop_prob;
	qos_id_t         qos_id;
	struct list_head list;
	uint_t           dropped;
	uint_t           handled;
	uint_t           key;
	struct qta_queue * qta_q;
	struct pshaper   ps;
	struct robject robj;
};

struct qta_queue_set {
	struct list_head queues;
	struct list_head list;
	port_id_t        port_id;
	unsigned int     occupation;
	struct robject robj;
};

struct qta_mux_data {
	struct list_head    list_queues;
	struct qta_config * config;
};

/** End OLD types **/

static ssize_t qta_mux_attr_show(struct robject * robj,
				 struct robj_attribute * attr,
				 char * buf)
{
	struct qta_mux * qta_mux;

	qta_mux = container_of(robj, struct qta_mux, robj);
	if (!qta_mux)
		return 0;

	if (strcmp(robject_attr_name(attr), "port_id") == 0) {
		return sprintf(buf, "%u\n", qta_mux->port_id);
	}

	if (strcmp(robject_attr_name(attr), "occupation") == 0) {
		return sprintf(buf, "%u\n", qta_mux->occupation);
	}

	return 0;
}

static ssize_t urgency_queue_attr_show(struct robject * robj,
				       struct robj_attribute * attr,
				       char * buf)
{
	struct urgency_queue * q;

	q = container_of(robj, struct urgency_queue, robj);
	if (!q)
		return 0;

	if (strcmp(robject_attr_name(attr), "urgency") == 0) {
		return sprintf(buf, "%u\n", q->urgency_level);
	}
	if (strcmp(robject_attr_name(attr), "queued_pdus") == 0) {
		return sprintf(buf, "%u\n", q->length);
	}

	return 0;
}

RINA_SYSFS_OPS(qta_mux);
RINA_ATTRS(qta_mux, port_id, occupation);
RINA_KTYPE(qta_mux);
RINA_SYSFS_OPS(urgency_queue);
RINA_ATTRS(urgency_queue, urgency, queued_pdus);
RINA_KTYPE(urgency_queue);

static struct stream_queue_conf * stream_queue_conf_create(void)
{
	struct stream_queue_conf * tmp;

	tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
	if (!tmp) {
		LOG_ERR("Problems creating stream queue configuration");
		return NULL;
	}

	INIT_LIST_HEAD(&tmp->list);
	tmp->qos_id = 0;
	tmp->urgency_level = 0;
	tmp->cherish_level = 0;
	tmp->max_burst_size = 0;
	tmp->max_rate = 0;

	return tmp;
}

static void stream_queue_conf_destroy(struct stream_queue_conf * conf)
{
	if (!conf)
		return;

	list_del(&conf->list);
	rkfree(conf);
}

static struct qta_mux_conf * qta_mux_conf_create(void)
{
	struct qta_mux_conf * tmp;

	tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
	if (!tmp) {
		LOG_ERR("Problems creating QTA MUX configuration");
		return NULL;
	}

	INIT_LIST_HEAD(&tmp->stream_queues_conf);
	tmp->cu_mux_conf.cherish_levels = 0;
	tmp->cu_mux_conf.urgency_levels = 0;
	tmp->cu_mux_conf.cherish_thresholds = NULL;

	return tmp;
}

static void qta_mux_conf_destroy(struct qta_mux_conf * qta_mux_conf)
{
	struct stream_queue_conf *pos, *next;

	if (!qta_mux_conf)
		return;

	if (qta_mux_conf->cu_mux_conf.cherish_thresholds)
		rkfree(qta_mux_conf->cu_mux_conf.cherish_thresholds);

	list_for_each_entry_safe(pos, next,
			&qta_mux_conf->stream_queues_conf, list) {
		stream_queue_conf_destroy(pos);
	}

	rkfree(qta_mux_conf);
}

static struct qta_mux_set * qta_mux_set_create(void)
{
	struct qta_mux_set * tmp;

	tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
	if (!tmp) {
		LOG_ERR("Problems creating QTA MUX set");
		return NULL;
	}

	INIT_LIST_HEAD(&tmp->qta_muxes);
	tmp->qta_mux_conf = NULL;

	return tmp;
}

static void pdu_entry_destroy(struct pdu_entry * pdu_entry, bool destroy)
{
	if (!pdu_entry)
		return;

	list_del(&pdu_entry->list);

	if (destroy) {
		du_destroy(pdu_entry->pdu);
	}

	rkfree(pdu_entry);

	return;
}

static void urgency_queue_destroy(struct urgency_queue * uq)
{
	struct pdu_entry *pos, *next;

	if (!uq)
		return;

	list_del(&uq->list);

	robject_del(&uq->robj);

	list_for_each_entry_safe(pos, next,
			&uq->queued_pdus, list) {
		pdu_entry_destroy(pos, true);
	}

	rkfree(uq);
}

static void stream_queue_destroy(struct stream_queue * sq)
{
	struct pdu_entry *pos, *next;

	if (!sq)
		return;

	list_del(&sq->list);

	list_for_each_entry_safe(pos, next,
			&sq->queued_pdus, list) {
		pdu_entry_destroy(pos, true);
	}

	rkfree(sq);
}

static void qta_mux_destroy(struct qta_mux * qta_mux)
{
	struct urgency_queue *pos, *next;
	struct stream_queue *pos2, *next2;
	struct pdu_entry *pos3, *next3;

	if (!qta_mux)
		return;

	list_del(&qta_mux->list);

	list_for_each_entry_safe(pos, next,
			&qta_mux->cu_mux.urgency_queues, list) {
		urgency_queue_destroy(pos);
	}

	list_for_each_entry_safe(pos2, next2,
			&qta_mux->stream_queues, list) {
		stream_queue_destroy(pos2);
	}

	list_for_each_entry_safe(pos3, next3,
			&qta_mux->cu_mux.mgmt_queue, list) {
		pdu_entry_destroy(pos3, true);
	}

	robject_del(&qta_mux->robj);

	rkfree(qta_mux);
}

static struct qta_mux * qta_mux_create(port_id_t port_id,
		   	   	       struct robject *parent)
{
	struct qta_mux * tmp;

	tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
	if (!tmp) {
		LOG_ERR("Problems creating QTA MUX");
		return NULL;
	}

	INIT_LIST_HEAD(&tmp->list);
	INIT_LIST_HEAD(&tmp->stream_queues);
	INIT_LIST_HEAD(&tmp->cu_mux.urgency_queues);
	INIT_LIST_HEAD(&tmp->cu_mux.mgmt_queue);
	tmp->port_id = port_id;
	tmp->occupation = 0;

	if (robject_init_and_add(&tmp->robj, &qta_mux_rtype, parent, "qta_mux")) {
		LOG_ERR("Failed to create QTA MUX sysfs entry");
		qta_mux_destroy(tmp);
		return NULL;
	}

	return tmp;
}

static struct stream_queue * stream_queue_create(struct cu_mux_conf * cu_mux_conf,
						 struct stream_queue_conf * sq_conf)
{
	struct stream_queue * tmp;

	tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
	if (!tmp) {
		LOG_ERR("Problems creating stream queue");
		return NULL;
	}

	INIT_LIST_HEAD(&tmp->list);
	INIT_LIST_HEAD(&tmp->queued_pdus);
	tmp->max_rate = sq_conf->max_rate;
	tmp->qos_id = sq_conf->qos_id;
	tmp->urgency_level = sq_conf->urgency_level;
	tmp->cherish_threshold =
			cu_mux_conf->cherish_thresholds[sq_conf->cherish_level -1];
	tmp->current_bytes = 0;
	/* TODO get max queue size from max rate and max burst size */
	tmp->max_bytes = 0;

	return tmp;
}

static struct urgency_queue * urgency_queue_create(uint_t urgency_level,
						   struct robject *parent)
{
	struct urgency_queue * tmp;

	tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
	if (!tmp) {
		LOG_ERR("Problems creating QTA MUX");
		return NULL;
	}

	INIT_LIST_HEAD(&tmp->list);
	INIT_LIST_HEAD(&tmp->queued_pdus);
	tmp->length = 0;
	tmp->urgency_level = urgency_level;

	if (robject_init_and_add(&tmp->robj, &urgency_queue_rtype, parent, "urgency_queue")) {
		LOG_ERR("Failed to create Urgency queue sysfs entry");
		urgency_queue_destroy(tmp);
		return NULL;
	}

	return tmp;
}

static struct pdu_entry * pdu_entry_create(struct du* du, uint_t urgency_level,
			   	    	   uint_t cherish_th)
{
	struct pdu_entry * tmp;

	tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
	if (!tmp) {
		LOG_ERR("Problems creating PDU entry");
		return NULL;
	}

	INIT_LIST_HEAD(&tmp->list);
	tmp->pdu = du;
	tmp->urgency_level = urgency_level;
	tmp->cherish_threshold = cherish_th;

	return tmp;
}

static void qta_mux_set_destroy(struct qta_mux_set * qta_mux_set)
{
	struct qta_mux *pos, *next;

	if (!qta_mux_set)
		return;

	if (qta_mux_set->qta_mux_conf) {
		qta_mux_conf_destroy(qta_mux_set->qta_mux_conf);
		qta_mux_set->qta_mux_conf = NULL;
	}

	list_for_each_entry_safe(pos, next,
			&qta_mux_set->qta_muxes, list) {
		qta_mux_destroy(pos);
	}

	rkfree(qta_mux_set);
}

static struct q_qos * q_qos_find(struct qta_queue * q, qos_id_t qos_id)
{
	struct q_qos * pos;

	list_for_each_entry(pos, &q->list_q_qos, list) {
		if (pos->qos_id == qos_id) {
			return pos;
		}
	}

	return NULL;
}

static struct stream_queue * stream_queue_find(struct qta_mux * qta_mux,
					       qos_id_t qos_id)
{
	struct stream_queue * pos;

	list_for_each_entry(pos, &qta_mux->stream_queues, list) {
		if (pos->qos_id == qos_id)
			return pos;
	}

	return NULL;
}

struct qta_queue_set * queue_set_find(struct qta_mux_data * q,
				      port_id_t port_id)
{
	struct qta_queue_set * entry;

	list_for_each_entry(entry, &q->list_queues, queues) {
		if (entry->port_id == port_id) {
			return entry;
		}
	}

	return NULL;
}

static struct du * dequeue_pdu(struct list_head * list)
{
	struct pdu_entry * pdu_entry = NULL;
	struct du * pdu;

	pdu_entry = list_first_entry(list, struct pdu_entry, list);
	if (!pdu_entry) {
		return NULL;
	}

	pdu = pdu_entry->pdu;

	pdu_entry_destroy(pdu_entry, false);

	return pdu;
}

struct du * qta_rmt_dequeue_policy(struct rmt_ps	  *ps,
				   struct rmt_n1_port *n1_port)
{
	struct du *            ret_pdu;
	struct qta_mux *       qta_mux;
	struct urgency_queue * pos;

	if (!ps || !n1_port || !n1_port->rmt_ps_queues) {
		LOG_ERR("Wrong input parameters for "
				"rmt_next_scheduled_policy_tx");
		return NULL;
	}

	qta_mux = n1_port->rmt_ps_queues;
	if (!qta_mux)
		return NULL;

	/* Layer management PDUs always have top priority */
	if (!list_empty(&qta_mux->cu_mux.mgmt_queue))
		return dequeue_pdu(&qta_mux->cu_mux.mgmt_queue);

	/* Go through the urgency queues in urgency level: the queues */
	/* with a higher urgency level are checked first */
	list_for_each_entry(pos, &qta_mux->cu_mux.urgency_queues, list) {
		/* If this urgency level contains one or more QoS ids */
		if (!list_empty(&pos->queued_pdus)) {
			ret_pdu = dequeue_pdu(&pos->queued_pdus);
			pos->length--;

			return ret_pdu;
		}
	}

	/* Nothing to dequeue */
	return NULL;
}

int qta_rmt_enqueue_policy(struct rmt_ps	  *ps,
			   struct rmt_n1_port *n1_port,
			   struct du	  *du)
{
	struct qta_mux * qta_mux;
	struct stream_queue *  stream_queue;
	qos_id_t               qos_id;
	pdu_type_t pdu_type;
	struct pdu_entry * pdu_entry;
	struct q_entry * entry;
	uint_t mlength;
	uint_t mbacklog;
	uint_t w;

	if (!ps || !n1_port || !du) {
		LOG_ERR("Wrong input parameters for "
				"rmt_enqueu_scheduling_policy_tx");
		return RMT_PS_ENQ_ERR;
	}

	qta_mux = n1_port->rmt_ps_queues;

	/*
	 * If layer management PDU enqueue in dedicated queue, bypass P/S
	 * TODO: add dedicated P/S for layer management traffic,
	 * to avoid (D)DoS
	 */
	pdu_type = pci_type(&du->pci);
	if (pdu_type == PDU_TYPE_MGMT) {
		pdu_entry = pdu_entry_create(du, 0, 0);
		if (!pdu_entry) {
			LOG_ERR("Problems allocating memory for PDU entry");
			du_destroy(du);
			return RMT_PS_ENQ_ERR;
		}

		list_add_tail(&pdu_entry->list, &qta_mux->cu_mux.mgmt_queue);
		return RMT_PS_ENQ_SCHED;
	}

	/* Retrieve stream_queue for the PDU's qos-id */
	qos_id = pci_qos_id(&du->pci);
	stream_queue = stream_queue_find(qta_mux, qos_id);
	if (!stream_queue) {
		LOG_ERR("No queue for QoS id %u, dropping PDU", qos_id);
		du_destroy(du);
		return RMT_PS_ENQ_ERR;
	}

	LOG_DBG("Enqueueing for QoS id %u", qos_id);

	/* Go through policer-shaper (P/S) */
	mlength  = q->ps.config.abs_max_length;
	mbacklog = q->ps.config.max_backlog * q->ps.config.rate;
	q->ps.length++;

	/* Drop PDU if too many packets queued at P/S */
	if (mlength && q->ps.length > mlength) {
		LOG_WARN("Length exceeded for QoS id %u, dropping PDU", qos_id);
		du_destroy(du);
		q->ps.length--;
		q->dropped++;
		return RMT_PS_ENQ_DROP;
	}

	/* Drop PDU if maximum rate for QoS id exceeded */
	w = (int) du_len(du);
	q->ps.backlog += w;
	if (mbacklog && q->ps.backlog > mbacklog) {
		LOG_WARN("Backlog exceeded for QoS id %u, dropping PDU", qos_id);
		q->dropped++;
		q->ps.length--;
		q->ps.backlog -= w;
		du_destroy(du);
		return RMT_PS_ENQ_DROP;
	}

	/* P/S processing ended, entering C/U mux */
	q->handled++;

	/* Drop PDU if C/U mux queue length > absolute threshold */
	if (q->abs_th < (qset->occupation + 1)) {
		LOG_WARN("Dropped PDU: abs_th exceeded %u", qset->occupation);
		q->dropped++;
		q->ps.length--;
		q->ps.backlog -= w;
		du_destroy(du);
		return RMT_PS_ENQ_DROP;
	}

	/* Drop PDU probabilistically if C/U mux queue length > threshold */
	get_random_bytes(&i, sizeof(i));
	i = i % NORM_PROB;
	if ((q->th < (qset->occupation + 1)) && (q->drop_prob > i)) {
		LOG_WARN("Dropped PDU: th exceeded %u", qset->occupation);
		q->dropped++;
		q->ps.length--;
		q->ps.backlog -= w;
		du_destroy(du);
		return RMT_PS_ENQ_DROP;
	}

	/* Enqueue PDU in C/U mux queue */
	entry = rkzalloc(sizeof(*entry), GFP_ATOMIC);
	if (!entry) {
		LOG_ERR("Failed to enqueue");
		q->dropped++;
		q->ps.length--;
		q->ps.backlog -= w;
		du_destroy(du);
		return RMT_PS_ENQ_ERR;
	}
	entry->data = du;
	entry->qqos = q;
	entry->w    = w;
	INIT_LIST_HEAD(&entry->next);
	list_add(&entry->next, &q->qta_q->list_q_entry);

	qset->occupation++;

	LOG_DBG("PDU enqueued");
	return RMT_PS_ENQ_SCHED;
}

void * qta_rmt_q_create_policy(struct rmt_ps      *ps,
			       struct rmt_n1_port *n1_port)
{
	struct qta_mux_set * qta_mux_set = ps->priv;
	struct qta_mux * qta_mux;
	struct qta_mux_conf * config;
	struct stream_queue_conf * pos;
	struct stream_queue * stream_queue;
	struct urgency_queue * urgency_queue;
	int i;

	if (!ps || !n1_port || !qta_mux_set) {
		LOG_ERR("Wrong input parameters for "
				"rmt_scheduling_create_policy_common");
		return NULL;
	}

	/* Create the qta_mux and add it to the n1_port */
	qta_mux = qta_mux_create(n1_port->port_id, &n1_port->robj);
	if (!qta_mux) {
		LOG_ERR("No scheduling policy created for port-id %d",
				n1_port->port_id);
		return NULL;
	}
	list_add(&qta_mux->list, &qta_mux_set->qta_muxes);
	config = qta_mux_set->qta_mux_conf;

	/* Create one stream queue per QoS level */
	list_for_each_entry(pos, &config->stream_queues_conf, list) {
		stream_queue = stream_queue_create(&config->cu_mux_conf, pos);
		if (!stream_queue) {
			LOG_ERR("Problems creating stream queue");
			qta_mux_destroy(qta_mux);
			return NULL;
		}

		list_add_tail(&stream_queue->list, &qta_mux->stream_queues);
		LOG_INFO("Added stream queue for QoS id %u", pos->qos_id);
	}

	/* Create one urgency_queue per urgency level, add it to the qta_mux */
	for (i=0; i<config->cu_mux_conf.urgency_levels; i++) {
		urgency_queue = urgency_queue_create(i+1, &qta_mux->robj);
		if (!urgency_queue) {
			LOG_ERR("Problems creating urgency queue");
			qta_mux_destroy(qta_mux);
			return NULL;
		}

		list_add_tail(&urgency_queue->list, &qta_mux->cu_mux.urgency_queues);
		LOG_INFO("Added urgency queue for urgency level %d", i);
	}

	return qta_mux;
}

int qta_rmt_q_destroy_policy(struct rmt_ps      *ps,
			     struct rmt_n1_port *n1_port)
{
	struct qta_mux_set *  data = ps->priv;
	struct qta_mux * tmp = n1_port->rmt_ps_queues;

	if (!tmp || !data)
		return -1;

	qta_mux_destroy(tmp);
	n1_port->rmt_ps_queues = NULL;

	return 0;
}

static int parse_int_value(const char* string, int c,
			   int * result, int * new_offset)
{
	int ret, offset;
	char *aux, *buf;

	aux = strchr(string, c);
	if (!aux) {
		LOG_ERR("Could not find character %d in string %s", c, string);
		return -1;
	}

	offset = aux - string;
	buf = rkzalloc((offset + 1), GFP_ATOMIC);
	if (buf) {
		memcpy(buf, string, offset);
		buf[offset] = '\0';
	}

	ret = kstrtoint(buf, 10, result);
	rkfree(buf);
	if (ret != 0) {
		LOG_ERR("Error parsing int value: %d", ret);
		return -1;
	}

	*new_offset = offset + 1;

	return 0;
}

static int qta_mux_ps_set_policy_set_param_priv(struct qta_mux_set * data,
						const char *     name,
						const char *     value)
{
	int offset, delta_offset;
	qos_id_t qos_id;
	uint_t urgency_level, cherish_level, max_burst_size, max_rate;
	char *dot, *mux;
	int i, c;
	uint_t * cherish_levels;
	struct stream_queue_conf * stream_queue_conf;

	if (!name) {
		LOG_ERR("Null parameter name");
		return -1;
	}

	if (!value) {
		LOG_ERR("Null parameter value");
		return -1;
	}

	dot = strchr(name, '.');
	mux = strchr(name, 'x');

	/*
	 * Parse entry of type name = qosid.<qos_id>
	 * value = <urgengcy_level>:<cherish_level>:<max_burst_size>:<rate>
	 */
	if (dot) {
		/* Parse qos-id from the parameter name */
		if (parse_int_value(name, '.', (int *) &qos_id, &offset))
			return -1;

		/* Parse urgency level from the parameter value */
		offset = 0;
		if (parse_int_value(value, ':', (int *) &urgency_level,
		    &delta_offset))
			return -1;

		/* Parse cherish level from the paramater value */
		offset += delta_offset;
		if (parse_int_value(value + offset, ':',
				    (int *) &cherish_level, &delta_offset))
			return -1;

		/* Parse max burst size from the paramater value */
		offset += delta_offset;
		if (parse_int_value(value + offset, ':',
				    (int *) &max_burst_size, &delta_offset))
			return -1;

		/* Parse max rate from the paramater value */
		offset += delta_offset;
		if (parse_int_value(value + offset, '\0', (int *) &max_rate,
				    &delta_offset))
			return -1;

		LOG_INFO("Values for qos_id %u: %u, %u, %u, %u",
			 qos_id, urgency_level, cherish_level,
			 max_burst_size, max_rate);

		stream_queue_conf = stream_queue_conf_create();
		if (!stream_queue_conf) {
			LOG_ERR("Could not allocate memory");
			return -1;
		}

		stream_queue_conf->qos_id = qos_id;
		stream_queue_conf->urgency_level = urgency_level;
		stream_queue_conf->cherish_level = cherish_level;
		stream_queue_conf->max_burst_size = max_burst_size;
		stream_queue_conf->max_rate = max_rate;
		list_add_tail(&stream_queue_conf->list,
			      &data->qta_mux_conf->stream_queues_conf);

		return 0;
	}

	/*
	 * Parse entry of type name = cumux
	 * value = <urgency_levels>:<cherish_levels>:<cherish_thres1>:...
	 *
	 */
	if (mux) {
		/* Parse urgency levels from the parameter value */
		offset = 0;
		if (parse_int_value(value, ':', (int *) &urgency_level,
				    &delta_offset))
			return -1;

		/* Parse cherish levels from the parameter value */
		offset += delta_offset;
		if (parse_int_value(value + offset, ':',
				    (int *) &cherish_level, &delta_offset))
			return -1;

		cherish_levels = rkzalloc(cherish_level * sizeof(uint_t),
					  GFP_ATOMIC);
		if (!cherish_levels) {
			LOG_ERR("Problems allocating memory, exiting");
			return -1;
		}

		LOG_INFO("Urgency levels: %u, cherish levels: %u",
			 urgency_level, cherish_level);

		for (i=0; i< cherish_level; i++) {
			if (i < cherish_level -1) {
				c = ':';
			} else {
				c = '\0';
			}

			offset += delta_offset;
			if (parse_int_value(value + offset, c,
					    (int *) &cherish_levels[i],
					    &delta_offset)) {
				rkfree(cherish_levels);
				return -1;
			}

			LOG_INFO("Parsed cherish_level[%d]: %u", i,
				 cherish_levels[i]);

		}

		data->qta_mux_conf->cu_mux_conf.cherish_levels = cherish_level;
		data->qta_mux_conf->cu_mux_conf.urgency_levels = urgency_level;
		data->qta_mux_conf->cu_mux_conf.cherish_thresholds =
				cherish_levels;

		return 0;
	}

	LOG_ERR("Unknown name for this configuration entry");

	return -1;
}

static int rmt_config_apply(struct policy_parm * param, void * data)
{
	return qta_mux_ps_set_policy_set_param_priv(data,
			policy_param_name(param),
			policy_param_value(param));
}

static int qta_ps_set_policy_set_param(struct ps_base * bps,
				       const char *     name,
				       const char *     value)
{
	struct rmt_ps *ps = container_of(bps, struct rmt_ps, base);
	struct qta_mux_set * data = ps->priv;

	return qta_mux_ps_set_policy_set_param_priv(data, name, value);
}

static struct ps_base *
rmt_ps_qta_create(struct rina_component * component)
{
	struct rmt * rmt = rmt_from_component(component);
	struct rmt_ps * ps = rkzalloc(sizeof(*ps), GFP_ATOMIC);
	struct qta_mux_set * qta_mux_set;
	struct rmt_config * rmt_cfg;

	if (!ps)
		return NULL;

	qta_mux_set = qta_mux_set_create();
	if (!qta_mux_set) {
		rkfree(ps);
		return NULL;
	}

	qta_mux_set->qta_mux_conf = qta_mux_conf_create();
	if (!qta_mux_set->qta_mux_conf) {
		rkfree(ps);
		qta_mux_set_destroy(qta_mux_set);
		return NULL;
	}

	ps->base.set_policy_set_param = qta_ps_set_policy_set_param;
	ps->dm = rmt;
	ps->priv = qta_mux_set;

	rmt_cfg = rmt_config_get(rmt);
	if (rmt_cfg) {
		policy_for_each(rmt_cfg->policy_set, qta_mux_set,
				rmt_config_apply);
	} else {
		/* TODO provide a suitable default for all the parameters. */
		LOG_WARN("Missing defaults");
	}

	ps->rmt_dequeue_policy = qta_rmt_dequeue_policy;
	ps->rmt_enqueue_policy = qta_rmt_enqueue_policy;
	ps->rmt_q_create_policy = qta_rmt_q_create_policy;
	ps->rmt_q_destroy_policy = qta_rmt_q_destroy_policy;

	LOG_INFO("Loaded QTA MUX policy set and its configuration");

	return &ps->base;
}

static void rmt_ps_qta_destroy(struct ps_base * bps)
{
	struct rmt_ps *ps = container_of(bps, struct rmt_ps, base);

	if (bps) {
		if (ps && ps->priv)
			qta_mux_set_destroy(ps->priv);
	}
}

static struct ps_factory qta_factory = {
		.owner   = THIS_MODULE,
		.create  = rmt_ps_qta_create,
		.destroy = rmt_ps_qta_destroy,
};

static int __init mod_init(void)
{
	int ret;

	strcpy(qta_factory.name, RINA_QTA_MUX_PS_NAME);

	ret = rmt_ps_publish(&qta_factory);
	if (ret) {
		LOG_ERR("Failed to publish policy set factory");
		return -1;
	}

	LOG_INFO("RMT QTA MUX policy set loaded successfully");

	return 0;
}

static void __exit mod_exit(void)
{
	int ret = rmt_ps_unpublish(RINA_QTA_MUX_PS_NAME);

	if (ret) {
		LOG_ERR("Failed to unpublish policy set factory");
		return;
	}

	LOG_INFO("RMT QTA MUX policy set unloaded successfully");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RMT QTA MUX policy set");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Vincenzo Maffione <v.maffione@nextworks.it>");
MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
MODULE_AUTHOR("Miquel Tarzan <miquel.tarzan@i2cat.net>");
