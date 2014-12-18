/*
 * RMT (Relaying and Multiplexing Task)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
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

#define rmap_hash(T, K) hash_min(K, HASH_BITS(T))

enum flow_state {
        PORT_STATE_ENABLED,
        PORT_STATE_DISABLED
};

struct rmt_queue {
        struct rfifo *         queue;
        port_id_t              port_id;
        struct ipcp_instance * n1_ipcp;
        struct hlist_node      hlist;
        enum flow_state        state;
};

static struct rmt_queue * queue_create(port_id_t id,
                                       struct ipcp_instance * n1_ipcp)
{
        struct rmt_queue * tmp;

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
        tmp->state   = PORT_STATE_ENABLED;

        LOG_DBG("Queue %pK created successfully (port-id = %d)", tmp, id);

        return tmp;
}

static int queue_destroy(struct rmt_queue * q)
{
        ASSERT(q);
        ASSERT(q->queue);

        LOG_DBG("Destroying queue %pK (port-id = %d)", q, q->port_id);

        hash_del(&q->hlist);

        if (q->queue) rfifo_destroy(q->queue, (void (*)(void *)) pdu_destroy);
        rkfree(q);

        return 0;
}

struct rmt_qmap {
        spinlock_t lock; /* FIXME: Has to be moved in the pipelines */

        DECLARE_HASHTABLE(queues, 7);
};

static struct rmt_qmap * qmap_create(void)
{
        struct rmt_qmap * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        hash_init(tmp->queues);
        spin_lock_init(&tmp->lock);

        return tmp;
}

static int qmap_destroy(struct rmt_qmap * m, struct kfa * kfa)
{
        struct rmt_queue *  entry;
        struct hlist_node * tmp;
        int                 bucket;

        ASSERT(m);

        hash_for_each_safe(m->queues, bucket, tmp, entry, hlist) {
                ASSERT(entry);

                /*kfa_flow_rmt_unbind(kfa, entry->port_id);*/
                if (queue_destroy(entry)) {
                        LOG_ERR("Could not destroy entry %pK", entry);
                        return -1;
                }
        }

        rkfree(m);

        return 0;
}

static struct rmt_queue * qmap_find(struct rmt_qmap * m,
                                    port_id_t         id)
{
        struct rmt_queue *        entry;
        const struct hlist_head * head;

        ASSERT(m);

        if (!is_port_id_ok(id))
                return NULL;

        head = &m->queues[rmap_hash(m->queues, id)];
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
        address_t               address;
        struct ipcp_instance *  parent;
        struct pft *            pft;
        struct kfa *            kfa;
        struct efcp_container * efcpc;
        struct serdes *         serdes;

        struct {
                struct workqueue_struct * wq;
                struct rwq_work_item *    item;
                struct rmt_qmap *         queues;
                struct pft_cache          cache;
        } ingress;

        struct {
                struct workqueue_struct * wq;
                struct rmt_qmap *         queues;
                struct pft_cache          cache;
        } egress;
};

#define MAX_NAME_SIZE 128

static const char * create_name(const char *       prefix,
                                const struct rmt * instance)
{
        static char name[MAX_NAME_SIZE]; /* FIXME: Remove, ot it will become
                                          *        source of troubles for sure
                                          */

        ASSERT(prefix);
        ASSERT(instance);

        if (snprintf(name, sizeof(name),
                     RINA_PREFIX "-%s-%pK", prefix, instance) >=
            sizeof(name))
                return NULL;

        return name;
}

int rmt_destroy(struct rmt * instance)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        if (instance->ingress.wq)     rwq_destroy(instance->ingress.wq);
        if (instance->ingress.item)   rwq_work_destroy(instance->ingress.item);
        if (instance->ingress.queues) qmap_destroy(instance->ingress.queues,
                                                   instance->kfa);
        pft_cache_fini(&instance->ingress.cache);

        if (instance->egress.wq)      rwq_destroy(instance->egress.wq);
        if (instance->egress.queues)  qmap_destroy(instance->egress.queues,
                                                   instance->kfa);
        pft_cache_fini(&instance->egress.cache);

        if (instance->pft)            pft_destroy(instance->pft);
        if (instance->serdes)         serdes_destroy(instance->serdes);

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

static int send_worker(void * o)
{
        struct rmt *        tmp;
        struct rmt_queue *  entry;
        struct hlist_node * ntmp;
        int                 bucket;

        LOG_DBG("Send worker called");

        tmp = (struct rmt *) o;
        if (!tmp) {
                LOG_ERR("No instance passed to send worker !!!");
                return -1;
        }

        spin_lock(&tmp->egress.queues->lock);
        hash_for_each_safe(tmp->egress.queues->queues,
                           bucket,
                           ntmp,
                           entry,
                           hlist) {
                struct sdu *           sdu;
                struct pdu *           pdu;
                struct pdu_ser *       pdu_ser;
                port_id_t              port_id;
                struct buffer *        buffer;
                struct serdes *        serdes;
                struct ipcp_instance * n1_ipcp;
                enum flow_state        state;

                ASSERT(entry);

                state = entry->state;
                if (state == PORT_STATE_DISABLED) {
                        LOG_DBG("Port state is DISABLED");
                        continue;
                }

                pdu = (struct pdu *) rfifo_pop(entry->queue);

                /* FIXME: Shouldn't we ASSERT() here ? */
                if (!pdu) {
                        LOG_DBG("No PDU to work in this queue ...");
                        continue;
                }

                port_id = entry->port_id;
                n1_ipcp = entry->n1_ipcp;
                spin_unlock(&tmp->egress.queues->lock);

                ASSERT(pdu);

                serdes = tmp->serdes;
                ASSERT(serdes);

                pdu_ser = pdu_serialize(serdes, pdu);
                if (!pdu_ser) {
                        LOG_ERR("Error creating serialized PDU");
                        spin_lock(&tmp->egress.queues->lock);
                        pdu_destroy(pdu);
                        continue;
                }

                buffer = pdu_ser_buffer(pdu_ser);
                if (!buffer_is_ok(buffer)) {
                        LOG_ERR("Buffer is not okay");
                        spin_lock(&tmp->egress.queues->lock);
                        pdu_ser_destroy(pdu_ser);
                        continue;
                }

                if (pdu_ser_buffer_disown(pdu_ser)) {
                        LOG_ERR("Could not disown buffer");
                        spin_lock(&tmp->egress.queues->lock);
                        pdu_ser_destroy(pdu_ser);
                        continue;
                }

                pdu_ser_destroy(pdu_ser);

                sdu = sdu_create_buffer_with(buffer);
                if (!sdu) {
                        spin_lock(&tmp->egress.queues->lock);
                        LOG_ERR("Error creating SDU from serialized PDU, "
                                "dropping PDU!");
                        buffer_destroy(buffer);
                        continue;
                }

                LOG_DBG("Gonna send SDU to port-id %d", port_id);
                if (n1_ipcp->ops->sdu_write(n1_ipcp->data,port_id, sdu)) {
                        LOG_ERR("Couldn't write SDU to KFA");
                        spin_lock(&tmp->egress.queues->lock);
                        continue; /* Useless for the moment */
                }

                spin_lock(&tmp->egress.queues->lock);
        }
        spin_unlock(&tmp->egress.queues->lock);

        return 0;
}

int rmt_send_port_id(struct rmt * instance,
                     port_id_t    id,
                     struct pdu * pdu)
{
        struct rwq_work_item * item;
        struct rmt_queue *     s_queue;

        if (!pdu_is_ok(pdu)) {
                LOG_ERR("Bogus PDU passed");
                return -1;
        }
        if (!instance) {
                LOG_ERR("Bogus RMT passed");

                pdu_destroy(pdu);
                return -1;
        }
        if (!instance->egress.queues) {
                LOG_ERR("No queues to push into");

                pdu_destroy(pdu);
                return -1;
        }

        /* Is this _ni() call really necessary ??? */
        item = rwq_work_create_ni(send_worker, instance);
        if (!item) {
                LOG_ERR("Cannot send PDU to port-id %d", id);
                pdu_destroy(pdu);
                return -1;
        }

        spin_lock(&instance->egress.queues->lock);
        s_queue = qmap_find(instance->egress.queues, id);
        if (!s_queue) {
                spin_unlock(&instance->egress.queues->lock);
                pdu_destroy(pdu);
                return -1;
        }

        if (rfifo_push_ni(s_queue->queue, pdu)) {
                spin_unlock(&instance->egress.queues->lock);
                pdu_destroy(pdu);
                return -1;
        }
        spin_unlock(&instance->egress.queues->lock);

        ASSERT(instance->egress.wq);
        if (rwq_work_post(instance->egress.wq, item)) {
                LOG_ERR("Cannot post work (PDU) to egress work-queue");

                spin_lock(&instance->egress.queues->lock);
                s_queue = qmap_find(instance->egress.queues, id);
                if (s_queue) {
                        struct pdu * tmp;

                        tmp = rfifo_pop(s_queue->queue);
                        if (tmp)
                                pdu_destroy(tmp);
                }
                spin_unlock(&instance->egress.queues->lock);

                return -1;
        }

        return 0;
}
EXPORT_SYMBOL(rmt_send_port_id);

int rmt_send(struct rmt * instance,
             address_t    address,
             qos_id_t     qos_id,
             struct pdu * pdu)
{
        int i;

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
        struct rmt_queue * tmp;

        tmp = queue_create(id, n1_ipcp);
        if (!tmp)
                return -1;

        hash_add(instance->egress.queues->queues, &tmp->hlist, id);

        LOG_DBG("Added send queue to rmt instance %pK for port-id %d",
                instance, id);

        return 0;
}

static int rmt_queue_send_add(struct rmt * instance,
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

        if (!instance->egress.queues) {
                LOG_ERR("Invalid RMT");
                return -1;
        }

        if (qmap_find(instance->egress.queues, id)) {
                LOG_ERR("Queue already exists");
                return -1;
        }

        return __queue_send_add(instance, id, n1_ipcp);
}

int rmt_enable_port_id(struct rmt * instance,
                       port_id_t    id)
{
        struct rmt_queue *     queue;
        struct rwq_work_item * item;

        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        if (!is_port_id_ok(id)) {
                LOG_ERR("Wrong port-id %d", id);
                return -1;
        }

        if (!instance->egress.queues) {
                LOG_ERR("Invalid RMT");
                return -1;
        }

        item = rwq_work_create_ni(send_worker, instance);
        if (!item) {
                LOG_ERR("Cannot create send item for port-id %d", id);
                return -1;
        }

        spin_lock(&instance->egress.queues->lock);
        queue = qmap_find(instance->egress.queues, id);
        if (!queue) {
                spin_unlock(&instance->egress.queues->lock);
                LOG_ERR("No queue associated to this port-id, %d", id);
                return -1;
        }

        if (queue->state != PORT_STATE_DISABLED) {
                spin_unlock(&instance->egress.queues->lock);
                LOG_DBG("Nothing to do for port-id %d", id);
                return 0;
        }
        queue->state = PORT_STATE_ENABLED;
        spin_unlock(&instance->egress.queues->lock);

        LOG_DBG("Changed state to ENABLED");
        ASSERT(instance->egress.wq);
        if (rwq_work_post(instance->egress.wq, item)) {
                LOG_ERR("Cannot post work (PDU) to egress work-queue");
                return -1;
        }

        return 0;
}
EXPORT_SYMBOL(rmt_enable_port_id);

int rmt_disable_port_id(struct rmt * instance,
                        port_id_t    id)
{
        struct rmt_queue * queue;

        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        if (!is_port_id_ok(id)) {
                LOG_ERR("Wrong port-id %d", id);
                return -1;
        }

        if (!instance->egress.queues) {
                LOG_ERR("Invalid RMT");
                return -1;
        }

        spin_lock(&instance->egress.queues->lock);
        queue = qmap_find(instance->egress.queues, id);
        if (!queue) {
                spin_unlock(&instance->egress.queues->lock);
                LOG_ERR("No queue associated to this port-id, %d", id);
                return -1;
        }

        if (queue->state == PORT_STATE_DISABLED) {
                spin_unlock(&instance->egress.queues->lock);
                LOG_DBG("Nothing to do for port-id %d", id);
                return 0;
        }
        queue->state = PORT_STATE_DISABLED;
        spin_unlock(&instance->egress.queues->lock);
        LOG_DBG("Changed state to DISABLED");

        return 0;
}
EXPORT_SYMBOL(rmt_disable_port_id);

static int rmt_queue_send_delete(struct rmt * instance,
                                 port_id_t    id)
{
        struct rmt_queue * q;

        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        if (!instance->egress.queues) {
                LOG_ERR("Bogus egress instance passed");
                return -1;
        }

        if (!is_port_id_ok(id)) {
                LOG_ERR("Wrong port-id %d", id);
                return -1;
        }

        q = qmap_find(instance->egress.queues, id);
        if (!q) {
                LOG_ERR("Queue does not exist");
                return -1;
        }

        return queue_destroy(q);
}

static int __queue_recv_add(struct rmt * instance,
                            port_id_t    id,
                            struct ipcp_instance * n1_ipcp)
{
        struct rmt_queue * tmp;

        tmp = queue_create(id, n1_ipcp);
        if (!tmp)
                return -1;

        hash_add(instance->ingress.queues->queues, &tmp->hlist, id);

        LOG_DBG("Added receive queue to rmt instance %pK for port-id %d",
                instance, id);

        return 0;
}

static int rmt_queue_recv_add(struct rmt *                instance,
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

        if (!instance->ingress.queues) {
                LOG_ERR("Invalid RMT");
                return -1;
        }

        if (qmap_find(instance->ingress.queues, id)) {
                LOG_ERR("Queue already exists");
                return -1;
        }

        return __queue_recv_add(instance, id, n1_ipcp);
}

static int rmt_queue_recv_delete(struct rmt * instance,
                                 port_id_t    id)
{
        struct rmt_queue * q;

        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        if (!instance->ingress.queues) {
                LOG_ERR("Bogus egress instance passed");
                return -1;
        }

        if (!is_port_id_ok(id)) {
                LOG_ERR("Wrong port-id %d", id);
                return -1;
        }

        q = qmap_find(instance->ingress.queues, id);
        if (!q) {
                LOG_ERR("Queue does not exist");
                return -1;
        }

        return queue_destroy(q);
}

int rmt_n1port_bind(struct rmt * instance,
                    port_id_t    id,
                    struct ipcp_instance * n1_ipcp)
{
        if (rmt_queue_send_add(instance, id, n1_ipcp))
                return -1;

        if (rmt_queue_recv_add(instance, id, n1_ipcp)) {
                rmt_queue_send_delete(instance, id);
                return -1;
        }

        return 0;
}
EXPORT_SYMBOL(rmt_n1port_bind);

int rmt_n1port_unbind(struct rmt * instance,
                      port_id_t    id)
{
        int retval = 0;

        if (rmt_queue_send_delete(instance, id))
                retval = -1;

        if (rmt_queue_recv_delete(instance, id))
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
        struct rmt_queue * queue;

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

        pdu_ser = pdu_serialize(serdes, pdu);
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

        sdu = sdu_create_buffer_with(buffer);
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

                        queue = qmap_find(rmt->ingress.queues, i);
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

                queue = qmap_find(rmt->ingress.queues, i);
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

static bool ev_resch(struct rmt_qmap * queues) {
        struct rmt_queue *  entry;
        int                 bucket;
        unsigned long       flags;

        if (!queues) {
                LOG_ERR("No queues passed...");
                return false;
        }

        spin_lock_irqsave(&queues->lock, flags);
        hash_for_each(queues->queues,
                      bucket,
                      entry,
                      hlist) {
                if (!rfifo_is_empty(entry->queue)) {
                        spin_unlock_irqrestore(&queues->lock, flags);
                        return true;
                }
        }
        spin_unlock_irqrestore(&queues->lock, flags);
        return false;
}

static int receive_worker(void * o)
{
        struct rmt *        tmp;
        struct rmt_queue *  entry;
        int                 bucket;
        struct hlist_node * ntmp;

        LOG_DBG("Receive worker called");

        tmp = (struct rmt *) o;
        if (!tmp) {
                LOG_ERR("No instance passed to receive worker !!!");
                return -RWQ_WORKERROR;
        }

        spin_lock(&tmp->ingress.queues->lock);
        hash_for_each_safe(tmp->ingress.queues->queues,
                           bucket,
                           ntmp,
                           entry,
                           hlist) {

                port_id_t          port_id;
                pdu_type_t         pdu_type;
                const struct pci * pci;
                struct pdu_ser *   pdu_ser;
                struct pdu *       pdu;
                address_t          dst_addr;
                qos_id_t           qos_id;
                struct serdes *    serdes;
                struct buffer *    buf;
                struct sdu *       sdu;

                ASSERT(entry);

                sdu = (struct sdu *) rfifo_pop(entry->queue);
                if (!sdu) {
                        LOG_DBG("No SDU to work with in this queue");
                        continue;
                }

                port_id = entry->port_id;
                spin_unlock(&tmp->ingress.queues->lock);

                buf = sdu_buffer_rw(sdu);
                if (!buf) {
                        LOG_DBG("No buffer present");
                        sdu_destroy(sdu);
                        spin_lock(&tmp->ingress.queues->lock);
                        continue;
                }

                if (sdu_buffer_disown(sdu)) {
                        LOG_DBG("Could not disown SDU");
                        sdu_destroy(sdu);
                        spin_lock(&tmp->ingress.queues->lock);
                        continue;
                }

                sdu_destroy(sdu);

                pdu_ser = pdu_ser_create_buffer_with(buf);
                if (!pdu_ser) {
                        LOG_DBG("No ser PDU to work with");
                        buffer_destroy(buf);
                        spin_lock(&tmp->ingress.queues->lock);
                        continue;
                }

                serdes = tmp->serdes;
                ASSERT(serdes);

                pdu = pdu_deserialize(serdes, pdu_ser);
                if (!pdu) {
                        LOG_ERR("Failed to deserialize PDU!");
                        pdu_ser_destroy(pdu_ser);
                        spin_lock(&tmp->ingress.queues->lock);
                        continue;
                }

                pci = pdu_pci_get_ro(pdu);
                if (!pci) {
                        LOG_ERR("No PCI to work with, dropping SDU!");
                        pdu_destroy(pdu);
                        spin_lock(&tmp->ingress.queues->lock);
                        continue;
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
                        spin_lock(&tmp->ingress.queues->lock);
                        continue;
                }

                /* pdu is not for me */
                if (tmp->address != dst_addr) {
                        if (!dst_addr) {
                                process_mgmt_pdu(tmp, port_id, pdu);
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
                                process_mgmt_pdu(tmp, port_id, pdu);
                                break;

                        case PDU_TYPE_CC:
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
                spin_lock(&tmp->ingress.queues->lock);
        }
        spin_unlock(&tmp->ingress.queues->lock);

        if (ev_resch(tmp->ingress.queues))
                return RWQ_RESCHEDULE;

        return RWQ_NORESCHEDULE;
}

int rmt_receive(struct rmt * instance,
                struct sdu * sdu,
                port_id_t    from)
{
        struct rmt_queue *     r_queue;

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
        if (!instance->ingress.queues) {
                LOG_ERR("No ingress queues in RMT: %pK", instance);
                sdu_destroy(sdu);
                return -1;
        }

        spin_lock(&instance->ingress.queues->lock);
        r_queue = qmap_find(instance->ingress.queues, from);
        if (!r_queue) {
                spin_unlock(&instance->ingress.queues->lock);
                sdu_destroy(sdu);
                return -1;
        }

        if (rfifo_push_ni(r_queue->queue, sdu)) {
                spin_unlock(&instance->ingress.queues->lock);
                sdu_destroy(sdu);
                return -1;
        }
        spin_unlock(&instance->ingress.queues->lock);

        if (rwq_work_post(instance->ingress.wq, instance->ingress.item))
                LOG_DBG("There was a pending work already in the ingress queue");

        return 0;
}
EXPORT_SYMBOL(rmt_receive);

int rmt_flush_work(struct rmt * rmt)
{
        if (!rmt) {
                LOG_ERR("No RMT passed");
                return -1;
        }

        rwq_flush(rmt->ingress.wq);
        LOG_DBG("RMT Ingress WQ  %p has been flushed", rmt->ingress.wq);

        return 0;
}

int rmt_restart_work(struct rmt * rmt)
{
        if (!rmt) {
                LOG_ERR("No RMT passed");
                return -1;
        }

        if (!rmt->ingress.item) {
                LOG_ERR("RMT Ingress wq has not work item");
                return -1;
        }

        rwq_work_post(rmt->ingress.wq, rmt->ingress.item);

        LOG_DBG("RMT Ingress WQ  %p has been restarted with WI %p",
                rmt->ingress.wq, rmt->ingress.item);

        return 0;
}

struct rmt * rmt_create(struct ipcp_instance *  parent,
                        struct kfa *            kfa,
                        struct efcp_container * efcpc)
{
        struct rmt * tmp;
        const char * name;

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
        if (!tmp->pft) {
                rmt_destroy(tmp);
                return NULL;
        }

        /* Egress */
        name = create_name("egress-wq", tmp);
        if (!name) {
                rmt_destroy(tmp);
                return NULL;
        }
        tmp->egress.wq = rwq_create(name);
        if (!tmp->egress.wq) {
                rmt_destroy(tmp);
                return NULL;
        }
        tmp->egress.queues = qmap_create();
        if (!tmp->egress.queues) {
                rmt_destroy(tmp);
                return NULL;
        }
        if (pft_cache_init(&tmp->egress.cache)) {
                rmt_destroy(tmp);
                return NULL;
        }

        /* Ingress */
        name = create_name("ingress-wq", tmp);
        if (!name) {
                rmt_destroy(tmp);
                return NULL;
        }
        tmp->ingress.wq = rwq_create(name);
        if (!tmp->ingress.wq) {
                rmt_destroy(tmp);
                return NULL;
        }
        tmp->ingress.queues = qmap_create();
        if (!tmp->ingress.queues) {
                rmt_destroy(tmp);
                return NULL;
        }
        if (pft_cache_init(&tmp->ingress.cache)) {
                rmt_destroy(tmp);
                return NULL;
        }
        tmp->ingress.item = rwq_work_create_single(receive_worker, tmp);
        if (!tmp->ingress.item) {
                rmt_destroy(tmp);
                return NULL;
        }
        LOG_DBG("Instance %pK initialized successfully", tmp);

        return tmp;
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
{
        return is_rmt_pft_ok(instance) ? pft_flush(instance->pft) : -1;
}
EXPORT_SYMBOL(rmt_pft_flush);

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
        struct rmt_queue *  tmp;
        port_id_t           id;
        struct pdu *        pdu;
        address_t           address;
        const char *        name;
        struct rmt_queue *  entry;
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
        rmt->egress.queues = qmap_create();
        if (!rmt->egress.queues) {
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
        if (rmt_queue_send_add(rmt, id)) {
                LOG_DBG("Failed to add queue");
                rmt_destroy(rmt);
                return false;
        }
        LOG_DBG("Added to qmap");

        tmp = qmap_find(rmt->egress.queues, id);
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

        LOG_DBG("Pushing PDU into queues");
        spin_lock(&rmt->egress.queues->lock);
        tmp = qmap_find(rmt->egress.queues, id);
        if (!tmp) {
                spin_unlock(&rmt->egress.queues->lock);
                pdu_destroy(pdu);
                rmt_destroy(rmt);
                return false;
        }

        if (rfifo_push_ni(tmp->queue, pdu)) {
                spin_unlock(&rmt->egress.queues->lock);
                pdu_destroy(pdu);
                rmt_destroy(rmt);
                return false;
        }
        spin_unlock(&rmt->egress.queues->lock);

        out = false;
        while (!out) {
                out = true;
                hash_for_each_safe(rmt->egress.queues->queues,
                                   bucket,
                                   ntmp,
                                   entry,
                                   hlist) {
                        struct sdu * sdu;
                        port_id_t    port_id;

                        spin_lock(&rmt->egress.queues->lock);
                        pdu     = (struct pdu *) rfifo_pop(entry->queue);
                        port_id = entry->port_id;
                        spin_unlock(&rmt->egress.queues->lock);

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

        if (queue_destroy(tmp)) {
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
        struct rmt_queue * tmp;
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
        rmt->ingress.queues = qmap_create();
        if (!rmt->ingress.queues) {
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
        if (rmt_queue_recv_add(rmt, id)) {
                LOG_DBG("Failed to add queue");
                rmt_destroy(rmt);
                return false;
        }
        LOG_DBG("Added to qmap");

        tmp = qmap_find(rmt->ingress.queues, id);
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
        spin_lock(&rmt->ingress.queues->lock);
        tmp = qmap_find(rmt->ingress.queues, id);
        if (!tmp) {
                spin_unlock(&rmt->ingress.queues->lock);
                sdu_destroy(sdu);
                rmt_destroy(rmt);
                return false;
        }

        if (rfifo_push_ni(tmp->queue, sdu)) {
                spin_unlock(&rmt->ingress.queues->lock);
                sdu_destroy(sdu);
                rmt_destroy(rmt);
                return false;
        }
        spin_unlock(&rmt->ingress.queues->lock);

        nothing_to_do = false;
        while (!nothing_to_do) {
                struct rmt_queue *  entry;
                int                 bucket;
                struct hlist_node * ntmp;

                nothing_to_do = true;
                hash_for_each_safe(rmt->ingress.queues->queues,
                                   bucket,
                                   ntmp,
                                   entry,
                                   hlist) {
                        struct sdu * sdu;
                        port_id_t    pid;
                        pdu_type_t   pdu_type;
                        struct pci * pci;

                        ASSERT(entry);

                        spin_lock(&rmt->ingress.queues->lock);
                        sdu     = (struct sdu *) rfifo_pop(entry->queue);
                        pid = entry->port_id;
                        spin_unlock(&rmt->ingress.queues->lock);

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

        if (queue_destroy(tmp)) {
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
                LOG_ERR("Failed regression test on egress queues");
                return false;
        }

        if (!regression_tests_ingress_queue()) {
                LOG_ERR("Failed regression test on ingress queues");
                return false;
        }
#endif
        return true;
}
#endif
