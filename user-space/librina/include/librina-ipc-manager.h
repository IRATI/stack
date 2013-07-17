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

#ifndef LIBRINA_IPC_MANAGER_H
#define LIBRINA_IPC_MANAGER_H

#ifdef __cplusplus

#include <map>

#include "librina-common.h"

namespace rina {

/**
 * Thrown when there are problems assigning an IPC Process to a DIF
 */
class AssignToDIFException: public IPCException {
public:
	AssignToDIFException():
		IPCException("Problems assigning ICP Process to DIF"){
	}
	AssignToDIFException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems notifying an IPC Process that it has been
 * registered to an N-1 DIF
 */
class NotifyRegistrationToDIFException: public IPCException {
public:
	NotifyRegistrationToDIFException():
		IPCException("Problems notifying an IPC Process that it has been registered to an N-1 DIF"){
	}
	NotifyRegistrationToDIFException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems notifying an IPC Process that it has been
 * unregistered from an N-1 DIF
 */
class NotifyUnregistrationFromDIFException: public IPCException {
public:
	NotifyUnregistrationFromDIFException():
		IPCException("Problems notifying an IPC Process that it has been unregistered from an N-1 DIF"){
	}
	NotifyUnregistrationFromDIFException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems instructing an IPC Process to enroll to a DIF
 */
class EnrollException: public IPCException {
public:
	EnrollException():
		IPCException("Problems causing an IPC Process to enroll to a DIF"){
	}
	EnrollException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems instructing an IPC Process to disconnect
 * from a neighbour
 */
class DisconnectFromNeighborException: public IPCException {
public:
	DisconnectFromNeighborException():
		IPCException("Problems causing an IPC Process to disconnect from a neighbour"){
	}
	DisconnectFromNeighborException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems instructing an IPC Process to register an
 * application
 */
class IpcmRegisterApplicationException: public IPCException {
public:
	IpcmRegisterApplicationException():
		IPCException("Problems while the IPC Process was trying to register an application"){
	}
	IpcmRegisterApplicationException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems instructing an IPC Process to unregister an
 * application
 */
class IpcmUnregisterApplicationException: public IPCException {
public:
	IpcmUnregisterApplicationException():
		IPCException("Problems while the IPC Process was trying to unregister an application"){
	}
	IpcmUnregisterApplicationException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems instructing an IPC Process to allocate a flow
 */
class AllocateFlowException: public IPCException {
public:
	AllocateFlowException():
		IPCException("Problems while the IPC Process was trying to allocate a flow"){
	}
	AllocateFlowException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems while querying the RIB of an IPC Process
 */
class QueryRIBException: public IPCException {
public:
	QueryRIBException():
		IPCException("Problems while querying the RIB of an IPC Process"){
	}
	QueryRIBException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems creating an IPC Process
 */
class CreateIPCProcessException: public IPCException {
public:
	CreateIPCProcessException():
		IPCException("Problems while creating an IPC Process"){
	}
	CreateIPCProcessException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems destroying an IPC Process
 */
class DestroyIPCProcessException: public IPCException {
public:
	DestroyIPCProcessException():
		IPCException("Problems while destroying an IPC Process"){
	}
	DestroyIPCProcessException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems notifying an application that it
 * has been registered to a DIF
 */
class NotifyApplicationRegisteredException: public IPCException {
public:
	NotifyApplicationRegisteredException():
		IPCException("Problems notifying an application about its registration to a DIF"){
	}
	NotifyApplicationRegisteredException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems notifying an application that it
 * has been unregistered from a DIF
 */
class NotifyApplicationUnregisteredException: public IPCException {
public:
	NotifyApplicationUnregisteredException():
		IPCException("Problems notifying an application about its unregistration from a DIF"){
	}
	NotifyApplicationUnregisteredException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems notifying an application that a flow
 * has been allocated
 */
class NotifyFlowAllocatedException: public IPCException {
public:
	NotifyFlowAllocatedException():
		IPCException("Problems notifying an application about the allocation of a flow"){
	}
	NotifyFlowAllocatedException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems notifying an application about the results
 * of a query of the properties of one ore more DIFs
 */
class GetDIFPropertiesResponseException: public IPCException {
public:
	GetDIFPropertiesResponseException():
		IPCException("Problems notifying an application about the query of DIF properties"){
	}
	GetDIFPropertiesResponseException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Event informing that an application has requested the
 * properties of one or more DIFs
 */
class GetDIFPropertiesRequestEvent: public IPCEvent {
	/** The application that wants to get the DIF properties */
	ApplicationProcessNamingInformation applicationName;

	/**
	 * The DIF whose properties are requested. If no DIF name is provided the
	 * IPC Manager will return the properties of all the DIFs visible to the
	 * application
	 */
	ApplicationProcessNamingInformation DIFName;

public:
	GetDIFPropertiesRequestEvent(
			const ApplicationProcessNamingInformation& appName,
			const ApplicationProcessNamingInformation& DIFName,
			unsigned int sequenceNumber);
	const ApplicationProcessNamingInformation& getApplicationName() const;
	const ApplicationProcessNamingInformation& getDIFName() const;
};

/**
 * Encapsulates the state and operations that can be performed over
 * a single IPC Process (besides creation/destruction)
 */
class IPCProcess {

	/** The identifier of the IPC Process, unique within the system */
	unsigned short id;

	/** The port at which the IPC Process is listening */
	unsigned int portId;

	/** The IPC Process type */
	DIFType type;

	/** The name of the IPC Process */
	ApplicationProcessNamingInformation name;

	/** The current configuration of the IPC Process*/
	DIFConfiguration difConfiguration;

	/** True if the IPC Process is a member of the DIF, false otherwise */
	bool difMember;

public:
	static const std::string error_assigning_to_dif;
	static const std::string error_registering_app;
	static const std::string error_unregistering_app;
	static const std::string error_not_a_dif_member;
	static const std::string error_allocating_flow;
	static const std::string error_querying_rib;
	IPCProcess();
	IPCProcess(unsigned short id, unsigned int portId, DIFType type,
			const ApplicationProcessNamingInformation& name);
	unsigned int getId() const;
	DIFType getType() const;
	const ApplicationProcessNamingInformation& getName() const;
	unsigned int getPortId() const;
	void setPortId(unsigned int portId);
	const DIFConfiguration& getConfiguration() const;
	void setConfiguration(const DIFConfiguration& difConfiguration);
	bool isDIFMember() const;
	void setDIFMember(bool difMember);

	/**
	 * Invoked by the IPC Manager to make an existing IPC Process a member of a
	 * DIF. Preconditions: the IPC Process must exist and must not be already a
	 * member of the DIF. Assigning an IPC Process to a DIF will initialize the
	 * IPC Process with all the information required to operate in the DIF (DIF
	 * name, data transfer constants, qos cubes, supported policies, address,
	 * credentials, etc). The operation will block until the IPC Process is
	 * assigned to the DIF or an error is returned.
	 *
	 * @param difConfiguration The configuration of the DIF
	 * @throws AssignToDIFException if an error happens during the process
	 */
	void assignToDIF(
			const DIFConfiguration& difConfiguration) throw (AssignToDIFException);

	/**
	 * Invoked by the IPC Manager to notify an IPC Process that he has been
	 * registered to the N-1 DIF designed by difName
	 *
	 * @param ipcProcessName The name of the IPC Process being registered
	 * @param difName The name of the N-1 DIF where the IPC Process has been
	 * registered
	 * @throws NotifyRegistrationToDIFException if the IPC Process was already registered to
	 * that DIF
	 */
	void notifyRegistrationToSupportingDIF(
			const ApplicationProcessNamingInformation& ipcProcessName,
			const ApplicationProcessNamingInformation& difName)
	throw (NotifyRegistrationToDIFException);

	/**
	 * Invoked by the IPC Manager to notify an IPC Process that he has been
	 * unregistered from the N-1 DIF designed by difName
	 *
	 * @param ipcProcessName The name of the IPC Process being unregistered
	 * @param difName The name of the N-1 DIF where the IPC Process has been
	 * unregistered
	 * @throws NotifyUnregistrationFromDIFException if the IPC Process was not registered to the DIF
	 */
	void notifyUnregistrationFromSupportingDIF(
			const ApplicationProcessNamingInformation& ipcProcessName,
			const ApplicationProcessNamingInformation& difName)
	throw (NotifyUnregistrationFromDIFException);

	/**
	 * Invoked by the IPC Manager to trigger the enrollment of an IPC Process
	 * in the system with a DIF, reachable through a certain N-1 DIF. The
	 * operation blocks until the IPC Process has successfully enrolled or an
	 * error occurs.
	 *
	 * @param difName The DIF that the IPC Process will try to join
	 * @param supportingDifName The supporting DIF used to contact the DIF to
	 * join
	 * @throws EnrollException if the enrollment is unsuccessful
	 */
	void enroll(const ApplicationProcessNamingInformation& difName,
			const ApplicationProcessNamingInformation& supportinDifName)
	throw (EnrollException);

	/**
	 * Invoked by the IPC Manager to force an IPC Process to deallocate all the
	 * N-1 flows to a neighbor IPC Process (for example, because it has been
	 * identified as a "rogue" member of the DIF). The operation blocks until the
	 * IPC Process has successfully completed or an error is returned.
	 *
	 * @param neighbor The neighbor to disconnect from
	 * @throws DisconnectFromNeighborException if an error occurs
	 */
	void disconnectFromNeighbor(
			const ApplicationProcessNamingInformation& neighbor)
	throw (DisconnectFromNeighborException);

	/**
	 * Invoked by the IPC Manager to register an application in a DIF through
	 * an IPC Process. The operation blocks until the IPC Process has
	 * successfully registered the application or an error occurs.
	 *
	 * @param applicationName The name of the application to be registered
	 * @param applicationPordId The port where the application can be contacted
	 * @throws IpcmRegisterApplicationException if an error occurs
	 */
	void registerApplication(
			const ApplicationProcessNamingInformation& applicationName,
			unsigned int applicationPortId)
	throw (IpcmRegisterApplicationException);

	/**
	 * Invoked by the IPC Manager to unregister an application in a DIF through
	 * an IPC Process. The operation blocks until the IPC Process has
	 * successfully unregistered the application or an error occurs.
	 *
	 * @param applicationName The name of the application to be unregistered
	 * @throws IpcmUnregisterApplicationException if an error occurs
	 */
	void unregisterApplication(
			const ApplicationProcessNamingInformation& applicationName)
	throw (IpcmUnregisterApplicationException);

	/**
	 * Invoked by the IPC Manager to request an IPC Process the allocation of a
	 * flow. Since all flow allocation requests go through the IPC Manager, and
	 * port_ids have to be unique within the whole system, the IPC Manager is
	 * the best candidate for managing the port-id space.
	 *
	 * @param flowRequest contains the names of source and destination
	 * applications, the portId as well as the characteristics required for the
	 * flow
	 * @param applicationPortId the port where the application that requested the
	 * flow can be contacted
	 * @throws AllocateFlowException if an error occurs
	 */
	void allocateFlow(const FlowRequestEvent& flowRequest,
			unsigned int applicationPortId) throw (AllocateFlowException);

	/**
	 * Invoked by the IPC Manager to query a subset of the RIB of the IPC
	 * Process.
	 * @param objectClass the queried object class
	 * @param objectName the queried object name
	 * @param objectInstance the queried object instance (either objecClass +
	 * objectName or objectInstance have to be specified)
	 * @param scope the amount of levels in the RIB tree -starting at the
	 * base object - that are affected by the query
	 * @param filter An expression evaluated for each object, to determine
	 * wether the object should be returned by the query
	 * @return A list contaning zero or more RIB objects
	 */
	const std::list<RIBObject> queryRIB(const std::string& objectClass,
			const std::string& objectName, unsigned long objectInstance,
			unsigned int scope, const std::string& filter)
					throw (QueryRIBException);
};

/**
 * Provides functions to create, destroy and list IPC processes in the
 * system. This class is a singleton.
 */
class IPCProcessFactory {

	/** The current IPC Processes in the system*/
	std::map<int, IPCProcess*> ipcProcesses;

public:
	static const std::string unknown_ipc_process_error;

	/**
	 * Invoked by the IPC Manager to instantiate a new IPC Process in the
	 * system. The operation will block until the IPC Process is created or an
	 * error is returned.
	 *
	 * @param ipcProcessId An identifier that uniquely identifies an IPC Process
	 * within a aystem.
	 * @param ipcProcessName The naming information of the IPC Process
	 * @param difType The type of IPC Process (Normal or one of the shims)
	 * @return a pointer to a data structure holding the IPC Process state
	 * @throws CreateIPCProcessException if an error happens during the creation
	 */
	IPCProcess * create(
			const ApplicationProcessNamingInformation& ipcProcessName,
			DIFType difType) throw (CreateIPCProcessException);

	/**
	 * Invoked by the IPC Manager to delete an IPC Process from the system. The
	 * operation will block until the IPC Process is destroyed or an error is
	 * returned.
	 *
	 * @param ipcProcessId The identifier of the IPC Process to be destroyed
	 * @throws DestroyIPCProcessException if an error happens during the operation execution
	 */
	void destroy(unsigned int ipcProcessId) throw (DestroyIPCProcessException);

	/**
	 * Returns a list to all the IPC Processes that are currently running in
	 * the system.
	 *
	 * @return vector<IPCProcess *> A list of the IPC Processes in the system
	 */
	std::vector<IPCProcess *> listIPCProcesses();
};

/**
 * Make IPC Process Factory singleton
 */
extern Singleton<IPCProcessFactory> ipcProcessFactory;

/**
 * Class to interact with application processes
 */
class ApplicationManager {
public:

	/**
	 * Invoked by the IPC Manager to notify an application about the  result of
	 * the registration in a DIF operation.
	 *
	 * @param sequenceNumber identifies the request whose reply this operation
	 * is providing
	 * @param response The result of the registration operation
	 * @throws NotifyApplicationRegisteredException If an error occurs during the operation
	 */
	void applicationRegistered(const ApplicationRegistrationRequestEvent & event,
			unsigned short ipcProcessId, int ipcProcessPortId, int result,
			const std::string& errorDescription)
				throw (NotifyApplicationRegisteredException);

	/**
	 * Invoked by the IPC Manager to notify an application about the  result of
	 * the unregistration in a DIF operation.
	 *
	 * @param transactionId identifies the request whose reply this operation
	 * is providing
	 * @param response The result of the unregistration operation
	 * @throws NotifyApplicationUnregisteredException If an error occurs during the operation
	 */
	void applicationUnregistered(const ApplicationUnregistrationRequestEvent & event,
			int result, const std::string& errorDescription)
				throw (NotifyApplicationUnregisteredException);

	/**
	 * Invoked by the IPC Manager to respond to the Application Process that
	 * requested a flow.
	 *
	 * @param flowRequestEvent Object containing information about the flow
	 * request
	 * @param errorDescription An optional string further describing the result
	 * @param ipcProcessId Required so that the application process can contact
	 * the IPC Process to read/write the flow
	 * @param ipcProcessPortId Required so that the application process can contact
	 * the IPC Process to read/write the flow
	 * @throws NotifyFlowAllocatedException If an error occurs during the operation
	 */
	void flowAllocated(const FlowRequestEvent &flowRequestEvent,
			std::string errorDescription,
			unsigned short ipcProcessId, unsigned int ipcProcessPortId)
				throw (NotifyFlowAllocatedException);

	/**
	 * Return the properties of zero or more DIFs to the application
	 * @param event the event containing the query
	 * @param result 0 if the operation was successful, a negative integer
	 * otherwise
	 * @param errorDescription More explanation about the errors (if any)
	 * @param difProperties The properties of zero or more DIFs
	 * @throws GetDIFPropertiesResponseException
	 */
	void getDIFPropertiesResponse(const GetDIFPropertiesRequestEvent& event,
			int result, const std::string& errorDescription,
			const std::list<DIFProperties>& difProperties)
			throw (GetDIFPropertiesResponseException);
};

/**
 * Make Application Manager singleton
 */
extern Singleton<ApplicationManager> applicationManager;

}

#endif

#endif
