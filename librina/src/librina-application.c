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

#include "librina-application.h"

#define RINA_PREFIX "application"

#include "logs.h"

int ev_poll(event_t * event)
{
	LOG_DBG("Event poll called");
	event->type = EVENT_ALLOCATE_FLOW_REQUEST_RECEIVED;
	return 0;
}

int ev_wait(event_t * event)
{
	LOG_DBG("Event wait called");
	event->type = EVENT_SDU_RECEIVED;

	return 0;
}

void ev_set_filter(event_filter_t filter)
{
	LOG_DBG("Event set filter called");
}

event_filter_t ev_get_filter(void)
{
	LOG_DBG("Event get filter called");

	event_filter_t *installed_filter;
	installed_filter = (event_filter_t *) malloc (sizeof (event_filter_t));
	return installed_filter;
}

port_id_t allocate_flow_request(const name_t * source,
                                const name_t * destination,
                                const flow_spec_t * flow_spec)
{
	LOG_DBG("Allocate flow request called");
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
	port_id_t port_id = 25;
	return port_id;
}

int allocate_flow_response(port_id_t port_id,
                           const response_reason_t response)
{
	LOG_DBG("Allocate flow response called");
	LOG_DBG("Port id: %d", port_id);
	LOG_DBG("Response: %s", response);
	return 0;
}

int deallocate_flow (port_id_t port_id)
{
	LOG_DBG("Deallocate flow called");
	LOG_DBG("Port id: %d", port_id);
	return 0;
}

int write_sdu(port_id_t port_id, sdu_t * sdu)
{
	LOG_DBG("Write SDU called");
	LOG_DBG("Port id: %d", port_id);
	LOG_DBG("Writing %d bytes ", sdu->size);
	int i;
	for(i=0; i<sdu->size; i++){
		LOG_DBG("Value of byte %d: %u", i, sdu->data[i]);
	}
	return 0;
}

dif_properties_t *get_dif_properties(const name_t * dif_name,
                       int * size)
{
	LOG_DBG("Get DIF properties called");
	*size = 2;
	dif_properties_t * dif_properties;
	dif_properties = (dif_properties_t *) malloc(2*sizeof(dif_properties_t));

	dif_properties[0].max_sdu_size=300;
	dif_properties[0].dif_name.process_name = "Test.DIF";
	qos_cube_t * qos_cube;
	qos_cube = (qos_cube_t *) malloc(2*sizeof(qos_cube_t));
	qos_cube[0].name = "qos cube number 1";
	qos_cube[0].id = 1;
	qos_cube[0].flow_spec.jitter = 25;
	qos_cube[0].flow_spec.delay = 300;
	qos_cube[1].name = "qos cube number 2";
	qos_cube[1].id = 2;
	qos_cube[1].flow_spec.jitter = 100;
	qos_cube[1].flow_spec.delay = 200;
	dif_properties[0].qos_cubes.elements = qos_cube;
	dif_properties[0].qos_cubes.size = 2;

	dif_properties[1].max_sdu_size=400;
	dif_properties[1].dif_name.process_name = "Test1.DIF";
	qos_cube = (qos_cube_t *) malloc(2*sizeof(qos_cube_t));
	qos_cube[0].name = "qos cube number 3";
	qos_cube[0].id = 3;
	qos_cube[0].flow_spec.jitter = 2;
	qos_cube[0].flow_spec.delay = 200;
	qos_cube[1].name = "qos cube number 4";
	qos_cube[1].id = 4;
	qos_cube[1].flow_spec.jitter = 80;
	qos_cube[1].flow_spec.delay = 180;
	dif_properties[1].qos_cubes.elements = qos_cube;
	dif_properties[1].qos_cubes.size = 2;

	return dif_properties;
}

int register_application(const name_t * name,
                         const name_t * dif)
{
	LOG_DBG("Register application called");
	LOG_DBG("Application process name: %s", name->process_name);
	LOG_DBG("Application process instance: %s", name->process_instance);
	LOG_DBG("Application entity name: %s", name->entity_name);
	LOG_DBG("Application entity instance: %s", name->entity_instance);
	LOG_DBG("DIF name: %s", dif->process_name);
	return 0;
}

int unregister_application(const name_t * name,
                           const name_t * dif)
{
	LOG_DBG("Unregister application called");
	LOG_DBG("Application process name: %s", name->process_name);
	LOG_DBG("Application process instance: %s", name->process_instance);
	LOG_DBG("Application entity name: %s", name->entity_name);
	LOG_DBG("Application entity instance: %s", name->entity_instance);
	LOG_DBG("DIF name: %s", dif->process_name);
	return 0;
}
