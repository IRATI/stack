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

using namespace rina;

/*#include "logs.h"

int ipcm_create(const name_t * name,
                dif_type_t type,
                ipc_process_id_t ipc_process_id)
{
	LOG_DBG("Create IPC Process called");
	LOG_DBG("IPC Process id: %d", ipc_process_id);
	LOG_DBG("IPC Process AP name: %s; AP instance: %s", name->process_name, name->process_instance);
	LOG_DBG("IPC Process type: %c", type);

        return 0;
}

int ipcm_destroy(ipc_process_id_t ipc_process_id)
{
	LOG_DBG("Destroy IPC Process called");
	LOG_DBG("IPC Process id: %d", ipc_process_id);

        return 0;
}

int ipcm_assign(ipc_process_id_t ipc_process_id,
                const dif_configuration_t * dif_configuration)
{
	LOG_DBG("Assign IPC Process to DIF called");
	LOG_DBG("IPC Process id: %d", ipc_process_id);
	LOG_DBG("IPC Process Configuration.");

        // FIXME: Compilation error ahead
#if 0
	LOG_DBG("	DIF name: %s", dif_configuration->dif_name);
#endif

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

        return 0;
}

int ipcm_notify_register(ipc_process_id_t ipc_process_id,
                         name_t * dif_name)
{
	LOG_DBG("Notify IPC Process registration to a DIF called");
	LOG_DBG("IPC Process id: %d", ipc_process_id);
	LOG_DBG("DIF name: %s", dif_name->process_name);

        return 0;
}

int ipcm_notify_unregister(ipc_process_id_t ipc_process_id,
                           name_t * dif_name)
{
	LOG_DBG("Notify IPC Process unregistration from a DIF called");
	LOG_DBG("IPC Process id: %d", ipc_process_id);
	LOG_DBG("DIF name: %s", dif_name->process_name);

        return 0;
}

int ipcm_enroll(ipc_process_id_t ipc_process_id,
                const name_t * dif_name,
                const name_t * nm1_dif)
{
	LOG_DBG("Make an IPC Process enroll to a DIF called");
	LOG_DBG("IPC Process id: %d", ipc_process_id);
	LOG_DBG("DIF to join: %s", dif_name->process_name);
	LOG_DBG("Supporting DIF: %s", nm1_dif->process_name);

        return 0;
}

int ipcm_disconnect(ipc_process_id_t ipc_process_id,
                    const name_t * neighbor)
{
	LOG_DBG("Make an IPC Process disconnect form a neighbor called");
	LOG_DBG("IPC Process id: %d", ipc_process_id);
	LOG_DBG("Neighbor to disconnect from: AP name: %s; AP instance: %s",
                neighbor->process_name, neighbor->process_instance);

        return 0;
}

int ipcm_register_app(name_t * app_name,
                      ipc_process_id_t ipc_process_id)
{
	LOG_DBG("Register application to DIF called");
	LOG_DBG("Application to be registered:");
	LOG_DBG("	Process name: %s", app_name->process_name);
	LOG_DBG("	Process instance: %s", app_name->process_instance);
	LOG_DBG("	Entity name: %s", app_name->entity_name);
	LOG_DBG("	Entity instance: %s", app_name->entity_instance);
	LOG_DBG("ID of IPC Process to register to: %d", ipc_process_id);

        return 0;
}

int ipcm_notify_app_registration(name_t * app_name,
                                 response_reason_t response_reason)
{
	LOG_DBG("Notify application about registration to DIF called");
	LOG_DBG("Application that requested the registration:");
	LOG_DBG("	Process name: %s", app_name->process_name);
	LOG_DBG("	Process instance: %s", app_name->process_instance);
	LOG_DBG("	Entity name: %s", app_name->entity_name);
	LOG_DBG("	Entity instance: %s", app_name->entity_instance);
	LOG_DBG("Result of the operation: %s", response_reason);

        return 0;
}

int ipcm_unregister_app(name_t * app_name,
                        ipc_process_id_t ipc_process_id)
{
	LOG_DBG("Unregister application from DIF called");
	LOG_DBG("Application to be unregistered:");
	LOG_DBG("	Process name: %s", app_name->process_name);
	LOG_DBG("	Process instance: %s", app_name->process_instance);
	LOG_DBG("	Entity name: %s", app_name->entity_name);
	LOG_DBG("	Entity instance: %s", app_name->entity_instance);
	LOG_DBG("ID of IPC Process to unregister from: %d", ipc_process_id);

        return 0;
}

int ipcm_notify_app_unregistration(name_t * app_name,
                                   response_reason_t response_reason)
{
	LOG_DBG("Notify application about unregistration from DIF called");
	LOG_DBG("Application that requested the unregistration:");
	LOG_DBG("	Process name: %s", app_name->process_name);
	LOG_DBG("	Process instance: %s", app_name->process_instance);
	LOG_DBG("	Entity name: %s", app_name->entity_name);
	LOG_DBG("	Entity instance: %s", app_name->entity_instance);
	LOG_DBG("Result of the operation: %s", response_reason);

        return 0;
}

int ipcm_allocate_flow(name_t * source,
                       name_t * destination,
                       flow_spec_t* flow_spec,
                       port_id_t port_id,
                       ipc_process_id_t ipc_process_id)
{
	LOG_DBG("Allocate flow called");
	LOG_DBG("Source application process name: %s", source->process_name);
	LOG_DBG("Source application process instance: %s", source->process_instance);
	LOG_DBG("Source application entity name: %s", source->entity_name);
	LOG_DBG("Source application entity instance: %s", source->entity_instance);
	LOG_DBG("Destination application process name: %s", destination->process_name);
	LOG_DBG("Destination application process instance: %s", destination->process_instance);
	LOG_DBG("Destination application entity name: %s", destination->entity_name);
	LOG_DBG("Destination application entity instance: %s", destination->entity_instance);
	LOG_DBG("Requested flow characteristics");
	LOG_DBG("	Average bandwidth. Min: %d; Max: %d", flow_spec->average_bandwidth.min_value,
                flow_spec->average_bandwidth.max_value);
	LOG_DBG("	Undetected bit error rate: %f", flow_spec->undetected_bit_error_rate);
	LOG_DBG("	Delay: %d; Jitter: %d", flow_spec->delay, flow_spec->jitter);
	LOG_DBG("	Ordered delivery: %u; Partial delivery: %u", flow_spec->ordered_delivery,
                flow_spec->partial_delivery);
	LOG_DBG("Port-id assigned to the flow: %d", port_id);
	LOG_DBG("Id of the IPC Process that has to allocate the flow: %d", ipc_process_id);

        return 0;
}

int ipcm_notify_flow_allocation(name_t * app_name,
                                port_id_t  port_id,
                                ipc_process_id_t ipc_process_id,
                                response_reason_t response_reason)
{
	LOG_DBG("Notify application about flow allocation result called");
	LOG_DBG("Application that requested the flow allocation:");
	LOG_DBG("	Process name: %s", app_name->process_name);
	LOG_DBG("	Process instance: %s", app_name->process_instance);
	LOG_DBG("	Entity name: %s", app_name->entity_name);
	LOG_DBG("	Entity instance: %s", app_name->entity_instance);
	LOG_DBG("Result of the operation: %s", response_reason);
	LOG_DBG("Port-id assigned to the flow: %d", port_id);
	LOG_DBG("Id of the IPC Process that has allocated the flow: %d", ipc_process_id);

        return 0;
}

rib_object_t *ipcm_query_rib(ipc_process_id_t ipc_process_id,
                             string_t object_class,
                             string_t object_name,
                             int scope,
                             regex_t filter,
                             int * number_of_objects)
{
	LOG_DBG("Query IPC Process RIB called");
	LOG_DBG("ID of the IPC Process to query: %d", ipc_process_id);
	LOG_DBG("Root object to query: ");
	LOG_DBG("	Class: %s", object_class);
	LOG_DBG("	Name: %s", object_name);
	LOG_DBG("Scope: %d", scope);

	if (filter != NULL) {
		LOG_DBG("Filter: %s", filter);
	}

	rib_object_t * rib_objects;
	rib_objects = (rib_object_t *) malloc(2*sizeof(rib_object_t));
	*number_of_objects = 2;

        // FIXME: Compilation errors ahead
#if 0
	rib_objects[0].object_class = "myClass";
	rib_objects[0].object_name = "/dif/datatransfer/test";
	rib_objects[0].object_instance = 398493843;
	rib_objects[0].value_type = STRING;
	rib_objects[0].object_value.string_value = "Test value";
	rib_objects[1].object_class = "myClass2";
	rib_objects[1].object_name = "/dif/management/test";
	rib_objects[1].object_instance = 9886232;
	rib_objects[1].value_type = INT;
	rib_objects[1].object_value.int_value = 8432;

	return rib_objects;
}*/
