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

#include "common.h"
#include "librina/concurrency.h"
#include "librina/configuration.h"
#include "librina/ipc-daemons.h"
#include "librina/rib_v2.h"

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
                const std::string& pathToLogFolder);

void request_ipcm_finalization(unsigned int localPort);

/** Destroys the IPC Manager
*/
void destroyIPCManager();

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
			unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id);
#ifndef SWIG
	const ApplicationProcessNamingInformation& getApplicationName() const;
	const ApplicationProcessNamingInformation& getDIFName() const;
#endif
};

/**
 * Encapsulates the state and operations that can be performed over
 * a single IPC Process (besides creation/destruction)
 */
class IPCProcessProxy {
public:
	/** The identifier of the IPC Process, unique within the system */
	unsigned short id;

	/** The port at which the IPC Process is listening */
	unsigned int portId;

	/** The OS process identifier */
	pid_t pid;

	/** The IPC Process type */
	std::string type;

	/** The sequence number of the NL message when creating it */
	unsigned int seq_num;

	/** The name of the IPC Process */
	ApplicationProcessNamingInformation name;

	static const std::string error_assigning_to_dif;
	static const std::string error_update_dif_config;
	static const std::string error_registering_app;
	static const std::string error_unregistering_app;
	static const std::string error_not_a_dif_member;
	static const std::string error_allocating_flow;
	static const std::string error_deallocating_flow;
	static const std::string error_querying_rib;

	IPCProcessProxy();
	IPCProcessProxy(unsigned short id, unsigned int portId, pid_t pid, const std::string& type,
			const ApplicationProcessNamingInformation& name);
#ifndef SWIG
	unsigned short getId() const;
	const std::string& getType() const;
	const ApplicationProcessNamingInformation& getName() const;
	unsigned int getPortId() const;
	void setPortId(unsigned int portId);
	pid_t getPid() const;
	void setPid(pid_t pid);
#endif

	/**
	 * Invoked by the IPC Manager to make an existing IPC Process a member of a
	 * DIF. Preconditions: the IPC Process must exist and must not be already a
	 * member of the DIF. Assigning an IPC Process to a DIF will initialize the
	 * IPC Process with all the information required to operate in the DIF (DIF
	 * name, data transfer constants, qos cubes, supported policies, address,
	 * credentials, etc).
	 *
	 * @param difInformation The information of the DIF (name, type configuration)
	 * @param opaque an opaque identifier to correlate requests and responses
	 * @throws AssignToDIFException if an error happens during the process
	 */
	void assignToDIF(const DIFInformation& difInformation, unsigned int opaque);

	/**
	 * Invoked by the IPC Manager to modify the configuration of an existing IPC
	 * process that is a member of a DIF. This oepration doesn't change the
	 * DIF membership, it just changes the configuration of the current DIF.
	 *
	 * @param difConfiguration The configuration of the DIF
	 * @param opaque an opaque identifier to correlate requests and responses
	 * @throws UpdateDIFConfigurationException if an error happens during the process
	 */
	void updateDIFConfiguration(
	                const DIFConfiguration& difConfiguration,
	                unsigned int opaque);

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
	 * Invoked by the IPC Manager to trigger the enrollment of an IPC Process
	 * in the system with a DIF, reachable through a certain N-1 DIF. The
	 * operation blocks until the IPC Process has successfully enrolled or an
	 * error occurs.
	 *
	 * @param difName The DIF that the IPC Process will try to join
	 * @param supportingDifName The supporting DIF used to contact the DIF to
	 * join
	 * @param neighborName The name of the neighbor we're enrolling to
	 * @param opaque an opaque identifier to correlate requests and responses
	 * @throws EnrollException if the enrollment is unsuccessful
	 */
	void enroll(const ApplicationProcessNamingInformation& difName,
			const ApplicationProcessNamingInformation& supportingDifName,
			const ApplicationProcessNamingInformation& neighborName,
			unsigned int opaque);

	/**
	 * Idem to enrollment, but also tell the IPCP to prepare for handover
	 */
	void enroll_prepare_hand(const ApplicationProcessNamingInformation& difName,
				 const ApplicationProcessNamingInformation& supportingDifName,
				 const ApplicationProcessNamingInformation& neighborName,
				 const ApplicationProcessNamingInformation& disc_neigh_name,
				 unsigned int opaque);

	/**
	 * Invoked by the IPC Manager to force an IPC Process to deallocate all the
	 * N-1 flows to a neighbor IPC Process (for example, because it has been
	 * identified as a "rogue" member of the DIF). The operation blocks until the
	 * IPC Process has successfully completed or an error is returned.
	 *
	 * @param neighbor The neighbor to disconnect from
	 * @param opaque an opaque identifier to correlate requests and responses
	 * @throws DisconnectFromNeighborException if an error occurs
	 */
	void disconnectFromNeighbor(const ApplicationProcessNamingInformation& neighbor,
			            unsigned int opaque);

	/**
	 * Invoked by the IPC Manager to register an application in a DIF through
	 * an IPC Process.
	 *
	 * @param applicationName The name of the application to be registered
	 * @param dafName The name of the DAF of the application to be registered (optional)
	 * @param regIpcProcessId The id of the registered IPC process (0 if it
	 * is an application)
	 * @param opaque an opaque identifier to correlate requests and responses
	 * @throws IpcmRegisterApplicationException if an error occurs
	 */
	void registerApplication(const ApplicationProcessNamingInformation& applicationName,
				 const ApplicationProcessNamingInformation& dafName,
				 unsigned short regIpcProcessId,
				 const ApplicationProcessNamingInformation& dif_name,
				 unsigned int opaque);

	/**
	 * Invoked by the IPC Manager to unregister an application in a DIF through
	 * an IPC Process.
	 *
	 * @param applicationName The name of the application to be unregistered
	 * @param opaque an opaque identifier to correlate requests and responses
	 * @throws IpcmUnregisterApplicationException if an error occurs
	 */
	void unregisterApplication(
			const ApplicationProcessNamingInformation& applicationName,
			const ApplicationProcessNamingInformation& dif_name,
			unsigned int opaque);

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
	 * @param opaque an opaque identifier to correlate requests and responses
	 * @throws AllocateFlowException if an error occurs
	 */
	void allocateFlow(const FlowRequestEvent& flowRequest, unsigned int opaque);

	/**
	 * Reply an IPC Process about the fate of a flow allocation request (whether
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
				  int result,
				  bool notifySource,
				  int flowAcceptorIpcProcessId);

	/**
	 * Tell the IPC Process to deallocate a flow
	 * @param portId
	 * @param opaque an opaque identifier to correlate requests and responses
	 * @throws IpcmDeallocateFlowException if there is an error during
	 * the flow deallocation procedure
	 */
	void deallocateFlow(int portId, unsigned int opaque);

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
	 * @param opaque an opaque identifier to correlate requests and responses
	 * @throws QueryRIBException
	 */
	void queryRIB(const std::string& objectClass,
			const std::string& objectName, unsigned long objectInstance,
			unsigned int scope, const std::string& filter,
			unsigned int opaque);

	/**
	 * Invoked by the IPC Manager to change a parameter value in a subcomponent
         * of the IPC process. The parameter addressed by @path can be either a
         * parametric policy or a policy-set-specific parameter.
	 *
	 * @param path The path of the addressed subcomponent (may be a policy-set)
         *             in dotted notation
         * @param name The name of the parameter to be changed
         * @value value The value of the parameter to be changed
         * @param opaque an opaque identifier to correlate requests and responses
	 * @throws SetPolicySetParamException if an error happens during
         *         the process
	 */
	void setPolicySetParam(const std::string& path,
                                       const std::string& name,
                                       const std::string& value,
                                       unsigned int oapque);

	/**
	 * Invoked by the IPC Manager to select a policy-set for a subcomponent
         * of the IPC process.
         *
	 * @param path The path of the addressed subcomponent (cannot be a
         *             policy-set) in dotted notation
         * @param name The name of the policy-set to select
         * @param opaque an opaque identifier to correlate requests and responses
	 * @throws SelectPolicySetException if an error happens during
         *         the process
	 */
	void selectPolicySet(const std::string& path,
                         const std::string& name,
                         unsigned int opaque);

	/**
	 * Invoked by the IPC Manager to load or unload a plugin for an
         * IPC process.
	 *
	 * @param name The name of the plugin to be loaded or unloaded
         * @param load True if the plugin is to be loaded, false if the
         *             plugin is to be unloaded
         * @param opaque an opaque identifier to correlate requests and responses
	 * @throws PluginLoadException if an error happens during
         *         the process
	 */
	void pluginLoad(const std::string& name, bool load,
			unsigned int opaque);

	/**
	 * Invoked by the IPC Manager to forward a CDAPrequest message to
         * IPC process, so that the latter can process the message
	 * through its RIB
	 *
	 * @param sermsg The serialized message
         * @param opaque an opaque identifier to correlate requests and responses
	 * @throws ForwardCDAPException if an error happens during
         *         the process
	 */
	void forwardCDAPRequestMessage(const ser_obj_t& sermsg,
				unsigned int opaque);

        /**
         * Invoked by the IPC Manager to forward a CDAP response message to
         * IPC process, so that the latter can process the message
         * through its RIB
         *
         * @param sermsg The serialized message
         * @param opaque an opaque identifier to correlate requests and responses
         * @throws ForwardCDAPException if an error happens during
         *         the process
         */
        void forwardCDAPResponseMessage(const ser_obj_t& sermsg,
                                unsigned int opaque);

        /**
         * Tell the IPCP to scan the physical media
         */
        void scan_media(void);
};

/**
 * Provides functions to create, destroy and list IPC processes in the
 * system. This class is a singleton.
 */
class IPCProcessFactory {

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
        IPCProcessProxy * create(
                        const ApplicationProcessNamingInformation& ipcProcessName,
                        const std::string& difType,
                        unsigned short ipcProcessId);

        /**
         * Invoked by the IPC Manager to delete an IPC Process from the system. The
         * operation will block until the IPC Process is destroyed or an error is
         * returned.
         *
         * @param ipcProcessId The identifier of the IPC Process to be destroyed
         * @throws DestroyIPCProcessException if an error happens during the operation execution
         */
        unsigned int destroy(IPCProcessProxy* ipcp);
};

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
	 * @param opaque an opaque identifier to correlate requests and responses
	 * @throws AppFlowArrivedException if something goes wrong or the application
	 * doesn't accept the flow
	 */
	void flowRequestArrived(const ApplicationProcessNamingInformation& localAppName,
				const ApplicationProcessNamingInformation& remoteAppName,
				const FlowSpecification& flowSpec,
				const ApplicationProcessNamingInformation& difName,
				int portId, unsigned int opaque, unsigned int ctrl_port);

	/**
	 * Invoked by the IPC Manager to notify that a flow has been remotely
	 * unallocated
	 * @param portId
	 * @param code
	 * @throws NotifyFlowDeallocatedException
	 */
	void flowDeallocatedRemotely(int portId, int code,
				     unsigned int ctrl_port);
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
        IpcmRegisterApplicationResponseEvent(int result,
        		unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id);
};

/**
 * Event informing about the result of an application unregistration
 */
class IpcmUnregisterApplicationResponseEvent: public BaseResponseEvent {
public:
        IpcmUnregisterApplicationResponseEvent(int result,
        		unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id);
};

/**
 * Event informing about the result of a flow allocation
 */
class IpcmAllocateFlowRequestResultEvent: public BaseResponseEvent {

public:
        /** The port id assigned to the flow */
        int portId;

        IpcmAllocateFlowRequestResultEvent(int result, int portId,
        		unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id);
#ifndef SWIG
        int getPortId() const;
#endif
};

/**
 * Event informing about the result of a query RIB operation
 */
class QueryRIBResponseEvent: public BaseResponseEvent {
public:
        std::list<rib::RIBObjectData> ribObjects;

        QueryRIBResponseEvent(const std::list<rib::RIBObjectData>& ribObjects,
                        int result,
                        unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id);
#ifndef SWIG
        const std::list<rib::RIBObjectData>& getRIBObject() const;
#endif
};

/**
 * Event informing about the result of an update DIF config operation
 */
class UpdateDIFConfigurationResponseEvent: public BaseResponseEvent {
public:
        UpdateDIFConfigurationResponseEvent(int result,
        		unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id);
};

/**
 * Event informing about the result of an enroll to DIF operation
 */
class EnrollToDIFResponseEvent: public BaseResponseEvent {
public:
        std::list<Neighbor> neighbors;

        DIFInformation difInformation;

        EnrollToDIFResponseEvent(const std::list<Neighbor> & neighbors,
                        const DIFInformation& difInformation,
                        int result, unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id);
#ifndef SWIG
        const std::list<Neighbor>& getNeighbors() const;
        const DIFInformation& getDIFInformation() const;
#endif
};

class DisconnectNeighborResponseEvent: public BaseResponseEvent {
public:
	DisconnectNeighborResponseEvent(int result,
					unsigned int sequenceNumber,
					unsigned int ctrl_port, unsigned short ipcp_id)
			: BaseResponseEvent(result,
                                            DISCONNECT_NEIGHBOR_RESPONSE_EVENT,
					    sequenceNumber, ctrl_port, ipcp_id) {}
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
                        unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id);
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
        TimerExpiredEvent(unsigned int sequenceNumber,
        		  unsigned int ctrl_p, unsigned short ipcp_id);
};

/**
 * Event informing about a new media report available
 */
class MediaReportEvent: public IPCEvent {
public:
	MediaReportEvent(const MediaReport& report,
			 unsigned int sequenceNumber,
			 unsigned int ctrl_p, unsigned short ipcp_id);

        // The media report resulting from a scan
        MediaReport media_report;
};

class CreateIPCPResponseEvent: public IPCEvent {
public:
	CreateIPCPResponseEvent(int res,
				unsigned int sequenceNumber,
				unsigned int ctrl_p, unsigned short ipcp_id);

        // Result of the operation, 0 success
        int result;
};

class DestroyIPCPResponseEvent: public IPCEvent {
public:
	DestroyIPCPResponseEvent(int res,
				 unsigned int sequenceNumber,
				 unsigned int ctrl_p, unsigned short ipcp_id);

        // Result of the operation, 0 success
        int result;
};

class IPCMFinalizationRequestEvent: public IPCEvent {
public:
	IPCMFinalizationRequestEvent():
		IPCEvent(IPCM_FINALIZATION_REQUEST_EVENT, 0, 0, 0) {};
};

}

#endif

#endif
