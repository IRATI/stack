/*
 *  Shim IPC Process for Ethernet
 *
 *    Sander Vrijders <sander.vrijders@intec.ugent.be>
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

#ifndef RINA_SHIM_ETH_H
#define RINA_SHIM_ETH_H

typedef utf8_t interface_name_t;

struct shim_eth_info_t {
	name_t *name;
	uint16_t vlan_id;
	interface_name_t *interface_name;
};

enum ipc_config_type_t {
	IPC_CONFIG_INT,
	IPC_CONFIG_UINT
};

struct ipc_config_t {
	const char *name;
	ipc_config_type_t type;
	/* will be interpreted based on type */
	void *value; 
};

enum port_id_state_t {
	PORT_STATE_NULL,
	PORT_STATE_RECIPIENT_ALLOCATE_PENDING,
	PORT_STATE_INITIATOR_ALLOCATE_PENDING,
	PORT_STATE_ALLOCATED
};

struct shim_eth_flow_t {
	uint64_t src_mac;
	uint64_t dst_mac;
	port_id_t port_id;
	port_id_state_t port_id_state;
	QUEUE(sdu_queue, sdu_t *);
};

/*
* Contains all the information associated to an instance of a
* shim Ethernet IPC Process, as well as pointers to the
* functions to manipulate it
*/
struct shim_eth_instance_t {
        /* The configuration of the shim IPC Process */
	ipc_config_t configuration;
	/* Pointer to the device driver data_structure */
	device_t * device_driver;
	/* Stores the state of flows indexed by port_id */
	HASH_TABLE(flow_to_port_id, port_id_t, shim_eth_flow_t);
};

/*
* The container for the shim IPC Process over Ethernet
* component
*/
struct shim_eth_t {
        /* Stores the state of shim IPC Process instances */
	HASH_TABLE(shim_eth_instances, ipc_process_id_t, shim_eth_instance_t); 
};

ipc_process_id_t shim_eth_create(const name_t *name, const ipc_config_t **config);
int shim_eth_destroy(ipc_process_id_t ipc_process_id);
port_id_t shim_eth_allocate_flow_request(const name_t *source, const name_t *dest, const flow_spec_t *flow_spec);
int shim_eth_allocate_flow_response(const port_id_t *port_id, const response_reason_t *response);
int shim_eth_deallocate_flow(port_id_t port_id);
int shim_eth_register_application(const name_t *name);
int shim_eth_unregister_application(const name_t *name);
int shim_eth_write_sdu(port_id_t port_id, sdu_t *sdu);
int shim_eth_init(void);
void shim_eth_exit(void);

#endif

