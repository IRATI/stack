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
#include "du.h"
#include "rmt.h"
#include "pff.h"
#include "efcp-utils.h"
#include "serdes.h"
#include "pdu-ser.h"
#include "rmt-ps.h"
#include "policies.h"
#include "rds/rstr.h"
#include "ipcp-instances.h"
#include "ipcp-utils.h"
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

struct rmt {
	struct rina_component base;
	address_t address;
	struct ipcp_instance *parent;
	struct pff *pff;
	struct kfa *kfa;
	struct efcp_container *efcpc;
	struct serdes *serdes;
	struct tasklet_struct egress_tasklet;
	struct n1pmap *n1_ports;
	struct pff_cache cache;
	struct rmt_config *rmt_cfg;
	struct sdup *sdup;
};

static struct rmt_n1_port *n1_port_create(port_id_t id,
					  struct ipcp_instance *n1_ipcp)
{
	struct rmt_n1_port *tmp;

	ASSERT(is_port_id_ok(id));

	tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
	if (!tmp)
		return NULL;

	INIT_HLIST_NODE(&tmp->hlist);

	tmp->port_id = id;
	tmp->n1_ipcp = n1_ipcp;
	tmp->state   = N1_PORT_STATE_ENABLED;

	atomic_set(&tmp->refs_c, 0);
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

	if (n1p->sdup_port)
		sdup_destroy_port_config(n1p->sdup_port);

	if (n1p->pending_pdu)
		pdu_destroy(n1p->pending_pdu);

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
	/* FIXME: Has to be moved in the pipelines */
	spinlock_t lock;

	DECLARE_HASHTABLE(n1_ports, 7);
};

static struct n1pmap *n1pmap_create(void)
{
	struct n1pmap *tmp;

	tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
	if (!tmp)
		return NULL;

	hash_init(tmp->n1_ports);
	spin_lock_init(&tmp->lock);

	return tmp;
}

static int n1pmap_destroy(struct rmt *instance)
{
	struct rmt_n1_port *entry;
	struct hlist_node *tmp;
	unsigned long flags;
	int bucket;
	struct n1pmap *m;

	ASSERT(instance);

	m = instance->n1_ports;
	if (!m)
		return 0;

	spin_lock_irqsave(&m->lock, flags);
	hash_for_each_safe(m->n1_ports, bucket, tmp, entry, hlist) {
		ASSERT(entry);
		if (n1_port_user_ipcp_unbind(entry))
			LOG_ERR("Could not destroy entry %pK", entry);

		if (n1_port_cleanup(instance, entry))
			LOG_ERR("Could not destroy entry %pK", entry);
	}
	spin_unlock_irqrestore(&m->lock, flags);

	rkfree(m);
	return 0;
}

#define n1_port_lock(port, flags)	\
	spin_lock_irqsave(&port->lock, flags)

#define n1_port_unlock(port, flags)	\
	spin_unlock_irqrestore(&port->lock, flags)

#define n1_port_unlock_release(port, flags)	\
	atomic_dec(&port->refs_c);		\
	n1_port_unlock(port, flags)

static void n1pmap_release(struct rmt *instance,
			   struct rmt_n1_port *n1_port)
{
	struct n1pmap *m;
	unsigned long flags;

	ASSERT(instance);

	m = instance->n1_ports;
	if (!m)
		return;

	n1_port_lock(n1_port, flags);
	if (atomic_dec_and_test(&n1_port->refs_c) &&
	    n1_port->state == N1_PORT_STATE_DEALLOCATED) {
		n1_port_unlock(n1_port, flags);
		spin_lock_irqsave(&m->lock, flags);
		n1_port_cleanup(instance, n1_port);
		spin_unlock_irqrestore(&m->lock, flags);
		return;
	}
	n1_port_unlock(n1_port, flags);
	return;
}

static struct rmt_n1_port *n1pmap_find(struct rmt *instance,
				       port_id_t id)
{
	struct rmt_n1_port *entry;
	const struct hlist_head *head;
	struct n1pmap *m;
	unsigned long flags;

	ASSERT(instance);

	if (!is_port_id_ok(id))
		return NULL;

	m = instance->n1_ports;
	if (!m)
		return NULL;

	spin_lock_irqsave(&m->lock, flags);
	head = &m->n1_ports[rmap_hash(m->n1_ports, id)];
	hlist_for_each_entry(entry, head, hlist)
		if (entry->port_id == id) {
			spin_lock(&entry->lock);
			if (entry->state == N1_PORT_STATE_DEALLOCATED) {
				spin_unlock(&entry->lock);
				spin_unlock_irqrestore(&m->lock, flags);
				return NULL;
			}
			atomic_inc(&entry->refs_c);
			spin_unlock(&entry->lock);
			spin_unlock_irqrestore(&m->lock, flags);
			return entry;
		}
	spin_unlock_irqrestore(&m->lock, flags);

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
                if (!ps->rmt_needs_sched_policy)
			ps->rmt_needs_sched_policy = default_rmt_needs_sched_policy;
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
		if (!ps)
			LOG_ERR("No policy-set selected for this RMT");
		else
			LOG_ERR("Unknown RMT parameter policy '%s'", name);

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

int rmt_destroy(struct rmt *instance)
{
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
	if (instance->serdes)
		serdes_destroy(instance->serdes);
	if (instance->rmt_cfg)
		rmt_config_destroy(instance->rmt_cfg);

	rina_component_fini(&instance->base);

	rkfree(instance);

	LOG_DBG("Instance %pK finalized successfully", instance);

	return 0;
}
EXPORT_SYMBOL(rmt_destroy);

int rmt_address_set(struct rmt *instance,
		    address_t address)
{
	if (!instance) {
		LOG_ERR("Bogus instance passed");
		return -1;
	}

	if (is_address_ok(instance->address)) {
		LOG_ERR("The RMT is already configured");
		return -1;
	}

	instance->address = address;

	return 0;
}
EXPORT_SYMBOL(rmt_address_set);

int rmt_dt_cons_set(struct rmt *instance,
		    struct dt_cons *dt_cons)
{
	if (!instance) {
		LOG_ERR("Bogus instance passed");
		return -1;
	}

	if (!dt_cons) {
		LOG_ERR("Bogus dt_cons passed");
		return -1;
	}

	instance->serdes = serdes_create(dt_cons);
	if (!instance->serdes) {
		LOG_ERR("Serdes creation failed");
		return -1;
	}

	return 0;
}
EXPORT_SYMBOL(rmt_dt_cons_set);

struct rmt_config *rmt_config_get(struct rmt *instance)
{
	if (!instance) {
		LOG_ERR("No RMT passed");
		return NULL;
	}
	return instance->rmt_cfg;
}
EXPORT_SYMBOL(rmt_config_get);

int rmt_config_set(struct rmt	     *instance,
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
		rmt_config_destroy(rmt_config);
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

	rmt_config_destroy(rmt_config);
	instance->rmt_cfg = NULL;
	return 0;
}
EXPORT_SYMBOL(rmt_config_set);

static int n1_port_write(struct rmt *rmt,
			 struct rmt_n1_port *n1_port,
			 struct pdu *pdu)
{
	struct sdu *sdu;
	struct pdu_ser *pdu_ser;
	port_id_t port_id;
	struct buffer *buffer;
	struct ipcp_instance *n1_ipcp;
	struct pci *pci;
	int ret;
	unsigned long flags;

	ASSERT(n1_port);
	ASSERT(rmt);
	ASSERT(rmt->serdes);

	if (!pdu) {
		LOG_DBG("No PDU to work in this queue ...");
		return -1;
	}

	port_id = n1_port->port_id;
	n1_ipcp = n1_port->n1_ipcp;

	pci = pdu_pci_get_rw(pdu);
	if (!pci) {
		LOG_ERR("Cannot get PCI");
		pdu_destroy(pdu);
		return -1;
	}

	pdu_ser = pdu_serialize_ni(rmt->serdes, pdu);
	if (!pdu_ser) {
		LOG_ERR("Error creating serialized PDU");
		goto error_nobuff;
	}

	/* SDU Protection */
	if (sdup_set_lifetime_limit(n1_port->sdup_port, pdu_ser, pci)){
                LOG_ERR("Error adding a Lifetime limit to serialized PDU");
                pdu_destroy(pdu);
                pdu_ser_destroy(pdu_ser);
                return -1;
        }

	if (sdup_protect_pdu(n1_port->sdup_port, pdu_ser)){
                LOG_ERR("Error Protecting serialized PDU");
                pdu_destroy(pdu);
                pdu_ser_destroy(pdu_ser);
                return -1;
        }
	/* end SDU Protection */

	buffer = pdu_ser_buffer(pdu_ser);
	if (!buffer_is_ok(buffer)) {
		LOG_ERR("Buffer is not okay");
		goto error_all;
	}

	if (pdu_ser_buffer_disown(pdu_ser)) {
		LOG_ERR("Could not disown buffer");
		goto error_all;
	}

	pdu_ser_destroy(pdu_ser);

	sdu = sdu_create_buffer_with_ni(buffer);
	if (!sdu) {
		LOG_ERR("Error creating SDU from serialized PDU");
		goto error_all;
	}

	LOG_DBG("Gonna send SDU to port-id %d", port_id);
	ret = n1_ipcp->ops->sdu_write(n1_ipcp->data, port_id, sdu);

	if (ret == -EAGAIN) {
		n1_port_lock(n1_port, flags);
		if (n1_port->pending_pdu) {
			LOG_ERR("Already a pending PDU present for port %d",
				n1_port->port_id);
			pdu_destroy(n1_port->pending_pdu);
		}
		/* FIXME: the serialized SDU should be kept instead of the pdu,
		 * but we need the pdu to be sent to n1_port_write
		 */
		n1_port->pending_pdu = pdu;

		if (n1_port->state == N1_PORT_STATE_DO_NOT_DISABLE) {
			n1_port->state = N1_PORT_STATE_ENABLED;
		       	tasklet_hi_schedule(&rmt->egress_tasklet);
		} else
			n1_port->state = N1_PORT_STATE_DISABLED;

		n1_port_unlock(n1_port, flags);
		return 0;
	}
	pdu_destroy(pdu);
	return ret;

error_all:
	buffer_destroy(buffer);
error_nobuff:
	pdu_destroy(pdu);
	return -1;
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
	struct pdu *pdu = NULL;
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
	if (!ps || !ps->rmt_dequeue_policy || !ps->rmt_needs_sched_policy) {
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

		if (n1_port->state == N1_PORT_STATE_DISABLED ||
		    !ps->rmt_needs_sched_policy(ps, n1_port)) {
			spin_unlock(&n1_port->lock);
			spin_lock(&rmt->n1_ports->lock);
			LOG_DBG("Port state is DISABLED or no PDUs to send");
			continue;
		}

		atomic_inc(&n1_port->refs_c);

		pdus_sent = 0;
		ret = 0;
		/* Try to send PDUs on that port-id here */
		while ((pdus_sent < MAX_PDUS_SENT_PER_CYCLE) &&
			ps->rmt_needs_sched_policy(ps, n1_port)) {
			pdu = ps->rmt_dequeue_policy(ps, n1_port);
			if (!pdu)
				break;

			spin_unlock(&n1_port->lock);
			ret =  n1_port_write(rmt, n1_port, pdu);
			spin_lock(&n1_port->lock);

			if (ret)
				break;

			pdus_sent++;
		}

		if (n1_port->state == N1_PORT_STATE_ENABLED &&
			ps->rmt_needs_sched_policy(ps, n1_port))
			reschedule++;

		rcu_read_unlock();

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

	if (reschedule) {
		LOG_DBG("Sheduling policy will schedule again...");
		tasklet_hi_schedule(&rmt->egress_tasklet);
	}
}

int rmt_send_port_id(struct rmt *instance,
		     port_id_t id,
		     struct pdu *pdu)
{
	struct rmt_n1_port *out_n1_port;
	struct rmt_ps *ps;
	unsigned long flags;
	int ret;

	if (!pdu_is_ok(pdu)) {
		LOG_ERR("Bogus PDU passed");
		return -1;
	}
	if (!instance) {
		LOG_ERR("Bogus RMT passed");
		pdu_destroy(pdu);
		return -1;
	}

	if (!instance->n1_ports) {
		LOG_ERR("No N-1 ports to push into");
		pdu_destroy(pdu);
		return -1;
	}

	out_n1_port = n1pmap_find(instance, id);
	if (!out_n1_port) {
		LOG_ERR("Could not find the N-1 port");
		pdu_destroy(pdu);
		return -1;
	}

	rcu_read_lock();
	ps = container_of(rcu_dereference(instance->base.ps),
	  		  struct rmt_ps, base);

	if (!ps || !ps->rmt_enqueue_policy) {
		rcu_read_unlock();
		n1pmap_release(instance, out_n1_port);
		LOG_ERR("PS or enqueue policy null, dropping pdu");
		pdu_destroy(pdu);
		return -1;
	}

	n1_port_lock(out_n1_port, flags);
	ret = ps->rmt_enqueue_policy(ps, out_n1_port, pdu);
	n1_port_unlock(out_n1_port, flags);
	rcu_read_unlock();

	switch (ret) {
	case RMT_PS_ENQ_SCHED:
		n1pmap_release(instance, out_n1_port);
		tasklet_hi_schedule(&instance->egress_tasklet);
		return 0;
	case RMT_PS_ENQ_DSEND:
		LOG_DBG("PDU ready to be sent, no need to enqueue");
		ret = n1_port_write(instance, out_n1_port, pdu);
		n1pmap_release(instance, out_n1_port);
		return ret;
	case RMT_PS_ENQ_DROP:
		n1pmap_release(instance, out_n1_port);
		LOG_DBG("PDU dropped while enqueing");
		return 0;
	case RMT_PS_ENQ_ERR:
		n1pmap_release(instance, out_n1_port);
		LOG_DBG("Some error occurred while enqueuing PDU");
		return 0;
	default:
		n1pmap_release(instance, out_n1_port);
		LOG_ERR("rmt_enqueu_policy returned wrong value");
		return -1;
	}
}
EXPORT_SYMBOL(rmt_send_port_id);

int rmt_send(struct rmt *instance,
	     struct pdu *pdu)
{
	int i;
	struct pci *pci;

	if (!instance) {
		LOG_ERR("Bogus RMT passed");
		return -1;
	}
	if (!pdu) {
		LOG_ERR("Bogus PDU passed");
		return -1;
	}

	pci = pdu_pci_get_rw(pdu);
	if (!pci_is_ok(pci)) {
		LOG_ERR("PCI is not ok");
		return -1;
	}

	if (pff_nhop(instance->pff, pci,
		     &(instance->cache.pids),
		     &(instance->cache.count))) {
		LOG_ERR("Cannot get the NHOP for this PDU");

		pdu_destroy(pdu);
		return -1;
	}

	if (instance->cache.count == 0) {
		LOG_WARN("No NHOP for this PDU ...");
		pdu_destroy(pdu);
		return 0;
	}

	/*
	 * FIXME:
	 *   pdu -> pci-> qos-id | cep_id_t -> connection -> qos-id (former)
	 *   address + qos-id (pdu-fwd-t) -> port-id
	 */

	for (i = 0; i < instance->cache.count; i++) {
		port_id_t   pid;
		struct pdu *p;

		pid = instance->cache.pids[i];

		if (i == instance->cache.count-1)
			p = pdu;
		else
			p = pdu_dup(pdu);

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
	unsigned long flags;
	struct rmt_ps *ps;
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
		LOG_ERR("No queue for this port-id or already deallocated, %d",
			id);
		return -1;
	}

	n1_port_lock(n1_port, flags);
	if (n1_port->state == N1_PORT_STATE_ENABLED) {
		n1_port->state = N1_PORT_STATE_DO_NOT_DISABLE;
		goto exit;
	}

	rcu_read_lock();
	ps = container_of(rcu_dereference(instance->base.ps),
			  struct rmt_ps,
			  base);
	if (!ps || !ps->rmt_needs_sched_policy) {
		rcu_read_unlock();
		LOG_ERR("Wrong RMT PS");
		ret = -1;
		goto exit;
	}
	n1_port->state = N1_PORT_STATE_ENABLED;
	if (ps->rmt_needs_sched_policy(ps, n1_port))
		tasklet_hi_schedule(&instance->egress_tasklet);
	rcu_read_unlock();

	LOG_DBG("Changed state to ENABLED");
exit:
	n1_port_unlock_release(n1_port, flags);

	return ret;
}
EXPORT_SYMBOL(rmt_enable_port_id);

int rmt_disable_port_id(struct rmt *instance,
			port_id_t id)
{
	struct rmt_n1_port *n1_port;
	unsigned long flags;
	struct rmt_ps *ps;
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

	n1_port_lock(n1_port, flags);
	if (n1_port->state == N1_PORT_STATE_DISABLED) {
		LOG_DBG("Nothing to do for port-id %d", id);
		goto exit;
	}

	if (n1_port->state == N1_PORT_STATE_DO_NOT_DISABLE) {
		n1_port->state = N1_PORT_STATE_ENABLED;
		rcu_read_lock();
		ps = container_of(rcu_dereference(instance->base.ps),
				  struct rmt_ps,
				  base);
		if (!ps || !ps->rmt_needs_sched_policy) {
			rcu_read_unlock();
			LOG_ERR("Wrong RMT PS");
			ret = -1;
			goto exit;
		}
		if (ps->rmt_needs_sched_policy(ps, n1_port))
			tasklet_hi_schedule(&instance->egress_tasklet);
		rcu_read_unlock();
		goto exit;
	}

	n1_port->state = N1_PORT_STATE_DISABLED;
	LOG_DBG("Changed state to DISABLED");

exit:
	n1_port_unlock_release(n1_port, flags);
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

	rcu_read_lock();
	ps = container_of(rcu_dereference(instance->base.ps),
			  struct rmt_ps,
			  base);
	if (!ps || !ps->rmt_q_create_policy) {
		rcu_read_unlock();
		LOG_ERR("No PS in the RMT, can't bind");
		return -1;
	}

	if (ps->rmt_q_create_policy(ps, tmp)) {
		rcu_read_unlock();
		LOG_ERR("Cannot create structs for scheduling policy");
		n1_port_destroy(tmp);
		return -1;
	}
	rcu_read_unlock();

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
	unsigned long flags;

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

	spin_lock_irqsave(&n1_port->lock, flags);
	n1_port->state = N1_PORT_STATE_DEALLOCATED;
	spin_unlock_irqrestore(&n1_port->lock, flags);
	/* NOTE: Releasing the lock to be able to use n1pmap_release... it is
	 * not wrong since once in N1_PORT_STATE_DEALLOCATED no other action
	 * will be performed on the n1_port but this action should be atomic */
	n1pmap_release(instance, n1_port);
	return 0;
}
EXPORT_SYMBOL(rmt_n1port_unbind);

static int process_mgmt_pdu_ni(struct rmt *rmt,
			       port_id_t port_id,
			       struct pdu *pdu)
{
	struct buffer *buffer;
	struct sdu *sdu;

	ASSERT(rmt);
	ASSERT(is_port_id_ok(port_id));
	ASSERT(pdu_is_ok(pdu));

	buffer = pdu_buffer_get_rw(pdu);
	if (!buffer_is_ok(buffer)) {
		LOG_ERR("PDU has no buffer ???");
		return -1;
	}

	sdu = sdu_create_buffer_with_ni(buffer);
	if (!sdu_is_ok(sdu)) {
		LOG_ERR("Cannot create SDU");
		pdu_destroy(pdu);
		return -1;
	}

	pdu_buffer_disown(pdu);
	pdu_destroy(pdu);

	ASSERT(rmt->parent);
	ASSERT(rmt->parent->ops);
	ASSERT(rmt->parent->ops->mgmt_sdu_post);

	return (rmt->parent->ops->mgmt_sdu_post(rmt->parent->data,
						port_id,
						sdu) ? -1 : 0);
}

static int process_dt_pdu(struct rmt *rmt,
			  port_id_t port_id,
			  struct pdu *pdu)
{
	address_t dst_addr;
	cep_id_t c;
	pdu_type_t pdu_type;

	ASSERT(rmt);
	ASSERT(is_port_id_ok(port_id));
	ASSERT(pdu_is_ok(pdu));

	/* (FUTURE) Address and qos-id are the same, do a single match only */

	dst_addr = pci_destination(pdu_pci_get_ro(pdu));
	if (!is_address_ok(dst_addr)) {
		LOG_ERR("PDU has wrong destination address");
		pdu_destroy(pdu);
		return -1;
	}

	pdu_type = pci_type(pdu_pci_get_ro(pdu));

	if (pdu_type == PDU_TYPE_MGMT) {
		LOG_ERR("MGMT should not be here");
		pdu_destroy(pdu);
		return -1;
	}

	c = pci_cep_destination(pdu_pci_get_ro(pdu));
	if (!is_cep_id_ok(c)) {
		LOG_ERR("Wrong CEP-id in PDU");
		pdu_destroy(pdu);
		return -1;
	}

	if (efcp_container_receive(rmt->efcpc, c, pdu)) {
		LOG_ERR("EFCP container problems");
		return -1;
	}

	LOG_DBG("process_dt_pdu internally finished");
	return 0;
}

int rmt_receive(struct rmt *rmt,
		struct sdu *sdu,
		port_id_t from)
{
	pdu_type_t pdu_type;
	struct pci *pci;
	struct pdu_ser *pdu_ser;
	struct pdu *pdu;
	address_t dst_addr;
	qos_id_t qos_id;
	struct serdes *serdes;
	struct buffer *buf;
	struct rmt_n1_port *n1_port;
	unsigned long flags;
	size_t		 ttl;

	if (!sdu_is_ok(sdu)) {
		LOG_ERR("Bogus SDU passed");
		return -1;
	}
	if (!rmt) {
		LOG_ERR("No RMT passed");
		sdu_destroy(sdu);
		return -1;
	}
	if (!is_port_id_ok(from)) {
		LOG_ERR("Wrong port-id %d", from);
		sdu_destroy(sdu);
		return -1;
	}

	n1_port = n1pmap_find(rmt, from);
	if (!n1_port) {
		LOG_ERR("Could not retrieve N-1 port for the received PDU...");
		sdu_destroy(sdu);
		return -1;
	}

	buf = sdu_buffer_rw(sdu);
	if (!buf) {
		LOG_DBG("No buffer present");
		sdu_destroy(sdu);
		return -1;
	}

	if (sdu_buffer_disown(sdu)) {
		LOG_DBG("Could not disown SDU");
		sdu_destroy(sdu);
		return -1;
	}

	sdu_destroy(sdu);

	pdu_ser = pdu_ser_create_buffer_with_ni(buf);
	if (!pdu_ser) {
		LOG_DBG("No ser PDU to work with");
		buffer_destroy(buf);
		return -1;
	}

	/* SDU Protection */
	if (sdup_unprotect_pdu(n1_port->sdup_port, pdu_ser)) {
                LOG_DBG("Failed to unprotect PDU");
                pdu_ser_destroy(pdu_ser);
                return -1;
        }

	if (sdup_get_lifetime_limit(n1_port->sdup_port, pdu_ser, &ttl)) {
                LOG_DBG("Failed to unprotect PDU");
                pdu_ser_destroy(pdu_ser);
                return -1;
        }
	/* end SDU Protection */

	n1pmap_release(rmt, n1_port);

        serdes = rmt->serdes;
        ASSERT(serdes);

        pdu = pdu_deserialize_ni(serdes, pdu_ser);
	if (!pdu) {
		LOG_ERR("Failed to deserialize PDU!");
		pdu_ser_destroy(pdu_ser);
		return -1;
	}

	pci = pdu_pci_get_rw(pdu);
	if (!pci) {
		LOG_ERR("No PCI to work with, dropping SDU!");
		pdu_destroy(pdu);
		return -1;
	}

	/* store TTL value received from SDUP module */
	pci_ttl_set(pci, ttl);

	ASSERT(pdu_is_ok(pdu));

	pdu_type = pci_type(pci);
	dst_addr = pci_destination(pci);
	qos_id = pci_qos_id(pci);
	if (!pdu_type_is_ok(pdu_type) ||
		!is_address_ok(dst_addr)  ||
		!is_qos_id_ok(qos_id)) {
		LOG_ERR("Wrong PDU type (%u), dst address (%u) or qos_id (%u)",
			pdu_type, dst_addr, qos_id);
		pdu_destroy(pdu);
		return -1;
	}

	/* pdu is not for me */
	if (rmt->address != dst_addr) {
		if (!dst_addr)
			return process_mgmt_pdu_ni(rmt, from, pdu);
		else {
			if (sdup_dec_check_lifetime_limit(n1_port->sdup_port, pdu)) {
				LOG_ERR("Lifetime of PDU reached dropping PDU!");
				pdu_destroy(pdu);
				return -1;
			}
			/* Forward PDU */
			/*NOTE: we could reuse the serialized pdu when
			 * forwarding */
			return rmt_send(rmt,
					pdu);
		}
	} else {
		/* pdu is for me */
		switch (pdu_type) {
		case PDU_TYPE_MGMT:
			return process_mgmt_pdu_ni(rmt, from, pdu);

		case PDU_TYPE_CACK:
		case PDU_TYPE_SACK:
		case PDU_TYPE_NACK:
		case PDU_TYPE_FC:
		case PDU_TYPE_ACK:
		case PDU_TYPE_ACK_AND_FC:
		case PDU_TYPE_DT:
		/*
		 * (FUTURE)
		 *
		 * enqueue PDU in pdus_dt[dest-addr, qos-id]
		 * don't process it now ...
		 */
			return process_dt_pdu(rmt, from, pdu);

		default:
			LOG_ERR("Unknown PDU type %d", pdu_type);
			pdu_destroy(pdu);
			return -1;
		}
	}
}
EXPORT_SYMBOL(rmt_receive);

struct rmt *rmt_create(struct ipcp_instance *parent,
		       struct kfa *kfa,
		       struct efcp_container *efcpc,
		       struct sdup *sdup)
{
	struct rmt *tmp;

	if (!parent || !kfa || !efcpc) {
		LOG_ERR("Bogus input parameters");
		return NULL;
	}

	tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
	if (!tmp)
		return NULL;

	tmp->address = address_bad();
	tmp->parent = parent;
	tmp->kfa = kfa;
	tmp->efcpc = efcpc;
	tmp->sdup      = sdup;
	rina_component_init(&tmp->base);
	tmp->pff = pff_create();
	if (!tmp->pff) {
		rmt_destroy(tmp);
		return NULL;
	}

	tmp->n1_ports = n1pmap_create();
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
