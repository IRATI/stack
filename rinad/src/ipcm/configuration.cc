/*
 * Configuration reader for IPC Manager
 *
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
#include <string>

#define RINA_PREFIX "ipcm"

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/logs.h>

#include "rina-configuration.h"
#include "json/json.h"
#include "ipcm.h"

using namespace std;


namespace rinad {

bool parse_configuration(string file_loc, IPCManager *ipcm)
{
        // FIXME: General note: Params should be checked before they are used
        // Some can be NULL

        // Parse config file with jsoncpp
        Json::Value root;
        Json::Reader reader;
        ifstream file;

        file.open(file_loc.c_str());
        if(file.fail()) {
                LOG_ERR("Failed to open config file");
                return false;
        }

        if (!reader.parse(file, root, false)) {
                LOG_ERR("Failed to parse configuration");

                // FIXME: Log messages need to take string for this to work
                cout << "Failed to parse JSON" << endl
                          << reader.getFormatedErrorMessages() << endl;

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
        list<rinad::ApplicationToDIFMapping> applicationToDIFMappings;
        Json::Value appToDIF = root["applicationToDIFMappings"];
        if (appToDIF != 0) {
                for (unsigned int i = 0; i < appToDIF.size(); i++) {
                        rinad::ApplicationToDIFMapping appToDIFMapping;

                        appToDIFMapping.difName =
                                rina::ApplicationProcessNamingInformation(
                                appToDIF[i].get("difName", string()).asString(),
                                string());
                        appToDIFMapping.encodedAppName =
                                appToDIF[i].get("encodedAppName",
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
                string applicationProcessName =
                        ipc_processes[i].get("applicationProcessName",
                                             name.getProcessName()).asString();
                string applicationProcessInstance =
                        ipc_processes[i].get("applicationProcessInstance",
                                             name.getProcessInstance()).asString();
                string applicationEntityName =
                        ipc_processes[i].get("applicationEntityName",
                                             name.getEntityName()).asString();
                string applicationEntityInstance =
                        ipc_processes[i].get("applicationEntityInstance",
                                             name.getEntityInstance()).asString();

                name.setProcessName(applicationProcessName);
                name.setProcessInstance(applicationProcessInstance);
                name.setEntityName(applicationEntityName);
                name.setEntityInstance(applicationEntityInstance);
                ipc.name = name;

                ipc.difName = rina::ApplicationProcessNamingInformation(
                        ipc_processes[i].get("difName", string()).asString(),
                        string());

                //Neighbors
                Json::Value neigh = ipc_processes[i]["neighbors"];
                if (neigh != 0) {
                        for (unsigned int j = 0; j < neigh.size(); j++) {
                                rinad::NeighborData neigh_data;

                                rina::ApplicationProcessNamingInformation name;
                                string applicationProcessName =
                                        neigh[j].get("applicationProcessName",
                                                     name.getProcessName()).asString();
                                string applicationProcessInstance =
                                        neigh[j].get("applicationProcessInstance",
                                                     name.getProcessInstance()).asString();
                                string applicationEntityName =
                                        neigh[j].get("applicationEntityName",
                                                     name.getEntityName()).asString();
                                string applicationEntityInstance =
                                        neigh[j].get("applicationEntityInstance",
                                                     name.getEntityInstance()).asString();

                                name.setProcessName(applicationProcessName);
                                name.setProcessInstance(applicationProcessInstance);
                                name.setEntityName(applicationEntityName);
                                name.setEntityInstance(applicationEntityInstance);
                                neigh_data.apName = name;

                                neigh_data.difName =
                                        rina::ApplicationProcessNamingInformation(
                                        neigh[j].get("difName", string()).asString(),
                                        string());

                                neigh_data.supportingDifName =
                                        rina::ApplicationProcessNamingInformation(
                                        neigh[j].get("supportingDifName",
                                        string()).asString(),
                                        string());

                                ipc.neighbors.push_back(neigh_data);

                        }
                }

                // DIFs to register at
                Json::Value difs_reg = ipc_processes[i]["difsToRegisterAt"];
                if (difs_reg != 0) {
                        for (unsigned int j = 0; j < difs_reg.size(); j++) {
                                ipc.difsToRegisterAt.push_back(
                                        rina::ApplicationProcessNamingInformation(
                                                difs_reg[j].asString(),
                                                string()
                                                )
                                        );
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

}
