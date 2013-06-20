/*
 * NetLink support
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
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

#ifndef RINA_NETLINK_SUPPORT
#define RINA_NETLINK_SUPPORT

#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <net/netlink.h>
#include <linux/genetlink.h>
#include <net/genetlink.h>
#include <linux/skbuff.h>


#define NETLINK_RINA "nlrina"
#define CALLBACKS_TABLE_NAME "cb_table"
#define CALLBACKS_TABLE_SIZE 32

/* este es un comentario largo y a ver si em lo corta a las 80 letras porque si
 * no me vuelvo loco*/

enum rina_nl_operation_code {
	/* Unespecified operation */
	RINA_C_UNSPEC = 20, 
	
	/* Allocate flow request, Application -> IPC Manager */
	RINA_C_APP_ALLOCATE_FLOW_REQUEST = 21, 
	
	/* Response to an application allocate flow request, 
	 * IPC Manager -> Application */
	RINA_C_APP_ALLOCATE_FLOW_REQUEST_RESULT = 22, 
	
	/* Allocate flow request from a remote application, 
	 * IPC Process -> Application */
	RINA_C_APP_ALLOCATE_FLOW_REQUEST_ARRIVED = 23, 
	
	/* Allocate flow response to an allocate request arrived operation, 
	 * Application -> IPC Process */
	RINA_C_APP_ALLOCATE_FLOW_RESPONSE = 24, 
	
	/* Application -> IPC Process */
	RINA_C_APP_DEALLOCATE_FLOW_REQUEST = 25, 
	
	/* IPC Process -> Application */
	RINA_C_APP_DEALLOCATE_FLOW_RESPONSE = 26, 
	
	/* IPC Process -> Application, flow deallocated without the application 
	 * having requested it */
	RINA_C_APP_FLOW_DEALLOCATED_NOTIFICATION = 27, 
	
	/* Application -> IPC Manager */
	RINA_C_APP_REGISTER_APPLICATION_REQUEST = 28, 
	
	/* IPC Manager -> Application */
	RINA_C_APP_REGISTER_APPLICATION_RESPONSE = 29, 
	
	/* Application -> IPC Manager */
	RINA_C_APP_UNREGISTER_APPLICATION_REQUEST = 30, 
	
	/* IPC Manager -> Application */
	RINA_C_APP_UNREGISTER_APPLICATION_RESPONSE = 31, 
	
	/* IPC Manager -> Application, application unregistered without the application
	 * having requested it */
	RINA_C_APP_APPLICATION_REGISTRATION_CANCELED_NOTIFICATION = 32, 
	
	/* Application -> IPC Manager */
	RINA_C_APP_GET_DIF_PROPERTIES_REQUEST = 33, 
	
	/* IPC Manager -> Application */
	RINA_C_APP_GET_DIF_PROPERTIES_RESPONSE = 34, 
	
	/* IPC Manager -> IPC Process */
	RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST = 35, 
	
	/* IPC Process -> IPC Manager */
	RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE = 36, 
	
	/* IPC Manager -> IPC Process */
	RINA_C_IPCM_IPC_PROCESS_REGISTERED_TO_DIF_NOTIFICATION = 37, 
	
	/* IPC Manager -> IPC Process */
	RINA_C_IPCM_IPC_PROCESS_UNREGISTERED_FROM_DIF_NOTIFICATION = 38, 
	
	/* IPC Manager -> IPC Process */
	RINA_C_IPCM_ENROLL_TO_DIF_REQUEST = 39, 
	
	/* IPC Process -> IPC Manager */
	RINA_C_IPCM_ENROLL_TO_DIF_RESPONSE = 40, 
	
	/* IPC Manager -> IPC Process */
	RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_REQUEST = 41, 
	
	/* IPC Process -> IPC Manager */
	RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_RESPONSE = 42, 
	
	/* IPC Manager -> IPC Process */
	RINA_C_IPCM_ALLOCATE_FLOW_REQUEST = 43, 
	
	/* IPC Process -> IPC Manager */
	RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE = 44, 
	
	/* IPC Manager -> IPC Process */
	RINA_C_IPCM_QUERY_RIB_REQUEST = 45, 
	
	/* IPC Process -> IPC Manager */
	RINA_C_IPCM_QUERY_RIB_RESPONSE = 46, 
	
	/* IPC Process (user space) -> RMT (kernel) */
	RINA_C_RMT_ADD_FTE_REQUEST = 47, 
	
	/* IPC Process (user space) -> RMT (kernel) */
	RINA_C_RMT_DELETE_FTE_REQUEST = 48, 
	
	/* IPC Process (user space) -> RMT (kernel) */
	RINA_C_RMT_DUMP_FT_REQUEST = 49, 
	
	/* RMT (kernel) -> IPC Process (user space) */
	RINA_C_RMT_DUMP_FT_REPLY = 50,

	/*  TODO: Just for test */
	NETLINK_RINA_C_ECHO = 51,
	
	__NETLINK_RINA_C_MAX,
};

/* attributes */
enum {
	NETLINL_RINA_A_UNSPEC,
	NETLINK_RINA_A_MSG,
	__NETLINK_RINA_A_MAX,
};

#define NETLINK_RINA_A_MAX (__NETLINK_RINA_A_MAX - 1)
#define NETLINK_RINA_C_MAX (__NETLINK_RINA_C_MAX - 1)
#define NETLINK_RINA_CB_INDEX(value) value - RINA_C_UNSPEC
#define NETLINK_RINA_CB_SIZE (NETLINK_RINA_C_MAX - RINA_C_UNSPEC)

/*  Table to collect callbacks */
typedef int (*message_handler_t)(struct sk_buff *, struct genl_info *);

int  rina_netlink_init(void);
void rina_netlink_exit(void);

#endif
