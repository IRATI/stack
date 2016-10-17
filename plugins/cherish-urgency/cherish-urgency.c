/*
 * Cherish Urgency plugin
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

#define RINA_PREFIX "cher-urg-plugin"

#include "logs.h"
#include "rds/rmem.h"
#include "rmt-ps.h"
#include "policies.h"
#include <linux/random.h>

#define RINA_CHERISH_URGENCY_PS_NAME "cher-urg-ps"
#define NORM_PROB                    100

#define RMT_QOS_QUEUE_HASHSIZE 7

#define TIMER_T  100

struct q_qos {
        uint_t           abs_th;
        uint_t           th;
        uint_t           drop_prob;
        qos_id_t         qos_id;
        struct list_head list;
        uint_t           dropped;
        uint_t           handled;
        uint_t           key;
        uint_t           skip_prob;
        uint_t           ecn_th;
        uint_t           queued_pdus;
        struct cu_queue * cu_q;
        struct robject   robj;
};

struct config_q_qos {
        uint_t           abs_th;
        uint_t           th;
        uint_t           drop_prob;
        qos_id_t         qos_id;
        uint_t           skip_prob;
        uint_t           key;
        uint_t           ecn_th;
        struct list_head list;
};

struct cher_urg_config {
        struct list_head list_queues;
};

struct cu_queue {
        uint_t           skip_prob;
        struct list_head list;        /* link to list of urgency queues */
        uint_t           key;         /* Urgency */
        struct list_head qos_id_list; /* head of list of qos queues */
        struct rfifo *   queue;
};

struct cu_queue_set {
        struct list_head queues;
        port_id_t        port_id;
        struct list_head list;
        uint_t           occupation;
        struct robject   robj;
};

static ssize_t cher_urg_qset_attr_show(struct robject * robj,
                                       struct robj_attribute * attr,
                                       char * buf)
{
	struct cu_queue_set * qset;

       qset = container_of(robj, struct cu_queue_set, robj);
       if (!qset)
               return 0;

       if (strcmp(robject_attr_name(attr), "port_id") == 0) {
               return sprintf(buf, "%u\n", qset->port_id);
       }

       if (strcmp(robject_attr_name(attr), "occupation") == 0) {
               return sprintf(buf, "%u\n", qset->occupation);
       }

       return 0;
}

static ssize_t qos_queue_attr_show(struct robject * robj,
                                       struct robj_attribute * attr,
                                       char * buf)
{
       struct q_qos * q;

       q = container_of(robj, struct q_qos, robj);
       if (!q)
               return 0;

       if (strcmp(robject_attr_name(attr), "urgency") == 0) {
               return sprintf(buf, "%u\n", q->key);
       }
       if (strcmp(robject_attr_name(attr), "skip_probability") == 0) {
               return sprintf(buf, "%u\n", q->skip_prob);
       }
       if (strcmp(robject_attr_name(attr), "queued_pdus") == 0) {
               return sprintf(buf, "%u\n", q->queued_pdus);
       }
       if (strcmp(robject_attr_name(attr), "drop_pdus") == 0) {
               return sprintf(buf, "%u\n", q->dropped);
       }
       if (strcmp(robject_attr_name(attr), "total_pdus") == 0) {
               return sprintf(buf, "%u\n", q->handled);
       }
       if (strcmp(robject_attr_name(attr), "threshold") == 0) {
               return sprintf(buf, "%u\n", q->th);
       }
       if (strcmp(robject_attr_name(attr), "absolute_threshold") == 0) {
               return sprintf(buf, "%u\n", q->abs_th);
       }
       if (strcmp(robject_attr_name(attr), "ecn_threshold") == 0) {
               return sprintf(buf, "%u\n", q->ecn_th);
       }
       if (strcmp(robject_attr_name(attr), "drop_probability") == 0) {
               return sprintf(buf, "%u\n", q->drop_prob);
       }
       if (strcmp(robject_attr_name(attr), "qos_id") == 0) {
               return sprintf(buf, "%u\n", q->qos_id);
       }

       return 0;
}

RINA_SYSFS_OPS(cher_urg_qset);
RINA_ATTRS(cher_urg_qset, port_id, occupation);
RINA_KTYPE(cher_urg_qset);
RINA_SYSFS_OPS(qos_queue);
RINA_ATTRS(qos_queue, queued_pdus, drop_pdus, total_pdus, urgency,
               skip_probability, threshold, absolute_threshold, ecn_th,
               drop_probability, qos_id);
RINA_KTYPE(qos_queue);

static void q_qos_destroy(struct q_qos * q)
{
        if (!q)
                return;

        list_del(&q->list);
        robject_del(&q->robj);
        rkfree(q);
}

static struct q_qos * q_qos_create(qos_id_t id,
                                   uint_t abs_th,
                                   uint_t th,
                                   uint_t drop_prob,
                                   uint_t key,
                                   uint_t skip_prob,
                                   uint_t ecn_th,
                                   struct cu_queue * cu_q,
                                   struct robject * parent)
{
        struct q_qos * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
        if (!tmp) {
                LOG_ERR("Couldn't create queue for QoS id %u", id);

                return NULL;
        }

        tmp->qos_id    = id;
        tmp->abs_th    = abs_th;
        tmp->th        = th;
        tmp->drop_prob = drop_prob;
        tmp->dropped   = 0;
        tmp->handled   = 0;
        tmp->ecn_th    = ecn_th;
        tmp->queued_pdus = 0;
        tmp->cu_q      = cu_q;
        INIT_LIST_HEAD(&tmp->list);

        if (robject_init_and_add(&tmp->robj, &qos_queue_rtype, parent, "queue-%d", id)) {
                LOG_ERR("Failed to create Cherish-Urgency QoS Queue sysfs entry");
                q_qos_destroy(tmp);
                return NULL;
        }

        return tmp;
}

static struct config_q_qos * config_q_qos_create(qos_id_t id)
{
        struct config_q_qos * tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);

        if (!tmp) {
                LOG_ERR("Could not create config queue %u", id);
                return NULL;
        }

        tmp->qos_id    = id;
        INIT_LIST_HEAD(&tmp->list);

        return tmp;
}

static void config_q_qos_destroy(struct config_q_qos * q)
{
        if (!q)
                return;

        rkfree(q);
}

static struct config_q_qos * config_q_qos_find(struct cher_urg_config * config,
                                               qos_id_t                 qos_id)
{
        struct config_q_qos * pos;

        list_for_each_entry(pos, &config->list_queues, list) {
                if (pos->qos_id == qos_id)
                        return pos;
        }

        return NULL;
}

static struct cher_urg_config * cherish_urgency_config_create(void)
{
        struct cher_urg_config * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
        if (!tmp) {
                LOG_ERR("Could not create config queue");
                return NULL;
        }
        INIT_LIST_HEAD(&tmp->list_queues);

        return tmp;
}

static void cherish_urgency_config_destroy(struct cher_urg_config * config)
{
        struct config_q_qos *pos, *next;

        if (!config)
                return;

        list_for_each_entry_safe(pos, next, &config->list_queues, list) {
                config_q_qos_destroy(pos);
        }

        return;
}

static struct cu_queue * queue_create(uint_t key)
{
        struct cu_queue * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
        if (!tmp) {
                LOG_ERR("No queue created for key %u", key);
                return NULL;
        }
        tmp->key = key;
        tmp->queue     = rfifo_create();
        if (!tmp->queue) {
                LOG_ERR("Couldn't create queue, ");
                rkfree(tmp);
                return NULL;
        }
        INIT_LIST_HEAD(&tmp->list);
        INIT_LIST_HEAD(&tmp->qos_id_list);

        return tmp;
}

static int queue_destroy(struct cu_queue * cu_q)
{
        struct q_qos *pos, *next;

        if (!cu_q)
                return -1;

        list_del(&cu_q->list);
        rfifo_destroy(cu_q->queue, (void (*)(void *)) pdu_destroy);
        list_for_each_entry_safe(pos, next, &cu_q->qos_id_list, list) {
                q_qos_destroy(pos);
        }

        rkfree(cu_q);

        return 0;
}

static int cu_queue_set_destroy(struct cu_queue_set * qset)
{
        struct cu_queue *pos, *next;

        if (!qset)
                return -1;

        list_del(&qset->list);
        list_for_each_entry_safe(pos, next, &qset->queues, list) {
                queue_destroy(pos);
        }
        rkfree(qset);

        return 0;
}

static struct cu_queue_set * cu_queue_set_create(port_id_t port_id,
						 struct robject * parent)
{
        struct cu_queue_set * tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);

        if (!tmp)
                return NULL;

        tmp->port_id = port_id;
        INIT_LIST_HEAD(&tmp->queues);
        INIT_LIST_HEAD(&tmp->list);
        tmp->occupation = 0;

        if (robject_init_and_add(&tmp->robj, &cher_urg_qset_rtype, parent, "qset")) {
                LOG_ERR("Failed to create Cherish-Urgency Queue Set sysfs entry");
                cu_queue_set_destroy(tmp);
                return NULL;
        }

        return tmp;
}

struct cherish_urgency_data {
        struct list_head         list_queues;
        struct cher_urg_config * config;
        unsigned long            start_time;
};

static struct cherish_urgency_data * cu_data_create(void)
{
        struct cherish_urgency_data * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
        if (!tmp) {
                LOG_ERR("Problems creating Cherish-Urgency data");
                return NULL;
        }

        INIT_LIST_HEAD(&tmp->list_queues);
        tmp->start_time = jiffies;

        return tmp;
}

static void cu_data_destroy(struct cherish_urgency_data * data)
{
        struct cu_queue_set *qset, *next;

        if (!data)
                return;

        cherish_urgency_config_destroy(data->config);
        list_for_each_entry_safe(qset, next, &data->list_queues, list) {
                cu_queue_set_destroy(qset);
        }

        rkfree(data);
}

static struct q_qos * q_qos_find(struct cu_queue * q, qos_id_t qos_id)
{
        struct q_qos * pos;

        list_for_each_entry(pos, &q->qos_id_list, list) {
                if (pos->qos_id == qos_id) {
                        return pos;
                }
        }

        return NULL;
}

static struct q_qos * cu_q_qos_find(struct cu_queue_set * qset,
                                    qos_id_t              qos_id)
{
        struct cu_queue * pos;

        list_for_each_entry(pos, &qset->queues, list) {
                struct q_qos * to_ret = q_qos_find(pos, qos_id);

                if (to_ret) {
                        return to_ret;
                }
        }

        return NULL;
}

static struct cu_queue * cu_queue_find_key(struct cu_queue_set * qset,
                                           uint_t                key)
{
        struct cu_queue * pos;

        list_for_each_entry(pos, &qset->queues, list) {
                if (pos->key == key)
                        return pos;
        }

        return NULL;
}

struct cu_queue_set * queue_set_find(struct cherish_urgency_data * q,
                                     port_id_t            port_id)
{
        struct cu_queue_set * entry;

        list_for_each_entry(entry, &q->list_queues, queues) {
                if (entry->port_id == port_id) {
                        return entry;
                }
        }

        return NULL;
}

static struct pdu * dequeue_mark_ecn_pdu(struct cu_queue     * entry,
				 	 struct cu_queue_set * qset)
{
        struct pdu * ret_pdu;
        struct q_qos *q;
        qos_id_t qos_id;
#if 0
        struct pci * pci;
        pdu_flags_t  pci_flags;
#endif

        ret_pdu = rfifo_pop(entry->queue);
        qset->occupation--;
        qos_id = pci_qos_id(pdu_pci_get_ro(ret_pdu));
        q      = q_qos_find(entry, qos_id);
        q->queued_pdus--;
#if 0
        if (!qqos->ecn_th) {
        	return ret_pdu;
        }
        if (qqos->ecn_th >= qset->occupation) {
        	return ret_pdu;
        }
        pci = pdu_pci_get_rw(ret_pdu);
        if (!pci) {
        	LOG_ERR("No PCI to mark in this PDU...");
		pdu_destroy(ret_pdu);
                return NULL;
        }
        pci_flags = pci_flags_get(pci);
        pci_flags_set(pci, pci_flags |= PDU_FLAGS_EXPLICIT_CONGESTION);
#endif
        return ret_pdu;
}

struct pdu * cu_rmt_dequeue_policy(struct rmt_ps      *ps,
				   struct rmt_n1_port *n1_port)
{
        struct cu_queue     *entry;
        struct pdu *          ret_pdu;
        struct cu_queue_set * qset;

        if (!ps || !n1_port || !n1_port->rmt_ps_queues) {
                LOG_ERR("Wrong input parameters for "
                        "rmt_next_scheduled_policy_tx");
                return NULL;
        }

        qset = n1_port->rmt_ps_queues;
        if (!qset)
                return NULL;

        list_for_each_entry(entry, &qset->queues, list) {
        	uint_t i = 0;
                get_random_bytes(&i, sizeof(i));
                i = i % NORM_PROB;
                if ((rfifo_length(entry->queue) <= 0) || entry->skip_prob >= i) {
                	continue;
                }
        	ret_pdu = dequeue_mark_ecn_pdu(entry, qset);
                return ret_pdu;
        }

        list_for_each_entry(entry, &qset->queues, list) {
                if (rfifo_length(entry->queue) <= 0) {
                	continue;
                }
        	ret_pdu = dequeue_mark_ecn_pdu(entry, qset);
                return ret_pdu;
        }
	LOG_ERR("FAILED DEQUEUEING");

        return NULL;
}

int cu_rmt_enqueue_policy(struct rmt_ps	  *ps,
			  struct rmt_n1_port *n1_port,
			  struct pdu	  *pdu)
{
        struct cu_queue_set * qset;
        struct q_qos *        q;
        int                   i;
        qos_id_t              qos_id;

        if (!pdu) {
        	LOG_ERR("Policy RMT enqueue called with no PDU");
                return RMT_PS_ENQ_ERR;
        }

        if (!ps || !n1_port) {
                LOG_ERR("Wrong input parameters for "
                        "rmt_enqueue_scheduling_policy_tx");
                pdu_destroy(pdu);
                return RMT_PS_ENQ_ERR;
        }

        qos_id = pci_qos_id(pdu_pci_get_ro(pdu));
        qset   = n1_port->rmt_ps_queues;
        q      = cu_q_qos_find(qset, qos_id);
        LOG_DBG("Enqueueing for QoS id %u", qos_id);
        if (!q) {
                LOG_ERR("No queue for QoS id %u, dropping PDU", qos_id);
                pdu_destroy(pdu);
                return RMT_PS_ENQ_ERR;
        }

        q->handled++;
        if (q->abs_th < (qset->occupation + 1)) {
                LOG_DBG("Dropped PDU: abs_th exceeded %ld", occupation);
                q->dropped++;
                pdu_destroy(pdu);
                return RMT_PS_ENQ_DROP;
        }
        get_random_bytes(&i, sizeof(i));
        i = i % NORM_PROB;
        if ((q->th < (qset->occupation + 1)) && (q->drop_prob > i)) {
        	LOG_DBG("Dropped PDU: th exceeded %ld", occupation);
                q->dropped++;
                pdu_destroy(pdu);
                return RMT_PS_ENQ_DROP;
        }

        if (rfifo_push_ni(q->cu_q->queue, pdu)) {
                LOG_ERR("Failed to enqueue");
                q->dropped++;
                pdu_destroy(pdu);
                return RMT_PS_ENQ_DROP;
        }
        LOG_DBG("PDU enqueued");
        qset->occupation++;
        q->queued_pdus++;

        return RMT_PS_ENQ_SCHED;
}

static void qset_add_queue(struct cu_queue *     queue,
                           struct cu_queue_set * qset)
{
        struct cu_queue * pos;

        if (list_empty(&qset->queues)) {
                list_add(&queue->list, &qset->queues);
                return;
        }

        list_for_each_entry(pos, &qset->queues, list) {
                if (pos->key > queue->key) {
                        list_add_tail(&queue->list, &pos->list);
                        break;
                }
        }

        /* Debug only */
        list_for_each_entry(pos, &qset->queues, list) {
                struct q_qos * qqos;

                LOG_INFO("Urgency class: %u", pos->key);
                list_for_each_entry(qqos, &pos->qos_id_list, list) {
                        LOG_INFO("Qos id: %u, Drop prob: %u",
                                 qqos->qos_id,
                                 qqos->drop_prob);
                }
        }

        return;
}

void * cu_rmt_q_create_policy(struct rmt_ps      *ps,
			      struct rmt_n1_port *n1_port)
{
        struct cherish_urgency_data *  data = ps->priv;
        struct cher_urg_config * config;
        struct config_q_qos * pos;
        struct cu_queue_set * tmp;
        struct cu_queue *     queue;
        struct q_qos *        q_qos;
        struct cu_queue     *entry;
        struct q_qos         *qqos;

        if (!ps || !n1_port || !data) {
                LOG_ERR("Wrong input parameters for "
                        "rmt_scheduling_create_policy_common");
                return NULL;
        }

        /* Create the n1_port queue group */
        /* Add it to the n1_port */
        tmp = cu_queue_set_create(n1_port->port_id, &n1_port->robj);
        if (!tmp) {
                LOG_ERR("No scheduling policy created for port-id %d",
                        n1_port->port_id);
                return NULL;
        }
        list_add(&tmp->list, &data->list_queues);
        config = data->config;

        list_for_each_entry(pos, &config->list_queues, list) {
                queue = cu_queue_find_key(tmp, pos->key);
                if (!queue) {
                        LOG_INFO("Created cu queue for class %u", pos->key);
                        queue = queue_create(pos->key);
                        if (!queue) {
                                cu_queue_set_destroy(tmp);
                                return NULL;
                        }
                        qset_add_queue(queue, tmp);
                        queue->skip_prob = pos->skip_prob;
                }
                q_qos = q_qos_create(pos->qos_id,
                                     pos->abs_th,
                                     pos->th,
                                     pos->drop_prob,
                                     pos->key,
                                     pos->skip_prob,
                                     pos->ecn_th,
                                     queue,
                                     &tmp->robj);
                if (!q_qos) {
                        cu_queue_set_destroy(tmp);
                        return NULL;
                }
                list_add(&q_qos->list, &queue->qos_id_list);
        }


        list_for_each_entry(entry, &tmp->queues, list) {
                LOG_INFO("Dequeuing from urgency class %u", entry->key);
                list_for_each_entry(qqos, &entry->qos_id_list, list) {
                	LOG_INFO("Added queue for QoS id %u", qqos->qos_id);
                }
        }

        return tmp;
}

int cu_rmt_q_destroy_policy(struct rmt_ps      *ps,
		   	    struct rmt_n1_port *n1_port)
{
        struct cherish_urgency_data *  data = ps->priv;
        struct cu_queue_set * tmp = n1_port->rmt_ps_queues;

        if (!tmp || !data)
                return -1;

        cu_queue_set_destroy(tmp);
        n1_port->rmt_ps_queues = NULL;

        return 0;
}

static int cher_urg_ps_set_policy_set_param_priv(struct cherish_urgency_data * data,
                                            	 const char *     name,
                                            	 const char *     value)
{
        struct config_q_qos * conf_qos;
        int int_value, ret, offset, dotlen;
        qos_id_t qos_id;
        char *dot, *buf;

        if (!name) {
                LOG_ERR("Null parameter name");
                return -1;
        }

        if (!value) {
                LOG_ERR("Null parameter value");
                return -1;
        }

        dot = strchr(name, '.');
        if (!dot) {
                LOG_ERR("No info enough");
                return -1;
        }

        offset = dotlen = dot - name;
        if (name[offset] == '.')
                offset++;

        buf = rkzalloc((dotlen + 1), GFP_ATOMIC);
        if (buf) {
                memcpy(buf, name, dotlen);
                buf[dotlen] = '\0';
        }

        ret = kstrtoint(buf, 10, &qos_id);
        LOG_DBG("Arrived parameter info for QoS id %u", qos_id);
        rkfree(buf);
        conf_qos = config_q_qos_find(data->config, qos_id);
        if (!conf_qos) {
                conf_qos = config_q_qos_create(qos_id);
                if (!conf_qos)
                        return -1;

                list_add(&conf_qos->list, &data->config->list_queues);
                LOG_DBG("Created queue for QoS id %u", qos_id);
        }

        if (strcmp(name + offset, "urgency-class") == 0) {
                ret = kstrtoint(value, 10, &int_value);
                if (!ret) {
                        LOG_DBG("Urgency class %u", int_value);
                        conf_qos->key = int_value;
                        return 0;
                }
                LOG_ERR("Could not parse urgency class for QoS %u", qos_id);
                return -1;
        }

        if (strcmp(name + offset, "skip-prob") == 0) {
                ret = kstrtoint(value, 10, &int_value);
                if (!ret) {
                	LOG_DBG("Skip probability %u", int_value);
                        conf_qos->skip_prob = int_value;
                        return 0;
                }
                LOG_ERR("Could not parse skip probability for QoS %u", qos_id);
                return -1;
        }

        if (strcmp(name + offset, "drop-prob") == 0) {
                ret = kstrtoint(value, 10, &int_value);
                if (!ret) {
                	LOG_DBG("Drop probability %u", int_value);
                        conf_qos->drop_prob = int_value;
                        return 0;
                }
                LOG_ERR("Could not parse drop probability for QoS %u", qos_id);
                return -1;
        }

        if (strcmp(name + offset, "abs-th") == 0) {
                ret = kstrtoint(value, 10, &int_value);
                if (!ret) {
                	LOG_DBG("Absolute threshold %u", int_value);
                        conf_qos->abs_th = int_value;
                        return 0;
                }
                LOG_ERR("Could not parse abs threshold for QoS %u", qos_id);
                return -1;
        }

        if (strcmp(name + offset, "th") == 0) {
                ret = kstrtoint(value, 10, &int_value);
                if (!ret) {
                	LOG_DBG("Threshold %u", int_value);
                        conf_qos->th = int_value;
                        return 0;
                }
                LOG_ERR("Could not parse threshold for QoS %u", qos_id);
                return -1;
        }

        if (strcmp(name + offset, "ecn_th") == 0) {
                ret = kstrtoint(value, 10, &int_value);
                if (!ret) {
                	LOG_DBG("ECN Threshold %u", int_value);
                        conf_qos->th = int_value;
                        return 0;
                }
                LOG_ERR("Could not parse threshold for QoS %u", qos_id);
                return -1;
        }

        LOG_ERR("No such parameter to set");

        return -1;
}

static int rmt_config_apply(struct policy_parm * param, void * data)
{
        struct cherish_urgency_data * tmp;

        tmp = (struct cherish_urgency_data *) data;

        return cher_urg_ps_set_policy_set_param_priv(data,
        					     policy_param_name(param),
        					     policy_param_value(param));
}

static int cher_urg_ps_set_policy_set_param(struct ps_base * bps,
                                            const char *     name,
                                            const char *     value)
{
        struct rmt_ps *ps = container_of(bps, struct rmt_ps, base);
        struct cherish_urgency_data * data = ps->priv;

        return cher_urg_ps_set_policy_set_param_priv(data, name, value);
}

static struct ps_base *
rmt_ps_cher_urg_create(struct rina_component * component)
{
        struct rmt * rmt = rmt_from_component(component);
        struct rmt_ps * ps = rkzalloc(sizeof(*ps), GFP_KERNEL);
        struct cherish_urgency_data * data;
        struct rmt_config * rmt_cfg;

        if (!ps)
                return NULL;

        data = cu_data_create();
        if (!data) {
                rkfree(ps);
                return NULL;
        }

        data->config = cherish_urgency_config_create();
        if (!data->config) {
                rkfree(ps);
                cu_data_destroy(data);
                return NULL;
        }

        ps->base.set_policy_set_param = cher_urg_ps_set_policy_set_param;
        ps->dm = rmt;
        ps->priv = data;

        rmt_cfg = rmt_config_get(rmt);
	if (rmt_cfg) {
		policy_for_each(rmt_cfg->policy_set, data, rmt_config_apply);
	} else {
		/* TODO provide a suitable default for all the parameters. */
		LOG_WARN("Missing defaults");
	}

        ps->rmt_dequeue_policy = cu_rmt_dequeue_policy;
        ps->rmt_enqueue_policy = cu_rmt_enqueue_policy;
        ps->rmt_q_create_policy = cu_rmt_q_create_policy;
        ps->rmt_q_destroy_policy = cu_rmt_q_destroy_policy;

        return &ps->base;
}

static void rmt_ps_cher_urg_destroy(struct ps_base * bps)
{
        struct rmt_ps *ps = container_of(bps, struct rmt_ps, base);

        if (bps) {
                if (ps && ps->priv)
                        cu_data_destroy(ps->priv);
        }
}

static struct ps_factory ch_ur_factory = {
        .owner   = THIS_MODULE,
        .create  = rmt_ps_cher_urg_create,
        .destroy = rmt_ps_cher_urg_destroy,
};

static int __init mod_init(void)
{
        int ret;

        strcpy(ch_ur_factory.name, RINA_CHERISH_URGENCY_PS_NAME);

        ret = rmt_ps_publish(&ch_ur_factory);
        if (ret) {
                LOG_ERR("Failed to publish policy set factory");
                return -1;
        }

        LOG_INFO("RMT Cherish-Urgency policy set loaded successfully");

        return 0;
}

static void __exit mod_exit(void)
{
        int ret = rmt_ps_unpublish(RINA_CHERISH_URGENCY_PS_NAME);

        if (ret) {
                LOG_ERR("Failed to unpublish policy set factory");
                return;
        }

        LOG_INFO("RMT Cherish-Urgency policy set unloaded successfully");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RMT Cherish-Urgency policy set");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Vincenzo Maffione <v.maffione@nextworks.it>");
MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
MODULE_AUTHOR("Miquel Tarzan <miquel.tarzan@i2cat.net>");
