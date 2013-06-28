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
class BaseNetlinkMessage: public StringConvertable{
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

	/** True if this is a request message */
	bool requestMessage;

	/** True if this is a response message */
	bool responseMessage;

	/** True if this is a notificaiton message */
	bool notificationMessage;

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
	bool isNotificationMessage() const;
	void setNotificationMessage(bool notificationMessage);
	void setOperationCode(RINANetlinkOperationCode operationCode);
	bool isRequestMessage() const;
	void setRequestMessage(bool requestMessage);
	bool isResponseMessage() const;
	void setResponseMessage(bool responseMessage);
	std::string toString();
};

/**
 * Models a Netlink notification message. Transforms the
 * Messate to an IPC Event
 */
class NetlinkRequestOrNotificationMessage: public BaseNetlinkMessage{
public:
	NetlinkRequestOrNotificationMessage(RINANetlinkOperationCode operationCode):
		BaseNetlinkMessage(operationCode){
	}
	virtual ~NetlinkRequestOrNotificationMessage(){}
	virtual IPCEvent* toIPCEvent() = 0;
};

/**
 * An allocate flow request message, exchanged between an Application Process
 * and the IPC Manager.
 */
class AppAllocateFlowRequestMessage: public NetlinkRequestOrNotificationMessage{

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
	IPCEvent* toIPCEvent();
};

/**
 * Response to an application allocate flow request, IPC Manager -> Application
 */
class AppAllocateFlowRequestResultMessage:
		public BaseNetlinkMessage{

	/** The application that requested the flow allocation */
	ApplicationProcessNamingInformation sourceAppName;

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
	 * The DIF where the flow has been allocated
	 */
	ApplicationProcessNamingInformation difName;

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
	unsigned int getIpcProcessPortId() const;
	void setIpcProcessPortId(unsigned int ipcProcessPortId);
	int getPortId() const;
	void setPortId(int portId);
	const ApplicationProcessNamingInformation& getDifName() const;
	void setDifName(const ApplicationProcessNamingInformation& difName);
	const ApplicationProcessNamingInformation& getSourceAppName() const;
	void setSourceAppName(
			const ApplicationProcessNamingInformation& sourceAppName);
};

/**
 * Allocate flow request from a remote application, IPC Process -> Application
 */
class AppAllocateFlowRequestArrivedMessage:
		public NetlinkRequestOrNotificationMessage {

	/** The source application name */
	ApplicationProcessNamingInformation sourceAppName;

	/** The destination application name */
	ApplicationProcessNamingInformation destAppName;

	/** The characteristics of the flow */
	FlowSpecification flowSpecification;

	/** The portId reserved for the flow */
	int portId;

	/** The dif Name */
	ApplicationProcessNamingInformation difName;

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
	const ApplicationProcessNamingInformation& getDifName() const;
	void setDifName(const ApplicationProcessNamingInformation& difName);
	IPCEvent* toIPCEvent();
};

/**
 * Allocate flow response to an allocate request arrived operation,
 * Application -> IPC Process
 */
class AppAllocateFlowResponseMessage: public BaseNetlinkMessage{

	/** The name of the DIF where the flow is being allocated */
	ApplicationProcessNamingInformation difName;

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
	const ApplicationProcessNamingInformation& getDifName() const;
	void setDifName(const ApplicationProcessNamingInformation& difName);
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
class AppDeallocateFlowRequestMessage:
		public NetlinkRequestOrNotificationMessage {

	/** The id of the flow to be deallocated */
	int portId;

	/** The name of the DIF where the flow is being allocated */
	ApplicationProcessNamingInformation difName;

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
	const ApplicationProcessNamingInformation& getDifName() const;
	void setDifName(const ApplicationProcessNamingInformation& difName);
	IPCEvent* toIPCEvent();
};

/**
 * Response by the IPC Process to the flow deallocation request
 */
class AppDeallocateFlowResponseMessage: public BaseNetlinkMessage {

	/** 0 if the operation was successful, an error code otherwise */
	int result;

	/** If there was an error, optional explanation providing more details */
	std::string errorDescription;

	/**
	 * The name of the applicaiton that requested the flow deallocation
	 */
	ApplicationProcessNamingInformation applicationName;

public:
	AppDeallocateFlowResponseMessage();
	const ApplicationProcessNamingInformation& getApplicationName() const;
	void setApplicationName(
			const ApplicationProcessNamingInformation& applicationName);
	const std::string& getErrorDescription() const;
	void setErrorDescription(const std::string& errorDescription);
	int getResult() const;
	void setResult(int result);
};

/**
 * IPC Process -> Application, flow deallocated without the application having
 *  requested it
 */
class AppFlowDeallocatedNotificationMessage:
		public NetlinkRequestOrNotificationMessage {

	/** The portId of the flow that has been deallocated */
	int portId;

	/** A number identifying a reason why the flow has been deallocated */
	int code;

	/** An optional explanation of why the flow has been deallocated */
	std::string reason;

	/**
	 * The name of the applicaiton that was using the flow
	 */
	ApplicationProcessNamingInformation applicationName;

	/**
	 * The name of the applicaiton that requested the flow deallocation
	 */
	ApplicationProcessNamingInformation difName;

public:
	AppFlowDeallocatedNotificationMessage();
	int getCode() const;
	void setCode(int code);
	int getPortId() const;
	void setPortId(int portId);
	const std::string& getReason() const;
	void setReason(const std::string& reason);
	const ApplicationProcessNamingInformation& getApplicationName() const;
	void setApplicationName(
			const ApplicationProcessNamingInformation& applicationName);
	const ApplicationProcessNamingInformation& getDifName() const;
	void setDifName(const ApplicationProcessNamingInformation& difName);
	IPCEvent* toIPCEvent();
};

/**
 * Invoked by the application when it wants to register an application
 * to a DIF. Application -> IPC Manager
 */
class AppRegisterApplicationRequestMessage:
		public NetlinkRequestOrNotificationMessage {

	/** The name of the application to be registered */
	ApplicationProcessNamingInformation applicationName;

	/** The DIF name where the application wants to register */
	ApplicationProcessNamingInformation difName;

public:
	AppRegisterApplicationRequestMessage();
	const ApplicationProcessNamingInformation& getApplicationName() const;
	void setApplicationName(
			const ApplicationProcessNamingInformation& applicationName);
	const ApplicationProcessNamingInformation& getDifName() const;
	void setDifName(const ApplicationProcessNamingInformation& difName);
	IPCEvent* toIPCEvent();
};

/**
 * Response of the IPC Manager to an application registration request.
 * IPC Manager -> Application
 */
class AppRegisterApplicationResponseMessage: public BaseNetlinkMessage {

	/** The DIF name where the application wants to register */
	ApplicationProcessNamingInformation applicationName;

	/**
	 * Result of the operation. 0 indicates success, a negative value an
	 * error code.
	 */
	int result;

	/**
	 * If the application registration didn't succeed, this field may provide
	 * further detail
	 */
	std::string errorDescription;

	/** The DIF name where the application wants to register */
	ApplicationProcessNamingInformation difName;

	/**
	 * The id of the Netlink port to be used to send messages to the IPC
	 * Process that registered the application. In the case of shim IPC
	 * Processes (they all reside in the kernel), the value of this field will
	 * always be 0.
	 */
	unsigned int ipcProcessPortId;

public:
	AppRegisterApplicationResponseMessage();
	const ApplicationProcessNamingInformation& getApplicationName() const;
	void setApplicationName(
			const ApplicationProcessNamingInformation& applicationName);
	const std::string& getErrorDescription() const;
	void setErrorDescription(const std::string& errorDescription);
	const ApplicationProcessNamingInformation& getDifName() const;
	void setDifName(const ApplicationProcessNamingInformation& difName);
	unsigned int getIpcProcessPortId() const;
	void setIpcProcessPortId(unsigned int ipcProcessPortId);
	int getResult() const;
	void setResult(int result);
};

}

#endif /* NETLINK_MESSAGES_H_ */
