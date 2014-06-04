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
#include "ipcp-utils.h"
#include "cidm.h"
#include "dt.h"
#include "dtp.h"
#include "dtcp.h"
#include "dtcp-utils.h"
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
        struct efcp_imap *   instances;
        struct cidm *        cidm;
        struct efcp_config * config;
        struct rmt *         rmt;
        struct kfa *         kfa;
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

                connection_destroy(instance->connection);
        }

        rkfree(instance);

        LOG_DBG("Instance %pK finalized successfully", instance);

        return 0;
}

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

        if (container->config)     efcp_config_destroy(container->config);
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

int efcp_container_set_config(struct efcp_config *    efcp_config,
                              struct efcp_container * container)
{
        if (!efcp_config || !container) {
                LOG_ERR("Bogus input parameters, bailing out");
                return -1;
        }

        container->config = efcp_config;

        LOG_DBG("Succesfully set EFCP config to EFCP container");

        return 0;
}
EXPORT_SYMBOL(efcp_container_set_config);

static int efcp_write(struct efcp * efcp,
                      struct sdu *  sdu)
{
        struct dtp *        dtp;

        if (!sdu) {
                LOG_ERR("Bogus SDU passed");
                return -1;
        }
        if (!efcp) {
                LOG_ERR("Bogus EFCP passed");
                sdu_destroy(sdu);
                return -1;
        }

        ASSERT(efcp->dt);

        dtp = dt_dtp(efcp->dt);
        if (!dtp) {
                LOG_ERR("No DTP instance available");
                sdu_destroy(sdu);
                return -1;
        }

        if (dtp_write(dtp, sdu)) {
                LOG_ERR("Could not write SDU to DTP");
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

static int efcp_receive(struct efcp * efcp,
                        struct pdu *  pdu)
{
        struct dtp *          dtp;
        struct dtcp *         dtcp;
        pdu_type_t            pdu_type;

        if (!pdu) {
                LOG_ERR("No pdu passed");
                return -1;
        }
        if (!efcp) {
                LOG_ERR("No efcp instance passed");
                pdu_destroy(pdu);
                return -1;
        }

        ASSERT(efcp->dt);

        pdu_type = pci_type(pdu_pci_get_ro(pdu));
        if (pdu_type_is_control(pdu_type)) {
                dtcp = dt_dtcp(efcp->dt);
                if (!dtcp) {
                        LOG_ERR("No DTCP instance available");
                        pdu_destroy(pdu);
                        return -1;
                }

                if (dtcp_common_rcv_control(dtcp, pdu))
                        return -1;

                return 0;
        }

        dtp = dt_dtp(efcp->dt);
        if (!dtp) {
                LOG_ERR("No DTP instance available");
                pdu_destroy(pdu);
                return -1;
        }

        if (dtp_receive(dtp, pdu)) {
                LOG_ERR("DTP cannot receive this PDU");
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
                /* FIXME: It should call unknown_flow policy of EFCP */
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

#if DTCP_TEST_ENABLE
        connection->policies_params->dtcp_present = true;
        if (!connection->policies_params->dtcp_cfg) {
                LOG_ERR("DTCP config was not there for DTCP_TEST_ENABLE");
                efcp_destroy(tmp);
                return cep_id_bad();
        }
        dtcp_max_closed_winq_length_set(connection->policies_params->dtcp_cfg,
                                        20);
        dtcp_flow_ctrl_set(connection->policies_params->dtcp_cfg, true);
        dtcp_rtx_ctrl_set(connection->policies_params->dtcp_cfg, false);
        dtcp_window_based_fctrl_set(connection->policies_params->dtcp_cfg,
                                    true);
        dtcp_rate_based_fctrl_set(connection->policies_params->dtcp_cfg,
                                  false);
#endif

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

        if (connection->policies_params->dtcp_present) {
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

        if (dtcp_window_based_fctrl(connection->policies_params->dtcp_cfg)) {
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

        if (dtcp_rtx_ctrl(connection->policies_params->dtcp_cfg)) {
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

        LOG_DBG("EFCP connection destroy called");

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
