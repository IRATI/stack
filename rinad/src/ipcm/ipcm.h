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

#include "event-loop.h"
#include "rina-configuration.h"

//Addons
#include "addons/console.h"


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
// @brief Validates the DIF configurations
//
// FIXME: Convert this into an abstract class and make all the subtypes
// derive from the DIF abstract class (e.g. shimEth, ShimHV..)
//
class DIFConfigValidator {
public:
        enum Types{
                NORMAL,
                SHIM_ETH,
                SHIM_DUMMY,
                SHIM_TCP_UDP,
                SHIM_HV,
                SHIM_NOT_DEFINED
        };
        DIFConfigValidator(const rina::DIFConfiguration &dif_config,
                        const rina::DIFInformation &dif_info, std::string type);
        bool validateConfigs();
private:
        Types type_;
        const rina::DIFConfiguration &dif_config_;
        const rina::DIFInformation &dif_info_;

	bool validateShimEth();
	bool validateShimHv();
	bool validateShimDummy();
        bool validateShimTcpUdp();
        bool validateNormal();
	bool validateBasicDIFConfigs();
	bool validateConfigParameters(const std::vector< std::string >&
                                      expected_params);
	bool dataTransferConstants();
	bool qosCubes();
	bool knownIPCProcessAddresses();
	bool pdufTableGeneratorConfiguration();
};


//
// @brief The IPCManager class is in charge of managing the IPC processes
// life-cycle.
//
class IPCManager_ : public EventLoopData {

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
        rina::IPCProcess *create_ipcp(
                        const rina::ApplicationProcessNamingInformation& name,
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
	// List the available IPCP types
	//
        int list_ipcp_types(std::ostream& os);

	//
	// Assing an ipcp to a DIF
	//
        int assign_to_dif(rina::IPCProcess *ipcp,
                          const rina::ApplicationProcessNamingInformation&
                          difName);

	//
	// Register an IPCP to a single DIF
	//
        int register_at_dif(rina::IPCProcess *ipcp,
                            const rina::ApplicationProcessNamingInformation&
                            difName);

	//
	// Register an existing IPCP to multiple DIFs
	//
        int register_at_difs(rina::IPCProcess *ipcp, const
                        std::list<rina::ApplicationProcessNamingInformation>&
                             difs);

	//
	// Enroll IPCP to a single DIF
	//
        int enroll_to_dif(rina::IPCProcess *ipcp,
                          const rinad::NeighborData& neighbor, bool sync);
	//
	// Enroll IPCP to multiple DIFs
	//
        int enroll_to_difs(rina::IPCProcess *ipcp,
                           const std::list<rinad::NeighborData>& neighbors);

	//TODO
        int unregister_app_from_ipcp(
                const rina::ApplicationUnregistrationRequestEvent& req_event,
                rina::IPCProcess *slave_ipcp);

	//TODO
        int unregister_ipcp_from_ipcp(rina::IPCProcess *ipcp,
                                      rina::IPCProcess *slave_ipcp);

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
                rina::IPCProcess *ipcp,
                const rina::DIFConfiguration& dif_config);

        int deallocate_flow(rina::IPCProcess *ipcp,
                            const rina::FlowDeallocateRequestEvent& event);

	//
	// Retrieve the IPCP RIB in the form of a string
	//
        std::string query_rib(rina::IPCProcess *ipcp);

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

//------------FIXME: all this section MUST be protected/private---------------
	//and handlers (C linkage) should call methods of IPCM and lock there.
	IPCMConcurrency concurrency;

        std::map<unsigned short, rina::IPCProcess*> pending_normal_ipcp_inits;

        std::map<unsigned int, rina::IPCProcess*> pending_ipcp_dif_assignments;

        std::map<unsigned int, rina::IPCProcess*> pending_ipcp_query_rib_responses;

        std::map<unsigned int, std::string > query_rib_responses;

        std::map<unsigned int,
                 std::pair<rina::IPCProcess*, rina::IPCProcess*>
                > pending_ipcp_registrations;

        std::map<unsigned int,
                 std::pair<rina::IPCProcess*, rina::IPCProcess*>
                > pending_ipcp_unregistrations;

        std::map<unsigned int, rina::IPCProcess*> pending_ipcp_enrollments;

        std::map<unsigned int,
                 std::pair<rina::IPCProcess*,
                           rina::ApplicationRegistrationRequestEvent
                          >
                > pending_app_registrations;

        std::map<unsigned int,
                 std::pair<rina::IPCProcess*,
                           rina::ApplicationUnregistrationRequestEvent
                          >
                > pending_app_unregistrations;

        std::map<unsigned int, rina::IPCProcess *> pending_dif_config_updates;

        std::map<unsigned int, PendingFlowAllocation> pending_flow_allocations;

        std::map<unsigned int,
                 std::pair<rina::IPCProcess *,
                           rina::FlowDeallocateRequestEvent
                          >
                > pending_flow_deallocations;

//-------------------------END OF FIXME---------------------------------------
private:


	//
	// RINA configuration internal state
	//
        rinad::RINAConfiguration config;

	//Script thread
        rina::Thread *script;

	//IPCM Console instance
        IPCMConsole *console;

	//Current logging level
        std::string log_level_;
};


//Singleton instance
extern Singleton<rinad::IPCManager_> IPCManager;

//TODO: is this really needed?
extern void register_handlers_all(EventLoop& loop);

}//rina namespace

#endif  /* __IPCM_H__ */
