/*
 * IPC Process
 *
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
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

#ifndef LIBRINA_IPC_MANAGER_H
#define LIBRINA_IPC_MANAGER_H

#ifdef __cplusplus

#include <map>

#include "librina/concurrency.h"
#include "librina/configuration.h"
#include "librina/ipc-daemons.h"

namespace rina {

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
 * Thrown when there are problems asking the application wether it accepts a new flow
 * or if the application denies it.
 */
class AppFlowArrivedException: public IPCException {
public:
	AppFlowArrivedException():
		IPCException("Problems notifying an application about a new flow allocation request"){
	}
	AppFlowArrivedException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems deallocating a flow
 */
class IpcmDeallocateFlowException: public IPCException {
public:
	IpcmDeallocateFlowException():
		IPCException("Problems deallocating a flow. "){
	}
	IpcmDeallocateFlowException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems notifying about a flow
 * deallocation event
 */
class NotifyFlowDeallocatedException: public IPCException {
public:
	NotifyFlowDeallocatedException():
		IPCException("Problems notifying about flow deallocation. "){
	}
	NotifyFlowDeallocatedException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Initializes the IPC Manager, opening a NL socket
 * to the specified local port, and sending an IPC
 * Manager present message to the kernel
 * @param localPort port of the NL socket
 * @param installationPath path to the IRATI stack installation
 * @param libraryPath path to the installation of librina
 * @param logLevel librina log level
 * @param pathToLogFile the path to the librina log file
 */
void initializeIPCManager(unsigned int localPort,
                const std::string& installationPath,
                const std::string& libraryPath,
                const std::string& logLevel,
                const std::string& pathToLogFile);

/**
 * Event informing that an application has requested the
 * properties of one or more DIFs
 */
class GetDIFPropertiesRequestEvent: public IPCEvent {
public:
	/** The application that wants to get the DIF properties */
	ApplicationProcessNamingInformation applicationName;

	/**
	 * The DIF whose properties are requested. If no DIF name is provided the
	 * IPC Manager will return the properties of all the DIFs visible to the
	 * application
	 */
	ApplicationProcessNamingInformation DIFName;

	GetDIFPropertiesRequestEvent(
			const ApplicationProcessNamingInformation& appName,
			const ApplicationProcessNamingInformation& DIFName,
			unsigned int sequenceNumber);
#ifndef SWIG
	const ApplicationProcessNamingInformation& getApplicationName() const;
	const ApplicationProcessNamingInformation& getDIFName() const;
#endif
};

/**
 * Encapsulates the state and operations that can be performed over
 * a single IPC Process (besides creation/destruction)
 */
class IPCProcess {
	/** The current information of the DIF where the IPC Process is assigned*/
	DIFInformation difInformation;

public:
	/** The identifier of the IPC Process, unique within the system */
	unsigned short id;

	/** The port at which the IPC Process is listening */
	unsigned int portId;

	/** The OS process identifier */
	pid_t pid;

	/** The IPC Process type */
	std::string type;

	/** The name of the IPC Process */
	ApplicationProcessNamingInformation name;

	/** True if the IPC Process is initialized and can start processing operations*/
	bool initialized;

	/** True if the IPC Process is a member of the DIF, false otherwise */
	bool difMember;

	/** True if an assign to DIF operation is in process */
	bool assignInProcess;

	/** True if a configure operation is in process */
	bool configureInProcess;

	/** The configuration that is in progress to be setup */
	DIFConfiguration newConfiguration;

	/** The list of applications registered in this IPC Process */
	std::list<ApplicationProcessNamingInformation> registeredApplications;

	/** The map of pending registrations */
	std::map<unsigned int, ApplicationProcessNamingInformation>
	        pendingRegistrations;

	/** The list of flows currently allocated in this IPC Process */
	std::list<FlowInformation> allocatedFlows;

	/** The map of pending flow operations */
	std::map<unsigned int, FlowInformation> pendingFlowOperations;

	/** The N-1 DIFs where this IPC Process is registered at */
	std::list<ApplicationProcessNamingInformation> nMinusOneDIFs;

	/** The list of neighbors of this IPC Process */
	std::list<Neighbor> neighbors;

	/** Return the information of a registration request */
	ApplicationProcessNamingInformation getPendingRegistration(
	                unsigned int seqNumber);

	/** Return the information of a flow operation */
	FlowInformation getPendingFlowOperation(unsigned int seqNumber);

	static const std::string error_assigning_to_dif;
	static const std::string error_update_dif_config;
	static const std::string error_registering_app;
	static const std::string error_unregistering_app;
	static const std::string error_not_a_dif_member;
	static const std::string error_allocating_flow;
	static const std::string error_deallocating_flow;
	static const std::string error_querying_rib;
	IPCProcess();
	IPCProcess(unsigned short id, unsigned int portId, pid_t pid, const std::string& type,
			const ApplicationProcessNamingInformation& name);
#ifndef SWIG
	unsigned short getId() const;
	const std::string& getType() const;
	const ApplicationProcessNamingInformation& getName() const;
	unsigned int getPortId() const;
	void setPortId(unsigned int portId);
	pid_t getPid() const;
	void setPid(pid_t pid);
	bool isDIFMember() const;
	void setDIFMember(bool difMember);
#endif
	const DIFInformation& getDIFInformation() const;
	void setDIFInformation(const DIFInformation& difInformation);

	/**
	 * Invoked by the IPC Manager to set the IPC Process as initialized.
	 */
	void setInitialized();

	/**
	 * Invoked by the IPC Manager to make an existing IPC Process a member of a
	 * DIF. Preconditions: the IPC Process must exist and must not be already a
	 * member of the DIF. Assigning an IPC Process to a DIF will initialize the
	 * IPC Process with all the information required to operate in the DIF (DIF
	 * name, data transfer constants, qos cubes, supported policies, address,
	 * credentials, etc).
	 *
	 * @param difInformation The information of the DIF (name, type configuration)
	 * @throws AssignToDIFException if an error happens during the process
	 * @returns the handle to the response message
	 */
	unsigned int assignToDIF(
			const DIFInformation& difInformation);

	/**
	 * Update the internal data structures based on the result of the assignToDIF
	 * operation
	 * @param success true if the operation was successful, false otherwise
	 * @throws AssignToDIFException if there was not an assingment operation ongoing
	 */
	void assignToDIFResult(bool success);

	/**
	 * Invoked by the IPC Manager to modify the configuration of an existing IPC
	 * process that is a member of a DIF. This oepration doesn't change the
	 * DIF membership, it just changes the configuration of the current DIF.
	 *
	 * @param difConfiguration The configuration of the DIF
	 * @throws UpdateDIFConfigurationException if an error happens during the process
	 * @returns the handle to the response message
	 */
	unsigned int updateDIFConfiguration(
	                const DIFConfiguration& difConfiguration);

	/**
	 * Update the internal data structures based on the result of the updateConfig
	 * operation
	 * @param success true if the operation was successful, false otherwise
	 * @throws  UpdateDIFConfigurationException if there was no update config
	 * operation ongoing
	 */
	void updateDIFConfigurationResult(bool success);

	/**
	 * Invoked by the IPC Manager to notify an IPC Process that he has been
	 * registered to the N-1 DIF designed by difName
	 *
	 * @param difName The name of the N-1 DIF where the IPC Process has been
	 * registered
	 * @param ipcProcessName the name of the N-1 IPC Process where the IPC Process
	 * has been registered (member of difName)
	 * @throws NotifyRegistrationToDIFException if the IPC Process was already registered to
	 * that DIF
	 */
	void notifyRegistrationToSupportingDIF(
			const ApplicationProcessNamingInformation& ipcProcessName,
			const ApplicationProcessNamingInformation& difName);

	/**
	 * Invoked by the IPC Manager to notify an IPC Process that he has been
	 * unregistered from the N-1 DIF designed by difName
	 *
	 * @param ipcProcessName The name of the N-1 IPC Process where the IPC
         * preocess has been unregistered (member of difName)
	 * @param difName The name of the N-1 DIF where the IPC Process has been
	 * unregistered
	 * @throws NotifyUnregistrationFromDIFException if the IPC Process was not registered to the DIF
	 */
	void notifyUnregistrationFromSupportingDIF(
			const ApplicationProcessNamingInformation& ipcProcessName,
			const ApplicationProcessNamingInformation& difName);

	/**
	 * Return the list of supporting DIFs where this IPC Process is registered at
	 * @return
	 */
	std::list<ApplicationProcessNamingInformation> getSupportingDIFs();

	/**
	 * Invoked by the IPC Manager to trigger the enrollment of an IPC Process
	 * in the system with a DIF, reachable through a certain N-1 DIF. The
	 * operation blocks until the IPC Process has successfully enrolled or an
	 * error occurs.
	 *
	 * @param difName The DIF that the IPC Process will try to join
	 * @param supportingDifName The supporting DIF used to contact the DIF to
	 * join
	 * @param neighborName The name of the neighbor we're enrolling to
	 * @returns the handle to the response message
	 * @throws EnrollException if the enrollment is unsuccessful
	 */
	unsigned int enroll(const ApplicationProcessNamingInformation& difName,
			const ApplicationProcessNamingInformation& supportingDifName,
			const ApplicationProcessNamingInformation& neighborName);

	/**
	 * Add new neighbors of the IPC Process
	 * operation
	 * @param neighbors the new neighbors of the IPC Process
	 */
	void addNeighbors(const std::list<Neighbor>& neighbors);

	/**
	 * Remove existing neighbors of the IPC Process
	 * @param neighbors the neighbors to be removed
	 */
	void removeNeighbors(const std::list<Neighbor>& neighbors);

	/**
	 * Returns the list of neighbors that this IPC Process is currently enrolled
	 * to
	 * @return
	 */
	std::list<Neighbor> getNeighbors();

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
			const ApplicationProcessNamingInformation& neighbor);

	/**
	 * Invoked by the IPC Manager to register an application in a DIF through
	 * an IPC Process.
	 *
	 * @param applicationName The name of the application to be registered
	 * @param regIpcProcessId The id of the registered IPC process (0 if it
	 * is an application)
	 * @throws IpcmRegisterApplicationException if an error occurs
	 * @returns the handle to the response message
	 */
	unsigned int registerApplication(
			const ApplicationProcessNamingInformation& applicationName,
			unsigned short regIpcProcessId);

	/**
	 * Invoked by the IPC Manager to inform about the result of a registration
	 * operation and update the internal data structures
	 * @param sequenceNumber the handle associated to the pending registration
	 * @param success true if success, false otherwise
	 * @throws IpcmRegisterApplicationException if the pending registration
	 * is not found
	 */
	void registerApplicationResult(unsigned int sequenceNumber, bool success);

	/**
	 * Return the list of applications registered in this IPC Process
	 * @return
	 */
	std::list<ApplicationProcessNamingInformation> getRegisteredApplications();

	/**
	 * Invoked by the IPC Manager to unregister an application in a DIF through
	 * an IPC Process.
	 *
	 * @param applicationName The name of the application to be unregistered
	 * @throws IpcmUnregisterApplicationException if an error occurs
	 * @returns the handle to the response message
	 */
	unsigned int unregisterApplication(
			const ApplicationProcessNamingInformation& applicationName);

	/**
	 * Invoked by the IPC Manager to inform about the result of an unregistration
	 * operation and update the internal data structures
	 * @param sequenceNumber the handle associated to the pending unregistration
	 * @param success true if success, false otherwise
	 * @throws IpcmUnregisterApplicationException if the pending unregistration
	 * is not found
	 */
	void unregisterApplicationResult(unsigned int sequenceNumber, bool success);

	/**
	 * Invoked by the IPC Manager to request an IPC Process the allocation of a
	 * flow. Since all flow allocation requests go through the IPC Manager, and
	 * port_ids have to be unique within the whole system, the IPC Manager is
	 * the best candidate for managing the port-id space.
         * TODO parameter description out-dated
	 *
	 * @param flowRequest contains the names of source and destination
	 * applications, the portId as well as the characteristics required for the
	 * flow
	 * @param applicationPortId the port where the application that requested the
	 * flow can be contacted
	 * @returns the handle to the response message
	 * @throws AllocateFlowException if an error occurs
	 */
	unsigned int allocateFlow(const FlowRequestEvent& flowRequest);

	/**
	 * Invoked by the IPC Manager to inform about the result of an allocate
	 * flow operation and update the internal data structures
	 * @param sequenceNumber the handle associated to the pending allocation
	 * @param success true if success, false otherwise
	 * @param portId the portId assigned to the flow
	 * @throws AllocateFlowException if the pending allocation
	 * is not found
	 */
	void allocateFlowResult(unsigned int sequenceNumber, bool success, int portId);

	/**
	 * Reply an IPC Process about the fate of a flow allocation request (wether
	 * it has been accepted or denied by the application). If it has been
	 * accepted, communicate the portId to the IPC Process
	 * @param flowRequest
	 * @param result 0 if the request is accepted, negative number indicating error
	 * otherwise
	 * @param notifySource true if the IPC Process has to reply to the source, false
	 * otherwise
	 * @param flowAcceptorIpcProcessId the IPC Process id of the Process that accepted
	 * or rejected the flow (0 if it is an application)
	 * @throws AllocateFlowException if something goes wrong
	 */
	void allocateFlowResponse(const FlowRequestEvent& flowRequest,
			int result, bool notifySource,
			int flowAcceptorIpcProcessId);

	/**
	 * Return the list of flows allocated by this IPC Process
	 * @return
	 */
	std::list<FlowInformation> getAllocatedFlows();

	/**
	 * Get the information of the flow identified by portId
         * @result will contain the flow identified by portId, if any
	 * @return true if the flow is found, false otherwise
	 */
	bool getFlowInformation(int portId, FlowInformation& result);

	/**
	 * Tell the IPC Process to deallocate a flow
	 * @param portId
	 * @throws IpcmDeallocateFlowException if there is an error during
	 * the flow deallocation procedure
	 * @returns the handle to the response message
	 */
	unsigned int deallocateFlow(int portId);

	/**
	 * Invoked by the IPC Manager to inform about the result of a deallocate
	 * flow operation and update the internal data structures
	 * @param sequenceNumber the handle associated to the pending allocation
	 * @param success true if success, false otherwise
	 * @throws IpcmDeallocateFlowException if the pending deallocation
	 * is not found
	 */
	void deallocateFlowResult(unsigned int sequenceNumber, bool success);

	/**
	 * Invoked by the IPC Manager to notify that a flow has been remotely
	 * deallocated, so that librina updates the internal data structures
	 * @returns the information of the flow deallocated
	 * @throws IpcmDeallocateFlowException if now flow with the given
	 * portId is found
	 */
	FlowInformation flowDeallocated(int portId);

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
	 * @returns the handle to the response message
	 * @throws QueryRIBException
	 */
	unsigned int queryRIB(const std::string& objectClass,
			const std::string& objectName, unsigned long objectInstance,
			unsigned int scope, const std::string& filter);
};

/**
 * Provides functions to create, destroy and list IPC processes in the
 * system. This class is a singleton.
 */
class IPCProcessFactory: public Lockable {

        /** The current IPC Processes in the system*/
        std::map<unsigned short, IPCProcess*> ipcProcesses;

public:
        static const std::string unknown_ipc_process_error;
        static const std::string path_to_ipc_process_types;
        static const std::string normal_ipc_process_type;

        IPCProcessFactory();
        ~IPCProcessFactory() throw();

        /**
         * Read the sysfs folder and get the list of IPC Process types supported
         * by the kernel
         * @return the list of supported IPC Process types
         */
        std::list<std::string> getSupportedIPCProcessTypes();

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
                        const std::string& difType);

        /**
         * Invoked by the IPC Manager to delete an IPC Process from the system. The
         * operation will block until the IPC Process is destroyed or an error is
         * returned.
         *
         * @param ipcProcessId The identifier of the IPC Process to be destroyed
         * @throws DestroyIPCProcessException if an error happens during the operation execution
         */
        void destroy(unsigned short ipcProcessId);

        /**
         * Returns a list to all the IPC Processes that are currently running in
         * the system.
         *
         * @return list<IPCProcess *> A list of the IPC Processes in the system
         */
        std::vector<IPCProcess *> listIPCProcesses();

        /**
         * Returns a pointer to the IPCProcess identified by ipcProcessId
         * @param ipcProcessId
         * @return a pointer to an IPC Process or NULL if no IPC Process
         * with the specified id is found
         */
        IPCProcess * getIPCProcess(unsigned short ipcProcessId);
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
	 * @param response The result of the registration operation
	 * @throws NotifyApplicationRegisteredException If an error occurs during the operation
	 */
	void applicationRegistered(const ApplicationRegistrationRequestEvent & event,
			const ApplicationProcessNamingInformation& difName, int result);

	/**
	 * Invoked by the IPC Manager to notify an application about the  result of
	 * the unregistration in a DIF operation.
	 *
	 * @param response The result of the unregistration operation
	 * @throws NotifyApplicationUnregisteredException If an error occurs during the operation
	 */
	void applicationUnregistered(const ApplicationUnregistrationRequestEvent & event,
			int result);

	/**
	 * Invoked by the IPC Manager to respond to the Application Process that
	 * requested a flow.
	 *
	 * @param flowRequestEvent Object containing information about the flow
	 * request
	 * @throws NotifyFlowAllocatedException If an error occurs during the operation
	 */
	void flowAllocated(const FlowRequestEvent &flowRequestEvent);

	/**
	 * Invoked by the IPC Manager to inform the Application Process that a remote
	 * application wants to allocate a flow to it. The remote application will
	 * decide whether it accepts or not the flow
	 * @param localAppName
	 * @param remoteAppName
	 * @param flowSpec
	 * @param difName
	 * @param portId
	 * @throws AppFlowArrivedException if something goes wrong or the application
	 * doesn't accept the flow
	 * @returns the handle to be able to identify the applicaiton response when
	 * it arrives
	 */
	unsigned int flowRequestArrived(
			const ApplicationProcessNamingInformation& localAppName,
			const ApplicationProcessNamingInformation& remoteAppName,
			const FlowSpecification& flowSpec,
			const ApplicationProcessNamingInformation& difName,
			int portId);

	/**
	 * Inform the application about the result of a flow deallocation operation
	 * @param event
	 * @param result
	 * @throws NotifyFlowDeallocatedException
	 */
	void flowDeallocated(const FlowDeallocateRequestEvent& event, int result);

	/**
	 * Invoked by the IPC Manager to notify that a flow has been remotely
	 * unallocated
	 * @param portId
	 * @param code
	 * @throws NotifyFlowDeallocatedException
	 */
	void flowDeallocatedRemotely(int portId, int code,
			const ApplicationProcessNamingInformation& appName);

	/**
	 * Return the properties of zero or more DIFs to the application
	 * @param event the event containing the query
	 * @param result 0 if the operation was successful, a negative integer
	 * otherwise
	 * @param difProperties The properties of zero or more DIFs
	 * @throws GetDIFPropertiesResponseException
	 */
	void getDIFPropertiesResponse(const GetDIFPropertiesRequestEvent& event,
			int result, const std::list<DIFProperties>& difProperties);
};

/**
 * Make Application Manager singleton
 */
extern Singleton<ApplicationManager> applicationManager;

/**
 * Event informing about the result of an application registration
 */
class IpcmRegisterApplicationResponseEvent: public BaseResponseEvent {
public:
        IpcmRegisterApplicationResponseEvent(
                        int result, unsigned int sequenceNumber);
};

/**
 * Event informing about the result of an application unregistration
 */
class IpcmUnregisterApplicationResponseEvent: public BaseResponseEvent {
public:
        IpcmUnregisterApplicationResponseEvent(
                        int result, unsigned int sequenceNumber);
};

/**
 * Event informing about the result of a flow deallocation
 */
class IpcmDeallocateFlowResponseEvent: public BaseResponseEvent {
public:
        IpcmDeallocateFlowResponseEvent(
                        int result, unsigned int sequenceNumber);
};

/**
 * Event informing about the result of a flow allocation
 */
class IpcmAllocateFlowRequestResultEvent: public BaseResponseEvent {

public:
        /** The port id assigned to the flow */
        int portId;

        IpcmAllocateFlowRequestResultEvent(
                        int result, int portId, unsigned int sequenceNumber);
#ifndef SWIG
        int getPortId() const;
#endif
};

/**
 * Event informing about the result of a query RIB operation
 */
class QueryRIBResponseEvent: public BaseResponseEvent {
public:
        std::list<RIBObjectData> ribObjects;

        QueryRIBResponseEvent(const std::list<RIBObjectData>& ribObjects,
                        int result,
                        unsigned int sequenceNumber);
#ifndef SWIG
        const std::list<RIBObjectData>& getRIBObject() const;
#endif
};

/**
 * Event informing about the result of an assign to DIF operation
 */
class AssignToDIFResponseEvent: public BaseResponseEvent {
public:
        AssignToDIFResponseEvent(
                        int result, unsigned int sequenceNumber);
};

/**
 * Event informing about the result of an update DIF config operation
 */
class UpdateDIFConfigurationResponseEvent: public BaseResponseEvent {
public:
        UpdateDIFConfigurationResponseEvent(
                        int result, unsigned int sequenceNumber);
};

/**
 * Event informing about the result of an enroll to DIF operation
 */
class EnrollToDIFResponseEvent: public BaseResponseEvent {
public:
        std::list<Neighbor> neighbors;

        DIFInformation difInformation;

        EnrollToDIFResponseEvent(
                        const std::list<Neighbor> & neighbors,
                        const DIFInformation& difInformation,
                        int result, unsigned int sequenceNumber);
#ifndef SWIG
        const std::list<Neighbor>& getNeighbors() const;
        const DIFInformation& getDIFInformation() const;
#endif
};

/**
 * Event informing about new neighbors being added or existing
 * neighbors being removed
 */
class NeighborsModifiedNotificationEvent: public IPCEvent {
public:
        unsigned short ipcProcessId;
        std::list<Neighbor> neighbors;
        bool added;

        NeighborsModifiedNotificationEvent(
                        unsigned short ipcProcessId,
                        const std::list<Neighbor> & neighbors,
                        bool added, unsigned int sequenceNumber);
#ifndef SWIG
        const std::list<Neighbor>& getNeighbors() const;
        bool isAdded() const;
        unsigned short getIpcProcessId() const;
#endif
};

/**
 * Event informing about the successful initialization of an IPC Process
 * Daemon
 */
class IPCProcessDaemonInitializedEvent: public IPCEvent {
public:
        unsigned short ipcProcessId;
        ApplicationProcessNamingInformation name;

        IPCProcessDaemonInitializedEvent(unsigned short ipcProcessId,
                        const ApplicationProcessNamingInformation& name,
                        unsigned int sequenceNumber);
#ifndef SWIG
        unsigned short getIPCProcessId() const;
        const ApplicationProcessNamingInformation& getName() const;
#endif
};

/**
 * Event informing about the expiration of a timer
 */
class TimerExpiredEvent: public IPCEvent {
public:
        TimerExpiredEvent(unsigned int sequenceNumber);
};


}

#endif

#endif
