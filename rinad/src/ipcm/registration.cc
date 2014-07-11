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

#define RINA_PREFIX     "ipcm"

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/logs.h>

#include "event-loop.h"
#include "rina-configuration.h"
#include "helpers.h"
#include "ipcm.h"

using namespace std;

namespace rinad {

int
IPCManager::unregister_app_from_ipcp(
                const rina::ApplicationUnregistrationRequestEvent& req_event,
                rina::IPCProcess *slave_ipcp)
{
        unsigned int seqnum;

        try {
                // Forward the unregistration request to the IPC process
                // that the application is registered to
                seqnum = slave_ipcp->unregisterApplication(req_event.
                                                           applicationName);
                pending_app_unregistrations[seqnum] =
                                make_pair(slave_ipcp, req_event);

                cout << "Requested unregistration of application " <<
                        req_event.applicationName.toString() << " from IPC "
                        "process " << slave_ipcp->name.toString() << endl;
        } catch (rina::IpcmUnregisterApplicationException) {
                cerr << __func__ << ": Error while unregistering application "
                        << req_event.applicationName.toString() << " from IPC "
                        "process " << slave_ipcp->name.toString() << endl;
                return -1;
        }

        return 0;
}

int
IPCManager::unregister_ipcp_from_ipcp(rina::IPCProcess *ipcp,
                                      rina::IPCProcess *slave_ipcp)
{
        unsigned int seqnum;

        try {
                // Forward the unregistration request to the IPC process
                // that the client IPC process is registered to
                seqnum = slave_ipcp->unregisterApplication(ipcp->name);
                pending_ipcp_unregistrations[seqnum] =
                                make_pair(ipcp, slave_ipcp);

                cout << "Requested unregistration of IPC process " <<
                        ipcp->name.toString() << " from IPC "
                        "process " << slave_ipcp->name.toString() << endl;

                concurrency.wait_for_event(
                                rina::IPCM_UNREGISTER_APP_RESPONSE_EVENT,
                                seqnum);
        } catch (rina::IpcmUnregisterApplicationException) {
                cerr << __func__ << ": Error while unregistering IPC process "
                        << ipcp->name.toString() << " from IPC "
                        "process " << slave_ipcp->name.toString() << endl;
                return -1;
        }

        return 0;
}

void
application_registration_request_event_handler(rina::IPCEvent *e,
                                               EventLoopData *opaque)
{
        DOWNCAST_DECL(e, rina::ApplicationRegistrationRequestEvent, event);
        DOWNCAST_DECL(opaque, IPCManager, ipcm);
        const rina::ApplicationRegistrationInformation& info = event->
                                applicationRegistrationInformation;
        const rina::ApplicationProcessNamingInformation app_name =
                                info.appName;
        rina::IPCProcess *slave_ipcp = NULL;
        unsigned int seqnum;

        if (info.applicationRegistrationType ==
                        rina::APPLICATION_REGISTRATION_ANY_DIF) {
                rina::ApplicationProcessNamingInformation dif_name;
                bool found;

                // See if the configuration specifies a mapping between
                // the registering application and a DIF.
                found = ipcm->config.lookup_dif_by_application(app_name,
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
                cerr << __func__ << ": Unsupported registration type: "
                        << info.applicationRegistrationType << endl;
                return;
        }

        if (!slave_ipcp) {
                cerr << __func__ << ": Cannot find a suitable DIF to "
                        "register application " << app_name.toString() << endl;
                return;
        }

        try {
                seqnum = slave_ipcp->registerApplication(app_name,
                                                info.ipcProcessId);
                ipcm->pending_app_registrations[seqnum] =
                                        make_pair(slave_ipcp, *event);

                cout << "Requested registration of application " <<
                        app_name.toString() << " to IPC process " <<
                        slave_ipcp->name.toString() << endl;
        } catch (rina::IpcmRegisterApplicationException) {
                cerr << __func__ << ": Error while registering application "
                        << app_name.toString() << endl;
        }
}

static bool ipcm_register_response_common(
        rina::IpcmRegisterApplicationResponseEvent *event,
        const rina::ApplicationProcessNamingInformation& app_name,
        rina::IPCProcess *slave_ipcp,
        const rina::ApplicationProcessNamingInformation& slave_dif_name)
{
        bool success = (event->result == 0);

        try {
                /* Notify the N-1 IPC process. */
                slave_ipcp->registerApplicationResult(
                                        event->sequenceNumber, success);

                cout << "IPC process " << slave_ipcp->name.toString() <<
                        " informed about registration of application " <<
                        app_name.toString() << " to N-1 DIF " <<
                        slave_dif_name.toString() << endl;
        } catch (rina::IpcmRegisterApplicationException) {
                cerr <<  __func__ << ": Error while reporting "
                        "registration result of application "
                        << app_name.toString() <<
                        " to N-1 DIF " << slave_dif_name.toString()
                        << endl;
        }

        return success;
}

static void ipcm_register_response_ipcp(
        rina::IpcmRegisterApplicationResponseEvent *event,
        IPCManager *ipcm,
        map<unsigned int,
            std::pair<rina::IPCProcess *, rina::IPCProcess *>
           >::iterator mit)
{
        rina::IPCProcess *ipcp = mit->second.first;
        rina::IPCProcess *slave_ipcp = mit->second.second;
        const rina::ApplicationProcessNamingInformation&
                slave_dif_name = slave_ipcp->
                getDIFInformation().dif_name_;
        bool success;

        success = ipcm_register_response_common(event, ipcp->name,
                                        slave_ipcp, slave_dif_name);
        if (success) {
                /* Notify the registered IPC process. */
                try {
                        ipcp->notifyRegistrationToSupportingDIF(
                                        slave_ipcp->name,
                                        slave_dif_name
                                        );
                        cout << "IPC process " << ipcp->name.toString() <<
                                " informed about its registration "
                                "to N-1 DIF " <<
                                slave_dif_name.toString() << endl;
                } catch (rina::NotifyRegistrationToDIFException) {
                        cerr << __func__ << ": Error while notifying "
                                "IPC process " <<
                                ipcp->name.toString() <<
                                " about registration to N-1 DIF"
                                << slave_dif_name.toString() << endl;
                }
        } else {
                cerr << __func__ << "Cannot register IPC process "
                        << ipcp->name.toString() <<
                        " to DIF " << slave_dif_name.toString() << endl;
        }

        ipcm->pending_ipcp_registrations.erase(mit);
}

static void ipcm_register_response_app(
        rina::IpcmRegisterApplicationResponseEvent *event,
        IPCManager *ipcm,
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
        bool success;

        success = ipcm_register_response_common(event, app_name, slave_ipcp,
                                           slave_dif_name);

        /* Notify the application about the successful registration. */
        try {
                rina::applicationManager->applicationRegistered(req_event,
                                slave_dif_name, success ? 0 : -1);
                cout << "Application " << app_name.toString() <<
                        " informed about its registration "
                        "to N-1 DIF " << slave_dif_name.toString() <<
                        " [success = " << success << "]" << endl;
        } catch (rina::NotifyApplicationRegisteredException) {
                cerr << __func__ << "Error while notifying application "
                        << app_name.toString() << " about registration "
                        "to DIF " << slave_dif_name.toString() << endl;
        }

        ipcm->pending_app_registrations.erase(mit);
}

void
ipcm_register_app_response_event_handler(rina::IPCEvent *e,
                                         EventLoopData *opaque)
{

        DOWNCAST_DECL(e, rina::IpcmRegisterApplicationResponseEvent, event);
        DOWNCAST_DECL(opaque, IPCManager, ipcm);
        map<unsigned int,
            std::pair<rina::IPCProcess *, rina::IPCProcess*>
           >::iterator it;
        map<unsigned int,
            std::pair<rina::IPCProcess *,
                      rina::ApplicationRegistrationRequestEvent
                     >
           >::iterator jt;

        it = ipcm->pending_ipcp_registrations.find(event->sequenceNumber);
        jt = ipcm->pending_app_registrations.find(event->sequenceNumber);

        if (it != ipcm->pending_ipcp_registrations.end()) {
                ipcm_register_response_ipcp(event, ipcm, it);
        } else if (jt != ipcm->pending_app_registrations.end()) {
                ipcm_register_response_app(event, ipcm, jt);
        } else {
                cerr <<  __func__ << ": Warning: DIF assignment response "
                        "received, but no pending DIF assignment " << endl;
        }
}

static void
application_manager_app_unregistered(
                rina::ApplicationUnregistrationRequestEvent event,
                int result)
{
        // Inform the application about the unregistration result
        try {
                if (event.sequenceNumber > 0) {
                        rina::applicationManager->
                                        applicationUnregistered(event, result);
                        cout << "Application " << event.applicationName.
                                toString() << " informed about its "
                                "unregistration [success = " << (!result) <<
                                "]" << endl;
                }
        } catch (rina::NotifyApplicationUnregisteredException) {
                cerr << __func__ << ": Error while notifying application "
                        << event.applicationName.toString() << " about "
                        "failed unregistration" << endl;
        }
}

void
application_unregistration_request_event_handler(rina::IPCEvent *e,
                                                 EventLoopData *opaque)
{
        DOWNCAST_DECL(e, rina::ApplicationUnregistrationRequestEvent, event);
        DOWNCAST_DECL(opaque, IPCManager, ipcm);
        // Select any IPC process in the DIF from which the application
        // wants to unregister from
        rina::IPCProcess *slave_ipcp = select_ipcp_by_dif(event->DIFName);
        int err;

        if (!slave_ipcp) {
                cerr << __func__ << ": Error: Application " <<
                        event->applicationName.toString() <<
                        " wants to unregister from DIF " <<
                        event->DIFName.toString() << " but couldn't find "
                        "any IPC process belonging to it" << endl;

                application_manager_app_unregistered(*event, -1);
                return;
        }

        err = ipcm->unregister_app_from_ipcp(*event, slave_ipcp);
        if (err) {
                // Inform the unregistering application that the unregistration
                // operation failed
                application_manager_app_unregistered(*event, -1);
        }
}

static bool ipcm_unregister_response_common(
                        rina::IpcmUnregisterApplicationResponseEvent *event,
                        rina::IPCProcess *slave_ipcp,
                        const rina::ApplicationProcessNamingInformation&
                        app_name)
{
        bool success = (event->result == 0);

        try {
                // Inform the IPC process about the application unregistration
                slave_ipcp->unregisterApplicationResult(event->
                                sequenceNumber, success);
                cout << "IPC process " << slave_ipcp->name.toString() <<
                        " informed about unregistration of application "
                        << app_name.toString() << " [success = " << success
                        << "]" << endl;
        } catch (rina::IpcmRegisterApplicationException) {
                cerr << __func__ << ": Error while reporing "
                        "unregistration result for application "
                        << app_name.toString() << endl;
        }

        return success;
}

static void ipcm_unregister_response_ipcp(
                        rina::IpcmUnregisterApplicationResponseEvent *event,
                        IPCManager *ipcm,
                        map<unsigned int,
                            pair<rina::IPCProcess *, rina::IPCProcess *>
                           >::iterator mit)
{
        rina::IPCProcess *ipcp = mit->second.first;
        rina::IPCProcess *slave_ipcp = mit->second.second;
        rina::ApplicationProcessNamingInformation slave_dif_name = slave_ipcp->
                                                getDIFInformation().dif_name_;
        bool success;

        // Inform the supporting IPC process
        success = ipcm_unregister_response_common(event, slave_ipcp,
                                                  ipcp->name);

        try {
                if (success) {
                        // Notify the IPCP process that it has been unregistered
                        // from the DIF
                        ipcp->notifyUnregistrationFromSupportingDIF(
                                                        slave_ipcp->name,
                                                        slave_dif_name);
                        cout << "IPC process " << ipcp->name.toString() <<
                                " informed about its unregistration from DIF "
                                << slave_dif_name.toString() << endl;
                } else {
                        cerr << __func__ << ": Cannot unregister IPC Process "
                                << ipcp->name.toString() << " from DIF " <<
                                slave_dif_name.toString() << endl;
                }
        } catch (rina::NotifyRegistrationToDIFException) {
                cerr << __func__ << ": Error while reporing "
                        "unregistration result for IPC process "
                        << ipcp->name.toString() << endl;
        }

        ipcm->pending_ipcp_unregistrations.erase(mit);
}

static void ipcm_unregister_response_app(
                        rina::IpcmUnregisterApplicationResponseEvent *event,
                        IPCManager *ipcm,
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

        ipcm->pending_app_unregistrations.erase(mit);
}

void
ipcm_unregister_app_response_event_handler(rina::IPCEvent *e,
                                           EventLoopData *opaque)
{
        DOWNCAST_DECL(e, rina::IpcmUnregisterApplicationResponseEvent, event);
        DOWNCAST_DECL(opaque, IPCManager, ipcm);
        map<unsigned int,
            std::pair<rina::IPCProcess *,
                      rina::ApplicationUnregistrationRequestEvent>
           >::iterator it;
        map<unsigned int,
            std::pair<rina::IPCProcess *, rina::IPCProcess *>
           >::iterator jt;

        it = ipcm->pending_app_unregistrations.find(event->sequenceNumber);
        jt = ipcm->pending_ipcp_unregistrations.find(event->sequenceNumber);
        if (it != ipcm->pending_app_unregistrations.end()) {
                ipcm_unregister_response_app(event, ipcm, it);
        } else if (jt != ipcm->pending_ipcp_unregistrations.end()) {
                ipcm_unregister_response_ipcp(event, ipcm, jt);
        } else {
                cerr <<  __func__ << ": Warning: Unregistration response "
                        "received, but no pending DIF assignment " << endl;
        }
}

void
register_application_response_event_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
        (void) event; // Stop compiler barfs
        (void) opaque;    // Stop compiler barfs
}

void
unregister_application_response_event_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
        (void) event; // Stop compiler barfs
        (void) opaque;    // Stop compiler barfs
}

void
application_registration_canceled_event_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
        (void) event; // Stop compiler barfs
        (void) opaque;    // Stop compiler barfs
}

}
