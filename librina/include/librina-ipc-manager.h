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


#endif
