/*
 * IPC Manager
 *
 *    Vincenzo Maffione <v.maffione@nextworks.it>
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
 *    Marc Sune         <marc.sune (at) bisdn.de>
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

#ifndef __IPCM_H__
#define __IPCM_H__

#include <assert.h>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
#include <utility>
#include <string>

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/patterns.h>

#include "dif-template-manager.h"
#include "dif-allocator.h"
#include "catalog.h"

//Addons
#include "addon.h"
#include "addons/console.h"
#include "addons/scripting.h"
//[+] Add more here...


//Constants
#define PROMISE_TIMEOUT_S 5
#define PROMISE_RETRY_NSEC 10000000 //10ms
#define _PROMISE_1_SEC_NSEC 1000000000

namespace rinad {

//
// Return codes
//
typedef enum ipcm_res{
	//Success
	IPCM_SUCCESS = 0,

	//Return value will be deferred
	IPCM_PENDING = 1,

	//Generic failure
	IPCM_FAILURE = -1,

	//TODO: add more codes here...
}ipcm_res_t;


//fwd decl
class TransactionState;

//
// Promise base class
//
class Promise {

public:

	virtual ~Promise(){};

	//
	// Wait (blocking)
	//
	ipcm_res_t wait(void);

	//
	// Signal the condition
	//
	inline void signal(void){
		wait_cond.signal();
	}

	//
	// Timed wait (blocking)
	//
	// Return SUCCESS or IPCM_PENDING if the operation did not finally
	// succeed
	//
	ipcm_res_t timed_wait(const unsigned int seconds);

	//
	// Return code
	//
	ipcm_res_t ret;

protected:
	//Protect setting of trans
	friend class TransactionState;

	//Transaction back reference
	TransactionState* trans;

	//Condition variable
	rina::ConditionVariable wait_cond;
};

//
// Create IPCP promise
//
class CreateIPCPPromise : public Promise {


public:
	int ipcp_id;
};

//
// Query RIB promise
//
class QueryRIBPromise : public Promise {

public:
	std::string serialized_rib;
};

//
// This base class encapsulates the generics of any two-step, so requiring
// interaction with the kernel, API call.
//
class TransactionState {

public:
	TransactionState(Addon* addon, Promise* promise);
	virtual ~TransactionState(){};

	//
	// Used to recover the promise from the transaction
	//
	// @arg Template T: type of the promise
	//
	template<typename T>
	T* get_promise(){
		try{
			T* t = dynamic_cast<T*>(promise);
			return t;
		}catch(...){
			assert(0);
			return NULL;
		}
	}

	//
	// This method and signals any existing set complete flag
	//
	void completed(ipcm_res_t _ret){
		rina::ScopedLock slock(mutex);

		if(finalised)
			return;

		if(!promise)
			return;

		promise->ret = _ret;
		promise->signal();
	}

	//Promise
	Promise* promise;

	//Transaction id
	const int tid;

	//Callee that generated the transaction
	Addon* callee;

protected:
	//Protect abort call
	friend class Promise;

	//
	// @brief Abort the transaction (hard timeout)
	//
	// This particular method aborts the transaction due to a wait in the
	// promise that has timedout
	//
	// @ret On success, the transaction has been aborted. On failure, a
	// completed() call has successfully finished the transaction
	//
	bool abort(void){
		rina::ScopedLock slock(mutex);

		if(finalised)
			return false;

		return finalised = true;
	}


	//
	// Constructor only used
	//
	TransactionState(Addon* callee_, Promise* promise_, const int tid_):
							promise(promise_),
							tid(tid_),
							callee(callee_),
							finalised(false){
		if(promise){
			promise->ret = IPCM_PENDING;
			promise->trans = this;
		}
	};

	//Completed flag
	bool finalised;

	// Mutex
	rina::Lockable mutex;
};

//
// Syscall specific transaction state
//
class SyscallTransState : public TransactionState {
public:
	SyscallTransState(Addon* callee, Promise* promise_, const int tid_) :
				TransactionState(callee, promise_, tid_){};
	virtual ~SyscallTransState(){};
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
	void init(const std::string& loglevel, std::string& config_file);

	//
	// Load the specified addons
	//
	// @param addons Comma separated list of addons
	void load_addons(const std::string& addon_list);

	//
	// TODO: XXX?????
	//
	ipcm_res_t apply_configuration();


	//
	// List the existing IPCPs in the system
	// TODO: deprecate this console only stuff
	//
	void list_ipcps(std::ostream& os);

	//
	// Get the list of IPCPs in the system (IPCP id)
	//
	// @param list The list will be filled here
	//
	void list_ipcps(std::list<int>& list);

	//
	// Get the IPCP name
	//
	std::string get_ipcp_name(int ipcp_id);

	//
	// Get the IPCP ID given a difName
	// TODO: multiple IPCPs in the same DIF?
	//
	int get_ipcp_by_dif_name(std::string& difName);

	//
	// Checks if an IPCP exists by its ID
	//
	bool ipcp_exists(const unsigned short ipcp_id);

	//
	// List the available IPCP types
	//
	void list_ipcp_types(std::list<std::string>& types);


	//
	// Get the IPCP identifier where the application is registered
	// FIXME: return id instead?
	//
	bool lookup_dif_by_application(
		const rina::ApplicationProcessNamingInformation& apName,
		rina::ApplicationProcessNamingInformation& difName);


	//
	// Creates an IPCP process
	//
	// @param promise Promise object containing the future result of the
	// operation. The promise shall always be accessible until the
	// operation has been finished, so promise->ret value is different than
	// IPCM_PENDING.
	//
	// @ret IPCM_FAILURE on failure, otherwise the IPCM_PENDING
	//
	ipcm_res_t create_ipcp(Addon* callee, CreateIPCPPromise* promise,
			const rina::ApplicationProcessNamingInformation& name,
			const std::string& type);

	//
	// Destroys an IPCP process
	//
	// This method is blocking
	//
	// @ret IPCM_SUCCESS on success IPCM_FAILURE
	//
	ipcm_res_t destroy_ipcp(Addon* callee, const unsigned short ipcp_id);

	//
	// Assing an ipcp to a DIF
	//
	// @param promise Promise object containing the future result of the
	// operation. The promise shall always be accessible until the
	// operation has been finished, so promise->ret value is different than
	// IPCM_PENDING.
	//
	// @ret IPCM_FAILURE on failure, otherwise the IPCM_PENDING
	//
	ipcm_res_t assign_to_dif(Addon* callee, Promise* promise,
			const unsigned short ipcp_id,
			rinad::DIFTemplate * dif_template,
			const rina::ApplicationProcessNamingInformation&
				difName);
	ipcm_res_t assign_to_dif(Addon* callee, Promise* promise,
		const unsigned short ipcp_id, rina::DIFInformation &dif_info,
		const rina::ApplicationProcessNamingInformation &dif_name);
	//
	// Register an IPCP to a single DIF
	//
	// @param promise Promise object containing the future result of the
	// operation. The promise shall always be accessible until the
	// operation has been finished, so promise->ret value is different than
	// IPCM_PENDING.
	//
	// @ret IPCM_FAILURE on failure, otherwise the IPCM_PENDING
	ipcm_res_t register_at_dif(Addon* callee, Promise* promise,
			const unsigned short ipcp_id,
			const rina::ApplicationProcessNamingInformation& difName);

	//
	// Enroll IPCP to a single DIF
	//
	// @param promise Promise object containing the future result of the
	// operation. The promise shall always be accessible until the
	// operation has been finished, so promise->ret value is different than
	// IPCM_PENDING.
	//
	// @ret IPCM_FAILURE on failure, otherwise the IPCM_PENDING
	ipcm_res_t enroll_to_dif(Addon* callee, Promise* promise,
			const unsigned short ipcp_id,
			const rinad::NeighborData& neighbor);

	//
	// Unregister app from an ipcp
	//
	// @param promise Promise object containing the future result of the
	// operation. The promise shall always be accessible until the
	// operation has been finished, so promise->ret value is different than
	// IPCM_PENDING.
	//
	// @ret IPCM_FAILURE on failure, otherwise the IPCM_PENDING
	ipcm_res_t unregister_app_from_ipcp(Addon* callee,
		Promise* promise,
		const rina::ApplicationUnregistrationRequestEvent& req_event,
		const unsigned short slave_ipcp_id);

	//
	// Unregister an ipcp from another one
	//
	// @param promise Promise object containing the future result of the
	// operation. The promise shall always be accessible until the
	// operation has been finished, so promise->ret value is different than
	// IPCM_PENDING.
	//
	// @ret IPCM_FAILURE on failure, otherwise the IPCM_PENDING
	ipcm_res_t unregister_ipcp_from_ipcp(Addon* callee, Promise* promise,
						const unsigned short ipcp_id,
						const rina::ApplicationProcessNamingInformation& dif_name);
	//
	// Update the DIF configuration
	//TODO: What is really this for?
	//
	// @param promise Promise object containing the future result of the
	// operation. The promise shall always be accessible until the
	// operation has been finished, so promise->ret value is different than
	// IPCM_PENDING.
	//
	// @ret IPCM_FAILURE on failure, otherwise the IPCM_PENDING
	ipcm_res_t update_dif_configuration(Addon* callee, Promise* promise,
				const unsigned short ipcp_id,
				const rina::DIFConfiguration& dif_config);

	//
	// Retrieve the IPCP RIB in the form of a string
	//
	// @param promise Promise object containing the future result of the
	// operation. The promise shall always be accessible until the
	// operation has been finished, so promise->ret value is different than
	// IPCM_PENDING.
	//
	// @ret IPCM_FAILURE on failure, otherwise the IPCM_PENDING
	ipcm_res_t query_rib(Addon* callee, QueryRIBPromise* promise,
						const unsigned short ipcp_id);

	//
	// Select a policy set
	//
	// @param promise Promise object containing the future result of the
	// operation. The promise shall always be accessible until the
	// operation has been finished, so promise->ret value is different than
	// IPCM_PENDING.
	//
	// @ret IPCM_FAILURE on failure, otherwise the IPCM_PENDING
	ipcm_res_t select_policy_set(Addon* callee, Promise* promise,
					const unsigned short ipcp_id,
					const std::string& component_path,
					const std::string& policy_set);
	//
	// Set a policy "set-param"
	//
	// @param promise Promise object containing the future result of the
	// operation. The promise shall always be accessible until the
	// operation has been finished, so promise->ret value is different than
	// IPCM_PENDING.
	//
	// @ret IPCM_FAILURE on failure, otherwise the IPCM_PENDING
	ipcm_res_t set_policy_set_param(Addon* callee, Promise* promise,
						const unsigned short ipcp_id,
						const std::string& path,
						const std::string& name,
						const std::string& value);
	//
	// Load policy plugin
	//
	// @param promise Promise object containing the future result of the
	// operation. The promise shall always be accessible until the
	// operation has been finished, so promise->ret value is different than
	// IPCM_PENDING.
	//
	// @ret IPCM_FAILURE on failure, otherwise the IPCM_PENDING
	ipcm_res_t plugin_load(Addon* callee, Promise* promise,
			       const unsigned short ipcp_id,
			       const std::string& plugin_name,
			       bool load);

	//
	// Get information about plugin
	//
	// @param plugin_name The name of the plugin to get info from
	// @param result A list that is filled in with information about
	//               the policy sets supported by the plugin
	//
	// @ret IPCM_FAILURE on failure, otherwise the IPCM_SUCCESS
        ipcm_res_t plugin_get_info(const std::string& plugin_name,
				   std::list<rina::PsInfo>& result);

        //
        // Just used for testing support for IPCP RIB delegation
        //
	// @param promise Promise object containing the future result of the
	// operation. The promise shall always be accessible until the
	// operation has been finished, so promise->ret value is different than
	// IPCM_PENDING.
        // @param ipcp_id the IPCP id
        // @param object_class the class of the object to be read
        // @param object_name the name of the object to be read
        //
        // @ret IPCM_PENDING if the NL message could be sent to the IPCP,
        // IPCM_FAILURE otherwise
	ipcm_res_t read_ipcp_ribobj(Addon* callee,
				    Promise* promise,
			      	    const unsigned short ipcp_id,
			      	    const std::string& object_class,
			      	    const std::string& object_name,
			      	    int scope);

	//
	// Update policy-set catalog, with the plugins stored in
	// the pluginsPaths configuration variable
	//
	// @ret IPCM_FAILURE on failure, otherwise the IPCM_SUCCESS
	ipcm_res_t update_catalog(Addon* callee);

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

	rinad::RINAConfiguration& getConfig() {
		return config;
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
		req_to_stop = true;
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
			bool write_lock=false);

	/**
	* Select a suitable IPCP
	* @param read_lock When true, the IPCProcess instance is recovered with
	* the read lock acquired, otherwise the write lock is acquired.
	*/
	IPCMIPCProcess* select_ipcp(bool write_lock=false);

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
						bool write_lock=false);

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
	IPCMIPCProcess* lookup_ipcp_by_id(const unsigned short id,
							bool write_lock=false);

	/// True if there is an IPCP assigned to the DIF, false otherwise
	bool is_any_ipcp_assigned_to_dif(const rina::ApplicationProcessNamingInformation& dif_name);

	//
	// Internal event API
	//

	//Flow mgmt
	void flow_allocation_requested_event_handler(rina::FlowRequestEvent* event);
	void allocate_flow_response_event_handler( rina::AllocateFlowResponseEvent *event);
	void flow_deallocation_requested_event_handler(rina::FlowDeallocateRequestEvent* event);
	void flow_deallocated_event_handler(rina::FlowDeallocatedEvent* event);
	void ipcm_deallocate_flow_response_event_handler(rina::IpcmDeallocateFlowResponseEvent* event);
	void ipcm_allocate_flow_request_result_handler(rina::IpcmAllocateFlowRequestResultEvent* event);
	void application_flow_allocation_failed_notify(
						rina::FlowRequestEvent *event);
	void flow_allocation_requested_local(rina::FlowRequestEvent *event);

	void flow_allocation_requested_remote(rina::FlowRequestEvent *event);
	ipcm_res_t deallocate_flow(Promise* promise, const int ipcp_id,
			    const rina::FlowDeallocateRequestEvent& event);



	//Application registration mgmt
	void app_reg_req_handler(rina::ApplicationRegistrationRequestEvent *e);
	void app_reg_response_handler(rina::IpcmRegisterApplicationResponseEvent* e);
	void application_unregistration_request_event_handler(rina::ApplicationUnregistrationRequestEvent* event);
	void unreg_app_response_handler(rina::IpcmUnregisterApplicationResponseEvent *e);
	void notify_app_reg(
		const rina::ApplicationRegistrationRequestEvent& req_event,
		const rina::ApplicationProcessNamingInformation& app_name,
		const rina::ApplicationProcessNamingInformation& slave_dif_name,
		bool success);
	void application_manager_app_unregistered(
		rina::ApplicationUnregistrationRequestEvent event,
		int result);
	int ipcm_unregister_response_app(
			rina::IpcmUnregisterApplicationResponseEvent *event,
			IPCMIPCProcess * ipcp,
			rina::ApplicationUnregistrationRequestEvent& req);
	int ipcm_register_response_app(
		rina::IpcmRegisterApplicationResponseEvent * event,
		IPCMIPCProcess * slave_ipcp,
		const rina::ApplicationRegistrationRequestEvent& req_event);

	//IPCP mgmt
	void ipcm_register_response_ipcp(IPCMIPCProcess * ipcp,
		rina::IpcmRegisterApplicationResponseEvent *event);
	void ipcm_unregister_response_ipcp(IPCMIPCProcess * ipcp,
				rina::IpcmUnregisterApplicationResponseEvent *event,
				TransactionState *trans);

	//DIF assignment mgmt
	void assign_to_dif_response_event_handler(rina::AssignToDIFResponseEvent * e);

	//DIF config mgmt
	void update_dif_config_response_event_handler(rina::UpdateDIFConfigurationResponseEvent* e);

	//Enrollment mgmt
	void enroll_to_dif_response_event_handler(rina::EnrollToDIFResponseEvent *e);

	//RIB queries
	void query_rib_response_event_handler(rina::QueryRIBResponseEvent *e);

	//Misc
	void os_process_finalized_handler(rina::OSProcessFinalizedEvent *event);
	void ipc_process_daemon_initialized_event_handler(
				rina::IPCProcessDaemonInitializedEvent *e);
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
				rina::SetPolicySetParamResponseEvent *e);
	void ipc_process_plugin_load_response_handler(
				rina::PluginLoadResponseEvent *e);
	void ipc_process_select_policy_set_response_handler(
					rina::SelectPolicySetResponseEvent *e);

	//
	// Load kernel space policy plugin
	//
	// @param plugin_name Name of the kernel module containing the
	//		      plugin to be loaded
	// @param load        True if the plugin is to be loaded,
	//                    false if the plugin is to be unloaded.
	// @ret IPCM_SUCCESS when load/unload is successful, otherwise
	//	IPCM_FAILURE
	ipcm_res_t plugin_load_kernel(const std::string& plugin_name,
				      bool load);

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

		//Read lock
		rina::ReadScopedLock readlock(trans_rwlock);

		if ( pend_transactions.find(tid) != pend_transactions.end() ) {
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
	int add_transaction_state(TransactionState* t);

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
	SyscallTransState* get_syscall_transaction_state(int tid){
		//Read lock
		rina::ReadScopedLock readlock(trans_rwlock);

		if ( pend_sys_trans.find(tid) !=  pend_sys_trans.end() )
			return pend_sys_trans[tid];

		return NULL;
	};

	/**
	* Add syscall transaction state (waiting for a notification)
	*
	* @ret 0 if success -1 otherwise.
	*/
	int add_syscall_transaction_state(SyscallTransState* t);

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
	std::map<int, SyscallTransState*> pend_sys_trans;

	//Rwlock for transactions
	rina::ReadWriteLockable trans_rwlock;

	//Current logging level
	std::string log_level_;

	//Keep running flag
	volatile bool keep_running;

	//Flag to indicate we have been requested to stop
	volatile bool req_to_stop;

	//IPCM factory
	IPCMIPCProcessFactory ipcp_factory_;

	// RINA configuration internal state
	rinad::RINAConfiguration config;

public:
	//Generator of opaque identifiers
	rina::ConsecutiveUnsignedIntegerGenerator __tid_gen;

	//The DIF template manager
	DIFTemplateManager * dif_template_manager;

	//The DIF Allocator
	DIFAllocator * dif_allocator;

	//Catalog of policies
	Catalog catalog;

private:
	//Singleton
	IPCManager_();
	virtual ~IPCManager_();

	//I/O loop main thread
	rina::Thread* io_thread;
	rina::ThreadAttributes io_thread_attrs;

	//Stop condition
	rina::ConditionVariable stop_cond;

	//Trampoline for the pthread_create
	static void* io_loop_trampoline(void* param);

	//Main I/O loop thread
	void io_loop(void);

	friend class Singleton<rinad::IPCManager_>;

	void pre_assign_to_dif(Addon* callee,
			const rina::ApplicationProcessNamingInformation& dif_name,
			const unsigned short ipcp_id, IPCMIPCProcess*& ipcp);
	void assign_to_dif(Addon* callee, Promise *promise,
			rina::DIFInformation dif_info, IPCMIPCProcess* ipcp);
};


//Singleton instance
extern Singleton<rinad::IPCManager_> IPCManager;

}//rinad namespace

#endif  /* __IPCM_H__ */
