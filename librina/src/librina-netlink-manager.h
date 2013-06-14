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

/*
 * librina-netlink-manager.cc
 *
 *  Created on: 12/06/2013
 *      Author: eduardgrasa
 */

#ifndef LIBRINA_NETLINK_MANAGER_H
#define LIBRINA_NETLINK_MANAGER_H

#include "exceptions.h"
#include "patterns.h"
#include "librina-common.h"
#include <netlink/netlink.h>

namespace rina{

enum RINANetlinkOperationCode{
        RINA_C_UNSPEC, /* Unespecified operation */
        RINA_C_APP_ALLOCATE_FLOW_REQUEST, /* Allocate flow request, Application -> IPC Manager */
        RINA_C_APP_ALLOCATE_FLOW_REQUEST_RESULT, /* Response to an application allocate flow request, IPC Manager -> Application */
        RINA_C_APP_ALLOCATE_FLOW_REQUEST_ARRIVED, /* Allocate flow request from a remote application, IPC Process -> Application */
        RINA_C_APP_ALLOCATE_FLOW_RESPONSE, /* Allocate flow response to an allocate request arrived operation, Application -> IPC Process */
        RINA_C_APP_DEALLOCATE_FLOW_REQUEST, /* Application -> IPC Process */
        RINA_C_APP_DEALLOCATE_FLOW_RESPONSE, /* IPC Process -> Application */
        RINA_C_APP_FLOW_DEALLOCATED_NOTIFICATION, /* IPC Process -> Application, flow deallocated without the application having requested it */
        RINA_C_APP_REGISTER_APPLICATION_REQUEST, /* Application -> IPC Manager */
        RINA_C_APP_REGISTER_APPLICATION_RESPONSE, /* IPC Manager -> Application */
        RINA_C_APP_UNREGISTER_APPLICATION_REQUEST, /* Application -> IPC Manager */
        RINA_C_APP_UNREGISTER_APPLICATION_RESPONSE, /* IPC Manager -> Application */
        RINA_C_APP_APPLICATION_REGISTRATION_CANCELED_NOTIFICATION, /* IPC Manager -> Application, application unregistered without the application having requested it */
        RINA_C_APP_GET_DIF_PROPERTIES_REQUEST, /* Application -> IPC Manager */
        RINA_C_APP_GET_DIF_PROPERTIES_RESPONSE, /* IPC Manager -> Application */
        RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST, /* IPC Manager -> IPC Process */
        RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE, /* IPC Process -> IPC Manager */
        RINA_C_IPCM_IPC_PROCESS_REGISTERED_TO_DIF_NOTIFICATION, /* IPC Manager -> IPC Process */
        RINA_C_IPCM_IPC_PROCESS_UNREGISTERED_FROM_DIF_NOTIFICATION, /* IPC Manager -> IPC Process */
        RINA_C_IPCM_ENROLL_TO_DIF_REQUEST, /* IPC Manager -> IPC Process */
        RINA_C_IPCM_ENROLL_TO_DIF_RESPONSE, /* IPC Process -> IPC Manager */
        RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_REQUEST, /* IPC Manager -> IPC Process */
        RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_RESPONSE, /* IPC Process -> IPC Manager */
        RINA_C_IPCM_ALLOCATE_FLOW_REQUEST, /* IPC Manager -> IPC Process */
        RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE, /* IPC Process -> IPC Manager */
        RINA_C_IPCM_QUERY_RIB_REQUEST, /* IPC Manager -> IPC Process */
        RINA_C_IPCM_QUERY_RIB_RESPONSE, /* IPC Process -> IPC Manager */
        RINA_C_RMT_ADD_FTE_REQUEST, /* IPC Process (user space) -> RMT (kernel) */
        RINA_C_RMT_DELETE_FTE_REQUEST, /* IPC Process (user space) -> RMT (kernel) */
        RINA_C_RMT_DUMP_FT_REQUEST, /* IPC Process (user space) -> RMT (kernel) */
        RINA_C_RMT_DUMP_FT_REPLY, /* RMT (kernel) -> IPC Process (user space) */
        __RINA_C_MAX,
 };

/**
 * Base class for Netlink messages to be sent or received
 */
class BaseNetlinkMessage{
	/** The port id of the source of the Netlink message */
	unsigned int sourcePortId;

	/** The port id of the destination of the Netlink message */
	unsigned int destPortId;

	/** The message sequence number */
	unsigned int sequenceNumber;

	/** The operation code */
	RINANetlinkOperationCode operationCode;

public:
	BaseNetlinkMessage(RINANetlinkOperationCode operationCode);
	virtual ~BaseNetlinkMessage();
	unsigned int getDestPortId() const;
	void setDestPortId(unsigned int destPortId);
	unsigned int getSequenceNumber() const;
	void setSequenceNumber(unsigned int sequenceNumber);
	unsigned int getSourcePortId() const;
	void setSourcePortId(unsigned int sourcePortId);
	RINANetlinkOperationCode getOperationCode() const;
};

/**
 * An allocate flow request message, exchanged between an Application Process
 * and the IPC Manager.
 */
class AppAllocateFlowRequestMessage: public BaseNetlinkMessage{

	/** The source application name */
	ApplicationProcessNamingInformation sourceAppName;

	/** The destination application name */
	ApplicationProcessNamingInformation destAppName;

	/** The characteristics of the flow */
	FlowSpecification flowSpecification;

public:
	AppAllocateFlowRequestMessage();
	const ApplicationProcessNamingInformation& getDestAppName() const;
	void setDestAppName(
			const ApplicationProcessNamingInformation& destAppName);
	const FlowSpecification& getFlowSpecification() const;
	void setFlowSpecification(const FlowSpecification& flowSpecification);
	const ApplicationProcessNamingInformation& getSourceAppName() const;
	void setSourceAppName(
			const ApplicationProcessNamingInformation& sourceAppName);
};

/**
 * Manages the creation, destruction and usage of a Netlink socket with the OS
 * process PID. The socket will be utilized by the OS process using RINA to
 * communicate with other OS processes in user space or the Kernel.
 */
class NetlinkManager{

	/** The address where the netlink socket will be bound */
	unsigned int localPort;

	/** The netlink socket structure */
	nl_sock *socket;

	/** Creates the Netlink socket and binds it to the netlinkPid */
	void initialize();
public:
	/**
	 * Creates an instance of a Netlink socket and binds it to the local port
	 * whose number is the same as the OS process PID
	 */
	NetlinkManager();

	/**
	 * Creates an instance of a Netlink socket and binds it to the specified
	 * local port
	 */
	NetlinkManager(unsigned int localPort);

	/**
	 * Closes the Netlink socket
	 */
	~NetlinkManager();

	void sendMessage(const AppAllocateFlowRequestMessage& message);

	AppAllocateFlowRequestMessage *  getMessage();
};
}

#endif /* LIBRINA_NETLINK_MANAGER_H_ */
