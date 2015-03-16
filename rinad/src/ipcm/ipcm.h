/*
 * IPC Manager
 *
 *    Vincenzo Maffione <v.maffione@nextworks.it>
 *
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

#ifndef __IPCM_H__
#define __IPCM_H__

#include <assert.h>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
#include <utility>

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/patterns.h>

#include "rina-configuration.h"

//Addons
#include "addon.h"
#include "addons/console.h"
#include "addons/scripting.h"
//[+] Add more here...



#ifndef DOWNCAST_DECL
	// Useful MACRO to perform downcasts in declarations.
	#define DOWNCAST_DECL(_var,_class,_name)\
		_class *_name = dynamic_cast<_class*>(_var);
#endif //DOWNCAST_DECL

#ifndef FLUSH_LOG
	//Force log flushing
	#define FLUSH_LOG(_lev_, _ss_)\
			do{\
				LOGF_##_lev_ ("%s", (_ss_).str().c_str());\
				ss.str(string());\
			}while (0)
#endif //FLUSH_LOG

namespace rinad {


//
// This base class encapsulates the generics of any two-step, so requiring
// interaction with the kernel, API call.
//
class TransactionState {

public:
	TransactionState(const Addon* _callee, const int _tid):
						callee(_callee), tid(_tid){};
	virtual ~TransactionState(){};

	//Callee
	const Addon* callee;

	//Transaction id
	const int tid;

	//Return value
	int ret;

	//Condition variable
	rina::ConditionVariable wait_cond;
};

#if 0
//
//
// TODO: revise
//
class IPCMConcurrency : public rina::ConditionVariable {
 public:
	bool wait_for_event(rina::IPCEventType ty, unsigned int seqnum,
			    int &result);
	void notify_event(rina::IPCEvent *event);
	void set_event_result(int result) { event_result = result; }

	IPCMConcurrency(unsigned int wt) :
				wait_time(wt), event_waiting(false) { }

	void setWaitTime(unsigned int wt){wait_time = wt;}
 private:
	unsigned int wait_time;
	bool event_waiting;
	rina::IPCEventType event_ty;
	unsigned int event_sn;
	int event_result;
};
#endif

/**
 * Encapsulates the state and operations that can be performed over
 * a single IPC Process (besides creation/destruction)
 */
class IPCMIPCProcess: public rina::Lockable {
public:
	enum State{IPCM_IPCP_CREATED,
			   IPCM_IPCP_INITIALIZED,
			   IPCM_IPCP_ASSIGN_TO_DIF_IN_PROGRESS,
			   IPCM_IPCP_ASSIGNED_TO_DIF};

	/** The IPC Process proxy class */
	rina::IPCProcessProxy* ipcp_proxy_;

	/** The current information of the DIF where the IPC Process is assigned*/
	rina::ApplicationProcessNamingInformation dif_name_;

	/** The list of flows currently allocated in this IPC Process */
	std::list<rina::FlowInformation> allocatedFlows;

	/** The list of applications registered in this IPC Process */
	std::list<rina::ApplicationProcessNamingInformation> registeredApplications;

	IPCMIPCProcess();
	IPCMIPCProcess(rina::IPCProcessProxy* ipcp_proxy);
	~IPCMIPCProcess() throw();

	const rina::ApplicationProcessNamingInformation& getDIFName() const;

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
	 * @param opaque an opaque identifier to correlate requests and responses
	 * @throws AssignToDIFException if an error happens during the process
	 */
	void assignToDIF(
			const rina::DIFInformation& difInformation, unsigned int opaque);

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
			const rina::ApplicationProcessNamingInformation& neighbor);

	/**
	 * Invoked by the IPC Manager to register an application in a DIF through
	 * an IPC Process.
	 *
	 * @param applicationName The name of the application to be registered
	 * @param regIpcProcessId The id of the registered IPC process (0 if it
	 * is an application)
	 * @param opaque an opaque identifier to correlate requests and responses
	 * @throws IpcmRegisterApplicationException if an error occurs
	 */
	void registerApplication(
			const rina::ApplicationProcessNamingInformation& applicationName,
			unsigned short regIpcProcessId,
			unsigned int opaque);

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
	 * Invoked by the IPC Manager to unregister an application in a DIF through
	 * an IPC Process.
	 *
	 * @param applicationName The name of the application to be unregistered
	 * @param opaque an opaque identifier to correlate requests and responses
	 * @throws IpcmUnregisterApplicationException if an error occurs
	 */
	void unregisterApplication(
			const rina::ApplicationProcessNamingInformation& applicationName,
			unsigned int opaque);

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
	 * @param opaque an opaque identifier to correlate requests and responses
	 * @throws AllocateFlowException if an error occurs
	 */
	void allocateFlow(const rina::FlowRequestEvent& flowRequest, unsigned int opaque);

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
	 * Get the information of the flow identified by portId
         * @result will contain the flow identified by portId, if any
	 * @return true if the flow is found, false otherwise
	 */
	bool getFlowInformation(int portId, rina::FlowInformation& result);

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
	void allocateFlowResponse(const rina::FlowRequestEvent& flowRequest,
			int result, bool notifySource,
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
	rina::FlowInformation flowDeallocated(int portId);

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

private:
	/** State of the IPC Process */
	State state_;

	/** The map of pending registrations */
	std::map<unsigned int, rina::ApplicationProcessNamingInformation> pendingRegistrations;

	/** The map of pending flow operations */
	std::map<unsigned int, rina::FlowInformation> pendingFlowOperations;

	rina::ApplicationProcessNamingInformation
		getPendingRegistration(unsigned int seqNumber);

	rina::FlowInformation
		getPendingFlowOperation(unsigned int seqNumber);
};

class IPCMIPCProcessFactory: public rina::Lockable {
public:
	IPCMIPCProcessFactory();
	~IPCMIPCProcessFactory() throw();

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
    void destroy(unsigned short ipcProcessId);

    /**
     * Returns a list to all the IPC Processes that are currently running in
     * the system.
     *
     * @return list<IPCProcess *> A list of the IPC Processes in the system
     */
    std::vector<IPCMIPCProcess *> listIPCProcesses();

    /**
     * Returns a pointer to the IPCProcess identified by ipcProcessId
     * @param ipcProcessId
     * @return a pointer to an IPC Process or NULL if no IPC Process
     * with the specified id is found
     */
    IPCMIPCProcess * getIPCProcess(unsigned short ipcProcessId);

private:
	//The underlying IPC Process Factory
	rina::IPCProcessFactory ipcp_factory_;

    /** The current IPC Processes in the system*/
    std::map<unsigned short, IPCMIPCProcess*> ipcProcesses;
};

//
// @brief Pending Flow allocation object
//
// TODO: revise
//
struct PendingFlowAllocation {
	IPCMIPCProcess *slave_ipcp;
	rina::FlowRequestEvent req_event;
	bool try_only_a_dif;

	PendingFlowAllocation(): slave_ipcp(NULL),
				 try_only_a_dif(true) { }
	PendingFlowAllocation(IPCMIPCProcess *p,
			const rina::FlowRequestEvent& e, bool once)
				: slave_ipcp(p), req_event(e),
					try_only_a_dif(once) { }
};

//
// @brief The IPCManager class is in charge of managing the IPC processes
// life-cycle.
//
class IPCManager_ {

public:
	//
	// Initialize the IPCManager
	//
	void init(unsigned int wait_time, const std::string& loglevel);

	//
	// Start the script worker thread
	//
	int start_script_worker();

	//
	// Start the console worker thread
	//
	int start_console_worker();

	//
	// TODO: XXX?????
	//
	int apply_configuration();


	//
	// List the existing IPCPs in the system
	//
	int list_ipcps(std::ostream& os);


	//
	// Get the IPCP ID given a difName
	// TODO: multiple IPCPs in the same DIF?
	//
	int get_ipcp_by_dif_name(std::string& difName);

	//
	// Checks if an IPCP exists by its ID
	//
	bool ipcp_exists(const int ipcp_id);

	//
	// List the available IPCP types
	//
	int list_ipcp_types(std::list<std::string>& types);


	//
	// Creates an IPCP process
	//
	// @ret -1 on failure, otherwise the IPCP id
	//
	int create_ipcp(const Addon* callee,
			const rina::ApplicationProcessNamingInformation& name,
			const std::string& type);

	//
	// Destroys an IPCP process
	//
	int destroy_ipcp(const Addon* callee, const unsigned int ipcp_id);

	//
	// Assing an ipcp to a DIF
	//
	int assign_to_dif(const int ipcp_id,
			  const rina::ApplicationProcessNamingInformation&
			  difName);

	//
	// Register an IPCP to a single DIF
	//
	int register_at_dif(const int ipcp_id,
			    const rina::ApplicationProcessNamingInformation&
			    difName);

	//
	// Register an existing IPCP to multiple DIFs
	//
	int register_at_difs(const int ipcp_id, const
			std::list<rina::ApplicationProcessNamingInformation>&
			     difs);

	//
	// Enroll IPCP to a single DIF
	//
	int enroll_to_dif(const int ipcp_id,
			  const rinad::NeighborData& neighbor, bool sync);
	//
	// Enroll IPCP to multiple DIFs
	//
	int enroll_to_difs(const int ipcp_id,
			   const std::list<rinad::NeighborData>& neighbors);

	//TODO
	int unregister_app_from_ipcp(
		const rina::ApplicationUnregistrationRequestEvent& req_event,
		int slave_ipcp_id);

	//TODO
	int unregister_ipcp_from_ipcp(const int ipcp_id,
						const int slave_ipcp_id);

	//
	// Get the IPCP identifier where the application is registered
	// FIXME: return id instead?
	//
	bool lookup_dif_by_application(
		const rina::ApplicationProcessNamingInformation& apName,
		rina::ApplicationProcessNamingInformation& difName);

	//
	// Update the DIF configuration
	//TODO: What is really this for?
	//
	int update_dif_configuration(
		const int ipcp_id,
		const rina::DIFConfiguration& dif_config);

	int deallocate_flow(const int ipcp_id,
			    const rina::FlowDeallocateRequestEvent& event);

	//
	// Retrieve the IPCP RIB in the form of a string
	//
	std::string query_rib(const int ipcp_id);

	int select_policy_set(const int ipcp_id,
			      const std::string& component_path,
			      const std::string& policy_set);

	int set_policy_set_param(const int ipcp_id,
				 const std::string& path,
				 const std::string& name,
				 const std::string& value);

	int plugin_load(const int ipcp_id,
			const std::string& plugin_name, bool load);

	//
	// Get the current logging debug level
	//
	std::string get_log_level() const;

	//
	// Set the config
	//
	void loadConfig(rinad::RINAConfiguration& newConf){
		config = newConf;
	}

	//
	// Dump the current configuration
	// TODO return ostream or overload << operator instead
	//
	void dumpConfig(void){
		std::cout << config.toString() << std::endl;
	}

	//
	// Run the main I/O loop
	//
	void run(void);

	//
	// Stop I/O loop
	//
	inline void stop(void){
		keep_running = false;
	}

protected:

	//
	// Internal APIs
	//

	/**
	* Get the IPCP by DIF name
	*
 	* Recovers the IPCProcess pointer with the rwlock acquired to either
	* read lock or write lock
	*
	* @param read_lock When true, the IPCProcess instance is recovered with
	* the read lock acquired, otherwise the write lock is acquired.
	*/
	IPCMIPCProcess* select_ipcp_by_dif(const
			rina::ApplicationProcessNamingInformation& dif_name,
			bool read_lock=true);

	/**
	* TODO????
	*/
	IPCMIPCProcess* select_ipcp(bool read_lock=true);

	/**
	* Check application registration
	*/
	bool application_is_registered_to_ipcp(
			const rina::ApplicationProcessNamingInformation&,
			IPCMIPCProcess *slave_ipcp);
	/**
	* Get the IPCP by port id
	*
 	* Recovers the IPCProcess pointer with the rwlock acquired to either
	* read lock or write lock
	*
	* @param read_lock When true, the IPCProcess instance is recovered with
	* the read lock acquired, otherwise the write lock is acquired.
	*/
	IPCMIPCProcess* lookup_ipcp_by_port(unsigned int port_id,
						bool read_lock=true);

	/**
	* Collect flows for an application name
	*/
	void collect_flows_by_application(
			const rina::ApplicationProcessNamingInformation& app_name,
			std::list<rina::FlowInformation>& result);

	/**
	* Get the IPCP instance pointer
	*
	* Recovers the IPCProcess pointer with the rwlock acquired to either
	* read lock or write lock
	*
	* @param read_lock When true, the IPCProcess instance is recovered with
	* the read lock acquired, otherwise the write lock is acquired.
	*/
	IPCMIPCProcess* lookup_ipcp_by_id(unsigned int id,
							bool read_lock=true);
	//
	// Internal event API
	//

	//Flow mgmt
	void flow_allocation_requested_event_handler(rina::IPCEvent *event);
	void allocate_flow_request_result_event_handler(rina::IPCEvent *event);
	void allocate_flow_response_event_handler(rina::IPCEvent *event);
	void flow_deallocation_requested_event_handler(rina::IPCEvent *event);
	void deallocate_flow_response_event_handler(rina::IPCEvent *event);
	void flow_deallocated_event_handler(rina::IPCEvent *event);
	void ipcm_deallocate_flow_response_event_handler(rina::IPCEvent *event);
	void ipcm_allocate_flow_request_result_handler(rina::IPCEvent *event);
	void application_flow_allocation_failed_notify(
						rina::FlowRequestEvent *event);
	void flow_allocation_requested_local(rina::FlowRequestEvent *event);

	void flow_allocation_requested_remote(rina::FlowRequestEvent *event);


	//Application registration mgmt
	void application_registration_request_event_handler(rina::IPCEvent *e);
	void ipcm_register_app_response_event_handler(rina::IPCEvent *e);
	void application_unregistration_request_event_handler(rina::IPCEvent *e);
	void ipcm_unregister_app_response_event_handler(rina::IPCEvent *e);
	void register_application_response_event_handler(rina::IPCEvent *event);
	void unregister_application_response_event_handler(rina::IPCEvent *event);
	void application_registration_canceled_event_handler(rina::IPCEvent *event);
	void application_unregistered_event_handler(rina::IPCEvent * event);
	void application_registration_notify(
		const rina::ApplicationRegistrationRequestEvent& req_event,
		const rina::ApplicationProcessNamingInformation& app_name,
		const rina::ApplicationProcessNamingInformation& slave_dif_name,
		bool success);
	int ipcm_register_response_app(
		rina::IpcmRegisterApplicationResponseEvent *event,
		std::map<unsigned int,
			std::pair<IPCMIPCProcess *,
				rina::ApplicationRegistrationRequestEvent>
		>::iterator mit);
	void application_manager_app_unregistered(
		rina::ApplicationUnregistrationRequestEvent event,
		int result);
	int ipcm_unregister_response_app(
			rina::IpcmUnregisterApplicationResponseEvent *event,
			std::map<unsigned int,
				std::pair<IPCMIPCProcess *,
				rina::ApplicationUnregistrationRequestEvent
				>
			>::iterator mit);

	//IPCP mgmt
	void ipc_process_create_connection_response_handler(rina::IPCEvent * event);
	void ipc_process_update_connection_response_handler(rina::IPCEvent * event);
	void ipc_process_create_connection_result_handler(rina::IPCEvent * event);
	void ipc_process_destroy_connection_result_handler(rina::IPCEvent * event);
	int ipcm_register_response_ipcp(
		rina::IpcmRegisterApplicationResponseEvent *event,
		std::map<unsigned int,
			std::pair<IPCMIPCProcess *, IPCMIPCProcess *>
		>::iterator mit);
	int ipcm_unregister_response_ipcp(
				rina::IpcmUnregisterApplicationResponseEvent *event,
				std::map<unsigned int,
				    std::pair<IPCMIPCProcess *, IPCMIPCProcess *>
				>::iterator mit);

	//DIF assignment mgmt
	void assign_to_dif_request_event_handler(rina::IPCEvent * event);
	void assign_to_dif_response_event_handler(rina::IPCEvent * e);

	//DIF config mgmt
	void update_dif_config_request_event_handler(rina::IPCEvent *event);
	void update_dif_config_response_event_handler(rina::IPCEvent *e);

	//Enrollment mgmt
	void enroll_to_dif_request_event_handler(rina::IPCEvent *event);
	void enroll_to_dif_response_event_handler(rina::IPCEvent *e);
	void neighbors_modified_notification_event_handler(rina::IPCEvent * e);

	//DIF misc TODO: revise
	void ipc_process_dif_registration_notification_handler(rina::IPCEvent *event);
	void ipc_process_query_rib_handler(rina::IPCEvent *event);
	void get_dif_properties_handler(rina::IPCEvent *event);
	void get_dif_properties_response_event_handler(rina::IPCEvent *event);

	//RIB queries
	void query_rib_response_event_handler(rina::IPCEvent *e);
	void ipc_process_dump_ft_response_handler(rina::IPCEvent * event);

	//Misc
	void os_process_finalized_handler(rina::IPCEvent *e);
	void ipc_process_daemon_initialized_event_handler(
				rina::IPCProcessDaemonInitializedEvent *e);
	void timer_expired_event_handler(rina::IPCEvent *event);
	bool ipcm_register_response_common(
		rina::IpcmRegisterApplicationResponseEvent *event,
		const rina::ApplicationProcessNamingInformation& app_name,
		IPCMIPCProcess *slave_ipcp,
		const rina::ApplicationProcessNamingInformation& slave_dif_name);

	bool ipcm_register_response_common(
		rina::IpcmRegisterApplicationResponseEvent *event,
		const rina::ApplicationProcessNamingInformation& app_name,
		IPCMIPCProcess *slave_ipcp);


	bool ipcm_unregister_response_common(
				 rina::IpcmUnregisterApplicationResponseEvent *event,
				 IPCMIPCProcess *slave_ipcp,
				 const rina::ApplicationProcessNamingInformation&);

	//Plugins
	void ipc_process_set_policy_set_param_response_handler(
							rina::IPCEvent *e);
	void ipc_process_plugin_load_response_handler(rina::IPCEvent *e);
	void ipc_process_select_policy_set_response_handler(
							rina::IPCEvent *e);

/**********************************************************/
	//
	// TODO: revise
	//
	//IPCMConcurrency concurrency;

	//Pending IPCP DIF assignments
	std::map<unsigned int, IPCMIPCProcess*> pending_ipcp_dif_assignments;

	//Pending query RIB
	std::map<unsigned int, IPCMIPCProcess*> pending_ipcp_query_rib_responses;

	//Query RIB responses
	std::map<unsigned int, std::string > query_rib_responses;

	//Pending IPCP registrations
	std::map<unsigned int,
		 std::pair<IPCMIPCProcess*, IPCMIPCProcess*>
		> pending_ipcp_registrations;

	//Pending IPCP unregistration
	std::map<unsigned int,
		 std::pair<IPCMIPCProcess*, IPCMIPCProcess*>
		> pending_ipcp_unregistrations;

	//Pending IPCP enrollments
	std::map<unsigned int, IPCMIPCProcess*> pending_ipcp_enrollments;

	//Pending AP registrations
	std::map<unsigned int,
		 std::pair<IPCMIPCProcess*,
			   rina::ApplicationRegistrationRequestEvent
			  >
		> pending_app_registrations;

	//Pending AP unregistrations
	std::map<unsigned int,
		 std::pair<IPCMIPCProcess*,
			   rina::ApplicationUnregistrationRequestEvent
			  >
		> pending_app_unregistrations;

	//Pending DIF config updates
	std::map<unsigned int, IPCMIPCProcess*> pending_dif_config_updates;

	//Pending flow allocations
	std::map<unsigned int, PendingFlowAllocation> pending_flow_allocations;

	//Pending flow deallocations
	std::map<unsigned int,
		 std::pair<IPCMIPCProcess *,
			   rina::FlowDeallocateRequestEvent
			  >
		> pending_flow_deallocations;

	//
	// RINA configuration internal state
	//
	rinad::RINAConfiguration config;

	/* FIXME REMOVE THIS*/
	std::map<unsigned int,
		IPCMIPCProcess *> pending_set_policy_set_param_ops;

	std::map<unsigned int,
		IPCMIPCProcess *> pending_select_policy_set_ops;

	std::map<unsigned int,
		IPCMIPCProcess *> pending_plugin_load_ops;
	/* FIXME REMOVE THIS*/

	//Script thread
	rina::Thread *script;

	//IPCM Console instance
	IPCMConsole *console;

/**********************************************************/

	/*
	* Get the transaction state. Template parameter is the type of the
	* specific state required for the type of transaction
	*
	* @ret A pointer to the state or NULL
	* @warning This method does NOT throw exceptions.
	*/
	template<typename T>
	T* get_transaction_state(int tid){
		T* t;
		TransactionState* state;

		//Rwlock: read
		if ( pend_transactions.find(tid) == pend_transactions.end() ) {
			state = pend_transactions[tid];
			assert(state->tid == tid);
			try{
				t = dynamic_cast<T*>(state);
				return t;
			}catch(...){
				assert(0);
				return NULL;
			}
		}
		return NULL;
	}

	/*
	* Add the transaction state. Template parameter is the type of the
	* specific state required for the type of transaction
	*
	* @ret 0 if success -1 otherwise.
	*/
	int add_transaction_state(int tid, TransactionState* t);

	/*
	* @Remove transaction state.
	*
	* Remove the transaction state from the pending transactions db. The
	* method does NOT free (delete) the transaction pointer.
	*
	* @ret 0 if success -1 otherwise.
	*/
	int remove_transaction_state(int tid);

	/*
	* This map encapsulates the existing transactions and their state
	*
	* The state of the transaction includes the input *and* the output
	* parameters for the specific operation that was started.
	*
	* key: transaction_id value: transaction state
	*/
	std::map<int, TransactionState*> pend_transactions;

	//TODO unify syscalls and non-syscall state

	/**
	* Get syscall transaction state (waiting for a notification)
	*/
	TransactionState* get_syscall_transaction_state(int tid){
		//Rwlock: read
		if ( pend_sys_trans.find(tid) !=  pend_sys_trans.end() )
			return pend_transactions[tid];

		return NULL;
	};

	/**
	* Add syscall transaction state (waiting for a notification)
	*
	* @ret 0 if success -1 otherwise.
	*/
	int add_syscall_transaction_state(int tid, TransactionState* t);

	/*
	* Remove syscall transaction state
	*
	* @ret 0 if success -1 otherwise.
	*/
	int remove_syscall_transaction_state(int tid);

	/**
	* Pending syscall states
	*
	* key: ipcp ID value: transaction state
	*/
	std::map<int, TransactionState*> pend_sys_trans;

	//Rwlock for transactions
	rina::ReadWriteLockable trans_rwlock;

	//Rwlock for IPCP array reading
	rina::ReadWriteLockable ipcps_rwlock;

	//TODO: map of addons

	//Current logging level
	std::string log_level_;

	//Keep running flag
	volatile bool keep_running;

	//Generator of opaque identifiers
	rina::ConsecutiveUnsignedIntegerGenerator opaque_generator_;

	IPCMIPCProcessFactory ipcp_factory_;

private:
	//Singleton
	IPCManager_();
	virtual ~IPCManager_();
	friend class Singleton<rinad::IPCManager_>;
};


//Singleton instance
extern Singleton<rinad::IPCManager_> IPCManager;

}//rinad namespace

#endif  /* __IPCM_H__ */
