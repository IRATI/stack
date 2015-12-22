/* This program is free software; you can redistribute it and/or modify
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
 *
 * Author: Kewin Rausch <kewin.rausch@create-net.org>
 *
 * This file is 80 character wide.
 * Use monospace font for better visualization.
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *
 */

#include <linux/hashtable.h>
#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/random.h>
#include <linux/string.h>
#include <linux/spinlock.h>

#define RINA_PREFIX "rmt-ps-ecn"

#include <logs.h>
#include <rds/rmem.h>
#include <rmt-ps.h>
#include <rmt.h>
#include <pci.h>
#include <policies.h>

/* Policy name. */
#define ECN_PS_NAME "ecn"

/* Change this to give disable/change log levels.
 * Redefine to obtain a completely silent module, or to specialize more the
 * output of the log.
 */
#define ECN_LOG(x, ARGS...)		LOG_INFO(" ECNL "x, ##ARGS)

/* Use a timed log in order to avoid to flood the message mechanism.
 */
#define ECN_TIMED_LOG

#ifdef ECN_TIMED_LOG

/* Ms to report. */
#define ECN_TL_MS	1000

/* Timespec to ms. */
static inline unsigned long ecn_to_ms(struct timespec * t) {
	return (t->tv_sec * 1000) + (t->tv_nsec / 1000000);
}

/* Last report time. */
static struct timespec ecn_tl_last = {0, 0};
/* Port creation time. */
static struct timespec ecn_tl_start = {0, 0};
/* Something has been dropped? */
static int ecn_tl_drop = 0;

#endif /* ECN_TIMED_LOG */

#define ECN_THRE			5

/* Threshold assigned to the RMT queue. */
static unsigned int ecn_thre = ECN_THRE;

#define DEFAULT_Q_MAX 1000

#define rmap_hash(T, K) hash_min(K, HASH_BITS(T))

#define ECN_P_GRANULARITY 		10

struct rmt_queue {
	struct rfifo * queue;
	port_id_t pid;
	struct hlist_node hlist;
};

struct rmt_queue_set {
	DECLARE_HASHTABLE(queues, RMT_PS_HASHSIZE);
};

struct rmt_ps_default_data {
	struct rmt_queue_set *outqs;
	/* Max size for queues. */
	unsigned int q_max;
	/* Threshold after that we will begin to mark packets. */
	int thre;
};

static struct rmt_queue * ecn_queue_create(port_id_t port) {
	struct rmt_queue *tmp;

	tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);

	if (!tmp) {
		return NULL;
	}

	tmp->queue = rfifo_create_ni();

	if (!tmp->queue) {
		rkfree(tmp);
		return NULL;
	}

	tmp->pid = port;
	INIT_HLIST_NODE(&tmp->hlist);

	return tmp;
}

static int ecn_queue_destroy(struct rmt_queue *q) {
	if (!q) {
		LOG_ERR("No RMT Key-queue to destroy...");
		return -1;
	}

	hash_del(&q->hlist);

	if (q->queue) {
		rfifo_destroy(q->queue, (void (*)(void *)) pdu_destroy);
	}

	rkfree(q);

	return 0;
}

static struct rmt_queue * ecn_queue_find(
	struct rmt_queue_set *qs,
	port_id_t port) {

	struct rmt_queue *entry;
	const struct hlist_head *head;

	head = &qs->queues[rmap_hash(qs->queues, port)];

	hlist_for_each_entry(entry, head, hlist) {
		if (entry->pid == port) {
			return entry;
		}
	}

	return NULL;
}

static struct rmt_queue_set * ecn_queue_set_create(void) {
	struct rmt_queue_set *tmp;

	tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);

	if (!tmp) {
		return NULL;
	}

	hash_init(tmp->queues);

	return tmp;
}

static int ecn_queue_set_destroy(struct rmt_queue_set *qs) {
	struct rmt_queue  *entry;
	struct hlist_node *tmp;
	int bucket;

	hash_for_each_safe(qs->queues, bucket, tmp, entry, hlist) {
		if (ecn_queue_destroy(entry)) {
			LOG_ERR("Could not destroy entry %pK", entry);
			return -1;
		}
	}

	rkfree(qs);

	return 0;
}

static void * ecn_create_q(
	struct rmt_ps      * ps,
	struct rmt_n1_port * n1_port) {

	struct rmt_queue *queue;
	struct rmt_ps_default_data *data;

	if (!ps || !n1_port || !ps->priv) {
		LOG_ERR("Wrong input parameters");
		return NULL;
	}

	data = ps->priv;

	queue = ecn_queue_create(n1_port->port_id);

	if (!queue) {
		LOG_ERR("Could not create queue for n1_port %u",
			n1_port->port_id);
		return NULL;
	}

	hash_add(data->outqs->queues, &queue->hlist, n1_port->port_id);

	LOG_DBG("Structures for scheduling policies created...");
	return queue;
}

static int ecn_destroy_q(
	struct rmt_ps      *ps,
	struct rmt_n1_port *n1_port) {

	struct rmt_queue *queue;
	struct rmt_ps_default_data *data;

	if (!ps || !n1_port || !ps->priv) {
		LOG_ERR("Wrong input parameters");
		return -1;
	}

	data = ps->priv;

	queue = ecn_queue_find(data->outqs, n1_port->port_id);

	if (!queue) {
		LOG_ERR("Could not find queue for n1_port %u",
			n1_port->port_id);
		return -1;
	}

	hash_del(&queue->hlist);
	ecn_queue_destroy(queue);

	return 0;
}

int ecn_enqueue(
	struct rmt_ps * ps,
	struct rmt_n1_port * n1_port,
	struct pdu * pdu) {

	struct rmt_queue *q;
	struct rmt_ps_default_data *data = ps->priv;

	if (!ps || !n1_port || !pdu) {
		LOG_ERR("Wrong input parameters");
		return RMT_PS_ENQ_ERR;
	}

	q = ecn_queue_find(data->outqs, n1_port->port_id);

	if (!q) {
		LOG_ERR("Could not find queue for n1_port %u",
			n1_port->port_id);
		pdu_destroy(pdu);
		return RMT_PS_ENQ_ERR;
	}

	if (rfifo_length(q->queue) >= data->q_max) {
		if(pci_type(pdu_pci_get_ro(pdu)) != PDU_TYPE_MGMT) {
#ifdef ECN_TIMED_LOG
			/* Dropping detected! */
			ecn_tl_drop = 1;
#endif
			pdu_destroy(pdu);
			return RMT_PS_ENQ_DROP;
		}
	}

	rfifo_push_ni(q->queue, pdu);
	return RMT_PS_ENQ_SCHED;
}

struct pdu * ecn_dequeue(
	struct rmt_ps *ps,
	struct rmt_n1_port *n1_port) {

	struct rmt_queue * q;
	struct rmt_ps_default_data * data = ps->priv;
	struct pdu * ret_pdu;
	struct pci * pci;

	ssize_t c;      /* Current level. */

#ifdef ECN_TIMED_LOG
	struct timespec now;
#endif

	if (!ps || !n1_port) {
		LOG_ERR("Wrong input parameters");
		return NULL;
	}

	/* NOTE: The policy is called with the n1_port lock taken */
	q = ecn_queue_find(data->outqs, n1_port->port_id);

	if (!q) {
		LOG_ERR("Could not find queue for n1_port %u",
			n1_port->port_id);
		return NULL;
	}

	ret_pdu = rfifo_pop(q->queue);

	if (!ret_pdu) {
		LOG_ERR("Could not dequeue scheduled pdu");
		return NULL;
	}

	c = rfifo_length(q->queue);

	/* Really simple behavior here: just mark a packet if the threshold is
	 * overcame.
	 */
	if(c > data->thre) {
		pci = pdu_pci_get_rw(ret_pdu);
		pci_flags_set(
			pci, pci_flags_get(pci) |
			PDU_FLAGS_EXPLICIT_CONGESTION);
	}

#ifdef ECN_TIMED_LOG
	getnstimeofday(&now);

	/* Time to log. */
	if(ecn_to_ms(&now) - ecn_to_ms(&ecn_tl_last) > ECN_TL_MS){
		printk("#>%u, %lu.%lu, %zd, %u, %d\n",
			n1_port->port_id,
			(ecn_to_ms(&now) - ecn_to_ms(&ecn_tl_start)) / 1000,
			ecn_to_ms(&now) - ecn_to_ms(&ecn_tl_start),
			c,
			data->q_max,
			ecn_tl_drop);

		ecn_tl_last.tv_sec  = now.tv_sec;
		ecn_tl_last.tv_nsec = now.tv_nsec;
		ecn_tl_drop = 0;
	}
#endif

	return ret_pdu;
}

static int ecn_set_policy_set_param(
	struct ps_base *bps,
	const char *name,
	const char *value) {

	struct rmt_ps *ps = container_of(bps, struct rmt_ps, base);
	struct rmt_ps_default_data *data = ps->priv;
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

	if (strcmp(name, "q_max") == 0) {
		ret = kstrtoint(value, 10, &ival);
		if (!ret) {
			data->q_max = ival;
		}
	}

	/* Change the threshold value. */
	if (strcmp(name, "q_thre") == 0) {
		ret = kstrtoint(value, 10, &ival);
		if (!ret) {
			data->thre = ival;
		}
	}

	return 0;
}

static struct ps_base * ecn_create(struct rina_component * component) {
	struct rmt *rmt;
	struct rmt_ps *ps;
	struct rmt_ps_default_data *data;
	struct rmt_config *rmt_cfg;
	struct policy_parm *parm = NULL;

	rmt = rmt_from_component(component);
	if(!rmt)
		return NULL;

	ps = rkmalloc(sizeof(*ps), GFP_KERNEL);
	if (!ps)
		return NULL;

	data = rkmalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		rkfree(ps);
		return NULL;
	}

	/* Allocate policy-set private data. */
	data->outqs = ecn_queue_set_create();

	if (!data->outqs) {
		rkfree(ps);
		rkfree(data);
		return NULL;
	}

	ps->base.set_policy_set_param = ecn_set_policy_set_param;
	ps->dm = rmt;
	ps->priv = data;

	rmt_cfg = rmt_config_get(rmt);
	if (rmt_cfg) {
		/* This is available at assign-to-dif time, but not at
		 * select-policy-set time. */
		parm = policy_param_find(rmt_cfg->policy_set, "q_max");
	}

	if (!parm) {
		LOG_WARN("No PS param q_max");
		data->q_max = DEFAULT_Q_MAX;
	} else {
		ecn_set_policy_set_param(&ps->base,
			policy_param_name(parm),
			policy_param_value(parm));
	}

	/* Start as fast as possible to signal the congestion. */
	data->thre = ecn_thre;

	ps->rmt_dequeue_policy = ecn_dequeue;
	ps->rmt_enqueue_policy = ecn_enqueue;
	ps->rmt_q_create_policy = ecn_create_q;
	ps->rmt_q_destroy_policy = ecn_destroy_q;

#ifdef ECN_TIMED_LOG
	getnstimeofday(&ecn_tl_start);
	ecn_tl_last.tv_sec = ecn_tl_start.tv_sec;
	ecn_tl_last.tv_nsec = ecn_tl_start.tv_nsec;
#endif /* ECN_TIMED_LOG */

	return &ps->base;
}

static void ecn_destroy(struct ps_base *bps)
{
	struct rmt_ps *ps;
	struct rmt_ps_default_data *data;

	ps = container_of(bps, struct rmt_ps, base);
	data = ps->priv;

	if (bps) {
		if (data) {
			if (data->outqs) {
				ecn_queue_set_destroy(data->outqs);
			}
			rkfree(data);
		}
		rkfree(ps);
	}
}


/* Policy structure containing the hooks to handle the policy.
 */
struct ps_factory ecn_rmt_factory = {
	.owner = THIS_MODULE,
	.create = ecn_create,
	.destroy = ecn_destroy,
};

/* Module entry point.
 */
static int __init ecn_module_init(void) {

	int ret = 0;

	/* Give a name to the policy. */
	strcpy(ecn_rmt_factory.name, ECN_PS_NAME);

	/* Publish the policy in the RMT module. */
	ret = rmt_ps_publish(&ecn_rmt_factory);

	/* Check for errors. */
	if (ret) {
		ECN_LOG("Failed to publish the RDSR RMT policy.");
		return ret;
	}

	ECN_LOG("RMT policy successfully published.");

	return 0;
}

/* Module exit point.
 */
static void __exit ecn_module_exit(void) {

	int ret = rmt_ps_unpublish(ECN_PS_NAME);

	if (ret) {
		ECN_LOG("Failed to unpublish %s policy, error %d.", ECN_PS_NAME,
		                ret);

		return;
	}

	ECN_LOG("Policy %s successfully unpublished.", ECN_PS_NAME);

	return;
}

module_init(ecn_module_init);
module_exit(ecn_module_exit);

MODULE_AUTHOR("Kewin Rausch <kewin.rausch@create-net.org>");
MODULE_DESCRIPTION("RMT policy for Explicit Congestion Notification.");
MODULE_LICENSE("GPL");
MODULE_VERSION("1");
