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
#include <stdio.h>
#include <stdlib.h>

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
int ev_poll(event_t * event){
	printf("Event poll called\n");
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
int ev_wait(event_t * event){
	printf("Event wait called\n");
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
void ev_set_filter(event_filter_t filter){
	printf("Event set filter called\n");
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
event_filter_t ev_get_filter(void){
	printf("Event get filter called\n");

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
                                const flow_spec_t * flow_spec){
	printf("Allocate flow request called\n");
	port_id_t port_id = 25;
	return port_id;
}
