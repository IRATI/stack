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

//
// @brief Pending Flow allocation object
//
// TODO: revise
//
struct PendingFlowAllocation {
        rina::IPCProcess *slave_ipcp;
        rina::FlowRequestEvent req_event;
        bool try_only_a_dif;

        PendingFlowAllocation(): slave_ipcp(NULL),
                                 try_only_a_dif(true) { }
        PendingFlowAllocation(rina::IPCProcess *p,
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

	IPCManager_();
	virtual ~IPCManager_();

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
	// Creates an IPCP process
	//
        int create_ipcp(const rina::ApplicationProcessNamingInformation& name,
                        const std::string& type);

	//
	// Destroys an IPCP process
	//
        int destroy_ipcp(unsigned int ipcp_id);

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
        int list_ipcp_types(std::ostream& os);

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
			std::pair<rina::IPCProcess *,
				rina::ApplicationRegistrationRequestEvent>
		>::iterator mit);
	void application_manager_app_unregistered(
                rina::ApplicationUnregistrationRequestEvent event,
                int result);
	int ipcm_unregister_response_app(
			rina::IpcmUnregisterApplicationResponseEvent *event,
			std::map<unsigned int,
				std::pair<rina::IPCProcess *,
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
			std::pair<rina::IPCProcess *, rina::IPCProcess *>
		>::iterator mit);
	int ipcm_unregister_response_ipcp(
				rina::IpcmUnregisterApplicationResponseEvent *event,
				std::map<unsigned int,
				    std::pair<rina::IPCProcess *, rina::IPCProcess *>
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
	void ipc_process_daemon_initialized_event_handler(rina::IPCEvent * e);
	void timer_expired_event_handler(rina::IPCEvent *event);
	bool ipcm_register_response_common(
		rina::IpcmRegisterApplicationResponseEvent *event,
		const rina::ApplicationProcessNamingInformation& app_name,
		rina::IPCProcess *slave_ipcp,
		const rina::ApplicationProcessNamingInformation& slave_dif_name);

	bool ipcm_register_response_common(
		rina::IpcmRegisterApplicationResponseEvent *event,
		const rina::ApplicationProcessNamingInformation& app_name,
		rina::IPCProcess *slave_ipcp);


	bool ipcm_unregister_response_common(
				 rina::IpcmUnregisterApplicationResponseEvent *event,
				 rina::IPCProcess *slave_ipcp,
				 const rina::ApplicationProcessNamingInformation&);

	//Plugins
	void ipc_process_set_policy_set_param_response_handler(
							rina::IPCEvent *e);
	void ipc_process_plugin_load_response_handler(rina::IPCEvent *e);
	void ipc_process_select_policy_set_response_handler(
							rina::IPCEvent *e);

	//
	// TODO: revise
	//
	IPCMConcurrency concurrency;

	//Pending IPCP initializations
        std::map<unsigned short, rina::IPCProcess*> pending_normal_ipcp_inits;

	//Pending IPCP DIF assignments
        std::map<unsigned int, rina::IPCProcess*> pending_ipcp_dif_assignments;

	//Pending query RIB
        std::map<unsigned int, rina::IPCProcess*> pending_ipcp_query_rib_responses;

	//Query RIB responses
        std::map<unsigned int, std::string > query_rib_responses;

	//Pending IPCP registrations
        std::map<unsigned int,
                 std::pair<rina::IPCProcess*, rina::IPCProcess*>
                > pending_ipcp_registrations;

	//Pending IPCP unregistration
        std::map<unsigned int,
                 std::pair<rina::IPCProcess*, rina::IPCProcess*>
                > pending_ipcp_unregistrations;

	//Pending IPCP enrollments
        std::map<unsigned int, rina::IPCProcess*> pending_ipcp_enrollments;

	//Pending AP registrations
        std::map<unsigned int,
                 std::pair<rina::IPCProcess*,
                           rina::ApplicationRegistrationRequestEvent
                          >
                > pending_app_registrations;

	//Pending AP unregistrations
        std::map<unsigned int,
                 std::pair<rina::IPCProcess*,
                           rina::ApplicationUnregistrationRequestEvent
                          >
                > pending_app_unregistrations;

	//Pending DIF config updates
        std::map<unsigned int, rina::IPCProcess *> pending_dif_config_updates;

	//Pending flow allocations
        std::map<unsigned int, PendingFlowAllocation> pending_flow_allocations;

	//Pending flow deallocations
        std::map<unsigned int,
                 std::pair<rina::IPCProcess *,
                           rina::FlowDeallocateRequestEvent
                          >
                > pending_flow_deallocations;

	//
	// RINA configuration internal state
	//
        rinad::RINAConfiguration config;

	/* FIXME REMOVE THIS*/
        std::map<unsigned int,
                 rina::IPCProcess *> pending_set_policy_set_param_ops;

        std::map<unsigned int,
                 rina::IPCProcess *> pending_select_policy_set_ops;

        std::map<unsigned int,
                 rina::IPCProcess *> pending_plugin_load_ops;
	/* FIXME REMOVE THIS*/

	//Script thread
        rina::Thread *script;

	//IPCM Console instance
        IPCMConsole *console;

	//Current logging level
        std::string log_level_;

	//Keep running flag
	volatile bool keep_running;
};


//Singleton instance
extern Singleton<rinad::IPCManager_> IPCManager;

}//rinad namespace

#endif  /* __IPCM_H__ */
