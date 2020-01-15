/*
 * Configuration reader for IPC Manager
 *
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <string>

#define RINA_PREFIX "ipcm.conf"

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/logs.h>
#include <librina/json/json.h>
#include "ipcm.h"

#define CONF_FILE_CUR_VERSION "1.4.1"

using namespace std;

namespace rinad {

string IPCProcessToCreate::ALL_SHIM_DIFS = "all-shims";

void parse_name(const Json::Value &root,
                rina::ApplicationProcessNamingInformation &name)
{
        name.processName     = root.get("apName",     string()).asString();
        name.processInstance = root.get("apInstance", string()).asString();
        name.entityName      = root.get("aeName",     string()).asString();
        name.entityInstance  = root.get("aeInstance", string()).asString();
}

void parse_ipc_to_create(const Json::Value          root,
                         list<IPCProcessToCreate> & ipcProcessesToCreate)
{
        Json::Value ipc_processes = root["ipcProcessesToCreate"];

        for (unsigned int i = 0; i < ipc_processes.size(); i++) {
                rinad::IPCProcessToCreate ipc;

                // IPC process Names
                // Might want to move this to another function
                parse_name(ipc_processes[i], ipc.name);

                ipc.difName = rina::ApplicationProcessNamingInformation
                        (ipc_processes[i].get
                         ("difName", string()).asString(),
                         string());

                //Neighbors
                Json::Value neigh = ipc_processes[i]["neighbors"];
                if (neigh != 0) {
                        for (unsigned int j = 0; j < neigh.size(); j++) {
                                rinad::NeighborData neigh_data;

                                parse_name(neigh[j], neigh_data.apName);

                                neigh_data.difName =
                                        rina::ApplicationProcessNamingInformation
                                        (neigh[j]
                                         .get("difName", string())
                                         .asString(),
                                         string());

                                neigh_data.supportingDifName =
                                        rina::ApplicationProcessNamingInformation
                                        (neigh[j]
                                         .get("supportingDifName",
                                              string())
                                         .asString(),
                                         string());

                                ipc.neighbors.push_back(neigh_data);

                        }
                }

                // DIFs to register at
                Json::Value difs_reg = ipc_processes[i]["difsToRegisterAt"];
                if (difs_reg != 0) {
                        for (unsigned int j = 0; j < difs_reg.size(); j++) {
                                ipc.difsToRegisterAt.push_back
                                        (rina::ApplicationProcessNamingInformation
                                         (difs_reg[j].asString(),
                                          string()));
                        }
                }

                ipc.hostname = ipc_processes[i].get
                        ("hostName", string()).asString();

                // Peer discovery
                Json::Value peer_disc = ipc_processes[i]["n1difPeerDiscovery"];
                if (peer_disc != 0) {
                	for (unsigned int j = 0; j < peer_disc.size(); j++) {
                		ipc.n1difsPeerDiscovery.push_back(peer_disc[j].asString());
                	}
                }

                // parameters
                Json::Value params = ipc_processes[i]["parameters"];
                if (params != 0) {
                        Json::Value::Members members = params.getMemberNames();
                        for (unsigned int j = 0; j < members.size(); j++) {
                                string value = params
                                        .get(members[i], string()).asString();
                                ipc.parameters
                                        .insert(pair<string, string>
                                                (members[i], value));
                        }
                }

                ipcProcessesToCreate.push_back(ipc);
        }
}

void check_conf_file_version(const Json::Value& root, const std::string& cur_version)
{
	std::string f_version;

	f_version = root.get("configFileVersion", f_version).asString();

	if (f_version == cur_version) {
		/* ok */
		return;
	}

	if (f_version.empty()) {
		LOG_ERR("configFileVersion is missing in configuration file");
	} else {
		LOG_ERR("Configuration file version mismatch: expected = '%s',"
			" provided = '%s'", cur_version.c_str(), f_version.c_str());
	}
	LOG_INFO("Exiting...");
	exit(EXIT_FAILURE);
}

void parse_local_conf(const Json::Value &         root,
                      rinad::LocalConfiguration & local)
{
        Json::Value local_conf = root["localConfiguration"];
	Json::Value plugins_paths;

	if (local_conf == 0) {
		return;
	}

	local.consoleSocket = local_conf.get("consoleSocket",
					   local.consoleSocket).asString();
	if (local.consoleSocket.empty()) {
		local.consoleSocket = DEFAULT_RUNDIR "/ipcm-console.sock";
	}

	local.installationPath = local_conf.get("installationPath",
				 local.installationPath).asString();
	if (local.installationPath.empty()) {
		local.installationPath = std::string(DEFAULT_BINDIR);
	}

	local.libraryPath = local_conf.get("libraryPath",
					   local.libraryPath).asString();
	if (local.libraryPath.empty()) {
		local.libraryPath = std::string(DEFAULT_LIBDIR);
	}

	local.logPath = local_conf.get("logPath", local.logPath).asString();
	if (local.logPath.empty()) {
		local.logPath = std::string(DEFAULT_LOGDIR);
	}

	plugins_paths = local_conf["pluginsPaths"];
	if (plugins_paths != 0) {
		for (unsigned int j = 0; j < plugins_paths.size();
				j++) {
			local.pluginsPaths.push_back(
					plugins_paths[j].asString());
		}
	}

	local.system_name.processName = local_conf.get("system-name",
						       local.system_name.processName).asString();
	if (local.system_name.processName.empty()) {
		local.system_name.processName = "";
		local.system_name.processInstance = "";
	} else {
		local.system_name.processInstance = "1";
	}
}

void parse_dif_configs(const Json::Value   & root,
                       list<DIFTemplateMapping> & difConfigurations)
{
        Json::Value dif_configs = root["difConfigurations"];
        if (dif_configs != 0) {
        	for (unsigned int i = 0; i < dif_configs.size(); i++) {
        		rinad::DIFTemplateMapping mapping;

                        mapping.dif_name =
                                rina::ApplicationProcessNamingInformation
                                (dif_configs[i].get("name", string()).asString(),
                                 string());

                        mapping.template_name = dif_configs[i].get("template", string())
                                .asString();

                        difConfigurations.push_back(mapping);
        	}
        }
}

bool parse_configuration(std::string& file_loc)
{
        // General note: Params should be checked before they are used
        // Some can be NULL

        // Parse config file with jsoncpp
        Json::Value  root;
        Json::Reader reader;
        ifstream     file;

        file.open(file_loc.c_str());
        if (file.fail()) {
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

        // Get everything in our data structures
        rinad::RINAConfiguration config;

	config.configuration_file = file_loc;
	config.configuration_file_version = CONF_FILE_CUR_VERSION;
	check_conf_file_version(root, config.configuration_file_version);
        parse_local_conf(root, config.local);
        parse_ipc_to_create(root, config.ipcProcessesToCreate);
        parse_dif_configs(root, config.difConfigurations);
        IPCManager->loadConfig(config);

        return true;
}

}
