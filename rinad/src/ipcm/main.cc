/*
 * IPC Manager
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
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
#include <fstream>
#include <map>
#include <vector>

#define RINA_PREFIX "ipcm"

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/logs.h>

#include "event-loop.h"
#include "rina-configuration.h"
#include "tclap/CmdLine.h"
#include "json/json.h"

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

        rinad::RINAConfiguration config;

        map<unsigned short, rina::IPCProcess*> pending_normal_ipcp_inits;
        map<long, rina::IPCProcess*> pending_ipcp_dif_assignments;

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

int
IPCManager::apply_configuration()
{
        /* Examine all the IPCProcesses that are going to be created
         * according to the configuration file.
         */
        for (list<rinad::IPCProcessToCreate>::iterator
                it = config.ipcProcessesToCreate.begin();
                        it != config.ipcProcessesToCreate.end(); it++) {
                rina::IPCProcess *ipcp;

                try {
                        ipcp = rina::ipcProcessFactory->create(it->name,
                                                               it->type);
                        if (it->type != rina::NORMAL_IPC_PROCESS) {
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
                                it->name.toString() << "' of type '" <<
                                it->type << "'" << endl;
                }

                if (it->difName.size()) {
                        rinad::DIFProperties dif_props;
                        rina::DIFInformation dif_info;
                        rina::DIFConfiguration dif_config;
                        rina::ApplicationProcessNamingInformation dif_name(
                                                        it->difName, string());
                        bool found;

                        /* Try to extract the DIF properties from the
                         * configuration. */
                        found = config.lookup_DIF_properties(it->difName,
                                                             dif_props);
                        if (!found) {
                                throw new rina::AssignToDIFException(
                                        string("Cannot find properties "
                                                "for DIF ") + it->difName);
                        }

                        /* Fill in the DIFConfiguration object. */
                        if (ipcp->getType() == rina::NORMAL_IPC_PROCESS) {
                                rina::EFCPConfiguration efcp_config;
                                long address;

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
                                                " in DIF " << it->difName <<
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
                                long seqnum = ipcp->assignToDIF(dif_info);

                                pending_ipcp_dif_assignments[seqnum] = ipcp;
                        } catch (rina::AssignToDIFException) {
                                cerr << "Cannot assign " <<
                                        ipcp->getName().toString() <<
                                        " to DIF " << it->difName << endl;
                        }
                }
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


bool ParseConfiguration(std::string file_loc, IPCManager *ipcm)
{
        // FIXME: General note: Params should be checked before they are used
        // Some can be NULL 

        // Parse config file with jsoncpp
        Json::Value root;
        Json::Reader reader;
        std::ifstream file;

        file.open(file_loc.c_str());
        if(file.fail()) {
                LOG_ERR("Failed to open config file");
                return false;
        }

        if (!reader.parse(file, root, false)) {
                LOG_ERR("Failed to parse configuration");
                
                // FIXME: Log messages need to take std::string for this to work
                std::cout << "Failed to parse JSON" << std::endl
                          << reader.getFormatedErrorMessages() << std::endl;

                return false;
        }
        file.close();

        rinad::RINAConfiguration config;
        rinad::LocalConfiguration local;

        Json::Value local_conf = root["localConfiguration"];
        if (local_conf != 0) {
                local.consolePort 
                        = local_conf.get("consolePort", 
                                         local.consolePort).asInt();
                local.cdapTimeoutInMs 
                        = local_conf.get("cdapTimeoutInMs", 
                                         local.cdapTimeoutInMs).asInt();
                local.enrollmentTimeoutInMs 
                        = local_conf.get("enrollmentTimeoutInMs",
                                         local.enrollmentTimeoutInMs).asInt();
                local.maxEnrollmentRetries
                        = local_conf.get("maxEnrollmentRetries",
                                         local.maxEnrollmentRetries).asInt();
                local.flowAllocatorTimeoutInMs 
                        = local_conf.get("flowAllocatorTimeoutInMs",
                                         local.flowAllocatorTimeoutInMs).asInt();
                local.watchdogPeriodInMs
                        = local_conf.get("watchdogPeriodInMs",
                                         local.watchdogPeriodInMs).asInt();
                local.declaredDeadIntervalInMs 
                        = local_conf.get("declaredDeadIntervalInMs", 
                                         local.declaredDeadIntervalInMs).asInt();
                local.neighborsEnrollerPeriodInMs
                        = local_conf.get("neighborsEnrollerPeriodInMs",
                                         local.neighborsEnrollerPeriodInMs).asInt();
                local.lengthOfFlowQueues
                        = local_conf.get("lengthOfFlowQueues",
                                         local.lengthOfFlowQueues).asInt();
                local.installationPath
                        = local_conf.get("installationPath",
                                         local.installationPath).asString();
                local.libraryPath
                        = local_conf.get("libraryPath",
                                         local.libraryPath).asString();
        }
        config.local = local;

        // App to DIF Mappings
        std::list<rinad::ApplicationToDIFMapping> applicationToDIFMappings;
        Json::Value appToDIF = root["applicationToDIFMappings"];
        if (appToDIF != 0) {
                for (unsigned int i = 0; i < appToDIF.size(); i++) {
                        rinad::ApplicationToDIFMapping appToDIFMapping;
                        appToDIFMapping.difName
                                = appToDIF[i].get("difName",
                                                  appToDIFMapping.difName).asString();
                        appToDIFMapping.encodedAppName
                                = appToDIF[i].get("encodedAppName",
                                                  appToDIFMapping.encodedAppName).asString();
                
                        applicationToDIFMappings.push_back(appToDIFMapping);
                }
        }
        config.applicationToDIFMappings = applicationToDIFMappings;

        // IPC processes to create 
        Json::Value ipc_processes = root["ipcProcessesToCreate"];
        for (unsigned int i = 0; i < ipc_processes.size(); i++) {

                rinad::IPCProcessToCreate ipc;

                ipc.type = ipc_processes[i].get("type",
                                                ipc.type).asString();
                
                // IPC process Names
                // Might want to move this to another function
                rina::ApplicationProcessNamingInformation name;
                std::string applicationProcessName =
                        ipc_processes[i].get("applicationProcessName",
                                             name.getProcessName()).asString();
                std::string applicationProcessInstance =
                        ipc_processes[i].get("applicationProcessInstance",
                                             name.getProcessInstance()).asString();
                std::string applicationEntityName =
                        ipc_processes[i].get("applicationEntityName",
                                             name.getEntityName()).asString();
                std::string applicationEntityInstance =
                        ipc_processes[i].get("applicationEntityInstance",
                                             name.getEntityInstance()).asString();

                name.setProcessName(applicationProcessName);
                name.setProcessInstance(applicationProcessInstance);
                name.setEntityName(applicationEntityName);
                name.setEntityInstance(applicationEntityInstance);    
                ipc.name = name;
              
                ipc.difName = ipc_processes[i].get("difName",
                                                   ipc.difName).asString();

                //Neighbors
                Json::Value neigh = ipc_processes[i]["neighbors"];
                if (neigh != 0) {
                        for (unsigned int j = 0; j < neigh.size(); j++) {
                                rinad::NeighborData neigh_data;

                                rina::ApplicationProcessNamingInformation name;
                                std::string applicationProcessName =
                                        neigh[j].get("applicationProcessName",
                                                     name.getProcessName()).asString();
                                std::string applicationProcessInstance =
                                        neigh[j].get("applicationProcessInstance",
                                                     name.getProcessInstance()).asString();
                                std::string applicationEntityName =
                                        neigh[j].get("applicationEntityName",
                                                     name.getEntityName()).asString();
                                std::string applicationEntityInstance =
                                        neigh[j].get("applicationEntityInstance",
                                                     name.getEntityInstance()).asString();

                                name.setProcessName(applicationProcessName);
                                name.setProcessInstance(applicationProcessInstance);
                                name.setEntityName(applicationEntityName);
                                name.setEntityInstance(applicationEntityInstance);    
                                neigh_data.apNameInfo = name;

                                neigh_data.difName = 
                                        neigh[j].get("difName",
                                                     neigh_data.difName).asString();

                                neigh_data.supportingDifName = 
                                        neigh[j].get("supportingDifName",
                                                     neigh_data.supportingDifName).asString();
                              
                                ipc.neighbors.push_back(neigh_data);

                        }
                }

                // DIFs to register at
                Json::Value difs_reg = ipc_processes[i]["difsToRegisterAt"];
                if (difs_reg != 0) {
                        for (unsigned int j = 0; j < difs_reg.size(); j++) {
                                std::string difName = difs_reg[j].asString();
                                ipc.difsToRegisterAt.push_back(difName);
                        }
                }

                ipc.hostname = ipc_processes[i].get("hostName",
                                                   ipc.hostname).asString();

                // SDU protection options
                Json::Value sdu_prot = ipc_processes[i]["sduProtectionOptions"];
                if (sdu_prot != 0) {
                        for (unsigned int j = 0; j < sdu_prot.size(); j++) {
                                rinad::SDUProtectionOption option;
                                option.nMinus1DIFName =
                                        sdu_prot[j].get("nMinus1DIFName",
                                                        option.nMinus1DIFName).asString();
                                option.nMinus1DIFName =
                                        sdu_prot[j].get("sduProtectionType",
                                                        option.sduProtectionType).asString();
                                ipc.sduProtectionOptions.push_back(option);
                        }
                }

                // parameters 
                Json::Value params = ipc_processes[i]["parameters"];
                if (params != 0) {
                        //FIXME: Parse the extra params here


                }


                config.ipcProcessesToCreate.push_back(ipc);
        }
      

        // FIXME: Set the DIF configurations here
        Json::Value dif_configs = root["difConfigurations"];
        for (unsigned int i = 0; i < dif_configs.size(); i++) {


        }


        ipcm->config = config;
        return true;
}


int main(int argc, char * argv[])
{
        std::string conf;

        // Wrap everything in a try block.  Do this every time, 
        // because exceptions will be thrown for problems. 
        try {

                // Define the command line object.
                TCLAP::CmdLine cmd("IPC Manager", ' ', "0.1");

                TCLAP::ValueArg<std::string> conf_arg("c",
                                                      "config",
                                                      "Configuration file to load",
                                                      true,
                                                      "ipcmanager.conf",
                                                      "string");

                cmd.add(conf_arg);
                
                // Parse the args.
                cmd.parse(argc, argv);

                // Get the value parsed by each arg. 
                conf = conf_arg.getValue();

                LOG_DBG("Config file is: %s", conf.c_str());

        } catch (ArgException &e) {
                LOG_ERR("Error: %s for arg %d", e.error().c_str(), e.argId().c_str());
                return EXIT_FAILURE;
        }

        IPCManager *ipcm = new IPCManager();

        if (!ParseConfiguration(conf, ipcm)) {
                LOG_ERR("Failed to load configuration");
                return EXIT_FAILURE;
        }

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
