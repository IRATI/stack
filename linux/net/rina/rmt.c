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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
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
#include <linux/interrupt.h>

#define RINA_PREFIX "rmt"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "du.h"
#include "rmt.h"
#include "pft.h"
#include "efcp-utils.h"
#include "serdes.h"
#include "pdu-ser.h"
#include "rmt-ps.h"

#define rmap_hash(T, K) hash_min(K, HASH_BITS(T))

static struct policy_set_list policy_sets = {
        .head = LIST_HEAD_INIT(policy_sets.head)
};


static struct rmt_n1_port * n1_port_create(port_id_t id,
                                           struct ipcp_instance * n1_ipcp)
{
        struct rmt_n1_port * tmp;

        ASSERT(is_port_id_ok(id));

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->queue = rfifo_create();
        if (!tmp->queue) {
                rkfree(tmp);
                return NULL;
        }

        INIT_HLIST_NODE(&tmp->hlist);

        tmp->port_id = id;
        tmp->n1_ipcp = n1_ipcp;
        tmp->state   = N1_PORT_STATE_ENABLED;
        atomic_set(&tmp->n_sdus, 0);
        spin_lock_init(&tmp->lock);

        LOG_DBG("N-1 port %pK created successfully (port-id = %d)", tmp, id);

        return tmp;
}

static int n1_port_destroy(struct rmt_n1_port * n1p, bool unbind)
{
        struct ipcp_instance * n1_ipcp;

        ASSERT(n1p);
        ASSERT(n1p->queue);

        LOG_DBG("Destroying N-1 port %pK (port-id = %d)", n1p, n1p->port_id);

        hash_del(&n1p->hlist);

        if (n1p->queue) rfifo_destroy(n1p->queue, (void (*)(void *)) pdu_destroy);

        n1_ipcp = n1p->n1_ipcp;
        if (!n1_ipcp)
                LOG_ERR("N-1 port in rmt has not n1_ipcp");

        ASSERT(n1_ipcp);
        ASSERT(n1_ipcp->ops);
        ASSERT(n1_ipcp->data);
        ASSERT(n1_ipcp->ops->flow_unbinding_user_ipcp);

        if (unbind                              &&
            n1_ipcp                             &&
            n1_ipcp->ops                        &&
            n1_ipcp->data                       &&
            n1_ipcp->ops->flow_unbinding_user_ipcp) {
                if (n1_ipcp->ops->flow_unbinding_user_ipcp(n1_ipcp->data,
                                                           n1p->port_id)) {
                        LOG_ERR("Could not unbind IPCP as user of an N-1 flow");
                }
        }
        rkfree(n1p);

        return 0;
}

struct n1pmap {
        spinlock_t lock; /* FIXME: Has to be moved in the pipelines */

        DECLARE_HASHTABLE(n1_ports, 7);
};

static struct n1pmap * n1pmap_create(void)
{
        struct n1pmap * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        hash_init(tmp->n1_ports);
        spin_lock_init(&tmp->lock);

        return tmp;
}

static int n1pmap_destroy(struct n1pmap * m, bool unbind)
{
        struct rmt_n1_port * entry;
        struct hlist_node *  tmp;
        int                  bucket;

        ASSERT(m);

        hash_for_each_safe(m->n1_ports, bucket, tmp, entry, hlist) {
                ASSERT(entry);

                if (n1_port_destroy(entry, unbind)) {
                        LOG_ERR("Could not destroy entry %pK", entry);
                        return -1;
                }
        }

        rkfree(m);

        return 0;
}

static struct rmt_n1_port * n1pmap_find(struct n1pmap * m,
                                      port_id_t       id)
{
        struct rmt_n1_port *      entry;
        const struct hlist_head * head;

        ASSERT(m);

        if (!is_port_id_ok(id))
                return NULL;

        head = &m->n1_ports[rmap_hash(m->n1_ports, id)];
        hlist_for_each_entry(entry, head, hlist) {
                if (entry->port_id == id)
                        return entry;
        }

        return NULL;
}

struct pft_cache {
        port_id_t * pids;  /* Array of port_id_t */
        size_t      count; /* Entries in the pids array */
};

static int pft_cache_init(struct pft_cache * c)
{
        ASSERT(c);

        c->pids  = NULL;
        c->count = 0;

        LOG_DBG("PFT cache %pK initialized", c);

        return 0;
}

static int pft_cache_fini(struct pft_cache * c)
{
        ASSERT(c);

        if (c->count) {
                ASSERT(c->pids);
                rkfree(c->pids);
        } else {
                ASSERT(!c->pids);
        }

        LOG_DBG("PFT cache %pK destroyed", c);

        return 0;
}

struct rmt {
        struct rina_component     base;
        address_t                 address;
        struct ipcp_instance *    parent;
        struct pft *              pft;
        struct kfa *              kfa;
        struct efcp_container *   efcpc;
        struct serdes *           serdes;

        struct {
                struct tasklet_struct ingress_tasklet;
                struct n1pmap *       n1_ports;
                struct pft_cache      cache;
        } ingress;

        struct {
                struct tasklet_struct egress_tasklet;
                struct n1pmap *       n1_ports;
                struct pft_cache      cache;
        } egress;
};

struct rmt *
rmt_from_component(struct rina_component * component)
{
        return container_of(component, struct rmt, base);
}
EXPORT_SYMBOL(rmt_from_component);

int rmt_select_policy_set(struct rmt * rmt,
                          const string_t * path,
                          const string_t * name)
{
        if (path && strcmp(path, "")) {
                LOG_ERR("This component has no selectable subcomponents");
                return -1;
        }

        return base_select_policy_set(&rmt->base, &policy_sets, name);
}
EXPORT_SYMBOL(rmt_select_policy_set);

int rmt_set_policy_set_param(struct rmt * rmt,
                             const char * path,
                             const char * name,
                             const char * value)
{
        struct rmt_ps *ps;
        int ret = -1;

        if (!rmt || !path || !name || !value) {
                LOG_ERRF("NULL arguments %p %p %p %p", rmt, path, name, value);
                return -1;
        }

        LOG_DBG("set-policy-set-param '%s' '%s' '%s'", path, name, value);

        if (strcmp(path, "") == 0) {
                /* The request addresses this RMT instance. */
                rcu_read_lock();
                ps = container_of(rcu_dereference(rmt->base.ps), struct rmt_ps, base);
                if (!ps) {
                        LOG_ERR("No policy-set selected for this RMT");
                } else if (strcmp(name, "max_q") == 0) {
                        ret = kstrtoint(value, 10, &ps->max_q);
                } else {
                        LOG_ERR("Unknown RMT parameter policy '%s'", name);
                }
                rcu_read_unlock();
        } else {
                ret = base_set_policy_set_param(&rmt->base, path, name, value);
        }

        return ret;
}
EXPORT_SYMBOL(rmt_set_policy_set_param);

int rmt_destroy(struct rmt * instance)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        if (instance->ingress.n1_ports)
                n1pmap_destroy(instance->ingress.n1_ports, false);

        tasklet_kill(&instance->ingress.ingress_tasklet);
        pft_cache_fini(&instance->ingress.cache);

        if (instance->egress.n1_ports)
                n1pmap_destroy(instance->egress.n1_ports, true);
        tasklet_kill(&instance->egress.egress_tasklet);
        pft_cache_fini(&instance->egress.cache);

        if (instance->pft)            pft_destroy(instance->pft);
        if (instance->serdes)         serdes_destroy(instance->serdes);

        rina_component_fini(&instance->base);

        rkfree(instance);

        LOG_DBG("Instance %pK finalized successfully", instance);

        return 0;
}
EXPORT_SYMBOL(rmt_destroy);

int rmt_address_set(struct rmt * instance,
                    address_t    address)
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

int rmt_dt_cons_set(struct rmt *     instance,
                    struct dt_cons * dt_cons)
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

static int n1_port_write(struct serdes * serdes,
                         struct rmt_n1_port * n1_port,
                         struct pdu * pdu)
{
        struct sdu *           sdu;
        struct pdu_ser *       pdu_ser;
        port_id_t              port_id;
        struct buffer *        buffer;
        struct ipcp_instance * n1_ipcp;
        struct pci *           pci;
        size_t                 ttl;

        ASSERT(n1_port);
        ASSERT(serdes);

        if (!pdu) {
                LOG_DBG("No PDU to work in this queue ...");
                return -1;
        }

        port_id = n1_port->port_id;
        n1_ipcp = n1_port->n1_ipcp;

        pci = 0;
        ttl = 0;

#ifdef CONFIG_RINA_IPCPS_TTL
        pci = pdu_pci_get_rw(pdu);
        if (!pci) {
                LOG_ERR("Cannot get PCI");
                pdu_destroy(pdu);
                return -1;
        }

        LOG_DBG("TTL to start with is %d", CONFIG_RINA_IPCPS_TTL_DEFAULT);

        if (pci_ttl_set(pci, CONFIG_RINA_IPCPS_TTL_DEFAULT)) {
                LOG_ERR("Could not set TTL");
                pdu_destroy(pdu);
                return -1;
        }
#endif

        pdu_ser = pdu_serialize_ni(serdes, pdu);
        if (!pdu_ser) {
                LOG_ERR("Error creating serialized PDU");
                pdu_destroy(pdu);
                return -1;
        }

        buffer = pdu_ser_buffer(pdu_ser);
        if (!buffer_is_ok(buffer)) {
                LOG_ERR("Buffer is not okay");
                pdu_ser_destroy(pdu_ser);
                return -1;
        }

        if (pdu_ser_buffer_disown(pdu_ser)) {
                LOG_ERR("Could not disown buffer");
                        pdu_ser_destroy(pdu_ser);
                        return -1;
        }

        pdu_ser_destroy(pdu_ser);

        sdu = sdu_create_buffer_with_ni(buffer);
        if (!sdu) {
                LOG_ERR("Error creating SDU from serialized PDU, "
                        "dropping PDU!");
                buffer_destroy(buffer);
                return -1;
        }

        LOG_DBG("Gonna send SDU to port-id %d", port_id);
        if (n1_ipcp->ops->sdu_write(n1_ipcp->data,port_id, sdu)) {
                LOG_ERR("Couldn't write SDU to N-1 IPCP");
                return -1;
        }

        atomic_dec(&n1_port->n_sdus);
        return 0;
}

static void send_worker(unsigned long o)
{
        struct rmt *         tmp;
        struct rmt_n1_port * entry;
        struct hlist_node *  ntmp;
        int                  bucket;
        int                  pending = 0;
        struct rfifo *       queue;
        struct rmt_ps *      ps;
        bool                 sched_restart = true;

        LOG_DBG("Send worker called");

        tmp = (struct rmt *) o;
        if (!tmp) {
                LOG_ERR("No instance passed to send worker !!!");
                return;
        }

        rcu_read_lock();
        ps = container_of(rcu_dereference(tmp->base.ps),
                          struct rmt_ps,
                          base);
        if (!ps) {
                rcu_read_unlock();
                LOG_ERR("No ps in send_worker");
                return;
        }
        rcu_read_unlock();

        spin_lock(&tmp->egress.n1_ports->lock);
        hash_for_each_safe(tmp->egress.n1_ports->n1_ports,
                           bucket,
                           ntmp,
                           entry,
                           hlist) {
                struct pdu * pdu;

                ASSERT(entry);

                spin_unlock(&tmp->egress.n1_ports->lock);

                spin_lock(&entry->lock);
                if (entry->state == N1_PORT_STATE_DISABLED ||
                    atomic_read(&entry->n_sdus) == 0) {
                        if (atomic_read(&entry->n_sdus) > 0)
                                pending++;
                        spin_unlock(&entry->lock);
                        spin_lock(&tmp->egress.n1_ports->lock);
                        LOG_DBG("Port state is DISABLED or empty");
                        continue;
                }

                for (;;) {

                        queue = ps->rmt_scheduling_policy_tx(ps,
                                                             entry,
                                                             sched_restart);
                        if (!queue) {
                                /* No more queues to serve. */
                                break;
                        }
                        sched_restart = false;

                        pdu = (struct pdu *) rfifo_pop(queue);
                        if (!pdu) {
                                spin_unlock(&entry->lock);
                                LOG_DBG("No PDU to work in this queue ...");
                                break;
                        }
                        entry->state = N1_PORT_STATE_BUSY;
                        spin_unlock(&entry->lock);

                        n1_port_write(tmp->serdes, entry, pdu);

                        spin_lock(&entry->lock);
                        if (atomic_read(&entry->n_sdus) <= 0) {
                                atomic_set(&entry->n_sdus, 0);
                                if (entry->state != N1_PORT_STATE_DISABLED)
                                        entry->state = N1_PORT_STATE_ENABLED;
                        } else {
                                pending++;
                        }
                }
                spin_unlock(&entry->lock);
                spin_lock(&tmp->egress.n1_ports->lock);
        }
        spin_unlock(&tmp->egress.n1_ports->lock);

        if (pending) {
                LOG_DBG("RMT worker scheduling tasklet...");
                tasklet_hi_schedule(&tmp->egress.egress_tasklet);
        }

        return;
}

int rmt_send_port_id(struct rmt * instance,
                     port_id_t    id,
                     struct pdu * pdu)
{
        struct n1pmap *      out_n1_ports;
        struct rmt_n1_port * out_n1_port;
        unsigned long        flags;
        struct rmt_ps *      ps;

        if (!pdu_is_ok(pdu)) {
                LOG_ERR("Bogus PDU passed");
                return -1;
        }
        if (!instance) {
                LOG_ERR("Bogus RMT passed");
                pdu_destroy(pdu);
                return -1;
        }

        if (!instance->egress.n1_ports) {
                LOG_ERR("No n1_ports to push into");
                pdu_destroy(pdu);
                return -1;
        }

        out_n1_ports = instance->egress.n1_ports;

        spin_lock_irqsave(&out_n1_ports->lock, flags);
        out_n1_port = n1pmap_find(out_n1_ports, id);
        if (!out_n1_port) {
                spin_unlock_irqrestore(&out_n1_ports->lock, flags);
                pdu_destroy(pdu);
                return -1;
        }
        spin_unlock_irqrestore(&out_n1_ports->lock, flags);

        /* FIXME-POLICY: Check if this is the correct place for
         * rmt_q_monitor_policy_tx */
        rcu_read_lock();
        ps = container_of(rcu_dereference(instance->base.ps), struct rmt_ps, base);
        if (ps) {
                /* MaxQPolicy hook. */
                if (ps->max_q_policy_tx &&
                        rfifo_length(out_n1_port->queue) >= ps->max_q) {
                        ps->max_q_policy_tx(ps, pdu, out_n1_port->queue);
                }

                /* RMTQMonitorPolicy hook. */
                if (ps->rmt_q_monitor_policy_tx) {
                        ps->rmt_q_monitor_policy_tx(ps, pdu, out_n1_port->queue);
                }
        }
        rcu_read_unlock();
        /* */
        spin_lock_irqsave(&out_n1_port->lock, flags);
        atomic_inc(&out_n1_port->n_sdus);
        if (out_n1_port->state == N1_PORT_STATE_ENABLED &&
            atomic_read(&out_n1_port->n_sdus) == 1) {
                int ret = 0;
                out_n1_port->state = N1_PORT_STATE_BUSY;
                spin_unlock_irqrestore(&out_n1_port->lock, flags);
                if (n1_port_write(instance->serdes, out_n1_port, pdu))
                        ret = -1;

                spin_lock_irqsave(&out_n1_port->lock, flags);
                if (atomic_read(&out_n1_port->n_sdus) <= 0) {
                        atomic_set(&out_n1_port->n_sdus, 0);
                        if (out_n1_port->state != N1_PORT_STATE_DISABLED) {
                                LOG_DBG("Sent and enabling port");
                                out_n1_port->state = N1_PORT_STATE_ENABLED;
                        }
                } else {
                        LOG_DBG("Sent and scheduling cause there are more"
                                " pdus in the port");
                        tasklet_hi_schedule(&instance->egress.egress_tasklet);
                }
                spin_unlock_irqrestore(&out_n1_port->lock, flags);
                return ret;
        } else if (out_n1_port->state == N1_PORT_STATE_BUSY) {
                if (rfifo_push_ni(out_n1_port->queue, pdu)) {
                        spin_unlock_irqrestore(&out_n1_port->lock, flags);
                        pdu_destroy(pdu);
                        return -1;
                }
                LOG_DBG("Port was busy, enqueuing PDU..");
        } else {
                LOG_ERR("Port state deallocated, discarding PDU");
                pdu_destroy(pdu);
                return -1;
        }

        spin_unlock_irqrestore(&out_n1_port->lock, flags);

        return 0;
}
EXPORT_SYMBOL(rmt_send_port_id);

int rmt_send(struct rmt * instance,
             address_t    address,
             qos_id_t     qos_id,
             struct pdu * pdu)
{
        int          i;

        if (!instance) {
                LOG_ERR("Bogus RMT passed");
                return -1;
        }
        if (!pdu) {
                LOG_ERR("Bogus PDU passed");
                return -1;
        }

        if (pft_nhop(instance->pft,
                     address,
                     qos_id,
                     &(instance->egress.cache.pids),
                     &(instance->egress.cache.count))) {
                LOG_ERR("Cannot get the NHOP for this PDU");

                pdu_destroy(pdu);
                return -1;
        }

        if (instance->egress.cache.count == 0) {
                LOG_WARN("No NHOP for this PDU ...");

                pdu_destroy(pdu);
                return 0;
        }

        /*
         * FIXME:
         *   pdu -> pci-> qos-id | cep_id_t -> connection -> qos-id (former)
         *   address + qos-id (pdu-fwd-t) -> port-id
         */

        for (i = 0; i < instance->egress.cache.count; i++) {
                port_id_t    pid;
                struct pdu * p;

                pid = instance->egress.cache.pids[i];

                if (instance->egress.cache.count > 1)
                        p = pdu_dup(pdu);
                else
                        p = pdu;

                LOG_DBG("Gonna send PDU to port-id: %d", pid);
                if (rmt_send_port_id(instance, pid, p))
                        LOG_ERR("Failed to send a PDU to port-id %d", pid);
        }

        return 0;
}
EXPORT_SYMBOL(rmt_send);

static int __queue_send_add(struct rmt * instance,
                            port_id_t    id,
                            struct ipcp_instance * n1_ipcp)
{
        struct rmt_n1_port * tmp;

        tmp = n1_port_create(id, n1_ipcp);
        if (!tmp)
                return -1;

        hash_add(instance->egress.n1_ports->n1_ports, &tmp->hlist, id);

        LOG_DBG("Added send queue to rmt instance %pK for port-id %d",
                instance, id);

        return 0;
}

static int rmt_n1_port_send_add(struct rmt * instance,
                              port_id_t    id,
                              struct ipcp_instance * n1_ipcp)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        if (!is_port_id_ok(id)) {
                LOG_ERR("Wrong port-id %d", id);
                return -1;
        }

        if (!instance->egress.n1_ports) {
                LOG_ERR("Invalid RMT");
                return -1;
        }

        if (n1pmap_find(instance->egress.n1_ports, id)) {
                LOG_ERR("Queue already exists");
                return -1;
        }

        return __queue_send_add(instance, id, n1_ipcp);
}

int rmt_enable_port_id(struct rmt * instance,
                       port_id_t    id)
{
        struct rmt_n1_port * n1_port;

        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        if (!is_port_id_ok(id)) {
                LOG_ERR("Wrong port-id %d", id);
                return -1;
        }

        if (!instance->egress.n1_ports) {
                LOG_ERR("Invalid RMT");
                return -1;
        }

        spin_lock(&instance->egress.n1_ports->lock);
        n1_port = n1pmap_find(instance->egress.n1_ports, id);
        if (!n1_port) {
                spin_unlock(&instance->egress.n1_ports->lock);
                LOG_ERR("No queue associated to this port-id, %d", id);
                return -1;
        }
        spin_unlock(&instance->egress.n1_ports->lock);

        spin_lock(&n1_port->lock);
        if (n1_port->state != N1_PORT_STATE_DISABLED) {
                spin_unlock(&n1_port->lock);
                LOG_DBG("Nothing to do for port-id %d", id);
                return 0;
        }
        n1_port->state = N1_PORT_STATE_ENABLED;
        spin_unlock(&n1_port->lock);

        LOG_DBG("Changed state to ENABLED");

        return 0;
}
EXPORT_SYMBOL(rmt_enable_port_id);

int rmt_disable_port_id(struct rmt * instance,
                        port_id_t    id)
{
        struct rmt_n1_port * n1_port;

        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        if (!is_port_id_ok(id)) {
                LOG_ERR("Wrong port-id %d", id);
                return -1;
        }

        if (!instance->egress.n1_ports) {
                LOG_ERR("Invalid RMT");
                return -1;
        }

        spin_lock(&instance->egress.n1_ports->lock);
        n1_port = n1pmap_find(instance->egress.n1_ports, id);
        if (!n1_port) {
                spin_unlock(&instance->egress.n1_ports->lock);
                LOG_ERR("No n1_port associated to this port-id, %d", id);
                return -1;
        }
        spin_unlock(&instance->egress.n1_ports->lock);

        spin_lock(&n1_port->lock);
        if (n1_port->state == N1_PORT_STATE_DISABLED) {
                spin_unlock(&n1_port->lock);
                LOG_DBG("Nothing to do for port-id %d", id);
                return 0;
        }
        n1_port->state = N1_PORT_STATE_DISABLED;
        spin_unlock(&n1_port->lock);
        LOG_DBG("Changed state to DISABLED");

        return 0;
}
EXPORT_SYMBOL(rmt_disable_port_id);

static int rmt_n1_port_send_delete(struct rmt * instance,
                                 port_id_t    id)
{
        struct rmt_n1_port * q;

        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        if (!instance->egress.n1_ports) {
                LOG_ERR("Bogus egress instance passed");
                return -1;
        }

        if (!is_port_id_ok(id)) {
                LOG_ERR("Wrong port-id %d", id);
                return -1;
        }

        q = n1pmap_find(instance->egress.n1_ports, id);
        if (!q) {
                LOG_ERR("Queue does not exist");
                return -1;
        }

        return n1_port_destroy(q, false);
}

static int __queue_recv_add(struct rmt * instance,
                            port_id_t    id,
                            struct ipcp_instance * n1_ipcp)
{
        struct rmt_n1_port * tmp;

        tmp = n1_port_create(id, n1_ipcp);
        if (!tmp)
                return -1;

        hash_add(instance->ingress.n1_ports->n1_ports, &tmp->hlist, id);

        LOG_DBG("Added receive queue to rmt instance %pK for port-id %d",
                instance, id);

        return 0;
}

static int rmt_n1_port_recv_add(struct rmt *                instance,
                              port_id_t                   id,
                              struct ipcp_instance * n1_ipcp)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        if (!is_port_id_ok(id)) {
                LOG_ERR("Wrong port-id %d", id);
                return -1;
        }

        if (!instance->ingress.n1_ports) {
                LOG_ERR("Invalid RMT");
                return -1;
        }

        if (n1pmap_find(instance->ingress.n1_ports, id)) {
                LOG_ERR("Queue already exists");
                return -1;
        }

        return __queue_recv_add(instance, id, n1_ipcp);
}

static int rmt_n1_port_recv_delete(struct rmt * instance,
                                 port_id_t    id)
{
        struct rmt_n1_port * q;

        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        if (!instance->ingress.n1_ports) {
                LOG_ERR("Bogus egress instance passed");
                return -1;
        }

        if (!is_port_id_ok(id)) {
                LOG_ERR("Wrong port-id %d", id);
                return -1;
        }

        q = n1pmap_find(instance->ingress.n1_ports, id);
        if (!q) {
                LOG_ERR("Queue does not exist");
                return -1;
        }

        return n1_port_destroy(q, false);
}

int rmt_n1port_bind(struct rmt * instance,
                    port_id_t    id,
                    struct ipcp_instance * n1_ipcp)
{
        if (rmt_n1_port_send_add(instance, id, n1_ipcp))
                return -1;

        if (rmt_n1_port_recv_add(instance, id, n1_ipcp)) {
                rmt_n1_port_send_delete(instance, id);
                return -1;
        }

        return 0;
}
EXPORT_SYMBOL(rmt_n1port_bind);

int rmt_n1port_unbind(struct rmt * instance,
                      port_id_t    id)
{
        int retval = 0;

        if (rmt_n1_port_send_delete(instance, id))
                retval = -1;

        if (rmt_n1_port_recv_delete(instance, id))
                retval = -1;

        return retval;
}
EXPORT_SYMBOL(rmt_n1port_unbind);

/* FIXME: This function is only used in testig and they are disabled */
#if 0
static struct pci * sdu_pci_copy(const struct sdu * sdu)
{
        if (!sdu_is_ok(sdu))
                return NULL;

        return pci_create_from(buffer_data_ro(sdu_buffer_ro(sdu)));
}
#endif
#if 0
static int process_mgmt_pdu(struct rmt * rmt,
                            port_id_t    port_id,
                            struct pdu * pdu)
{
        struct buffer * buffer;
        struct sdu *    sdu;

        ASSERT(rmt);
        ASSERT(is_port_id_ok(port_id));
        ASSERT(pdu_is_ok(pdu));

        buffer = pdu_buffer_get_rw(pdu);
        if (!buffer_is_ok(buffer)) {
                LOG_ERR("PDU has no buffer ???");
                return -1;
        }

        sdu = sdu_create_buffer_with(buffer);
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
#endif
static int process_mgmt_pdu_ni(struct rmt * rmt,
                               port_id_t    port_id,
                               struct pdu * pdu)
{
        struct buffer * buffer;
        struct sdu *    sdu;

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

static int process_dt_pdu(struct rmt * rmt,
                          port_id_t    port_id,
                          struct pdu * pdu)
{
        address_t  dst_addr;
        cep_id_t   c;
        pdu_type_t pdu_type;

        ASSERT(rmt);
        ASSERT(is_port_id_ok(port_id));
        ASSERT(pdu_is_ok(pdu));

        /* (FUTURE) Address and qos-id are the same, do a single match only */

        dst_addr = pci_destination(pdu_pci_get_ro(pdu));
        if (!is_address_ok(dst_addr)) {
                LOG_ERR("PDU has Wrong destination address");
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

static int forward_pdu(struct rmt * rmt,
                       port_id_t    port_id,
                       address_t    dst_addr,
                       qos_id_t     qos_id,
                       struct pdu * pdu)
{
        int                i;
        struct sdu *       sdu;
        struct pdu_ser *   pdu_ser;
        struct buffer *    buffer;
        struct serdes *    serdes;
        struct rmt_n1_port * queue;

        if (!is_address_ok(dst_addr)) {
                LOG_ERR("PDU has Wrong destination address");
                pdu_destroy(pdu);
                return -1;
        }

        if (!is_qos_id_ok(qos_id)) {
                LOG_ERR("QOS id is wrong...");
                pdu_destroy(pdu);
                return -1;
        }

        serdes = rmt->serdes;
        ASSERT(serdes);

        pdu_ser = pdu_serialize_ni(serdes, pdu);
        if (!pdu_ser) {
                LOG_ERR("Error creating serialized PDU");
                pdu_destroy(pdu);
                return -1;
        }

        buffer = pdu_ser_buffer(pdu_ser);
        if (!buffer_is_ok(buffer)) {
                LOG_ERR("Buffer is not okay");
                pdu_ser_destroy(pdu_ser);
                return -1;
        }

        if (pdu_ser_buffer_disown(pdu_ser)) {
                LOG_ERR("Could not disown buffer");
                pdu_ser_destroy(pdu_ser);
                return -1;
        }

        pdu_ser_destroy(pdu_ser);

        sdu = sdu_create_buffer_with_ni(buffer);
        if (!sdu) {
                LOG_ERR("Error creating SDU from serialized PDU, "
                        "dropping PDU!");
                buffer_destroy(buffer);
                return -1;
        }

        ASSERT(rmt->address != dst_addr);

        if (pft_nhop(rmt->pft,
                     dst_addr,
                     qos_id,
                     &(rmt->ingress.cache.pids),
                     &(rmt->ingress.cache.count))) {
                LOG_ERR("Cannot get NHOP");
                sdu_destroy(sdu);
                return -1;
        }

        if (rmt->ingress.cache.count > 0) {
                for (i = 1; i < rmt->ingress.cache.count; i++) {
                        struct sdu * tmp;

                        tmp = sdu_dup(sdu);
                        if (!tmp)
                                continue;

                        queue = n1pmap_find(rmt->ingress.n1_ports,
                                            rmt->ingress.cache.pids[i]);
                        if (!queue)
                                continue;

                        ASSERT(queue->n1_ipcp);
                        ASSERT(queue->n1_ipcp->ops->sdu_write);
                        if (queue->n1_ipcp->ops->sdu_write(queue->n1_ipcp->data,
                                                           rmt->ingress.cache.pids[i],
                                                           tmp))
                                LOG_ERR("Cannot write SDU to N-1 IPCP port-id %d",
                                        rmt->ingress.cache.pids[i]);
                }

                queue = n1pmap_find(rmt->ingress.n1_ports,
                                    rmt->ingress.cache.pids[0]);
                if (!queue)
                        /* FIXME: As in the continue before, some port could not
                         * be used */
                        return 0;

                ASSERT(queue->n1_ipcp);
                ASSERT(queue->n1_ipcp->ops->sdu_write);
                if (queue->n1_ipcp->ops->sdu_write(queue->n1_ipcp->data,
                                                   rmt->ingress.cache.pids[0],
                                                   sdu))
                        LOG_ERR("Cannot write SDU to N-1 IPCP port-id %d",
                                rmt->ingress.cache.pids[0]);
        } else {
                LOG_DBG("Could not forward PDU");
                sdu_destroy(sdu);
        }

        return 0;
}

static int receive_direct(struct rmt       * tmp,
                          struct rmt_n1_port * entry,
                          struct sdu * sdu)
{
        port_id_t           port_id;
        pdu_type_t          pdu_type;
        const struct pci *  pci;
        struct pdu_ser *    pdu_ser;
        struct pdu *        pdu;
        address_t           dst_addr;
        qos_id_t            qos_id;
        struct serdes *     serdes;
        struct buffer *     buf;

        port_id = entry->port_id;

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

        serdes = tmp->serdes;
        ASSERT(serdes);

        pdu = pdu_deserialize_ni(serdes, pdu_ser);
        if (!pdu) {
                LOG_ERR("Failed to deserialize PDU!");
                pdu_ser_destroy(pdu_ser);
                return -1;
        }

        pci = pdu_pci_get_ro(pdu);
        if (!pci) {
                LOG_ERR("No PCI to work with, dropping SDU!");
                pdu_destroy(pdu);
                return -1;
        }

        ASSERT(pdu_is_ok(pdu));

        pdu_type = pci_type(pci);
        dst_addr = pci_destination(pci);
        qos_id   = pci_qos_id(pci);
        if (!pdu_type_is_ok(pdu_type) ||
            !is_address_ok(dst_addr)  ||
            !is_qos_id_ok(qos_id)) {
            LOG_ERR("Wrong PDU type, dst address or qos_id,"
                    " dropping SDU! %u, %u, %u",
                    pdu_type, dst_addr, qos_id);
                pdu_destroy(pdu);
                return -1;
        }

        /* pdu is not for me */
        if (tmp->address != dst_addr) {
                if (!dst_addr) {
                        return process_mgmt_pdu_ni(tmp, port_id, pdu);
                } else {
                        return forward_pdu(tmp,
                                           port_id,
                                           dst_addr,
                                           qos_id,
                                           pdu);
                }
        } else {
                /* pdu is for me */
                switch (pdu_type) {
                case PDU_TYPE_MGMT:
                        return process_mgmt_pdu_ni(tmp, port_id, pdu);

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
                        return process_dt_pdu(tmp, port_id, pdu);

               default:
                        LOG_ERR("Unknown PDU type %d", pdu_type);
                        pdu_destroy(pdu);
                        return -1;
                }
       }
}

static void receive_worker(unsigned long o)
{
        struct rmt *         tmp;
        struct rfifo *       queue;
        struct rmt_ps *      ps;
        bool                 sched_restart = true;
        struct rmt_n1_port * entry;
        int                  bucket;
        struct hlist_node *  ntmp;

        port_id_t            port_id;
        pdu_type_t           pdu_type;
        const struct pci *   pci;
        struct pdu_ser *     pdu_ser;
        struct pdu *         pdu;
        address_t            dst_addr;
        qos_id_t             qos_id;
        struct serdes *      serdes;
        struct buffer *      buf;
        struct sdu *         sdu;

        LOG_DBG("RMT tasklet receive worker called");

        tmp = (struct rmt *) o;
        if (!tmp) {
                LOG_ERR("No instance passed to receive worker !!!");
                return;
        }

        rcu_read_lock();
        ps = container_of(rcu_dereference(tmp->base.ps),
                          struct rmt_ps,
                          base);
        if (!ps) {
                LOG_ERR("No ps in the receive_worker");
                rcu_read_unlock();
                return;
        }
        rcu_read_unlock();

        spin_lock(&tmp->ingress.n1_ports->lock);
        hash_for_each_safe(tmp->ingress.n1_ports->n1_ports,
                           bucket,
                           ntmp,
                           entry,
                           hlist) {
                ASSERT(entry);

                for (;;) {
                        queue = ps->rmt_scheduling_policy_rx(ps,
                                        entry, sched_restart);
                        if (!queue) {
                                // No more queues to serve.
                                break;
                        }
                        sched_restart = false;

                        sdu = (struct sdu *) rfifo_pop(queue);
                        if (!sdu) {
                                LOG_DBG("No SDU to work with in this queue");
                                break;
                        }

                        atomic_dec(&entry->n_sdus);

                        port_id = entry->port_id;
                        spin_unlock(&tmp->ingress.n1_ports->lock);

                        buf = sdu_buffer_rw(sdu);
                        if (!buf) {
                                LOG_DBG("No buffer present");
                                sdu_destroy(sdu);
                                spin_lock(&tmp->ingress.n1_ports->lock);
                                break;
                        }

                        if (sdu_buffer_disown(sdu)) {
                                LOG_DBG("Could not disown SDU");
                                sdu_destroy(sdu);
                                spin_lock(&tmp->ingress.n1_ports->lock);
                                break;
                        }

                        sdu_destroy(sdu);

                        pdu_ser = pdu_ser_create_buffer_with_ni(buf);
                        if (!pdu_ser) {
                                LOG_DBG("No ser PDU to work with");
                                buffer_destroy(buf);
                                spin_lock(&tmp->ingress.n1_ports->lock);
                                break;
                        }

                        serdes = tmp->serdes;
                        ASSERT(serdes);

                        pdu = pdu_deserialize_ni(serdes, pdu_ser);
                        if (!pdu) {
                                LOG_ERR("Failed to deserialize PDU!");
                                pdu_ser_destroy(pdu_ser);
                                spin_lock(&tmp->ingress.n1_ports->lock);
                                break;
                        }

                        pci = pdu_pci_get_ro(pdu);
                        if (!pci) {
                                LOG_ERR("No PCI to work with, dropping SDU!");
                                pdu_destroy(pdu);
                                spin_lock(&tmp->ingress.n1_ports->lock);
                                break;
                        }

                        ASSERT(pdu_is_ok(pdu));

                        pdu_type = pci_type(pci);
                        dst_addr = pci_destination(pci);
                        qos_id   = pci_qos_id(pci);
                        if (!pdu_type_is_ok(pdu_type) ||
                            !is_address_ok(dst_addr)  ||
                            !is_qos_id_ok(qos_id)) {
                                LOG_ERR("Wrong PDU type, dst address or qos_id,"
                                        " dropping SDU! %u, %u, %u",
                                        pdu_type, dst_addr, qos_id);
                                pdu_destroy(pdu);
                                spin_lock(&tmp->ingress.n1_ports->lock);
                                break;
                        }

                        /* pdu is not for me */
                        if (tmp->address != dst_addr) {
                                if (!dst_addr) {
                                        process_mgmt_pdu_ni(tmp, port_id, pdu);
                                } else {
                                        forward_pdu(tmp,
                                                    port_id,
                                                    dst_addr,
                                                    qos_id,
                                                    pdu);
                                }
                        } else {
                                /* pdu is for me */
                                switch (pdu_type) {
                                case PDU_TYPE_MGMT:
                                        process_mgmt_pdu_ni(tmp, port_id, pdu);
                                        break;

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
                                        process_dt_pdu(tmp, port_id, pdu);
                                        LOG_DBG("Finishing  process_dt_sdu");
                                        break;

                                default:
                                        LOG_ERR("Unknown PDU type %d", pdu_type);
                                        pdu_destroy(pdu);
                                        break;
                                }
                        }
                        if (atomic_read(&entry->n_sdus) <= 0)
                                atomic_set(&entry->n_sdus, 0);
                        spin_lock(&tmp->ingress.n1_ports->lock);
                }
        }

        spin_unlock(&tmp->ingress.n1_ports->lock);
        tasklet_hi_schedule(&tmp->ingress.ingress_tasklet);

        return;
}

int rmt_receive(struct rmt * instance,
                struct sdu * sdu,
                port_id_t    from)
{
        struct rmt_ps *      ps;
        struct n1pmap *      in_n1_ports;
        struct rmt_n1_port * in_n1_port;
        unsigned long        flags;
        bool                 sched_restart = true;
        struct rfifo *       queue = NULL;

        if (!sdu_is_ok(sdu)) {
                LOG_ERR("Bogus SDU passed");
                return -1;
        }
        if (!instance) {
                LOG_ERR("No RMT passed");
                sdu_destroy(sdu);
                return -1;
        }
        if (!is_port_id_ok(from)) {
                LOG_ERR("Wrong port-id %d", from);
                sdu_destroy(sdu);
                return -1;
        }
        if (!instance->ingress.n1_ports) {
                LOG_ERR("No ingress n1_ports in RMT instance %pK", instance);
                sdu_destroy(sdu);
                return -1;
        }

        in_n1_ports = instance->ingress.n1_ports;

        spin_lock_irqsave(&in_n1_ports->lock, flags);
        in_n1_port = n1pmap_find(in_n1_ports, from);
        if (!in_n1_port) {
                spin_unlock_irqrestore(&in_n1_ports->lock, flags);
                sdu_destroy(sdu);
                return -1;
        }
        spin_unlock_irqrestore(&in_n1_ports->lock, flags);
        spin_lock_irqsave(&in_n1_port->lock, flags);

        rcu_read_lock();
        ps = container_of(rcu_dereference(instance->base.ps),
                          struct rmt_ps,
                          base);
        if (!ps) {
                rcu_read_unlock();
                spin_unlock_irqrestore(&in_n1_port->lock, flags);
                LOG_ERR("No ps in rmt_receive");
                return -1;
        }

        if (ps->rmt_scheduling_policy_rx)
                queue = ps->rmt_scheduling_policy_rx(ps,
                                                     in_n1_port,
                                                     sched_restart);
        if (!queue) {
                rcu_read_unlock();
                spin_unlock_irqrestore(&in_n1_port->lock, flags);
                LOG_ERR("scheduling policy return no queue to check while"
                        "receiving");
                return -1;
        }
        rcu_read_unlock();

        sched_restart = false;

        if (rfifo_is_empty(queue)) {
                spin_unlock_irqrestore(&in_n1_port->lock, flags);
                LOG_DBG("RMT sent directly to DTP");
                return receive_direct(instance, in_n1_port, sdu);
        }

        /* FIXME-POLICY: check if this is the correct place for
         * rmt_q_monitor policy */
        rcu_read_lock();
        ps = container_of(rcu_dereference(instance->base.ps),
                          struct rmt_ps,
                          base);
        if (ps) {
                /* MaxQPolicy hook. */
                if (ps->max_q_policy_rx && rfifo_length(queue) >= ps->max_q) {
                        ps->max_q_policy_rx(ps, sdu, queue);
                }

                /* RMTQMonitorPolicy hook. */
                if (ps->rmt_q_monitor_policy_rx) {
                        ps->rmt_q_monitor_policy_rx(ps, sdu, queue);
                }
        }
        rcu_read_unlock();

        if (rfifo_push_ni(queue, sdu)) {
                spin_unlock_irqrestore(&in_n1_port->lock, flags);
                sdu_destroy(sdu);
                return -1;
        }
        atomic_inc(&in_n1_port->n_sdus);
        spin_unlock_irqrestore(&in_n1_port->lock, flags);

        tasklet_hi_schedule(&instance->ingress.ingress_tasklet);

        LOG_DBG("RMT tasklet scheduled");

        return 0;
}
EXPORT_SYMBOL(rmt_receive);

struct rmt * rmt_create(struct ipcp_instance *  parent,
                        struct kfa *            kfa,
                        struct efcp_container * efcpc)
{
        struct rmt * tmp;

        if (!parent || !kfa || !efcpc) {
                LOG_ERR("Bogus input parameters");
                return NULL;
        }

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->address = address_bad();
        tmp->parent  = parent;
        tmp->kfa     = kfa;
        tmp->efcpc   = efcpc;
        tmp->pft     = pft_create();
        if (!tmp->pft)
                goto fail;

        tmp->ingress.n1_ports = n1pmap_create();
        if (!tmp->ingress.n1_ports || pft_cache_init(&tmp->ingress.cache))
                goto fail;

        tmp->egress.n1_ports = n1pmap_create();
        if (!tmp->egress.n1_ports || pft_cache_init(&tmp->egress.cache))
                goto fail;

        tasklet_init(&tmp->ingress.ingress_tasklet,
                     receive_worker,
                     (unsigned long) tmp);
        tasklet_init(&tmp->egress.egress_tasklet,
                     send_worker,
                     (unsigned long) tmp);

        /* Try to select the default policy set factory. */
        rina_component_init(&tmp->base);
        if (rmt_select_policy_set(tmp, "", RINA_PS_DEFAULT_NAME)) {
                goto fail;
        }

        LOG_DBG("Instance %pK initialized successfully", tmp);
        return tmp;
fail:
        rmt_destroy(tmp);
        return NULL;
}
EXPORT_SYMBOL(rmt_create);

/* FIXME: To be rearranged */
static bool is_rmt_pft_ok(struct rmt * instance)
{ return (instance && instance->pft) ? true : false; }

int rmt_pft_add(struct rmt *       instance,
                address_t          destination,
                qos_id_t           qos_id,
                const port_id_t  * ports,
                size_t             count)
{
        return is_rmt_pft_ok(instance) ? pft_add(instance->pft,
                                                 destination,
                                                 qos_id,
                                                 ports,
                                                 count) : -1;
}
EXPORT_SYMBOL(rmt_pft_add);

int rmt_pft_remove(struct rmt *       instance,
                   address_t          destination,
                   qos_id_t           qos_id,
                   const port_id_t  * ports,
                   const size_t       count)
{
        return is_rmt_pft_ok(instance) ? pft_remove(instance->pft,
                                                    destination,
                                                    qos_id,
                                                    ports,
                                                    count) : -1;
}
EXPORT_SYMBOL(rmt_pft_remove);

int rmt_pft_dump(struct rmt *       instance,
                 struct list_head * entries)
{
        return is_rmt_pft_ok(instance) ? pft_dump(instance->pft,
                                                  entries) : -1;
}
EXPORT_SYMBOL(rmt_pft_dump);

int rmt_pft_flush(struct rmt * instance)
{ return is_rmt_pft_ok(instance) ? pft_flush(instance->pft) : -1; }
EXPORT_SYMBOL(rmt_pft_flush);

int rmt_ps_publish(struct ps_factory * factory)
{
        if (factory == NULL) {
                LOG_ERR("%s: NULL factory", __func__);
                return -1;
        }

        return ps_publish(&policy_sets, factory);
}
EXPORT_SYMBOL(rmt_ps_publish);

int rmt_ps_unpublish(const char * name)
{ return ps_unpublish(&policy_sets, name); }
EXPORT_SYMBOL(rmt_ps_unpublish);

#ifdef CONFIG_RINA_RMT_REGRESSION_TESTS
#if 0
/* FIXME: THese regressions are outdated */

static struct pdu * regression_tests_pdu_create(address_t address)
{
        struct buffer * buffer;
        struct pdu *    pdu;
        struct pci *    pci;
        char *          data = "Hello, world";

        buffer =  buffer_create_from(data, strlen(data) + 1);
        if (!buffer) {
                LOG_DBG("Failed to create buffer");
                return NULL;
        }
        pci = pci_create();
        if (!pci) {
                buffer_destroy(buffer);
                return NULL;
        }

        if (pci_format(pci,
                       0,
                       0,
                       address,
                       0,
                       0,
                       0,
                       PDU_TYPE_MGMT)) {
                buffer_destroy(buffer);
                pci_destroy(pci);
                return NULL;
        }

        pdu = pdu_create();
        if (!pdu) {
                buffer_destroy(buffer);
                pci_destroy(pci);
                return NULL;
        }

        if (pdu_buffer_set(pdu, buffer)) {
                pdu_destroy(pdu);
                buffer_destroy(buffer);
                pci_destroy(pci);
                return NULL;
        }

        if (pdu_pci_set(pdu, pci)) {
                pdu_destroy(pdu);
                pci_destroy(pci);
                return NULL;
        }

        return pdu;
}

static bool regression_tests_egress_queue(void)
{
        struct rmt *        rmt;
        struct rmt_n1_port *  tmp;
        port_id_t           id;
        struct pdu *        pdu;
        address_t           address;
        const char *        name;
        struct rmt_n1_port *  entry;
        bool                out;
        struct hlist_node * ntmp;
        int                 bucket;

        address = 11;

        rmt = rkzalloc(sizeof(*rmt), GFP_KERNEL);
        if (!rmt) {
                LOG_DBG("Could not malloc memory for RMT");
                return false;
        }

        LOG_DBG("Creating a new qmap instance");
        rmt->egress.n1_ports = n1pmap_create();
        if (!rmt->egress.n1_ports) {
                LOG_DBG("Failed to create qmap");
                rmt_destroy(rmt);
                return false;
        }

        LOG_DBG("Creating rmt-egress-wq");
        name = create_name("egress-wq", rmt);
        if (!name) {
                rmt_destroy(rmt);
                return false;
        }
        rmt->egress.wq = rwq_create(name);
        if (!rmt->egress.wq) {
                rmt_destroy(rmt);
                return false;
        }

        id = 1;
        if (rmt_n1_port_send_add(rmt, id)) {
                LOG_DBG("Failed to add queue");
                rmt_destroy(rmt);
                return false;
        }
        LOG_DBG("Added to qmap");

        tmp = n1pmap_find(rmt->egress.n1_ports, id);
        if (!tmp) {
                LOG_DBG("Failed to retrieve queue");
                rmt_destroy(rmt);
                return false;
        }
        tmp = NULL;

        pdu = regression_tests_pdu_create(address);
        if (!pdu) {
                LOG_DBG("Failed to create pdu");
                rmt_destroy(rmt);
                return false;
        }

        LOG_DBG("Data:       %s",
                (char *) buffer_data_ro(pdu_buffer_get_ro(pdu)));
        LOG_DBG("Length:     %d",
                buffer_length(pdu_buffer_get_ro(pdu)));
        LOG_DBG("PDU Type:   %X",
                pci_type(pdu_pci_get_ro(pdu)));
        LOG_DBG("PCI Length: %d",
                pci_length(pdu_pci_get_ro(pdu)));

        LOG_DBG("Pushing PDU into n1_ports");
        spin_lock(&rmt->egress.n1_ports->lock);
        tmp = n1pmap_find(rmt->egress.n1_ports, id);
        if (!tmp) {
                spin_unlock(&rmt->egress.n1_ports->lock);
                pdu_destroy(pdu);
                rmt_destroy(rmt);
                return false;
        }

        if (rfifo_push_ni(tmp->queue, pdu)) {
                spin_unlock(&rmt->egress.n1_ports->lock);
                pdu_destroy(pdu);
                rmt_destroy(rmt);
                return false;
        }
        spin_unlock(&rmt->egress.n1_ports->lock);

        out = false;
        while (!out) {
                out = true;
                hash_for_each_safe(rmt->egress.n1_ports->n1_ports,
                                   bucket,
                                   ntmp,
                                   entry,
                                   hlist) {
                        struct sdu * sdu;
                        port_id_t    port_id;

                        spin_lock(&rmt->egress.n1_ports->lock);
                        pdu     = (struct pdu *) rfifo_pop(entry->queue);
                        port_id = entry->port_id;
                        spin_unlock(&rmt->egress.n1_ports->lock);

                        if (!pdu) {
                                LOG_DBG("Where is our PDU???");
                                break;
                        }
                        out = false;
                        sdu = sdu_create_pdu_with(pdu);
                        if (!sdu) {
                                LOG_DBG("Where is our SDU???");
                                break;
                        }

                        if (sdu_destroy(sdu)) {
                                LOG_DBG("Failed destruction of SDU");
                                LOG_DBG("SDU was not ok");
                        }
                }
        }

        if (n1_port_destroy(tmp)) {
                LOG_DBG("Failed to destroy queue");
                rmt_destroy(rmt);
                return false;
        }

        rmt_destroy(rmt);

        return true;
}

static bool regression_tests_process_mgmt_pdu(struct rmt * rmt,
                                              port_id_t    port_id,
                                              struct sdu * sdu)
{
        struct buffer * buffer;
        struct pdu *    pdu;

        pdu = pdu_create_with(sdu);
        if (!pdu) {
                LOG_DBG("Cannot get PDU from SDU");
                sdu_destroy(sdu);
                return false;
        }

        buffer = pdu_buffer_get_rw(pdu);
        if (!buffer_is_ok(buffer)) {
                LOG_DBG("PDU has no buffer ???");
                return false;
        }

        sdu = sdu_create_buffer_with(buffer);
        if (!sdu_is_ok(sdu)) {
                LOG_DBG("Cannot create SDU");
                pdu_destroy(pdu);
                return false;
        }

        if (pdu_buffer_disown(pdu)) {
                pdu_destroy(pdu);
                /* FIXME: buffer is owned by PDU and SDU, we're leaking sdu */
                return false;
        }

        pdu_destroy(pdu);

        if (sdu_destroy(sdu)) {
                LOG_DBG("Cannot destroy SDU something bad happened");
                return false;
        }

        return true;
}

static bool regression_tests_ingress_queue(void)
{
        struct rmt *       rmt;
        struct rmt_n1_port * tmp;
        port_id_t          id;
        struct sdu *       sdu;
        struct pdu *       pdu;
        address_t          address;
        const char *       name;
        bool               nothing_to_do;

        address = 17;

        rmt = rkzalloc(sizeof(*rmt), GFP_KERNEL);
        if (!rmt) {
                LOG_DBG("Could not malloc memory for RMT");
                return false;
        }

        LOG_DBG("Creating a qmap instance for ingress");
        rmt->ingress.n1_ports = n1pmap_create();
        if (!rmt->ingress.n1_ports) {
                LOG_DBG("Failed to create qmap");
                rmt_destroy(rmt);
                return false;
        }

        LOG_DBG("Creating rmt-ingress-wq");
        name = create_name("ingress-wq", rmt);
        if (!name) {
                rmt_destroy(rmt);
                return false;
        }
        rmt->ingress.wq = rwq_create_hp(name);
        if (!rmt->ingress.wq) {
                rmt_destroy(rmt);
                return false;
        }

        id = 1;
        if (rmt_n1_port_recv_add(rmt, id)) {
                LOG_DBG("Failed to add queue");
                rmt_destroy(rmt);
                return false;
        }
        LOG_DBG("Added to qmap");

        tmp = n1pmap_find(rmt->ingress.n1_ports, id);
        if (!tmp) {
                LOG_DBG("Failed to retrieve queue");
                rmt_destroy(rmt);
                return false;
        }
        tmp = NULL;

        pdu = regression_tests_pdu_create(address);
        if (!pdu) {
                LOG_DBG("Failed to create pdu");
                rmt_destroy(rmt);
                return false;
        }

        sdu = sdu_create_pdu_with(pdu);
        if (!sdu) {
                LOG_DBG("Failed to create SDU");
                pdu_destroy(pdu);
                rmt_destroy(rmt);
                return false;
        }
        spin_lock(&rmt->ingress.n1_ports->lock);
        tmp = n1pmap_find(rmt->ingress.n1_ports, id);
        if (!tmp) {
                spin_unlock(&rmt->ingress.n1_ports->lock);
                sdu_destroy(sdu);
                rmt_destroy(rmt);
                return false;
        }

        if (rfifo_push_ni(tmp->queue, sdu)) {
                spin_unlock(&rmt->ingress.n1_ports->lock);
                sdu_destroy(sdu);
                rmt_destroy(rmt);
                return false;
        }
        spin_unlock(&rmt->ingress.n1_ports->lock);

        nothing_to_do = false;
        while (!nothing_to_do) {
                struct rmt_n1_port *  entry;
                int                 bucket;
                struct hlist_node * ntmp;

                nothing_to_do = true;
                hash_for_each_safe(rmt->ingress.n1_ports->n1_ports,
                                   bucket,
                                   ntmp,
                                   entry,
                                   hlist) {
                        struct sdu * sdu;
                        port_id_t    pid;
                        pdu_type_t   pdu_type;
                        struct pci * pci;

                        ASSERT(entry);

                        spin_lock(&rmt->ingress.n1_ports->lock);
                        sdu     = (struct sdu *) rfifo_pop(entry->queue);
                        pid = entry->port_id;
                        spin_unlock(&rmt->ingress.n1_ports->lock);

                        if (!sdu) {
                                LOG_DBG("No SDU to work with");
                                break;
                        }

                        nothing_to_do = false;
                        pci = sdu_pci_copy(sdu);
                        if (!pci) {
                                LOG_DBG("No PCI to work with");
                                break;
                        }

                        pdu_type = pci_type(pci);
                        if (!pdu_type_is_ok(pdu_type)) {
                                LOG_ERR("Wrong PDU type");
                                pci_destroy(pci);
                                sdu_destroy(sdu);
                                break;
                        }
                        LOG_DBG("PDU type: %X", pdu_type);
                        switch (pdu_type) {
                        case PDU_TYPE_MGMT:
                                regression_tests_process_mgmt_pdu(rmt,
                                                                  pid,
                                                                  sdu);
                                break;
                        case PDU_TYPE_DT:
                                /*
                                 * (FUTURE)
                                 *
                                 * enqueue PDU in pdus_dt[dest-addr, qos-id]
                                 * don't process it now ...
                                 *
                                 * process_dt_pdu(rmt, port_id, sdu);
                                 */
                                break;
                        default:
                                LOG_ERR("Unknown PDU type %d", pdu_type);
                                sdu_destroy(sdu);
                                break;
                        }
                        pci_destroy(pci);

                        /* (FUTURE) foreach_end() */
                }
        }

        if (n1_port_destroy(tmp)) {
                LOG_DBG("Failed to destroy queue");
                rmt_destroy(rmt);
                return false;
        }

        rmt_destroy(rmt);

        return true;
}
#endif

bool regression_tests_rmt(void)
{
#if 0
        if (!regression_tests_egress_queue()) {
                LOG_ERR("Failed regression test on egress n1_ports");
                return false;
        }

        if (!regression_tests_ingress_queue()) {
                LOG_ERR("Failed regression test on ingress n1_ports");
                return false;
        }
#endif
        return true;
}
#endif
