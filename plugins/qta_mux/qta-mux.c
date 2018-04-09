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
#include <linux/ktime.h>

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
};

struct urgency_queue {
	struct list_head list;
	struct list_head queued_pdus;
	uint_t           urgency_level;
	uint_t           length;
	struct robject   robj;
	uint_t		 dropped_pdus;
	uint_t		 dropped_bytes;
	uint_t		 tx_pdus;
	uint_t		 tx_bytes;
};

struct cu_mux {
	struct list_head   urgency_queues;
	struct list_head   mgmt_queue;
	struct robject	   robj;
	struct rset *      rset;
	uint_t             urgency_levels;
	uint_t             cherish_levels;
};

struct token_bucket_filter {
	struct list_head list;
	qos_id_t         qos_id;
	uint_t		 urgency_level;
	uint_t           abs_cherish_threshold; /* in PDUs */
	uint_t		 prob_cherish_threshold; /* in PDUs */
	uint_t		 drop_probability; /* 1- 100*/
	s64              bucket_capacity;  /* in bytes */
	s64              tokens; /* in bytes */
	uint_t           max_rate; /* in bps */
	s64		 last_pdu_time; /* in ns */
	uint_t		 dropped_pdus;
	uint_t		 dropped_bytes;
	uint_t		 tx_pdus;
	uint_t		 tx_bytes;
};

struct token_bucket_conf {
	struct list_head list;
	qos_id_t qos_id;
	uint_t   urgency_level;
	uint_t   cherish_level;
	uint_t   max_burst_size; /* in bytes */
	uint_t   max_rate; /* in bps */
};

struct cherish_thres_t {
	uint_t abs_trheshold;    /* number of PDUs */
	uint_t prob_threshold;   /* number of PDUs */
	uint_t drop_probability; /* 0 - 100 */
};

struct cu_mux_conf {
	uint_t   	  urgency_levels;
	uint_t   	  cherish_levels;
	struct cherish_thres_t * cherish_thresholds;
};

struct qta_mux_conf {
	struct cu_mux_conf cu_mux_conf;
	struct list_head token_buckets_conf;
};

struct qta_mux {
	struct list_head list;
	struct list_head token_bucket_filters;
	struct cu_mux    cu_mux;
	port_id_t        port_id;
	uint_t           occupation;
	struct robject   robj;
};

struct qta_mux_set {
	struct list_head      qta_muxes;
	struct qta_mux_conf * qta_mux_conf;
};

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

static ssize_t cu_mux_attr_show(struct robject * robj,
				struct robj_attribute * attr,
				char * buf)
{
	struct cu_mux * cu_mux;

	cu_mux = container_of(robj, struct cu_mux, robj);
	if (!cu_mux)
		return 0;

	if (strcmp(robject_attr_name(attr), "urgency_levels") == 0) {
		return sprintf(buf, "%u\n", cu_mux->urgency_levels);
	}

	if (strcmp(robject_attr_name(attr), "cherish_levels") == 0) {
		return sprintf(buf, "%u\n", cu_mux->cherish_levels);
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
	if (strcmp(robject_attr_name(attr), "dropped_pdus") == 0) {
		return sprintf(buf, "%u\n", q->dropped_pdus);
	}
	if (strcmp(robject_attr_name(attr), "tx_pdus") == 0) {
		return sprintf(buf, "%u\n", q->tx_pdus);
	}

	return 0;
}

RINA_SYSFS_OPS(qta_mux);
RINA_ATTRS(qta_mux, port_id, occupation);
RINA_KTYPE(qta_mux);
RINA_SYSFS_OPS(cu_mux);
RINA_ATTRS(cu_mux, urgency_levels, cherish_levels);
RINA_KTYPE(cu_mux);
RINA_SYSFS_OPS(urgency_queue);
RINA_ATTRS(urgency_queue, urgency, queued_pdus, dropped_pdus, tx_pdus);
RINA_KTYPE(urgency_queue);

static struct token_bucket_conf * token_bucket_conf_create(void)
{
	struct token_bucket_conf * tmp;

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

static void token_bucket_conf_destroy(struct token_bucket_conf * conf)
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

	INIT_LIST_HEAD(&tmp->token_buckets_conf);
	tmp->cu_mux_conf.cherish_levels = 0;
	tmp->cu_mux_conf.urgency_levels = 0;
	tmp->cu_mux_conf.cherish_thresholds = NULL;

	return tmp;
}

static void qta_mux_conf_destroy(struct qta_mux_conf * qta_mux_conf)
{
	struct token_bucket_conf *pos, *next;

	if (!qta_mux_conf)
		return;

	if (qta_mux_conf->cu_mux_conf.cherish_thresholds)
		rkfree(qta_mux_conf->cu_mux_conf.cherish_thresholds);

	list_for_each_entry_safe(pos, next,
			&qta_mux_conf->token_buckets_conf, list) {
		token_bucket_conf_destroy(pos);
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

static void token_bucket_filter_destroy(struct token_bucket_filter * sq)
{
	if (!sq)
		return;

	list_del(&sq->list);

	rkfree(sq);
}

static void qta_mux_destroy(struct qta_mux * qta_mux)
{
	struct urgency_queue *pos, *next;
	struct token_bucket_filter *pos2, *next2;
	struct pdu_entry *pos3, *next3;

	if (!qta_mux)
		return;

	list_del(&qta_mux->list);

	list_for_each_entry_safe(pos, next,
			&qta_mux->cu_mux.urgency_queues, list) {
		urgency_queue_destroy(pos);
	}

	list_for_each_entry_safe(pos2, next2,
			&qta_mux->token_bucket_filters, list) {
		token_bucket_filter_destroy(pos2);
	}

	list_for_each_entry_safe(pos3, next3,
			&qta_mux->cu_mux.mgmt_queue, list) {
		pdu_entry_destroy(pos3, true);
	}

	robject_del(&qta_mux->cu_mux.robj);
	if (qta_mux->cu_mux.rset)
		rset_unregister(qta_mux->cu_mux.rset);
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
	INIT_LIST_HEAD(&tmp->token_bucket_filters);
	INIT_LIST_HEAD(&tmp->cu_mux.urgency_queues);
	INIT_LIST_HEAD(&tmp->cu_mux.mgmt_queue);
	tmp->port_id = port_id;
	tmp->occupation = 0;

	if (robject_init_and_add(&tmp->robj, &qta_mux_rtype, parent, "qta_mux")) {
		LOG_ERR("Failed to create QTA MUX sysfs object");
		qta_mux_destroy(tmp);
		return NULL;
	}

	if (robject_init_and_add(&tmp->cu_mux.robj, &cu_mux_rtype, &tmp->robj, "cumux")) {
		LOG_ERR("Failed to create CU MUX sysfs object");
		qta_mux_destroy(tmp);
		return NULL;
	}

	tmp->cu_mux.rset = rset_create_and_add("urgency_queues", parent);
	if (!tmp->cu_mux.rset) {
		LOG_ERR("Failed to create urgency_queues sysfs set");
		qta_mux_destroy(tmp);
		return NULL;
	}

	return tmp;
}

static struct token_bucket_filter * token_bucket_filter_create(struct cu_mux_conf * cu_mux_conf,
						 	       struct token_bucket_conf * sq_conf)
{
	struct token_bucket_filter * tmp;
	struct cherish_thres_t cherish_thres;

	tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
	if (!tmp) {
		LOG_ERR("Problems creating stream queue");
		return NULL;
	}

	cherish_thres = cu_mux_conf->cherish_thresholds[sq_conf->cherish_level -1];

	INIT_LIST_HEAD(&tmp->list);
	tmp->max_rate = sq_conf->max_rate;
	tmp->qos_id = sq_conf->qos_id;
	tmp->urgency_level = sq_conf->urgency_level;
	tmp->abs_cherish_threshold = cherish_thres.abs_trheshold;
	tmp->prob_cherish_threshold = cherish_thres.prob_threshold;
	tmp->drop_probability = cherish_thres.drop_probability;
	tmp->last_pdu_time = ktime_get_ns();
	tmp->tokens = sq_conf->max_burst_size;
	tmp->bucket_capacity = sq_conf->max_burst_size;
	tmp->max_rate = sq_conf->max_rate;

	tmp->dropped_bytes = 0;
	tmp->dropped_pdus = 0;
	tmp->tx_bytes = 0;
	tmp->tx_pdus = 0;

	return tmp;
}

static struct urgency_queue * urgency_queue_create(uint_t urgency_level,
						   struct rset *parent)
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
	tmp->dropped_bytes = 0;
	tmp->dropped_pdus = 0;
	tmp->tx_bytes = 0;
	tmp->tx_pdus = 0;

	robject_init(&tmp->robj, &urgency_queue_rtype);
	if (robject_rset_add(&tmp->robj, parent, "%d", tmp->urgency_level)) {
		LOG_ERR("Failed to create Urgency queue sysfs entry");
		urgency_queue_destroy(tmp);
		return NULL;
	}

	return tmp;
}

static struct pdu_entry * pdu_entry_create(struct du* du)
{
	struct pdu_entry * tmp;

	tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
	if (!tmp) {
		LOG_ERR("Problems creating PDU entry");
		return NULL;
	}

	INIT_LIST_HEAD(&tmp->list);
	tmp->pdu = du;

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

static struct token_bucket_filter * tbf_find(struct qta_mux * qta_mux,
					     qos_id_t qos_id)
{
	struct token_bucket_filter * pos;

	list_for_each_entry(pos, &qta_mux->token_bucket_filters, list) {
		if (pos->qos_id == qos_id)
			return pos;
	}

	return NULL;
}

static struct urgency_queue * urgency_queue_find(struct qta_mux * qta_mux,
						 uint_t urgency_level)
{
	struct urgency_queue * pos;

	list_for_each_entry(pos, &qta_mux->cu_mux.urgency_queues, list) {
		if (pos->urgency_level == urgency_level)
			return pos;
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
			pos->tx_pdus++;
			pos->tx_bytes += du_len(ret_pdu);

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
	struct qta_mux * 	     qta_mux;
	struct token_bucket_filter * tbf;
	struct urgency_queue *       urgency_queue;
	qos_id_t               	     qos_id;
	pdu_type_t 		     pdu_type;
	struct pdu_entry * 	     pdu_entry;
	s64			     now;
	s64			     delta_tokens;
	ssize_t			     pdu_length;
	uint_t			     random_bytes;

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
		pdu_entry = pdu_entry_create(du);
		if (!pdu_entry) {
			LOG_ERR("Problems allocating memory for PDU entry");
			du_destroy(du);
			return RMT_PS_ENQ_ERR;
		}

		list_add_tail(&pdu_entry->list, &qta_mux->cu_mux.mgmt_queue);
		return RMT_PS_ENQ_SCHED;
	}

	/* Retrieve token bucket filter for the PDU's qos-id */
	qos_id = pci_qos_id(&du->pci);
	tbf = tbf_find(qta_mux, qos_id);
	if (!tbf) {
		LOG_ERR("No TBF for QoS id %u, dropping PDU", qos_id);
		du_destroy(du);
		return RMT_PS_ENQ_ERR;
	}

	/* Update num tokens and see if PDU can go through */
	now = ktime_get_ns();
	delta_tokens = (now - tbf->last_pdu_time) * tbf->max_rate;
	do_div(delta_tokens, 1000000000);
	do_div(delta_tokens, 8);
	tbf->tokens += delta_tokens;
	if (tbf->tokens > tbf->bucket_capacity)
		tbf->tokens = tbf->bucket_capacity;
	tbf->last_pdu_time = now;

	pdu_length = du_len(du);
	if (pdu_length > tbf->tokens) {
		/* PDU has to be dropped, not enough tokens to transmit it */
		tbf->dropped_pdus++;
		tbf->dropped_bytes += pdu_length;
		du_destroy(du);
		return RMT_PS_ENQ_DROP;
	}

	/* PDU can be transmitted, put it in the right urgency queue */
	tbf->tokens -= pdu_length;
	tbf->tx_bytes += pdu_length;
	tbf->tx_pdus++;

	urgency_queue = urgency_queue_find(qta_mux, tbf->urgency_level);
	if (!urgency_queue) {
		LOG_ERR("No urgency queue for level %u, dropping PDU",
			tbf->urgency_level);
		du_destroy(du);
		return RMT_PS_ENQ_ERR;
	}

	/* queue length is larger than cherish threshold, drop PDU */
	if (urgency_queue->length > tbf->abs_cherish_threshold) {
		urgency_queue->dropped_bytes += pdu_length;
		urgency_queue->dropped_pdus++;
		du_destroy(du);
		return RMT_PS_ENQ_DROP;
	}

	/* queue length is larger than probabilistic ch. threshold */
	if (urgency_queue->length > tbf->prob_cherish_threshold) {
		get_random_bytes(&random_bytes, sizeof(random_bytes));
		random_bytes = random_bytes % NORM_PROB;
		if (tbf->drop_probability > random_bytes) {
			urgency_queue->dropped_bytes += pdu_length;
			urgency_queue->dropped_pdus++;
			du_destroy(du);
			return RMT_PS_ENQ_DROP;
		}
	}

	pdu_entry = pdu_entry_create(du);
	if (!pdu_entry) {
		LOG_ERR("Problems allocating memory for PDU entry");
		du_destroy(du);
		return RMT_PS_ENQ_ERR;
	}

	list_add_tail(&pdu_entry->list, &urgency_queue->queued_pdus);
	urgency_queue->length++;
	qta_mux->occupation++;

	return RMT_PS_ENQ_SCHED;
}

void * qta_rmt_q_create_policy(struct rmt_ps      *ps,
			       struct rmt_n1_port *n1_port)
{
	struct qta_mux_set * qta_mux_set = ps->priv;
	struct qta_mux * qta_mux;
	struct qta_mux_conf * config;
	struct token_bucket_conf * pos;
	struct token_bucket_filter * tbf;
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

	/* Create one token bucket filter per QoS level */
	list_for_each_entry(pos, &config->token_buckets_conf, list) {
		tbf = token_bucket_filter_create(&config->cu_mux_conf, pos);
		if (!tbf) {
			LOG_ERR("Problems creating token bucket filter");
			qta_mux_destroy(qta_mux);
			return NULL;
		}

		list_add_tail(&tbf->list, &qta_mux->token_bucket_filters);
		LOG_INFO("Added token bucket filter for QoS id %u", pos->qos_id);
	}

	/* Create one urgency_queue per urgency level, add it to the qta_mux */
	for (i=0; i<config->cu_mux_conf.urgency_levels; i++) {
		urgency_queue = urgency_queue_create(i+1, qta_mux->cu_mux.rset);
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
	struct cherish_thres_t * cherish_levels;
	struct token_bucket_conf * token_bucket_conf;

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

		token_bucket_conf = token_bucket_conf_create();
		if (!token_bucket_conf) {
			LOG_ERR("Could not allocate memory");
			return -1;
		}

		token_bucket_conf->qos_id = qos_id;
		token_bucket_conf->urgency_level = urgency_level;
		token_bucket_conf->cherish_level = cherish_level;
		token_bucket_conf->max_burst_size = max_burst_size;
		token_bucket_conf->max_rate = max_rate;
		list_add_tail(&token_bucket_conf->list,
			      &data->qta_mux_conf->token_buckets_conf);

		return 0;
	}

	/*
	 * Parse entry of type name = cumux
	 * value = <urgency_levels>:<cherish_levels>:
	 * <abs_cherish_thres1>,<prob_cherish_thres1>,<drop_prob1>:...
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

		cherish_levels = rkzalloc(cherish_level * sizeof(struct cherish_thres_t),
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
			if (parse_int_value(value + offset, ',',
					    (int *) &cherish_levels[i].abs_trheshold,
					    &delta_offset)) {
				rkfree(cherish_levels);
				return -1;
			}

			offset += delta_offset;
			if (parse_int_value(value + offset, ',',
					    (int *) &cherish_levels[i].prob_threshold,
					    &delta_offset)) {
				rkfree(cherish_levels);
				return -1;
			}

			offset += delta_offset;
			if (parse_int_value(value + offset, c,
					    (int *) &cherish_levels[i].drop_probability,
					    &delta_offset)) {
				rkfree(cherish_levels);
				return -1;
			}

			LOG_INFO("Parsed cherish_level[%d]: %u, %u, %u", i,
				 cherish_levels[i].abs_trheshold,
				 cherish_levels[i].prob_threshold,
				 cherish_levels[i].drop_probability);

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
MODULE_AUTHOR("Eduard Grasa <eduard.grasa@i2cat.net");
