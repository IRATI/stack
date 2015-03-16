/*
 * IPC Manager - Registration and unregistration
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
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

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

#define RINA_PREFIX "ipcm.app"
#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/logs.h>

#include "rina-configuration.h"
#include "ipcm.h"

using namespace std;

namespace rinad {

void IPCManager_::os_process_finalized_handler(rina::IPCEvent *e)
{
	DOWNCAST_DECL(e, rina::OSProcessFinalizedEvent, event);
	const vector<rina::IPCProcess *>& ipcps =
		rina::ipcProcessFactory->listIPCProcesses();
	const rina::ApplicationProcessNamingInformation& app_name =
						event->applicationName;
	list<rina::FlowInformation> involved_flows;
	ostringstream ss;

	ss  << "Application " << app_name.toString()
			<< "terminated" << endl;
	FLUSH_LOG(INFO, ss);

	// Look if the terminating application has allocated flows
	// with some IPC processes
	collect_flows_by_application(app_name, involved_flows);
	for (list<rina::FlowInformation>::iterator fit = involved_flows.begin();
			fit != involved_flows.end(); fit++) {
		rina::IPCProcess *ipcp = select_ipcp_by_dif(fit->difName);
		rina::FlowDeallocateRequestEvent req_event(fit->portId, 0);

		if (!ipcp) {
			ss  << ": Cannot find the IPC process "
				"that provides the flow with port-id " <<
				fit->portId << endl;
			FLUSH_LOG(ERR, ss);
			continue;
		}

		IPCManager->deallocate_flow(ipcp->id, req_event);
	}

	// Look if the terminating application has pending registrations
	// with some IPC processes
	for (unsigned int i = 0; i < ipcps.size(); i++) {
		if (application_is_registered_to_ipcp(app_name,
							    ipcps[i])) {
			// Build a structure that will be used during
			// the unregistration process. The last argument
			// is the request sequence number: 0 means that
			// the unregistration response does not match
			// an application request - this is indeed an
			// unregistration forced by the IPCM.
			rina::ApplicationUnregistrationRequestEvent
				req_event(app_name, ipcps[i]->
					getDIFInformation().dif_name_, 0);

			IPCManager->unregister_app_from_ipcp(req_event, ipcps[i]->id);
		}
	}

	if (event->ipcProcessId != 0) {
		// TODO The process that crashed was an IPC Process daemon
		// Should we destroy the state in the kernel? Or try to
		// create another IPC Process in user space to bring it back?
	}
}

int
IPCManager_::unregister_app_from_ipcp(
                const rina::ApplicationUnregistrationRequestEvent& req_event,
                int slave_ipcp_id)
{
        ostringstream ss;
        unsigned int seqnum;
        rina::IPCProcess *slave_ipcp;

        try {
		slave_ipcp = lookup_ipcp_by_id(slave_ipcp_id);

		if (!slave_ipcp) {
			ss << "Cannot find any IPC process belonging "<<endl;
			FLUSH_LOG(ERR, ss);
			throw Exception();
		}

                // Forward the unregistration request to the IPC process
                // that the application is registered to
				seqnum = opaque_generator_.next();
                slave_ipcp->unregisterApplication(req_event.applicationName,
                                                  seqnum);
                pending_app_unregistrations[seqnum] =
                                make_pair(slave_ipcp, req_event);

                ss << "Requested unregistration of application " <<
                        req_event.applicationName.toString() << " from IPC "
                        "process " << slave_ipcp->name.toString() << endl;
                FLUSH_LOG(INFO, ss);
        } catch (rina::IpcmUnregisterApplicationException) {
                ss  << ": Error while unregistering application "
                        << req_event.applicationName.toString() << " from IPC "
                        "process " << slave_ipcp->name.toString() << endl;
                FLUSH_LOG(ERR, ss);
                return -1;
        }

        return 0;
}


void IPCManager_::application_registration_notify(
        const rina::ApplicationRegistrationRequestEvent& req_event,
        const rina::ApplicationProcessNamingInformation& app_name,
        const rina::ApplicationProcessNamingInformation& slave_dif_name,
        bool success)
{
        ostringstream ss;

        try {
                rina::applicationManager->applicationRegistered(req_event,
                                slave_dif_name, success ? 0 : -1);
                ss << "Application " << app_name.toString() <<
                        " informed about its registration "
                        "to N-1 DIF " << slave_dif_name.toString() <<
                        " [success = " << success << "]" << endl;
                FLUSH_LOG(INFO, ss);
        } catch (rina::NotifyApplicationRegisteredException) {
                ss  << "Error while notifying application "
                        << app_name.toString() << " about registration "
                        "to DIF " << slave_dif_name.toString() << endl;
                FLUSH_LOG(ERR, ss);
        }
}

void IPCManager_::application_registration_request_event_handler(rina::IPCEvent *e)
{
        DOWNCAST_DECL(e, rina::ApplicationRegistrationRequestEvent, event);
        const rina::ApplicationRegistrationInformation& info = event->
                                applicationRegistrationInformation;
        const rina::ApplicationProcessNamingInformation app_name =
                                info.appName;
        rina::IPCProcess *slave_ipcp = NULL;
        unsigned int seqnum;
        ostringstream ss;

        if (info.applicationRegistrationType ==
                        rina::APPLICATION_REGISTRATION_ANY_DIF) {
                rina::ApplicationProcessNamingInformation dif_name;
                bool found;

                // See if the configuration specifies a mapping between
                // the registering application and a DIF.
                found = lookup_dif_by_application(app_name,
                                                              dif_name);
                if (found) {
                        // If a mapping exists, select an IPC process
                        // from the specified DIF.
                        slave_ipcp = select_ipcp_by_dif(dif_name);
                } else {
                        // Otherwise select any IPC process.
                        slave_ipcp = select_ipcp();
                }
        } else if (info.applicationRegistrationType ==
                        rina::APPLICATION_REGISTRATION_SINGLE_DIF) {
                // Select an IPC process from the DIF specified in the
                // registration request.
                slave_ipcp = select_ipcp_by_dif(info.difName);
        } else {
                ss  << ": Unsupported registration type: "
                        << info.applicationRegistrationType << endl;
                FLUSH_LOG(ERR, ss);

                // Notify the application about the unsuccessful registration.
                application_registration_notify(*event, app_name,
                        rina::ApplicationProcessNamingInformation(), false);
                return;
        }

        if (!slave_ipcp) {
                ss  << ": Cannot find a suitable DIF to "
                        "register application " << app_name.toString() << endl;
                FLUSH_LOG(ERR, ss);
                // Notify the application about the unsuccessful registration.
                application_registration_notify(*event, app_name,
                        rina::ApplicationProcessNamingInformation(), false);
                return;
        }

        try {
        		seqnum = opaque_generator_.next();
                slave_ipcp->registerApplication(app_name,
                                                info.ipcProcessId,
                                                seqnum);
                pending_app_registrations[seqnum] =
                                        make_pair(slave_ipcp, *event);

                ss << "Requested registration of application " <<
                        app_name.toString() << " to IPC process " <<
                        slave_ipcp->name.toString() << endl;
                FLUSH_LOG(INFO, ss);
        } catch (rina::IpcmRegisterApplicationException) {
                pending_app_registrations.erase(seqnum);
                ss  << ": Error while registering application "
                        << app_name.toString() << endl;
                FLUSH_LOG(ERR, ss);
                // Notify the application about the unsuccessful registration.
                application_registration_notify(*event, app_name,
                                                slave_ipcp->name, false);
        }
}

bool IPCManager_::ipcm_register_response_common(
        rina::IpcmRegisterApplicationResponseEvent *event,
        const rina::ApplicationProcessNamingInformation& app_name,
        rina::IPCProcess *slave_ipcp,
        const rina::ApplicationProcessNamingInformation& slave_dif_name)
{
        bool success = (event->result == 0);
        ostringstream ss;

        try {
                // Notify the N-1 IPC process.
                slave_ipcp->registerApplicationResult(
                                        event->sequenceNumber, success);
        } catch (rina::IpcmRegisterApplicationException) {
                ss << ": Error while reporting "
                        "registration result of application "
                        << app_name.toString() <<
                        " to N-1 DIF " << slave_dif_name.toString()
                        << endl;
                FLUSH_LOG(ERR, ss);
        }

        return success;
}


int IPCManager_::ipcm_register_response_app(
        rina::IpcmRegisterApplicationResponseEvent *event,
        map<unsigned int,
            std::pair<rina::IPCProcess *,
                      rina::ApplicationRegistrationRequestEvent
                     >
           >::iterator mit)
{
        rina::IPCProcess *slave_ipcp = mit->second.first;
        const rina::ApplicationRegistrationRequestEvent& req_event =
                        mit->second.second;
        const rina::ApplicationProcessNamingInformation& app_name =
                req_event.applicationRegistrationInformation.
                appName;
        const rina::ApplicationProcessNamingInformation&
                slave_dif_name = slave_ipcp->
                getDIFInformation().dif_name_;
        ostringstream ss;
        bool success;

        success = ipcm_register_response_common(event, app_name, slave_ipcp,
                                           slave_dif_name);

        // Notify the application about the (un)successful registration.
        application_registration_notify(req_event, app_name,
                                        slave_dif_name, success);

        pending_app_registrations.erase(mit);

        return success ? 0 : -1;
}

void IPCManager_::ipcm_register_app_response_event_handler(rina::IPCEvent *e)
{

        DOWNCAST_DECL(e, rina::IpcmRegisterApplicationResponseEvent, event);
        map<unsigned int,
            std::pair<rina::IPCProcess *, rina::IPCProcess*>
           >::iterator it;
        map<unsigned int,
            std::pair<rina::IPCProcess *,
                      rina::ApplicationRegistrationRequestEvent
                     >
           >::iterator jt;
        ostringstream ss;
        int ret = -1;

        it = pending_ipcp_registrations.find(event->sequenceNumber);
        jt = pending_app_registrations.find(event->sequenceNumber);

        if (it != pending_ipcp_registrations.end()) {
                ret = ipcm_register_response_ipcp(event, it);
        } else if (jt != pending_app_registrations.end()) {
                ret = ipcm_register_response_app(event, jt);
        } else {
                ss << ": Warning: DIF assignment response "
                        "received, but no pending DIF assignment " << endl;
                FLUSH_LOG(WARN, ss);
        }

        //concurrency.set_event_result(ret);
}

void IPCManager_::application_manager_app_unregistered(
                rina::ApplicationUnregistrationRequestEvent event,
                int result)
{
        ostringstream ss;

        // Inform the application about the unregistration result
        try {
                if (event.sequenceNumber > 0) {
                        rina::applicationManager->
                                        applicationUnregistered(event, result);
                        ss << "Application " << event.applicationName.
                                toString() << " informed about its "
                                "unregistration [success = " << (!result) <<
                                "]" << endl;
                        FLUSH_LOG(INFO, ss);
                }
        } catch (rina::NotifyApplicationUnregisteredException) {
                ss  << ": Error while notifying application "
                        << event.applicationName.toString() << " about "
                        "failed unregistration" << endl;
                FLUSH_LOG(ERR, ss);
        }
}

void IPCManager_::application_unregistration_request_event_handler(rina::IPCEvent *e)
{
        DOWNCAST_DECL(e, rina::ApplicationUnregistrationRequestEvent, event);
        // Select any IPC process in the DIF from which the application
        // wants to unregister from
        rina::IPCProcess *slave_ipcp = select_ipcp_by_dif(event->DIFName);
        ostringstream ss;
        int err;

        if (!slave_ipcp) {
                ss  << ": Error: Application " <<
                        event->applicationName.toString() <<
                        " wants to unregister from DIF " <<
                        event->DIFName.toString() << " but couldn't find "
                        "any IPC process belonging to it" << endl;
                FLUSH_LOG(ERR, ss);

                application_manager_app_unregistered(*event, -1);
                return;
        }

        err = unregister_app_from_ipcp(*event, slave_ipcp->id);
        if (err) {
                // Inform the unregistering application that the unregistration
                // operation failed
                application_manager_app_unregistered(*event, -1);
        }
}

bool IPCManager_::ipcm_unregister_response_common(
                        rina::IpcmUnregisterApplicationResponseEvent *event,
                        rina::IPCProcess *slave_ipcp,
                        const rina::ApplicationProcessNamingInformation&
                        app_name)
{
        bool success = (event->result == 0);
        ostringstream ss;

        try {
                // Inform the IPC process about the application unregistration
                slave_ipcp->unregisterApplicationResult(event->
                                sequenceNumber, success);
                ss << "IPC process " << slave_ipcp->name.toString() <<
                        " informed about unregistration of application "
                        << app_name.toString() << " [success = " << success
                        << "]" << endl;
                FLUSH_LOG(INFO, ss);
        } catch (rina::IpcmRegisterApplicationException) {
                ss  << ": Error while reporing "
                        "unregistration result for application "
                        << app_name.toString() << endl;
                FLUSH_LOG(ERR, ss);
        }

        return success;
}

int IPCManager_::ipcm_unregister_response_app(
                        rina::IpcmUnregisterApplicationResponseEvent *event,
                        map<unsigned int,
                            pair<rina::IPCProcess *,
                                 rina::ApplicationUnregistrationRequestEvent
                                >
                           >::iterator mit)
{
        rina::ApplicationUnregistrationRequestEvent& req_event =
                                        mit->second.second;

        // Inform the supporting IPC process
        ipcm_unregister_response_common(event, mit->second.first,
                                        req_event.applicationName);

        // Inform the application
        application_manager_app_unregistered(req_event,
                                             event->result);

        pending_app_unregistrations.erase(mit);

        return 0;
}

void IPCManager_::ipcm_unregister_app_response_event_handler(rina::IPCEvent *e)
{
        DOWNCAST_DECL(e, rina::IpcmUnregisterApplicationResponseEvent, event);
        map<unsigned int,
            std::pair<rina::IPCProcess *,
                      rina::ApplicationUnregistrationRequestEvent>
           >::iterator it;
        map<unsigned int,
            std::pair<rina::IPCProcess *, rina::IPCProcess *>
           >::iterator jt;
        ostringstream ss;
        int ret = -1;

        it = pending_app_unregistrations.find(event->sequenceNumber);
        jt = pending_ipcp_unregistrations.find(event->sequenceNumber);
        if (it != pending_app_unregistrations.end()) {
                ret = ipcm_unregister_response_app(event, it);
        } else if (jt != pending_ipcp_unregistrations.end()) {
                ret = ipcm_unregister_response_ipcp(event, jt);
        } else {
                ss << ": Warning: Unregistration response "
                        "received, but no pending DIF assignment " << endl;
                FLUSH_LOG(WARN, ss);
        }

        //concurrency.set_event_result(ret);
}

void IPCManager_::register_application_response_event_handler(rina::IPCEvent *event)
{
        (void) event; // Stop compiler barfs
}

void IPCManager_::unregister_application_response_event_handler(rina::IPCEvent *event)
{
        (void) event; // Stop compiler barfs
}

void IPCManager_::application_registration_canceled_event_handler(rina::IPCEvent *event)
{
        (void) event; // Stop compiler barfs
}

}
