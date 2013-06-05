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

#ifndef RINA_SHIMS_SHIM_ETH_H
#define RINA_SHIMS_SHIM_ETH_H

#include <linux/hashtable.h>

#include "../kipcm.h"
#include "../common.h"

/* Holds the configuration of one shim IPC process */
struct shim_eth_info_t {
	struct name_t * name;
	uint16_t        vlan_id;
	string_t *      interface_name;
};

enum ipc_config_type_t {
	IPC_CONFIG_NAME,
	IPC_CONFIG_UINT,
	IPC_CONFIG_STRING
};

struct ipc_config_t {
	enum ipc_config_type_t type;
	/* will be interpreted based on type */
	void *value; 
};

enum port_id_state_t {
	PORT_STATE_NULL,
	PORT_STATE_RECIPIENT_ALLOCATE_PENDING,
	PORT_STATE_INITIATOR_ALLOCATE_PENDING,
	PORT_STATE_ALLOCATED
};

/* Hold the information related to one flow*/
struct shim_eth_flow_t {
	uint64_t             src_mac;
	uint64_t             dst_mac;
	port_id_t            port_id;
	enum port_id_state_t port_id_state;
	/* FIXME: Will probably be a sk_buff_head or a linked list 
	   holding only the SDUs */

        /* QUEUE(sdu_queue, sdu_t *); */
};

/*
 * Contains all the information associated to an instance of a
 * shim Ethernet IPC Process, as well as pointers to the
 * functions to manipulate it
 */
struct shim_eth_instance_t {
        /* The configuration of the shim IPC Process */
	struct shim_eth_info_t info;

	/* FIXME: Pointer to the device driver data_structure */
	/* device_t * device_driver; */

	/* FIXME: Stores the state of flows indexed by port_id */
	/* HASH_TABLE(flow_to_port_id, port_id_t, shim_eth_flow_t); */
};

/*
 * The container for the shim IPC Process over Ethernet
 * component
 */
/* Stores the state of shim IPC Process instances */
/* HASH_TABLE(shim_eth_instances, ipc_process_id_t, shim_eth_instance_t); */
struct shim_eth_t {
	ipc_process_id_t ipc_process_id;
	struct shim_eth_instance_t shim_eth_instance;
	struct list_head list;
};

ipc_process_id_t shim_eth_create(struct ipc_config_t ** config);
int              shim_eth_destroy(ipc_process_id_t ipc_process_id);

/* FIXME: flow_spec_t should not reach this point ..., QoS ids should suffice */
port_id_t shim_eth_allocate_flow_request(struct name_t *      source,
                                         struct name_t *      dest,
                                         struct flow_spec_t * flow_spec);
int  shim_eth_allocate_flow_response(port_id_t           port_id,
                                     response_reason_t * response);
int  shim_eth_deallocate_flow(port_id_t port_id);
int  shim_eth_register_application(struct name_t * name);
int  shim_eth_unregister_application(struct name_t * name);
int  shim_eth_write_sdu(port_id_t port_id, const struct sdu_t * sdu);
int  shim_eth_init(void);
void shim_eth_exit(void);
int  shim_eth_ipc_create(const struct name_t * name,
			 ipc_process_id_t      ipcp_id);
int  shim_eth_ipc_configure(ipc_process_id_t      ipcp_id,
			    const struct ipc_process_shim_ethernet_conf_t *configuration);

#endif

