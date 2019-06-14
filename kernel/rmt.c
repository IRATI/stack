/*
 * RMT (Relaying and Multiplexing Task)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
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
#include <linux/types.h>
#include <linux/hashtable.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/string.h>
/* FIXME: to be re-removed after removing tasklets */
#include <linux/interrupt.h>

#define RINA_PREFIX "rmt"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "efcp-str.h"
#include "rmt.h"
#include "pff.h"
#include "efcp-utils.h"
#include "rmt-ps.h"
#include "policies.h"
#include "rds/rstr.h"
#include "ipcp-instances.h"
#include "ipcp-utils.h"
#include "du.h"
#include "rmt-ps-default.h"

#define rmap_hash(T, K) hash_min(K, HASH_BITS(T))
#define MAX_PDUS_SENT_PER_CYCLE 10

static struct policy_set_list policy_sets = {
	.head = LIST_HEAD_INIT(policy_sets.head)
};

struct pff_cache {
	/* Array of port_id_t */
	port_id_t *pids;
	/* Entries in the pids array */
	size_t count;
};

struct rmt_address {
        address_t	 address;
        struct list_head list;
};

struct rmt {
	struct rina_component base;
	spinlock_t	      lock;
	struct list_head      addresses;
	struct ipcp_instance *parent;
	struct pff *pff;
	struct kfa *kfa;
	struct efcp_container *efcpc;
	struct tasklet_struct egress_tasklet;
	struct n1pmap *n1_ports;
	struct pff_cache cache;
	struct rmt_config *rmt_cfg;
	struct sdup *sdup;
	struct robject robj;
};

#define stats_get(name, n1_port, retval)				\
        spin_lock_bh(&n1_port->lock);					\
        retval = n1_port->stats.name;					\
        spin_unlock_bh(&n1_port->lock);

#define stats_inc(name, n1_port, bytes)					\
        n1_port->stats.name##_pdus++;					\
	n1_port->stats.name##_bytes += (unsigned int) bytes;		\

static ssize_t rmt_attr_show(struct robject *        robj,
                             struct robj_attribute * attr,
                             char *                  buf)
{
	struct rmt * rmt;

	rmt = container_of(robj, struct rmt, robj);
	if (!rmt || !rmt->base.ps)
		return 0;

	if (strcmp(robject_attr_name(attr), "ps_name") == 0) {
		return sprintf(buf, "%s\n", rmt->base.ps_factory->name);
	}
	return 0;
}

static ssize_t rmt_n1_port_attr_show(struct robject *        robj,
                         	     struct robj_attribute * attr,
                                     char *                  buf)
{
	struct rmt_n1_port * n1_port;
	unsigned int stats_ret;
	bool wbusy;
	enum flow_state state;

	n1_port = container_of(robj, struct rmt_n1_port, robj);
	if (!n1_port)
		return 0;

	if (strcmp(robject_attr_name(attr), "queued_pdus") == 0) {
		stats_get(plen, n1_port, stats_ret);
		return sprintf(buf, "%u\n", stats_ret);
	}
	if (strcmp(robject_attr_name(attr), "drop_pdus") == 0) {
		stats_get(drop_pdus, n1_port, stats_ret);
		return sprintf(buf, "%u\n", stats_ret);
	}
	if (strcmp(robject_attr_name(attr), "err_pdus") == 0) {
		stats_get(err_pdus, n1_port, stats_ret);
		return sprintf(buf, "%u\n", stats_ret);
	}
	if (strcmp(robject_attr_name(attr), "tx_pdus") == 0) {
		stats_get(tx_pdus, n1_port, stats_ret);
		return sprintf(buf, "%u\n", stats_ret);
	}
	if (strcmp(robject_attr_name(attr), "rx_pdus") == 0) {
		stats_get(rx_pdus, n1_port, stats_ret);
		return sprintf(buf, "%u\n", stats_ret);
	}
	if (strcmp(robject_attr_name(attr), "tx_bytes") == 0) {
		stats_get(tx_bytes, n1_port, stats_ret);
		return sprintf(buf, "%u\n", stats_ret);
	}
	if (strcmp(robject_attr_name(attr), "rx_bytes") == 0) {
		stats_get(rx_bytes, n1_port, stats_ret);
		return sprintf(buf, "%u\n", stats_ret);
	}
	if (strcmp(robject_attr_name(attr), "wbusy") == 0) {
		spin_lock_bh(&n1_port->lock);
		wbusy = n1_port->wbusy;
		spin_unlock_bh(&n1_port->lock);
		return sprintf(buf, "%s\n", wbusy?"true":"false");
	}
	if (strcmp(robject_attr_name(attr), "state") == 0) {
		spin_lock_bh(&n1_port->lock);
		state = n1_port->state;
		spin_unlock_bh(&n1_port->lock);
		return sprintf(buf, "%d\n", (int) state);
	}
	return 0;
}
RINA_SYSFS_OPS(rmt);
RINA_ATTRS(rmt, ps_name);
RINA_KTYPE(rmt);
RINA_SYSFS_OPS(rmt_n1_port);
RINA_ATTRS(rmt_n1_port, queued_pdus, drop_pdus, err_pdus, tx_pdus,
	   tx_bytes, rx_pdus, rx_bytes, wbusy, state);
RINA_KTYPE(rmt_n1_port);

static struct rmt_n1_port *n1_port_create(port_id_t id,
					  struct ipcp_instance *n1_ipcp)
{
	struct rmt_n1_port *tmp;

	ASSERT(is_port_id_ok(id));

	tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
	if (!tmp)
		return NULL;

	robject_init(&tmp->robj, &rmt_n1_port_rtype);
	INIT_HLIST_NODE(&tmp->hlist);

	tmp->port_id = id;
	tmp->n1_ipcp = n1_ipcp;
	tmp->state   = N1_PORT_STATE_ENABLED;

	atomic_set(&tmp->refs_c, 0);
	tmp->wbusy = false;
	tmp->stats.plen = 0;
	tmp->stats.drop_pdus = 0;
	tmp->stats.err_pdus = 0;
	tmp->stats.tx_pdus = 0;
	tmp->stats.tx_bytes = 0;
	tmp->stats.rx_pdus = 0;
	tmp->stats.rx_bytes = 0;
	tmp->sdup_port = 0;
	spin_lock_init(&tmp->lock);

	LOG_DBG("N-1 port %pK created successfully (port-id = %d)", tmp, id);

	return tmp;
}

static int n1_port_user_ipcp_unbind(struct rmt_n1_port *n1p)
{
	struct ipcp_instance *n1_ipcp;

	ASSERT(n1p);

	n1_ipcp = n1p->n1_ipcp;
	if (n1_ipcp && n1_ipcp->ops && n1_ipcp->data &&
	    n1_ipcp->ops->flow_unbinding_user_ipcp)
		if (n1_ipcp->ops->flow_unbinding_user_ipcp(n1_ipcp->data,
							   n1p->port_id))
			LOG_ERR("Could not unbind IPCP as user of an N-1 flow");
			return -1;

	return 0;
}

static int n1_port_destroy(struct rmt_n1_port *n1p)
{
	ASSERT(n1p);
	LOG_DBG("Destroying N-1 port %pK (port-id = %d)", n1p, n1p->port_id);

	hash_del(&n1p->hlist);

	robject_del(&n1p->robj);

	if (n1p->sdup_port)
		sdup_destroy_port_config(n1p->sdup_port);

	if (n1p->pending_du)
		du_destroy(n1p->pending_du);

	if (n1p->wbusy)
		LOG_WARN("Deleting n1_port with bussy writer... there may be something wrong...");

	rkfree(n1p);

	return 0;
}

static int n1_port_cleanup(struct rmt *instance,
			   struct rmt_n1_port *n1_port)
{
	struct rmt_ps *ps;

	ASSERT(instance);

	rcu_read_lock();
	ps = container_of(rcu_dereference(instance->base.ps),
			  struct rmt_ps, base);
	if (ps && ps->rmt_q_destroy_policy)
		ps->rmt_q_destroy_policy(ps, n1_port);
	rcu_read_unlock();

	n1_port_destroy(n1_port);

	return 0;
}

struct n1pmap {
	spinlock_t lock;
	struct rset *rset;
	DECLARE_HASHTABLE(n1_ports, 7);
};

static int n1pmap_destroy(struct rmt *instance)
{
	struct rmt_n1_port *entry;
	struct hlist_node *tmp;
	int bucket;
	struct n1pmap *m;

	ASSERT(instance);

	m = instance->n1_ports;
	if (!m)
		return 0;

	spin_lock_bh(&m->lock);
	hash_for_each_safe(m->n1_ports, bucket, tmp, entry, hlist) {
		ASSERT(entry);
		if (n1_port_user_ipcp_unbind(entry))
			LOG_ERR("Could not destroy entry %pK", entry);

		if (n1_port_cleanup(instance, entry))
			LOG_ERR("Could not destroy entry %pK", entry);
	}
	spin_unlock_bh(&m->lock);

	if (m->rset)
		rset_unregister(m->rset);

	rkfree(m);
	return 0;
}

static struct n1pmap *n1pmap_create(struct robject *parent)
{
	struct n1pmap *tmp;

	tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
	if (!tmp)
		return NULL;

	tmp->rset = NULL;

	hash_init(tmp->n1_ports);
	spin_lock_init(&tmp->lock);

	tmp->rset = rset_create_and_add("n1_ports", parent);
	if (!tmp->rset) {
                LOG_ERR("Failed to create n1_ports sysfs entry");
                rkfree(tmp);
                return NULL;
	}

	return tmp;
}

#define n1_port_lock(port)	\
	spin_lock_bh(&port->lock)

#define n1_port_unlock(port)	\
	spin_unlock_bh(&port->lock)

#define n1_port_unlock_release(port)	\
	atomic_dec(&port->refs_c);		\
	n1_port_unlock(port)

static void n1pmap_release(struct rmt *instance,
			   struct rmt_n1_port *n1_port)
{
	struct n1pmap *m;

	ASSERT(instance);

	m = instance->n1_ports;
	if (!m)
		return;

	n1_port_lock(n1_port);
	if (atomic_dec_and_test(&n1_port->refs_c) &&
	    n1_port->state == N1_PORT_STATE_DEALLOCATED) {
		n1_port_unlock(n1_port);
		spin_lock_bh(&m->lock);
		n1_port_cleanup(instance, n1_port);
		spin_unlock_bh(&m->lock);
		return;
	}
	n1_port_unlock(n1_port);
	return;
}

static struct rmt_n1_port *n1pmap_find(struct rmt *instance,
				       port_id_t id)
{
	struct rmt_n1_port *entry;
	const struct hlist_head *head;
	struct n1pmap *m;

	ASSERT(instance);

	if (!is_port_id_ok(id))
		return NULL;

	m = instance->n1_ports;
	if (!m)
		return NULL;

	spin_lock_bh(&m->lock);
	head = &m->n1_ports[rmap_hash(m->n1_ports, id)];
	hlist_for_each_entry(entry, head, hlist)
		if (entry->port_id == id) {
			spin_lock(&entry->lock);
			if (entry->state == N1_PORT_STATE_DEALLOCATED) {
				spin_unlock(&entry->lock);
				spin_unlock_bh(&m->lock);
				return NULL;
			}
			atomic_inc(&entry->refs_c);
			spin_unlock(&entry->lock);
			spin_unlock_bh(&m->lock);
			return entry;
		}
	spin_unlock_bh(&m->lock);

	return NULL;
}

static int pff_cache_init(struct pff_cache *c)
{
	ASSERT(c);

	c->pids	 = NULL;
	c->count = 0;

	LOG_DBG("PFF cache %pK initialized", c);

	return 0;
}

static int pff_cache_fini(struct pff_cache *c)
{
	ASSERT(c);

	if (c->count) {
		ASSERT(c->pids);
		rkfree(c->pids);
	} else
		ASSERT(!c->pids);

	LOG_DBG("PFF cache %pK destroyed", c);

	return 0;
}

struct rmt *rmt_from_component(struct rina_component *component)
{ return container_of(component, struct rmt, base); }
EXPORT_SYMBOL(rmt_from_component);

int rmt_select_policy_set(struct rmt *rmt,
			  const string_t *path,
			  const string_t *name)
{
        struct ps_select_transaction trans;
        size_t cmplen;
        size_t offset;

        ASSERT(path);

        ps_factory_parse_component_id(path, &cmplen, &offset);

        if (cmplen && strncmp(path, "pff", cmplen) == 0) {
                /* The request addresses the PFF subcomponent. */
                return pff_select_policy_set(rmt->pff, path + offset, name);
        }

        if (cmplen && strncmp(path, "sdup", cmplen) == 0) {
                /* The request addresses the SDU protection subcomponent. */
                return sdup_select_policy_set(rmt->sdup, path + offset, name);
        }

        if (strcmp(path, "") != 0) {
                LOG_ERR("This component has no subcomponent named '%s'", path);
                return -1;
        }

        /* The request addresses this policy-set. */
        base_select_policy_set_start(&rmt->base, &trans, &policy_sets, name);

        if (trans.state == PS_SEL_TRANS_PENDING) {
                struct rmt_ps * ps;

                ps = container_of(trans.candidate_ps, struct rmt_ps, base);

		/* Fill in default policies */
                if (!ps->rmt_dequeue_policy)
			ps->rmt_dequeue_policy = default_rmt_dequeue_policy;
                if (!ps->rmt_enqueue_policy)
			ps->rmt_enqueue_policy = default_rmt_enqueue_policy;
                if (!ps->rmt_q_create_policy)
			ps->rmt_q_create_policy = default_rmt_q_create_policy;
                if (!ps->rmt_q_destroy_policy)
			ps->rmt_q_destroy_policy = default_rmt_q_destroy_policy;
        }

        base_select_policy_set_finish(&rmt->base, &trans);

        return trans.state == PS_SEL_TRANS_COMMITTED ? 0 : -1;
}
EXPORT_SYMBOL(rmt_select_policy_set);

int rmt_set_policy_set_param(struct rmt *rmt,
			     const char *path,
			     const char *name,
			     const char *value)
{
	size_t cmplen;
	size_t offset;
	struct rmt_ps *ps;
	int ret = -1;

	if (!rmt || !path || !name || !value) {
		LOG_ERRF("NULL arguments %p %p %p %p", rmt, path, name, value);
		return -1;
	}

	ps_factory_parse_component_id(path, &cmplen, &offset);

	LOG_DBG("set-policy-set-param '%s' '%s' '%s'", path, name, value);

	if (strcmp(path, "") == 0) {
		/* The request addresses this RMT instance. */
		rcu_read_lock();
		ps = container_of(rcu_dereference(rmt->base.ps),
				  struct rmt_ps, base);
		if (!ps) {
			LOG_ERR("No policy-set selected for this RMT");
		} else {
			LOG_ERR("Unknown RMT parameter policy '%s'", name);
		}

		rcu_read_unlock();

	} else if (cmplen && strncmp(path, "pff", cmplen) == 0) {
		/* The request addresses the PFF subcomponent. */
		return pff_set_policy_set_param(rmt->pff,
						path + offset,
						name, value);

	} else {
		/* The request addresses the RMT policy-set. */
		ret = base_set_policy_set_param(&rmt->base, path, name, value);
        }

	return ret;
}
EXPORT_SYMBOL(rmt_set_policy_set_param);

int rmt_address_add(struct rmt *instance,
		    address_t address)
{
	struct rmt_address * rmt_addr;

	if (!instance) {
		LOG_ERR("Bogus instance passed");
		return -1;
	}

	rmt_addr = rkzalloc(sizeof(*rmt_addr), GFP_KERNEL);
	if (!rmt_addr)
		return -1;
	rmt_addr->address = address;

	INIT_LIST_HEAD(&rmt_addr->list);
	spin_lock_bh(&instance->lock);
	list_add(&rmt_addr->list, &instance->addresses);
	spin_unlock_bh(&instance->lock);

	return 0;
}
EXPORT_SYMBOL(rmt_address_add);

int rmt_address_remove(struct rmt *instance,
		       address_t address)
{
	struct rmt_address * rmt_addr;

	if (!instance) {
		LOG_ERR("Bogus instance passed");
		return -1;
	}

        spin_lock_bh(&instance->lock);

        list_for_each_entry(rmt_addr, &instance->addresses, list) {
                if (rmt_addr->address == address) {
                        if (!list_empty(&rmt_addr->list)) {
                                list_del(&rmt_addr->list);
                        }
                        spin_unlock_bh(&instance->lock);
                        rkfree(rmt_addr);
                        return 0;
                }
        }

        spin_unlock_bh(&instance->lock);
        LOG_ERR("Could not find address to be removed: %u", address);
        return -1;
}
EXPORT_SYMBOL(rmt_address_remove);

int rmt_destroy(struct rmt *instance)
{
	struct rmt_address * addr, * naddr;

	if (!instance) {
		LOG_ERR("Bogus instance passed, bailing out");
		return -1;
	}

	tasklet_kill(&instance->egress_tasklet);
	if (instance->n1_ports)
		n1pmap_destroy(instance);
	pff_cache_fini(&instance->cache);

	if (instance->pff)
		pff_destroy(instance->pff);
	if (instance->rmt_cfg)
		rmt_config_free(instance->rmt_cfg);

	robject_del(&instance->robj);

	list_for_each_entry_safe(addr, naddr, &instance->addresses, list) {
		if (!list_empty(&addr->list)) {
			list_del(&addr->list);
		}

	        rkfree(addr);
	}

	rina_component_fini(&instance->base);

	rkfree(instance);

	LOG_DBG("Instance %pK finalized successfully", instance);

	return 0;
}
EXPORT_SYMBOL(rmt_destroy);

struct robject * rmt_robject(struct rmt * instance)
{
	if (!instance) {
		LOG_ERR("Bogus instance passed");
		return NULL;
	}

	return &instance->robj;
}
EXPORT_SYMBOL(rmt_robject);

struct rmt_config *rmt_config_get(struct rmt *instance)
{
	if (!instance) {
		LOG_ERR("No RMT passed");
		return NULL;
	}
	return instance->rmt_cfg;
}
EXPORT_SYMBOL(rmt_config_get);

int rmt_config_set(struct rmt *instance,
		   struct rmt_config *rmt_config)
{
	const string_t *rmt_ps_name;
	const string_t *pff_ps_name;

	if (!rmt_config || !rmt_config->pff_conf) {
		LOG_ERR("Bogus rmt_config passed");
		return -1;
	}

	if (!instance) {
		LOG_ERR("Bogus instance passed");
		rmt_config_free(rmt_config);
		return -1;
	}

	instance->rmt_cfg = rmt_config;

	rmt_ps_name = policy_name(rmt_config->policy_set);
	pff_ps_name = policy_name(rmt_config->pff_conf->policy_set);

	LOG_INFO("RMT PS to be selected: %s", rmt_ps_name);
	if (rmt_select_policy_set(instance, "", rmt_ps_name))
		LOG_ERR("Could not set policy set %s for RMT", rmt_ps_name);

	LOG_INFO("PFF PS to be selected: %s", pff_ps_name);
	if (pff_select_policy_set(instance->pff, "", pff_ps_name))
		LOG_ERR("Could not set policy set %s for PFF", pff_ps_name);

	rmt_config_free(instance->rmt_cfg);
	instance->rmt_cfg = NULL;
	return 0;
}
EXPORT_SYMBOL(rmt_config_set);

static int n1_port_write_du(struct rmt *rmt,
			    struct rmt_n1_port *n1_port,
			    struct du * du)
{
	int ret;
	ssize_t bytes = du_len(du);

	LOG_DBG("Gonna send SDU to port-id %d", n1_port->port_id);
	ret = n1_port->n1_ipcp->ops->du_write(n1_port->n1_ipcp->data,
					      n1_port->port_id,
					      du, false);
	if (!ret)
		return (int) bytes;

	if (ret == -EAGAIN) {
		n1_port_lock(n1_port);
		if (n1_port->pending_du) {
			LOG_ERR("Already a pending SDU present for port %d",
					n1_port->port_id);
			du_destroy(n1_port->pending_du);
			n1_port->stats.plen--;
		}

		n1_port->pending_du = du;
		n1_port->stats.plen++;

		if (n1_port->state == N1_PORT_STATE_DO_NOT_DISABLE) {
			n1_port->state = N1_PORT_STATE_ENABLED;
			tasklet_hi_schedule(&rmt->egress_tasklet);
		} else
			n1_port->state = N1_PORT_STATE_DISABLED;

		n1_port_unlock(n1_port);
	}

	return ret;
}

static inline int n1_port_write(struct rmt *rmt,
				struct rmt_n1_port *n1_port,
				struct du *du)
{
	/* SDU Protection */
	if (sdup_set_lifetime_limit(n1_port->sdup_port, du)){
		LOG_ERR("Error adding a Lifetime limit to serialized PDU");
		du_destroy(du);
		return -1;
	}

	if (sdup_protect_pdu(n1_port->sdup_port, du)){
		LOG_ERR("Error Protecting serialized PDU");
		du_destroy(du);
		return -1;
	}

	return n1_port_write_du(rmt, n1_port, du);
}

static void send_worker(unsigned long o)
{
	struct rmt *rmt;
	struct rmt_n1_port *n1_port;
	struct hlist_node *ntmp;
	int bucket;
	int reschedule = 0;
	int pdus_sent;
	struct rmt_ps *ps;
	struct du * du = NULL;
	struct du * pendu = NULL;
	int ret;

	LOG_DBG("Send worker called");

	rmt = (struct rmt *) o;
	if (!rmt) {
		LOG_ERR("No instance passed to send worker");
		return;
	}

	rcu_read_lock();
	ps = container_of(rcu_dereference(rmt->base.ps),
			  struct rmt_ps,
			  base);
	if (!ps || !ps->rmt_dequeue_policy) {
		rcu_read_unlock();
		LOG_ERR("Wrong RMT PS");
		return;
	}

	spin_lock(&rmt->n1_ports->lock);
	hash_for_each_safe(rmt->n1_ports->n1_ports, bucket,
			   ntmp, n1_port, hlist) {
		if (!n1_port)
			continue;

		spin_lock(&n1_port->lock);
		spin_unlock(&rmt->n1_ports->lock);
		if (n1_port->state == N1_PORT_STATE_DEALLOCATED &&
			atomic_read(&n1_port->refs_c) == 0) {
			spin_unlock(&n1_port->lock);
			spin_lock(&rmt->n1_ports->lock);
			n1_port_cleanup(rmt, n1_port);
			continue;
		}

		if (n1_port->state == N1_PORT_STATE_DISABLED	||
		    !n1_port->stats.plen) {
			spin_unlock(&n1_port->lock);
			spin_lock(&rmt->n1_ports->lock);
			LOG_DBG("Port state is DISABLED or no PDUs to send");
			continue;
		}

		if (n1_port->wbusy) {
			reschedule++;
			spin_unlock(&n1_port->lock);
			spin_lock(&rmt->n1_ports->lock);
			LOG_DBG("Port is sending a PDU, check afterwards");
			continue;
		}

		atomic_inc(&n1_port->refs_c);
		n1_port->wbusy = true;

		pdus_sent = 0;
		ret = 0;
		/* Try to send PDUs on that port-id here */

		while ((pdus_sent < MAX_PDUS_SENT_PER_CYCLE) &&
			n1_port->stats.plen) {
			du = NULL;
			pendu = NULL;
			if (n1_port->pending_du) {
				pendu = n1_port->pending_du;
				n1_port->pending_du = NULL;
				n1_port->stats.plen--;
			} else {
				du = ps->rmt_dequeue_policy(ps, n1_port);
				if (!du) {
					if (n1_port->stats.plen)
						LOG_ERR("rmt_dequeue_policy returned no pdu but plen is %u",
								n1_port->stats.plen);
					break;
				}
				n1_port->stats.plen--;
			}

			spin_unlock(&n1_port->lock);
			if (pendu)
				ret = n1_port_write_du(rmt, n1_port, pendu);
			else
				ret = n1_port_write(rmt, n1_port, du);
			spin_lock(&n1_port->lock);

			if (ret < 0)
				break;

			pdus_sent++;
			stats_inc(tx, n1_port, ret);
		}

		if ((n1_port->state == N1_PORT_STATE_ENABLED ||
		    n1_port->state == N1_PORT_STATE_DO_NOT_DISABLE) &&
		    n1_port->stats.plen)
			reschedule++;

		n1_port->wbusy = false;
		if (atomic_dec_and_test(&n1_port->refs_c) &&
		    n1_port->state == N1_PORT_STATE_DEALLOCATED) {
			spin_unlock(&n1_port->lock);
			spin_lock(&rmt->n1_ports->lock);
			n1_port_cleanup(rmt, n1_port);
			continue;
		}

		spin_unlock(&n1_port->lock);
		spin_lock(&rmt->n1_ports->lock);

	}
	spin_unlock(&rmt->n1_ports->lock);
	rcu_read_unlock();

	if (reschedule) {
		LOG_DBG("Sheduling policy will schedule again...");
		tasklet_hi_schedule(&rmt->egress_tasklet);
	}
}

int rmt_send_port_id(struct rmt *instance,
		     port_id_t id,
		     struct du * du)
{
	struct rmt_n1_port *n1_port;
	struct rmt_ps *ps;
	int ret;
	bool must_enqueue;

	rcu_read_lock();
	ps = container_of(rcu_dereference(instance->base.ps),
	  		  struct rmt_ps, base);

	if (!ps || !ps->rmt_enqueue_policy) {
		rcu_read_unlock();
		LOG_ERR("PS or enqueue policy null, dropping pdu");
		du_destroy(du);
		return -1;
	}

	n1_port = n1pmap_find(instance, id);
	if (!n1_port) {
		rcu_read_unlock();
		LOG_ERR("Could not find the N-1 port");
		du_destroy(du);
		return -1;
	}

	n1_port_lock(n1_port);

	must_enqueue = false;
	if (n1_port->stats.plen 				||
		n1_port->wbusy 					||
		n1_port->state == N1_PORT_STATE_DISABLED) {
		must_enqueue = true;
	}

	ret = ps->rmt_enqueue_policy(ps, n1_port, du, must_enqueue);
	rcu_read_unlock();
	switch (ret) {
	case RMT_PS_ENQ_SCHED:
		n1_port->stats.plen++;
		tasklet_hi_schedule(&instance->egress_tasklet);
		ret = 0;
		break;
	case RMT_PS_ENQ_DROP:
		n1_port->stats.drop_pdus++;
		LOG_ERR("PDU dropped while enqueing");
		ret = 0;
		break;
	case RMT_PS_ENQ_ERR:
		n1_port->stats.err_pdus++;
		LOG_ERR("Some error occurred while enqueuing PDU");
		ret = 0;
		break;
	case RMT_PS_ENQ_SEND:
		if (must_enqueue) {
			LOG_ERR("Wrong behaviour of the policy");
			du_destroy(du);
			n1_port->stats.err_pdus++;
			LOG_DBG("Policy should have enqueue, returned SEND");
			ret = -1;
			break;
		}

		n1_port->wbusy = true;
		n1_port_unlock(n1_port);
		LOG_DBG("PDU ready to be sent, no need to enqueue");
		ret = n1_port_write(instance, n1_port, du);
		/*FIXME LB: This is just horrible, needs to be rethinked */
		n1_port_lock(n1_port);
		n1_port->wbusy = false;
		if (ret >= 0) {
			stats_inc(tx, n1_port, ret);
			ret = 0;
		} else if (ret == -EAGAIN)
			ret = 0;
		break;
	default:
		LOG_ERR("rmt_enqueu_policy returned wrong value");
		break;
		ret = 0;
	}

	n1_port_unlock(n1_port);
	n1pmap_release(instance, n1_port);
	return ret;
}
EXPORT_SYMBOL(rmt_send_port_id);

int rmt_send(struct rmt *instance,
	     struct du * du)
{
	int i;

	if (!instance || !du || !pci_is_ok(&du->pci)) {
		LOG_ERR("Bogus input parameters passed");
		du_destroy(du);
		return -1;
	}

	if (pff_nhop(instance->pff, &du->pci,
		     &(instance->cache.pids),
		     &(instance->cache.count))) {
		LOG_ERR("Cannot get the NHOP for this PDU (saddr: %u daddr: %u type: %u)",
				pci_source(&du->pci), pci_destination(&du->pci),
				pci_type(&du->pci));

		du_destroy(du);
		return -1;
	}

	if (instance->cache.count == 0) {
		LOG_WARN("No NHOP for this PDU ...");
		du_destroy(du);
		return 0;
	}

	for (i = 0; i < instance->cache.count; i++) {
		port_id_t   pid;
		struct du *p;

		pid = instance->cache.pids[i];

		if (i == instance->cache.count-1)
			p = du;
		else
			p = du_dup(du);

		if (rmt_send_port_id(instance, pid, p))
			LOG_ERR("Failed to send a PDU to port-id %d", pid);
	}

	return 0;
}
EXPORT_SYMBOL(rmt_send);

int rmt_enable_port_id(struct rmt *instance,
		       port_id_t id)
{
	struct rmt_n1_port *n1_port;
	int ret = 0;

	if (!instance) {
		LOG_ERR("Bogus instance passed");
		return -1;
	}

	if (!is_port_id_ok(id)) {
		LOG_ERR("Wrong port-id %d", id);
		return -1;
	}

	if (!instance->n1_ports) {
		LOG_ERR("Invalid RMT");
		return -1;
	}

	/* incs refs_c */
	n1_port = n1pmap_find(instance, id);
	if (!n1_port) {
		LOG_WARN("No queue for N-1 port-id %d or already deallocated",
			id);
		return -1;
	}

	n1_port_lock(n1_port);
	if (n1_port->state == N1_PORT_STATE_ENABLED) {
		n1_port->state = N1_PORT_STATE_DO_NOT_DISABLE;
		goto exit;
	}

	n1_port->state = N1_PORT_STATE_ENABLED;
	LOG_DBG("Changed state to ENABLED");

exit:
	if (n1_port->stats.plen)
		tasklet_hi_schedule(&instance->egress_tasklet);

	n1_port_unlock_release(n1_port);

	return ret;
}
EXPORT_SYMBOL(rmt_enable_port_id);

int rmt_disable_port_id(struct rmt *instance,
			port_id_t id)
{
	struct rmt_n1_port *n1_port;
	int ret = 0;

	if (!instance) {
		LOG_ERR("Bogus instance passed");
		return -1;
	}

	if (!is_port_id_ok(id)) {
		LOG_ERR("Wrong port-id %d", id);
		return -1;
	}

	if (!instance->n1_ports) {
		LOG_ERR("Invalid RMT");
		return -1;
	}

	n1_port = n1pmap_find(instance, id);
	if (!n1_port) {
		LOG_ERR("No n1_port for port-id or deallocated, %d", id);
		return -1;
	}

	n1_port_lock(n1_port);
	if (n1_port->state == N1_PORT_STATE_DISABLED) {
		LOG_DBG("Nothing to do for port-id %d", id);
		goto exit;
	}

	if (n1_port->state == N1_PORT_STATE_DO_NOT_DISABLE) {
		n1_port->state = N1_PORT_STATE_ENABLED;
		if (n1_port->stats.plen)
			tasklet_hi_schedule(&instance->egress_tasklet);
		goto exit;
	}

	n1_port->state = N1_PORT_STATE_DISABLED;
	LOG_DBG("Changed state to DISABLED");

exit:
	n1_port_unlock_release(n1_port);
	return ret;
}
EXPORT_SYMBOL(rmt_disable_port_id);

int rmt_n1port_bind(struct rmt *instance,
		    port_id_t id,
		    struct ipcp_instance *n1_ipcp)
{
	struct rmt_n1_port *tmp;
	struct rmt_ps *ps;
	const struct name *dif_name;

	if (!instance) {
		LOG_ERR("Bogus instance passed");
		return -1;
	}

	if (!is_port_id_ok(id)) {
		LOG_ERR("Wrong port-id %d", id);
		return -1;
	}

	if (!n1_ipcp) {
		LOG_ERR("Invalid N-1 IPCP passed");
		return -1;
	}

	if (!instance->n1_ports) {
		LOG_ERR("Invalid RMT");
		return -1;
	}

	if (n1pmap_find(instance, id)) {
		LOG_ERR("Queue already exists");
		return -1;
	}

	tmp = n1_port_create(id, n1_ipcp);
	if (!tmp)
		return -1;
	if (robject_rset_add(&tmp->robj, instance->n1_ports->rset, "%d", id)) {
		n1_port_destroy(tmp);
		return -1;
	}

	rcu_read_lock();
	ps = container_of(rcu_dereference(instance->base.ps),
			  struct rmt_ps,
			  base);
	if (!ps || !ps->rmt_q_create_policy) {
		rcu_read_unlock();
		LOG_ERR("No PS in the RMT, can't bind");
		n1_port_destroy(tmp);
		return -1;
	}

	tmp->rmt_ps_queues = ps->rmt_q_create_policy(ps, tmp);
	rcu_read_unlock();
	if (!tmp->rmt_ps_queues) {
		LOG_ERR("Cannot create structs for scheduling policy");
		n1_port_destroy(tmp);
		return -1;
	}

	hash_add(instance->n1_ports->n1_ports, &tmp->hlist, id);
	LOG_DBG("Added send queue to rmt instance %pK for port-id %d",
		instance, id);

	dif_name = n1_ipcp->ops->dif_name(n1_ipcp->data);
	tmp->sdup_port = sdup_init_port_config(instance->sdup, dif_name, id);
	if (!tmp->sdup_port){
		LOG_ERR("Failed init of SDUP configuration for port-id %d", id);
		n1_port_destroy(tmp);
		return -1;
	}

	return 0;
}
EXPORT_SYMBOL(rmt_n1port_bind);

int rmt_n1port_unbind(struct rmt *instance,
		      port_id_t id)
{
	struct rmt_n1_port *n1_port;

	if (!instance) {
		LOG_ERR("Bogus instance passed");
		return -1;
	}

	if (!is_port_id_ok(id)) {
		LOG_ERR("Wrong port-id %d", id);
		return -1;
	}

	if (!instance->n1_ports) {
		LOG_ERR("Bogus egress instance passed");
		return -1;
	}

	n1_port = n1pmap_find(instance, id);
	if (!n1_port) {
		LOG_WARN("N1 port already deallocated, nothing to do...");
		return 0;
	}

	n1_port_lock(n1_port);
	n1_port->state = N1_PORT_STATE_DEALLOCATED;
	n1_port_unlock(n1_port);
	/* NOTE: Releasing the lock to be able to use n1pmap_release... it is
	 * not wrong since once in N1_PORT_STATE_DEALLOCATED no other action
	 * will be performed on the n1_port but this action should be atomic */
	n1pmap_release(instance, n1_port);
	return 0;
}
EXPORT_SYMBOL(rmt_n1port_unbind);

static inline int process_mgmt_pdu(struct rmt *rmt,
				   port_id_t port_id,
				   struct du * du)
{
	return (rmt->parent->ops->mgmt_du_post(rmt->parent->data,
					       port_id,
					       du) ? -1 : 0);
}

static int process_dt_pdu(struct rmt *rmt,
			  port_id_t port_id,
			  struct du *du)
{
	address_t dst_addr;
	cep_id_t c;
	pdu_type_t pdu_type;

	dst_addr = pci_destination(&du->pci);
	if (!is_address_ok(dst_addr)) {
		LOG_ERR("PDU has wrong destination address");
		du_destroy(du);
		return -1;
	}

	pdu_type = pci_type(&du->pci);

	if (pdu_type == PDU_TYPE_MGMT) {
		LOG_ERR("MGMT should not be here");
		du_destroy(du);
		return -1;
	}

	c = pci_cep_destination(&du->pci);
	if (!is_cep_id_ok(c)) {
		LOG_ERR("Wrong CEP-id in PDU");
		du_destroy(du);
		return -1;
	}

	if (efcp_container_receive(rmt->efcpc, c, du)) {
		LOG_ERR("EFCP container problems");
		return -1;
	}

	LOG_DBG("process_dt_pdu internally finished");
	return 0;
}

int pdu_is_addressed_to_me(struct rmt * rmt, address_t address)
{
	struct rmt_address * addr;

	spin_lock_bh(&rmt->lock);
	list_for_each_entry(addr, &rmt->addresses, list) {
                if (addr->address == address) {
                	spin_unlock_bh(&rmt->lock);
                	return 1;
                }
	}

	spin_unlock_bh(&rmt->lock);
	return 0;
}

int rmt_receive(struct rmt *rmt,
		struct du * du,
		port_id_t from)
{
	pdu_type_t pdu_type;
	address_t dst_addr;
	qos_id_t qos_id;
	struct rmt_n1_port *n1_port;
	ssize_t bytes;

	if (!rmt) {
		LOG_ERR("No RMT passed");
		du_destroy(du);
		return -1;
	}
	if (!is_port_id_ok(from)) {
		LOG_ERR("Wrong port-id %d", from);
		du_destroy(du);
		return -1;
	}

	bytes = du_len(du);
	du->cfg = rmt->efcpc->config;

	n1_port = n1pmap_find(rmt, from);
	if (!n1_port) {
		LOG_ERR("Could not retrieve N-1 port for the received PDU...");
                du_destroy(du);
		return -1;
	}
	stats_inc(rx, n1_port, bytes);

	/* SDU Protection */
	if (sdup_unprotect_pdu(n1_port->sdup_port, du)) {
                LOG_ERR("Failed to unprotect PDU");
                du_destroy(du);
                return -1;
        }

	/* This one updates the pci->sdup_header and pdu->skb->data pointers */
	if (sdup_get_lifetime_limit(n1_port->sdup_port, du)) {
                LOG_ERR("Failed to get PDU's TTL");
                du_destroy(du);
                return -1;
        }
	/* end SDU Protection */

	n1pmap_release(rmt, n1_port);

	if (unlikely(du_decap(du))) { /*Decap PDU */
		LOG_ERR("Could not decap PDU");
		du_destroy(du);
		return -1;
	}

	pdu_type = pci_type(&du->pci);
	dst_addr = pci_destination(&du->pci);
	qos_id = pci_qos_id(&du->pci);
	if (!pdu_type_is_ok(pdu_type) ||
		!is_address_ok(dst_addr)  ||
		!is_qos_id_ok(qos_id)) {
		LOG_ERR("Wrong PDU type (%u), dst address (%u) or qos_id (%u)",
			pdu_type, dst_addr, qos_id);
		du_destroy(du);
		return -1;
	}

	/* pdu is for me */
	if (pdu_is_addressed_to_me(rmt, dst_addr)) {
		/* pdu is for me */
		switch (pdu_type) {
		case PDU_TYPE_MGMT:
			return process_mgmt_pdu(rmt, from, du);

		case PDU_TYPE_CACK:
		case PDU_TYPE_SACK:
		case PDU_TYPE_NACK:
		case PDU_TYPE_FC:
		case PDU_TYPE_ACK:
		case PDU_TYPE_ACK_AND_FC:
		case PDU_TYPE_RENDEZVOUS:
		case PDU_TYPE_DT:
			/*
			 * (FUTURE)
			 *
			 * enqueue PDU in pdus_dt[dest-addr, qos-id]
			 * don't process it now ...
			 */
			return process_dt_pdu(rmt, from, du);

		default:
			LOG_ERR("Unknown PDU type %d", pdu_type);
			du_destroy(du);
			return -1;
		}
	/* pdu is not for me*/
	} else {
		if (!dst_addr)
			return process_mgmt_pdu(rmt, from, du);
		else {
			/* We need to rencap the pdu again to get pci properly
			 * in rmt_send */
			if (unlikely(du_encap(du, pdu_type)))
				return -1;
			if (sdup_dec_check_lifetime_limit(n1_port->sdup_port, du)) {
				LOG_ERR("Lifetime of PDU reached dropping PDU!");
				du_destroy(du);
				return -1;
			}
			/* Forward PDU */
			return rmt_send(rmt, du);
		}
	}
}
EXPORT_SYMBOL(rmt_receive);

struct rmt *rmt_create(struct kfa *kfa,
		       struct efcp_container *efcpc,
		       struct sdup *sdup,
		       struct robject *parent)
{
	struct rmt *tmp;

	if (!parent || !kfa || !efcpc) {
		LOG_ERR("Bogus input parameters");
		return NULL;
	}

	tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
	if (!tmp)
		return NULL;

	INIT_LIST_HEAD(&tmp->addresses);
	tmp->parent = container_of(parent, struct ipcp_instance, robj);
	tmp->kfa = kfa;
	tmp->efcpc = efcpc;
	tmp->sdup = sdup;
	rina_component_init(&tmp->base);

	if (robject_init_and_add(&tmp->robj, &rmt_rtype, parent, "rmt")) {
                LOG_ERR("Failed to create RMT sysfs entry");
                rmt_destroy(tmp);
                return NULL;
	}

	tmp->pff = pff_create(&tmp->robj, tmp->parent);
	if (!tmp->pff) {
		rmt_destroy(tmp);
		return NULL;
	}

	tmp->n1_ports = n1pmap_create(&tmp->robj);
	if (!tmp->n1_ports) {
		LOG_ERR("Failed to create N-1 ports map");
		rmt_destroy(tmp);
		return NULL;
	}

	if (pff_cache_init(&tmp->cache)) {
		LOG_ERR("Failed to init pff cache");
		rmt_destroy(tmp);
		return NULL;
	}

	tasklet_init(&tmp->egress_tasklet,
		     send_worker,
		     (unsigned long) tmp);

	LOG_DBG("Instance %pK initialized successfully", tmp);
	return tmp;
}
EXPORT_SYMBOL(rmt_create);

static bool is_rmt_pff_ok(struct rmt *instance)
{ return (instance && instance->pff) ? true : false; }

int rmt_pff_add(struct rmt *instance,
		struct mod_pff_entry *entry)
{ return is_rmt_pff_ok(instance) ? pff_add(instance->pff, entry) : -1; }
EXPORT_SYMBOL(rmt_pff_add);

int rmt_pff_remove(struct rmt *instance,
		   struct mod_pff_entry *entry)
{ return is_rmt_pff_ok(instance) ? pff_remove(instance->pff, entry) : -1; }
EXPORT_SYMBOL(rmt_pff_remove);

int rmt_pff_port_state_change(struct rmt *rmt,
			      port_id_t	port_id,
			      bool up)
{
	return is_rmt_pff_ok(rmt) ?
		pff_port_state_change(rmt->pff, port_id, up) : -1;
}
EXPORT_SYMBOL(rmt_pff_port_state_change);

int rmt_pff_dump(struct rmt *instance,
		 struct list_head *entries)
{ return is_rmt_pff_ok(instance) ? pff_dump(instance->pff, entries) : -1; }
EXPORT_SYMBOL(rmt_pff_dump);

int rmt_pff_flush(struct rmt *instance)
{ return is_rmt_pff_ok(instance) ? pff_flush(instance->pff) : -1; }
EXPORT_SYMBOL(rmt_pff_flush);

int rmt_pff_modify(struct rmt *instance,
		    struct list_head *entries)
{ return is_rmt_pff_ok(instance) ? pff_modify(instance->pff, entries) : -1; }
EXPORT_SYMBOL(rmt_pff_modify);

int rmt_ps_publish(struct ps_factory *factory)
{
	if (factory == NULL) {
		LOG_ERR("%s: NULL factory", __func__);
		return -1;
	}

	return ps_publish(&policy_sets, factory);
}
EXPORT_SYMBOL(rmt_ps_publish);

int rmt_ps_unpublish(const char *name)
{ return ps_unpublish(&policy_sets, name); }
EXPORT_SYMBOL(rmt_ps_unpublish);
