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
#include "debug.h"
#include "efcp.h"
#include "dtp.h"
#include "dtcp.h"

#if 0
struct efcp_conf {

        /* Length of the address fields of the PCI */
        int address_length;

        /* Length of the port_id fields of the PCI */
        int port_id_length;

        /* Length of the cep_id fields of the PCI */
        int cep_id_length;

        /* Length of the qos_id field of the PCI */
        int qos_id_length;

        /* Length of the length field of the PCI */
        int length_length;

        /* Length of the sequence number fields of the PCI */
        int seq_number_length;
};
#endif

struct efcp {
        /* The connection endpoint id that identifies this instance */
        cep_id_t      id;

        /* The Data transfer protocol state machine instance */
        struct dtp *  dtp;

        /* The Data transfer control protocol state machine instance */
        struct dtcp * dtcp;

#if 0
        /* Pointer to the flow data structure of the K-IPC Manager */
        /* FIXME: Do we really need this pointer ? */
        struct flow * flow;
#endif
};

struct efcp * efcp_create(void)
{
        struct efcp * e = NULL;

        LOG_DBG("Initializing instance");

        e = rkzalloc(sizeof(*e), GFP_KERNEL);

        return e;
}

int efcp_destroy(struct efcp * instance)
{
        ASSERT(instance);

        LOG_DBG("Finalizing instance %pK", instance);

        rkfree(instance);

        return 0;
}

int efcp_connection_create(struct efcp *             instance,
                           const struct connection * connection,
                           cep_id_t *                id)
{
        ASSERT(instance);
        ASSERT(connection);

        instance->dtp  = dtp_create(/* connection->port_id */);
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
        dtp_bind(instance->dtp,   instance->dtcp);
        dtcp_bind(instance->dtcp, instance->dtp);

        /* FIXME: We need to assign the id */
        LOG_MISSING;

        LOG_DBG("EFCP instance created");

        return 0;
}

int efcp_connection_destroy(struct efcp * instance,
                            cep_id_t      id)
{
        ASSERT(instance);

        dtp_unbind(instance->dtp);
        dtcp_unbind(instance->dtcp);

        if (dtp_destroy(instance->dtp))
                return -1;
        instance->dtp = 0;

        if (dtcp_destroy(instance->dtcp))
                return -1;
        instance->dtcp = 0;

        LOG_DBG("EFCP instance destroyed");

        return 0;
}

int efcp_connection_update(struct efcp * instance,
                           cep_id_t      from,
                           cep_id_t      to)
{
        ASSERT(instance);

        LOG_MISSING;

        return -1;
}

int efcp_write(struct efcp *      instance,
               port_id_t          id,
               const struct sdu * sdu)
{
        ASSERT(instance);
        ASSERT(is_port_id_ok(id));
        ASSERT(sdu);

        if (!instance->dtp) {
                LOG_ERR("No DTP instance available, cannot send");
                return -1;
        }

        return dtp_send(instance->dtp, sdu);
}

struct pdu * efcp_receive_pdu(struct efcp * instance)
{
        ASSERT(instance);

        LOG_MISSING;

        return NULL;
}
