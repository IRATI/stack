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
        struct connection * connection;
        struct dtp *        dtp;
        struct dtcp *       dtcp;
};

static struct efcp * efcp_create(void)
{
        struct efcp * instance;

        instance = rkzalloc(sizeof(*instance), GFP_KERNEL);
        if (!instance) {
                LOG_ERR("Cannot create a new instance");
                return NULL;
        }

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
        struct efcp_imap *              instances;
        struct cidm *                   cidm;
        struct data_transfer_constants  dt_cons;
        struct rmt *                    rmt;
        struct kfa *                    kfa;
};

// efcp_imap maps cep_id_t to efcp_instances


struct efcp_container * efcp_container_create(struct kfa * kfa)
{
        struct efcp_container * container;

        if (!kfa) {
                LOG_ERR("Bogus KFA instances passed, bailing out");
                return NULL;
        }

        container = rkzalloc(sizeof(*container), GFP_KERNEL);
        if (!container) {
                LOG_ERR("Failed to create EFCP container instance");
                return NULL;
        }

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

        if (container->instances) 
                efcp_imap_destroy(container->instances, efcp_destroy);
        if (container->cidm) cidm_destroy(container->cidm);
        rkfree(container);

        return 0;
}
EXPORT_SYMBOL(efcp_container_destroy);

int efcp_container_set_dt_cons(struct data_transfer_constants * dt_cons,
                               struct efcp_container          * container)
{
        container->dt_cons.address_length = dt_cons->address_length;
        container->dt_cons.cep_id_length  = dt_cons->cep_id_length;
        container->dt_cons.length_length  = dt_cons->length_length;
        container->dt_cons.port_id_length = dt_cons->port_id_length;
        container->dt_cons.qos_id_length  = dt_cons->qos_id_length;
        container->dt_cons.seq_num_length = dt_cons->seq_num_length;
        container->dt_cons.max_pdu_size   = dt_cons->max_pdu_size;
        container->dt_cons.max_pdu_life   = dt_cons->max_pdu_life;
        container->dt_cons.dif_integrity  = dt_cons->dif_integrity;

        LOG_DBG("Succesfully set data transfer constants to efcp container");
        return 0;
}
EXPORT_SYMBOL(efcp_container_set_dt_cons);

int efcp_container_write(struct efcp_container * container,
                         cep_id_t                cep_id,
                         struct sdu *            sdu)
{
        struct efcp *       efcp;

        efcp = efcp_imap_find(container->instances, cep_id);
        if (!efcp) {
                LOG_ERR("There is no EFCP bound to this cep_id %d", cep_id);
                return -1;
        }
        if (efcp_send(efcp, sdu))
                return -1;

        return 0;
}
EXPORT_SYMBOL(efcp_container_write);

int efcp_container_receive(struct efcp_container * container,
                           cep_id_t                cep_id,
                           struct sdu *            sdu)
{
        struct efcp * tmp;
        LOG_MISSING;

        tmp = efcp_find(container, cep_id);
        if (!tmp)
                return -1;

        dtp_receive(tmp->dtp, NULL);

        return 0;
}
EXPORT_SYMBOL(efcp_container_receive);

static int is_connection_ok(const struct connection * connection)
{
        if (!connection                                   ||
            !is_cep_id_ok(connection->source_cep_id)      ||
            !is_cep_id_ok(connection->destination_cep_id) ||
            !is_port_id_ok(connection->port_id))
                return 0;

        return 1;
}

cep_id_t efcp_connection_create(struct efcp_container *   container,
                                struct connection * connection)
{
        struct efcp * tmp;
        cep_id_t      cep_id;

        if (!container) {
                LOG_ERR("Bogus container passed, bailing out");
                return -1;
        }
        if (!is_connection_ok(connection)) {
                LOG_ERR("Bogus connection passed, bailing out");
                return -1;
        }
        ASSERT(connection);

        tmp = efcp_create();
        if (!tmp)
                return cep_id_bad();

        cep_id = cidm_allocate(container->cidm);
        /* We must ensure that the DTP is instantiated, at least ... */
        connection->source_cep_id = cep_id;
        tmp->connection = connection;
        tmp->dtp        = dtp_create(container->rmt, container->kfa);
        if (!tmp->dtp) {
                efcp_destroy(tmp);
                return cep_id_bad();
        }

        /* FIXME: We need to know if DTCP is needed ...
           tmp->dtcp = dtcp_create();
           if (!tmp->dtcp) {
           efcp_destroy(tmp);
           return -1;
           }
        */

        /* No needs to check here, bindings are straightforward */
        dtp_bind(tmp->dtp,   tmp->dtcp);
        /* dtcp_bind(tmp->dtcp, tmp->dtp); */

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

        LOG_DBG("Connection created \n "
                "Source address: %d \n"
                "Destination address %d \n"
                "Destination cep id: %d \n"
                "Source cep id: %d \n",
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

        tmp = efcp_imap_find(container->instances, from);
        if (!tmp) {
                LOG_ERR("Cannot get instance %d from container %pK",
                        from, container);
                return -1;
        }
        tmp->connection->destination_cep_id = to;

        LOG_DBG("Connection updated \n ");
        LOG_DBG("Source address: %d \n", tmp->connection->source_address);
        LOG_DBG("Destination address %d \n",
                        tmp->connection->destination_address);
        LOG_DBG("Destination cep id: %d \n",
                        tmp->connection->destination_cep_id);
        LOG_DBG("Source cep id: %d \n", tmp->connection->source_cep_id);

        return 0;
}
EXPORT_SYMBOL(efcp_connection_update);

struct efcp * efcp_find(struct efcp_container * container,
                        cep_id_t                id)
{
        struct efcp * tmp;

        if (!container) {
                LOG_ERR("Bogus container passed, bailing out");
                return NULL;
        }

        tmp = efcp_imap_find(container->instances, id);

        return tmp;
}

int efcp_send(struct efcp * instance,
              struct sdu *  sdu)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        if (!is_sdu_ok(sdu)) {
                LOG_ERR("Bogus SDU passed");
                return -1;
        }

        if (!instance->dtp) {
                LOG_ERR("No DTP instance available, cannot send");
                return -1;
        }

        return dtp_send(instance->dtp, sdu);
}
