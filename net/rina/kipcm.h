/*
 * K-IPCM (Kernel-IPC Manager)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
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

#ifndef RINA_KIPCM_H
#define RINA_KIPCM_H

#include <linux/syscalls.h>
#if 0
#include <linux/hash.h>
#include <linux/hashtable.h>
#endif

#include "common.h"
#include "efcp.h"
#include "rmt.h"

/* FIXME: These inclusions should be avoided */
//#include "shim-eth.h"
#include "shim-tcp-udp.h"

typedef enum {
        DIF_TYPE_NORMAL,

        /* FIXME: DIF_TYPE_SHIM_IP should be DIF_TYPE_SHIM_TCP_IP */
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

	/* EFCP component configuration */
	struct efcp_conf_t *  efcp_config;

	/* RMT component configuration */
	struct rmt_conf_t *   rmt_config;
};

struct ipc_process_shim_ethernet_conf_t {
	/*
	 * Configuration of the kernel component of a shim Ethernet IPC 
	 * Process
	 */

	/* The vlan id */
	int        vlan_id;

	/* The name of the device driver of the Ethernet interface */
	string_t * device_name;
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

	union {
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

struct ipc_process_t {
	dif_type_t type;

	union {
		struct normal_ipc_process_t *        normal_ipcp;
		struct ipc_process_shim_ethernet_t * shim_eth_ipcp;
		struct ipc_process_shim_tcp_udp_t *  shim_tcp_udp_ipcp;
	} data;
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
	struct kfifo *sdu_ready;
};

struct kipc_t {
	/*
         * Maintained and used by the K-IPC Manager to return the proper flow
	 * instance that contains the modules that provide the Data Transfer
	 * Service in each kind of IPC Process.
         */

	//FIXME Define HASH_TABLE
	//HASH_TABLE(port_id_to_flow, port_id_t, struct flow_t *);
        struct list_head * port_id_to_flow;
	
	/* A table with all the instances of IPC Processes, indexed by
	 * process_id. */
	//FIXME Define HASH_TABLE
	//HASH_TABLE(id_to_ipcp, ipc_process_id_t, struct ipc_process_t *);
	struct list_head * id_to_ipcp;
};

struct id_to_ipcp_t {
        ipc_process_id_t       id; /* key */
        struct ipc_process_t * ipcprocess; /* Value*/
        struct list_head       list;
};

struct port_id_to_flow_t {
        port_id_t         port_id; /* key */
        const struct flow_t   * flow; /* value */
        struct list_head  list;
};

int  kipcm_init(void);
void kipcm_exit(void);
int  kipcm_add_entry(port_id_t port_id, const struct flow_t * flow);
int  kipcm_remove_entry(port_id_t port_id);
int  kipcm_post_sdu(port_id_t port_id, const struct sdu_t * sdu);

int  kipcm_read_sdu(port_id_t      port_id,
	      bool_t         block,
	      struct sdu_t * sdu);
int  kipcm_write_sdu(port_id_t            port_id,
	       const struct sdu_t * sdu);

int  kipcm_ipc_process_create(const struct name_t * name,
			ipc_process_id_t      ipcp_id,
			dif_type_t            type);
int  kipcm_ipc_process_configure(ipc_process_id_t                  ipcp_id,
			   const struct ipc_process_conf_t * configuration);
int  kipcm_ipc_process_destroy(ipc_process_id_t ipcp_id);

#endif
