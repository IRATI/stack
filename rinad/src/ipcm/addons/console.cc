/*
 * IPC Manager console
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
 *    Marc Sune             <marc.sune (at) bisdn.de>
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
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

#define RINA_PREFIX "ipcm.console"
#include <librina/logs.h>
#include <librina/timer.h>

#include "rina-configuration.h"
#include "../ipcm.h"
#include "console.h"

using namespace std;

namespace rinad {

const string IPCMConsole::NAME = "console";

class CreateIPCPConsoleCmd: public rina::ConsoleCmdInfo {
public:
	CreateIPCPConsoleCmd(IPCMConsole * console) :
		rina::ConsoleCmdInfo("USAGE: create-ipcp <ipcp-type> <dif-name>", console) {};

	int execute(std::vector<string>& args) {
		CreateIPCPPromise promise;

		if (args.size() < 3) {
			console->outstream << console->commands_map[args[0]]->usage << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		rina::ApplicationProcessNamingInformation ipcp_name;

		if(IPCManager->create_ipcp((IPCMConsole*) console, &promise, ipcp_name, args[1], args[2]) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS){
			console->outstream << "Error while creating IPC process" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		console->outstream << "IPC process created successfully [id = "
		      << promise.ipcp_id << ", name="<<args[1]<<"]" << endl;

		return rina::UNIXConsole::CMDRETCONT;
	}
};

class DestroyIPCPConsoleCmd: public rina::ConsoleCmdInfo {
public:
	DestroyIPCPConsoleCmd(IPCMConsole * console) :
		rina::ConsoleCmdInfo("USAGE: destroy-ipcp <ipcp-id>", console) {};

	int execute(std::vector<string>& args) {
		int ipcp_id;

		if (args.size() < 2) {
			console->outstream << console->commands_map[args[0]]->usage << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if(rina::string2int(args[1], ipcp_id)){
			console->outstream << "Invalid IPC process id" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if(IPCManager->destroy_ipcp((IPCMConsole*) console, ipcp_id) != IPCM_SUCCESS){
			console->outstream << "Destroy operation failed" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		console->outstream << "IPC process successfully destroyed" << endl;

		return rina::UNIXConsole::CMDRETCONT;
	}
};

class ListIPCPsConsoleCmd: public rina::ConsoleCmdInfo {
public:
	ListIPCPsConsoleCmd(IPCMConsole * console) :
		rina::ConsoleCmdInfo("USAGE: list-ipcps", console) {};

	int execute(std::vector<string>& args) {
		IPCManager->list_ipcps(console->outstream);

		return rina::UNIXConsole::CMDRETCONT;
	}
};

class ListDIFAllocatorMapCmd: public rina::ConsoleCmdInfo {
public:
	ListDIFAllocatorMapCmd(IPCMConsole * console) :
		rina::ConsoleCmdInfo("USAGE: list-da-map", console) {};

	int execute(std::vector<string>& args) {
		IPCManager->list_da_mappings(console->outstream);

		return rina::UNIXConsole::CMDRETCONT;
	}
};

class ListIPCPTypesConsoleCmd: public rina::ConsoleCmdInfo {
public:
	ListIPCPTypesConsoleCmd(IPCMConsole * console) :
		rina::ConsoleCmdInfo("USAGE: list-ipcp-types", console) {};

	int execute(std::vector<std::string>& args) {
		std::list<string> types;

		IPCManager->list_ipcp_types(types);

		console->outstream << "Supported IPC process types:" << endl;
		for (list<string>::const_iterator it = types.begin();
						it != types.end(); it++) {
			console->outstream << "    " << *it << endl;
		}

		return rina::UNIXConsole::CMDRETCONT;
	}
};

class AssignToDIFConsoleCmd: public rina::ConsoleCmdInfo {
public:
	AssignToDIFConsoleCmd(IPCMConsole * console) :
		rina::ConsoleCmdInfo("USAGE: assign-to-dif <ipcp-id> "
				"<dif-name> <dif-template-name>", console) {};

	int execute(std::vector<string>& args) {
		int ipcp_id;
		Promise promise;
		rinad::DIFTemplate dif_template;
		int rv;

		if (args.size() < 4) {
			console->outstream << console->commands_map[args[0]]->usage << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		rina::ApplicationProcessNamingInformation dif_name(args[2], string());

		if (rina::string2int(args[1], ipcp_id)) {
			console->outstream << "Invalid IPC process id" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if (!IPCManager->ipcp_exists(ipcp_id)) {
			console->outstream << "No such IPC process id" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		rv = IPCManager->dif_template_manager->get_dif_template(args[3], dif_template);
		if (rv != 0) {
			console->outstream << "Cannot find DIF template called " << args[3] << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if (IPCManager->assign_to_dif((IPCMConsole*) console, &promise, ipcp_id, dif_template, dif_name) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS){
			console->outstream << "DIF assignment failed" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		console->outstream << "DIF assignment completed successfully"<< endl;

		return rina::UNIXConsole::CMDRETCONT;
	}
};

class QueryRIBConsoleCmd: public rina::ConsoleCmdInfo {
public:
	QueryRIBConsoleCmd(IPCMConsole * console) :
		rina::ConsoleCmdInfo("USAGE: query-rib <ipcp-id> "
				"[(<object-class>|-) [<object-name>]]", console) {};

	int execute(std::vector<string>& args) {
		int ipcp_id;
		QueryRIBPromise promise;
		string objectClass, objectName;

		if (args.size() < 2) {
			console->outstream << console->commands_map[args[0]]->usage << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if(rina::string2int(args[1], ipcp_id)){
			console->outstream << "Invalid IPC process id" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if (!IPCManager->ipcp_exists(ipcp_id)) {
			console->outstream << "No such IPC process id" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if (args.size() > 2) {
			if (args[2] != "-")
				objectClass = args[2];
			if (args.size() > 3)
				objectName = args[3];
		}

		if (IPCManager->query_rib((IPCMConsole*) console, &promise, ipcp_id,
					  objectClass, objectName) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			console->outstream << "Query RIB operation failed" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		console->outstream << promise.serialized_rib << endl;

		return rina::UNIXConsole::CMDRETCONT;
	}
};

class RegisterAtDIFConsoleCmd: public rina::ConsoleCmdInfo {
public:
	RegisterAtDIFConsoleCmd(IPCMConsole * console) :
		rina::ConsoleCmdInfo("USAGE: register-at-dif <ipcp-id> "
				"<dif-name>", console) {};

	int execute(std::vector<string>& args) {
		int ipcp_id;
		Promise promise;

		if (args.size() < 3) {
			console->outstream << console->commands_map[args[0]]->usage << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		rina::ApplicationProcessNamingInformation dif_name(args[2], string());

		if (rina::string2int(args[1], ipcp_id)){
			console->outstream << "Invalid IPC process id" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if (!IPCManager->ipcp_exists(ipcp_id)) {
			console->outstream << "No such IPC process id" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if(IPCManager->register_at_dif((IPCMConsole*) console, &promise, ipcp_id, dif_name) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			console->outstream << "Registration failed" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		console->outstream << "IPC process registration completed successfully" << endl;

		return rina::UNIXConsole::CMDRETCONT;
	}
};

class UnregisterFromDIFConsoleCmd: public rina::ConsoleCmdInfo {
public:
	UnregisterFromDIFConsoleCmd(IPCMConsole * console) :
		rina::ConsoleCmdInfo("USAGE: unregister-from-dif <ipcp-id> "
				"<dif-name>", console) {};

	int execute(std::vector<string>& args) {
		int ipcp_id;
		Promise promise;

		if (args.size() < 3) {
			console->outstream << console->commands_map[args[0]]->usage << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		rina::ApplicationProcessNamingInformation dif_name(args[2], string());

		if(rina::string2int(args[1], ipcp_id)){
			console->outstream << "Invalid IPC process id" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}
		if (!IPCManager->ipcp_exists(ipcp_id)){
			console->outstream << "No such IPC process id" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		//Call IPCManager
		if(IPCManager->unregister_ipcp_from_ipcp((IPCMConsole*) console, &promise, ipcp_id,
			dif_name) == IPCM_FAILURE || promise.wait() != IPCM_SUCCESS) {
			console->outstream << "Unregistration failed" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		console->outstream << "IPC process unregistration completed "
				"successfully" << endl;

		return rina::UNIXConsole::CMDRETCONT;
	}
};

class UpdateDIFConfigConsoleCmd: public rina::ConsoleCmdInfo {
public:
	UpdateDIFConfigConsoleCmd(IPCMConsole * console) :
		rina::ConsoleCmdInfo("USAGE: update-dif-config <ipcp-id>", console) {};

	int execute(std::vector<string>& args) {
		int ipcp_id;
		Promise promise;

		if (args.size() < 3) {
			console->outstream << console->commands_map[args[0]]->usage << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		rina::ApplicationProcessNamingInformation dif_name(args[2], string());

		if(rina::string2int(args[1], ipcp_id)){
			console->outstream << "Invalid IPC process id" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}
		if (!IPCManager->ipcp_exists(ipcp_id)){
			console->outstream << "No such IPC process id" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		//Call IPCManager
		if(IPCManager->unregister_ipcp_from_ipcp((IPCMConsole*) console, &promise, ipcp_id,
			dif_name) == IPCM_FAILURE || promise.wait() != IPCM_SUCCESS) {
			console->outstream << "Unregistration failed" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		console->outstream << "IPC process unregistration completed "
				"successfully" << endl;

		return rina::UNIXConsole::CMDRETCONT;
	}
};

class EnrollToDIFConsoleCmd: public rina::ConsoleCmdInfo {
public:
	EnrollToDIFConsoleCmd(IPCMConsole * console) :
		rina::ConsoleCmdInfo("USAGE: enroll-to-dif <ipcp-id> <dif-name> "
				"<supporting-dif-name> <neighbor-process-name>"
				"<neighbor-process-instance>  or  "
				"enroll-to-dif <ipcp-id> <dif-name> "
				"<supporting-dif-name>", console) {};

	int execute(std::vector<string>& args) {
	        int t0 = rina::Time::get_time_in_ms();
	        NeighborData neighbor_data;
		int ipcp_id;
		Promise promise;

		if (args.size() < 6 && args.size() < 4) {
			console->outstream << console->commands_map[args[0]]->usage << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if(rina::string2int(args[1], ipcp_id)){
			console->outstream << "Invalid IPC process id" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		neighbor_data.difName = rina::ApplicationProcessNamingInformation(
					args[2], string());
		neighbor_data.supportingDifName =
			rina::ApplicationProcessNamingInformation(args[3], string());

		if (args.size() == 6) {
			neighbor_data.apName =
					rina::ApplicationProcessNamingInformation(args[4], args[5]);
		}

		if (!IPCManager->ipcp_exists(ipcp_id)) {
			console->outstream << "No such IPC process id" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if(IPCManager->enroll_to_dif((IPCMConsole*) console, &promise, ipcp_id, neighbor_data) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			console->outstream << "Enrollment operation failed" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}
	        int t1 = rina::Time::get_time_in_ms();
	        console->outstream << "DIF enrollment succesfully completed in " << t1 - t0 << " ms" << endl;

		return rina::UNIXConsole::CMDRETCONT;
	}
};

class DisconnectNeighborConsoleCmd: public rina::ConsoleCmdInfo {
public:
	DisconnectNeighborConsoleCmd(IPCMConsole * console) :
		rina::ConsoleCmdInfo("USAGE: disc-nei <ipcp-id> "
				"<neighbor-process-name> "
				"<neighbor-process-instance>", console) {};

	int execute(std::vector<string>& args) {
		int t0 = rina::Time::get_time_in_ms();
	        rina::ApplicationProcessNamingInformation neighbor;
		int ipcp_id;
		Promise promise;

		if (args.size() < 4) {
			console->outstream << console->commands_map[args[0]]->usage << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if(rina::string2int(args[1], ipcp_id)){
			console->outstream << "Invalid IPC process id" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		neighbor =
			rina::ApplicationProcessNamingInformation(args[2], args[3]);

		if (!IPCManager->ipcp_exists(ipcp_id)) {
			console->outstream << "No such IPC process id" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if(IPCManager->disconnect_neighbor((IPCMConsole*) console, &promise, ipcp_id, neighbor) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			console->outstream << "Disconnect neighbor operation failed" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		int t1 = rina::Time::get_time_in_ms();
		console->outstream << "Neighbor disconnection operation completed in " << t1 - t0 << " ms" << endl;

		return rina::UNIXConsole::CMDRETCONT;
	}
};

class SelectPolicySetConsoleCmd: public rina::ConsoleCmdInfo {
public:
	SelectPolicySetConsoleCmd(IPCMConsole * console) :
		rina::ConsoleCmdInfo("USAGE: select-policy-set <ipcp-id> "
				"<component-path> <policy-set-name> ", console) {};

	int execute(std::vector<string>& args) {
		int ipcp_id;
		Promise promise;

		if (args.size() < 4) {
			console->outstream << console->commands_map[args[0]]->usage << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if(rina::string2int(args[1], ipcp_id) ){
			console->outstream << "Invalid IPC process id" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if (!IPCManager->ipcp_exists(ipcp_id)) {
			console->outstream << "No such IPC process id" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if(IPCManager->select_policy_set((IPCMConsole*) console, &promise, ipcp_id, args[2],
				args[3]) == IPCM_FAILURE  || promise.wait() != IPCM_SUCCESS) {
			console->outstream << "select-policy-set operation failed" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		console->outstream << "Policy-set selection succesfully completed" << endl;

		return rina::UNIXConsole::CMDRETCONT;
	}
};

class SelectPolicySetParamConsoleCmd: public rina::ConsoleCmdInfo {
public:
	SelectPolicySetParamConsoleCmd(IPCMConsole * console) :
		rina::ConsoleCmdInfo("USAGE: set-policy-set-param <ipcp-id> "
				"<policy-path> <param-name> <param-value>", console) {};

	int execute(std::vector<string>& args) {
		int ipcp_id;
		Promise promise;

		if (args.size() < 5) {
			console->outstream << console->commands_map[args[0]]->usage << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if(rina::string2int(args[1], ipcp_id)){
			console->outstream << "Invalid IPC process id" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if (!IPCManager->ipcp_exists(ipcp_id)) {
			console->outstream << "No such IPC process id" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if(IPCManager->set_policy_set_param((IPCMConsole*) console, &promise, ipcp_id, args[2],
				args[3], args[4]) == IPCM_FAILURE || promise.wait() != IPCM_SUCCESS) {
			console->outstream << "set-policy-set-param operation failed"<< endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		console->outstream << "Policy-set parameter succesfully set"<< endl;

		return rina::UNIXConsole::CMDRETCONT;
	}
};

class PluginLoadConsoleCmd: public rina::ConsoleCmdInfo {
public:
	PluginLoadConsoleCmd(IPCMConsole * console) :
		rina::ConsoleCmdInfo("USAGE: plugin-load <ipcp-id> "
				"<plugin-name>", console) {};

	int execute(std::vector<string>& args) {
		return ((IPCMConsole *) console)->plugin_load_unload(args, true);
	}
};

class PluginUnloadConsoleCmd: public rina::ConsoleCmdInfo {
public:
	PluginUnloadConsoleCmd(IPCMConsole * console) :
		rina::ConsoleCmdInfo("USAGE: plugin-unload <ipcp-id> "
				"<plugin-name>", console) {};

	int execute(std::vector<string>& args) {
		return ((IPCMConsole *) console)->plugin_load_unload(args, false);
	}
};

class PluginGetInfoConsoleCmd: public rina::ConsoleCmdInfo {
public:
	PluginGetInfoConsoleCmd(IPCMConsole * console) :
		rina::ConsoleCmdInfo("USAGE: plugin-get-info <plugin-name>", console) {};

	int execute(std::vector<string>& args) {
		if (args.size() < 2) {
			console->outstream << console->commands_map[args[0]]->usage << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

	        const std::string& plugin_name = args[1];
		std::list<rina::PsInfo> policy_sets;

		if (IPCManager->plugin_get_info(plugin_name, policy_sets) == IPCM_FAILURE) {
			console->outstream << "Failed to get information about plugin "
	                                << plugin_name << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		for (std::list<rina::PsInfo>::iterator lit = policy_sets.begin();
						lit != policy_sets.end(); lit++) {
			console->outstream << "Policy set: Name='" << lit->name <<
				     "', Component='" << lit->app_entity <<
				     "', Version='" << lit->version << "'" << endl;
		}

		return rina::UNIXConsole::CMDRETCONT;
	}
};

class ShowDIFTemplatesConsoleCmd: public rina::ConsoleCmdInfo {
public:
	ShowDIFTemplatesConsoleCmd(IPCMConsole * console) :
		rina::ConsoleCmdInfo("USAGE: show-dif-templates", console) {};

	int execute(std::vector<string>& args) {
		std::list<DIFTemplate> dif_templates;

		if (args.size() != 1) {
			console->outstream << console->commands_map[args[0]]->usage << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		IPCManager->dif_template_manager->get_all_dif_templates(dif_templates);

		console->outstream<< "CURRENT DIF TEMPLATES:" << endl;

		std::list<DIFTemplate>::iterator it;
		for (it = dif_templates.begin(); it != dif_templates.end(); ++it) {
			console->outstream << it->toString() << endl;
		}

		return rina::UNIXConsole::CMDRETCONT;
	}
};

class ReadIPCPRIBObjConsoleCmd: public rina::ConsoleCmdInfo {
public:
	ReadIPCPRIBObjConsoleCmd(IPCMConsole * console) :
		rina::ConsoleCmdInfo("USAGE: read-ipcp-ribobj <ipcp-id> <object-class> "
				"<object-name> <scope>", console) {};

	int execute(std::vector<string>& args) {
		Promise promise;
		int ipcp_id;
		int scope;

		if (args.size() != 5) {
			console->outstream << console->commands_map[args[0]]->usage << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if (rina::string2int(args[1], ipcp_id)){
			console->outstream << "Invalid IPC process id" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if (rina::string2int(args[4], scope)){
			console->outstream << "Invalid scope" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if (!IPCManager->ipcp_exists(ipcp_id)) {
			console->outstream << "No such IPC process id" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		// CAREFUL, DELEGATION OBJECT SET TO NULL, this function is only
		// for testing, return result is not being processed.
		rina::ser_obj_t obj_value;
		if (IPCManager->delegate_ipcp_ribobj(NULL,
				ipcp_id, rina::cdap::cdap_m_t::M_READ, args[2], args[3], obj_value, scope, 1, 0)
				== IPCM_FAILURE || promise.wait() != IPCM_SUCCESS) {
			console->outstream << "Error occured while forwarding CDAP message to IPCP"
					<< endl;
		} else {
			console->outstream << "Successfully sent M_READ request "
				  << "with object class = " << args[2]
				  << " and object name = " << args[3] << endl;
		}

		return rina::UNIXConsole::CMDRETCONT;
	}
};

class ShowCatalogueConsoleCmd: public rina::ConsoleCmdInfo {
public:
	ShowCatalogueConsoleCmd(IPCMConsole * console) :
		rina::ConsoleCmdInfo("USAGE: show-catalog [<component-name>]", console) {};

	int execute(std::vector<string>& args) {
		switch (args.size()) {
		case 1:
			console->outstream << IPCManager->catalog.toString();
			break;

		case 2:
			console->outstream << IPCManager->catalog.toString(args[1]);
			break;

		default:
			console->outstream << console->commands_map[args[0]]->usage << endl;
			break;
		}

		return rina::UNIXConsole::CMDRETCONT;
	}
};

class UpdateCatalogueConsoleCmd: public rina::ConsoleCmdInfo {
public:
	UpdateCatalogueConsoleCmd(IPCMConsole * console) :
		rina::ConsoleCmdInfo("USAGE: update-catalog", console) {};

	int execute(std::vector<string>& args) {
		if (args.size() != 1) {
			console->outstream << console->commands_map[args[0]]->usage << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		console->outstream << IPCManager->query_ma_rib();

		return rina::UNIXConsole::CMDRETCONT;
	}
};

class QueryMARIBConsoleCmd: public rina::ConsoleCmdInfo {
public:
	QueryMARIBConsoleCmd(IPCMConsole * console) :
		rina::ConsoleCmdInfo("USAGE: query_ma_rib", console) {};

	int execute(std::vector<string>& args) {
		IPCManager->update_catalog((IPCMConsole*) console);
		console->outstream << "Catalog updated" << endl;

		return rina::UNIXConsole::CMDRETCONT;
	}
};

class RegisterIPVPNConsoleCmd: public rina::ConsoleCmdInfo {
public:
	RegisterIPVPNConsoleCmd(IPCMConsole * console) :
		rina::ConsoleCmdInfo("USAGE: register-ip-vpn <vpn-name> "
				"<dif-name>", console) {};

	int execute(vector<string>& args) {
		stringstream ss;
		Promise promise;

		if (args.size() < 3) {
			console->outstream << console->commands_map[args[0]]->usage << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		rina::ApplicationProcessNamingInformation dif_name(args[2], string());

		if(IPCManager->register_ip_vpn_to_dif(&promise, args[1], dif_name) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			console->outstream << "IP VPN registration failed" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		console->outstream << "IP VPN registration completed successfully" << endl;

		return rina::UNIXConsole::CMDRETCONT;
	}
};

class UnegisterIPVPNConsoleCmd: public rina::ConsoleCmdInfo {
public:
	UnegisterIPVPNConsoleCmd(IPCMConsole * console) :
		rina::ConsoleCmdInfo("USAGE: unregister-ip-vpn <vpn-name> "
				"<dif-name>", console) {};

	int execute(vector<string>& args) {
		stringstream ss;
		Promise promise;

		if (args.size() < 3) {
			console->outstream << console->commands_map[args[0]]->usage << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		rina::ApplicationProcessNamingInformation dif_name(args[2], string());

		if(IPCManager->unregister_ip_vpn_from_dif(&promise, args[1], dif_name) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			console->outstream << "IP VPN unregistration failed" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		console->outstream << "IP VPN unregistration completed successfully" << endl;

		return rina::UNIXConsole::CMDRETCONT;
	}
};

class AllocateIPVPNFlowConsoleCmd: public rina::ConsoleCmdInfo {
public:
	AllocateIPVPNFlowConsoleCmd(IPCMConsole * console) :
		rina::ConsoleCmdInfo("USAGE: allocate-ip-vpn-flow <vpn_name> "
				     "<dst-system-name> [<dif-name>]", console) {};

	int execute(vector<string>& args) {
		Promise promise;
		std::string dif_name;
		rina::FlowSpecification flow_spec;

		if (args.size() != 3 && args.size() != 4) {
			console->outstream << console->commands_map[args[0]]->usage << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if (args.size() == 4) {
			dif_name = args[3];
		} else {
			dif_name = "";
		}

		if(IPCManager->allocate_ipvpn_flow(&promise, args[1], args[2],
						   dif_name, flow_spec) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			console->outstream << "IP VPN flow allocation failed" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}
		console->outstream << "IP VPN flow allocated successfully" << endl;

		return rina::UNIXConsole::CMDRETCONT;
	}
};

class DeallocateIPVPNFlowConsoleCmd: public rina::ConsoleCmdInfo {
public:
	DeallocateIPVPNFlowConsoleCmd(IPCMConsole * console) :
		rina::ConsoleCmdInfo("USAGE: deallocate-ip-vpn-flow <port_id>]", console) {};

	int execute(vector<string>& args) {
		int port_id;

		if (args.size() != 2) {
			console->outstream << console->commands_map[args[0]]->usage << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if (rina::string2int(args[1], port_id)){
			console->outstream << "Invalid IPC port id" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if(IPCManager->deallocate_ipvpn_flow(port_id) != IPCM_SUCCESS) {
			console->outstream << "IP VPN flow deallocation failed" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		console->outstream << "IP VPN flow deallocated successfully" << endl;

		return rina::UNIXConsole::CMDRETCONT;
	}
};

class MapIPPrefixToFlowConsoleCmd: public rina::ConsoleCmdInfo {
public:
	MapIPPrefixToFlowConsoleCmd(IPCMConsole * console) :
		rina::ConsoleCmdInfo("USAGE: map-ip-prefix-to-flow <ip_prefix> "
				     "<port_id>", console) {};

	int execute(vector<string>& args) {
		int port_id;

		if (args.size() != 3) {
			console->outstream << console->commands_map[args[0]]->usage << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if (rina::string2int(args[2], port_id)){
			console->outstream << "Invalid IPC port id" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if(IPCManager->map_ip_prefix_to_flow(args[1], port_id) != IPCM_SUCCESS) {
			console->outstream << "Mapping IP prefix to flow failed" << endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		console->outstream << "IP Prefix mapped to flow successfully" << endl;

		return rina::UNIXConsole::CMDRETCONT;
	}
};

IPCMConsole::IPCMConsole(const string& socket_path_) :
		rina::UNIXConsole(socket_path_, "IPCM"),
		Addon(IPCMConsole::NAME)
{
	commands_map["create-ipcp"] = new CreateIPCPConsoleCmd(this);
	commands_map["destroy-ipcp"] = new DestroyIPCPConsoleCmd(this);
	commands_map["list-ipcps"] = new ListIPCPsConsoleCmd(this);
	commands_map["list-ipcp-types"] = new ListIPCPTypesConsoleCmd(this);
	commands_map["assign-to-dif"] = new AssignToDIFConsoleCmd(this);
	commands_map["register-at-dif"] = new RegisterAtDIFConsoleCmd(this);
	commands_map["unregister-from-dif"] = new UnregisterFromDIFConsoleCmd(this);
	commands_map["update-dif-config"] = new UpdateDIFConfigConsoleCmd(this);
	commands_map["enroll-to-dif"] = new EnrollToDIFConsoleCmd(this);
	commands_map["disc-neigh"] = new DisconnectNeighborConsoleCmd(this);
	commands_map["query-rib"] = new QueryRIBConsoleCmd(this);
	commands_map["select-policy-set"] = new SelectPolicySetConsoleCmd(this);
	commands_map["set-policy-set-param"] = new SelectPolicySetParamConsoleCmd(this);
	commands_map["plugin-load"] = new PluginLoadConsoleCmd(this);
	commands_map["plugin-unload"] = new PluginUnloadConsoleCmd(this);
	commands_map["plugin-get-info"] = new PluginGetInfoConsoleCmd(this);
	commands_map["show-dif-templates"] = new ShowDIFTemplatesConsoleCmd(this);
	commands_map["read-ipcp-ribobj"] = new ReadIPCPRIBObjConsoleCmd(this);
	commands_map["show-catalog"] = new ShowCatalogueConsoleCmd(this);
	commands_map["update-catalog"] = new UpdateCatalogueConsoleCmd(this);
	commands_map["query-ma-rib"] = new QueryMARIBConsoleCmd(this);
	commands_map["list-da-map"] = new ListDIFAllocatorMapCmd(this);
	commands_map["register-ip-vpn"] = new RegisterIPVPNConsoleCmd(this);
	commands_map["unregister-ip-vpn"] = new UnegisterIPVPNConsoleCmd(this);
	commands_map["allocate-ip-vpn-flow"] = new AllocateIPVPNFlowConsoleCmd(this);
	commands_map["deallocate-ip-vpn-flow"] = new DeallocateIPVPNFlowConsoleCmd(this);
	commands_map["map-ip-prefix-to-flow"] = new MapIPPrefixToFlowConsoleCmd(this);
}

int IPCMConsole::plugin_load_unload(vector<string>& args, bool load)
{
	int ipcp_id;
	Promise promise;

	if (args.size() < 3) {
		outstream << commands_map[args[0]]->usage << endl;
		return CMDRETCONT;
	}

	if (rina::string2int(args[1], ipcp_id)){
		outstream << "Invalid IPC process id" << endl;
		return CMDRETCONT;
	}

	if (!IPCManager->ipcp_exists(ipcp_id)) {
		outstream << "No such IPC process id" << endl;
		return CMDRETCONT;
	}

	string un;

	if (!load) {
		un = "un";
	}

	if (IPCManager->plugin_load(this, &promise, ipcp_id, args[2], load) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		outstream << "Plugin " << un << "loading failed" << endl;
		return CMDRETCONT;
	}

	outstream << "Plugin " << un << "loaded succesfully" << endl;

	return CMDRETCONT;
}

}//namespace rinad
