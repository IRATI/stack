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
#include "qta-mux-debug.h"

#define RINA_QTA_MUX_PS_NAME "qta-mux-ps"
#define NORM_PROB                100

#define TIMER_T  100

struct pdu_entry {
	struct list_head list;
	struct du *      pdu;
};

struct urgency_queue {
	struct list_head 	   list;
	struct list_head 	   queued_pdus;
	uint_t           	   urgency_level;
	uint_t           	   length;
	struct robject   	   robj;
	uint_t			   dropped_pdus;
	uint_t		 	   dropped_bytes;
	uint_t		 	   tx_pdus;
	uint_t		 	   tx_bytes;
	uint_t			   dequeue_prob;
	uint_t			   max_occupation;
#if QTA_MUX_DEBUG
	struct urgency_queue_debug_info * debug_info;
#endif
};

struct cu_mux {
	struct list_head      urgency_queues;
	struct list_head      mgmt_queue;
	struct robject	      robj;
	struct rset *         rset;
};

struct token_bucket_filter {
	struct list_head list;
	qos_id_t         qos_id;
	uint_t		 urgency_level;
	uint_t		 cherish_level;
	uint_t           abs_cherish_threshold; /* in PDUs */
	uint_t		 prob_cherish_threshold; /* in PDUs */
	uint_t		 drop_probability; /* 1- 100*/
	s64              bucket_capacity;  /* in bytes */
	s64              tokens; /* in bytes */
	uint_t           max_rate; /* in bps */
	s64		 last_pdu_time; /* in ns */
	bool		 drop; /* true to drop the packets, false to set ECN flag */
	uint_t		 dropped_pdus;
	uint_t		 dropped_pdus_cu_mux;
	uint_t		 dropped_bytes;
	uint_t		 tx_pdus;
	uint_t		 tx_bytes;
	struct robject   robj;
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
	bool drop; /* true to drop the packets, false to set ECN flag */
};

struct urgency_queue_conf {
	struct list_head         list;
	uint_t   	         urgency_level;
	uint_t			 dequeue_prob;
	struct cherish_thres_t * cherish_thresholds;
};

struct qta_mux_conf {
	struct list_head urgency_queue_conf;
	struct list_head token_buckets_conf;
};

struct qta_mux {
	struct list_head list;
	struct list_head token_bucket_filters;
	struct cu_mux    cu_mux;
	port_id_t        port_id;
	struct robject   robj;
	struct rset *    rset;
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
	if (strcmp(robject_attr_name(attr), "dequeue_prob") == 0) {
			return sprintf(buf, "%u\n", q->dequeue_prob);
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
	if (strcmp(robject_attr_name(attr), "max_occupation") == 0) {
		return sprintf(buf, "%u\n", q->max_occupation);
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
		return sprintf(buf, "%u\n", 0);
	}

	if (strcmp(robject_attr_name(attr), "cherish_levels") == 0) {
		return sprintf(buf, "%u\n", 0);
	}

	return 0;
}

static ssize_t token_bucket_filter_attr_show(struct robject * robj,
				       	     struct robj_attribute * attr,
					     char * buf)
{
	struct token_bucket_filter * tbf;

	tbf = container_of(robj, struct token_bucket_filter, robj);
	if (!tbf)
		return 0;

	if (strcmp(robject_attr_name(attr), "abs_cherish_th") == 0) {
		return sprintf(buf, "%u\n", tbf->abs_cherish_threshold);
	}
	if (strcmp(robject_attr_name(attr), "prob_cherish_th") == 0) {
		return sprintf(buf, "%u\n", tbf->prob_cherish_threshold);
	}
	if (strcmp(robject_attr_name(attr), "drop_prob") == 0) {
		return sprintf(buf, "%u\n", tbf->drop_probability);
	}
	if (strcmp(robject_attr_name(attr), "bucket_capacity") == 0) {
		return sprintf(buf, "%lld\n", tbf->bucket_capacity);
	}
	if (strcmp(robject_attr_name(attr), "last_pdu_time") == 0) {
		return sprintf(buf, "%lld\n", tbf->last_pdu_time);
	}
	if (strcmp(robject_attr_name(attr), "max_rate") == 0) {
		return sprintf(buf, "%u\n", tbf->max_rate);
	}
	if (strcmp(robject_attr_name(attr), "urgency_level") == 0) {
		return sprintf(buf, "%u\n", tbf->urgency_level);
	}
	if (strcmp(robject_attr_name(attr), "tokens") == 0) {
		return sprintf(buf, "%lld\n", tbf->tokens);
	}
	if (strcmp(robject_attr_name(attr), "tbf_drop_bytes") == 0) {
		return sprintf(buf, "%u\n", tbf->dropped_bytes);
	}
	if (strcmp(robject_attr_name(attr), "tbf_drop_pdus") == 0) {
		return sprintf(buf, "%u\n", tbf->dropped_pdus);
	}
	if (strcmp(robject_attr_name(attr), "dropped_pdus_cu_mux") == 0) {
		return sprintf(buf, "%u\n", tbf->dropped_pdus_cu_mux);
	}
	if (strcmp(robject_attr_name(attr), "tbf_tx_bytes") == 0) {
		return sprintf(buf, "%u\n", tbf->tx_bytes);
	}
	if (strcmp(robject_attr_name(attr), "tbf_tx_pdus") == 0) {
		return sprintf(buf, "%u\n", tbf->tx_pdus);
	}

	return 0;
}

RINA_SYSFS_OPS(qta_mux);
RINA_ATTRS(qta_mux, port_id);
RINA_KTYPE(qta_mux);
RINA_SYSFS_OPS(cu_mux);
RINA_ATTRS(cu_mux, urgency_levels, cherish_levels);
RINA_KTYPE(cu_mux);
RINA_SYSFS_OPS(urgency_queue);
RINA_ATTRS(urgency_queue, urgency, dequeue_prob, queued_pdus, dropped_pdus, tx_pdus, max_occupation);
RINA_KTYPE(urgency_queue);
RINA_SYSFS_OPS(token_bucket_filter);
RINA_ATTRS(token_bucket_filter, abs_cherish_th, prob_cherish_th, drop_prob,
	   bucket_capacity, last_pdu_time, max_rate, urgency_level, tokens,
	   tbf_drop_bytes, tbf_drop_pdus, dropped_pdus_cu_mux, tbf_tx_bytes, tbf_tx_pdus);
RINA_KTYPE(token_bucket_filter);

static struct token_bucket_conf * token_bucket_conf_create(void)
{
	struct token_bucket_conf * tmp;

	tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
	if (!tmp) {
		LOG_ERR("Problems creating TBF configuration");
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

static struct urgency_queue_conf * urgency_queue_conf_create(void)
{
	struct urgency_queue_conf * tmp;

	tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
	if (!tmp) {
		LOG_ERR("Problems creating urgency_queue_conf configuration");
		return NULL;
	}

	INIT_LIST_HEAD(&tmp->list);
	tmp->urgency_level = 0;
	tmp->dequeue_prob = 100;
	tmp->cherish_thresholds = NULL;

	return tmp;
}

static void urgency_queue_conf_destroy(struct urgency_queue_conf * conf)
{
	if (!conf)
		return;

	list_del(&conf->list);

	rkfree(conf->cherish_thresholds);
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
	INIT_LIST_HEAD(&tmp->urgency_queue_conf);

	return tmp;
}

static void qta_mux_conf_destroy(struct qta_mux_conf * qta_mux_conf)
{
	struct token_bucket_conf *pos, *next;
	struct urgency_queue_conf *pos2, *next2;

	if (!qta_mux_conf)
		return;

	list_for_each_entry_safe(pos, next,
			&qta_mux_conf->token_buckets_conf, list) {
		token_bucket_conf_destroy(pos);
	}

	list_for_each_entry_safe(pos2, next2,
			&qta_mux_conf->urgency_queue_conf, list) {
		urgency_queue_conf_destroy(pos2);
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

#if QTA_MUX_DEBUG
	udebug_info_destroy(uq->debug_info);
#endif

	rkfree(uq);
}

static void token_bucket_filter_destroy(struct token_bucket_filter * sq)
{
	if (!sq)
		return;

	list_del(&sq->list);

	robject_del(&sq->robj);

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
	if (qta_mux->rset)
		rset_unregister(qta_mux->rset);
	robject_del(&qta_mux->robj);

	rkfree(qta_mux);
}

static struct qta_mux * qta_mux_create(port_id_t port_id,
		   	   	       struct robject *parent,
				       struct qta_mux_conf * conf)
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

	tmp->cu_mux.rset = rset_create_and_add("urgency_queues", &tmp->cu_mux.robj);
	if (!tmp->cu_mux.rset) {
		LOG_ERR("Failed to create urgency_queues sysfs set");
		qta_mux_destroy(tmp);
		return NULL;
	}

	tmp->rset = rset_create_and_add("token_bucket_filters", &tmp->robj);
	if (!tmp->rset) {
		LOG_ERR("Failed to create token bucket filters sysfs set");
		qta_mux_destroy(tmp);
		return NULL;
	}

	return tmp;
}

static struct token_bucket_filter * token_bucket_filter_create(struct urgency_queue_conf * uq_conf,
						 	       struct token_bucket_conf * sq_conf,
							       struct rset * parent)
{
	struct token_bucket_filter * tmp;
	struct cherish_thres_t cherish_thres;

	tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
	if (!tmp) {
		LOG_ERR("Problems creating stream queue");
		return NULL;
	}

	cherish_thres = uq_conf->cherish_thresholds[sq_conf->cherish_level -1];

	INIT_LIST_HEAD(&tmp->list);
	tmp->max_rate = sq_conf->max_rate;
	tmp->qos_id = sq_conf->qos_id;
	tmp->urgency_level = sq_conf->urgency_level;
	tmp->cherish_level = sq_conf->cherish_level;
	tmp->abs_cherish_threshold = cherish_thres.abs_trheshold;
	tmp->prob_cherish_threshold = cherish_thres.prob_threshold;
	tmp->drop_probability = cherish_thres.drop_probability;
	tmp->last_pdu_time = ktime_get_ns();
	tmp->tokens = sq_conf->max_burst_size;
	tmp->bucket_capacity = sq_conf->max_burst_size;
	tmp->max_rate = sq_conf->max_rate;
	tmp->drop = cherish_thres.drop;

	tmp->dropped_bytes = 0;
	tmp->dropped_pdus = 0;
	tmp->dropped_pdus_cu_mux = 0;
	tmp->tx_bytes = 0;
	tmp->tx_pdus = 0;

	robject_init(&tmp->robj, &token_bucket_filter_rtype);
	if (robject_rset_add(&tmp->robj, parent, "%d", tmp->qos_id)) {
		LOG_ERR("Failed to token bucket filter sysfs entry");
		token_bucket_filter_destroy(tmp);
		return NULL;
	}

	return tmp;
}

static struct urgency_queue * urgency_queue_create(uint_t urgency_level,
						   uint_t dequeue_prob,
						   port_id_t port_id,
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
	tmp->dequeue_prob = dequeue_prob;
	tmp->dropped_bytes = 0;
	tmp->dropped_pdus = 0;
	tmp->tx_bytes = 0;
	tmp->tx_pdus = 0;
	tmp->max_occupation = 0;

	robject_init(&tmp->robj, &urgency_queue_rtype);
	if (robject_rset_add(&tmp->robj, parent, "%d", tmp->urgency_level)) {
		LOG_ERR("Failed to create Urgency queue sysfs entry");
		urgency_queue_destroy(tmp);
		return NULL;
	}

#if QTA_MUX_DEBUG
	tmp->debug_info = uqueue_debug_info_create(port_id, urgency_level);
#endif

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
	uint_t		       random_bytes;
	struct urgency_queue * candidate;

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
	candidate = NULL;
	list_for_each_entry(pos, &qta_mux->cu_mux.urgency_queues, list) {
		if (!list_empty(&pos->queued_pdus)) {
			/* This queue is not empty, can be a candidate */
			get_random_bytes(&random_bytes, sizeof(random_bytes));
			random_bytes = random_bytes % NORM_PROB;
			if (pos->dequeue_prob > random_bytes) {
				/* We can dequeue from this urgency queue */
				candidate = pos;
				goto exit;
			} else if (!candidate) {
				candidate = pos;
			}
		}
	}

exit:
	if (!candidate)
		return NULL;

	ret_pdu = dequeue_pdu(&candidate->queued_pdus);
	candidate->length--;
	candidate->tx_pdus++;
	candidate->tx_bytes += du_len(ret_pdu);

#if QTA_MUX_DEBUG
if (candidate->debug_info->q_index < UQUEUE_DEBUG_SIZE) {
	candidate->debug_info->q_log[candidate->debug_info->q_index][0] = candidate->length;
	candidate->debug_info->q_log[candidate->debug_info->q_index][1] = ktime_get_ns();
	candidate->debug_info->q_index++;
}
#endif

	return ret_pdu;
}

int qta_rmt_enqueue_policy(struct rmt_ps	  *ps,
			   struct rmt_n1_port *n1_port,
			   struct du	  *du,
			   bool must_enqueue)
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
	bool 			     ecn_mark;
	pdu_flags_t     	     pci_flags;

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
		if (!must_enqueue && list_empty(&qta_mux->cu_mux.mgmt_queue))
			return RMT_PS_ENQ_SEND;

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

	/* PDU can be transmitted, update TBF tokens and stats */
	tbf->tokens -= pdu_length;
	tbf->tx_bytes += pdu_length;
	tbf->tx_pdus++;

	/* If we don't need to enqueue it means there are 0 queued PDUs, tx */
	if (!must_enqueue)
		return RMT_PS_ENQ_SEND;

	/* Put PDU put it in the right urgency queue */
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
		tbf->dropped_pdus_cu_mux++;
		du_destroy(du);
		return RMT_PS_ENQ_DROP;
	}

	/* queue length is larger than probabilistic ch. threshold */
	ecn_mark = false;
	if (urgency_queue->length > tbf->prob_cherish_threshold) {
		get_random_bytes(&random_bytes, sizeof(random_bytes));
		random_bytes = random_bytes % NORM_PROB;
		if (tbf->drop_probability > random_bytes) {
			if (tbf->drop) {
				urgency_queue->dropped_bytes += pdu_length;
				urgency_queue->dropped_pdus++;
				tbf->dropped_pdus_cu_mux++;
				du_destroy(du);
				return RMT_PS_ENQ_DROP;
			} else {
				ecn_mark = true;
			}
		}
	}

	pdu_entry = pdu_entry_create(du);
	if (!pdu_entry) {
		LOG_ERR("Problems allocating memory for PDU entry");
		du_destroy(du);
		return RMT_PS_ENQ_ERR;
	}

	if (ecn_mark) {
		pci_flags = pci_flags_get(&du->pci);
		pci_flags_set(&du->pci,
			      pci_flags |= PDU_FLAGS_EXPLICIT_CONGESTION);
	}
	list_add_tail(&pdu_entry->list, &urgency_queue->queued_pdus);
	urgency_queue->length++;
	if (urgency_queue->length > urgency_queue->max_occupation)
		urgency_queue->max_occupation = urgency_queue->length;

#if QTA_MUX_DEBUG
	if (urgency_queue->debug_info->q_index < UQUEUE_DEBUG_SIZE) {
		urgency_queue->debug_info->q_log[urgency_queue->debug_info->q_index][0] = urgency_queue->length;
		urgency_queue->debug_info->q_log[urgency_queue->debug_info->q_index][1] = now;
		urgency_queue->debug_info->q_index++;
	}
#endif

	return RMT_PS_ENQ_SCHED;
}

static struct urgency_queue_conf * uqc_find(struct qta_mux_conf * conf,
					    uint_t urgency_level)
{
	struct urgency_queue_conf * pos;

	list_for_each_entry(pos, &conf->urgency_queue_conf, list) {
		if (pos->urgency_level == urgency_level)
			return pos;
	}

	return NULL;
}

static struct token_bucket_conf * tbc_find(struct qta_mux_conf * conf,
					   qos_id_t qos_id)
{
	struct token_bucket_conf * pos;

	list_for_each_entry(pos, &conf->token_buckets_conf, list) {
		if (pos->qos_id == qos_id)
			return pos;
	}

	return NULL;
}

static void qta_mux_add_uqueue(struct qta_mux *     qta_mux,
			       struct urgency_queue * uq)
{
	struct urgency_queue * pos;

	if (list_empty(&qta_mux->cu_mux.urgency_queues)) {
		list_add(&uq->list, &qta_mux->cu_mux.urgency_queues);
		return;
	}

	list_for_each_entry(pos, &qta_mux->cu_mux.urgency_queues, list) {
		if (pos->urgency_level > uq->urgency_level) {
			list_add_tail(&uq->list, &pos->list);
			return;
		}
	}

	list_add_tail(&uq->list, &qta_mux->cu_mux.urgency_queues);
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
	struct urgency_queue_conf * uq_conf;

	if (!ps || !n1_port || !qta_mux_set) {
		LOG_ERR("Wrong input parameters for "
				"rmt_scheduling_create_policy_common");
		return NULL;
	}

	/* Create the qta_mux and add it to the n1_port */
	config = qta_mux_set->qta_mux_conf;
	qta_mux = qta_mux_create(n1_port->port_id, &n1_port->robj, config);
	if (!qta_mux) {
		LOG_ERR("No scheduling policy created for port-id %d",
				n1_port->port_id);
		return NULL;
	}
	list_add(&qta_mux->list, &qta_mux_set->qta_muxes);

	/* Create one token bucket filter per QoS level */
	list_for_each_entry(pos, &config->token_buckets_conf, list) {
		uq_conf = uqc_find(config, pos->urgency_level);
		if (!uq_conf) {
			LOG_ERR("Could not find conf for urgency level %u",
				pos->urgency_level);
			qta_mux_destroy(qta_mux);
			return NULL;
		}
		tbf = token_bucket_filter_create(uq_conf, pos, qta_mux->rset);
		if (!tbf) {
			LOG_ERR("Problems creating token bucket filter");
			qta_mux_destroy(qta_mux);
			return NULL;
		}

		list_add_tail(&tbf->list, &qta_mux->token_bucket_filters);
		LOG_INFO("Added token bucket filter for QoS id %u", pos->qos_id);
	}

	/* Create one urgency_queue per urgency level, add it to the qta_mux */
	list_for_each_entry(uq_conf, &config->urgency_queue_conf, list) {
		urgency_queue = urgency_queue_create(uq_conf->urgency_level,
						     uq_conf->dequeue_prob,
						     n1_port->port_id,
						     qta_mux->cu_mux.rset);
		if (!urgency_queue) {
			LOG_ERR("Problems creating urgency queue");
			qta_mux_destroy(qta_mux);
			return NULL;
		}

		qta_mux_add_uqueue(qta_mux, urgency_queue);
		LOG_INFO("Added urgency queue for urgency level %d",
			 uq_conf->urgency_level);
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
	uint_t urgency_level, cherish_level, max_burst_size, max_rate,
	       drop, dequeue_prob;
	char *qos, *mux;
	int i, c;
	struct cherish_thres_t * cherish_levels;
	struct token_bucket_conf * token_bucket_conf;
	struct urgency_queue_conf * urgency_queue_conf;
	struct qta_mux * qta_mux;
	struct token_bucket_filter * tbf;
	bool append;
	struct cherish_thres_t cherish_thres;

	if (!name) {
		LOG_ERR("Null parameter name");
		return -1;
	}

	if (!value) {
		LOG_ERR("Null parameter value");
		return -1;
	}

	qos = strchr(name, 'q');
	mux = strchr(name, 'x');

	/*
	 * Parse entry of type name = qosid.<qos_id>
	 * value = <urgency_level>:<cherish_level>:<max_burst_size>:<rate>
	 */
	if (qos) {
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

		token_bucket_conf = tbc_find(data->qta_mux_conf, qos_id);
		if (token_bucket_conf) {
			LOG_INFO("Overriding token bucket conf config for qos_id %d",
				 qos_id);
			append = false;
		} else {
			append = true;
			token_bucket_conf = token_bucket_conf_create();
			if (!token_bucket_conf) {
				LOG_ERR("Could not allocate memory");
				return -1;
			}
			token_bucket_conf->qos_id = qos_id;
		}

		token_bucket_conf->urgency_level = urgency_level;
		token_bucket_conf->cherish_level = cherish_level;
		token_bucket_conf->max_burst_size = max_burst_size;
		token_bucket_conf->max_rate = max_rate;

		if (append) {
			list_add_tail(&token_bucket_conf->list,
				      &data->qta_mux_conf->token_buckets_conf);
		} else {
			list_for_each_entry(qta_mux, &data->qta_muxes, list) {
				//FIXME add lock to qta_mux and take it
				list_for_each_entry(tbf, &qta_mux->token_bucket_filters, list) {
					if (tbf->qos_id == qos_id) {
						tbf->max_rate = max_rate;
						tbf->urgency_level = urgency_level;
						tbf->cherish_level = cherish_level;
						tbf->bucket_capacity = max_burst_size;
					}
				}
			}
		}

		return 0;
	}

	/*
	 * Parse entry of type name = <urgency_level>.cumux
	 * value = <cherish_levels>:<drop>:<dequeue_prob>:
	 * <abs_cherish_thres1>,<prob_cherish_thres1>,<drop_prob1>:...
	 *
	 */
	if (mux) {
		/* Parse urgency-level from the parameter name */
		offset = 0;
		if (parse_int_value(name, '.', (int *) &urgency_level,
				    &delta_offset))
			return -1;

		/* Parse # of cherish levels from the parameter value */
		offset = 0;
		if (parse_int_value(value, ':', (int *) &cherish_level,
				    &delta_offset))
			return -1;

		/* Parse drop from the parameter value */
		offset += delta_offset;
		if (parse_int_value(value + offset, ':',
				    (int *) &drop, &delta_offset))
			return -1;

		/* Parse dequeue prob from the parameter value */
		offset += delta_offset;
		if (parse_int_value(value + offset, ':', (int*) &dequeue_prob,
				    &delta_offset))
			return -1;

		/* Parse info of each cherish level from the parameter value */
		cherish_levels = rkzalloc(cherish_level * sizeof(struct cherish_thres_t),
					  GFP_ATOMIC);
		if (!cherish_levels) {
			LOG_ERR("Problems allocating memory, exiting");
			return -1;
		}

		LOG_INFO("Urgency level: %u, Drop: %u, Dequeue prob: %u",
				urgency_level, drop, dequeue_prob);

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

			cherish_levels[i].drop = drop;

			LOG_INFO("Parsed cherish_level[%d]: %u, %u, %u", i,
				 cherish_levels[i].abs_trheshold,
				 cherish_levels[i].prob_threshold,
				 cherish_levels[i].drop_probability);
		}

		urgency_queue_conf = uqc_find(data->qta_mux_conf, urgency_level);
		if (urgency_queue_conf) {
			LOG_INFO("Overriding urgency queue conf config for urgency %u",
				 urgency_level);
			append = false;
			if (urgency_queue_conf->cherish_thresholds) {
				rkfree(urgency_queue_conf->cherish_thresholds);
				urgency_queue_conf->cherish_thresholds = NULL;
			}
		} else {
			urgency_queue_conf = urgency_queue_conf_create();
			if (!urgency_queue_conf) {
				LOG_ERR("Could not allocate memory");
				return -1;
			}
			urgency_queue_conf->urgency_level = urgency_level;
			append = true;
		}

		urgency_queue_conf->cherish_thresholds = cherish_levels;
		urgency_queue_conf->dequeue_prob = dequeue_prob;
		if (append) {
			list_add_tail(&urgency_queue_conf->list,
				      &data->qta_mux_conf->urgency_queue_conf);
		} else {
			list_for_each_entry(qta_mux, &data->qta_muxes, list) {
				//FIXME add lock to qta_mux and take it
				list_for_each_entry(tbf, &qta_mux->token_bucket_filters, list) {
					if (tbf->urgency_level == urgency_level) {
						cherish_thres = cherish_levels[tbf->cherish_level -1];
						tbf->abs_cherish_threshold = cherish_thres.abs_trheshold;
						tbf->drop = cherish_thres.drop;
						tbf->prob_cherish_threshold = cherish_thres.prob_threshold;
						tbf->drop_probability = cherish_thres.drop_probability;
					}
				}
			}
		}

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

#if QTA_MUX_DEBUG
	qta_mux_debug_proc_init();
#endif

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

#if QTA_MUX_DEBUG
	qta_mux_debug_proc_exit();
#endif

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
