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

#include <linux/slab.h>
#include <linux/kobject.h>

#define RINA_PREFIX "efcp"

#include "logs.h"
#include "utils.h"
#include "efcp.h"

struct efcp_descriptor {
        int this_is_dummy;
};

void * efcp_init(struct kobject * parent)
{
        struct efcp_descriptor * e = NULL;

        LOG_FBEGN;

        LOG_DBG("Initializing instance");

        e = kmalloc(sizeof(*e), GFP_KERNEL);
        if (!e) {
                LOG_CRIT("Cannot allocate %zu bytes of memory",
                         sizeof(*e));

                LOG_FEXIT;
                return e;
        }

        LOG_FEXIT;

        return e;
}

int efcp_fini(void * opaque)
{
        LOG_FBEGN;

        LOG_DBG("Finalizing instance %pK", opaque);

        ASSERT(opaque);
        kfree(opaque);

        LOG_FEXIT;

        return 0;
}

int efcp_write(void *               opaque,
               port_id_t            id,
               const struct sdu_t * sdu)
{
        LOG_DBG("Written SDU");

        return 0;
}

int efcp_receive_pdu(void *       opaque,
                     struct pdu_t pdu)
{
        LOG_DBG("PDU received in the EFCP");

        return 0;
}

cep_id_t efcp_create(void *                      opaque,
                     const struct connection_t * connection)
{
        LOG_DBG("EFCP instance created");

        return 0;
}

int efcp_destroy(void *   opaque,
                 cep_id_t id)
{
        LOG_DBG("EFCP instance destroyed");

        return 0;
}

int efcp_update(void *   opaque,
                cep_id_t id_from,
                cep_id_t id_to)
{
        LOG_DBG("EFCP instance updated");

        return 0;
}
