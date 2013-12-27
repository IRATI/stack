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

#define RINA_PREFIX "rmt"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "du.h"
#include "rmt.h"
#include "pft.h"
#include "efcp-utils.h"

struct rmt_queue {
        DECLARE_HASHTABLE(queues, 7);
        spinlock_t    lock;
        int           in_use;
};

#define rmap_hash(T, K) hash_min(K, HASH_BITS(T))

struct send_queue {
        struct rqueue *   queue;
        port_id_t         port_id;
        struct hlist_node hlist;
        spinlock_t        lock;
};

struct rmt {
        struct pft *              pft;
        struct kfa *              kfa;
        struct efcp_container *   efcpc;
        struct workqueue_struct * egress_wq;
        struct workqueue_struct * ingress_wq;
        address_t                 address;
        struct rfifo *            mgmt_sdu_wpi_queue;
        struct rmt_queue *        send_queues;
        /* HASH_TABLE(queues, port_id_t, rmt_queues_t *); */
};

struct rmt * rmt_create(struct kfa *            kfa,
                        struct efcp_container * efcpc)
{
        struct rmt * tmp;

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

        tmp->kfa   = kfa;
        tmp->efcpc = efcpc;

        /* FIXME: the name should be unique, not shared with all the RMT's */
        tmp->egress_wq = rwq_create("rmt-egress-wq");
        if (!tmp->egress_wq) {
                rmt_destroy(tmp);
                return NULL;
        }
        /* FIXME: the name should be unique, not shared with all the RMT's */
        tmp->ingress_wq = rwq_create("rmt-ingress-wq");
        if (!tmp->ingress_wq) {
                rwq_destroy(tmp->egress_wq);
                rmt_destroy(tmp);
                return NULL;
        }

        tmp->address = address_bad();

        tmp->send_queues = rkzalloc(sizeof(struct rmt_queue), GFP_KERNEL);
        if (!tmp->send_queues) {
                rwq_destroy(tmp->egress_wq);
                rwq_destroy(tmp->ingress_wq);
                rmt_destroy(tmp);
                return NULL;
        }
        hash_init(tmp->send_queues->queues);
        spin_lock_init(&tmp->send_queues->lock);
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
        if (instance->egress_wq) rwq_destroy(instance->egress_wq);
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
                LOG_ERR("The RMT already has an address");
                return -1;
        }

        instance->address = address;

        return 0;
}
EXPORT_SYMBOL(rmt_address_set);

int rmt_mgmt_sdu_wpi_queue_set(struct rmt *   instance,
                               struct rfifo * queue)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        if (instance->mgmt_sdu_wpi_queue) {
                LOG_ERR("The RMT is already bound to the MGMT SDU queue");
                return -1;
        }

        instance->mgmt_sdu_wpi_queue = queue;

        return 0;
}
EXPORT_SYMBOL(rmt_mgmt_sdu_wpi_queue_set);

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

        tmp->kfa    = kfa;
        tmp->rmt_q = queues;

        return tmp;
}

bool is_send_data_complete(const struct send_data * data)
{
        bool ret;

        if (!data)
                return false;

        ret = ((!data->rmt_q || !data->kfa) ? false : true);

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

        if (!memcpy(data, pci, pci_size)) {
                rkfree(data);
                return NULL;
        }
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
        struct send_data *    tmp;
        struct send_queue *   entry;
        struct pdu *          pdu;
        int                   out;

        struct hlist_node * ntmp;
        int                 bucket;

        out = 1;
        tmp = (struct send_data *) o;
        if (!tmp) {
                LOG_ERR("No send data passed");
                return -1;
        }

        while (out) {
                out = 0;
                spin_lock(&tmp->rmt_q->lock);
                hash_for_each_safe(tmp->rmt_q->queues, bucket, ntmp, entry, hlist) {
                        struct sdu * sdu;
                        port_id_t port_id;
                        spin_lock(&entry->lock);
                        pdu = (struct pdu *) rqueue_head_pop(entry->queue);
                        port_id = entry->port_id;
                        spin_unlock(&entry->lock);
                        spin_unlock(&tmp->rmt_q->lock);
                        if (!pdu)
                                break;

                        out = out + 1;
                        sdu = pdu_process(pdu);
                        if (!sdu)
                                break;

                        /* FIXME : Port id will be retrieved from the pduft */
                        if (kfa_flow_sdu_write(tmp->kfa,
                                               port_id,
                                               sdu)) {
                                LOG_ERR("Couldn't write SDU to KFA");
                        }
                }

        }

        return 0;
}

static struct send_queue * find_queue(struct rmt_queue * rq,
                                      port_id_t          id)
{
        struct send_queue *       entry;
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
        struct send_queue *    squeue;
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
        squeue = find_queue(instance->send_queues, id);
        if (!squeue) {
                spin_unlock(&instance->send_queues->lock);
                return -1;
        }
        spin_lock(&squeue->lock);
        spin_unlock(&instance->send_queues->lock);
        if (rqueue_tail_push_ni(squeue->queue, &pdu)) {
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
        if (!data) {
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
        id = port_id_bad(); /* FIXME: We must call PDU FT */
        if (rmt_send_port_id(instance, id, pdu))
                return -1;

        return 0;
}
EXPORT_SYMBOL(rmt_send);

struct receive_data {
        port_id_t               from;
        struct sdu *            sdu;
        struct kfa *            kfa;
        struct efcp_container * efcpc;
        struct rmt *            rmt;
};

static struct receive_data *
receive_data_create(port_id_t               from,
                    struct sdu *            sdu,
                    struct kfa *            kfa,
                    struct efcp_container * efcpc,
                    struct rmt *            rmt)
{
        struct receive_data * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->from  = from;
        tmp->sdu   = sdu;
        tmp->kfa   = kfa;
        tmp->efcpc = efcpc;
        tmp->rmt   = rmt;

        return tmp;
}

static bool is_receive_data_complete(const struct receive_data * data)
{
        if (!data)
                return false;

        if (!data->efcpc ||
            !data->sdu   ||
            !data->rmt   ||
            !data->kfa   ||
            !is_port_id_ok(data->from))
                return false;

        return true;
}

static int receive_data_destroy(struct receive_data * data)
{
        if (!data) {
                LOG_ERR("No receive data passed");
                return -1;
        }

        if (data->sdu) sdu_destroy(data->sdu);

        rkfree(data);

        return 0;
}

static int rmt_receive_worker(void * o)
{
        struct receive_data * tmp;
        struct pdu *          pdu;
        pdu_type_t            pdu_type;
        address_t             dest_add;

        tmp = (struct receive_data *) o;
        if (!tmp) {
                LOG_ERR("No data passed");
                return -1;
        }

        if (!is_receive_data_complete(tmp)) {
                LOG_ERR("Wrong receive data passed");
                receive_data_destroy(tmp);
                return -1;
        }

        pdu = pdu_create_with(tmp->sdu);
        if (!pdu_is_ok(pdu)) {
                receive_data_destroy(tmp);
                return -1;
        }

        dest_add = pci_destination(pdu_pci_get_ro(pdu));
        if (!is_address_ok(dest_add)) {
                receive_data_destroy(tmp);
                return -1;
        }

        if (tmp->rmt->address != dest_add) {
                /* FIXME : Port id will be retrieved from the pduft */
                if (kfa_flow_sdu_write(tmp->rmt->kfa,
                                       port_id_bad(),
                                       tmp->sdu)) {
                        receive_data_destroy(tmp);
                        return -1;
                }
        }

        pdu_type = pci_type(pdu_pci_get_rw(pdu));
        if (!pdu_type_is_ok(pdu_type)) {
                receive_data_destroy(tmp);
                return -1;
        }
        switch (pdu_type) {
        case PDU_TYPE_MGMT: {
                struct buffer  * buffer;
                struct sdu_wpi * sdu_wpi;

                buffer = pdu_buffer_get_rw(pdu);
                sdu_wpi = sdu_wpi_create_with(buffer);
                if (!sdu_wpi) {
                        receive_data_destroy(tmp);
                        return -1;
                }
                sdu_wpi->port_id = tmp->from;
                if (rfifo_push(tmp->rmt->mgmt_sdu_wpi_queue, &sdu_wpi)) {
                        receive_data_destroy(tmp);
                        return -1;
                }

                return 0;
        }
        case PDU_TYPE_DT: {
                if (efcp_container_receive(tmp->efcpc,
                                           pci_cep_destination(pdu_pci_get_ro(pdu)),
                                           pdu)) {
                        receive_data_destroy(tmp);
                        return -1;
                }

                return 0;
        }
        default:
                LOG_ERR("Unknown PDU type %d", pdu_type);
                return -1;
        }
}

int rmt_receive(struct rmt * instance,
                struct sdu * sdu,
                port_id_t    from)
{
        struct receive_data *  data;
        struct rwq_work_item * item;

        if (!instance) {
                LOG_ERR("No RMT passed");
                return -1;
        }

        data = receive_data_create(from,
                                   sdu,
                                   instance->kfa,
                                   instance->efcpc,
                                   instance);
        if (!is_receive_data_complete(data)) {
                if (data)
                        rkfree(data);
                return -1;
        }

        /* Is this _ni() call really necessary ??? */
        item = rwq_work_create_ni(rmt_receive_worker, data);
        if (!item) {
                rkfree(data);
                return -1;
        }

        ASSERT(instance->ingress_wq);

        if (rwq_work_post(instance->ingress_wq, item)) {
                receive_data_destroy(data);
                return -1;
        }

        return 0;
}
