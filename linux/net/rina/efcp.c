/*
 * EFCP (Error and Flow Control Protocol)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
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
#include <linux/kobject.h>

#define RINA_PREFIX "efcp"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "efcp.h"
#include "efcp-utils.h"
#include "dtp.h"
#include "dtcp.h"
#include "rmt.h"

struct efcp {
        struct connection *     connection;

        /* FIXME: DTP and DTCP instances to be replaced with DT instance */
        struct dtp *            dtp;
        struct dtcp *           dtcp;

        struct efcp_container * container;
};

static struct efcp * efcp_create(void)
{
        struct efcp * instance;

        instance = rkzalloc(sizeof(*instance), GFP_KERNEL);
        if (!instance)
                return NULL;

        LOG_DBG("Instance %pK initialized successfully", instance);

        return instance;
}

static int efcp_destroy(struct efcp * instance)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        if (instance->dtp)        dtp_unbind(instance->dtp);
        if (instance->dtcp)       dtcp_unbind(instance->dtcp);

        if (instance->dtp)        dtp_destroy(instance->dtp);
        if (instance->dtcp)       dtcp_destroy(instance->dtcp);

        if (instance->connection) rkfree(instance->connection);

        rkfree(instance);

        LOG_DBG("Instance %pK finalized successfully", instance);

        return 0;
}

struct efcp_container {
        struct efcp_imap *        instances;
        struct cidm *             cidm;
        struct dt_cons            dt_cons;
        struct rmt *              rmt;
        struct kfa *              kfa;
        struct workqueue_struct * egress_wq;
        struct workqueue_struct * ingress_wq;
};

struct efcp_container * efcp_container_create(struct kfa * kfa)
{
        struct efcp_container * container;

        if (!kfa) {
                LOG_ERR("Bogus KFA instances passed, bailing out");
                return NULL;
        }

        container = rkzalloc(sizeof(*container), GFP_KERNEL);
        if (!container)
                return NULL;

        container->instances   = efcp_imap_create();
        container->cidm        = cidm_create();
        if (!container->instances ||
            !container->cidm) {
                LOG_ERR("Failed to create EFCP container instance");
                efcp_container_destroy(container);
                return NULL;
        }

        /* FIXME: name should be unique, not shared with all the EFCPC's */
        container->egress_wq = rwq_create("efcpc-egress-wq");
        if (!container->egress_wq) {
                LOG_ERR("Cannot create efcpc egress workqueue");
                efcp_container_destroy(container);
                return NULL;
        }

        /* FIXME: name should be unique, not shared with all the EFCPC's */
        container->ingress_wq = rwq_create("efcpc-ingress-wq");
        if (!container->egress_wq) {
                LOG_ERR("Cannot create efcpc ingress workqueue");
                efcp_container_destroy(container);
                return NULL;
        }

        container->kfa = kfa;

        return container;
}
EXPORT_SYMBOL(efcp_container_create);

int efcp_container_destroy(struct efcp_container * container)
{
        if (!container) {
                LOG_ERR("Bogus container passed, bailing out");
                return -1;
        }

        if (container->instances)  efcp_imap_destroy(container->instances,
                                                     efcp_destroy);
        if (container->cidm)       cidm_destroy(container->cidm);

        if (container->egress_wq)  rwq_destroy(container->egress_wq);
        if (container->ingress_wq) rwq_destroy(container->ingress_wq);

        rkfree(container);

        return 0;
}
EXPORT_SYMBOL(efcp_container_destroy);

int efcp_container_set_dt_cons(struct dt_cons *        dt_cons,
                               struct efcp_container * container)
{
        if (!dt_cons || !container) {
                LOG_ERR("Bogus input parameters, bailing out");
                return -1;
        }

#if 0
        /* FIXME: Why not copying the struct directly ??? */
        container->dt_cons.address_length = dt_cons->address_length;
        container->dt_cons.cep_id_length  = dt_cons->cep_id_length;
        container->dt_cons.length_length  = dt_cons->length_length;
        container->dt_cons.port_id_length = dt_cons->port_id_length;
        container->dt_cons.qos_id_length  = dt_cons->qos_id_length;
        container->dt_cons.seq_num_length = dt_cons->seq_num_length;
        container->dt_cons.max_pdu_size   = dt_cons->max_pdu_size;
        container->dt_cons.max_pdu_life   = dt_cons->max_pdu_life;
        container->dt_cons.dif_integrity  = dt_cons->dif_integrity;
#endif
        container->dt_cons = *dt_cons;

        LOG_DBG("Succesfully set data transfer constants to efcp container");

        return 0;
}
EXPORT_SYMBOL(efcp_container_set_dt_cons);

struct write_data {
        struct efcp * efcp;
        struct sdu *  sdu;
};

static bool is_write_data_ok(const struct write_data * data)
{ return ((!data || !data->efcp || !data->sdu) ? false : true); }

static int write_data_destroy(struct write_data * data)
{
        if (!data) {
                LOG_ERR("No write data passed");
                return -1;
        }

        rkfree(data);

        return 0;
}

static struct write_data * write_data_create(struct efcp * efcp,
                                             struct sdu *  sdu)
{
        struct write_data * tmp;

        tmp = rkmalloc(sizeof(*tmp), GFP_ATOMIC);
        if (!tmp)
                return NULL;

        tmp->efcp = efcp;
        tmp->sdu  = sdu;

        return tmp;
}

static int efcp_write_worker(void * o)
{
        struct write_data * tmp;

        tmp = (struct write_data *) o;
        if (!tmp) {
                LOG_ERR("No write data passed");
                return -1;
        }

        if (!is_write_data_ok(tmp)) {
                LOG_ERR("Wrong data passed to efcp_write_worker");
                write_data_destroy(tmp);
                return -1;
        }

        if (dtp_write(tmp->efcp->dtp, tmp->sdu)) {
                LOG_ERR("Could not send SDU to DTP");
                return -1;
        }

        return 0;
}

static int efcp_write(struct efcp * efcp,
                      struct sdu *  sdu)
{
        struct write_data *    tmp;
        struct rwq_work_item * item;

        if (!efcp) {
                LOG_ERR("Bogus EFCP passed");
                return -1;
        }
        if (!sdu) {
                LOG_ERR("Bogus SDU passed");
                return -1;
        }

        tmp = write_data_create(efcp, sdu);
        ASSERT(is_write_data_ok(tmp));

        /* Is this _ni() really necessary ??? */
        item = rwq_work_create_ni(efcp_write_worker, tmp);
        if (!item) {
                write_data_destroy(tmp);
                return -1;
        }

        ASSERT(efcp->container->egress_wq);

        if (rwq_work_post(efcp->container->egress_wq, item)) {
                write_data_destroy(tmp);
                sdu_destroy(sdu);
                return -1;
        }

        return 0;
}

int efcp_container_write(struct efcp_container * container,
                         cep_id_t                cep_id,
                         struct sdu *            sdu)
{
        struct efcp * efcp;

        if (!container || !sdu_is_ok(sdu)) {
                LOG_ERR("Bogus input parameters, cannot write into container");
                return -1;
        }
        if (!is_cep_id_ok(cep_id)) {
                LOG_ERR("Bad cep-id, cannot write into container");
                return -1;
        }

        efcp = efcp_imap_find(container->instances, cep_id);
        if (!efcp) {
                LOG_ERR("There is no EFCP bound to this cep_id %d", cep_id);
                return -1;
        }

        if (efcp_write(efcp, sdu))
                return -1;

        return 0;
}
EXPORT_SYMBOL(efcp_container_write);

/* FIXME: Goes directly into RMT ... */
int efcp_container_mgmt_write(struct efcp_container * container,
                              address_t               src_address,
                              port_id_t               port_id,
                              struct sdu *            sdu)
{ return dtp_mgmt_write(container->rmt, src_address, port_id, sdu); }
EXPORT_SYMBOL(efcp_container_mgmt_write);

struct receive_data {
        struct efcp * efcp;
        struct pdu *  pdu;
};

static bool is_receive_data_ok(const struct receive_data * data)
{ return ((!data || !data->efcp || !data->pdu) ? false : true); }

static int receive_data_destroy(struct receive_data * data)
{
        if (!data) {
                LOG_ERR("No receive data passed");
                return -1;
        }

        rkfree(data);

        return 0;
}

static struct receive_data * receive_data_create(struct efcp * efcp,
                                                 struct pdu *  pdu)
{
        struct receive_data * tmp;

        tmp = rkmalloc(sizeof(*tmp), GFP_ATOMIC);
        if (!tmp)
                return NULL;

        tmp->efcp = efcp;
        tmp->pdu  = pdu;

        return tmp;
}

static int efcp_receive_worker(void * o)
{
        struct receive_data * tmp;

        tmp = (struct receive_data *) o;
        if (!tmp) {
                LOG_ERR("No receive data passed");
                return -1;
        }

        if (!is_receive_data_ok(tmp)) {
                LOG_ERR("Wrong data passed to efcp_receive_worker");
                receive_data_destroy(tmp);
                return -1;
        }

        if (dtp_receive(tmp->efcp->dtp, tmp->pdu)) {
                LOG_ERR("Could not send SDU to DTP");
                return -1;
        }

        return 0;
}

static int efcp_receive(struct efcp * efcp,
                        struct pdu *  pdu)
{
        struct receive_data *  data;
        struct rwq_work_item * item;

        if (!efcp) {
                LOG_ERR("No efcp instance passed");
                return -1;
        }
        if (!pdu) {
                LOG_ERR("No pdu passed");
                return -1;
        }

        data = receive_data_create(efcp, pdu);
        ASSERT(is_receive_data_ok(data));

        /* Is this _ni() call really necessary ??? */
        item = rwq_work_create_ni(efcp_receive_worker, data);
        if (!item) {
                receive_data_destroy(data);
                return -1;
        }

        ASSERT(efcp->container->ingress_wq);

        if (rwq_work_post(efcp->container->ingress_wq, item)) {
                receive_data_destroy(data);
                pdu_destroy(pdu);
                return -1;
        }

        return 0;
}

int efcp_container_receive(struct efcp_container * container,
                           cep_id_t                cep_id,
                           struct pdu *            pdu)
{
        struct efcp * tmp;

        if (!container || !pdu) {
                LOG_ERR("Bogus input parameters");
                return -1;
        }
        if (!is_cep_id_ok(cep_id)) {
                LOG_ERR("Bad cep-id, cannot write into container");
                return -1;
        }

        tmp = efcp_container_find(container, cep_id);
        if (!tmp) {
                LOG_ERR("Cannot find the requested instance");
                return -1;
        }

        if (efcp_receive(tmp, pdu))
                return -1;

        return 0;
}
EXPORT_SYMBOL(efcp_container_receive);

static int is_connection_ok(const struct connection * connection)
{
        /* FIXME: Add checks for policy params */

        if (!connection                                   ||
            !is_cep_id_ok(connection->source_cep_id)      ||
            !is_cep_id_ok(connection->destination_cep_id) ||
            !is_port_id_ok(connection->port_id))
                return 0;

        return 1;
}

cep_id_t efcp_connection_create(struct efcp_container * container,
                                struct connection *     connection)
{
        struct efcp * tmp;
        cep_id_t      cep_id;

        if (!container) {
                LOG_ERR("Bogus container passed, bailing out");
                return cep_id_bad();
        }
        if (!is_connection_ok(connection)) {
                LOG_ERR("Bogus connection passed, bailing out");
                return cep_id_bad();
        }

        ASSERT(connection);

        tmp = efcp_create();
        if (!tmp)
                return cep_id_bad();

        cep_id = cidm_allocate(container->cidm);

        /* We must ensure that the DTP is instantiated, at least ... */
        tmp->container            = container;
        connection->source_cep_id = cep_id;
        tmp->connection           = connection;

        /* FIXME: dtp_create() takes ownership of the connection parameter */
        tmp->dtp                  = dtp_create(container->rmt,
                                               container->kfa,
                                               connection);
        if (!tmp->dtp) {
                efcp_destroy(tmp);
                return cep_id_bad();
        }

        if (connection->policies_params.dtcp_present) {
                tmp->dtcp = dtcp_create();
                if (!tmp->dtcp) {
                        efcp_destroy(tmp);
                        return cep_id_bad();
                }

                /* No needs to check here, bindings are straightforward */
                dtp_bind(tmp->dtp,   tmp->dtcp);
                dtcp_bind(tmp->dtcp, tmp->dtp);
        }

        if (efcp_imap_add(container->instances,
                          connection->source_cep_id,
                          tmp)) {
                LOG_ERR("Cannot add a new instance into container %pK",
                        container);
                rkfree(connection);
                dtp_destroy(tmp->dtp);
                efcp_destroy(tmp);
                return cep_id_bad();
        }

        LOG_DBG("Connection created ("
                "Source address %d,"
                "Destination address %d, "
                "Destination cep-id %d, "
                "Source cep-id %d)",
                connection->source_address,
                connection->destination_address,
                connection->destination_cep_id,
                connection->source_cep_id);

        return cep_id;
}
EXPORT_SYMBOL(efcp_connection_create);

int efcp_connection_destroy(struct efcp_container * container,
                            cep_id_t                id)
{
        struct efcp * tmp;

        if (!container) {
                LOG_ERR("Bogus container passed, bailing out");
                return -1;
        }
        if (!is_cep_id_ok(id)) {
                LOG_ERR("Bad cep-id, cannot destroy connection");
                return -1;
        }

        tmp = efcp_imap_find(container->instances, id);
        if (!tmp) {
                LOG_ERR("Cannot find instance %d in container %pK",
                        id, container);
                return -1;
        }

        if (efcp_imap_remove(container->instances, id)) {
                LOG_ERR("Cannot remove instance %d from container %pK",
                        id, container);
                return -1;
        }

        if (efcp_destroy(tmp)) {
                LOG_ERR("Cannot destroy instance %d, instance lost", id);
                return -1;
        }

        return 0;
}
EXPORT_SYMBOL(efcp_connection_destroy);

int efcp_connection_update(struct efcp_container * container,
                           cep_id_t                from,
                           cep_id_t                to)
{
        struct efcp * tmp;

        if (!container) {
                LOG_ERR("Bogus container passed, bailing out");
                return -1;
        }
        if (!is_cep_id_ok(from)) {
                LOG_ERR("Bad from cep-id, cannot update connection");
                return -1;
        }
        if (!is_cep_id_ok(to)) {
                LOG_ERR("Bad to cep-id, cannot update connection");
                return -1;
        }

        tmp = efcp_imap_find(container->instances, from);
        if (!tmp) {
                LOG_ERR("Cannot get instance %d from container %pK",
                        from, container);
                return -1;
        }
        tmp->connection->destination_cep_id = to;

        LOG_DBG("Connection updated");
        LOG_DBG("  Source address:     %d",
                tmp->connection->source_address);
        LOG_DBG("  Destination address %d",
                tmp->connection->destination_address);
        LOG_DBG("  Destination cep id: %d",
                tmp->connection->destination_cep_id);
        LOG_DBG("  Source cep id:      %d",
                tmp->connection->source_cep_id);

        return 0;
}
EXPORT_SYMBOL(efcp_connection_update);

struct efcp * efcp_container_find(struct efcp_container * container,
                                  cep_id_t                id)
{
        if (!container) {
                LOG_ERR("Bogus container passed, bailing out");
                return NULL;
        }
        if (!is_cep_id_ok(id)) {
                LOG_ERR("Bad cep-id, cannot find instance");
                return NULL;
        }

        return efcp_imap_find(container->instances, id);
}
EXPORT_SYMBOL(efcp_container_find);

int efcp_bind_rmt(struct efcp_container * container,
                  struct rmt *            rmt)
{
        if (!container) {
                LOG_ERR("Bogus EFCP container passed");
                return -1;
        }
        if (!rmt) {
                LOG_ERR("Bogus RMT instance passed");
                return -1;
        }

        container->rmt = rmt;

        return 0;
}
EXPORT_SYMBOL(efcp_bind_rmt);

int efcp_unbind_rmt(struct efcp_container * container)
{
        if (!container) {
                LOG_ERR("Bogus EFCP container passed");
                return -1;
        }

        container->rmt = NULL;

        return 0;
}
EXPORT_SYMBOL(efcp_unbind_rmt);
