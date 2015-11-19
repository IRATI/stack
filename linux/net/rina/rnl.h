/*
 * RNL (RINA NetLink Layer)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
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

#ifndef RINA_RNL_H
#define RINA_RNL_H

#include <linux/netlink.h>
#include <linux/genetlink.h>
#include <linux/skbuff.h>
#include <linux/types.h>
#include <net/genetlink.h>
#include <net/netlink.h>
#include <net/sock.h>

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

        /* 29 FIXME: Need to be logically moved after flow
         * allocation messages. Put here in order to maintain
         * compatibility with MS7 version of user space*/
        /* IPC Process (user space) -> KIPCM*/
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

        /* 43 */
        RINA_C_MAX,
} msg_type_t;

typedef uint32_t rnl_sn_t;
typedef uint32_t rnl_port_t;

int              rnl_init(void);
void             rnl_exit(void);

struct rnl_set;

struct rnl_set * rnl_set_create(void);
int              rnl_set_destroy(struct rnl_set * set);

typedef int (* message_handler_cb)(void *             data,
                                   struct sk_buff *   buff,
                                   struct genl_info * info);
int              rnl_handler_register(struct rnl_set *   set,
                                      msg_type_t         msg_type,
                                      void *             data,
                                      message_handler_cb handler);
int              rnl_handler_unregister(struct rnl_set * set,
                                        msg_type_t       msg_type);

int              rnl_set_register(struct rnl_set * set);
int              rnl_set_unregister(struct rnl_set * set);
rnl_sn_t         rnl_get_next_seqn(struct rnl_set * set);
rnl_port_t       rnl_get_ipc_manager_port(void);
void             rnl_set_ipc_manager_port(rnl_port_t port);

#endif
