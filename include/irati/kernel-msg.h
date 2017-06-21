/*
 * Kernel control messages for the IRATI implementation,
 * shared by kernel and user space components
 *
 *    Eduard Grasa <eduard.grasa@i2cat.net>
 *    Vincenzo Maffione <v.maffione@nextworks.it>
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

#ifndef IRATI_KERN_MSG_H
#define IRATI_KERN_MSG_H

#include "irati/kucommon.h"

#define IRATI_RINA_C_MIN (RINA_C_MIN + 1)
#define IRATI_RINA_C_MAX (RINA_C_MAX)

typedef enum {

        /* 0 Unespecified operation */
        RINA_C_MIN = 0,

        /* 1 IPC Manager -> IPC Process */
        RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST,

        /* 2 IPC Process -> IPC Manager */
        RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE,

        /* 3 IPC Manager -> IPC Process */
        RINA_C_IPCM_UPDATE_DIF_CONFIG_REQUEST,

        /* 4 IPC Process -> IPC Manager */
        RINA_C_IPCM_UPDATE_DIF_CONFIG_RESPONSE,

        /* 5 IPC Manager -> IPC Process */
        RINA_C_IPCM_IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION,

        /* 6 IPC Manager -> IPC Process */
        RINA_C_IPCM_IPC_PROCESS_DIF_UNREGISTRATION_NOTIFICATION,

        /* 7 IPC Manager -> IPC Process */
        RINA_C_IPCM_ENROLL_TO_DIF_REQUEST,

        /* 8 IPC Process -> IPC Manager */
        RINA_C_IPCM_ENROLL_TO_DIF_RESPONSE,

        /* 9 IPC Manager -> IPC Process */
        RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_REQUEST,

        /* 10 IPC Process -> IPC Manager */
        RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_RESPONSE,

        /* 11 IPC Manager -> IPC Process */
        RINA_C_IPCM_ALLOCATE_FLOW_REQUEST,

        /* 12 Allocate flow request from a remote application */
        /* IPC Process -> IPC Manager */
        RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_ARRIVED,

        /* 13 IPC Process -> IPC Manager */
        RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_RESULT,

        /* 14 IPC Manager -> IPC Process */
        RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE,

        /* 15 IPC Manager -> IPC Process */
        RINA_C_IPCM_DEALLOCATE_FLOW_REQUEST,

        /* 16 IPC Process -> IPC Manager*/
        RINA_C_IPCM_DEALLOCATE_FLOW_RESPONSE,

        /*  17 IPC Process -> IPC Manager, flow deallocated without the */
        /*  application having requested it */
        RINA_C_IPCM_FLOW_DEALLOCATED_NOTIFICATION,

        /* 18 IPC Manager -> IPC Process */
        RINA_C_IPCM_REGISTER_APPLICATION_REQUEST,

        /* 19 IPC Process -> IPC Manager */
        RINA_C_IPCM_REGISTER_APPLICATION_RESPONSE,

        /* 20 IPC Manager -> IPC Process */
        RINA_C_IPCM_UNREGISTER_APPLICATION_REQUEST,

        /* 21 IPC Process -> IPC Manager */
        RINA_C_IPCM_UNREGISTER_APPLICATION_RESPONSE,

        /* 22 IPC Manager -> IPC Process */
        RINA_C_IPCM_QUERY_RIB_REQUEST,

        /* 23 IPC Process -> IPC Manager */
        RINA_C_IPCM_QUERY_RIB_RESPONSE,

        /* 24 IPC Process (user space) -> RMT (kernel) */
        RINA_C_RMT_MODIFY_FTE_REQUEST,

        /* 25 IPC Process (user space) -> RMT (kernel) */
        RINA_C_RMT_DUMP_FT_REQUEST,

        /* 26 RMT (kernel) -> IPC Process (user space) */
        RINA_C_RMT_DUMP_FT_REPLY,

        /* 27 NL layer -> IPC Manager */
        RINA_C_IPCM_SOCKET_CLOSED_NOTIFICATION,

        /* 28 IPC Manager -> Kernel (NL Layer) */
        RINA_C_IPCM_IPC_MANAGER_PRESENT,

        /* 29 IPC Process (user space) -> KIPCM*/
        RINA_C_IPCP_CONN_CREATE_REQUEST,

        /* 30 KIPCM -> IPC Process (user space)*/
        RINA_C_IPCP_CONN_CREATE_RESPONSE,

        /* 31 IPC Process (user space) -> KIPCM */
        RINA_C_IPCP_CONN_CREATE_ARRIVED,

        /* 32 KIPCM -> IPC Process (user space)*/
        RINA_C_IPCP_CONN_CREATE_RESULT,

        /* 33 IPC Process (user space) -> KIPCM */
        RINA_C_IPCP_CONN_UPDATE_REQUEST,

        /* 34 KIPCM -> IPC Process (user space)*/
        RINA_C_IPCP_CONN_UPDATE_RESULT,

        /* 35 IPC Process (user space) -> KIPCM */
        RINA_C_IPCP_CONN_DESTROY_REQUEST,

        /* 36 KIPCM -> IPC Process (user space)*/
        RINA_C_IPCP_CONN_DESTROY_RESULT,

        /* 37 IPC Process (user space) -> KIPCM */
        RINA_C_IPCP_SET_POLICY_SET_PARAM_REQUEST,

        /* 38 KIPCM -> IPC Process (user space) */
        RINA_C_IPCP_SET_POLICY_SET_PARAM_RESPONSE,

        /* 39 IPC Process (user space) -> KIPCM */
        RINA_C_IPCP_SELECT_POLICY_SET_REQUEST,

        /* 40 KIPCM -> IPC Process (user space) */
        RINA_C_IPCP_SELECT_POLICY_SET_RESPONSE,

        /* 41, IPC Process (user space) -> KIPCM */
        RINA_C_IPCP_UPDATE_CRYPTO_STATE_REQUEST,

        /* 42 KIPCM -> IPC Process (user space) */
        RINA_C_IPCP_UPDATE_CRYPTO_STATE_RESPONSE,

	/* 43 IPC Process (user space) -> KIPCM */
	RINA_C_IPCP_ADDRESS_CHANGE_REQUEST,

	/* 44, IPC Process (user space) -> K IPCM (kernel) */
	RINA_C_IPCP_ALLOCATE_PORT_REQUEST,

	/* 45, K IPCM (kernel) -> IPC Process (user space) */
	RINA_C_IPCP_ALLOCATE_PORT_RESPONSE,

	/* 46, IPC Process (user space) -> K IPCM (kernel) */
	RINA_C_IPCP_DEALLOCATE_PORT_REQUEST,

	/* 47, K IPCM (kernel) -> IPC Process (user space) */
	RINA_C_IPCP_DEALLOCATE_PORT_RESPONSE,

	/* 48, IPC Process (user space) -> K IPCM (kernel) */
	RINA_C_IPCP_MANAGEMENT_SDU_WRITE_REQUEST,

	/* 49, K IPCM (kernel) -> IPC Process (user space) */
	RINA_C_IPCP_MANAGEMENT_SDU_WRITE_RESPONSE,

	/* 50, K IPCM (kernel) -> IPC Process (user space) */
	RINA_C_IPCP_MANAGEMENT_SDU_READ_NOTIF,

	/* 51, IPC Process (user space) -> K IPCM (kernel) */
	RINA_C_IPCM_CREATE_IPCP_REQUEST,

	/* 52, K IPCM (kernel) -> IPC Process (user space) */
	RINA_C_IPCM_CREATE_IPCP_RESPONSE,

	/* 53, IPC Process (user space) -> K IPCM (kernel) */
	RINA_C_IPCM_DESTROY_IPCP_REQUEST,

	/* 54, K IPCM (kernel) -> IPC Process (user space) */
	RINA_C_IPCM_DESTROY_IPCP_RESPONSE,

	/* 55 */
        RINA_C_MAX,
} msg_type_t;


/* Numtables for kernel <==> IPCM and IPCP Daemon messages exchange. */

extern struct irati_msg_layout irati_ker_numtables[RINA_C_MAX+1];

/* All the messages MUST follow a common format and attribute ordering:
 *   - the first field must be 'rl_msg_t msg_type'
 *   - the second field must be 'irati_msg_port_t src_port'
 *   - the third field must be 'irati_msg_port_t dest_port'
 *   - the fourth field must be 'ipc_process_id_t src_ipcp_id'
 *   - the fifth field must be 'ipc_process_id_t dest_ipcp_id'
 *   - the sixth field must be 'uint32_t event_id'
 *   - then come (if any) all the fields that are not 'struct name' nor
 *     strings ('char *'), nor flow_specs, nor dif_config, nor dtp_config
 *     nor dtcp_config, nor buffers , in whatever order
 *   - then come (if any) all the fields that are 'struct name', in
 *     whatever order
 *   - then come (if any) all the fields that are strings ('char *'), in
 *     whatever order
 *   - then come (if any) all the fields that are fspecs (struct flow_spec),
 *     in whatever order
 *   - then come (if any) all the fields that are dif configs
 *     (struct dif_config), in whatever order
 *   - then come (if any) all the fields that are dtp configs
 *     (struct dtp_config), in whatever order
 *   - then come (if any) all the fields that are dtcp configs
 *     (struct dtcp_config), in whatever order
 *   - then come (if any) all the files that are buffer (struct buffer),
 *     in whatever order
 */

/* TODO 1 RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST */
struct irati_kmsg_ipcm_assign_to_dif {
	irati_msg_t msg_type;
	irati_msg_port_t src_port;
	irati_msg_port_t dest_port;
	ipc_process_id_t src_ipcp_id;
	ipc_process_id_t dest_ipcp_id;
	uint32_t event_id;

	struct name * dif_name;
        string_t * type;
        struct dif_config * dif_config;
} __attribute__((packed));

/* 2 RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE */
/* Uses base response message */

/* TODO 3 RINA_C_IPCM_UPDATE_DIF_CONFIG_REQUEST */
struct irati_kmsg_ipcm_update_config {
	irati_msg_t msg_type;
	irati_msg_port_t src_port;
	irati_msg_port_t dest_port;
	ipc_process_id_t src_ipcp_id;
	ipc_process_id_t dest_ipcp_id;
	uint32_t event_id;

	struct dif_config * dif_config;
} __attribute__((packed));

/* 4 RINA_C_IPCM_UPDATE_DIF_CONFIG_RESPONSE */
/* Uses base response message */

/* 5 RINA_C_IPCM_IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION */
/* 6 RINA_C_IPCM_IPC_PROCESS_DIF_UNREGISTRATION_NOTIFICATION */
struct irati_kmsg_ipcp_dif_reg_not {
	irati_msg_t msg_type;
	irati_msg_port_t src_port;
	irati_msg_port_t dest_port;
	ipc_process_id_t src_ipcp_id;
	ipc_process_id_t dest_ipcp_id;
	uint32_t event_id;

	bool          is_registered;
        struct name * ipcp_name;
        struct name * dif_name;
} __attribute__((packed));

/* 7 TODO RINA_C_IPCM_ENROLL_TO_DIF_REQUEST */

/* 8 RINA_C_IPCM_ENROLL_TO_DIF_RESPONSE */
/* Uses base response message */

/* 9 TODO RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_REQUEST */

/* 10 RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_RESPONSE */
/* Uses base response message  */

/* 11 RINA_C_IPCM_ALLOCATE_FLOW_REQUEST*/
/* TODO 12 RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_ARRIVED */
struct irati_kmsg_ipcm_allocate_flow {
	irati_msg_t msg_type;
	irati_msg_port_t src_port;
	irati_msg_port_t dest_port;
	ipc_process_id_t src_ipcp_id;
	ipc_process_id_t dest_ipcp_id;
	uint32_t event_id;

        struct name      * source;
        struct name      * dest;
        struct name      * dif_name;
        struct flow_spec * fspec;
} __attribute__((packed));

/* 14 RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE */
struct irati_kmsg_ipcp_dif_reg_not {
	irati_msg_t msg_type;
	irati_msg_port_t src_port;
	irati_msg_port_t dest_port;
	ipc_process_id_t src_ipcp_id;
	ipc_process_id_t dest_ipcp_id;
	uint32_t event_id;

        uint8_t    result;
        bool      notify_src;
        port_id_t id;
} __attribute__((packed));

/* 15 RINA_C_IPCM_DEALLOCATE_FLOW_RESPONSE */
/* Uses base netlink message */

/* 18 RINA_C_IPCM_REGISTER_APPLICATION_REQUEST */
struct irati_kmsg_ipcm_reg_app {
	irati_msg_t msg_type;
	irati_msg_port_t src_port;
	irati_msg_port_t dest_port;
	ipc_process_id_t src_ipcp_id;
	ipc_process_id_t dest_ipcp_id;
	uint32_t event_id;

        struct name * app_name;
        struct name * daf_name;
        struct name * dif_name;
} __attribute__((packed));

/* 19 RINA_C_IPCM_REGISTER_APPLICATION_RESPONSE */
/* Uses base response message */

/* 20 RINA_C_IPCM_UNREGISTER_APPLICATION_REQUEST */
struct irati_kmsg_ipcm_unreg_app {
	irati_msg_t msg_type;
	irati_msg_port_t src_port;
	irati_msg_port_t dest_port;
	ipc_process_id_t src_ipcp_id;
	ipc_process_id_t dest_ipcp_id;
	uint32_t event_id;

        struct name * app_name;
        struct name * dif_name;
} __attribute__((packed));

/* 21 RINA_C_IPCM_UNREGISTER_APPLICATION_RESPONSE */
/* Uses base response message */

/* 22 RINA_C_IPCM_QUERY_RIB_REQUEST */
struct irati_kmsg_ipcm_query_rib {
	irati_msg_t msg_type;
	irati_msg_port_t src_port;
	irati_msg_port_t dest_port;
	ipc_process_id_t src_ipcp_id;
	ipc_process_id_t dest_ipcp_id;
	uint32_t event_id;

        uint64_t   object_instance;
        uint32_t   scope;
        string_t * filter;
        string_t * object_class;
        string_t * object_name;
} __attribute__((packed));

/* TODO 23 RINA_C_IPCM_QUERY_RIB_RESPONSE */
struct irati_kmsg_ipcm_query_rib_resp {
	irati_msg_t msg_type;
	irati_msg_port_t src_port;
	irati_msg_port_t dest_port;
	ipc_process_id_t src_ipcp_id;
	ipc_process_id_t dest_ipcp_id;
	uint32_t event_id;

	uint8_t result;
        struct list_head rib_entries;
} __attribute__((packed));

/* 25 RINA_C_RMT_DUMP_FT_REQUEST */
/* Uses base message */

/* 24 RINA_C_RMT_MODIFY_FTE_REQUEST */
/* TODO 26 RINA_C_RMT_DUMP_FT_REPLY */
struct irati_kmsg_rmt_dump_ft {
	irati_msg_t msg_type;
	irati_msg_port_t src_port;
	irati_msg_port_t dest_port;
	ipc_process_id_t src_ipcp_id;
	ipc_process_id_t dest_ipcp_id;
	uint32_t event_id;

	uint8_t result;
        struct list_head pft_entries;
} __attribute__((packed));

/* 27 RINA_C_IPCM_SOCKET_CLOSED_NOTIFICATION */
/* Uses base message */

/* 28 RINA_C_IPCM_IPC_MANAGER_PRESENT */
/* Uses base message */

/* 29 RINA_C_IPCP_CONN_CREATE_REQUEST */
/* TODO 31 RINA_C_IPCP_CONN_CREATE_ARRIVED */
struct irati_kmsg_ipcp_conn_create_arrived {
	irati_msg_t msg_type;
	irati_msg_port_t src_port;
	irati_msg_port_t dest_port;
	ipc_process_id_t src_ipcp_id;
	ipc_process_id_t dest_ipcp_id;
	uint32_t event_id;

        port_id_t            port_id;
        address_t            src_addr;
        address_t            dst_addr;
        cep_id_t             dst_cep;
        qos_id_t             qos_id;
        ipc_process_id_t     flow_user_ipc_process_id;
        struct dtp_config *  dtp_cfg;
        struct dtcp_config * dtcp_cfg;
} __attribute__((packed));

/* 30 RINA_C_IPCP_CONN_CREATE_RESPONSE */
/* 32 RINA_C_IPCP_CONN_CREATE_RESULT */
/* 33 RINA_C_IPCP_CONN_UPDATE_REQUEST */
struct irati_kmsg_ipcp_conn_update {
	irati_msg_t msg_type;
	irati_msg_port_t src_port;
	irati_msg_port_t dest_port;
	ipc_process_id_t src_ipcp_id;
	ipc_process_id_t dest_ipcp_id;
	uint32_t event_id;

        port_id_t port_id;
        cep_id_t  src_cep;
        cep_id_t  dst_cep;
} __attribute__((packed));

/* 37 RINA_C_IPCP_SET_POLICY_SET_PARAM_REQUEST */
struct irati_kmsg_ipcp_select_ps_param {
	irati_msg_t msg_type;
	irati_msg_port_t src_port;
	irati_msg_port_t dest_port;
	ipc_process_id_t src_ipcp_id;
	ipc_process_id_t dest_ipcp_id;
	uint32_t event_id;

        string_t * path;
        string_t * name;
        string_t * value;
} __attribute__((packed));

/* 38 RINA_C_IPCP_SET_POLICY_SET_RESPONSE */
/* Uses base response message */

/* 39 RINA_C_IPCP_SELECT_POLICY_SET_REQUEST */
struct irati_kmsg_ipcp_select_ps {
	irati_msg_t msg_type;
	irati_msg_port_t src_port;
	irati_msg_port_t dest_port;
	ipc_process_id_t src_ipcp_id;
	ipc_process_id_t dest_ipcp_id;
	uint32_t event_id;

        string_t * path;
        string_t * name;
} __attribute__((packed));

/* 40 RINA_C_IPCP_SELECT_POLICY_SET_RESPONSE */
/* Uses base response message */

/* TODO 41 RINA_C_IPCP_UPDATE_CRYPTO_STATE_REQUEST */
struct irati_kmsg_ipcp_address_change {
	irati_msg_t msg_type;
	irati_msg_port_t src_port;
	irati_msg_port_t dest_port;
	ipc_process_id_t src_ipcp_id;
	ipc_process_id_t dest_ipcp_id;
	uint32_t event_id;

	port_id_t port_id;
	struct sdup_crypto_state * state;
} __attribute__((packed));

/* 43 RINA_C_IPCP_ADDRESS_CHANGE_REQUEST */
struct irati_kmsg_ipcp_address_change {
	irati_msg_t msg_type;
	irati_msg_port_t src_port;
	irati_msg_port_t dest_port;
	ipc_process_id_t src_ipcp_id;
	ipc_process_id_t dest_ipcp_id;
	uint32_t event_id;

	address_t new_address;
	address_t old_address;
	timeout_t use_new_timeout;
	timeout_t deprecate_old_timeout;
} __attribute__((packed));

/* 44 RINA_C_IPCP_ALLOCATE_PORT_REQUEST */
struct irati_kmsg_ipcp_select_ps_param {
	irati_msg_t msg_type;
	irati_msg_port_t src_port;
	irati_msg_port_t dest_port;
	ipc_process_id_t src_ipcp_id;
	ipc_process_id_t dest_ipcp_id;
	uint32_t event_id;

        struct name * app_name;
} __attribute__((packed));

/* 13 RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_RESULT */
/* 15 RINA_C_IPCM_DEALLOCATE_FLOW_REQUEST */
/* 17 RINA_C_IPCM_FLOW_DEALLOCATED_NOTIFICATION */
/* 34 RINA_C_IPCP_CONN_UPDATE_RESPONSE */
/* 35 RINA_C_IPCP_CONN_DESTROY_REQUEST */
/* 36 RINA_C_IPCP_CONN_DESTROY_RESPONSE */
/* 42 RINA_C_IPCP_UPDATE_CRYPTO_STATE_RESPONSE */
/* 45 RINA_C_IPCP_ALLOCATE_PORT_RESPONSE */
/* 46 RINA_C_IPCP_DEALLOCATE_PORT_REQUEST */
/* 47 RINA_C_IPCP_DEALLOCATE_PORT_RESPONSE */
struct irati_kmsg_multi_msg {
	irati_msg_t msg_type;
	irati_msg_port_t src_port;
	irati_msg_port_t dest_port;
	ipc_process_id_t src_ipcp_id;
	ipc_process_id_t dest_ipcp_id;
	uint32_t event_id;

	uint8_t   result;
        port_id_t port_id;
        cep_id_t  cep_id;
} __attribute__((packed));

/* 48 RINA_C_IPCP_MANAGEMENT_SDU_WRITE_REQUEST */
/* 50 RINA_C_IPCP_MANAGEMENT_SDU_READ_NOTIF */
struct irati_kmsg_ipcp_mgmt_sdu{
	irati_msg_t msg_type;
	irati_msg_port_t src_port;
	irati_msg_port_t dest_port;
	ipc_process_id_t src_ipcp_id;
	ipc_process_id_t dest_ipcp_id;
	uint32_t event_id;

	port_id_t port_id;
        struct buffer * sdu;
} __attribute__((packed));

/* 49 RINA_C_IPCP_MANAGEMENT_SDU_WRITE_RESPONSE */
/* Uses base response message */

/* 51 RINA_C_IPCM_CREATE_IPCP_REQUEST */
struct irati_kmsg_ipcm_create_ipcp {
	irati_msg_t msg_type;
	irati_msg_port_t src_port;
	irati_msg_port_t dest_port;
	ipc_process_id_t src_ipcp_id;
	ipc_process_id_t dest_ipcp_id;
	uint32_t event_id;

	ipc_process_id_t ipcp_id;
	irati_msg_port_t irati_port_id;
	struct name * ipcp_name;
	string_t * dif_type;
} __attribute__((packed));

/* 52, K IPCM (kernel) -> IPC Process (user space) */
/* Uses base response message */

/* 53 RINA_C_IPCM_DESTROY_IPCP_REQUEST */
struct irati_kmsg_ipcm_destroy_ipcp {
	irati_msg_t msg_type;
	irati_msg_port_t src_port;
	irati_msg_port_t dest_port;
	ipc_process_id_t src_ipcp_id;
	ipc_process_id_t dest_ipcp_id;
	uint32_t event_id;

	ipc_process_id_t ipcp_id;
} __attribute__((packed));

/* 54, K IPCM (kernel) -> IPC Process (user space) */
/* Uses base response message */

#endif /* IRATI_KERN_MSG_H */
