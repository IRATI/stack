/*
 * RMT (Relaying and Multiplexing Task)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
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

struct rs_queue {
        struct rfifo *    queue;
        port_id_t         port_id;
        struct hlist_node hlist;
        spinlock_t        lock;
};

static int rs_queue_destroy(struct rs_queue * send_q)
{
        if (!send_q)
                return -1;

        LOG_DBG("Destroying rs-queue %pK", send_q);

        rfifo_destroy(send_q->queue, (void (*)(void *)) pdu_destroy);
        hash_del(&send_q->hlist);
        rkfree(send_q);

        return 0;
}

struct rmt_queue {
        DECLARE_HASHTABLE(queues, 7);
        spinlock_t    lock;
        int           in_use;
};

static struct rmt_queue * rmtq_create(void)
{
        struct rmt_queue * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        hash_init(tmp->queues);
        spin_lock_init(&tmp->lock);
        tmp->in_use = 0;

        return tmp;
}

static int rmtq_destroy(struct rmt_queue * q)
{
        struct rs_queue *   entry;
        struct hlist_node * tmp;
        int                 bucket;

        if (!q)
                return -1;

        hash_for_each_safe(q->queues, bucket, tmp, entry, hlist) {
                if (rs_queue_destroy(entry)) {
                        LOG_ERR("Could not destroy entry %pK", entry);
                        return -1;
                }
        }

        rkfree(q);

        return 0;
}

struct mgmt_data {
        struct rfifo *    sdu_ready;
        wait_queue_head_t readers;
        spinlock_t        lock;
};

struct rmt {
        struct pft *              pft;
        struct kfa *              kfa;
        struct efcp_container *   efcpc;
        struct workqueue_struct * egress_wq;
        struct workqueue_struct * ingress_wq;
        address_t                 address;
        struct rmt_queue *        send_queues;
        struct rmt_queue *        recv_queues;
        struct mgmt_data *        mgmt_data;
};

static struct mgmt_data * rmt_mgmt_data_create(void)
{
        struct mgmt_data * data;

        data = rkzalloc(sizeof(*data), GFP_KERNEL);
        if (!data) {
                LOG_ERR("Could not allocate memory for RMT mgmt struct");
                return NULL;
        }

        data->sdu_ready = rfifo_create();
        if (!data->sdu_ready) {
                LOG_ERR("Could not create MGMT SDUs queue");
                rfifo_destroy(data->sdu_ready, sdu_wpi_destructor);
                rkfree(data);
                return NULL;
        }

        init_waitqueue_head(&data->readers);
        spin_lock_init(&data->lock);

        return data;

}

struct rmt * rmt_create(struct kfa *            kfa,
                        struct efcp_container * efcpc)
{
        struct rmt * tmp;
        char         string_rmt_id[30];

        if (!kfa)
                return NULL;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->pft = pft_create();
        if (!tmp->pft) {
                rkfree(tmp);
                return NULL;
        }

        tmp->mgmt_data = rmt_mgmt_data_create();
        if (!tmp->mgmt_data) {
                rmt_destroy(tmp);
                return NULL;
        }

        tmp->kfa   = kfa;
        tmp->efcpc = efcpc;

        snprintf(string_rmt_id, 30, "rmt-egress-wq-%pK", tmp);
        tmp->egress_wq = rwq_create(string_rmt_id);
        if (!tmp->egress_wq) {
                rmt_destroy(tmp);
                return NULL;
        }

        snprintf(string_rmt_id, 30, "rmt-ingress-wq-%pK", tmp);
        tmp->ingress_wq = rwq_create(string_rmt_id);
        if (!tmp->ingress_wq) {
                rmt_destroy(tmp);
                return NULL;
        }

        tmp->address = address_bad();

        tmp->send_queues = rmtq_create();
        if (!tmp->send_queues) {
                rmt_destroy(tmp);
                return NULL;
        }

        tmp->recv_queues = rmtq_create();
        if (!tmp->recv_queues) {
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

        ASSERT(instance->pft);

        pft_destroy(instance->pft);

        if (instance->mgmt_data) {
                if (instance->mgmt_data->sdu_ready)
                        rfifo_destroy(instance->mgmt_data->sdu_ready,
                                      sdu_wpi_destructor);
                rkfree(instance->mgmt_data);
        }

        if (instance->egress_wq)  rwq_destroy(instance->egress_wq);
        if (instance->ingress_wq) rwq_destroy(instance->ingress_wq);

        if (instance->send_queues)
                rmtq_destroy(instance->send_queues);

        if (instance->recv_queues)
                rmtq_destroy(instance->recv_queues);

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
        struct kfa *       kfa;
        struct rmt_queue * rmt_q;
};

static struct send_data * send_data_create(struct kfa *       kfa,
                                           struct rmt_queue * queues)
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
        tmp->rmt_q = queues;

        return tmp;
}

static bool is_send_data_complete(const struct send_data * data)
{
        bool ret;

        ret = ((!data || !data->rmt_q || !data->kfa) ? false : true);

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
        sdu = sdu_create_with(tmp_buff);
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
        struct rs_queue *   entry;
        bool                out;
        struct hlist_node * ntmp;
        int                 bucket;

        out = false;
        tmp = (struct send_data *) o;
        if (!tmp) {
                LOG_ERR("No send data passed");
                return -1;
        }

        if (!tmp->rmt_q) {
                LOG_ERR("No RMT queues passed");
                return -1;
        }

        if (!tmp->kfa) {
                LOG_ERR("No KFA passed");
                spin_lock(&tmp->rmt_q->lock);
                tmp->rmt_q->in_use = 0;
                spin_unlock(&tmp->rmt_q->lock);
                return -1;
        }

        while (!out) {
                out = true;
                hash_for_each_safe(tmp->rmt_q->queues,
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

                        /* FIXME : Port id will be retrieved from the pduft */
                        LOG_DBG("Gonna SEND sdu to port_id %d", port_id);
                        if (kfa_flow_sdu_write(tmp->kfa,
                                               port_id,
                                               sdu)) {
                                LOG_ERR("Couldn't write SDU to KFA");
                        }
                }
        }

        spin_lock(&tmp->rmt_q->lock);
        tmp->rmt_q->in_use = 0;
        spin_unlock(&tmp->rmt_q->lock);

        return 0;
}

static struct rs_queue * find_rs_queue(struct rmt_queue * rq,
                                       port_id_t          id)
{
        struct rs_queue *         entry;
        const struct hlist_head * head;

        if (!rq) {
                LOG_ERR("Cannot look-up in a empty map");
                return NULL;
        }

        if (!is_port_id_ok(id)) {
                LOG_ERR("Bogus port id");
                return NULL;
        }

        head = &rq->queues[rmap_hash(rq->queues, id)];
        hlist_for_each_entry(entry, head, hlist) {
                if (entry->port_id == id)
                        return entry;
        }

        return NULL;
}

static int rmt_send_port_id(struct rmt *  instance,
                            port_id_t     id,
                            struct pdu *  pdu)
{
        struct rs_queue *      squeue;
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
        spin_lock(&instance->send_queues->lock);
        squeue = find_rs_queue(instance->send_queues, id);
        if (!squeue) {
                spin_unlock(&instance->send_queues->lock);
                return -1;
        }

        spin_lock(&squeue->lock);
        spin_unlock(&instance->send_queues->lock);
        if (rfifo_push_ni(squeue->queue, pdu)) {
                spin_unlock(&squeue->lock);
                return -1;
        }
        spin_unlock(&squeue->lock);

        spin_lock(&instance->send_queues->lock);
        if (instance->send_queues->in_use) {
                LOG_DBG("Work already posted, nothing more to do");
                spin_unlock(&instance->send_queues->lock);
                return 0;
        }
        instance->send_queues->in_use = 1;
        spin_unlock(&instance->send_queues->lock);

        data = send_data_create(instance->kfa, instance->send_queues);
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

        ASSERT(instance->egress_wq);

        if (rwq_work_post(instance->egress_wq, item)) {
                send_data_destroy(data);
                pdu_destroy(pdu);
                return -1;
        }

        return 0;
}

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
        id = (address == 16 ? 2 : 1); /* FIXME: We must call PDU FT */

        LOG_DBG("Gonna SEND to port_id: %d", id);

        if (rmt_send_port_id(instance, id, pdu))
                return -1;

        return 0;
}
EXPORT_SYMBOL(rmt_send);

int rmt_send_queue_add(struct rmt * instance,
                       port_id_t    id)
{
        struct rs_queue * tmp;

        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        if (!is_port_id_ok(id)) {
                LOG_ERR("Wrong port id");
                return -1;
        }

        if (!instance->send_queues) {
                LOG_ERR("Invalid RMT");
                return -1;
        }

        if (find_rs_queue(instance->send_queues, id)) {
                LOG_ERR("Queue already exists");
                return -1;
        }

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return -1;

        tmp->queue = rfifo_create();
        if (!tmp->queue) {
                rkfree(tmp);
                return -1;
        }
        INIT_HLIST_NODE(&tmp->hlist);
        hash_add(instance->send_queues->queues, &tmp->hlist, id);
        tmp->port_id = id;
        spin_lock_init(&tmp->lock);

        LOG_DBG("Added send queue to rmt %pK for port id %d", instance, id);

        return 0;
}
EXPORT_SYMBOL(rmt_send_queue_add);

int rmt_management_sdu_read(struct rmt *      instance,
                            struct sdu_wpi ** sdu_wpi)
{

        int retval;

        IRQ_BARRIER;

        spin_lock(&instance->mgmt_data->lock);
        while (rfifo_is_empty(instance->mgmt_data->sdu_ready)) {
                LOG_DBG("Mgmt read going to sleep...");
                spin_unlock(&instance->mgmt_data->lock);
                retval = wait_event_interruptible(instance->mgmt_data->readers,
                                                  !rfifo_is_empty(instance->mgmt_data->sdu_ready));

                if (retval) {
                        LOG_ERR("Mgmt queue waken up by interruption, "
                                "bailing out");
                        return retval;
                }
                spin_lock(&instance->mgmt_data->lock);
        }

        if (rfifo_is_empty(instance->mgmt_data->sdu_ready)) {
                spin_unlock(&instance->mgmt_data->lock);
                return -1;
        }
        ASSERT(!rfifo_is_empty(instance->mgmt_data->sdu_ready));

        *sdu_wpi = rfifo_pop(instance->mgmt_data->sdu_ready);

        spin_unlock(&instance->mgmt_data->lock);

        if (!sdu_wpi_is_ok(*sdu_wpi)) {
                LOG_ERR("There is not enough data in the management queue");
                return -1;
        }

        return 0;
}
EXPORT_SYMBOL(rmt_management_sdu_read);

int rmt_recv_queue_add(struct rmt * instance,
                       port_id_t    id)
{
        struct rs_queue * tmp;

        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        if (!is_port_id_ok(id)) {
                LOG_ERR("Wrong port id");
                return -1;
        }

        if (!instance->recv_queues) {
                LOG_ERR("Invalid RMT");
                return -1;
        }

        if (find_rs_queue(instance->recv_queues, id)) {
                LOG_ERR("Queue already exists");
                return -1;
        }

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return -1;

        tmp->queue = rfifo_create();
        if (!tmp->queue) {
                rkfree(tmp);
                return -1;
        }
        INIT_HLIST_NODE(&tmp->hlist);
        hash_add(instance->recv_queues->queues, &tmp->hlist, id);
        tmp->port_id = id;
        spin_lock_init(&tmp->lock);

        LOG_DBG("Added receive queue to rmt %pK for port id %d", instance, id);

        return 0;
}
EXPORT_SYMBOL(rmt_recv_queue_add);

static int rmt_receive_worker(void * o)
{
        struct rmt *        tmp;
        pdu_type_t          pdu_type;
        address_t           dest_add;
        struct rs_queue *   entry;
        bool                out;
        struct hlist_node * ntmp;
        int                 bucket;

        LOG_DBG("RMT receive worker called");
        out = false;

        tmp = (struct rmt *) o;
        if (!tmp) {
                LOG_ERR("No send data passed");
                return -1;
        }

        while (!out) {
                out = true;
                hash_for_each_safe(tmp->recv_queues->queues,
                                   bucket,
                                   ntmp,
                                   entry,
                                   hlist) {
                        struct sdu * sdu;
                        struct pdu * pdu;
                        port_id_t port_id;

                        spin_lock(&entry->lock);
                        sdu = (struct sdu *) rfifo_pop(entry->queue);
                        port_id = entry->port_id;
                        spin_unlock(&entry->lock);
                        if (!sdu) {
                                break;
                        }
                        out = false;
                        pdu = pdu_create_with(sdu);
                        if (!pdu) {
                                break;
                        }
                        dest_add = pci_destination(pdu_pci_get_ro(pdu));
                        if (!is_address_ok(dest_add)) {
                                break;
                        }
                        if (tmp->address != dest_add) {
                                /*
                                 * FIXME : Port id will be retrieved
                                 * from the pduft
                                 */
                                kfa_flow_sdu_write(tmp->kfa,
                                                   port_id_bad(),
                                                   sdu);
                                break;
                        }
                        pdu_type = pci_type(pdu_pci_get_rw(pdu));
                        if (!pdu_type_is_ok(pdu_type)) {
                                pdu_destroy(pdu);
                                break;
                        }
                        switch (pdu_type) {
                        case PDU_TYPE_MGMT: {
                                struct buffer  * buffer;
                                struct sdu_wpi * sdu_wpi;

                                buffer = pdu_buffer_get_rw(pdu);
                                sdu_wpi = sdu_wpi_create_with(buffer);
                                if (!sdu_wpi)
                                        break;

                                sdu_wpi->port_id = port_id;
                                spin_lock(&tmp->mgmt_data->lock);
                                if (rfifo_push_ni(tmp->mgmt_data->sdu_ready,
                                                  sdu_wpi)) {
                                        spin_unlock(&tmp->mgmt_data->lock);
                                        break;
                                }
                                spin_unlock(&tmp->mgmt_data->lock);
                                wake_up(&tmp->mgmt_data->readers);
                                break;
                        }
                        case PDU_TYPE_DT: {
                                efcp_container_receive(tmp->efcpc,
                                                       pci_cep_destination(pdu_pci_get_ro(pdu)),
                                                       pdu);
                                break;
                        }
                        default:
                                LOG_ERR("Unknown PDU type %d", pdu_type);
                        }
                }
        }
        spin_lock(&tmp->recv_queues->lock);
        tmp->recv_queues->in_use = 0;
        spin_unlock(&tmp->recv_queues->lock);

        return 0;
}

int rmt_receive(struct rmt * instance,
                struct sdu * sdu,
                port_id_t    from)
{
        struct rwq_work_item * item;
        struct rs_queue *      rcv_queue;

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

        spin_lock(&instance->recv_queues->lock);
        rcv_queue = find_rs_queue(instance->recv_queues, from);
        if (!rcv_queue) {
                spin_unlock(&instance->recv_queues->lock);
                return -1;
        }
        spin_lock(&rcv_queue->lock);
        spin_unlock(&instance->recv_queues->lock);
        if (rfifo_push_ni(rcv_queue->queue, sdu)) {
                spin_unlock(&rcv_queue->lock);
                return -1;
        }
        spin_unlock(&rcv_queue->lock);

        spin_lock(&instance->recv_queues->lock);
        if (instance->recv_queues->in_use) {
                LOG_DBG("Work already posted, nothing more to do");
                spin_unlock(&instance->recv_queues->lock);
                return 0;
        }
        instance->recv_queues->in_use = 1;
        spin_unlock(&instance->recv_queues->lock);

        /* Is this _ni() call really necessary ??? */
        item = rwq_work_create_ni(rmt_receive_worker, instance);
        if (!item) {
                LOG_ERR("Couldn't create rwq work");
                return -1;
        }

        ASSERT(instance->ingress_wq);

        if (rwq_work_post(instance->ingress_wq, item)) {
                LOG_ERR("Couldn't put work in the ingress workqueue");
                return -1;
        }

        return 0;
}
