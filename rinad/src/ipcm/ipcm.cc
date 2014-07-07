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

/* Macro useful to perform downcasts in declarations. */
#define DOWNCAST_DECL(_var,_class,_name)        \
        _class *_name = dynamic_cast<_class*>(_var);

#define IPCM_LOG_FILE "/tmp/ipcm-log-file"

void *console_function(void *opaque)
{
        IPCManager *ipcm = static_cast<IPCManager *>(opaque);

        cout << "Console starts: " << ipcm << endl;
        cout << "Console stops" << endl;

        return NULL;
}

void *script_function(void *opaque)
{
        IPCManager *ipcm = static_cast<IPCManager *>(opaque);

        cout << "Script starts: " << ipcm << endl;

        ipcm->apply_configuration();

        cout << "Script stops" << endl;

        return NULL;
}

IPCManager::IPCManager()
{
        /* Initialize the IPC manager infrastructure in librina. */
        try {
                rina::initializeIPCManager(1, config.local.installationPath,
                                           config.local.libraryPath,
                                           LOG_LEVEL_INFO, IPCM_LOG_FILE);
        } catch (rina::InitializationException) {
                cerr << "Cannot initialize librina-ipc-manager" << endl;
                exit(EXIT_FAILURE);
        }

        /* Create and start the console thread. */
        console = new rina::Thread(new rina::ThreadAttributes(),
                                   console_function, this);

        script = new rina::Thread(new rina::ThreadAttributes(),
                                   script_function, this);
}

IPCManager::~IPCManager()
{
        delete console;
        delete script;
}

void
IPCMConcurrency::wait_for_event(rina::IPCEventType ty, unsigned int seqnum)
{
        event_waiting = true;
        event_ty = ty;
        event_sn = seqnum;
        doWait();
}

void IPCMConcurrency::notify_event(rina::IPCEvent *event)
{
        if (event_waiting && event_ty == event->eventType
                        && (event_sn == 0 ||
                            event_sn == event->sequenceNumber)) {
                signal();
                event_waiting = false;
        }
}

static void
ipcm_pre_function(rina::IPCEvent *event, EventLoopData *opaque)
{
        DOWNCAST_DECL(opaque, IPCManager, ipcm);

        ipcm->concurrency.lock();
}

static void
ipcm_post_function(rina::IPCEvent *event, EventLoopData *opaque)
{
        DOWNCAST_DECL(opaque, IPCManager, ipcm);

        ipcm->concurrency.notify_event(event);
        ipcm->concurrency.unlock();
}

rina::IPCProcess *
IPCManager::create_ipcp(const rina::ApplicationProcessNamingInformation& name,
                        const string& type)
{
        rina::IPCProcess *ipcp = NULL;
        bool wait = false;

        concurrency.lock();

        try {
                ipcp = rina::ipcProcessFactory->create(name,
                                type);
                if (type != rina::NORMAL_IPC_PROCESS) {
                        /* Shim IPC processes are set as initialized
                         * immediately. */
                        ipcp->setInitialized();
                } else {
                        /* Normal IPC processes can be set as
                         * initialized only when the corresponding
                         * IPC process daemon is initialized, so we
                         * defer the operation. */
                        pending_normal_ipcp_inits[ipcp->id] = ipcp;
                        wait = true;
                }
        } catch (rina::CreateIPCProcessException) {
                cerr << "Failed to create  IPC process '" <<
                        name.toString() << "' of type '" <<
                        type << "'" << endl;
        }

        if (wait) {
                concurrency.wait_for_event(
                        rina::IPC_PROCESS_DAEMON_INITIALIZED_EVENT, 0);
        }

        concurrency.unlock();

        return ipcp;
}

int
IPCManager::assign_to_dif(rina::IPCProcess *ipcp,
                          const rina::ApplicationProcessNamingInformation&
                          dif_name)
{
        if (!ipcp) {
                return 0;
        }

        rinad::DIFProperties dif_props;
        rina::DIFInformation dif_info;
        rina::DIFConfiguration dif_config;
        bool found;
        int ret = -1;

        concurrency.lock();

        /* Try to extract the DIF properties from the
         * configuration. */
        found = config.lookup_dif_properties(dif_name,
                        dif_props);
        if (!found) {
                cerr << "Cannot find properties for DIF "
                        << dif_name.toString();
                goto out;
        }

        /* Fill in the DIFConfiguration object. */
        if (ipcp->type == rina::NORMAL_IPC_PROCESS) {
                rina::EFCPConfiguration efcp_config;
                unsigned int address;

                /* FIll in the EFCPConfiguration object. */
                efcp_config.set_data_transfer_constants(
                                dif_props.dataTransferConstants);
                for (list<rina::QoSCube>::iterator
                                qit = dif_props.qosCubes.begin();
                                qit != dif_props.qosCubes.end();
                                qit++) {
                        efcp_config.add_qos_cube(*qit);
                }

                found = dif_props.
                        lookup_ipcp_address(ipcp->name,
                                        address);
                if (!found) {
                        cerr << "No address for IPC process " <<
                                ipcp->name.toString() <<
                                " in DIF " << dif_name.toString() <<
                                endl;
                        goto out;
                }
                dif_config.set_efcp_configuration(efcp_config);
                dif_config.set_address(address);
        }

        for (list<rina::Parameter>::const_iterator
                        pit = dif_props.configParameters.begin();
                        pit != dif_props.configParameters.end();
                        pit++) {
                dif_config.add_parameter(*pit);
        }

        /* Fill in the DIFInformation object. */
        dif_info.set_dif_name(dif_name);
        dif_info.set_dif_type(ipcp->type);
        dif_info.set_dif_configuration(dif_config);

        /* Invoke librina to assign the IPC process to the
         * DIF specified by dif_info. */
        try {
                unsigned int seqnum = ipcp->assignToDIF(dif_info);

                pending_ipcp_dif_assignments[seqnum] = ipcp;
                concurrency.wait_for_event(rina::ASSIGN_TO_DIF_RESPONSE_EVENT,
                                           seqnum);
        } catch (rina::AssignToDIFException) {
                cerr << "Error while assigning " <<
                        ipcp->name.toString() <<
                        " to DIF " << dif_name.toString() << endl;
        }

        ret = 0;
out:
        concurrency.unlock();

        return ret;
}

int
IPCManager::register_at_dif(rina::IPCProcess *ipcp,
                            const rina::ApplicationProcessNamingInformation&
                            dif_name)
{
        /* Select a slave (N-1) IPC process. */
        rina::IPCProcess *slave_ipcp = select_ipcp_by_dif(dif_name);
        unsigned int seqnum;

        if (!slave_ipcp) {
                cerr << "Cannot find any IPC process belonging "
                        << "to DIF " << dif_name.toString() << endl;
                return -1;
        }

        concurrency.lock();

        /* Try to register @ipcp to the slave IPC process. */
        try {
                seqnum = slave_ipcp->registerApplication(
                                ipcp->name, ipcp->id);

                pending_ipcp_registrations[seqnum] =
                        PendingIPCPRegistration(ipcp, slave_ipcp);

                concurrency.wait_for_event(
                        rina::IPCM_REGISTER_APP_RESPONSE_EVENT, seqnum);
        } catch (Exception) {
                cerr << __func__ << ": Error while requesting "
                        << "registration" << endl;
        }

        concurrency.unlock();

        return 0;
}

int IPCManager::register_at_difs(rina::IPCProcess *ipcp,
                const list<rina::ApplicationProcessNamingInformation>& difs)
{
        for (list<rina::ApplicationProcessNamingInformation>::const_iterator
                        nit = difs.begin(); nit != difs.end(); nit++) {
                register_at_dif(ipcp, *nit);
        }

        return 0;
}

int
IPCManager::enroll_to_dif(rina::IPCProcess *ipcp,
                          const rinad::NeighborData& neighbor)
{
        concurrency.lock();

        try {
                unsigned int seqnum;

                seqnum = ipcp->enroll(neighbor.difName,
                                neighbor.supportingDifName,
                                neighbor.apName);
                pending_ipcp_enrollments[seqnum] = ipcp;
        } catch (rina::EnrollException) {
                cerr << __func__ << ": Error while enrolling "
                        << "to DIF " << neighbor.difName.toString()
                        << endl;
        }

        concurrency.unlock();

        return 0;
}

int IPCManager::enroll_to_difs(rina::IPCProcess *ipcp,
                               const list<rinad::NeighborData>& neighbors)
{
        for (list<rinad::NeighborData>::const_iterator
                        nit = neighbors.begin();
                                nit != neighbors.end(); nit++) {
                enroll_to_dif(ipcp, *nit);
        }

        return 0;
}

int
IPCManager::apply_configuration()
{
        list<rina::IPCProcess *> ipcps;
        list<rinad::IPCProcessToCreate>::iterator cit;
        list<rina::IPCProcess *>::iterator pit;

        /* Examine all the IPCProcesses that are going to be created
         * according to the configuration file.
         */
        for (cit = config.ipcProcessesToCreate.begin();
                        cit != config.ipcProcessesToCreate.end(); cit++) {
                rina::IPCProcess *ipcp;

                ipcp = create_ipcp(cit->name, cit->type);
                assign_to_dif(ipcp, cit->difName);
                register_at_difs(ipcp, cit->difsToRegisterAt);

                ipcps.push_back(ipcp);
        }

        /* Perform all the enrollments specified by the configuration file. */
        for (pit = ipcps.begin(), cit = config.ipcProcessesToCreate.begin();
                                        pit != ipcps.end(); pit++, cit++) {
                enroll_to_difs(*pit, cit->neighbors);
        }

        return 0;
}

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
                                PendingAppUnregistration(slave_ipcp, req_event);
        } catch (rina::IpcmUnregisterApplicationException) {
                cerr << __func__ << ": Error while unregistering application "
                        << req_event.applicationName.toString() << " from IPC "
                        "process " << slave_ipcp->name.toString() << endl;
                return -1;
        }

        return 0;
}

static void flow_allocation_requested_event_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

static void allocate_flow_request_result_event_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

static void allocate_flow_response_event_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

static void flow_deallocation_requested_event_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

static void deallocate_flow_response_event_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

static void application_unregistered_event_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

static void flow_deallocated_event_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

static void application_registration_request_event_handler(rina::IPCEvent *e,
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
                        PendingAppRegistration(slave_ipcp, *event);
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
        } catch (rina::IpcmRegisterApplicationException) {
                cerr <<  __func__ << ": Error while reporting DIF "
                        "assignment result of IPC process "
                        << app_name.toString() <<
                        " to N-1 DIF " << slave_dif_name.toString()
                        << endl;
        }

        return success;
}

static void ipcm_register_response_ipcp(
        rina::IpcmRegisterApplicationResponseEvent *event,
        IPCManager *ipcm,
        map<unsigned int, PendingIPCPRegistration>::iterator mit)
{
        rina::IPCProcess *ipcp = mit->second.ipcp;
        rina::IPCProcess *slave_ipcp = mit->second.slave_ipcp;
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
        map<unsigned int, PendingAppRegistration>::iterator mit)
{
        rina::IPCProcess *slave_ipcp = mit->second.slave_ipcp;
        const rina::ApplicationRegistrationRequestEvent& req_event =
                        mit->second.req_event;
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
        } catch (rina::NotifyApplicationRegisteredException) {
                cerr << __func__ << "Error while notifying application "
                        << app_name.toString() << " about registration "
                        "to DIF " << slave_dif_name.toString() << endl;
        }
}

static void ipcm_register_app_response_event_handler(rina::IPCEvent *e,
                                                EventLoopData *opaque)
{

        DOWNCAST_DECL(e, rina::IpcmRegisterApplicationResponseEvent, event);
        DOWNCAST_DECL(opaque, IPCManager, ipcm);
        map<unsigned int, PendingIPCPRegistration>::iterator it;
        map<unsigned int, PendingAppRegistration>::iterator jt;

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

static void application_manager_app_unregistered(
                rina::ApplicationUnregistrationRequestEvent event,
                int result)
{
        // Inform the application about the unregistration result
        try {
                if (event.sequenceNumber > 0) {
                        rina::applicationManager->
                                        applicationUnregistered(event, result);
                }
        } catch (rina::NotifyApplicationUnregisteredException) {
                cerr << __func__ << ": Error while notifying application "
                        << event.applicationName.toString() << " about "
                        "failed unregistration" << endl;
        }
}

static void application_unregistration_request_event_handler(
                                                rina::IPCEvent *e,
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
        } catch (rina::IpcmRegisterApplicationException) {
                cerr << __func__ << ": Error while reporing "
                        "unregistration result for application "
                        << app_name.toString() << endl;
        }

        return success;
}

static void ipcm_unregister_response_ipcp(
                        rina::IpcmUnregisterApplicationResponseEvent *event,
                        IPCManager *ipcm, map<unsigned int,
                        PendingIPCPUnregistration>::iterator mit)
{
        rina::IPCProcess *slave_ipcp = mit->second.slave_ipcp;
        rina::IPCProcess *ipcp = mit->second.ipcp;
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
                        IPCManager *ipcm, map<unsigned int,
                        PendingAppUnregistration>::iterator mit)
{
        // Inform the supporting IPC process
        ipcm_unregister_response_common(event, mit->second.slave_ipcp,
                                        mit->second.req_event.applicationName);

        // Inform the application
        application_manager_app_unregistered(mit->second.req_event,
                        event->result);

        ipcm->pending_app_unregistrations.erase(mit);
}

static void ipcm_unregister_app_response_event_handler(rina::IPCEvent *e,
                                                       EventLoopData *opaque)
{
        DOWNCAST_DECL(e, rina::IpcmUnregisterApplicationResponseEvent, event);
        DOWNCAST_DECL(opaque, IPCManager, ipcm);
        map<unsigned int, PendingAppUnregistration>::iterator it;
        map<unsigned int, PendingIPCPUnregistration>::iterator jt;

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

static void register_application_response_event_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

static void unregister_application_response_event_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

static void application_registration_canceled_event_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

static void assign_to_dif_request_event_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

static void assign_to_dif_response_event_handler(rina::IPCEvent *e,
                                            EventLoopData *opaque)
{
        DOWNCAST_DECL(e, rina::AssignToDIFResponseEvent, event);
        DOWNCAST_DECL(opaque, IPCManager, ipcm);
        map<unsigned int, rina::IPCProcess*>::iterator mit;
        bool success = (event->result == 0);

        mit = ipcm->pending_ipcp_dif_assignments.find(
                                        event->sequenceNumber);
        if (mit != ipcm->pending_ipcp_dif_assignments.end()) {
                rina::IPCProcess *ipcp = mit->second;

                try {
                        ipcp->assignToDIFResult(success);
                } catch (rina::AssignToDIFException) {
                        cerr <<  __func__ << ": Error while reporting DIF "
                                "assignment result for IPC process "
                                << ipcp->name.toString() << endl;
                }
                ipcm->pending_ipcp_dif_assignments.erase(mit);
        } else {
                cerr <<  __func__ << ": Warning: DIF assignment response "
                        "received, but no pending DIF assignment " << endl;
        }
}

static void update_dif_config_request_event_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

static void update_dif_config_response_event_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

static void enroll_to_dif_request_event_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

static void enroll_to_dif_response_event_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

static void neighbors_modified_notificaiton_event_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

static void ipc_process_dif_registration_notification_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

static void ipc_process_query_rib_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

static void get_dif_properties_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

static void get_dif_properties_response_event_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

static void os_process_finalized_handler(rina::IPCEvent *e,
                                         EventLoopData *opaque)
{
        DOWNCAST_DECL(e, rina::OSProcessFinalizedEvent, event);
        DOWNCAST_DECL(opaque, IPCManager, ipcm);
        const vector<rina::IPCProcess *>& ipcps =
                rina::ipcProcessFactory->listIPCProcesses();
        const rina::ApplicationProcessNamingInformation& app_name =
                                                event->applicationName;

        // TODO clean pending flows

        // Look if the terminating application has pending registrations
        // with some IPC process
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

                        ipcm->unregister_app_from_ipcp(req_event, ipcps[i]);
                }
        }
}

static void ipcm_deallocate_flow_response_event_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

static void ipcm_allocate_flow_request_result_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

static void query_rib_response_event_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

static void ipc_process_daemon_initialized_event_handler(rina::IPCEvent *e,
                                                    EventLoopData *opaque)
{
        DOWNCAST_DECL(e, rina::IPCProcessDaemonInitializedEvent, event);
        DOWNCAST_DECL(opaque, IPCManager, ipcm);
        map<unsigned short, rina::IPCProcess *>::iterator mit;

        /* Perform deferred "setInitiatialized()" of a normal IPC process, if
         * needed. */
        mit = ipcm->pending_normal_ipcp_inits.find(event->ipcProcessId);
        if (mit != ipcm->pending_normal_ipcp_inits.end()) {
                mit->second->setInitialized();
                ipcm->pending_normal_ipcp_inits.erase(mit);
        } else {
                cerr <<  __func__ << ": Warning: IPCP daemon initialized, "
                        "but no pending normal IPC process initialization"
                        << endl;
        }
}

static void timer_expired_event_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

static void ipc_process_create_connection_response_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

static void ipc_process_update_connection_response_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

static void ipc_process_create_connection_result_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

static void ipc_process_destroy_connection_result_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

static void ipc_process_dump_ft_response_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
}

void register_handlers_all(EventLoop& loop)
{
        loop.register_pre_function(ipcm_pre_function);
        loop.register_post_function(ipcm_post_function);

        loop.register_event(rina::FLOW_ALLOCATION_REQUESTED_EVENT,
                        flow_allocation_requested_event_handler);
        loop.register_event(rina::ALLOCATE_FLOW_REQUEST_RESULT_EVENT,
                        allocate_flow_request_result_event_handler);
        loop.register_event(rina::ALLOCATE_FLOW_RESPONSE_EVENT,
                        allocate_flow_response_event_handler);
        loop.register_event(rina::FLOW_DEALLOCATION_REQUESTED_EVENT,
                        flow_deallocation_requested_event_handler);
        loop.register_event(rina::DEALLOCATE_FLOW_RESPONSE_EVENT,
                        deallocate_flow_response_event_handler);
        loop.register_event(rina::APPLICATION_UNREGISTERED_EVENT,
                        application_unregistered_event_handler);
        loop.register_event(rina::FLOW_DEALLOCATED_EVENT,
                        flow_deallocated_event_handler);
        loop.register_event(rina::APPLICATION_REGISTRATION_REQUEST_EVENT,
                        application_registration_request_event_handler);
        loop.register_event(rina::REGISTER_APPLICATION_RESPONSE_EVENT,
                        register_application_response_event_handler);
        loop.register_event(rina::APPLICATION_UNREGISTRATION_REQUEST_EVENT,
                        application_unregistration_request_event_handler);
        loop.register_event(rina::UNREGISTER_APPLICATION_RESPONSE_EVENT,
                        unregister_application_response_event_handler);
        loop.register_event(rina::APPLICATION_REGISTRATION_CANCELED_EVENT,
                        application_registration_canceled_event_handler);
        loop.register_event(rina::ASSIGN_TO_DIF_REQUEST_EVENT,
                        assign_to_dif_request_event_handler);
        loop.register_event(rina::ASSIGN_TO_DIF_RESPONSE_EVENT,
                        assign_to_dif_response_event_handler);
        loop.register_event(rina::UPDATE_DIF_CONFIG_REQUEST_EVENT,
                        update_dif_config_request_event_handler);
        loop.register_event(rina::UPDATE_DIF_CONFIG_RESPONSE_EVENT,
                        update_dif_config_response_event_handler);
        loop.register_event(rina::ENROLL_TO_DIF_REQUEST_EVENT,
                        enroll_to_dif_request_event_handler);
        loop.register_event(rina::ENROLL_TO_DIF_RESPONSE_EVENT,
                        enroll_to_dif_response_event_handler);
        loop.register_event(rina::NEIGHBORS_MODIFIED_NOTIFICAITON_EVENT,
                        neighbors_modified_notificaiton_event_handler);
        loop.register_event(rina::IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION,
                        ipc_process_dif_registration_notification_handler);
        loop.register_event(rina::IPC_PROCESS_QUERY_RIB,
                        ipc_process_query_rib_handler);
        loop.register_event(rina::GET_DIF_PROPERTIES,
                        get_dif_properties_handler);
        loop.register_event(rina::GET_DIF_PROPERTIES_RESPONSE_EVENT,
                        get_dif_properties_response_event_handler);
        loop.register_event(rina::OS_PROCESS_FINALIZED,
                        os_process_finalized_handler);
        loop.register_event(rina::IPCM_REGISTER_APP_RESPONSE_EVENT,
                        ipcm_register_app_response_event_handler);
        loop.register_event(rina::IPCM_UNREGISTER_APP_RESPONSE_EVENT,
                        ipcm_unregister_app_response_event_handler);
        loop.register_event(rina::IPCM_DEALLOCATE_FLOW_RESPONSE_EVENT,
                        ipcm_deallocate_flow_response_event_handler);
        loop.register_event(rina::IPCM_ALLOCATE_FLOW_REQUEST_RESULT,
                        ipcm_allocate_flow_request_result_handler);
        loop.register_event(rina::QUERY_RIB_RESPONSE_EVENT,
                        query_rib_response_event_handler);
        loop.register_event(rina::IPC_PROCESS_DAEMON_INITIALIZED_EVENT,
                        ipc_process_daemon_initialized_event_handler);
        loop.register_event(rina::TIMER_EXPIRED_EVENT,
                        timer_expired_event_handler);
        loop.register_event(rina::IPC_PROCESS_CREATE_CONNECTION_RESPONSE,
                        ipc_process_create_connection_response_handler);
        loop.register_event(rina::IPC_PROCESS_UPDATE_CONNECTION_RESPONSE,
                        ipc_process_update_connection_response_handler);
        loop.register_event(rina::IPC_PROCESS_CREATE_CONNECTION_RESULT,
                        ipc_process_create_connection_result_handler);
        loop.register_event(rina::IPC_PROCESS_DESTROY_CONNECTION_RESULT,
                        ipc_process_destroy_connection_result_handler);
        loop.register_event(rina::IPC_PROCESS_DUMP_FT_RESPONSE,
                        ipc_process_dump_ft_response_handler);
}

}
