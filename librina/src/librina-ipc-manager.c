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

#include "librina-ipc-manager.h"

#define RINA_PREFIX "ipc-manager"

#include "logs.h"

/*
 * Description
 *
 * Invoked by the IPC Manager to instantiate a new IPC Process in the system.
 * The operation will block until the IPC Process is created or an error
 * returned.
 *
 * Inputs
 *
 * name: The IPC Process naming information (required)
 * type: The IPC Process type (required)
 * ipc_process_id: A unique identifier for the IPC Process in the system
 */
int ipcm_create(const name_t * name,
				dif_type_t type,
				ipc_process_id_t ipc_process_id)
{
	LOG_DBG("Create IPC Process called");
	LOG_DBG("IPC Process id: %d", ipc_process_id);
	LOG_DBG("IPC Process AP name: %s; AP instance: %s", name->process_name, name->process_instance);
	LOG_DBG("IPC Process type: %c", type);
}

/*
 * Description
 *
 * Invoked by the IPC Manager to delete an IPC Process from the system. The
 * operation will block until the IPC Process is deleted or an error returned.
 *
 * Inputs
 *
 * ipc_process_id: A unique identifier for the IPC Process in the system
 */
int ipcm_destroy(ipc_process_id_t ipc_process_id)
{
	LOG_DBG("Destroy IPC Process called");
	LOG_DBG("IPC Process id: %d", ipc_process_id);
}

/*
 * Description
 *
 * Invoked by the IPC Manager to make an IPC Process in the system member of a
 * DIF. Preconditions: the IPC Process must exist and must not be already a
 * member of a DIF. Assigning an IPC Process to a DIF will initialize
 * the IPC Process with all the information required to operate in the DIF
 * (DIF name, data transfer constants, qos cubes, supported policies, address,
 * credentials, etc). The operation will block until the IPC Process is
 * assigned to the DIF or an error is returned.
 *
 * Inputs
 *
 * ipc_process_id: The id of the IPC Process affected by the operation
 * (required).
 * dif_configuration: A data structure containing all the information to initialize
 * the IPC Process as a member of the DIF (DIF name, data transfer constants,
 * qos cubes, supported policies, address, credentials, etc). It may also specify
 * one or more N-1 DIFs to register at, and one or more ÒneighborÓ IPC Processes to
 * initiate enrollment with (required).
 */
int ipcm_assign(ipc_process_id_t ipc_process_id,
				const dif_configuration_t * dif_configuration)
{
	LOG_DBG("Assign IPC Process to DIF called");
	LOG_DBG("IPC Process id: %d", ipc_process_id);
	LOG_DBG("IPC Process Configuration.");
	LOG_DBG("	DIF name: %s", dif_configuration->dif_name);
	LOG_DBG("	DIF type: %d", dif_configuration->dif_type);
	LOG_DBG("	Max SDU size: %d", dif_configuration->max_sdu_size);
	if (dif_configuration->cubes.elements != NULL &&
			dif_configuration->cubes.size > 0)
	{
		int i;
		for(i=0; i<dif_configuration->cubes.size; i++)
		{
			LOG_DBG("	QoS cube name: %s", dif_configuration->cubes.elements[i].name);
			LOG_DBG("		Id: %d", dif_configuration->cubes.elements[i].id);
			LOG_DBG("		Delay: %d", dif_configuration->cubes.elements[i].flow_spec.delay);
			LOG_DBG("		Jitter: %d", dif_configuration->cubes.elements[i].flow_spec.jitter);
		}
	}

	switch(dif_configuration->dif_type)
	{
	case DIF_TYPE_NORMAL:
		LOG_DBG("Normal DIFs not yet supported");
		return -1;
	case DIF_TYPE_SHIM_IP:
		LOG_DBG("Shim DIFs over TCP/UDP not yet supported");
		return -1;
	case DIF_TYPE_SHIM_ETH:
		LOG_DBG("	Device name: %s", dif_configuration->specific_conf.shim_eth_dif_config.device_name);
		break;
	}
}
