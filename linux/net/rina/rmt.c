/*
 * RMT (Relaying and Multiplexing Task)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
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

#define rmap_hash(T, K) hash_min(K, HASH_BITS(T))

struct rmt_queue {
        struct rfifo *    queue;
        port_id_t         port_id;
        struct hlist_node hlist;
        spinlock_t        lock;
};

static struct rmt_queue * rmt_queue_create(port_id_t id)
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
        spin_lock_init(&tmp->lock);

        LOG_DBG("Queue %pK created successfully (port-id = %d)", tmp, id);

        return tmp;
}

static int rmt_queue_destroy(struct rmt_queue * q)
{
        ASSERT(q);
        ASSERT(q->queue);

        LOG_DBG("Destroying queue %pK (port-id = %d)", q, q->port_id);

        rfifo_destroy(q->queue, (void (*)(void *)) pdu_destroy);
        hash_del(&q->hlist);
        rkfree(q);

        return 0;
}

struct rmt_qmap {
        DECLARE_HASHTABLE(queues, 7);
        spinlock_t    lock;
        int           in_use; /* FIXME: Use rwqo and remove in_use */
};

static struct rmt_qmap * qmap_create(void)
{
        struct rmt_qmap * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        hash_init(tmp->queues);
        spin_lock_init(&tmp->lock);
        tmp->in_use = 0;

        return tmp;
}

static int qmap_destroy(struct rmt_qmap * m)
{
        struct rmt_queue *  entry;
        struct hlist_node * tmp;
        int                 bucket;

        ASSERT(m);

        hash_for_each_safe(m->queues, bucket, tmp, entry, hlist) {
                ASSERT(entry);

                if (rmt_queue_destroy(entry)) {
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

struct rmt {
        address_t               address;
        struct ipcp_instance *  parent;
        struct pft *            pft;
        struct kfa *            kfa;
        struct efcp_container * efcpc;

        struct {
                struct workqueue_struct * wq;
                struct rmt_qmap *         queues;
        } ingress;

        struct {
                struct workqueue_struct * wq;
                struct rmt_qmap *         queues;
        } egress;
};

static const char * create_name(const char *       prefix,
                                const struct rmt * instance)
{
        static char name[64];

        ASSERT(prefix);
        ASSERT(instance);

        if (snprintf(name, sizeof(name), "%s-%pK", prefix, instance) >=
            sizeof(name))
                return NULL;

        return name;
}

struct rmt * rmt_create(struct ipcp_instance *  parent,
                        struct kfa *            kfa,
                        struct efcp_container * efcpc)
{
        struct rmt * tmp;
        const char * name;

        if (!kfa)
                return NULL;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        if (!parent || !kfa || !efcpc) {
                LOG_ERR("Bogus input parameters");
                return NULL;
        }

        tmp->parent = parent;
        tmp->kfa    = kfa;
        tmp->efcpc  = efcpc;

        tmp->pft = pft_create();
        if (!tmp->pft) {
                rmt_destroy(tmp);
                return NULL;
        }

        name = create_name("rmt-egress-wq", tmp);
        if (!name)
                return NULL;
        tmp->egress.wq = rwq_create(name);
        if (!tmp->egress.wq) {
                rmt_destroy(tmp);
                return NULL;
        }

        name = create_name("rmt-ingress-wq", tmp);
        if (!name)
                return NULL;
        tmp->ingress.wq = rwq_create(name);
        if (!tmp->ingress.wq) {
                rmt_destroy(tmp);
                return NULL;
        }

        tmp->address = address_bad();

        tmp->ingress.queues = qmap_create();
        if (!tmp->ingress.queues) {
                rmt_destroy(tmp);
                return NULL;
        }

        tmp->egress.queues = qmap_create();
        if (!tmp->egress.queues) {
                rmt_destroy(tmp);
                return NULL;
        }

        LOG_DBG("Instance %pK initialized successfully", tmp);

        return tmp;
}
EXPORT_SYMBOL(rmt_create);

int rmt_destroy(struct rmt * instance)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        if (instance->ingress.wq)     rwq_destroy(instance->ingress.wq);
        if (instance->ingress.queues) qmap_destroy(instance->ingress.queues);

        if (instance->egress.wq)      rwq_destroy(instance->egress.wq);
        if (instance->egress.queues)  qmap_destroy(instance->egress.queues);

        if (instance->pft)            pft_destroy(instance->pft);

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

struct send_data {
        struct kfa *      kfa;
        struct rmt_qmap * qmap;
};

static struct send_data * send_data_create(struct kfa *      kfa,
                                           struct rmt_qmap * queues)
{
        struct send_data * tmp;

        if (!kfa)
                return NULL;

        if (!queues)
                return NULL;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->kfa   = kfa;
        tmp->qmap = queues;

        return tmp;
}

static bool is_send_data_complete(const struct send_data * data)
{
        bool ret;

        ret = ((!data || !data->qmap || !data->kfa) ? false : true);

        LOG_DBG("Send data complete? %d", ret);

        return ret;
}

static int send_data_destroy(struct send_data * data)
{
        if (!data) {
                LOG_ERR("No write data passed");
                return -1;
        }

        rkfree(data);

        return 0;
}

static struct sdu * pdu_process(struct pdu * pdu)
{
        struct sdu *          sdu;
        const struct buffer * buffer;
        const struct pci *    pci;
        const void *          buffer_data;
        size_t                size;
        ssize_t               buffer_size;
        ssize_t               pci_size;
        struct buffer *       tmp_buff;
        char *                data;

        if (!pdu)
                return NULL;

        /* FIXME: Add pdu_destroy() on each return */

        buffer = pdu_buffer_get_ro(pdu);
        if (!buffer)
                return NULL;

        buffer_data = buffer_data_ro(buffer);
        if (!buffer_data)
                return NULL;

        pci = pdu_pci_get_ro(pdu);
        if (!pci)
                return NULL;

        buffer_size = buffer_length(buffer);
        if (buffer_size <= 0)
                return NULL;

        pci_size = pci_length(pci);
        if (pci_size <= 0)
                return NULL;

        size = pci_size + buffer_size;
        data = rkmalloc(size, GFP_KERNEL);
        if (!data)
                return NULL;

        /* FIXME: Useless check */
        if (!memcpy(data, pci, pci_size)) {
                rkfree(data);
                return NULL;
        }

        /* FIXME: Useless check */
        if (!memcpy(data + pci_size, buffer_data, buffer_size)) {
                rkfree(data);
                return NULL;
        }

        tmp_buff = buffer_create_with(data, size);
        if (!tmp_buff) {
                rkfree(data);
                return NULL;
        }
        sdu = sdu_create_buffer_with(tmp_buff);
        if (!sdu) {
                buffer_destroy(tmp_buff);
                return NULL;
        }

        pdu_destroy(pdu);

        return sdu;
}

static int rmt_send_worker(void * o)
{
        struct send_data *  tmp;
        struct rmt_queue *   entry;
        bool                out;
        struct hlist_node * ntmp;
        int                 bucket;

        out = false;
        tmp = (struct send_data *) o;
        if (!tmp) {
                LOG_ERR("No send data passed");
                return -1;
        }

        if (!tmp->qmap) {
                LOG_ERR("No RMT queues passed");
                return -1;
        }

        if (!tmp->kfa) {
                LOG_ERR("No KFA passed");

                spin_lock(&tmp->qmap->lock);
                tmp->qmap->in_use = 0;
                spin_unlock(&tmp->qmap->lock);

                return -1;
        }

        while (!out) {
                out = true;
                hash_for_each_safe(tmp->qmap->queues,
                                   bucket,
                                   ntmp,
                                   entry,
                                   hlist) {
                        struct sdu * sdu;
                        struct pdu * pdu;
                        port_id_t    port_id;

                        spin_lock(&entry->lock);
                        pdu     = (struct pdu *) rfifo_pop(entry->queue);
                        port_id = entry->port_id;
                        spin_unlock(&entry->lock);

                        if (!pdu)
                                break;

                        out = false;
                        sdu = pdu_process(pdu);
                        if (!sdu)
                                break;

                        LOG_DBG("Gonna SEND sdu to port_id %d", port_id);
                        if (kfa_flow_sdu_write(tmp->kfa, port_id, sdu)) {
                                LOG_ERR("Couldn't write SDU to KFA");
                        }
                }
        }

        spin_lock(&tmp->qmap->lock);
        tmp->qmap->in_use = 0;
        spin_unlock(&tmp->qmap->lock);

        return 0;
}

int rmt_send_port_id(struct rmt *  instance,
                     port_id_t     id,
                     struct pdu *  pdu)
{
        struct rmt_queue *     squeue;
        struct rwq_work_item * item;
        struct send_data *     data;

        if (!instance) {
                LOG_ERR("Bogus RMT passed");
                return -1;
        }
        if (!pdu) {
                LOG_ERR("Bogus PDU passed");
                return -1;
        }

        spin_lock(&instance->ingress.queues->lock);
        if (!instance->ingress.queues) {
                spin_unlock(&instance->ingress.queues->lock);
                return -1;
        }

        squeue = qmap_find(instance->ingress.queues, id);
        if (!squeue) {
                spin_unlock(&instance->ingress.queues->lock);
                return -1;
        }

        spin_lock(&squeue->lock);
        spin_unlock(&instance->ingress.queues->lock);
        if (rfifo_push_ni(squeue->queue, pdu)) {
                spin_unlock(&squeue->lock);
                return -1;
        }
        spin_unlock(&squeue->lock);

        spin_lock(&instance->ingress.queues->lock);
        if (instance->ingress.queues->in_use) {
                LOG_DBG("Work already posted, nothing more to do");
                spin_unlock(&instance->ingress.queues->lock);
                return 0;
        }
        instance->ingress.queues->in_use = 1;
        spin_unlock(&instance->ingress.queues->lock);

        data = send_data_create(instance->kfa, instance->ingress.queues);
        if (!is_send_data_complete(data)) {
                LOG_ERR("Couldn't create send data");
                return -1;
        }

        /* Is this _ni() call really necessary ??? */
        item = rwq_work_create_ni(rmt_send_worker, data);
        if (!item) {
                send_data_destroy(data);
                LOG_ERR("Couldn't create work");
                return -1;
        }

        ASSERT(instance->egress.wq);

        if (rwq_work_post(instance->egress.wq, item)) {
                send_data_destroy(data);
                pdu_destroy(pdu);
                return -1;
        }

        return 0;
}
EXPORT_SYMBOL(rmt_send_port_id);

int rmt_send(struct rmt * instance,
             address_t    address,
             cep_id_t     connection_id,
             struct pdu * pdu)
{
        port_id_t id;

        if (!instance) {
                LOG_ERR("Bogus RMT passed");
                return -1;
        }
        if (!pdu) {
                LOG_ERR("Bogus PDU passed");
                return -1;
        }
        if (!is_cep_id_ok(connection_id)) {
                LOG_ERR("Bad cep id");
                return -1;
        }

        /* FIXME: Look-up for port-id in pdu-fwd-t and qos-map */

        /* pdu -> pci-> qos-id | cep_id_t -> connection -> qos-id (former) */
        /* address + qos-id (pdu-fwd-t) -> port-id */

        /* FIXME: Remove this hardwire */
        id = (address == 16 ? 2 : 1); /* FIXME: We must call PDU FT */

        LOG_DBG("Gonna SEND to port_id: %d", id);

        if (rmt_send_port_id(instance, id, pdu))
                return -1;

        return 0;
}
EXPORT_SYMBOL(rmt_send);

static int __rmt_queue_send_add(struct rmt * instance,
                                port_id_t    id)
{
        struct rmt_queue * tmp;

        tmp = rmt_queue_create(id);
        if (!tmp)
                return -1;

        hash_add(instance->ingress.queues->queues, &tmp->hlist, id);

        LOG_DBG("Added send queue to rmt %pK for port id %d", instance, id);

        return 0;
}

int rmt_queue_send_add(struct rmt * instance,
                       port_id_t    id)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        if (!is_port_id_ok(id)) {
                LOG_ERR("Wrong port id");
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

        return __rmt_queue_send_add(instance, id);
}
EXPORT_SYMBOL(rmt_queue_send_add);

int rmt_queue_send_delete(struct rmt * instance,
                          port_id_t    id)
{
        LOG_MISSING;

        return -1;
}
EXPORT_SYMBOL(rmt_queue_send_delete);

static int __rmt_queue_recv_add(struct rmt * instance,
                                port_id_t    id)
{
        struct rmt_queue * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return -1;

        tmp->queue = rfifo_create();
        if (!tmp->queue) {
                rkfree(tmp);
                return -1;
        }

        INIT_HLIST_NODE(&tmp->hlist);
        hash_add(instance->egress.queues->queues, &tmp->hlist, id);
        tmp->port_id = id;
        spin_lock_init(&tmp->lock);

        LOG_DBG("Added receive queue to rmt %pK for port id %d", instance, id);

        return 0;
}

int rmt_queue_recv_add(struct rmt * instance,
                       port_id_t    id)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        if (!is_port_id_ok(id)) {
                LOG_ERR("Wrong port id");
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

        return __rmt_queue_recv_add(instance, id);
}
EXPORT_SYMBOL(rmt_queue_recv_add);

int rmt_queue_recv_delete(struct rmt * instance,
                          port_id_t    id)
{
        LOG_MISSING;

        return -1;
}
EXPORT_SYMBOL(rmt_queue_recv_delete);

static int process_mgmt_pdu(struct rmt * rmt,
                            port_id_t    port_id,
                            struct pdu * pdu)
{
        struct buffer * buffer;
        struct sdu    * sdu;

        ASSERT(rmt);
        ASSERT(is_port_id_ok(port_id));
        ASSERT(pdu);

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

        if (pdu_buffer_set(pdu, NULL)) {
                pdu_destroy(pdu);
                /* FIXME: buffer is owned by PDU and SDU, we're leaking sdu */
                return -1;
        }

        pdu_destroy(pdu);

        ASSERT(rmt->parent);
        ASSERT(rmt->parent->ops);
        ASSERT(rmt->parent->ops->management_sdu_post);

        return (rmt->parent->ops->management_sdu_post(rmt->parent->data,
                                                      port_id,
                                                      sdu) ? -1 : 0);
}

/* FIXME: (FUTURE) process_dt_pdu should process multiple PDUs at time */
static int process_dt_pdu(struct rmt * rmt,
                          port_id_t    port_id,
                          struct pdu * pdu)
{
        const struct pci * p;
        cep_id_t           c;
        address_t          dest_add;

        /* (FUTURE) Address and qos-id are the same, do a single match only */

        p = pdu_pci_get_ro(pdu);
        if (!p) {
                LOG_ERR("Cannot get PCI from PDU");
                pdu_destroy(pdu);
                return -1;
        }

        dest_add = pci_destination(p);
        if (!is_address_ok(dest_add)) {
                LOG_ERR("Wrong destination address");
                pdu_destroy(pdu);
                return -1;
        }

        if (rmt->address != dest_add) {
                size_t *     size;
                port_id_t *  pid;
                qos_id_t     qos_id;
                int          i;
                struct sdu * sdu;

                size   = 0;
                qos_id = pci_qos_id(p);
                if (pft_nhop(rmt->pft, dest_add, qos_id, &pid, size)) {
                        LOG_ERR("No next hops");
                        pdu_destroy(pdu);
                        return -1;
                }

                /* (FUTURE) Construct a SDU with all the ingress PDUs */

                sdu = sdu_create_pdu_with(pdu);
                if (!sdu) {
                        pdu_destroy(pdu);
                        return -1;
                }
                pdu_destroy(pdu);

                for (i = 0; i < *size; i++) {
                        if (kfa_flow_sdu_write(rmt->kfa, pid[i], sdu))
                                LOG_ERR("Cannot write SDU to KFA port-id %d",
                                        pid[i]);
                }

                return 0;
        }

        c = pci_cep_destination(p);
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

static int rmt_receive_worker(void * o)
{
        struct rmt * tmp;

        LOG_DBG("RMT receive worker called");

        tmp = (struct rmt *) o;
        if (!tmp) {
                LOG_ERR("No instance passed to receive worker!!!");
                return -1;
        }

        for (;;) {
                pdu_type_t          pdu_type;
                struct rmt_queue *  entry;
                int                 bucket;
                struct hlist_node * ntmp;

                hash_for_each_safe(tmp->egress.queues->queues,
                                   bucket,
                                   ntmp,
                                   entry,
                                   hlist) {
                        struct sdu * sdu;
                        struct pdu * pdu;
                        port_id_t    port_id;

                        ASSERT(entry);

                        spin_lock(&entry->lock);
                        sdu     = (struct sdu *) rfifo_pop(entry->queue);
                        port_id = entry->port_id;
                        spin_unlock(&entry->lock);

                        if (!sdu) {
                                LOG_ERR("No SDU to work with");
                                break;
                        }

                        /* (FUTURE) This SDU could be holding more PDUs ...*/

                        /* (FUTURE) for_each_pdu_in_sdu() ... */

                        pdu = pdu_create_with(sdu);
                        if (!pdu) {
                                LOG_ERR("No PDU to work with");
                                break;
                        }

                        pdu_type = pci_type(pdu_pci_get_rw(pdu));
                        if (!pdu_type_is_ok(pdu_type)) {
                                LOG_ERR("Wrong PDU type");
                                pdu_destroy(pdu);
                                break;
                        }

                        /* (FUTURE) PDU ownership is going to be passed on */

                        switch (pdu_type) {
                        case PDU_TYPE_MGMT:
                                process_mgmt_pdu(tmp, port_id, pdu);
                                break;
                        case PDU_TYPE_DT:
                                /*
                                 * (FUTURE)
                                 *
                                 * enqueue PDU in pdus_dt[dest-addr, qos-id]
                                 * don't process it now ...
                                 */
                                process_dt_pdu(tmp, port_id, pdu);
                                break;
                        default:
                                LOG_ERR("Unknown PDU type %d", pdu_type);
                                pdu_destroy(pdu);
                                break;
                        }

                        /* (FUTURE) foreach_end() */
                }

                break;
        }

        /* (FUTURE) for-each list in pdus_dt call process_dt_pdus(pdus_dt) */

        spin_lock(&tmp->egress.queues->lock);
        tmp->egress.queues->in_use = 0;
        spin_unlock(&tmp->egress.queues->lock);

        return 0;
}

int rmt_receive(struct rmt * instance,
                struct sdu * sdu,
                port_id_t    from)
{
        struct rwq_work_item * item;
        struct rmt_queue *     rcv_queue;

        if (!instance) {
                LOG_ERR("No RMT passed");
                return -1;
        }

        if (!sdu) {
                LOG_ERR("Bogus PDU passed");
                return -1;
        }
        if (!is_port_id_ok(from)) {
                LOG_ERR("Wrong port id");
                return -1;
        }

        spin_lock(&instance->egress.queues->lock);
        if (!instance->egress.queues) {
                spin_unlock(&instance->egress.queues->lock);
                return -1;
        }

        rcv_queue = qmap_find(instance->egress.queues, from);
        if (!rcv_queue) {
                spin_unlock(&instance->egress.queues->lock);
                return -1;
        }
        spin_lock(&rcv_queue->lock);
        spin_unlock(&instance->egress.queues->lock);
        if (rfifo_push_ni(rcv_queue->queue, sdu)) {
                spin_unlock(&rcv_queue->lock);
                return -1;
        }
        spin_unlock(&rcv_queue->lock);

        spin_lock(&instance->egress.queues->lock);
        if (instance->egress.queues->in_use) {
                LOG_DBG("Work already posted, nothing more to do");
                spin_unlock(&instance->egress.queues->lock);
                return 0;
        }
        instance->egress.queues->in_use = 1;
        spin_unlock(&instance->egress.queues->lock);

        /* Is this _ni() call really necessary ??? */
        item = rwq_work_create_ni(rmt_receive_worker, instance);
        if (!item) {
                LOG_ERR("Couldn't create rwq work");
                return -1;
        }

        ASSERT(instance->ingress.wq);

        if (rwq_work_post(instance->ingress.wq, item)) {
                LOG_ERR("Couldn't put work in the ingress workqueue");
                return -1;
        }

        return 0;
}

/* FIXME: To be rearranged */
static bool is_rmt_pft_ok(struct rmt * instance)
{ return (instance && instance->pft) ? true : false; }

int rmt_pft_flush(struct rmt * instance)
{ return (is_rmt_pft_ok(instance) ? pft_flush(instance->pft) : -1); }

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
