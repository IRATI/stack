/*
 * EFCP (Error and Flow Control Protocol)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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
#include "efcp.h"
#include "debug.h"

struct efcp {
        /* The connection endpoint id that identifies this instance */
        cep_id_t      id;

        /* The Data transfer protocol state machine instance */
        struct dtp *  dtp;

        /* The Data transfer control protocol state machine instance */
        struct dtcp * dtcp;

        /* Pointer to the flow data structure of the K-IPC Manager */
        /* FIXME: Do we really need this pointer ? */
        struct flow * flow;
};

struct efcp * efcp_init(struct kobject * parent)
{
        struct efcp * e = NULL;

        LOG_DBG("Initializing instance");

        e = rkzalloc(sizeof(*e), GFP_KERNEL);

        return e;
}

int efcp_fini(struct efcp * instance)
{
        ASSERT(instance);

        LOG_DBG("Finalizing instance %pK", instance);

        rkfree(instance);

        return 0;
}

int efcp_write(struct efcp *      instance,
               port_id_t          id,
               const struct sdu * sdu)
{
        ASSERT(instance);

        return dtp_send(instance->dtp, sdu);
}

int efcp_receive_pdu(struct efcp * instance,
                     struct pdu *  pdu)
{
        ASSERT(instance);

        LOG_MISSING;

        LOG_DBG("PDU received in the EFCP");

        return 0;
}

int efcp_create(struct efcp *             instance,
                const struct connection * connection,
                cep_id_t *                id)
{
        ASSERT(instance);
        ASSERT(connection);

        instance->dtp  = dtp_create(connection->port_id);
        if (!instance->dtp)
                return -1;

        instance->dtcp = dtcp_create();
        if (!instance->dtcp) {
                if (dtp_destroy(instance->dtp))
                        return -1;
                instance->dtp = 0;
                return -1;
        }

        /* No needs to check here, bindings are straightforward */
        dtp_state_vector_bind(instance->dtp,   instance->dtcp->state_vector);
        dtcp_state_vector_bind(instance->dtcp, instance->dtp->state_vector);

        /* FIXME: We need to assign the id */
        LOG_MISSING;

        LOG_DBG("EFCP instance created");

        return 0;
}

int efcp_destroy(struct efcp * instance,
                 cep_id_t      id)
{
        ASSERT(instance);

        dtp_state_vector_unbind(instance->dtp);
        dtcp_state_vector_unbind(instance->dtcp);

        if (dtp_destroy(instance->dtp))
                return -1;
        instance->dtp = 0;

        if (dtcp_destroy(instance->dtcp))
                return -1;
        instance->dtcp = 0;

        LOG_DBG("EFCP instance destroyed");

        return 0;
}

int efcp_update(struct efcp * instance,
                cep_id_t      from,
                cep_id_t      to)
{
        ASSERT(instance);

        LOG_UNSUPPORTED;

        return -1;
}
