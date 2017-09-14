/*
 * IPCM IPCP related classes
 *
 *    Marc Sune <marc.sune (at) bisdn.de>
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
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

#ifndef __IPCP_H__
#define __IPCP_H__

#include <assert.h>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
#include <utility>
#include <list>
#include <string>

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/patterns.h>

namespace rinad {

//fwd decl
class IPCMIPCProcessFactory;

/**
 * Encapsulates the state and operations that can be performed over
 * a single IPC Process (besides creation/destruction)
 */
class IPCMIPCProcess {

public:
	enum State{IPCM_IPCP_CREATED,
		   IPCM_IPCP_INITIALIZED,
		   IPCM_IPCP_ASSIGN_TO_DIF_IN_PROGRESS,
		   IPCM_IPCP_ASSIGNED_TO_DIF};

	/** The current information of the DIF where the IPC Process is assigned*/
	rina::ApplicationProcessNamingInformation dif_name_;

	/** The list of flows currently allocated in this IPC Process */
	std::list<rina::FlowInformation> allocatedFlows;

	/** The list of applications registered in this IPC Process */
	std::list<rina::ApplicationRegistrationInformation> registeredApplications;

	/** The list of neighbors of this IPC Process */
	std::list<rina::Neighbor> neighbors;

	/** Rwlock */
	rina::ReadWriteLockable rwlock;

	/** The IPC Process proxy class */
	rina::IPCProcessProxy* proxy_;

	bool kernel_ready;

	//Constructors and destructurs

	IPCMIPCProcess();
	IPCMIPCProcess(rina::IPCProcessProxy* ipcp_proxy);
	~IPCMIPCProcess() throw();

	/**
	* Get the IPCP id
	*/
	inline unsigned short get_id(void) const{
		return proxy_->id;
	}

	/**
	* Get the IPCP name
	*/
	inline rina::ApplicationProcessNamingInformation get_name(void) const{
		return proxy_->name;
	}

	/**
	* Get the IPCP type
	*/
	inline std::string get_type(void) const{
		return proxy_->type;
	}
	const rina::ApplicationProcessNamingInformation& getDIFName() const;

	/**
	* Get the IPCP state
	*/
	inline State get_state(void) const{
		return state_;
	}

	std::list<rina::ApplicationProcessNamingInformation> get_neighbors_with_n1dif(const rina::ApplicationProcessNamingInformation& dif_name);

	void get_description(std::ostream& os);

	/**
	 * Invoked by the IPC Manager to set the IPC Process as initialized.
	 * This method is NOT thread safe and must be called with the writelock
	 * acquired
	 */
	void setInitialized();

	/**
	 * Get ctrl port where to send the flow allocate request (to the local app)
	 */
	unsigned int get_fa_ctrl_port(const rina::ApplicationProcessNamingInformation& reg_app);

	/**
	 * Invoked by the IPC Manager to make an existing IPC Process a member of a
	 * DIF. Preconditions: the IPC Process must exist and must not be already a
	 * member of the DIF. Assigning an IPC Process to a DIF will initialize the
	 * IPC Process with all the information required to operate in the DIF (DIF
	 * name, data transfer constants, qos cubes, supported policies, address,
	 * credentials, etc).
	 * This method is NOT thread safe and must be called with the writelock
	 * acquired
	 *
	 * @param difInformation The information of the DIF (name, type configuration)
	 * @param opaque an opaque identifier to correlate requests and responses
	 * @throws AssignToDIFException if an error happens during the process
	 */
	void assignToDIF(
			const rina::DIFInformation& difInformation, unsigned int opaque);

	/**
	 * Update the internal data structures based on the result of the assignToDIF
	 * operation
	 * This method is NOT thread safe and must be called with the writelock
	 * acquired
	 *
	 * @param success true if the operation was successful, false otherwise
	 * @throws AssignToDIFException if there was not an assingment operation ongoing
	 */
	void assignToDIFResult(bool success);

	/**
	 * Invoked by the IPC Manager to modify the configuration of an existing IPC
	 * process that is a member of a DIF. This oepration doesn't change the
	 * DIF membership, it just changes the configuration of the current DIF.
	 * This method is NOT thread safe and must be called with the writelock
	 * acquired
	 *
	 * @param difConfiguration The configuration of the DIF
	 * @param opaque an opaque identifier to correlate requests and responses
	 * @throws UpdateDIFConfigurationException if an error happens during the process
	 */
	void updateDIFConfiguration(
	                const rina::DIFConfiguration& difConfiguration,
	                unsigned int opaque);

	void notifyRegistrationToSupportingDIF(
			const rina::ApplicationProcessNamingInformation& ipcProcessName,
			const rina::ApplicationProcessNamingInformation& difName);

	void notifyUnregistrationFromSupportingDIF(
			const rina::ApplicationProcessNamingInformation& ipcProcessName,
			const rina::ApplicationProcessNamingInformation& difName);

	/**
	 * Invoked by the IPC Manager to trigger the enrollment of an IPC Process
	 * in the system with a DIF, reachable through a certain N-1 DIF. The
	 * operation blocks until the IPC Process has successfully enrolled or an
	 * error occurs.
	 * This method is NOT thread safe and must be called with the writelock
	 * acquired
	 *
	 *
	 * @param difName The DIF that the IPC Process will try to join
	 * @param supportingDifName The supporting DIF used to contact the DIF to
	 * join
	 * @param neighborName The name of the neighbor we're enrolling to
	 * @param opaque an opaque identifier to correlate requests and responses
	 * @throws EnrollException if the enrollment is unsuccessful
	 */
	void enroll(const rina::ApplicationProcessNamingInformation& difName,
			const rina::ApplicationProcessNamingInformation& supportingDifName,
			const rina::ApplicationProcessNamingInformation& neighborName,
			unsigned int opaque);

	void enroll_prepare_handover(const rina::ApplicationProcessNamingInformation& difName,
				     const rina::ApplicationProcessNamingInformation& supportingDifName,
				     const rina::ApplicationProcessNamingInformation& neighborName,
				     const rina::ApplicationProcessNamingInformation& disc_neigh_name,
				     unsigned int opaque);

	/**
	 * Invoked by the IPC Manager to force an IPC Process to deallocate all the
	 * N-1 flows to a neighbor IPC Process (for example, because it has been
	 * identified as a "rogue" member of the DIF). The operation blocks until the
	 * IPC Process has successfully completed or an error is returned.
	 * This method is NOT thread safe and must be called with the writelock
	 * acquired
	 *
	 * @param neighbor The neighbor to disconnect from
	 * @throws DisconnectFromNeighborException if an error occurs
	 */
	void disconnectFromNeighbor(const rina::ApplicationProcessNamingInformation& neighbor,
				    unsigned int opaque);

	/**
	 * Invoked by the IPC Manager to register an application in a DIF through
	 * an IPC Process.
	 * This method is NOT thread safe and must be called with the writelock
	 * acquired
	 *
	 * @param applicationName The name of the application to be registered
	 * @param dafName The name of the DAF of the application to be registered (optional)
	 * @param regIpcProcessId The id of the registered IPC process (0 if it
	 * is an application)
	 * @param opaque an opaque identifier to correlate requests and responses
	 * @throws IpcmRegisterApplicationException if an error occurs
	 */
	void registerApplication(const rina::ApplicationRegistrationInformation& ari,
				 unsigned int opaque);

	/**
	 * Invoked by the IPC Manager to inform about the result of a registration
	 * operation and update the internal data structures
	 * This method is NOT thread safe and must be called with the writelock
	 * acquired
	 *
	 * @param sequenceNumber the handle associated to the pending registration
	 * @param success true if success, false otherwise
	 * @throws IpcmRegisterApplicationException if the pending registration
	 * is not found
	 */
	void registerApplicationResult(unsigned int sequenceNumber, bool success);

	void disconnectFromNeighborResult(unsigned int sequenceNumber, bool success);

	void add_neighbors(const std::list<rina::Neighbor> & neighbors);

	/**
	 * Invoked by the IPC Manager to unregister an application in a DIF through
	 * an IPC Process.
	 * This method is NOT thread safe and must be called with the writelock
	 * acquired
	 *
	 * @param applicationName The name of the application to be unregistered
	 * @param opaque an opaque identifier to correlate requests and responses
	 * @throws IpcmUnregisterApplicationException if an error occurs
	 */
	void unregisterApplication(const rina::ApplicationProcessNamingInformation& app_name,
				   unsigned int opaque);

	/**
	 * Invoked by the IPC Manager to inform about the result of an unregistration
	 * operation and update the internal data structures
	 * This method is NOT thread safe and must be called with the writelock
	 * acquired
	 *
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
	 * This method is NOT thread safe and must be called with the writelock
	 * acquired
	 *
	 * @param flowRequest contains the names of source and destination
	 * applications, the portId as well as the characteristics required for the
	 * flow
	 * @param applicationPortId the port where the application that requested the
	 * flow can be contacted
	 * @param opaque an opaque identifier to correlate requests and responses
	 * @throws AllocateFlowException if an error occurs
	 */
	void allocateFlow(const rina::FlowRequestEvent& flowRequest, unsigned int opaque);

	/**
	 * Invoked by the IPC Manager to inform about the result of an allocate
	 * flow operation and update the internal data structures
	 * This method is NOT thread safe and must be called with the writelock
	 * acquired
	 *
	 * @param sequenceNumber the handle associated to the pending allocation
	 * @param success true if success, false otherwise
	 * @param portId the portId assigned to the flow
	 * @throws AllocateFlowException if the pending allocation
	 * is not found
	 */
	void allocateFlowResult(unsigned int sequenceNumber,
				bool success, int portId);

	/**
	 * Get the information of the flow identified by portId
	 * This method is NOT thread safe and must be called with the readlock
	 * acquired
	 *
         * @result will contain the flow identified by portId, if any
	 * @return true if the flow is found, false otherwise
	 */
	bool getFlowInformation(int portId, rina::FlowInformation& result);

	/**
	 * Reply an IPC Process about the fate of a flow allocation request (wether
	 * it has been accepted or denied by the application). If it has been
	 * accepted, communicate the portId to the IPC Process
	 * This method is NOT thread safe and must be called with the writelock
	 * acquired
	 *
	 * @param flowRequest
	 * @param result 0 if the request is accepted, negative number indicating error
	 * otherwise
	 * @param notifySource true if the IPC Process has to reply to the source, false
	 * otherwise
	 * @param flowAcceptorIpcProcessId the IPC Process id of the Process that accepted
	 * or rejected the flow (0 if it is an application)
	 * @throws AllocateFlowException if something goes wrong
	 */
	void allocateFlowResponse(const rina::FlowRequestEvent& flowRequest,
				  int result, pid_t pid,
				  bool notifySource,
				  int flowAcceptorIpcProcessId);

	/**
	 * Tell the IPC Process to deallocate a flow
	 * This method is NOT thread safe and must be called with the writelock
	 * acquired
	 *
	 * @param portId
	 * @param opaque an opaque identifier to correlate requests and responses
	 * @throws IpcmDeallocateFlowException if there is an error during
	 * the flow deallocation procedure
	 */
	void deallocateFlow(int portId, unsigned int opaque);

	/**
	 * Invoked by the IPC Manager to notify that a flow has been remotely
	 * deallocated, so that librina updates the internal data structures
	 * This method is NOT thread safe and must be called with the writelock
	 * acquired
	 *
	 * @returns the information of the flow deallocated
	 * @throws IpcmDeallocateFlowException if now flow with the given
	 * portId is found
	 */
	rina::FlowInformation flowDeallocated(int portId);

	/**
	 * Invoked by the IPC Manager to query a subset of the RIB of the IPC
	 * Process.
	 *
	 * This method is NOT thread safe and must be called with the readlock
	 * acquired
	 *
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
	 * This method is NOT thread safe and must be called with the writelock
	 * acquired
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
	 * This method is NOT thread safe and must be called with the writelock
	 * acquired
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
	 * This method is NOT thread safe and must be called with the writelock
	 * acquired
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
	 * Invoked by the IPC Manager to forward a CDAP message to
         * IPC process, so that the latter can process the message
	 * through its RIB
	 *
	 * @param msg The CDAP message to send
         * @param opaque an opaque identifier to correlate requests and responses
	 * @throws ForwardCDAPException if an error happens during
         *         the process
	 */
	void forwardCDAPMessage(const rina::cdap::CDAPMessage& msg,
				unsigned int opaque);

private:

	//Friendship relation to be able to destroy proxies from
	//the IPCMIPCProcessFactory
	friend class IPCMIPCProcessFactory;

	/** State of the IPC Process */
	State state_;

	/** The map of pending registrations */
	std::map<unsigned int, rina::ApplicationRegistrationInformation> pendingRegistrations;

	/** The map of pending disconnections */
	std::map<unsigned int, rina::ApplicationProcessNamingInformation> pendingDisconnections;

	/** The map of pending flow operations */
	std::map<unsigned int, rina::FlowInformation> pendingFlowOperations;

	rina::ApplicationRegistrationInformation
		getPendingRegistration(unsigned int seqNumber);

	rina::FlowInformation
		getPendingFlowOperation(unsigned int seqNumber);

	rina::ApplicationProcessNamingInformation
		getPendingDisconnection(unsigned int seqNumber);
};

/**
 * Factory for IPCMIPCProcess
 */
class IPCMIPCProcessFactory{

public:
	~IPCMIPCProcessFactory() throw(){};

	/** Rwlock */
	rina::ReadWriteLockable rwlock;


    /**
     * Read the sysfs folder and get the list of IPC Process types supported
     * by the kernel
     * @return the list of supported IPC Process types
     */
    std::list<std::string> getSupportedIPCProcessTypes();

    /**
     * Invoked by the IPC Manager to instantiate a new IPC Process in the
     * system. The operation will block until the IPC Process is created or an
     * error is returned. The IPCMIPCProcess reference is always returned with
     * the write lock acquired
     *
     * @param ipcProcessId An identifier that uniquely identifies an IPC Process
     * within a aystem.
     * @param ipcProcessName The naming information of the IPC Process
     * @param difType The type of IPC Process (Normal or one of the shims)
     * @return a pointer to a data structure holding the IPC Process state
     * @throws CreateIPCProcessException if an error happens during the creation
     */
    IPCMIPCProcess * create(
                    const rina::ApplicationProcessNamingInformation& ipcProcessName,
                    const std::string& difType);

    /**
     * Invoked by the IPC Manager to delete an IPC Process from the system. The
     * operation will block until the IPC Process is destroyed or an error is
     * returned.
     *
     * @param ipcProcessId The identifier of the IPC Process to be destroyed
     * @throws DestroyIPCProcessException if an error happens during the operation execution
     */
    unsigned int destroy(unsigned short ipcProcessId);

    /**
     * Returns a list to all the IPC Processes that are currently running in
     * the system.
     *
     * @param list<IPCProcess *> A reference to a list of the IPC Processes in the system
     */
    void listIPCProcesses(std::vector<IPCMIPCProcess *>& result);

    /**
     * Check if the ipcp exists
     */
    bool exists(const unsigned short id);

    /**
     * Check if the ipcp exists by PID. If so, return IPCP ID,
     * otherwise return 0
     */
    unsigned short exists_by_pid(pid_t pid);

    /**
     * Returns a pointer to the IPCProcess identified by ipcProcessId
     * @param ipcProcessId
     * @return a pointer to an IPC Process or NULL if no IPC Process
     * with the specified id is found
     */
    IPCMIPCProcess * getIPCProcess(unsigned short ipcProcessId);

    IPCMIPCProcess * getIPCProcessByName(const rina::ApplicationProcessNamingInformation& ipcp_name);

    /// Returns the names of the DIFs local IPCPs are assigned to
    void get_local_dif_names(std::list<std::string>& result);

private:
	//The underlying IPC Process Factory
	rina::IPCProcessFactory proxy_factory_;

	/** The current IPC Processes in the system*/
	std::map<unsigned short, IPCMIPCProcess*> ipcProcesses;
};

}//rinad namespace

#endif  /* __IPCP_H__ */
