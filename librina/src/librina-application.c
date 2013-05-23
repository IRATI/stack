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
	LOG_DBG("Event poll called\n");
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
	LOG_DBG("Event wait called\n");
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
	LOG_DBG("Event set filter called\n");
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
	LOG_DBG("Event get filter called\n");

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
	LOG_DBG("Allocate flow request called\n");
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
int allocate_flow_response(const port_id_t * port_id,
                           const response_reason_t * response)
{
	LOG_DBG("Allocate flow response called\n");
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
	LOG_DBG("Deallocate flow called\n");
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
	LOG_DBG("Write SDU called\n");
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
 * dif_properties: The properties of the requested DIFs.
 */
int get_dif_properties(const name_t * dif_name,
                       dif_properties_t * dif_properties)
{
	LOG_DBG("Get DIF properties called\n");
	return 0;
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
	LOG_DBG("Register application called\n");
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
	LOG_DBG("Unregister application called\n");
	return 0;
}
