/*
 *  Common definition placeholder
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net> 
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

#ifndef RINA_COMMON_H
#define RINA_COMMON_H


#include <linux/types.h>

typedef unsigned int	ipc_process_address_t;
typedef uint16_t	port_id_t;
typedef int		ipc_process_id_t;
typedef unsigned char	utf8_t;
typedef unsigned char	string_t;
typedef uint		uint_t;
typedef uint		response_reason_t;
typedef uint            cep_id_t;
typedef uint16_t        address_t;
typedef uint            seq_num_t;
/* FIXME : The PDU type should be defined correctly in the near future */
typedef uint            pdu_type_t;
/* FIXME : The qos_id_t should be defined correctly in the near future */
typedef uint            qos_id_t;
typedef uint            timeout_t;

/*
 * The value should be interpreted as false if the value is 0 or true
 * otherwise.
 * This typedef should be interpreted as ISO C99 bool/_Bool and could be
 * replaced by the inclusion of stdbool.h where/when possible.
 *
 */
typedef int bool_t;

/* This structure represents raw data */
struct buffer_t {
	char * data;
	size_t size;
};

/* This structure represents a SDU is */
struct sdu_t {
        struct buffer_t * buffer;
};

struct uint_range_t {
	/* Minimum value */
	uint_t min_value;

	/* Maximum value */
	uint_t max_value;
};

struct name_t {
	/*
	 * The process_name identifies an application process within the
	 * application process namespace. This value is required, it
	 * cannot be NULL. This name has global scope (it is defined by
	 * the chain of IDD databases that are linked together), and is
	 * assigned by an authority that manages the namespace that
	 * particular application name belongs to.
         */
	string_t * process_name;

	/*
	 * The process_instance identifies a particular instance of the
	 * process. This value is optional, it may be NULL.
	 *
         */
	string_t * process_instance;
	
	/*
         * The entity_name identifies an application entity within the
	 * application process. This value is optional, it may be NULL.
         */
	string_t * entity_name;

	/*
         * The entity_name identifies a particular instance of an entity within
	 * the application process. This value is optional, it may be NULL.
         */
	string_t * entity_instance;
};

struct flow_spec_t {
	/* This structure defines the characteristics of a flow */

	/* Average bandwidth in bytes/s */
	struct uint_range_t *average_bandwidth;
	/* Average bandwidth in SDUs/s */
	struct uint_range_t *average_sdu_bandwidth;
	/* In milliseconds */
	struct uint_range_t *peak_bandwidth_duration;
	/* In milliseconds */
	struct uint_range_t *peak_sdu_bandwidth_duration;

	/* A value of 0 indicates 'do not care' */
	double               undetected_bit_error_rate;
	/* Indicates if partial delivery of SDUs is allowed or not */
	bool_t               partial_delivery;
	/* Indicates if SDUs have to be delivered in order */
	bool_t               ordered_delivery;
	/*
	 * Indicates the maximum gap allowed among SDUs, a gap of N
	 * SDUs is considered the same as all SDUs delivered.
	 * A value of -1 indicates 'Any'
	 */
	int                  max_allowable_gap;
	/*
	 * In milliseconds, indicates the maximum delay allowed in this
	 * flow. A value of 0 indicates 'do not care'
	 */
	uint_t               delay;
	/*
	 * In milliseconds, indicates the maximum jitter allowed
	 * in this flow. A value of 0 indicates 'do not care'
	 */
	uint_t               jitter;
	/*
	 * The maximum SDU size for the flow. May influence the choice
	 * of the DIF where the flow will be created.
	 */
	uint_t              max_sdu_size;
};

struct pci_t {
	address_t source;
	address_t destination;
	pdu_type_t type;
	cep_id_t source_cep_id;
	cep_id_t dest_cep_id;
	qos_id_t qos_id;
	seq_num_t sequence_number;
};

struct pdu_t {
	struct pci_t    *pci;
	struct buffer_t *buffer;
};

struct connection_t {
	/* This structure defines an EFCP connection */

	/* The port_id this connection is bound to */
	port_id_t port_id;
	
	/*
	 * The address of the IPC Process that is the source of this
	 * connection
	 */
	address_t source_address;
	
	/*
	 * The address of the IPC Process that is the destination of
	 * this connection
	 */
	address_t destination_address;
	
	/* The source connection endpoint Id */
	cep_id_t source_cep_id;
	
	/* The destination connection endpoint id */
	cep_id_t dest_cep_id;
	
	/* The QoS id */
	qos_id_t qos_id;
	
	/* FIXME : policy type remains undefined */
	/* The list of policies associated with this connection */
	/* policy_t ** policies; */
};

struct rmt_conf_t {

	/* To do, only a placeholder right now */

};

struct rmt_instance_t {
	/* This structure holds per-RMT instance data */

	/* HASH_TABLE(queues, port_id_t, rmt_queues_t *); */
	struct rmt_instance_config_t * configuration;

	/*
         * The PDU-FT access might change in future prototypes but
         * changes in its underlying data-model will not be reflected
         * into the (external) API, since the PDU-FT is accessed by
         * RMT only.
         */

	/* LIST_HEAD(pdu_fwd_table, pdu_fwd_entry_t); */
};

typedef enum {
	DIF_TYPE_NORMAL,
	DIF_TYPE_SHIM_IP,
	DIF_TYPE_SHIM_ETH
} dif_type_t;

struct normal_ipc_process_conf_t {
	/*
	 * Configuration of the kernel components of a normal IPC Process.
	 * Defines the struct for the kernel components of a fully RINA IPC
	 * Process.
 	 */

	/* The address of the IPC Process */
	ipc_process_address_t address;

	/* The configuration of the EFCP component */
	struct efcp_conf_t *efcp_config;

	/* The configuration of the RMT component */

	struct rmt_conf_t *rmt_config;
};

struct ipc_process_shim_ethernet_conf_t {
	/*
	 * Configuration of the kernel component of a shim Ethernet IPC 
	 * Process
	 */

	/* The vlan id */
	int vlan_id;

	/* The name of the device driver of the Ethernet interface */
	string_t *device_name;
};

struct ipc_process_shim_tcp_udp_conf_t {

	
	/*
	 * Configuration of the kernel component of a shim TCP/UDP IPC 
	 * Process
	 */

	/* FIXME: inet address the IPC process is bound to */
	//in_addr_t *inet_address;

	/* The name of the DIF */
	struct name_t *dif_name;
};

struct ipc_process_conf_t {
	/*	
	 * Contains the configuration of the kernel components of an IPC
	 * Proccess.
	 */

	/* The DIF type discriminator */
	dif_type_t type;

	union{
		struct normal_ipc_process_conf_t *normal_ipcp_conf;
		struct ipc_process_shim_ethernet_conf_t *shim_eth_ipcp_conf;
		struct ipc_process_shim_tcp_udp_conf_t *shim_tcp_udp_ipcp_conf;
	} ipc_process_conf;
};

struct normal_ipc_process_t {
	/*
         * Contains all the data structures of a normal IPC Process 
         */

	/* The ID of the IPC Process */
	ipc_process_id_t ipcp_id; 
	
	/* Contains the configuration of the kernel components */
	struct normal_ipc_process_cont_t  *configuration;

	/* The EFCP data structure associated to the IPC Process */
	struct efcp_ipc_t *efcp;
	
	/* The RMT instance associated to the IPC Process */
	struct rmt_instance_t *rmt;

};

struct ipc_process_shim_ethernet_t {
	/* 
	 * Contains all he data structures of a shim IPC Process over Ethernet
	 */

	/* The ID of the IPC Process */
	ipc_process_id_t ipcp_id;

	/* The configuration of the module */
	struct ipc_process_shim_ethernet_conf_t *configuration;

	/* The module that performs the processing */
	struct shim_eth_instance_t *shim_eth_ipc_process;

};

struct ipc_process_shim_tcp_udp_t {
	/* 
	 * Contains all he data structures of a shim IPC Process over TCP/IP 
	 */

	/* The ID of the IPC Process */
	ipc_process_id_t ipcp_id;

	/* The configuration of the module */
	struct ipc_process_shim_tcp_udp_conf_t *configuration;

	/* The module that performs the processing */
	struct shim_tcp_udp_instance_t *shim_tcp_udp_ipc_process;
};

struct ipc_process_data_t {
	
	/* The DIF type descriminator */
	dif_type_t type;

	union {
		struct normal_ipc_process_t *normal_ipcp;
		struct ipc_process_shim_ethernet_t *shim_eth_ipcp;
		struct ipc_process_shim_tcp_udp_t *shim_tcp_udp_ipcp;
	} ipc_process;

};

struct ipc_process_t {
	dif_type_t type;
	struct ipc_process_data_t data;
};

struct flow_t {
	/* The port-id identifying the flow */
	port_id_t port_id;

	/*
         * The components of the IPC Process that will handle the
         * write calls to this flow
         */
	struct ipc_process_t *ipc_process;

	/*
         * True if this flow is serving a user-space application, false
         * if it is being used by an RMT
         */
	bool_t application_owned;

	/*
         * In case this flow is being used by an RMT, this is a pointer
         * to the RMT instance.
         */

	struct rmt_instance_t rmt_instance;

	//FIXME: Define QUEUE
	//QUEUE(segmentation_queue, pdu_t *);
	//QUEUE(reassembly_queue,	pdu_t *);
	//QUEUE(sdu_ready, sdu_t *);
};

#endif