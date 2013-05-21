/*
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

#define RINA_PREFIX "shim-eth"

#include <linux/if_ether.h>
#include "logs.h"
#include "shim-eth.h"

ipc_process_id_t shim_eth_create(const name_t *name, const ipc_config_t **config)
{

}

int shim_eth_destroy(ipc_process_id_t ipc_process_id)
{

}

port_id_t shim_eth_allocate_flow_request(const name_t *source, const name_t *dest, const flow_spec_t *flow_spec)
{

}

nt shim_eth_allocate_flow_response(const port_id_t *port_id, const response_reason_t *response)
{

}

int shim_eth_deallocate_flow(port_id_t port_id)
{

}

int shim_eth_register_application(const name_t *name)
{

}

int shim_eth_unregister_application(const name_t *name)
{

}

int shim_eth_write_sdu(port_id_t port_id, sdu_t *sdu)
{

}

int shim_eth_init(void)
{
        LOG_DBG("init");

        return 0;
}

void shim_eth_exit(void)
{
        LOG_DBG("exit");
}


