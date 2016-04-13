/*
 * Netlink messages
 *
 *    Eduard Grasa      <eduard.grasa@i2cat.net>
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#ifndef LIBRINA_NETLINK_MESSAGES_H
#define LIBRINA_NETLINK_MESSAGES_H

#ifdef __cplusplus

#include "librina/configuration.h"
#include "librina/ipc-process.h"
#include "librina/rib_v2.h"

namespace rina {

enum RINANetlinkOperationCode{
	RINA_C_UNSPEC, /* 0 Unespecified operation */
	RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST, /* 1 IPC Manager -> IPC Process */
	RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE, /* 2 IPC Process -> IPC Manager */
        RINA_C_IPCM_UPDATE_DIF_CONFIG_REQUEST, /* 3 IPC Manager -> IPC Process */
        RINA_C_IPCM_UPDATE_DIF_CONFIG_RESPONSE, /* 4 IPC Process -> IPC Manager */
	RINA_C_IPCM_IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION, /* 5 IPC Manager -> IPC Process */
	RINA_C_IPCM_IPC_PROCESS_DIF_UNREGISTRATION_NOTIFICATION, /* 6 IPC Manager -> IPC Process */
	RINA_C_IPCM_ENROLL_TO_DIF_REQUEST, /* 7 IPC Manager -> IPC Process */
	RINA_C_IPCM_ENROLL_TO_DIF_RESPONSE, /* 8 IPC Process -> IPC Manager */
	RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_REQUEST, /* TODO 9 IPC Manager -> IPC Process */
	RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_RESPONSE, /* TODO 10 IPC Process -> IPC Manager */
	RINA_C_IPCM_ALLOCATE_FLOW_REQUEST, /* 11 IPC Manager -> IPC Process */
	RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_ARRIVED, /* 12 Allocate flow request from a remote application, IPC Process -> IPC Manager */
	RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_RESULT, /* 13 IPC Process -> IPC Manager */
	RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE, /* 14 IPC Manager -> IPC Process */
	RINA_C_IPCM_DEALLOCATE_FLOW_REQUEST, /* 15 IPC Manager -> IPC Process */
	RINA_C_IPCM_DEALLOCATE_FLOW_RESPONSE, /* 16 IPC Process -> IPC Manager */
	RINA_C_IPCM_FLOW_DEALLOCATED_NOTIFICATION, /* 17 IPC Process -> IPC Manager, flow deallocated without the application having requested it */
	RINA_C_IPCM_REGISTER_APPLICATION_REQUEST, /* 18 IPC Manager -> IPC Process */
	RINA_C_IPCM_REGISTER_APPLICATION_RESPONSE, /* 19 IPC Process -> IPC Manager */
	RINA_C_IPCM_UNREGISTER_APPLICATION_REQUEST, /* 20 IPC Manager -> IPC Process */
	RINA_C_IPCM_UNREGISTER_APPLICATION_RESPONSE, /* 21 IPC Process -> IPC Manager */
	RINA_C_IPCM_QUERY_RIB_REQUEST, /* 22 IPC Manager -> IPC Process */
	RINA_C_IPCM_QUERY_RIB_RESPONSE, /* 23 IPC Process -> IPC Manager */
	RINA_C_RMT_MODIFY_FTE_REQUEST, /* 24 IPC Process (user space) -> Kernel IPC Process (kernel) */
	RINA_C_RMT_DUMP_FT_REQUEST, /* 25 IPC Process (user space) -> RMT (kernel) */
	RINA_C_RMT_DUMP_FT_REPLY, /* 26 RMT (kernel) -> IPC Process (user space) */
	RINA_C_IPCM_SOCKET_CLOSED_NOTIFICATION, /* 27 Kernel (NL layer) -> IPC Manager */
	RINA_C_IPCM_IPC_MANAGER_PRESENT, /* 28 IPC Manager -> Kernel (NL layer) */
	RINA_C_IPCP_CONN_CREATE_REQUEST, /* 29 IPC Process Daemon -> EFCP (Kernel) */
	RINA_C_IPCP_CONN_CREATE_RESPONSE, /* 30 EFCP(Kernel) -> IPC Process Daemon */
	RINA_C_IPCP_CONN_CREATE_ARRIVED, /* 31 IPC Process Daemon -> EFCP (Kernel) */
	RINA_C_IPCP_CONN_CREATE_RESULT, /* 32 EFCP(kernel) -> IPC Process daemon */
	RINA_C_IPCP_CONN_UPDATE_REQUEST, /* 33 IPC Process Daemon -> EFCP (Kernel) */
	RINA_C_IPCP_CONN_UPDATE_RESULT, /* 34 EFCP(kernel) -> IPC Process daemon */
	RINA_C_IPCP_CONN_DESTROY_REQUEST, /* 35 IPC Process Daemon -> EFCP (Kernel) */
	RINA_C_IPCP_CONN_DESTROY_RESULT, /* 36 EFCP(kernel) -> IPC Process daemon */
        RINA_C_IPCM_SET_POLICY_SET_PARAM_REQUEST, /* 37, IPC Manager -> IPC Process */
        RINA_C_IPCM_SET_POLICY_SET_PARAM_RESPONSE, /* 38, IPC Process -> IPC Manager */
        RINA_C_IPCM_SELECT_POLICY_SET_REQUEST, /* 39, IPC Manager -> IPC Process */
        RINA_C_IPCM_SELECT_POLICY_SET_RESPONSE, /* 40, IPC Process -> IPC Manager */
        RINA_C_IPCP_UPDATE_CRYPTO_STATE_REQUEST, /* 41, IPC Process (user space) -> IPC Process (kernel) */
        RINA_C_IPCP_UPDATE_CRYPTO_STATE_RESPONSE, /* 42, IPC Process (kernel) -> IPC Process (user space) */

        /* Userspace only messages MUST be after all the messages that are also
         * handled by the kernel. */
	RINA_C_IPCM_IPC_PROCESS_INITIALIZED, /* 43 IPC Process -> IPC Manager */
	RINA_C_APP_ALLOCATE_FLOW_REQUEST, /* 44 Allocate flow request, Application -> IPC Manager */
	RINA_C_APP_ALLOCATE_FLOW_REQUEST_RESULT, /* 45 Response to an application allocate flow request, IPC Manager -> Application */
	RINA_C_APP_ALLOCATE_FLOW_REQUEST_ARRIVED, /* 46 Allocate flow request from a remote application, IPC Manager -> Application */
	RINA_C_APP_ALLOCATE_FLOW_RESPONSE, /* 47 Allocate flow response to an allocate request arrived operation, Application -> IPC Manager */
	RINA_C_APP_DEALLOCATE_FLOW_REQUEST, /* 48 Application -> IPC Manager */
	RINA_C_APP_DEALLOCATE_FLOW_RESPONSE, /* 49 IPC Manager -> Application */
	RINA_C_APP_FLOW_DEALLOCATED_NOTIFICATION, /* 50 IPC Manager -> Application, flow deallocated without the application having requested it */
	RINA_C_APP_REGISTER_APPLICATION_REQUEST, /* 51 Application -> IPC Manager */
	RINA_C_APP_REGISTER_APPLICATION_RESPONSE, /* 52 IPC Manager -> Application */
	RINA_C_APP_UNREGISTER_APPLICATION_REQUEST, /* 53 Application -> IPC Manager */
	RINA_C_APP_UNREGISTER_APPLICATION_RESPONSE, /* 54 IPC Manager -> Application */
	RINA_C_APP_APPLICATION_REGISTRATION_CANCELED_NOTIFICATION, /* 55 IPC Manager -> Application, application unregistered without the application having requested it */
	RINA_C_APP_GET_DIF_PROPERTIES_REQUEST, /* 56 Application -> IPC Manager */
	RINA_C_APP_GET_DIF_PROPERTIES_RESPONSE, /* 57 IPC Manager -> Application */
        RINA_C_IPCM_PLUGIN_LOAD_REQUEST, /* 58, IPC Manager -> IPC Process */
        RINA_C_IPCM_PLUGIN_LOAD_RESPONSE, /* 59, IPC Process -> IPC Manager */
        RINA_C_IPCM_FWD_CDAP_MSG_REQUEST, /* 60, IPC Manager <-> IPC Process */
        RINA_C_IPCM_FWD_CDAP_MSG_RESPONSE, /* 61, IPC Manager <-> IPC Process */
	__RINA_C_MAX,
 };

struct rinaHeader{
	unsigned short sourceIPCProcessId;
	unsigned short destIPCProcessId;
};

/**
 * Base class for Netlink messages to be sent or received
 */
class BaseNetlinkMessage {
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

	/** True if this is a notification message */
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
	const std::string toString();

private:
	static const std::string operationCodeToString(RINANetlinkOperationCode operationCode);
};

class BaseNetlinkResponseMessage: public BaseNetlinkMessage {
public:
	/**
	 * Result of the operation. 0 indicates success, a negative value an
	 * error code.
	 */
	int result;

	BaseNetlinkResponseMessage(RINANetlinkOperationCode operationCode);
	int getResult() const;
	void setResult(int result);
};

/**
 * An allocate flow request message, exchanged between an Application Process
 * and the IPC Manager.
 */

class AppAllocateFlowRequestMessage: public BaseNetlinkMessage {

	/** The source application name */
	ApplicationProcessNamingInformation sourceAppName;

	/** The destination application name */
	ApplicationProcessNamingInformation destAppName;

	/** The characteristics of the flow */
	FlowSpecification flowSpecification;

	/** The DIF name where the flow is to be allocated, optional*/
	ApplicationProcessNamingInformation difName;

public:
	AppAllocateFlowRequestMessage();
	const ApplicationProcessNamingInformation& getDestAppName() const;
	void setDestAppName(const ApplicationProcessNamingInformation& destAppName);
	const FlowSpecification& getFlowSpecification() const;
	void setFlowSpecification(const FlowSpecification& flowSpecification);
	const ApplicationProcessNamingInformation& getSourceAppName() const;
	void setSourceAppName(
			const ApplicationProcessNamingInformation& sourceAppName);
        const ApplicationProcessNamingInformation& getDifName() const;
        void setDifName(const ApplicationProcessNamingInformation& difName);
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
 * Allocate flow request from a remote application, IPC Manager -> Application
 */
class AppAllocateFlowRequestArrivedMessage: public BaseNetlinkMessage {

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
class AppAllocateFlowResponseMessage: public BaseNetlinkResponseMessage {

	/**
	 * If the flow was denied, this field controls wether the application
	 * wants the IPC Process to reply to the source or not
	 */
	bool notifySource;
public:
	AppAllocateFlowResponseMessage();
	bool isNotifySource() const;
	void setNotifySource(bool notifySource);
	IPCEvent* toIPCEvent();
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

	/** the portid of the flow deallocated */
	int portId;

public:
	AppDeallocateFlowResponseMessage();
	const ApplicationProcessNamingInformation& getApplicationName() const;
	void setApplicationName(
			const ApplicationProcessNamingInformation& applicationName);
	void setPortId(int portId);
	int getPortId() const;
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
class AppRegisterApplicationRequestMessage: public BaseNetlinkMessage {

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
class AppUnregisterApplicationRequestMessage: public BaseNetlinkMessage {

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
		public BaseNetlinkMessage {
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
		public BaseNetlinkMessage {
public:
	/** The name of the application to be registered */
	ApplicationProcessNamingInformation applicationName;

	/** The id of the IPC Process being registered (0 if it is an app) */
	unsigned short regIpcProcessId;

	/** The DIF name where the application wants to register */
	ApplicationProcessNamingInformation difName;

	IpcmRegisterApplicationRequestMessage();
	const ApplicationProcessNamingInformation& getApplicationName() const;
	void setApplicationName(
			const ApplicationProcessNamingInformation& applicationName);
	const ApplicationProcessNamingInformation& getDifName() const;
	void setDifName(const ApplicationProcessNamingInformation& difName);
	unsigned short getRegIpcProcessId() const;
	void setRegIpcProcessId(unsigned short regIpcProcessId);
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
class IpcmUnregisterApplicationRequestMessage: public BaseNetlinkMessage {

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
class IpcmAssignToDIFRequestMessage: public BaseNetlinkMessage {

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
                public BaseNetlinkMessage {

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


/**
 * Instruct a normal IPC Process to enroll in a given DIF, using the
 * supporting N-1 DIF (IPC Manager -> IPC Process)
 */
class IpcmEnrollToDIFRequestMessage: public BaseNetlinkMessage {

        /** The DIF to enroll to */
        ApplicationProcessNamingInformation difName;

        /** The N-1 DIF name to allocate a flow to the member */
        ApplicationProcessNamingInformation supportingDIFName;

        /** The neighbor to enroll to */
        ApplicationProcessNamingInformation neighborName;

public:
        IpcmEnrollToDIFRequestMessage();
        const ApplicationProcessNamingInformation& getDifName() const;
        void setDifName(const ApplicationProcessNamingInformation& difName);
        const ApplicationProcessNamingInformation& getNeighborName() const;
        void setNeighborName(
                const ApplicationProcessNamingInformation& neighborName);
        const ApplicationProcessNamingInformation&
                getSupportingDifName() const;
        void setSupportingDifName(
                const ApplicationProcessNamingInformation& supportingDifName);
        IPCEvent* toIPCEvent();
};

/**
 * Reports the IPC Manager about the result of an Enroll to DIF operation
 * IPC Process -> IPC Manager
 */
class IpcmEnrollToDIFResponseMessage:
                public BaseNetlinkResponseMessage {
        /**
         * The new neighbors of the IPC Process after the enrollment
         * operation
         */
        std::list<Neighbor> neighbors;

        /**
         * If the IPC Process was not previously enrolled in a DIF,
         * this contains the DIF configuration
         */
        DIFInformation difInformation;

public:
        IpcmEnrollToDIFResponseMessage();
        const std::list<Neighbor>& getNeighbors() const;
        void setNeighbors(const std::list<Neighbor>& neighbors);
        void addNeighbor(const Neighbor& qosCube);
        void setDIFInformation(const DIFInformation& difInformation);
        const DIFInformation& getDIFInformation() const;
        IPCEvent* toIPCEvent();
};

/**
 * Used by the IPC Manager to request a flow allocation to an IPC Process
 */
class IpcmAllocateFlowRequestMessage: public BaseNetlinkMessage {
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
		public BaseNetlinkMessage {

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
class IpcmDeallocateFlowRequestMessage: public BaseNetlinkMessage {

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
		public BaseNetlinkMessage {

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

	std::list<rib::RIBObjectData> ribObjects;

public:
	IpcmDIFQueryRIBResponseMessage();
	const std::list<rib::RIBObjectData>& getRIBObjects() const;
	void setRIBObjects(const std::list<rib::RIBObjectData>& ribObjects);
	void addRIBObject(const rib::RIBObjectData& ribObject);
	IPCEvent* toIPCEvent();
};

/**
 * Used by the IPC Manager to ask an IPC process to modify a
 * policy-set-specific parameter or a parametric policy
 * IPC Manager -> IPC Process
 */
class IpcmSetPolicySetParamRequestMessage:
		public BaseNetlinkMessage {
public:
        /** The path of the sybcomponent/policy-set to be addressed */
	std::string path;

	/** The name of the parameter being accessed */
	std::string name;

	/** The value to assign to the parameter */
	std::string value;

	IpcmSetPolicySetParamRequestMessage();
	IPCEvent* toIPCEvent();
};

/**
 * Reports the IPC Manager about the result of a set-policy-set-param
 * request operation
 * IPC Process -> IPC Manager
 */
class IpcmSetPolicySetParamResponseMessage:
                public BaseNetlinkResponseMessage {
public:
        IpcmSetPolicySetParamResponseMessage();
        IPCEvent* toIPCEvent();
};

/**
 * Used by the IPC Manager to ask an IPC process to select a
 * policy-set for a component
 * IPC Manager -> IPC Process
 */
class IpcmSelectPolicySetRequestMessage:
		public BaseNetlinkMessage {
public:
        /** The path of the sybcomponent to be addressed */
	std::string path;

	/** The name of the policy-set to be selected */
	std::string name;

	IpcmSelectPolicySetRequestMessage();
	IPCEvent* toIPCEvent();
};

/**
 * Reports the IPC Manager about the result of a select-policy-set
 * request operation
 * IPC Process -> IPC Manager
 */
class IpcmSelectPolicySetResponseMessage:
                public BaseNetlinkResponseMessage {
public:
        IpcmSelectPolicySetResponseMessage();
        IPCEvent* toIPCEvent();
};

/**
 * Used by the IPC Manager to ask an IPC process to load or
 * unload a plugin.
 * IPC Manager -> IPC Process
 */
class IpcmPluginLoadRequestMessage:
		public BaseNetlinkMessage {
public:
	/** The name of the plugin to be loaded or unloaded */
	std::string name;

	/** Specifies whether the plugin is to be loaded or unloaded */
	bool load;

	IpcmPluginLoadRequestMessage();
	IPCEvent* toIPCEvent();
};

/**
 * Used by the IPC Manager or the IPCProcess to forward
 * a CDAP request message to an IPC process or the IPCManager
 */
class IpcmFwdCDAPRequestMessage:
		public BaseNetlinkMessage {
public:
	/** The serialized object containing the message to be forwarded */
	ser_obj_t sermsg;

	/** Result of a forward operation, used only when IPC Process forwards
	 *  back a CDAP response to the IPC Manager. */
	int result;

	IpcmFwdCDAPRequestMessage();
	IPCEvent* toIPCEvent();
};

/**
 * Used by the IPC Manager or the IPCProcess to forward
 * a CDAP response message to an IPC process or the IPCManager
 */
class IpcmFwdCDAPResponseMessage:
                public BaseNetlinkMessage {
public:
        /** The serialized object containing the message to be forwarded */
        ser_obj_t sermsg;

        /** Result of a forward operation, used only when IPC Process forwards
         *  back a CDAP response to the IPC Manager. */
        int result;

        IpcmFwdCDAPResponseMessage();
        IPCEvent* toIPCEvent();
};

/**
 * Reports the IPC Manager about the result of a plugin-load
 * request operation
 * IPC Process -> IPC Manager
 */
class IpcmPluginLoadResponseMessage:
                public BaseNetlinkResponseMessage {
public:
        IpcmPluginLoadResponseMessage();
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
        /** The IPC Process name */
        ApplicationProcessNamingInformation name;

public:
        IpcmIPCProcessInitializedMessage();
        IpcmIPCProcessInitializedMessage(
                const ApplicationProcessNamingInformation& name);
        void setName(const ApplicationProcessNamingInformation& name);
        const ApplicationProcessNamingInformation& getName() const;
        IPCEvent* toIPCEvent();
};

/**
 * IPC Process Daemon -> Kernel IPC Process. Request the creation of a
 * connection to the EFCP module in the kernel.
 */
class IpcpConnectionCreateRequestMessage: public BaseNetlinkMessage {

        /** Contains the data of the connection to be created */
        Connection connection;

public:
        IpcpConnectionCreateRequestMessage();
        const Connection& getConnection() const;
        void setConnection(const Connection& connection);
        IPCEvent* toIPCEvent();
};

/**
 * Kernel IPC Process -> IPC Process Daemon. Report about the result of a
 * connection create operation
 */
class IpcpConnectionCreateResponseMessage: public BaseNetlinkMessage {

        /** The port-id where the connection will be bound to */
        int portId;

        /**
         * The source connection-endpoint id if the connection was created
         * successfully, or a negative number indicating an error code in
         * case of failure
         */
        int cepId;

public:
        IpcpConnectionCreateResponseMessage();
        int getCepId() const;
        void setCepId(int cepId);
        int getPortId() const;
        void setPortId(int portId);
        IPCEvent* toIPCEvent();
};

/**
 * IPC Process Daemon -> Kernel IPC Process. Inform the kernel IPC Process
 * the destination cep-id of a connection, as well as who will be using it
 * (IPC Process vs. application)
 */
class IpcpConnectionUpdateRequestMessage: public BaseNetlinkMessage {

        /** The port-id where the connection will be bound to */
        int portId;

        /** The connection's source CEP-id */
        int sourceCepId;

        /** The connection's destination CEP-ids */
        int destinationCepId;

        /**
         * The id of the IPC Process that will be using the flow
         * (0 if it is an application)
         */
        unsigned short flowUserIpcProcessId;

public:
        IpcpConnectionUpdateRequestMessage();
        int getDestinationCepId() const;
        void setDestinationCepId(int destinationCepId);
        unsigned short getFlowUserIpcProcessId() const;
        void setFlowUserIpcProcessId(unsigned short flowUserIpcProcessId);
        int getPortId() const;
        void setPortId(int portId);
        int getSourceCepId() const;
        void setSourceCepId(int sourceCepId);
        IPCEvent* toIPCEvent();
};

/**
 * Kernel IPC Process -> IPC Process Daemon. Report about the result of a
 * connection update operation
 */
class IpcpConnectionUpdateResultMessage:
                public BaseNetlinkResponseMessage {

        /** The port-id of the flow associated to this connection */
        int portId;

public:
        IpcpConnectionUpdateResultMessage();
        int getPortId() const;
        void setPortId(int portId);
        IPCEvent* toIPCEvent();
};

/**
 * IPC Process Daemon -> Kernel IPC Process. Request the creation of a
 * connection to the EFCP module in the kernel (receiving side of the
 * flow allocation process)
 */
class IpcpConnectionCreateArrivedMessage: public BaseNetlinkMessage {

        /** Contains the data of the connection to be created */
        Connection connection;

public:
        IpcpConnectionCreateArrivedMessage();
        const Connection& getConnection() const;
        void setConnection(const Connection& connection);
        IPCEvent* toIPCEvent();
};

/**
 * Kernel IPC Process -> IPC Process Daemon. Report about the result of a
 * connection create arrived operation
 */
class IpcpConnectionCreateResultMessage: public BaseNetlinkMessage {

        /** The port-id where the connection will be bound to */
        int portId;

        /**
         * The source connection-endpoint id if the connection was created
         * successfully, or a negative number indicating an error code in
         * case of failure
         */
        int sourceCepId;

        /** The destination cep-id */
        int destCepId;

public:
        IpcpConnectionCreateResultMessage();
        int getSourceCepId() const;
        void setSourceCepId(int cepId);
        int getDestCepId() const;
        void setDestCepId(int cepId);
        int getPortId() const;
        void setPortId(int portId);
        IPCEvent* toIPCEvent();
};

/**
 * IPC Process Daemon -> Kernel IPC Process. Request the destruction of a
 * connection
 */
class IpcpConnectionDestroyRequestMessage: public BaseNetlinkMessage {

        /** The port-id where the connection will be bound to */
        int portId;

        /**
         * The identifier of the connection to destroy
         */
        int cepId;

public:
        IpcpConnectionDestroyRequestMessage();
        int getCepId() const;
        void setCepId(int cepId);
        int getPortId() const;
        void setPortId(int portId);
        IPCEvent* toIPCEvent();
};

/**
 * Kernel IPC PRocess -> IPC Process Daemon. Report about the result of
 * a connection destroy request operation
 */
class IpcpConnectionDestroyResultMessage: public BaseNetlinkMessage {

        /** The port-id where the connection will be bound to */
        int portId;

        /**
         * The result of the operaiton
         */
        int result;

public:
        IpcpConnectionDestroyResultMessage();
        int getResult() const;
        void setResult(int result);
        int getPortId() const;
        void setPortId(int portId);
        IPCEvent* toIPCEvent();
};

/**
 * IPC Process -> Kernel IPC Process. Add the following entries to the
 * PDU Forwarding Table.
 */
class RmtModifyPDUFTEntriesRequestMessage: public BaseNetlinkMessage {
        /** The entries to be added */
        std::list<PDUForwardingTableEntry *> entries;

        /** 0 add, 1 remove, 2 flush and add */
        int mode;

public:
        RmtModifyPDUFTEntriesRequestMessage();
        const std::list<PDUForwardingTableEntry *>& getEntries() const;
        void setEntries(const std::list<PDUForwardingTableEntry *>& entries);
        void addEntry(PDUForwardingTableEntry * entry);
        int getMode() const;
        void setMode(int mode);
        IPCEvent* toIPCEvent();
};

/**
 * IPC Process -> Kernel IPC Process. Request a view on all the entries
 * in the PDU Forwarding Table.
 */
class RmtDumpPDUFTEntriesRequestMessage: public BaseNetlinkMessage {

public:
        RmtDumpPDUFTEntriesRequestMessage();
        IPCEvent* toIPCEvent();
};

/**
 * Kernel IPC Process -> IPC Process. Result of Dump PDU Forwarding Table
 * Request, lists all the entries in the PDU Forwarding table
 */
class RmtDumpPDUFTEntriesResponseMessage: public BaseNetlinkResponseMessage {
        /** The entries in the table */
        std::list<PDUForwardingTableEntry> entries;

public:
        RmtDumpPDUFTEntriesResponseMessage();
        const std::list<PDUForwardingTableEntry>& getEntries() const;
        void setEntries(const std::list<PDUForwardingTableEntry>& entries);
        void addEntry(const PDUForwardingTableEntry& entry);
        IPCEvent* toIPCEvent();
};

class IPCPUpdateCryptoStateRequestMessage : public BaseNetlinkMessage {
public:
	IPCPUpdateCryptoStateRequestMessage();
	IPCEvent* toIPCEvent();

	CryptoState state;
};

class IPCPUpdateCryptoStateResponseMessage: public BaseNetlinkResponseMessage {
public:
	IPCPUpdateCryptoStateResponseMessage();
	IPCEvent* toIPCEvent();

	int port_id;
};

}

#endif

#endif
