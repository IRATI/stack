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

#include <linux/kobject.h>
#include <linux/hashtable.h>
#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/random.h>
#include <linux/string.h>
#include <linux/spinlock.h>
#include <linux/version.h>

#define RINA_PREFIX "rmt-ps-ecn"

#include <logs.h>
#include <rds/rmem.h>
#include <rmt-ps.h>
#include <rmt.h>
#include <pci.h>
#include <policies.h>

/* Policy name. */
#define ECN_PS_NAME "ecn"

/* Timespec to ms. */
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,5,0)
static inline unsigned long ecn_to_ms(struct timespec * t) {
#else
static inline unsigned long ecn_to_ms(struct timespec64 *t) {
#endif
	return (t->tv_sec * 1000) + (t->tv_nsec / 1000000);
}

#define ECN_THRE			35

/* Threshold assigned to the RMT queue. */
static unsigned int ecn_thre = ECN_THRE;

/*
 * Other definitions.
 */

#define DEFAULT_Q_MAX 1000

#define rmap_hash(T, K) hash_min(K, HASH_BITS(T))

#define ECN_P_GRANULARITY 		10

struct rmt_queue_set {
	DECLARE_HASHTABLE(queues, RMT_PS_HASHSIZE);
};

struct rmt_ps_data {
	struct rmt_queue_set *outqs;
	/* Max size for queues. */
	unsigned int q_max;
	/* Threshold after that we will begin to mark packets. */
	int thre;
};

struct rmt_queue {
	struct kobject qobj;
	struct rfifo * queue;
	struct rfifo * mgmt;
	port_id_t pid;
	struct hlist_node hlist;
};

/*
 * Kobject defs (taken from linux/samples/kobject)
 */

/* Kernel object for the policy. */
struct kobject ecn_obj;

/* Attribute used in ECN policy. */
struct ecn_attribute {
	struct attribute attr;
	ssize_t (* show)(
		struct kobject * kobj,
		struct attribute * attr,
		char * buf);
	ssize_t (* store)(
		struct kobject * foo,
		struct attribute * attr,
		const char * buf,
		size_t count);
};

/* Show procedure! */
static ssize_t ecn_attr_show(
	struct kobject * kobj,
	struct attribute * attr,
	char * buf) {

	struct rmt_queue * rmtq = 0;

	if(strcmp(attr->name, "queue_size") == 0) {
		rmtq = container_of(kobj, struct rmt_queue, qobj);
		return snprintf(
			buf, PAGE_SIZE, "%zd\n", rfifo_length(rmtq->queue));
	}

	if(strcmp(attr->name, "mgmt_size") == 0) {
		rmtq = container_of(kobj, struct rmt_queue, qobj);
		return snprintf(
			buf, PAGE_SIZE, "%zd\n", rfifo_length(rmtq->mgmt));
	}

	if(strcmp(attr->name, "threshold") == 0) {
		rmtq = container_of(kobj, struct rmt_queue, qobj);
		return snprintf(buf, PAGE_SIZE, "%u\n", ecn_thre);
	}

	if(strcmp(attr->name, "max_size") == 0) {
		rmtq = container_of(kobj, struct rmt_queue, qobj);
		return snprintf(buf, PAGE_SIZE, "%u\n", DEFAULT_Q_MAX);
	}

	return 0;
}

/* Store procedure! */
static ssize_t ecn_attr_store(
	struct kobject * kobj,
	struct attribute * attr,
	const char * buf,
	size_t count) {

	int ft = 0;
	int op = 0;

	if(strcmp(attr->name, "threshold") == 0) {
		op = kstrtoint(buf, 10, &ft);

		if(op < 0) {
			LOG_ERR("Failed to set the reset rate.");
			return op;
		}

		/* Single flow rate is now changed! */
		ecn_thre = ft;
	}

	return count;
}

/* Sysfs operations. */
static const struct sysfs_ops ecn_sysfs_ops = {
	.show  = ecn_attr_show,
	.store = ecn_attr_store,
};

/*
 * Attributes visible in ECN sysfs directory.
 */

static struct ecn_attribute ecn_qs_attr =
	__ATTR(queue_size, 0444, ecn_attr_show, ecn_attr_store);

static struct ecn_attribute ecn_gs_attr =
	__ATTR(mgmt_size, 0444, ecn_attr_show, ecn_attr_store);

static struct ecn_attribute ecn_t_attr =
	__ATTR(threshold, 0664, ecn_attr_show, ecn_attr_store);

static struct ecn_attribute ecn_ms_attr =
	__ATTR(max_size, 0444, ecn_attr_show, ecn_attr_store);

static struct attribute * ecn_attrs[] = {
	&ecn_t_attr.attr,
	NULL,
};

static struct attribute * ecn_queue_attrs[] = {
	&ecn_qs_attr.attr,
	&ecn_gs_attr.attr,
	&ecn_ms_attr.attr,
	NULL,
};

/* Operations associated with the release of the kobj. */
static void ecn_release(struct kobject *kobj) {
	/* Nothing... */
}

/* Master ECN ktype, different for the type of shown attributes. */
static struct kobj_type ecn_ktype = {
	.sysfs_ops = &ecn_sysfs_ops,
	.release = ecn_release,
	.default_attrs = ecn_attrs,
};

/* Queues ECN ktype, different for the type of shown attributes. */
static struct kobj_type ecn_queue_ktype = {
	.sysfs_ops = &ecn_sysfs_ops,
	.release = ecn_release,
	.default_attrs = ecn_queue_attrs,
};

/*
 * Procedures:
 */

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

	tmp->mgmt = rfifo_create_ni();

	if (!tmp->mgmt) {
		rfifo_destroy(tmp->queue, (void (*)(void *)) du_destroy);
		rkfree(tmp);
		return NULL;
	}

	tmp->pid = port;
	INIT_HLIST_NODE(&tmp->hlist);

	/* Don't care of error code... */
	if(kobject_init_and_add(
		&tmp->qobj, &ecn_queue_ktype, &ecn_obj, "%d", port)) {

		LOG_ERR("Cannot create ecn sysfs object for %d", port);
	}

	return tmp;
}

static int ecn_queue_destroy(struct rmt_queue *q) {
	if (!q) {
		LOG_ERR("No RMT Key-queue to destroy...");
		return -1;
	}

	/* Block any logging... */
	flush_scheduled_work();

	if (q->queue) {
		rfifo_destroy(q->queue, (void (*)(void *)) du_destroy);
	}

	if (q->mgmt) {
		rfifo_destroy(q->mgmt, (void (*)(void *)) du_destroy);
	}

	kobject_put(&q->qobj);

	rkfree(q);

	return 0;
}

static void * ecn_create_q(
	struct rmt_ps      * ps,
	struct rmt_n1_port * n1_port) {

	struct rmt_queue *queue;
	struct rmt_ps_data *data;

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

	LOG_DBG("Structures for scheduling policies created...");
	return queue;
}

static int ecn_destroy_q(
	struct rmt_ps      *ps,
	struct rmt_n1_port *n1_port) {

	struct rmt_queue *queue;
	struct rmt_ps_data *data;

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

	ecn_queue_destroy(queue);

	return 0;
}

int ecn_enqueue(struct rmt_ps * ps,
		struct rmt_n1_port * n1_port,
		struct du * du,
		bool must_enqueue)
{
	struct rmt_queue *q;
	struct rmt_ps_data * data = ps->priv;

	ssize_t c = 0;

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

	c = rfifo_length(q->queue);

	/* NOTE: This is a workaround... */
	if(pci_type(&du->pci) == PDU_TYPE_MGMT) {
		if (!must_enqueue && rfifo_is_empty(q->mgmt))
			return RMT_PS_ENQ_SEND;

		rfifo_push_ni(q->mgmt, du);
		return RMT_PS_ENQ_SCHED;
	}

	if (c >= data->q_max) {
		du_destroy(du);
		return RMT_PS_ENQ_DROP;
	}

	/* Really simple behavior here: just mark a packet if the threshold is
	 * overcame. Mark directly the pdu which was present during the
	 * congestion.
	 */
	if(c > data->thre) {
		pci_flags_set(&du->pci, pci_flags_get(&du->pci) |
				PDU_FLAGS_EXPLICIT_CONGESTION);
	}

	if (!must_enqueue && rfifo_is_empty(q->queue))
		return RMT_PS_ENQ_SEND;

	rfifo_push_ni(q->queue, du);
	return RMT_PS_ENQ_SCHED;
}

struct du * ecn_dequeue(
	struct rmt_ps *ps,
	struct rmt_n1_port *n1_port) {

	struct rmt_queue * q;
	struct du * ret_pdu;

	if (!ps || !n1_port) {
		LOG_ERR("Wrong input parameters");
		return NULL;
	}

	/* NOTE: The policy is called with the n1_port lock taken */
	q = n1_port->rmt_ps_queues;

	if (!q) {
		LOG_ERR("Could not find queue for n1_port %u",
			n1_port->port_id);
		return NULL;
	}

	/* Priority to the mgmt queue. */
	if(rfifo_length(q->mgmt) > 0) {
		ret_pdu = rfifo_pop(q->mgmt);
	} else {
		ret_pdu = rfifo_pop(q->queue);
	}

	if (!ret_pdu) {
		LOG_ERR("Could not dequeue scheduled pdu");
		return NULL;
	}

	return ret_pdu;
}

static int ecn_set_policy_set_param(
	struct ps_base *bps,
	const char *name,
	const char *value) {

	struct rmt_ps *ps = container_of(bps, struct rmt_ps, base);
	struct rmt_ps_data *data = ps->priv;
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
	struct rmt_ps_data *data;
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

	return &ps->base;
}

static void ecn_destroy(struct ps_base *bps)
{
	struct rmt_ps *ps;
	struct rmt_ps_data *data;

	ps = container_of(bps, struct rmt_ps, base);
	data = ps->priv;

	if (bps) {
		if (data) {
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
		LOG_ERR("Failed to publish the RDSR RMT policy.");
		return ret;
	}

	if(kobject_init_and_add(&ecn_obj, &ecn_ktype, 0, "ecn")) {
		LOG_ERR("Sysfs failed!");
		return 0;
	}

	LOG_INFO("RMT policy successfully published.");

	return 0;
}

/* Module exit point.
 */
static void __exit ecn_module_exit(void) {

	int ret = rmt_ps_unpublish(ECN_PS_NAME);

	if (ret) {
		LOG_ERR("Failed to unpublish %s policy, error %d.", ECN_PS_NAME,
		                ret);

		return;
	}

	LOG_INFO("Policy %s successfully unpublished.", ECN_PS_NAME);
	kobject_put(&ecn_obj);

	return;
}

module_init(ecn_module_init);
module_exit(ecn_module_exit);

MODULE_AUTHOR("Kewin Rausch <kewin.rausch@create-net.org>");
MODULE_DESCRIPTION("RMT policy for Explicit Congestion Notification.");
MODULE_LICENSE("GPL");
MODULE_VERSION("1");
