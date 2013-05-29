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

/*
 * Polls for currently pending events, and returns 1 if there are any
 * pending events, or 0 if there are none available.  If 'event' is
 * not NULL, the next event is removed from the queue and stored in
 * that area.
 *
 * Outputs
 *
 * event: An event informing that something happened.
 */
int ev_poll(event_t * event)
{
	LOG_DBG("Event poll called");
	event->type = EVENT_ALLOCATE_FLOW_REQUEST_RECEIVED;
	return 0;
}

/*
 * Description
 *
 * Waits indefinitely for the next available event, returning 1, or 0
 * if there was an error while waiting for events.  If 'event' is not
 * NULL, the next event is removed from the queue and stored in that
 * area.
 *
 * Outputs
 *
 * event: An event informing that something happened.
 */
int ev_wait(event_t * event)
{
	LOG_DBG("Event wait called");
	event->type = EVENT_SDU_RECEIVED;

	return 0;
}

/*
 * Description
 *
 * The ev_set_filter() function sets up a filter to process all events
 * before they change internal state and are posted to the internal
 * event queue. If the filter returns 1, then the event will be added
 * to the internal queue. If it returns 0, then the event will be dropped
 * from the queue, but the internal state will still be updated. This
 * allows selective filtering of dynamically arriving events.
 *
 * Inputs
 *
 * filter: The filter to setup.
 */
void ev_set_filter(event_filter_t filter)
{
	LOG_DBG("Event set filter called");
}

/*
 * Description
 *
 * The ev_get_filter() returns the current filter installed. If the function
 * returns NULL, then no filters are currently installed.
 *
 * Outputs
 *
 * filter: The current filter installed.
 */
event_filter_t ev_get_filter(void)
{
	LOG_DBG("Event get filter called");

	event_filter_t *installed_filter;
	installed_filter = (event_filter_t *) malloc (sizeof (event_filter_t));
	return installed_filter;
}

/*
 * Description
 *
 * Invoked by the source application, when it wants a flow to be allocated
 * to a destination application with certain characteristics.
 *
 * Inputs
 *
 * source: The source application process naming information (required)
 * destination: The destination application process naming information (required)
 * flow_spec: The QoS params requested for the flow (optional)
 *
 * Outputs
 *
 * port_id: The handle to the flow, provided by the system
 */
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

/*
 * Description
 *
 * Invoked by the destination application, confirming or denying a flow
 * allocation request.
 *
 * Inputs
 *
 * port_id: The port id of the flow, whose request is being confirmed
 * or denied
 * response: Indicates whether the flow is accepted or not, with an
 * optional reason explaining why and indications if a response should
 * be returned to the flow requestor.
 */
int allocate_flow_response(port_id_t port_id,
                           const response_reason_t response)
{
	LOG_DBG("Allocate flow response called");
	LOG_DBG("Port id: %d", port_id);
	LOG_DBG("Response: %s", response);
	return 0;
}

/*
 * Description
 *
 * Causes the resources allocated to a certain flow to be released,
 * and any state associated to the flow to be removed.
 *
 * Inputs
 *
 * port_id: The port id of the flow to be deallocated.
 */
int deallocate_flow (port_id_t port_id)
{
	LOG_DBG("Deallocate flow called");
	LOG_DBG("Port id: %d", port_id);
	return 0;
}

/*
 * Description
 *
 * Called by an application when it wants to write an SDU to
 * the flow.
 *
 * Inputs
 *
 * port_id: The port id of the flow.
 * sdu: The SDU to be written to the flow.
 */
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

/*
 * Description
 *
 * Called by an application when it wants to know the DIFs in the
 * system it can use, and what are their properties.
 *
 * Inputs
 *
 * dif_name: The name of the DIF whose properties the application
 * wants to know. If no name is provided, the call will return the
 * properties of all the DIFs available to the application.
 *
 * Outputs
 *
 * dif_properties: A pointer to an array of properties of the requested DIFs.
 * int: The number of elements in the array (0 or more), if the call is
 * successful. A negative number indicating an error if the call fails.
 */
int get_dif_properties(const name_t * dif_name,
                       dif_properties_t * dif_properties)
{
	LOG_DBG("Get DIF properties called");
	dif_properties[0].max_sdu_size=300;
	dif_properties[0].dif_name.process_name = "Test.DIF";
	dif_properties[1].max_sdu_size=400;
	dif_properties[1].dif_name.process_name = "Test1.DIF";

	return 2;
}

/*
 * Description
 *
 * Called by an application when it wants to be advertised (and
 * reachable) through a DIF..
 *
 * Inputs
 *
 * name: The name of the application (required).
 * dif: The name of a DIF where the application wants to be registered.
 * In case none is provided, the RINA software could decide for the
 * application (e.g. register the application in all DIFs, register
 * the application in a default one, etc.)
 */
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

/*
 * Description
 *
 * Called by an application when it wants to stop being advertised
 * (and reachable) through a DIF.
 *
 * Inputs
 *
 * name: the name of the application (required)
 * dif: the name of a DIF where the application is registered. In case
 * none is provided, it will be unregistered from all the DIFs the
 * application is currently registered at.
 */
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
