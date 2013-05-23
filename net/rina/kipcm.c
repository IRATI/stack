/*
 *  KIPCM (Kernel-IPC Manager)
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

#define RINA_PREFIX "kipcm"

#include "logs.h"
#include "kipcm.h"

int kipcm_init()
{
        LOG_DBG("init");

        return 0;
}

void kipcm_exit()
{
        LOG_DBG("exit");
}

int kipcm_add_entry(port_id_t port_id, const flow_t *flow)
{
	LOG_DBG("adding entry to the port_id_to_flow table");

	return 0;
}

int kipcm_remove_entry(port_id_t port_id)
{
	LOG_DBG("removing entry to the port_id_to_flow table");

	return 0;
}

int kipcm_post_sdu(port_id_t port_id, const sdu_t *sdu)
{
	LOG_DBG("removing entry to the port_id_to_flow table");

	return 0;
}

SYSCALL_DEFINE3(ipc_process_create, const name_t *name, ipc_process_id_t ipcp_id, dif_type_t type)
{
	LOG_DBG("syscall ipc_process_create");
	
	return 0;
}