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
 * netlink-messages.h
 *
 *  Created on: 12/06/2013
 *      Author: eduardgrasa
 */

#ifndef NETLINK_MESSAGES_H
#define NETLINK_MESSAGES_H

#include "librina-common.h"

namespace rina{

enum RINANetlinkOperationCode{
        RINA_C_UNSPEC = 20, /* Unespecified operation */
        RINA_C_APP_ALLOCATE_FLOW_REQUEST = 21, /* Allocate flow request, Application -> IPC Manager */
        RINA_C_APP_ALLOCATE_FLOW_REQUEST_RESULT = 22, /* Response to an application allocate flow request, IPC Manager -> Application */
        RINA_C_APP_ALLOCATE_FLOW_REQUEST_ARRIVED = 23, /* Allocate flow request from a remote application, IPC Process -> Application */
        RINA_C_APP_ALLOCATE_FLOW_RESPONSE = 24, /* Allocate flow response to an allocate request arrived operation, Application -> IPC Process */
        RINA_C_APP_DEALLOCATE_FLOW_REQUEST = 25, /* Application -> IPC Process */
        RINA_C_APP_DEALLOCATE_FLOW_RESPONSE = 26, /* IPC Process -> Application */
        RINA_C_APP_FLOW_DEALLOCATED_NOTIFICATION = 27, /* IPC Process -> Application, flow deallocated without the application having requested it */
        RINA_C_APP_REGISTER_APPLICATION_REQUEST = 28, /* Application -> IPC Manager */
        RINA_C_APP_REGISTER_APPLICATION_RESPONSE = 29, /* IPC Manager -> Application */
        RINA_C_APP_UNREGISTER_APPLICATION_REQUEST = 30, /* Application -> IPC Manager */
        RINA_C_APP_UNREGISTER_APPLICATION_RESPONSE = 31, /* IPC Manager -> Application */
        RINA_C_APP_APPLICATION_REGISTRATION_CANCELED_NOTIFICATION = 32, /* IPC Manager -> Application, application unregistered without the application having requested it */
        RINA_C_APP_GET_DIF_PROPERTIES_REQUEST = 33, /* Application -> IPC Manager */
        RINA_C_APP_GET_DIF_PROPERTIES_RESPONSE = 34, /* IPC Manager -> Application */
        RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST = 35, /* IPC Manager -> IPC Process */
        RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE = 36, /* IPC Process -> IPC Manager */
        RINA_C_IPCM_IPC_PROCESS_REGISTERED_TO_DIF_NOTIFICATION = 37, /* IPC Manager -> IPC Process */
        RINA_C_IPCM_IPC_PROCESS_UNREGISTERED_FROM_DIF_NOTIFICATION = 38, /* IPC Manager -> IPC Process */
        RINA_C_IPCM_ENROLL_TO_DIF_REQUEST = 39, /* IPC Manager -> IPC Process */
        RINA_C_IPCM_ENROLL_TO_DIF_RESPONSE = 40, /* IPC Process -> IPC Manager */
        RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_REQUEST = 41, /* IPC Manager -> IPC Process */
        RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_RESPONSE = 42, /* IPC Process -> IPC Manager */
        RINA_C_IPCM_ALLOCATE_FLOW_REQUEST = 43, /* IPC Manager -> IPC Process */
        RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE = 44, /* IPC Process -> IPC Manager */
        RINA_C_IPCM_QUERY_RIB_REQUEST = 45, /* IPC Manager -> IPC Process */
        RINA_C_IPCM_QUERY_RIB_RESPONSE = 46, /* IPC Process -> IPC Manager */
        RINA_C_RMT_ADD_FTE_REQUEST = 47, /* IPC Process (user space) -> RMT (kernel) */
        RINA_C_RMT_DELETE_FTE_REQUEST = 48, /* IPC Process (user space) -> RMT (kernel) */
        RINA_C_RMT_DUMP_FT_REQUEST = 49, /* IPC Process (user space) -> RMT (kernel) */
        RINA_C_RMT_DUMP_FT_REPLY = 50, /* RMT (kernel) -> IPC Process (user space) */
        __RINA_C_MAX,
 };

/**
 * Base class for Netlink messages to be sent or received
 */
class BaseNetlinkMessage{
	/**
	 * The identity of the Generic RINA Netlink family - dynamically allocated
	 * by the Netlink controller
	 */
	int family;

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
	int getFamily() const;
	void setFamily(int family);
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
 * Response to an application allocate flow request, IPC Manager -> Application
 */
class AppAllocateFlowRequestResultMessage: public BaseNetlinkMessage{

	/**
	 * The port-id assigned to the flow, or error code if the value is
	 * negative
	 */
	int portId;

	/**
	 * If the flow allocation didn't succeed, this field provides a description
	 * of the error
	 */
	std::string errorDescription;

	/**
	 * The id of the IPC Process that has allocated the flow
	 */
	unsigned int ipcProcessId;

	/**
	 * The id of the Netlink port to be used to send messages to the IPC Process
	 * that allocated the flow. In the case of shim IPC Processes (they all
	 * reside in the kernel), the value of this field will always be 0.
	 */
	unsigned int ipcProcessPortId;

public:
	AppAllocateFlowRequestResultMessage();
	const std::string& getErrorDescription() const;
	void setErrorDescription(const std::string& errorDescription);
	unsigned int getIpcProcessId() const;
	void setIpcProcessId(unsigned int ipcProcessId);
	unsigned int getIpcProcessPortId() const;
	void setIpcProcessPortId(unsigned int ipcProcessPortId);
	int getPortId() const;
	void setPortId(int portId);
};

/**
 * Allocate flow request from a remote application, IPC Process -> Application
 */
class AppAllocateFlowRequestArrivedMessage: public BaseNetlinkMessage{

	/** The source application name */
	ApplicationProcessNamingInformation sourceAppName;

	/** The destination application name */
	ApplicationProcessNamingInformation destAppName;

	/** The characteristics of the flow */
	FlowSpecification flowSpecification;

	/** The portId reserved for the flow */
	int portId;

public:
	AppAllocateFlowRequestArrivedMessage();
	const ApplicationProcessNamingInformation& getDestAppName() const;
	void setDestAppName(
			const ApplicationProcessNamingInformation& destAppName);
	const FlowSpecification& getFlowSpecification() const;
	void setFlowSpecification(const FlowSpecification& flowSpecification);
	const ApplicationProcessNamingInformation& getSourceAppName() const;
	void setSourceAppName(
			const ApplicationProcessNamingInformation& sourceAppName);
	int getPortId() const;
	void setPortId(int portId);
};

/**
 * Allocate flow response to an allocate request arrived operation,
 * Application -> IPC Process
 */
class AppAllocateFlowResponseMessage: public BaseNetlinkMessage{

	/** True if the application accepts the flow, false otherwise */
	bool accept;

	/**
	 * If the flow was denied and the application wishes to do so, it
	 * can provide an explanation of why this decision was taken
	 */
	std::string denyReason;

	/**
	 * If the flow was denied, this field controls wether the application
	 * wants the IPC Process to reply to the source or not
	 */
	bool notifySource;

public:
	AppAllocateFlowResponseMessage();
	bool isAccept() const;
	void setAccept(bool accept);
	const std::string& getDenyReason() const;
	void setDenyReason(const std::string& denyReason);
	bool isNotifySource() const;
	void setNotifySource(bool notifySource);
};

/**
 * Issued by the application process when it whishes to deallocate a flow.
 * Application -> IPC Process
 */
class AppDeallocateFlowRequestMessage: public BaseNetlinkMessage {

	/** The id of the flow to be deallocated */
	int portId;

	/**
	 * The name of the applicaiton requesting the flow deallocation,
	 * to verify it is allowed to do so
	 */
	ApplicationProcessNamingInformation applicationName;

public:
	AppDeallocateFlowRequestMessage();
	const ApplicationProcessNamingInformation& getApplicationName() const;
	void setApplicationName(
			const ApplicationProcessNamingInformation& applicationName);
	int getPortId() const;
	void setPortId(int portId);
};

/**
 * Response by the IPC Process to the flow deallocation request
 */
class AppDeallocateFlowResponseMessage: public BaseNetlinkMessage {

	/** 0 if the operation was successful, an error code otherwise */
	int result;

	/** If there was an error, optional explanation providing more details */
	std::string errorDescription;

public:
	AppDeallocateFlowResponseMessage();
	const std::string& getErrorDescription() const;
	void setErrorDescription(const std::string& errorDescription);
	int getResult() const;
	void setResult(int result);
};

}

#endif /* NETLINK_MESSAGES_H_ */
