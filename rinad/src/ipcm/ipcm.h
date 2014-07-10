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

#include "event-loop.h"
#include "rina-configuration.h"
#include "console.h"


namespace rinad {

class IPCMConcurrency : public rina::ConditionVariable {
 public:
        void wait_for_event(rina::IPCEventType ty, unsigned int seqnum);
        void notify_event(rina::IPCEvent *event);

        IPCMConcurrency() : event_waiting(false) { }

 private:
        bool event_waiting;
        rina::IPCEventType event_ty;
        unsigned int event_sn;
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
        IPCManager();
        ~IPCManager();

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

        rinad::RINAConfiguration config;

        std::map<unsigned short, rina::IPCProcess*> pending_normal_ipcp_inits;

        std::map<unsigned int, rina::IPCProcess*> pending_ipcp_dif_assignments;

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

        IPCMConcurrency concurrency;

 private:
        IPCMConsole *console;
        rina::Thread *script;
};

void register_handlers_all(EventLoop& loop);

}

/* Macro useful to perform downcasts in declarations. */
#define DOWNCAST_DECL(_var,_class,_name)        \
        _class *_name = dynamic_cast<_class*>(_var);

#endif  /* __IPCM_H__ */
