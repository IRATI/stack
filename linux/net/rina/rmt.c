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

#define RINA_PREFIX "rmt"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "du.h"
#include "rmt.h"
#include "pft.h"
#include "efcp-utils.h"

struct rmt {
        struct pft *              pft;
        struct kfa *              kfa;
        struct efcp_container *   efcpc;
        struct workqueue_struct * egress_wq;
        struct workqueue_struct * ingress_wq;
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

struct send_data {
        struct rmt * rmt;
        struct pdu * pdu;
        address_t    address;
        cep_id_t     connection_id;
};

bool is_send_data_complete(const struct send_data * data)
{
        bool ret;

        ret = ((!data || !data->rmt || !data->pdu) ? false : true);

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

static struct send_data * send_data_create(struct rmt * rmt,
                                           struct pdu * pdu,
                                           address_t    address,
                                           cep_id_t     connection_id)
{
        struct send_data * tmp;

        tmp = rkmalloc(sizeof(*tmp), GFP_ATOMIC);
        if (!tmp)
                return NULL;

        tmp->rmt           = rmt;
        tmp->pdu           = pdu;
        tmp->address       = address;
        tmp->connection_id = connection_id;

        return tmp;
}

static int rmt_send_worker(void * o)
{
        struct send_data *    tmp;
        struct sdu *          sdu;
        const struct buffer * buffer;
        const struct pci *    pci;
        const void *          buffer_data;
        size_t                size;
        ssize_t               buffer_size;
        ssize_t               pci_size;
        struct buffer *       tmp_buff;
        char *                data;

        tmp = (struct send_data *) o;
        if (!tmp) {
                LOG_ERR("No send data passed");
                return -1;
        }

        if (!is_send_data_complete(tmp)) {
                LOG_ERR("Wrong data passed to RMT send worker");
                send_data_destroy(tmp);
                return -1;
        }

        buffer = pdu_buffer_get_ro(tmp->pdu);
        if (!buffer)
                return -1;

        buffer_data = buffer_data_ro(buffer);
        if (!buffer_data)
                return -1;

        pci = pdu_pci_get_ro(tmp->pdu);
        if (!pci)
                return -1;

        buffer_size = buffer_length(buffer);
        if (buffer_size <= 0)
                return -1;

        pci_size = pci_length(pci);
        if (pci_size <= 0)
                return -1;

        size = pci_size + buffer_size;
        data = rkmalloc(size, GFP_KERNEL);
        if (!data)
                return -1;

        if (!memcpy(data, pci, pci_size)) {
                rkfree(data);
                return -1;
        }
        if (!memcpy(data + pci_size, buffer_data, buffer_size)) {
                rkfree(data);
                return -1;
        }
        tmp_buff = buffer_create_with(data, size);
        if (!tmp_buff) {
                rkfree(data);
                return -1;
        }
        sdu = sdu_create_with(tmp_buff);
        if (!sdu) {
                buffer_destroy(tmp_buff);
                return -1;
        }
        pdu_destroy(tmp->pdu);
        /* FIXME : Port id will be retrieved from the pduft */
        if (kfa_flow_sdu_write(tmp->rmt->kfa, port_id_bad(), sdu)) {
                return -1;
        }

        return 0;
}

int rmt_send(struct rmt * instance,
             address_t    address,
             cep_id_t     connection_id,
             struct pdu * pdu)
{
        struct send_data *     tmp;
        struct rwq_work_item * item;

        LOG_MISSING;

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

        tmp = send_data_create(instance, pdu, address, connection_id);
        if (!is_send_data_complete(tmp))
                return -1;

        /* Is this _ni() call really necessary ??? */
        item = rwq_work_create_ni(rmt_send_worker, tmp);
        if (!item) {
                send_data_destroy(tmp);
                return -1;
        }

        ASSERT(instance->egress_wq);

        if (rwq_work_post(instance->egress_wq, item)) {
                send_data_destroy(tmp);
                pdu_destroy(pdu);
                return -1;
        }

        return 0;
}

struct receive_data {
        port_id_t               from;
        struct sdu *            sdu;
        struct kfa *            kfa;
        struct efcp_container * efcpc;
};

static struct receive_data *
receive_data_create(port_id_t               from,
                    struct sdu *            sdu,
                    struct kfa *            kfa,
                    struct efcp_container * efcpc)
{
        struct receive_data * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->from  = from;
        tmp->sdu   = sdu;
        tmp->kfa   = kfa;
        tmp->efcpc = efcpc;

        return tmp;
}

static bool is_receive_data_complete(const struct receive_data * data)
{
        LOG_MISSING;

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
        if (!pdu) {
                receive_data_destroy(tmp);
                return -1;
        }

        pdu_type = pci_type(pdu_pci_get_rw(pdu));
        switch (pdu_type) {
        case PDU_TYPE_MGMT: {
                struct sdu *    sdu;
                struct buffer * buffer;

                buffer = pdu_buffer_get_rw(pdu);
                sdu = sdu_create_with(buffer);
                if (!sdu) {
                        receive_data_destroy(tmp);
                        return -1;
                }
                if (kfa_sdu_post_to_user_space(tmp->kfa, sdu, tmp->from)) {
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

        data = receive_data_create(from, sdu, instance->kfa, instance->efcpc);
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
