/*
 *  EFCP (Error and Flow Control Protocol)
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

#define RINA_PREFIX "efcp"

#include "logs.h"
#include "efcp.h"

int efcp_init(void)
{
        LOG_FBEGN;

        LOG_FEXIT;

        return 0;
}

void efcp_exit(void)
{
        LOG_FBEGN;

        LOG_FEXIT;
}

/* Internal APIs */

int efcp_write(port_id_t    port_id,
		       const struct sdu_t *sdu)
{
	LOG_DBG("Written SDU");

	return 0;
}

int efcp_receive_pdu(struct pdu_t pdu)
{
	LOG_DBG("PDU received in the EFCP");

	return 0;
}

/* Syscalls */

cep_id_t efcp_create(struct connection_t *connection)
{
	LOG_DBG("EFCP instance created");

	return 0;
}

int efcp_destroy(cep_id_t cep_id)
{
	LOG_DBG("EFCP instance destroyed");

	return 0;
}

int efcp_update(cep_id_t cep_id,
		    	cep_id_t dest_cep_id)
{
	LOG_DBG("EFCP instance updated");

	return 0;
}
