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
	RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST, /* IPC Manager -> IPC Process */
	RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE, /* IPC Process -> IPC Manager */
        RINA_C_IPCM_UPDATE_DIF_CONFIG_REQUEST, /* IPC Manager -> IPC Process */
        RINA_C_IPCM_UPDATE_DIF_CONFIG_RESPONSE, /* IPC Process -> IPC Manager */
	RINA_C_IPCM_IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION, /* IPC Manager -> IPC Process */
	RINA_C_IPCM_IPC_PROCESS_DIF_UNREGISTRATION_NOTIFICATION, /* IPC Manager -> IPC Process */
	RINA_C_IPCM_ENROLL_TO_DIF_REQUEST, /* TODO IPC Manager -> IPC Process */
	RINA_C_IPCM_ENROLL_TO_DIF_RESPONSE, /* TODO IPC Process -> IPC Manager */
	RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_REQUEST, /* TODO IPC Manager -> IPC Process */
	RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_RESPONSE, /* TODO IPC Process -> IPC Manager */
	RINA_C_IPCM_ALLOCATE_FLOW_REQUEST, /* IPC Manager -> IPC Process */
	RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_ARRIVED, /* Allocate flow request from a remote application, IPC Process -> IPC Manager */
	RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_RESULT, /* IPC Process -> IPC Manager */
	RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE, /* IPC Manager -> IPC Process */
	RINA_C_IPCM_DEALLOCATE_FLOW_REQUEST, /* IPC Manager -> IPC Process */
	RINA_C_IPCM_DEALLOCATE_FLOW_RESPONSE, /* IPC Process -> IPC Manager */
	RINA_C_IPCM_FLOW_DEALLOCATED_NOTIFICATION, /* IPC Process -> IPC Manager, flow deallocated without the application having requested it */
	RINA_C_IPCM_REGISTER_APPLICATION_REQUEST, /*IPC Manager -> IPC Process */
	RINA_C_IPCM_REGISTER_APPLICATION_RESPONSE, /*IPC Process -> IPC Manager */
	RINA_C_IPCM_UNREGISTER_APPLICATION_REQUEST, /* IPC Manager -> IPC Process */
	RINA_C_IPCM_UNREGISTER_APPLICATION_RESPONSE, /* IPC Process -> IPC Manager */
	RINA_C_IPCM_QUERY_RIB_REQUEST, /* IPC Manager -> IPC Process */
	RINA_C_IPCM_QUERY_RIB_RESPONSE, /* IPC Process -> IPC Manager */
	RINA_C_RMT_ADD_FTE_REQUEST, /* TODO IPC Process (user space) -> RMT (kernel) */
	RINA_C_RMT_DELETE_FTE_REQUEST, /* TODO IPC Process (user space) -> RMT (kernel) */
	RINA_C_RMT_DUMP_FT_REQUEST, /* TODO IPC Process (user space) -> RMT (kernel) */
	RINA_C_RMT_DUMP_FT_REPLY, /* TODO RMT (kernel) -> IPC Process (user space) */
	RINA_C_IPCM_SOCKET_CLOSED_NOTIFICATION, /* Kernel (NL layer) -> IPC Manager */
	RINA_C_IPCM_IPC_MANAGER_PRESENT, /* IPC Manager -> Kernel (NL layer) */
	RINA_C_IPCM_IPC_PROCESS_INITIALIZED, /* IPC Process -> IPC Manager */
	RINA_C_APP_ALLOCATE_FLOW_REQUEST, /* Allocate flow request, Application -> IPC Manager */
	RINA_C_APP_ALLOCATE_FLOW_REQUEST_RESULT, /* Response to an application allocate flow request, IPC Manager -> Application */
	RINA_C_APP_ALLOCATE_FLOW_REQUEST_ARRIVED, /* Allocate flow request from a remote application, IPC Manager -> Application */
	RINA_C_APP_ALLOCATE_FLOW_RESPONSE, /* Allocate flow response to an allocate request arrived operation, Application -> IPC Manager */
	RINA_C_APP_DEALLOCATE_FLOW_REQUEST, /* Application -> IPC Manager */
	RINA_C_APP_DEALLOCATE_FLOW_RESPONSE, /* IPC Manager -> Application */
	RINA_C_APP_FLOW_DEALLOCATED_NOTIFICATION, /* IPC Manager -> Application, flow deallocated without the application having requested it */
	RINA_C_APP_REGISTER_APPLICATION_REQUEST, /* Application -> IPC Manager */
	RINA_C_APP_REGISTER_APPLICATION_RESPONSE, /* IPC Manager -> Application */
	RINA_C_APP_UNREGISTER_APPLICATION_REQUEST, /* Application -> IPC Manager */
	RINA_C_APP_UNREGISTER_APPLICATION_RESPONSE, /* IPC Manager -> Application */
	RINA_C_APP_APPLICATION_REGISTRATION_CANCELED_NOTIFICATION, /* IPC Manager -> Application, application unregistered without the application having requested it */
	RINA_C_APP_GET_DIF_PROPERTIES_REQUEST, /* Application -> IPC Manager */
	RINA_C_APP_GET_DIF_PROPERTIES_RESPONSE, /* IPC Manager -> Application */
	__RINA_C_MAX,
 };

struct rinaHeader{
	unsigned short sourceIPCProcessId;
	unsigned short destIPCProcessId;
};

/**
 * Base class for Netlink messages to be sent or received
 */
class BaseNetlinkMessage: public StringConvertable {
	/**
	 * The identity of the Generic RINA Netlink family - dynamically allocated
	 * by the Netlink controller
	 */
	int family;

	/** The port id of the source of the Netlink message */
	unsigned int sourcePortId;

	/**
	 * Contains the id of the IPC Process that is the source of the message. It
	 *  is 0 if the source of the message is not an IPC Process (an application
	 *  or the IPC Manager).
	 */
	unsigned short sourceIPCProcessId;

	/** The port id of the destination of the Netlink message */
	unsigned int destPortId;

	/**
	 * Contains the id of the IPC Process that is the destination of the message.
	 * It is 0 if the destination of the message is not an IPC Process (an
	 * application or the IPC Manager).
	 */
	unsigned short destIPCProcessId;

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
	virtual IPCEvent* toIPCEvent() = 0;
	unsigned int getDestPortId() const;
	void setDestPortId(unsigned int destPortId);
	unsigned int getSequenceNumber() const;
	void setSequenceNumber(unsigned int sequenceNumber);
	unsigned int getSourcePortId() const;
	void setSourcePortId(unsigned int sourcePortId);
	unsigned short getDestIpcProcessId() const;
	void setDestIpcProcessId(unsigned short destIpcProcessId);
	unsigned short getSourceIpcProcessId() const;
	void setSourceIpcProcessId(unsigned short sourceIpcProcessId);
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

class BaseNetlinkResponseMessage: public BaseNetlinkMessage {
	/**
	 * Result of the operation. 0 indicates success, a negative value an
	 * error code.
	 */
	int result;

public:
	BaseNetlinkResponseMessage(RINANetlinkOperationCode operationCode);
	int getResult() const;
	void setResult(int result);
};

/**
 * An allocate flow request message, exchanged between an Application Process
 * and the IPC Manager.
 */

class AppAllocateFlowRequestMessage: public BaseNetlinkResponseMessage {

	/** The source application name */
	ApplicationProcessNamingInformation sourceAppName;

	/** The destination application name */
	ApplicationProcessNamingInformation destAppName;

	/** The characteristics of the flow */
	FlowSpecification flowSpecification;

public:
	AppAllocateFlowRequestMessage();
	const ApplicationProcessNamingInformation& getDestAppName() const;
	void setDestAppName(const ApplicationProcessNamingInformation& destAppName);
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
class AppAllocateFlowRequestResultMessage: public BaseNetlinkMessage {

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

public:
	AppAllocateFlowRequestResultMessage();
	const std::string& getErrorDescription() const;
	void setErrorDescription(const std::string& errorDescription);
	int getPortId() const;
	void setPortId(int portId);
	const ApplicationProcessNamingInformation& getDifName() const;
	void setDifName(const ApplicationProcessNamingInformation& difName);
	const ApplicationProcessNamingInformation& getSourceAppName() const;
	void setSourceAppName(
			const ApplicationProcessNamingInformation& sourceAppName);
	IPCEvent* toIPCEvent();
};

/**
 * Allocate flow request from a remote application, IPC Process -> Application
 */
class AppAllocateFlowRequestArrivedMessage: public BaseNetlinkResponseMessage {

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
	void setDestAppName(const ApplicationProcessNamingInformation& destAppName);
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
class AppAllocateFlowResponseMessage: public BaseNetlinkMessage {

	/** 0 if the application accepts the flow, error code otherwise */
	int result;

	/**
	 * If the flow was denied, this field controls wether the application
	 * wants the IPC Process to reply to the source or not
	 */
	bool notifySource;

public:
	AppAllocateFlowResponseMessage();
	int getResult() const;
	void setResult(int result);
	bool isNotifySource() const;
	void setNotifySource(bool notifySource);
	IPCEvent* toIPCEvent();
};

/**
 * Issued by the application process when it whishes to deallocate a flow.
 * Application -> IPC Process
 */
class AppDeallocateFlowRequestMessage: public BaseNetlinkResponseMessage {

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
	IPCEvent* toIPCEvent();
};

/**
 * Response by the IPC Process to the flow deallocation request
 */
class AppDeallocateFlowResponseMessage: public BaseNetlinkResponseMessage {

	/**
	 * The name of the applicaiton that requested the flow deallocation
	 */
	ApplicationProcessNamingInformation applicationName;

public:
	AppDeallocateFlowResponseMessage();
	const ApplicationProcessNamingInformation& getApplicationName() const;
	void setApplicationName(
			const ApplicationProcessNamingInformation& applicationName);
	IPCEvent* toIPCEvent();
};

/**
 * IPC Process -> IPC Manager, flow deallocated without the application having
 *  requested it
 */
class AppFlowDeallocatedNotificationMessage: public BaseNetlinkResponseMessage {

	/** The portId of the flow that has been deallocated */
	int portId;

	/** A number identifying a reason why the flow has been deallocated */
	int code;

	/**
	 * The name of the application that was using the flow
	 */
	ApplicationProcessNamingInformation applicationName;


public:
	AppFlowDeallocatedNotificationMessage();
	int getCode() const;
	void setCode(int code);
	int getPortId() const;
	void setPortId(int portId);
	const ApplicationProcessNamingInformation& getApplicationName() const;
	void setApplicationName(
			const ApplicationProcessNamingInformation& applicationName);
	IPCEvent* toIPCEvent();
};

/**
 * Invoked by the application when it wants to register an application
 * to a DIF. Application -> IPC Manager
 */
class AppRegisterApplicationRequestMessage: public BaseNetlinkResponseMessage {

	/** Information about the registration request */
	ApplicationRegistrationInformation applicationRegistrationInformation;

public:
	AppRegisterApplicationRequestMessage();
	const ApplicationRegistrationInformation&
		getApplicationRegistrationInformation() const;
	void setApplicationRegistrationInformation(
			const ApplicationRegistrationInformation& appRegistrationInfo);
	IPCEvent* toIPCEvent();
};

/**
 * Response of the IPC Manager to an application registration request.
 * IPC Manager -> Application
 */
class AppRegisterApplicationResponseMessage: public BaseNetlinkResponseMessage {

	/** The DIF name where the application wants to register */
	ApplicationProcessNamingInformation applicationName;

	/** The new DIF names where the application is now registered*/
	ApplicationProcessNamingInformation difName;

public:
	AppRegisterApplicationResponseMessage();
	const ApplicationProcessNamingInformation& getApplicationName() const;
	void setApplicationName(
			const ApplicationProcessNamingInformation& applicationName);
	const ApplicationProcessNamingInformation& getDifName() const;
	void setDifName(const ApplicationProcessNamingInformation& difName);
	IPCEvent* toIPCEvent();
};

/**
 * Invoked by the application when it wants to unregister an application.
 * Application -> IPC Manager
 */
class AppUnregisterApplicationRequestMessage: public BaseNetlinkResponseMessage {

	/** The name of the application to be registered */
	ApplicationProcessNamingInformation applicationName;

	/** The DIF name where the application is registered */
	ApplicationProcessNamingInformation difName;

public:
	AppUnregisterApplicationRequestMessage();
	const ApplicationProcessNamingInformation& getApplicationName() const;
	void setApplicationName(
			const ApplicationProcessNamingInformation& applicationName);
	const ApplicationProcessNamingInformation& getDifName() const;
	void setDifName(const ApplicationProcessNamingInformation& difName);
	IPCEvent* toIPCEvent();
};

/**
 * Response of the IPC Manager to an application unregistration request.
 * IPC Manager -> Application
 */
class AppUnregisterApplicationResponseMessage:
		public BaseNetlinkResponseMessage {

	/** The name of the application to be registered */
	ApplicationProcessNamingInformation applicationName;

public:
	AppUnregisterApplicationResponseMessage();
	const ApplicationProcessNamingInformation& getApplicationName() const;
	void setApplicationName(
			const ApplicationProcessNamingInformation& applicationName);
	IPCEvent* toIPCEvent();
};

/**
 * IPC Process -> Application, application unregistered without the application
 * having requested it
 */
class AppRegistrationCanceledNotificationMessage:
		public BaseNetlinkResponseMessage {

	/**
	 * A number identifying a reason why the application registration has
	 * been canceled
	 */
	int code;

	/**
	 * An optional explanation of why the application registration has
	 * been canceled
	 */
	std::string reason;

	/**
	 * The name of the application whose registration was canceled
	 */
	ApplicationProcessNamingInformation applicationName;

	/**
	 * The name of dif the application was registered before
	 */
	ApplicationProcessNamingInformation difName;

public:
	AppRegistrationCanceledNotificationMessage();
	int getCode() const;
	void setCode(int code);
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
 * Sent by the application to request the IPC Manager the properties of
 * one or more DIFs (Application -> IPC Manager)
 */
class AppGetDIFPropertiesRequestMessage:
		public BaseNetlinkResponseMessage {
	/**
	 * The name of the application that is querying the DIF properties
	 */
	ApplicationProcessNamingInformation applicationName;

	/**
	 * The name of the DIF whose properties the application wants to know
	 * If no DIF name is specified, the IPC Manager will return the
	 * properties of all the DIFs available to the application
	 */
	ApplicationProcessNamingInformation difName;

public:
	AppGetDIFPropertiesRequestMessage();
	const ApplicationProcessNamingInformation& getApplicationName() const;
	void setApplicationName(
			const ApplicationProcessNamingInformation& applicationName);
	const ApplicationProcessNamingInformation& getDifName() const;
	void setDifName(const ApplicationProcessNamingInformation& difName);
	IPCEvent* toIPCEvent();
};

/**
 * IPC Manager response, containing the properties of zero or more DIFs.
 * IPC Manager -> Application
 */
class AppGetDIFPropertiesResponseMessage: public BaseNetlinkResponseMessage {
	/**
	 * The name of the application that is querying the DIF properties
	 */
	ApplicationProcessNamingInformation applicationName;

	/** The properties of zero or more DIFs */
	std::list<DIFProperties> difProperties;
public:
	AppGetDIFPropertiesResponseMessage();
	const ApplicationProcessNamingInformation& getApplicationName() const;
	void setApplicationName(
				const ApplicationProcessNamingInformation& applicationName);
	const std::list<DIFProperties>& getDIFProperties() const;
	void setDIFProperties(const std::list<DIFProperties>& difProperties);
	void addDIFProperty(const DIFProperties& difProperties);
	IPCEvent* toIPCEvent();
};

/**
 * Invoked by the IPCManager when it wants to register an application
 * to a DIF. IPC Manager -> IPC Process
 */
class IpcmRegisterApplicationRequestMessage:
		public BaseNetlinkResponseMessage {

	/** The name of the application to be registered */
	ApplicationProcessNamingInformation applicationName;

	/** The DIF name where the application wants to register */
	ApplicationProcessNamingInformation difName;

public:
	IpcmRegisterApplicationRequestMessage();
	const ApplicationProcessNamingInformation& getApplicationName() const;
	void setApplicationName(
			const ApplicationProcessNamingInformation& applicationName);
	const ApplicationProcessNamingInformation& getDifName() const;
	void setDifName(const ApplicationProcessNamingInformation& difName);
	IPCEvent* toIPCEvent();
};

/**
 * Response of the IPC Process to an application registration request.
 * IPC Process -> IPC Manager
 */
class IpcmRegisterApplicationResponseMessage: public BaseNetlinkResponseMessage {

public:
	IpcmRegisterApplicationResponseMessage();
	IPCEvent* toIPCEvent();
};


/**
 * Invoked by the IPC Manager when it wants to unregister an application.
 * IPC Manager -> IPC Process
 */
class IpcmUnregisterApplicationRequestMessage: public BaseNetlinkResponseMessage {

	/** The name of the application to be registered */
	ApplicationProcessNamingInformation applicationName;

	/** The DIF name where the application wants to register */
	ApplicationProcessNamingInformation difName;

public:
	IpcmUnregisterApplicationRequestMessage();
	const ApplicationProcessNamingInformation& getApplicationName() const;
	void setApplicationName(
			const ApplicationProcessNamingInformation& applicationName);
	const ApplicationProcessNamingInformation& getDifName() const;
	void setDifName(const ApplicationProcessNamingInformation& difName);
	IPCEvent* toIPCEvent();
};


/**
 * Response of the IPC Process to an application unregistration request.
 * IPC Process -> IPC Manager
 */
class IpcmUnregisterApplicationResponseMessage: public BaseNetlinkResponseMessage {

public:
	IpcmUnregisterApplicationResponseMessage();
	IPCEvent* toIPCEvent();
};



/**
 * Makes an IPC Process a member of a DIF.
 * IPC Manager -> IPC Process
 */
class IpcmAssignToDIFRequestMessage: public BaseNetlinkResponseMessage {

	/** The information of the DIF where the IPC Process is assigned */
	DIFInformation difInformation;

public:
	IpcmAssignToDIFRequestMessage();
	const DIFInformation& getDIFInformation() const;
	void setDIFInformation(const DIFInformation& difInformation);
	IPCEvent* toIPCEvent();
};

/**
 * Reports the IPC Manager about the result of an Assign to DIF request
 * IPC Process -> IPC Manager
 */

class IpcmAssignToDIFResponseMessage: public BaseNetlinkResponseMessage {

public:
	IpcmAssignToDIFResponseMessage();
	IPCEvent* toIPCEvent();
};

/**
 * Updates the configuration of the DIF the IPC process is a member of
 * IPC Manager -> IPC Process
 */
class IpcmUpdateDIFConfigurationRequestMessage:
                public BaseNetlinkResponseMessage {

        /** The new configuration of the DIF */
        DIFConfiguration difConfiguration;

public:
        IpcmUpdateDIFConfigurationRequestMessage();
        const DIFConfiguration& getDIFConfiguration() const;
        void setDIFConfiguration(const DIFConfiguration& difConfiguration);
        IPCEvent* toIPCEvent();
};

/**
 * Reports the IPC Manager about the result of an Update DIF Config operation
 * IPC Process -> IPC Manager
 */

class IpcmUpdateDIFConfigurationResponseMessage:
                public BaseNetlinkResponseMessage {

public:
        IpcmUpdateDIFConfigurationResponseMessage();
        IPCEvent* toIPCEvent();
};

class IpcmAllocateFlowRequestMessage: public BaseNetlinkResponseMessage {
	/** The source application name*/
	ApplicationProcessNamingInformation sourceAppName;

	/** The destination application name*/
	ApplicationProcessNamingInformation destAppName;

	/** The characteristics requested for the flow*/
	FlowSpecification flowSpec;

	/** The DIF where the Flow is being allocated */
	ApplicationProcessNamingInformation difName;

public:
	IpcmAllocateFlowRequestMessage();
	const ApplicationProcessNamingInformation& getDestAppName() const;
	void setDestAppName(const ApplicationProcessNamingInformation& destAppName);
	const FlowSpecification& getFlowSpec() const;
	void setFlowSpec(const FlowSpecification& flowSpec);
	const ApplicationProcessNamingInformation& getSourceAppName() const;
	void setSourceAppName(
			const ApplicationProcessNamingInformation& sourceAppName);
	const ApplicationProcessNamingInformation& getDifName() const;
	void setDifName(const ApplicationProcessNamingInformation& difName);
	IPCEvent* toIPCEvent();
};

/**
 * Sent by the IPC Process to inform about the result of the flow allocation
 * request operation. IPC Process -> IPC Manager
 */
class IpcmAllocateFlowRequestResultMessage: public BaseNetlinkResponseMessage {

        /** The port id allocated to the flow */
        int portId;
public:
	IpcmAllocateFlowRequestResultMessage();
	int getPortId() const;
	void setPortId(int portId);
	IPCEvent* toIPCEvent();
};

/**
 * Allocate flow request from a remote application, IPC Process -> IPC Manager
 */
class IpcmAllocateFlowRequestArrivedMessage:
		public BaseNetlinkResponseMessage {

	/** The source application name */
	ApplicationProcessNamingInformation sourceAppName;

	/** The destination application name */
	ApplicationProcessNamingInformation destAppName;

	/** The characteristics of the flow */
	FlowSpecification flowSpecification;

	/** The dif Name */
	ApplicationProcessNamingInformation difName;

	/** The port id allocated to the flow */
	int portId;

public:
	IpcmAllocateFlowRequestArrivedMessage();
	const ApplicationProcessNamingInformation& getDestAppName() const;
	void setDestAppName(const ApplicationProcessNamingInformation& destAppName);
	const FlowSpecification& getFlowSpecification() const;
	void setFlowSpecification(const FlowSpecification& flowSpecification);
	const ApplicationProcessNamingInformation& getSourceAppName() const;
	void setSourceAppName(
			const ApplicationProcessNamingInformation& sourceAppName);
	const ApplicationProcessNamingInformation& getDifName() const;
	void setDifName(const ApplicationProcessNamingInformation& difName);
	int getPortId() const;
	void setPortId(int portId);
	IPCEvent* toIPCEvent();
};

/**
 * Allocate flow response to an allocate request arrived operation,
 * IPC Manager -> IPC Process
 */
class IpcmAllocateFlowResponseMessage: public BaseNetlinkMessage {

	/** Result of the flow allocation operation */
	int result;

	/**
	 * If the flow was denied, this field controls wether the application
	 * wants the IPC Process to reply to the source or not
	 */
	bool notifySource;

public:
	IpcmAllocateFlowResponseMessage();
	int getResult() const;
	void setResult(int result);
	bool isNotifySource() const;
	void setNotifySource(bool notifySource);
	IPCEvent* toIPCEvent();
};

/**
 * IPC Manager -> IPC Process
 */
class IpcmDeallocateFlowRequestMessage: public BaseNetlinkResponseMessage {

	/** The id of the flow to be deallocated */
	int portId;

public:
	IpcmDeallocateFlowRequestMessage();
	int getPortId() const;
	void setPortId(int portId);
	IPCEvent* toIPCEvent();
};

/**
 * Response by the IPC Process to the flow deallocation request
 */
class IpcmDeallocateFlowResponseMessage: public BaseNetlinkResponseMessage {

public:
	IpcmDeallocateFlowResponseMessage();
	IPCEvent* toIPCEvent();
};

/**
 * IPC Manager -> Application, flow deallocated without the application having
 *  requested it
 */
class IpcmFlowDeallocatedNotificationMessage: public BaseNetlinkResponseMessage {

	/** The portId of the flow that has been deallocated */
	int portId;

	/** A number identifying a reason why the flow has been deallocated */
	int code;

public:
	IpcmFlowDeallocatedNotificationMessage();
	int getCode() const;
	void setCode(int code);
	int getPortId() const;
	void setPortId(int portId);
	IPCEvent* toIPCEvent();
};

/**
 * Used by the IPC Manager to notify the IPC Process about having been
 * registered to or unregistered from a DIF. IPC Manager -> IPC Process
 */
class IpcmDIFRegistrationNotification:
		public BaseNetlinkResponseMessage {
	/** The name of the IPC Process registered to the N-1 DIF */
	ApplicationProcessNamingInformation ipcProcessName;

	/** The name of the N-1 DIF where the IPC Process has been registered*/
	ApplicationProcessNamingInformation difName;

	/**
	 * If true the IPC Process has been registered, if false,
	 * it has been unregistered
	 */
	bool registered;

public:
	IpcmDIFRegistrationNotification();
	const ApplicationProcessNamingInformation& getDifName() const;
	void setDifName(const ApplicationProcessNamingInformation& difName);
	const ApplicationProcessNamingInformation& getIpcProcessName() const;
	void setIpcProcessName(
			const ApplicationProcessNamingInformation& ipcProcessName);
	void setRegistered(bool registered);
	bool isRegistered() const;
	IPCEvent* toIPCEvent();
};

/**
 * Used by the IPC Manager to request information from an IPC Process RIB
 * IPC Manager -> IPC Process
 */
class IpcmDIFQueryRIBRequestMessage:
		public BaseNetlinkResponseMessage {

	/** The class of the object being queried*/
	std::string objectClass;

	/** The name of the object being queried */
	std::string objectName;

	/**
	 * The instance of the object being queried. Either objectname +
	 * object class or object instance have to be specified
	 */
	unsigned long objectInstance;

	/** Number of levels below the object_name the query affects*/
	unsigned int scope;

	/**
	 * Regular expression applied to all nodes affected by the query
	 * in order to decide whether they have to be returned or not
	 */
	std::string filter;

public:
	IpcmDIFQueryRIBRequestMessage();
	const std::string& getFilter() const;
	void setFilter(const std::string& filter);
	const std::string& getObjectClass() const;
	void setObjectClass(const std::string& objectClass);
	unsigned long getObjectInstance() const;
	void setObjectInstance(unsigned long objectInstance);
	const std::string& getObjectName() const;
	void setObjectName(const std::string& objectName);
	unsigned int getScope() const;
	void setScope(unsigned int scope);
	IPCEvent* toIPCEvent();
};

/**
 * Replies the IPC Manager with zero or more objects in the RIB of the
 * IPC Process. IPC Process -> IPC Manager
 */
class IpcmDIFQueryRIBResponseMessage:
		public BaseNetlinkResponseMessage {

	std::list<RIBObject> ribObjects;

public:
	IpcmDIFQueryRIBResponseMessage();
	const std::list<RIBObject>& getRIBObjects() const;
	void setRIBObjects(const std::list<RIBObject>& ribObjects);
	void addRIBObject(const RIBObject& ribObject);
	IPCEvent* toIPCEvent();
};

/**
 * NL layer (kernel) -> IPC Manager. Received when a user-space NL socket has been
 * closed, which means the user process associated to that socket will be no longer
 * reachable.
 */
class IpcmNLSocketClosedNotificationMessage:
		public BaseNetlinkResponseMessage {

	/** The portId of the NL socket that has been closed*/
	int portId;

public:
	IpcmNLSocketClosedNotificationMessage();
	int getPortId() const;
	void setPortId(int portId);
	IPCEvent* toIPCEvent();
};

/**
 * IPC Manager -> Kernel (NL layer). Sent when the IPC Manager starts up, to
 * signal the kernel that the IPC Manager is ready and make its NL port-id known.
 */
class IpcmIPCManagerPresentMessage: public BaseNetlinkMessage {
public:
	IpcmIPCManagerPresentMessage();
	IPCEvent* toIPCEvent();
};

/**
 * IPC Process -> IPC Manager. Sent after the IPC Process daemon has initialized
 * the NL infrastructure and is ready to receive messages.
 */
class IpcmIPCProcessInitializedMessage: public BaseNetlinkMessage {
public:
        IpcmIPCProcessInitializedMessage();
        IPCEvent* toIPCEvent();
};

}

#endif /* NETLINK_MESSAGES_H_ */
