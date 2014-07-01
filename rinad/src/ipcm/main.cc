/*
 * IPC Manager
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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
#include "tclap/CmdLine.h"

using namespace std;
using namespace TCLAP;


/* Macro useful to perform downcasts in declarations. */
#define DOWNCAST_DECL(_var,_class,_name)        \
        _class *_name = dynamic_cast<_class*>(_var);

#define IPCM_LOG_FILE "/tmp/ipcm-log-file"

class IPCManager : public EventLoopData {
 public:
        IPCManager();
        int apply_configuration();

        rina::IPCProcess *create_ipcp(const rinad::IPCProcessToCreate& iptc);
        int assign_to_dif(rina::IPCProcess *ipcp,
                          const rinad::IPCProcessToCreate& iptc);
        int register_at_difs(rina::IPCProcess *ipcp,
                             const list<string>& difs);
        rina::IPCProcess *select_ipcp_by_dif(const
                        rina::ApplicationProcessNamingInformation& dif_name);
        int enroll_to_difs(rina::IPCProcess *ipcp,
                           const list<rinad::NeighborData>& neighbors);

        rinad::RINAConfiguration config;

        map<unsigned short, rina::IPCProcess*> pending_normal_ipcp_inits;
        map<unsigned int, rina::IPCProcess*> pending_ipcp_dif_assignments;
        map<unsigned int, rina::IPCProcess*> pending_ipcp_registrations;
        map<unsigned int, rina::IPCProcess*> pending_ipcp_enrollments;

 private:
        rina::Thread *console;
};

void *console_work(void *arg)
{
        IPCManager *ipcm = static_cast<IPCManager *>(arg);

        cout << "Console starts: " << ipcm << endl;
        cout << "Console stops" << endl;

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
                                   console_work, this);
}

rina::IPCProcess *
IPCManager::create_ipcp(const rinad::IPCProcessToCreate& iptc)
{
        rina::IPCProcess *ipcp = NULL;

        try {
                ipcp = rina::ipcProcessFactory->create(iptc.name,
                                iptc.type);
                if (iptc.type != rina::NORMAL_IPC_PROCESS) {
                        /* Shim IPC processes are set as initialized
                         * immediately. */
                        ipcp->setInitialized();
                } else {
                        /* Normal IPC processes can be set as
                         * initialized only when the corresponding
                         * IPC process daemon is initialized, so we
                         * defer the operation. */
                        pending_normal_ipcp_inits[ipcp->getId()] = ipcp;
                }
        } catch (rina::CreateIPCProcessException) {
                cerr << "Failed to create  IPC process '" <<
                        iptc.name.toString() << "' of type '" <<
                        iptc.type << "'" << endl;
        }

        return ipcp;
}

int
IPCManager::assign_to_dif(rina::IPCProcess *ipcp,
                              const rinad::IPCProcessToCreate& iptc)
{
        if (!ipcp || !iptc.difName.size()) {
                return 0;
        }

        rinad::DIFProperties dif_props;
        rina::DIFInformation dif_info;
        rina::DIFConfiguration dif_config;
        rina::ApplicationProcessNamingInformation dif_name(
                        iptc.difName, string());
        bool found;

        /* Try to extract the DIF properties from the
         * configuration. */
        found = config.lookup_dif_properties(iptc.difName,
                        dif_props);
        if (!found) {
                throw new rina::AssignToDIFException(
                                string("Cannot find properties "
                                        "for DIF ") + iptc.difName);
        }

        /* Fill in the DIFConfiguration object. */
        if (ipcp->getType() == rina::NORMAL_IPC_PROCESS) {
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
                        lookup_ipcp_address(ipcp->getName(),
                                        address);
                if (!found) {
                        ostringstream ss;

                        ss << "No address for IPC process " <<
                                ipcp->getName().toString() <<
                                " in DIF " << iptc.difName <<
                                endl;

                        throw new rina::AssignToDIFException(
                                        ss.str());
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
        dif_info.set_dif_type(ipcp->getType());
        dif_info.set_dif_configuration(dif_config);

        /* Invoke librina to assign the IPC process to the
         * DIF specified by dif_info. */
        try {
                unsigned int seqnum = ipcp->assignToDIF(dif_info);

                pending_ipcp_dif_assignments[seqnum] = ipcp;
        } catch (rina::AssignToDIFException) {
                cerr << "Cannot assign " <<
                        ipcp->getName().toString() <<
                        " to DIF " << iptc.difName << endl;
        }

        return 0;
}

/* Returns an IPC process assigned to the DIF specified by @dif_name,
 * if any.
 */
rina::IPCProcess *IPCManager::select_ipcp_by_dif(const
                        rina::ApplicationProcessNamingInformation& dif_name)
{
        const vector<rina::IPCProcess *>& ipcps =
                rina::ipcProcessFactory->listIPCProcesses();

        for (unsigned int i = 0; i < ipcps.size(); i++) {
                rina::DIFInformation dif_info = ipcps[i]->getDIFInformation();

                if (dif_info.get_dif_name() == dif_name) {
                        return ipcps[i];
                }
        }

        return NULL;
}

int IPCManager::register_at_difs(rina::IPCProcess *ipcp,
                                 const list<string>& difs)
{
        for (list<string>::const_iterator sit = difs.begin();
                        sit != difs.end(); sit++) {
                rina::ApplicationProcessNamingInformation dif_name(
                                                        *sit, string());
                /* Select a slave (N-1) IPC process. */
                rina::IPCProcess *slave_ipcp = select_ipcp_by_dif(dif_name);
                unsigned int seqnum;

                if (!slave_ipcp) {
                        cerr << "Cannot find any IPC process belonging "
                                << "to DIF " << *sit << endl;
                        continue;
                }

                /* Try to register @ipcp to the slave IPC process. */
                try {
                        seqnum = slave_ipcp->registerApplication(
                                        ipcp->getName(), ipcp->getId());
                        pending_ipcp_registrations[seqnum] = ipcp;
                } catch (Exception) {
                        cerr << __func__ << ": Error while requesting "
                                << "registration" << endl;
                }
        }
}

int IPCManager::enroll_to_difs(rina::IPCProcess *ipcp,
                               const list<rinad::NeighborData>& neighbors)
{
        for (list<rinad::NeighborData>::const_iterator
                        nit = neighbors.begin();
                                nit != neighbors.end(); nit++) {
                try {
                        unsigned int seqnum;

                        seqnum = ipcp->enroll(nit->difName,
                                              nit->supportingDifName,
                                              nit->apName);
                        pending_ipcp_enrollments[seqnum] = ipcp;
                } catch (rina::EnrollException) {
                        cerr << __func__ << ": Error while enrolling "
                                << "to DIF " << nit->difName.toString()
                                << endl;
                }
        }
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

                ipcp = create_ipcp(*cit);
                assign_to_dif(ipcp, *cit);
                register_at_difs(ipcp, cit->difsToRegisterAt);

                ipcps.push_back(ipcp);
        }

        for (pit = ipcps.begin(), cit = config.ipcProcessesToCreate.begin();
                                        pit != ipcps.end(); pit++, cit++) {
                enroll_to_difs(*pit, cit->neighbors);
        }

        return 0;
}

static void FlowAllocationRequestedEventHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void AllocateFlowRequestResultEventHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void AllocateFlowResponseEventHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void FlowDeallocationRequestedEventHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void DeallocateFlowResponseEventHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void ApplicationUnregisteredEventHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void FlowDeallocatedEventHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void ApplicationRegistrationRequestEventHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void RegisterApplicationResponseEventHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void ApplicationUnregistrationRequestEventHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void UnregisterApplicationResponseEventHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void ApplicationRegistrationCanceledEventHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void AssignToDifRequestEventHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void AssignToDifResponseEventHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void UpdateDifConfigRequestEventHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void UpdateDifConfigResponseEventHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void EnrollToDifRequestEventHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void EnrollToDifResponseEventHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void NeighborsModifiedNotificaitonEventHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void IpcProcessDifRegistrationNotificationHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void IpcProcessQueryRibHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void GetDifPropertiesHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void GetDifPropertiesResponseEventHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void OsProcessFinalizedHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void IpcmRegisterAppResponseEventHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void IpcmUnregisterAppResponseEventHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void IpcmDeallocateFlowResponseEventHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void IpcmAllocateFlowRequestResultHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void QueryRibResponseEventHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void IpcProcessDaemonInitializedEventHandler(rina::IPCEvent *e,
                                                    EventLoopData *dm)
{
        DOWNCAST_DECL(e, rina::IPCProcessDaemonInitializedEvent, event);
        DOWNCAST_DECL(dm, IPCManager, ipcm);
        map<unsigned short, rina::IPCProcess *>::iterator mit;

        /* Perform deferred "setInitiatialized()" of a normal IPC process, if
         * needed. */
        mit = ipcm->pending_normal_ipcp_inits.find(event->getIPCProcessId());
        if (mit != ipcm->pending_normal_ipcp_inits.end()) {
                mit->second->setInitialized();
                ipcm->pending_normal_ipcp_inits.erase(mit);
        } else {
                cerr <<  __func__ << ": Warning: IPCP daemon initialized, "
                        "but no pending normal IPC process initialization"
                        << endl;
        }
}

static void TimerExpiredEventHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void IpcProcessCreateConnectionResponseHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void IpcProcessUpdateConnectionResponseHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void IpcProcessCreateConnectionResultHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void IpcProcessDestroyConnectionResultHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

static void IpcProcessDumpFtResponseHandler(rina::IPCEvent *event, EventLoopData *dm)
{
}

int main(int argc, char * argv[])
{
        // Wrap everything in a try block.  Do this every time, 
        // because exceptions will be thrown for problems. 
        try {

                // Define the command line object.
                TCLAP::CmdLine cmd("Command description message", ' ', "0.9");

                // Define a value argument and add it to the command line.
                TCLAP::ValueArg<string> nameArg("n",
                                                     "name",
                                                     "Name to print",
                                                     true,
                                                     "homer",
                                                     "string");
                cmd.add(nameArg);

                // Parse the args.
                cmd.parse(argc, argv);

                // Get the value parsed by each arg. 
                string name = nameArg.getValue();

                cout << "My name is: " << name << endl;

        } catch (ArgException &e) {
                cerr << "error: " << e.error() << " for arg "
                          << e.argId() << endl;
        }

        IPCManager *ipcm = new IPCManager();
        EventLoop loop(ipcm);

        loop.register_event(rina::FLOW_ALLOCATION_REQUESTED_EVENT,
                        FlowAllocationRequestedEventHandler);
        loop.register_event(rina::ALLOCATE_FLOW_REQUEST_RESULT_EVENT,
                        AllocateFlowRequestResultEventHandler);
        loop.register_event(rina::ALLOCATE_FLOW_RESPONSE_EVENT,
                        AllocateFlowResponseEventHandler);
        loop.register_event(rina::FLOW_DEALLOCATION_REQUESTED_EVENT,
                        FlowDeallocationRequestedEventHandler);
        loop.register_event(rina::DEALLOCATE_FLOW_RESPONSE_EVENT,
                        DeallocateFlowResponseEventHandler);
        loop.register_event(rina::APPLICATION_UNREGISTERED_EVENT,
                        ApplicationUnregisteredEventHandler);
        loop.register_event(rina::FLOW_DEALLOCATED_EVENT,
                        FlowDeallocatedEventHandler);
        loop.register_event(rina::APPLICATION_REGISTRATION_REQUEST_EVENT,
                        ApplicationRegistrationRequestEventHandler);
        loop.register_event(rina::REGISTER_APPLICATION_RESPONSE_EVENT,
                        RegisterApplicationResponseEventHandler);
        loop.register_event(rina::APPLICATION_UNREGISTRATION_REQUEST_EVENT,
                        ApplicationUnregistrationRequestEventHandler);
        loop.register_event(rina::UNREGISTER_APPLICATION_RESPONSE_EVENT,
                        UnregisterApplicationResponseEventHandler);
        loop.register_event(rina::APPLICATION_REGISTRATION_CANCELED_EVENT,
                        ApplicationRegistrationCanceledEventHandler);
        loop.register_event(rina::ASSIGN_TO_DIF_REQUEST_EVENT,
                        AssignToDifRequestEventHandler);
        loop.register_event(rina::ASSIGN_TO_DIF_RESPONSE_EVENT,
                        AssignToDifResponseEventHandler);
        loop.register_event(rina::UPDATE_DIF_CONFIG_REQUEST_EVENT,
                        UpdateDifConfigRequestEventHandler);
        loop.register_event(rina::UPDATE_DIF_CONFIG_RESPONSE_EVENT,
                        UpdateDifConfigResponseEventHandler);
        loop.register_event(rina::ENROLL_TO_DIF_REQUEST_EVENT,
                        EnrollToDifRequestEventHandler);
        loop.register_event(rina::ENROLL_TO_DIF_RESPONSE_EVENT,
                        EnrollToDifResponseEventHandler);
        loop.register_event(rina::NEIGHBORS_MODIFIED_NOTIFICAITON_EVENT,
                        NeighborsModifiedNotificaitonEventHandler);
        loop.register_event(rina::IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION,
                        IpcProcessDifRegistrationNotificationHandler);
        loop.register_event(rina::IPC_PROCESS_QUERY_RIB,
                        IpcProcessQueryRibHandler);
        loop.register_event(rina::GET_DIF_PROPERTIES,
                        GetDifPropertiesHandler);
        loop.register_event(rina::GET_DIF_PROPERTIES_RESPONSE_EVENT,
                        GetDifPropertiesResponseEventHandler);
        loop.register_event(rina::OS_PROCESS_FINALIZED,
                        OsProcessFinalizedHandler);
        loop.register_event(rina::IPCM_REGISTER_APP_RESPONSE_EVENT,
                        IpcmRegisterAppResponseEventHandler);
        loop.register_event(rina::IPCM_UNREGISTER_APP_RESPONSE_EVENT,
                        IpcmUnregisterAppResponseEventHandler);
        loop.register_event(rina::IPCM_DEALLOCATE_FLOW_RESPONSE_EVENT,
                        IpcmDeallocateFlowResponseEventHandler);
        loop.register_event(rina::IPCM_ALLOCATE_FLOW_REQUEST_RESULT,
                        IpcmAllocateFlowRequestResultHandler);
        loop.register_event(rina::QUERY_RIB_RESPONSE_EVENT,
                        QueryRibResponseEventHandler);
        loop.register_event(rina::IPC_PROCESS_DAEMON_INITIALIZED_EVENT,
                        IpcProcessDaemonInitializedEventHandler);
        loop.register_event(rina::TIMER_EXPIRED_EVENT,
                        TimerExpiredEventHandler);
        loop.register_event(rina::IPC_PROCESS_CREATE_CONNECTION_RESPONSE,
                        IpcProcessCreateConnectionResponseHandler);
        loop.register_event(rina::IPC_PROCESS_UPDATE_CONNECTION_RESPONSE,
                        IpcProcessUpdateConnectionResponseHandler);
        loop.register_event(rina::IPC_PROCESS_CREATE_CONNECTION_RESULT,
                        IpcProcessCreateConnectionResultHandler);
        loop.register_event(rina::IPC_PROCESS_DESTROY_CONNECTION_RESULT,
                        IpcProcessDestroyConnectionResultHandler);
        loop.register_event(rina::IPC_PROCESS_DUMP_FT_RESPONSE,
                        IpcProcessDumpFtResponseHandler);

        loop.run();

        return EXIT_SUCCESS;
}
