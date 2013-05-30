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

#ifndef LIBRINA_IPC_MANAGER_H
#define LIBRINA_IPC_MANAGER_H

#include "librina-common.h"

/*
 * This library provides the functionalities facilitating the IPC Manager to
 * perform the tasks related to IPC Process creation, deletion and
 * configuration; to serve requests by application processes; and to
 * manage the Inter-­‐DIF Directory. It abstracts the details that
 * allow the IPC Manager to communicate with the IPC Process daemons and
 * application processes in user space (using Netlink sockets) and the RINA
 * components at the kernel (using system calls, Netlink sockets and/or procfs
 * and sysfs). IPC Manager operations with IPC Processes include their creation,
 * update, deletion, assignment to DIFs, registration in N-­1 DIFs and
 * triggering the enrollment in DIFs. Application requests include the
 * registration and unregistration in DIFs, and the allocation and
 * deallocation of flows. The Inter-DIF Directory management involves
 * coordinating with other IPC Managers to keep the IDD up to date.
 */

typedef enum {
	/* Defines the different types of DIFs */

	DIF_TYPE_NORMAL,
	DIF_TYPE_SHIM_IP,
	DIF_TYPE_SHIM_ETH
} dif_type_t;

typedef struct {
	/*
	 * This is a generic placeholder which should be defined
	 * during the second prototype activities
	 */
} policy_t;

typedef struct {
	/* This structure defines an array of name_t */

	/* Pointer to the first element of the array */
	policy_t * elements;

	/* Number of elements in the array */
	int size;
} array_of_policy_t;

typedef struct {
	/* Encapsulates the information to configure a normal IPC Process*/

	/* The list of policies */
	array_of_policy_t policies;

	/*
	 * The names of the DIFs that this DIFs has to use as N-1 DIFs.
	 * Alternatively if it’s a shim-dif they could be the ethernet
	 * interfaces
	 */
	array_of_name_t n_minus_1_dif_names;
} normal_dif_config_t;

typedef struct {
	/* Encapsulates the information to configure a normal IPC Process*/

	/* The device name to which the shim DIF will be bound (a VLAN) */
	string_t device_name;
} shim_eth_dif_config_t;

typedef union {
	/*
	 * This union contains the configuration data specific to each
	 * type of DIF
	 */

	 /* Specific configuration of a normal DIF */
	 normal_dif_config_t    normal_dif_config;

	 /* Specific configuraiton of a shim DIF over Ethernet */
	 shim_eth_dif_config_t  shim_eth_dif_config;
} specific_configuration_t;

typedef struct {
	/* The DIF type, used as the union discriminator */
	dif_type_t dif_type;

	/* The DIF name */
	name_t dif_name;

	/*
	 * The maximum SDU size this DIF can handle (writes with bigger
	 * SDUs will return an error, and read will never return an SDU
	 * bigger than this size
	 */
	int max_sdu_size;

	/* The QoS cubes supported by the DIF */
	array_of_qos_cube_t cubes;

	/*
	 * This union contains the configuration data specific to each
	 * type of DIF
	 */
    specific_configuration_t specific_conf;
} dif_configuration_t;

/* A regular expression */
typedef string_t regex_t;

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
				ipc_process_id_t ipc_process_id);

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
int ipcm_destroy(ipc_process_id_t ipc_process_id);

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
 * one or more N-1 DIFs to register at, and one or more “neighbor” IPC Processes to
 * initiate enrollment with (required).
 */
int ipcm_assign(ipc_process_id_t ipc_process_id,
				const dif_configuration_t * dif_configuration);

/*
 * Description
 *
 * Invoked by the IPC Manager to notify an IPC Process that he has been
 * registered to the N-1 DIF designed by dif_name.
 *
 * Inputs
 *
 * ipc_process_id: The ID of the IPC Process that has been registered.
 * dif_name: The name of the DIF where the IPC Process has been registered.
 */
int ipcm_notify_register(ipc_process_id_t ipc_process_id,
						name_t * dif_name);

/*
 * Description
 *
 * Invoked by the IPC Manager to notify an IPC Process that he has been
 * unregistered from the N-1 DIF designed by dif_name.
 *
 * Inputs
 *
 * ipc_process_id: The ID of the IPC Process that has been unregistered.
 * dif_name: The name of the DIF where the IPC Process has been unregistered.
 */
int ipcm_notify_unregister(ipc_process_id_t ipc_process_id,
							name_t * dif_name);

/*
 * Description
 *
 * Invoked by the IPC Manager to trigger the enrollment of an IPC Process in
 * the system with a DIF, reachable through a certain N-1 DIF. The operation
 * blocks until the IPC Process has successfully enrolled or an error occurs.
 *
 * Inputs
 *
 * process_id: The id of the IPC Process affected by the operation (required).
 * dif_name: The name of the DIF the IPC Process has to enroll to (it will
 * resolve to an IPC Process Name).
 * nm1_dif: The underlying DIF that will support the flow(s) between the IPC
 * Process and the DIF to join.
 */
int ipcm_enroll(ipc_process_id_t ipc_process_id,
				const name_t * dif_name,
				const name_t * nm1_dif);

/*
 * Description
 *
 * Invoked by the IPC Manager to force an IPC Process to deallocate all the N-1
 * flows to a neighbor IPC Process (for example, because it has been identified
 * as a “rogue” member of the DIF). The operation blocks until the IPC Process
 * has successfully completed or an error is returned.
 *
 * Inputs
 *
 * ipc_process_id:  The id of the IPC Process affected by the operation
 * (required).
 * neighbor: The name of the neighbor IPC Process we have to disconnect from.
 */
int ipcm_disconnect(ipc_process_id_t ipc_process_id,
					const name_t * neighbor);

/*
 * Description
 *
 * Invoked by the IPC Manager to register an application in a DIF through an
 * IPC Process. A register_app Netlink message is sent to the IPC Process. The
 * operation blocks until the IPC Process has successfully registered or an
 * error occurs.
 *
 * Inputs
 *
 * app_name: The name of the application to be registered.
 * app_socket:  The process id of the application process to be registered. It is
 * also the address of the Netlink socket where the application process will
 * be listening.
 * ipc_process_id: The ID of the IPC Process where the application will be
 * registered.
 */
int ipcm_register_app(name_t * app_name,
					  ipc_process_id_t ipc_process_id);

/*
 * Description
 *
 * Invoked by the IPC Manager to notify an application about the result of the
 * registration in a DIF operation. A register_application_response Netlink
 * message is sent to the Application Process.
 *
 * Inputs
 *
 * app_name: The name of the application that has registered
 * response_reason:  The structure with the information about the registration
 * process result (required).
 */
int ipcm_notify_app_registration(name_t * app_name,
								response_reason_t response_reason);

/*
 * Description
 *
 * Invoked by the IPC Manager to unregister an application from a DIF. An
 * unregister_app Netlink message is sent to the IPC Process. The operation
 * blocks until the IPC Process has successfully unregistered the application
 * or an error occurs, and the IPC Process sends an unregister_app_response
 * Netlink message with the information of the result.
 *
 * Inputs
 *
 * app_name: The name of the application to be unregistered.
 * ipc_process_id: The ID of the IPC Process where the application will be
 * registered
 *
 */
int ipcm_unregister_app(name_t * app_name,
						ipc_process_id_t ipc_process_id);

/*
 * Description
 *
 * Invoked by the IPC Manager to notify an application about the result of
 * the unregistration in a DIF operation. An unregister_application_response
 * Netlink message is sent to the Application Process.
 *
 * Inputs
 *
 * app_name: The name of the application that has unregistered
 * response_reason:  The structure with the information about the
 * unregistration process result (required).
 */
int ipcm_notify_app_unregistration(name_t * app_name,
						response_reason_t response_reason);

/*
 * Description
 *
 * Invoked by the IPC Manager to request an IPC Process the allocation of a
 * flow. Since all flow allocation requests go through the IPC Manager,
 * and port_ids have to be unique within the whole system, the IPC Manager
 * is the best candidate for managing the port_id space.
 *
 * Inputs
 *
 * source: Name of the source application (required).
 * destination: Name of the destination application (required)
 * flow_spec: Characteristics of the requested flow (optional)
 * port_id: The port_id selected for this flow allocation
 * ipc_process_id: The id of the IPC Process to whom the allocate flow
 * request is
 * being redirected.
 *
 */
int ipcm_allocate_flow(name_t * source,
					name_t * destination,
					flow_spec_t* flow_spec,
					port_id_t port_id,
					ipc_process_id_t ipc_process_id);

/*
 * Description
 *
 * Invoked by the IPC Manager to respond to the Application Process that
 * requested a flow. It sends an allocate_request_result Netlink message
 * with the information required to operate with the flow.
 *
 * Inputs
 *
 * app_name: Name of the source application (required).
 * port_id: The port_id selected for this flow allocation (required).
 * ipc_process_id: The IPC process id of the IPC Process that allocated the
 * flow. It is required so that the application will be able to send Netlink
 * messages to the IPC Process.
 * response_reason: Informs about the result of the flow allocation request.
 */
int ipcm_notify_flow_allocation(name_t * app_name,
					port_id_t  port_id,
					ipc_process_id_t ipc_process_id,
					response_reason_t response_reason);

/*
 * Description
 *
 * This API is used by the IPC Manager to retrieve information from the RIB of a
 * specific IPC process. The operation will block until a reply with 0 or more
 * objects is received, or an error occurs.
 *
 * Inputs
 *
 * ipc_process_id:  The id of the IPC Process affected by the operation
 * (required).
 * object_class: The class of the object being queried
 * object_name: The object name, i.e. the node in the object tree where the
 * query starts (required).
 * scope: Number of levels below the object_name the query affects (optional,
 * 0 is the default).
 * filter: a regular expression (regexp) applied to all nodes affected by the
 * query, in order to decide whether they have to be returned or not (optional).
 *
 * Outputs
 * rib_object_t: Points to the first element of the array of RIB objects returned
 * by this call.
 * numberOfObjects: The number of RIB objects returned
 */
rib_object_t *ipcm_query_rib(ipc_process_id_t ipc_process_id,
				string_t object_class,
				string_t object_name,
				int scope,
				regex_t filter,
				int * number_of_objects);


#endif
