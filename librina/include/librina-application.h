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

#ifndef LIBRINA_APPLICATION_H
#define LIBRINA_APPLICATION_H

#include "librina-common.h"

/*
 * The librina-application library provides the native RINA API,
 * allowing applications to i) express their availability to be
 * accessed through one or more DIFS (application registration);
 * ii) allocate and deallocate flows to destination applications
 * (flow allocation and deallocation); iii) read and write data
 * from/to allocated flows (in the form of Service Data Units or
 * SDUs) and iv) query the DIFs available in the system and
 * their properties.
 *
 * For the "slow-path" operations, librina-application interacts
 * with the RINA daemons in by exchanging messages over Netlink
 * sockets. In the case of the "fast-path" operations - i.e. those
 * that need to be invoked for every single SDU: read and write -
 * librina-application communicates directly with the services
 * provided by the kernel through the use of system calls.
 *
 * The librina-application API is event-based; that is: the API
 * provides a different method for each action that can be invoked
 * (allocate_flow, register_application and so on), but only two
 * methods - one blocking, the other non-blocking - to get the
 * results of the operations and SDUs available to be read
 * (event_wait and event_poll).
 */

typedef struct {
	/* This structure defines the characteristics of a flow */

	/* Average bandwidth in bytes/s */
	uint_range_t average_bandwidth;

	/* Average bandwidth in SDUs/s */
	uint_range_t average_sdu_bandwidth;

	/* In milliseconds */
	uint_range_t peak_bandwidth_duration;

	/* In milliseconds */
	uint_range_t peak_sdu_bandwidth_duration;

	/* A value of 0 indicates 'do not care' */
	double undetected_bit_error_rate;

	/* Indicates if partial delivery of SDUs is allowed or not */
	bool_t partial_delivery;

	/* Indicates if SDUs have to be delivered in order */
	bool_t ordered_delivery;

	/*
	 * Indicates the maximum gap allowed among SDUs, a gap of N SDUs
	 * is considered the same as all SDUs delivered. A value of -1
	 * indicates 'Any'
	 */
	int max_allowable_gap;

	/*
	 * In milliseconds, indicates the maximum delay allowed in this
	 * flow. A value of 0 indicates 'do not care'
	 */
	uint32_t  delay;

	/*
	 * In milliseconds, indicates the maximum jitter allowed in this
	 * flow. A value of 0 indicates 'do not care'
	 */
	uint32_t  jitter;

	/*
	 * The maximum SDU size for the flow. May influence the choice
	 * of the DIF where the flow will be created.
	 */
	uint32_t  max_sdu_size;
} flow_spec_t;

typedef struct {
	/* Contains the information of a single event affecting a flow*/

	/* type maps to EVENT_FLOW_[REQUEST|RESPONSE]_RECEIVED */
	uint32_t    type;
	name_t  source_application;
	name_t  destination_application;

	/* The flow characteristics */
	flow_spec_t flow_spec;

	/* The port id of the flow */
	port_id_t  port_id;

	/* Maps a response result */
	response_reason_t response_reason;
} event_flow_t;

typedef struct {
	/* Contains the information of an event that reports available SDUs*/

	/* The port id of the flow */
	port_id_t port_id;

	/* The SDU received */
	sdu_t sdu;
} event_sdu_t;

typedef struct {
	/* Contains the information of an event related to application registration*/

	/* The application affected by the event */
	name_t  application_name;

	/* DIFs affected by the event */
	name_t ** dif_names;
} event_registration_t;

typedef enum {
	/* Defines the different events relevant to an application process*/

	EVENT_ALLOCATE_FLOW_REQUEST_RECEIVED,
	EVENT_FLOW_DEALLOCATED,
	EVENT_APPLICATION_REGISTRATION_CANCELED,
	EVENT_SDU_RECEIVED
} event_type_t;

typedef struct {
	/* The event discriminator */
	event_type_t type;

	/* This union contains the event related data */
        union {
		event_flow_t         flow;
		event_registration_t registration;
		event_sdu_t 	   sdu;
        } data;
} event_t;

typedef struct {
	/* This structure defines the properties of a QoS cube */

	/* The QoS cube name */
	string_t name;

	/* The QoS cube id */
	int id;

	/* The flow characteristics supported by this QoS cube */
	flow_spec_t flow_spec;
} qos_cube_t;

typedef struct {
	/*
	 * This structure defines the service properties of a DIF; that
	 * is, the properties of a DIF visible to an application
	 */

	/* The name of the DIF */
	name_t dif_name;

	/*
	 * The maximum SDU size this DIF can handle (writes with bigger
	 * SDUs will return an error, and read will never return an SDUs
	 * bigger than this size
	 */
	int  max_sdu_size;

	/* The QoS cubes supported by the DIF */
	qos_cube_t ** qos_cubes;
} dif_properties_t;

typedef int (* event_filter_t)(const event_t * event);

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
int ev_poll(event_t * event);

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
int ev_wait(event_t * event);

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
void ev_set_filter(event_filter_t filter);

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
event_filter_t ev_get_filter(void);

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
                                const flow_spec_t * flow_spec);

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
                           const response_reason_t response);

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
int deallocate_flow (port_id_t port_id);

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
int write_sdu(port_id_t port_id, sdu_t * sdu);

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
                       dif_properties_t * dif_properties);

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
                         const name_t * dif);

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
                           const name_t * dif);

#endif
