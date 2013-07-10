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

#ifndef LIBRINA_IPC_PROCESS_H
#define LIBRINA_IPC_PROCESS_H

#ifdef __cplusplus

#include "librina-common.h"
#include "librina-application.h"

namespace rina {

/**
 * Event informing the IPC Process about a flow deallocation request
 */
class FlowDeallocateRequestEvent: public IPCEvent {
	/** The port-id that locally identifies the flow */
	int portId;

	/** The name of the DIF that is providing this flow */
	ApplicationProcessNamingInformation DIFName;

	/** The application that requested the flow deallocation*/
	ApplicationProcessNamingInformation applicationName;

	/** The id of the IPC Process that should deallocate the flow */
	unsigned short ipcProcessId;

public:
	FlowDeallocateRequestEvent(int portId,
			const ApplicationProcessNamingInformation& DIFName,
			const ApplicationProcessNamingInformation& appName,
			unsigned short ipcProcessId,
			unsigned int sequenceNumber);
	int getPortId() const;
	const ApplicationProcessNamingInformation& getDIFName() const;
	const ApplicationProcessNamingInformation& getApplicationName() const;
	unsigned short getIPCProcessId() const;
};


/**
 * Event informing the IPC Process about an application unregistration request
 */
class IpcmUnregistrationRequestEvent: public IPCEvent {

	/** The name of the DIF that is providing this flow */
	ApplicationProcessNamingInformation DIFName;

	/** The application that requested the flow deallocation*/
	ApplicationProcessNamingInformation applicationName;


public:
	IpcmUnregistrationRequestEvent(const ApplicationProcessNamingInformation& DIFName,
			const ApplicationProcessNamingInformation& appName,
			unsigned int sequenceNumber);
	const ApplicationProcessNamingInformation& getDIFName() const;
	const ApplicationProcessNamingInformation& getApplicationName() const;
};



/**
 * The IPC Manager requests the IPC Process to become a member of a
 * DIF, and provides de related information
 */
class AssignToDIFRequestEvent: public IPCEvent {

	/** The configuration of the DIF the IPC Process is being assigned to*/
	DIFConfiguration difConfiguration;

public:
	AssignToDIFRequestEvent(const DIFConfiguration& difConfiguration,
			unsigned int sequenceNumber);
	const DIFConfiguration& getDIFConfiguration() const;
};

/**
 * Supporting class for IPC Process DIF Registration events
 */
class IPCProcessDIFRegistrationEvent: public IPCEvent {

	/** The name of the IPC Process registered to the N-1 DIF */
	ApplicationProcessNamingInformation ipcProcessName;

	/** The name of the N-1 DIF where the IPC Process has been registered*/
	ApplicationProcessNamingInformation difName;

public:
	IPCProcessDIFRegistrationEvent(IPCEventType eventType,
			const ApplicationProcessNamingInformation& ipcProcessName,
			const ApplicationProcessNamingInformation& difName,
			unsigned int sequenceNumber);
	const ApplicationProcessNamingInformation& getIPCProcessName() const;
	const ApplicationProcessNamingInformation& getDIFName() const;
};

/**
 * The IPC Manager informs the IPC Process that it has been registered
 * to an N-1 DIF
 */
class IPCProcessRegisteredToDIFEvent: public IPCProcessDIFRegistrationEvent{
public:
	IPCProcessRegisteredToDIFEvent(
			const ApplicationProcessNamingInformation& ipcProcessName,
			const ApplicationProcessNamingInformation& difName,
			unsigned int sequenceNumber);
};

/**
 * The IPC Manager informs the IPC Process that it has been unregistered
 * from an N-1 DIF
 */
class IPCProcessUnregisteredFromDIFEvent: public IPCProcessDIFRegistrationEvent{
public:
	IPCProcessUnregisteredFromDIFEvent(
			const ApplicationProcessNamingInformation& ipcProcessName,
			const ApplicationProcessNamingInformation& difName,
			unsigned int sequenceNumber);
};

/**
 * The IPC Manager queries the RIB of the IPC Process
 */
class QueryRIBRequestEvent: public IPCEvent {

	/** The class of the object being queried*/
	std::string objectClass;

	/** The name of the object being queried */
	std::string objectName;

	/**
	 * The instance of the object being queried. Either objectname +
	 * object class or object instance have to be specified
	 */
	long objectInstance;

	/** Number of levels below the object_name the query affects*/
	int scope;

	/**
	 * Regular expression applied to all nodes affected by the query
	 * in order to decide whether they have to be returned or not
	 */
	std::string filter;

public:
	QueryRIBRequestEvent(const std::string& objectClass,
			const std::string& objectName, long objectInstance,
			int scope, const std::string& filter,
			unsigned int sequenceNumber);
	const std::string& getObjectClass() const;
	const std::string& getObjectName() const;
	long getObjectInstance() const;
	int getScope() const;
	const std::string& getFilter() const;
};

/**
 * Class used by the IPC Processes to interact with the IPC Manager. Extends
 * the basic IPC Manager in librina-application with IPC Process specific
 * functionality
 */
class ExtendedIPCManager: public IPCManager{
	/** The ID of the IPC Process */
	unsigned int ipcProcessId;

	/** The current configuration of the IPC Process */
	DIFConfiguration currentConfiguration;

public:
	const DIFConfiguration& getCurrentConfiguration() const;
	void setCurrentConfiguration(
			const DIFConfiguration& currentConfiguration);
	unsigned int getIpcProcessId() const;
	void setIpcProcessId(unsigned int ipcProcessId);

	/**
	 * Reply to the IPC Manager, informing it about the result of an "assign
	 * to DIF" operation
	 * @param event the event that trigered the operation
	 * @param result the result of the operation (0 successful)
	 * @param errorDescription An optional explanation of the error (if any)
	 */
	void assignToDIFResponse(const AssignToDIFRequestEvent& event, int result,
			const std::string& errorDescription) throw(IPCException);

	/**
	 * Reply to the IPC Manager, informing it about the result of a "register
	 * application request" operation
	 * @param event
	 * @param result
	 * @param errorDescription
	 */
	void registerApplicationResponse(
			const ApplicationRegistrationRequestEvent& event, int result,
			const std::string& errorDescription) throw(IPCException);

	/**
	 * Reply to the IPC Manager, informing it about the result of a "unregister
	 * application request" operation
	 * @param event
	 * @param result
	 * @param errorDescription
	 */
	void unregisterApplicationResponse(
			const ApplicationUnregistrationRequestEvent& event, int result,
			const std::string& errorDescription) throw(IPCException);


	/**
	 * Reply to the IPC Manager, informing it about the result of a "allocate
	 * flow response" operation
	 * @param event
	 * @param result
	 * @param errorDescription
	 */
	void allocateFlowResponse(const FlowRequestEvent& event, int result,
			const std::string& errorDescription) throw(IPCException);
};

/**
 * Make Extended IPC Manager singleton
 */
extern Singleton<ExtendedIPCManager> extendedIPCManager;

/**
 * Class used by IPC Processes to interact with application processes
 */
class IPCProcessApplicationManager {

public:
	/**
	 * Invoked by the IPC Process to respond to the Application Process that
	 * requested a flow deallocation
	 * @param flowDeallocateEvent Object containing information about the flow
	 * deallocate request event
	 * @param result 0 indicates success, a negative number an error code
	 * @param errorDescription optional explanation about the error (if any)
	 * @throws IPCException if there are issues replying ot the application
	 */
	void flowDeallocated(const FlowDeallocateRequestEvent flowDeallocateEvent,
			int result, std::string errorDescription) throw (IPCException);
};

/**
 * Make IPC Process Application Manager singleton
 */
extern Singleton<IPCProcessApplicationManager> ipcProcessApplicationManager;

}

#endif

#endif
