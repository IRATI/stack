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

#include <librina/common.h>
#include <librina/ipc-manager.h>

#include "event-loop.h"
#include "rina-configuration.h"


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

struct PendingIPCPRegistration {
        rina::IPCProcess *ipcp;
        rina::IPCProcess *slave_ipcp;

        PendingIPCPRegistration() : ipcp(NULL), slave_ipcp(NULL) { }
        PendingIPCPRegistration(rina::IPCProcess *p, rina::IPCProcess *s)
                                : ipcp(p), slave_ipcp(s) { }
};

struct PendingAppRegistration {
        rina::IPCProcess *slave_ipcp;
        rina::ApplicationRegistrationRequestEvent req_event;

        PendingAppRegistration() : slave_ipcp(NULL) { }
        PendingAppRegistration(rina::IPCProcess *p,
                        const rina::ApplicationRegistrationRequestEvent& n)
                                        : slave_ipcp(p), req_event(n) { }
};

struct PendingIPCPUnregistration {
        rina::IPCProcess *slave_ipcp;
        rina::IPCProcess *ipcp;
};

struct PendingAppUnregistration {
        rina::IPCProcess *slave_ipcp;
        rina::ApplicationUnregistrationRequestEvent req_event;

        PendingAppUnregistration() : slave_ipcp(NULL) { }
        PendingAppUnregistration(rina::IPCProcess *p,
                        const rina::ApplicationUnregistrationRequestEvent& n)
                                        : slave_ipcp(p), req_event(n) { }
};


class IPCManager : public EventLoopData {
 public:
        IPCManager();
        ~IPCManager();

        int apply_configuration();

        rina::IPCProcess *create_ipcp(
                        const rina::ApplicationProcessNamingInformation& name,
                        const std::string& type);

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
                          const rinad::NeighborData& neighbor);

        int enroll_to_difs(rina::IPCProcess *ipcp,
                           const std::list<rinad::NeighborData>& neighbors);

        int unregister_app_from_ipcp(
                const rina::ApplicationUnregistrationRequestEvent& req_event,
                rina::IPCProcess *slave_ipcp);

        rinad::RINAConfiguration config;

        std::map<unsigned short, rina::IPCProcess*> pending_normal_ipcp_inits;
        std::map<unsigned int, rina::IPCProcess*> pending_ipcp_dif_assignments;
        std::map<unsigned int, PendingIPCPRegistration> pending_ipcp_registrations;
        std::map<unsigned int, PendingIPCPUnregistration> pending_ipcp_unregistrations;
        std::map<unsigned int, rina::IPCProcess*> pending_ipcp_enrollments;
        std::map<unsigned int, PendingAppRegistration> pending_app_registrations;
        std::map<unsigned int, PendingAppUnregistration> pending_app_unregistrations;

        IPCMConcurrency concurrency;

 private:
        rina::Thread *console;
        rina::Thread *script;
};

void register_handlers_all(EventLoop& loop);

#endif  /* __IPCM_H__ */
