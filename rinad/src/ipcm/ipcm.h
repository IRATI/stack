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

#define RINA_PREFIX     "ipcm"

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/logs.h>

#include "event-loop.h"
#include "rina-configuration.h"
#include "console.h"


#define FLUSH_LOG(_lev_, _ss_)                                          \
                do {                                                    \
                        LOGF_##_lev_ ("%s", (_ss_).str().c_str());       \
                        ss.str(string());                               \
                } while (0)
namespace rinad {

class IPCMConcurrency : public rina::ConditionVariable {
 public:
        bool wait_for_event(rina::IPCEventType ty, unsigned int seqnum,
                            int &result);
        void notify_event(rina::IPCEvent *event);
        void set_event_result(int result) { event_result = result; }

        IPCMConcurrency(unsigned int wt) :
                                wait_time(wt), event_waiting(false) { }

 private:
        unsigned int wait_time;
        bool event_waiting;
        rina::IPCEventType event_ty;
        unsigned int event_sn;
        int event_result;
};

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

class IPCManager : public EventLoopData {
 public:
        IPCManager(unsigned int wait_time);
        ~IPCManager();

        void init(const std::string& logfile, const std::string& loglevel);

        int start_script_worker();
        int start_console_worker();

        int apply_configuration();

        rina::IPCProcess *create_ipcp(
                        const rina::ApplicationProcessNamingInformation& name,
                        const std::string& type);

        int destroy_ipcp(unsigned int ipcp_id);

        int list_ipcps(std::ostream& os);

        int list_ipcp_types(std::ostream& os);

        int assign_to_dif(rina::IPCProcess *ipcp,
                          const rina::ApplicationProcessNamingInformation&
                          difName);

        int register_at_dif(rina::IPCProcess *ipcp,
                            const rina::ApplicationProcessNamingInformation&
                            difName);

        int register_at_difs(rina::IPCProcess *ipcp, const
                        std::list<rina::ApplicationProcessNamingInformation>&
                             difs);

        int enroll_to_dif(rina::IPCProcess *ipcp,
                          const rinad::NeighborData& neighbor, bool sync);

        int enroll_to_difs(rina::IPCProcess *ipcp,
                           const std::list<rinad::NeighborData>& neighbors);

        int unregister_app_from_ipcp(
                const rina::ApplicationUnregistrationRequestEvent& req_event,
                rina::IPCProcess *slave_ipcp);

        int unregister_ipcp_from_ipcp(rina::IPCProcess *ipcp,
                                      rina::IPCProcess *slave_ipcp);

        int update_dif_configuration(
                rina::IPCProcess *ipcp,
                const rina::DIFConfiguration& dif_config);

        int deallocate_flow(rina::IPCProcess *ipcp,
                            const rina::FlowDeallocateRequestEvent& event);

        int select_policy_set(rina::IPCProcess *ipcp,
                              const std::string& component_path,
                              const std::string& policy_set);

        int set_policy_set_param(rina::IPCProcess *ipcp,
                                 const std::string& path,
                                 const std::string& name,
                                 const std::string& value);

        int plugin_load(rina::IPCProcess *ipcp,
                        const std::string& plugin_name, bool load);

        std::string query_rib(rina::IPCProcess *ipcp);

        rinad::RINAConfiguration config;

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

        std::map<unsigned int,
                 rina::IPCProcess *> pending_set_policy_set_param_ops;

        std::map<unsigned int,
                 rina::IPCProcess *> pending_select_policy_set_ops;

        std::map<unsigned int,
                 rina::IPCProcess *> pending_plugin_load_ops;

        IPCMConcurrency concurrency;

 private:
        rina::Thread *script;
        IPCMConsole *console;
};

class DIFConfigValidator {
public:
	enum Types{
		SHIM_ETH,
		NORMAL,
		SHIM_DUMMY
	};
	DIFConfigValidator(const rina::DIFConfiguration &dif_config,
			const rina::DIFInformation &dif_info, std::string type);
	bool validateConfigs();
private:
	Types type_;
	const rina::DIFConfiguration &dif_config_;
    const rina::DIFInformation &dif_info_;

	bool validateShimEth();
	bool validateShimDummy();
	bool validateNormal();
	bool validateBasicDIFConfigs();
	bool validateConfigParameters();
	bool dataTransferConstants();
	bool qosCubes();
	bool knownIPCProcessAddresses();
	bool pdufTableGeneratorConfiguration();
};

void register_handlers_all(EventLoop& loop);

}

/* Macro useful to perform downcasts in declarations. */
#define DOWNCAST_DECL(_var,_class,_name)        \
        _class *_name = dynamic_cast<_class*>(_var);

#endif  /* __IPCM_H__ */
