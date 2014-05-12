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
#include "cidm.h"
#include "dt.h"
#include "dtp.h"
#include "dtcp.h"
#include "rmt.h"
#include "dt-utils.h"

#ifndef DTCP_TEST_ENABLE
#define DTCP_TEST_ENABLE 1
#endif

struct efcp {
        struct connection *     connection;
        struct dt *             dt;
        struct efcp_container * container;
};

struct efcp_container {
        struct efcp_imap *        instances;
        struct cidm *             cidm;
        struct dt_cons            dt_cons;
        struct rmt *              rmt;
        struct kfa *              kfa;

        struct workqueue_struct * egress_wq;
        struct workqueue_struct * ingress_wq;
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

        if (instance->dt) {
                struct dtp *  dtp  = dt_dtp_unbind(instance->dt);
                struct dtcp * dtcp = dt_dtcp_unbind(instance->dt);
                struct cwq *  cwq  = dt_cwq_unbind(instance->dt);
                struct rtxq * rtxq = dt_rtxq_unbind(instance->dt);

                /* FIXME: We should watch for memleaks here ... */
                if (dtp)  dtp_destroy(dtp);
                if (dtcp) dtcp_destroy(dtcp);
                if (cwq)  cwq_destroy(cwq);
                if (rtxq) rtxq_destroy(rtxq);

                dt_destroy(instance->dt);
        } else
                LOG_WARN("No DT instance present");

        if (instance->connection) {
                if (is_cep_id_ok(instance->connection->source_cep_id)) {
                        ASSERT(instance->container);
                        ASSERT(instance->container->cidm);

                        cidm_release(instance->container->cidm,
                                     instance->connection->source_cep_id);
                }

                rkfree(instance->connection);
        }

        rkfree(instance);

        LOG_DBG("Instance %pK finalized successfully", instance);

        return 0;
}

#define MAX_NAME_SIZE 128

static const char * create_name(const char *                  prefix,
                                const struct efcp_container * instance)
{
        static char name[MAX_NAME_SIZE];

        ASSERT(prefix);
        ASSERT(instance);

        if (snprintf(name, sizeof(name),
                     RINA_PREFIX "-%s-%pK", prefix, instance) >=
            sizeof(name))
                return NULL;

        return name;
}

struct efcp_container * efcp_container_create(struct kfa * kfa)
{
        struct efcp_container * container;
        const char * name_egress;
        const char * name_ingress;

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

        name_egress = create_name("c-egress-wq", container);
        if (!name_egress) {
                LOG_ERR("Cannot create efcpc egress wq name");
                efcp_container_destroy(container);
                return NULL;
        }
        container->egress_wq = rwq_create(name_egress);
        if (!container->egress_wq) {
                LOG_ERR("Cannot create efcpc egress workqueue");
                efcp_container_destroy(container);
                return NULL;
        }

        name_ingress = create_name("c-ingress-wq", container);
        if (!name_ingress) {
                LOG_ERR("Cannot create efcpc ingress wq name");
                efcp_container_destroy(container);
                return NULL;
        }
        container->ingress_wq = rwq_create(name_ingress);
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

int efcp_container_set_dt_cons(struct dt_cons *        dt_cons,
                               struct efcp_container * container)
{
        if (!dt_cons || !container) {
                LOG_ERR("Bogus input parameters, bailing out");
                return -1;
        }

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
        struct dtp *        dtp;

        tmp = (struct write_data *) o;
        if (!tmp) {
                LOG_ERR("No write data passed");
                return -1;
        }

        if (!is_write_data_ok(tmp)) {
                LOG_ERR("Wrong data passed to efcp_write_worker");
                sdu_destroy(tmp->sdu);
                write_data_destroy(tmp);
                return -1;
        }

        ASSERT(tmp->efcp);
        ASSERT(tmp->efcp->dt);

        dtp = dt_dtp(tmp->efcp->dt);
        if (!dtp) {
                LOG_ERR("No DTP instance available");
                sdu_destroy(tmp->sdu);
                write_data_destroy(tmp);
                return -1;
        }

        if (dtp_write(dtp, tmp->sdu)) {
                LOG_ERR("Could not write SDU to DTP");
                write_data_destroy(tmp);
                return -1;
        }

        write_data_destroy(tmp);
        return 0;
}

static int efcp_write(struct efcp * efcp,
                      struct sdu *  sdu)
{
        struct write_data *    tmp;
        struct rwq_work_item * item;

        if (!sdu) {
                LOG_ERR("Bogus SDU passed");
                return -1;
        }
        if (!efcp) {
                LOG_ERR("Bogus EFCP passed");
                sdu_destroy(sdu);
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
        struct efcp * tmp;

        if (!container || !sdu_is_ok(sdu)) {
                LOG_ERR("Bogus input parameters, cannot write into container");
                sdu_destroy(sdu);
                return -1;
        }
        if (!is_cep_id_ok(cep_id)) {
                LOG_ERR("Bad cep-id, cannot write into container");
                sdu_destroy(sdu);
                return -1;
        }

        tmp = efcp_imap_find(container->instances, cep_id);
        if (!tmp) {
                LOG_ERR("There is no EFCP bound to this cep-id %d", cep_id);
                sdu_destroy(sdu);
                return -1;
        }

        if (efcp_write(tmp, sdu))
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
        struct dtp *          dtp;
        struct dtcp *         dtcp;
        pdu_type_t            pdu_type;

        tmp = (struct receive_data *) o;
        if (!tmp) {
                LOG_ERR("No receive data passed");
                return -1;
        }

        if (!is_receive_data_ok(tmp)) {
                LOG_ERR("Wrong data passed to efcp_receive_worker");
                if (tmp->pdu)
                        pdu_destroy(tmp->pdu);

                receive_data_destroy(tmp);
                return -1;
        }

        ASSERT(tmp->efcp);
        ASSERT(tmp->efcp->dt);

        pdu_type = pci_type(pdu_pci_get_ro(tmp->pdu));
        LOG_ERR("PDU_TYPE: %d", pdu_type);
        if (pdu_type_is_control(pdu_type)) {
                LOG_ERR("PDU type is control");
                dtcp = dt_dtcp(tmp->efcp->dt);
                if (!dtcp) {
                        LOG_ERR("No DTCP instance available");
                        pdu_destroy(tmp->pdu);
                        receive_data_destroy(tmp);
                        return -1;
                }

                if (dtcp_common_rcv_control(dtcp, tmp->pdu)) {
                        receive_data_destroy(tmp);
                        return -1;
                }

                receive_data_destroy(tmp);
                return 0;
                /* FIXME: The PDU has been consumed ... */
        }

        /*
         * FIXME: Based on the previous FIXME, shouldn't the rest of this
         *        function be in the else case ?
         */
        dtp = dt_dtp(tmp->efcp->dt);
        if (!dtp) {
                LOG_ERR("No DTP instance available");
                pdu_destroy(tmp->pdu);
                receive_data_destroy(tmp);
                return -1;
        }

        if (dtp_receive(dtp, tmp->pdu)) {
                LOG_ERR("DTP cannot receive this PDU");
                receive_data_destroy(tmp);
                return -1;
        }

        receive_data_destroy(tmp);
        return 0;
}

static int efcp_receive(struct efcp * efcp,
                        struct pdu *  pdu)
{
        struct receive_data *  data;
        struct rwq_work_item * item;

        if (!pdu) {
                LOG_ERR("No pdu passed");
                return -1;
        }
        if (!efcp) {
                LOG_ERR("No efcp instance passed");
                pdu_destroy(pdu);
                return -1;
        }

        LOG_ERR("CREATING EFCP RECEIVE DATA");
        data = receive_data_create(efcp, pdu);
        ASSERT(is_receive_data_ok(data));

        /* Is this _ni() call really necessary ??? */
        item = rwq_work_create_ni(efcp_receive_worker, data);
        if (!item) {
                pdu_destroy(pdu);
                receive_data_destroy(data);
                return -1;
        }

        ASSERT(efcp->container->ingress_wq);

        if (rwq_work_post(efcp->container->ingress_wq, item)) {
                pdu_destroy(pdu);
                receive_data_destroy(data);
                return -1;
        }

        return 0;
}

int efcp_container_receive(struct efcp_container * container,
                           cep_id_t                cep_id,
                           struct pdu *            pdu)
{
        struct efcp * tmp;

        if (!container || !pdu_is_ok(pdu)) {
                LOG_ERR("Bogus input parameters");
                pdu_destroy(pdu);
                return -1;
        }
        if (!is_cep_id_ok(cep_id)) {
                LOG_ERR("Bad cep-id, cannot write into container");
                pdu_destroy(pdu);
                return -1;
        }

        tmp = efcp_imap_find(container->instances, cep_id);
        if (!tmp) {
                LOG_ERR("Cannot find the requested instance");
                pdu_destroy(pdu);
                return -1;
        }

        if (efcp_receive(tmp, pdu))
                return -1;

        return 0;
}
EXPORT_SYMBOL(efcp_container_receive);

static bool is_connection_ok(const struct connection * connection)
{
        /* FIXME: Add checks for policy params */

        if (!connection                                   ||
            !is_cep_id_ok(connection->source_cep_id)      ||
            !is_cep_id_ok(connection->destination_cep_id) ||
            !is_port_id_ok(connection->port_id))
                return false;

        return true;
}

cep_id_t efcp_connection_create(struct efcp_container * container,
                                struct connection *     connection)
{
        struct efcp * tmp;
        cep_id_t      cep_id;
        struct dtp *  dtp;
        struct dtcp * dtcp;
        struct cwq *  cwq;
        struct rtxq * rtxq;

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

        tmp->dt = dt_create();
        if (!tmp->dt) {
                efcp_destroy(tmp);
                return cep_id_bad();
        }

        ASSERT(tmp->dt);

        /* FIXME: Initialization of dt required */

        cep_id                    = cidm_allocate(container->cidm);

        /* We must ensure that the DTP is instantiated, at least ... */
        tmp->container            = container;

        /* Initial value to avoid problems in case of errors */
        connection->source_cep_id = cep_id_bad();

        /* FIXME: We change the connection cep-id and we return cep-id ... */
        connection->source_cep_id = cep_id;
        tmp->connection           = connection;

        /* FIXME: dtp_create() takes ownership of the connection parameter */
        dtp = dtp_create(tmp->dt,
                         container->rmt,
                         container->kfa,
                         connection);
        if (!dtp) {
                efcp_destroy(tmp);
                return cep_id_bad();
        }

        ASSERT(dtp);

        if (dt_dtp_bind(tmp->dt, dtp)) {
                dtp_destroy(dtp);
                efcp_destroy(tmp);
                return cep_id_bad();
        }

        dtcp = NULL;

#if DTCP_TEST_ENABLE
        connection->policies_params.dtcp_present = true;
        connection->policies_params.flow_ctrl = true;
        connection->policies_params.rate_based_fctrl = false;
        connection->policies_params.rtx_ctrl = false;
        connection->policies_params.window_based_fctrl = true;
#endif

        if (connection->policies_params.dtcp_present) {
                dtcp = dtcp_create(tmp->dt, connection, container->rmt);
                if (!dtcp) {
                        efcp_destroy(tmp);
                        return cep_id_bad();
                }

                if (dt_dtcp_bind(tmp->dt, dtcp)) {
                        dtcp_destroy(dtcp);
                        efcp_destroy(tmp);
                        return cep_id_bad();
                }
        }

        if (connection->policies_params.window_based_fctrl) {
                cwq = cwq_create();
                if (!cwq) {
                        LOG_ERR("Failed to create closed window queue");
                        efcp_destroy(tmp);
                        return cep_id_bad();
                }
                if (dt_cwq_bind(tmp->dt, cwq)) {
                        cwq_destroy(cwq);
                        efcp_destroy(tmp);
                        return cep_id_bad();
                }
        }

        if (connection->policies_params.rtx_ctrl) {
                rtxq = rtxq_create(tmp->dt);
                if (!rtxq) {
                        LOG_ERR("Failed to create rexmsn queue");
                        efcp_destroy(tmp);
                        return cep_id_bad();
                }
                if (dt_rtxq_bind(tmp->dt, rtxq)) {
                        rtxq_destroy(rtxq);
                        efcp_destroy(tmp);
                        return cep_id_bad();
                }
        }

        if (efcp_imap_add(container->instances,
                          connection->source_cep_id,
                          tmp)) {
                LOG_ERR("Cannot add a new instance into container %pK",
                        container);

                rkfree(connection);

                if (dtp)  dtp_destroy(dtp);
                if (dtcp) dtcp_destroy(dtcp);

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
