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

#include <cstdlib>
#include <cstddef>
#include <iostream>
#include <map>
#include <vector>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <sstream>

#define RINA_PREFIX "ipcm.console"
#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/logs.h>
#include <debug.h>

#include "rina-configuration.h"
#include "../ipcm.h"
#include "console.h"

using namespace std;


namespace rinad {

//Static members
const std::string IPCMConsole::NAME = "console";

int string2int(const string& s, int& ret)
{
	char *dummy;
	const char *cstr = s.c_str();

	ret = strtoul(cstr, &dummy, 10);
	if (!s.size() || *dummy != '\0') {
		ret = ~0U;
		return -1;
	}

	return 0;
}


void *
console_function(void *opaque)
{
	IPCMConsole *console = static_cast<IPCMConsole *>(opaque);

	console->body();

	return NULL;
}

IPCMConsole::IPCMConsole(const std::string& socket_path_) :
		Addon(IPCMConsole::NAME),
		socket_path(socket_path_)
{
	commands_map["help"] = ConsoleCmdInfo(&IPCMConsole::help,
				"USAGE: help [<command>]");
	commands_map["quit"] = ConsoleCmdInfo(&IPCMConsole::quit,
					      "USAGE: quit");
	commands_map["exit"] = ConsoleCmdInfo(&IPCMConsole::quit,
					      "USAGE: exit");
	commands_map["create-ipcp"] =
			ConsoleCmdInfo(&IPCMConsole::create_ipcp,
				"USAGE: create-ipcp <process-name> "
				"<process-instance> <ipcp-type>");
	commands_map["destroy-ipcp"] =
			ConsoleCmdInfo(&IPCMConsole::destroy_ipcp,
				"USAGE: destroy-ipcp <ipcp-id>");
	commands_map["list-ipcps"] =
			ConsoleCmdInfo(&IPCMConsole::list_ipcps,
				"USAGE: list-ipcps");
	commands_map["list-ipcp-types"] =
			ConsoleCmdInfo(&IPCMConsole::list_ipcp_types,
				"USAGE: list-ipcp-types");
	commands_map["assign-to-dif"] =
			ConsoleCmdInfo(&IPCMConsole::assign_to_dif,
				"USAGE: assign-to-dif <ipcp-id> "
				"<dif-name> <dif-template-name>");
	commands_map["register-at-dif"] =
			ConsoleCmdInfo(&IPCMConsole::register_at_dif,
				"USAGE: register-at-dif <ipcp-id> "
				"<dif-name>");
	commands_map["unregister-from-dif"] =
			ConsoleCmdInfo(&IPCMConsole::unregister_from_dif,
				"USAGE: unregister-from-dif <ipcp-id> "
				"<dif-name>");
	commands_map["update-dif-config"] =
			ConsoleCmdInfo(&IPCMConsole::update_dif_config,
				"USAGE: update-dif-config <ipcp-id>");
	commands_map["enroll-to-dif"] =
			ConsoleCmdInfo(&IPCMConsole::enroll_to_dif,
				"USAGE: enroll-to-dif <ipcp-id> <dif-name> "
				"<supporting-dif-name> <neighbor-process-name>"
				"<neighbor-process-instance>");
	commands_map["query-rib"] =
			ConsoleCmdInfo(&IPCMConsole::query_rib,
				"USAGE: query-rib <ipcp-id>");
	commands_map["select-policy-set"] =
			ConsoleCmdInfo(&IPCMConsole::select_policy_set,
				"USAGE: select-policy-set <ipcp-id> "
				"<component-path> <policy-set-name> ");
	commands_map["set-policy-set-param"] =
			ConsoleCmdInfo(&IPCMConsole::set_policy_set_param,
				"USAGE: set-policy-set-param <ipcp-id> "
				"<policy-path> <param-name> <param-value>");
	commands_map["plugin-load"] =
			ConsoleCmdInfo(&IPCMConsole::plugin_load,
				"USAGE: plugin-load <ipcp-id> "
				"<plugin-name>");
	commands_map["plugin-unload"] =
			ConsoleCmdInfo(&IPCMConsole::plugin_unload,
				"USAGE: plugin-unload <ipcp-id> "
				"<plugin-name>");
	commands_map["plugin-get-info"] =
			ConsoleCmdInfo(&IPCMConsole::plugin_get_info,
				"USAGE: plugin-get-info <plugin-name>");
	commands_map["show-dif-templates"] = ConsoleCmdInfo(&IPCMConsole::show_dif_templates,
					"USAGE: show-dif-templates");
	commands_map["read-ipcp-ribobj"] =
			ConsoleCmdInfo(&IPCMConsole::read_ipcp_ribobj,
				"USAGE: read-ipcp-ribobj <ipcp-id> <object-class> "
				"<object-name> <scope>");
	commands_map["show-catalog"] =
			ConsoleCmdInfo(&IPCMConsole::show_catalog,
				"USAGE: show-catalog [<component-name>]");

	commands_map["update-catalog"] =
			ConsoleCmdInfo(&IPCMConsole::update_catalog,
				"USAGE: update-catalog");

	keep_on_running = true;
	rina::ThreadAttributes ta;
	worker = new rina::Thread(console_function, this, &ta);
	worker->start();
}

IPCMConsole::~IPCMConsole() throw()
{
	void *status;
	this->keep_on_running = false;
	worker->join(&status);
	if (socket_path[0] != '@') {
		cleanup_filesystem_socket();
	}
	delete worker;
}

bool
IPCMConsole::cleanup_filesystem_socket()
{
	struct stat stat;
	const char *path = socket_path.c_str();
	// Ignore if it's not a socket to avoid data loss;
	// bind() will fail and we'll output a log there.
	return lstat(path, &stat) || !S_ISSOCK(stat.st_mode) || !unlink(path);
}

int
IPCMConsole::init()
{
	struct sockaddr_un sa_un;
	char fill[108] = "";
	memcpy(sa_un.sun_path, fill, 108);
	int sfd = -1;
	unsigned len = socket_path.size();

	if (len >= sizeof sa_un.sun_path) {
		LOG_ERR("AF_UNIX path too long");
		return -1;
	}

	socket_path.copy(sa_un.sun_path, len);
	if (socket_path[0] == '@') {
		sa_un.sun_path[0] = 0; // abstract unix socket
	} else if (!cleanup_filesystem_socket()) {
		LOG_ERR("Error [%i] unlinking %s", errno, socket_path.c_str());
		return -1;
	}
	len += offsetof(struct sockaddr_un, sun_path);

	sa_un.sun_family = AF_UNIX;
	try {
		sfd = socket(sa_un.sun_family, SOCK_STREAM, 0);
		if (sfd < 0) {
			throw rina::Exception("socket");
		}
		if (bind(sfd, reinterpret_cast<struct sockaddr *>(&sa_un),
				len)) {
			throw rina::Exception("bind");
		}
		if (listen(sfd, 5)) {
			throw rina::Exception("listen");
		}
	} catch (rina::Exception& e) {
		LOG_ERR("Error [%i] calling %s()", errno, e.what());
		if (sfd >= 0) {
			close(sfd);
			sfd = -1;
		}
	}

	return sfd;
}

int IPCMConsole::read_with_timeout(int cfd, void *buf, size_t count)
{
	const char *fname;
	while(this->keep_on_running) {
		struct timeval timeout = {2,0};
		fd_set readset;
		FD_ZERO(&readset);
		FD_SET(cfd, &readset);

		int res = select(cfd+1, &readset, NULL, NULL, &timeout);
		if (res > 0) {
			res = read(cfd, buf, count);
			if (res >= 0) {
				return res;
			}
			fname = "read";
		} else if (res) {
			fname = "select";
		} else {
			continue;
		}
		LOG_ERR("Error [%i] calling %s()\n", errno, fname);
		break;
	}
	return -1;
}


int IPCMConsole::accept_with_timeout(int sfd, struct sockaddr *addr,
				     socklen_t *addrlen)
{
	const char *fname;
	while(this->keep_on_running) {
		struct timeval timeout = {2,0};
		fd_set readset;
		FD_ZERO(&readset);
		FD_SET(sfd, &readset);

		int res = select(sfd+1, &readset, NULL, NULL, &timeout);
		if (res > 0) {
			res = accept(sfd, addr, addrlen);
			if (res >= 0) {
				return res;
			}
			fname = "read";
		} else if (res) {
			fname = "select";
		} else {
			continue;
		}
		LOG_ERR("Error [%i] calling %s()\n", errno, fname);
		break;
	}
	return -1;
}

void IPCMConsole::body()
{
	int sfd = init();

	if (sfd < 0) {
		return;
	}

	LOG_DBG("Console starts [fd=%d]\n", sfd);

	while (this->keep_on_running) {
		int cfd;
		char *cmdbuf, *p, *q;
		unsigned tail, size;
		int cmdret;
		int n;

		cfd = accept_with_timeout(sfd, NULL, NULL);
		if (cfd < 0) {
			break;
		}

		cmdbuf = NULL;
		tail = size = 0;
		while (this->keep_on_running) {
			outstream << "IPCM >>> ";
			if (flush_output(cfd)) {
				break;
			}

			do {
				n = size - tail;
				if (n < 128) {
					size = tail + (n = 128);
					p = (char*) realloc(cmdbuf, size);
					if (!p) {
					      goto stop;
					}
					cmdbuf = p;
				}

				n = read_with_timeout(
					cfd, p = cmdbuf + tail, n);
				if (n <= 0) {
					goto stop;
				}
				q = (char*) memchr(p, '\n', n);
				tail += n;
			} while (!q);

			p = cmdbuf;
			do {
				cmdret = process_command(cfd, p, q - p);
				if (cmdret == CMDRETSTOP) {
					goto stop;
				}
				p = q + 1;
				n = cmdbuf + tail - p;
			} while ((q = (char*) memchr(p, '\n', n)));
			memmove(cmdbuf, p, tail = n);
		}

	stop:
		free(cmdbuf);
		close(cfd);
	}

	close(sfd);
	LOG_DBG("Console stops\n");
}

int
IPCMConsole::flush_output(int cfd)
{
	int n;
	const string& str = outstream.str();
	ostringstream ss;

	n = write(cfd, str.c_str(), str.size());
	if (n < 0) {
		if (errno != EPIPE) {
		    ss  << " Error [" << errno <<
			"] calling write()" << endl;
		    FLUSH_LOG(ERR, ss);
		    return -1;
		} else {
		    ss  << " Console client disconnected" << endl;
		    FLUSH_LOG(INFO, ss);
		    return -1;
		}
	}

	// Make the stringstream empty
	outstream.str(string());

	return 0;
}

int
IPCMConsole::process_command(int cfd, char *cmdbuf, int size)
{
	map<string, ConsoleCmdInfo>::iterator mit;
	istringstream iss(string(cmdbuf, size));
	ConsoleCmdFunction fun;
	vector<string> args;
	string token;
	int ret;

	while (iss >> token) {
		args.push_back(token);
	}

	if (args.size() < 1) {
		return 0;
	}

	mit = commands_map.find(args[0]);
	if (mit == commands_map.end()) {
		outstream << "Unknown command '" << args[0] << "'" << endl;
		if (flush_output(cfd)) {
		    return CMDRETSTOP;
		}
		return 0;
	}

	fun = mit->second.fun;
	ret = (this->*fun)(args);

	outstream << endl;
	if (flush_output(cfd)) {
	    return CMDRETSTOP;
	}

	return ret;
}

int
IPCMConsole::quit(vector<string>& args)
{
	return CMDRETSTOP;
}

int IPCMConsole::help(vector<string>& args)
{
	if (args.size() < 2) {
		outstream << "Available commands:" << endl;
		for (map<string, ConsoleCmdInfo>::iterator mit =
				commands_map.begin();
					mit != commands_map.end(); mit++) {
			outstream << "    " << mit->first << endl;
		}
	} else {
		map<string, ConsoleCmdInfo>::iterator mit =
				commands_map.find(args[1]);

		if (mit == commands_map.end()) {
			outstream << "Unknown command '" << args[1]
					<< "'" << endl;
		} else {
			outstream << mit->second.usage << endl;
		}
	}

	return CMDRETCONT;
}

int
IPCMConsole::create_ipcp(vector<string>& args)
{
	CreateIPCPPromise promise;

	if (args.size() < 4) {
		outstream << commands_map[args[0]].usage << endl;
		return CMDRETCONT;
	}

	rina::ApplicationProcessNamingInformation ipcp_name(args[1], args[2]);

	if(IPCManager->create_ipcp(this, &promise, ipcp_name, args[3]) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS){
		outstream << "Error while creating IPC process" << endl;
		return CMDRETCONT;
	}

	outstream << "IPC process created successfully [id = "
	      << promise.ipcp_id << ", name="<<args[1]<<"]" << endl;

	return CMDRETCONT;
}

int
IPCMConsole::destroy_ipcp(vector<string>& args)
{
	int ipcp_id;

	if (args.size() < 2) {
		outstream << commands_map[args[0]].usage << endl;
		return CMDRETCONT;
	}

	if(string2int(args[1], ipcp_id)){
		outstream << "Invalid IPC process id" << endl;
		return CMDRETCONT;
	}

	if(IPCManager->destroy_ipcp(this, ipcp_id) != IPCM_SUCCESS){
		outstream << "Destroy operation failed" << endl;
		return CMDRETCONT;
	}

	outstream << "IPC process successfully destroyed" << endl;

	return CMDRETCONT;
}

int
IPCMConsole::list_ipcps(vector<string>&args)
{
	IPCManager->list_ipcps(outstream);

	return CMDRETCONT;
}

int
IPCMConsole::list_ipcp_types(std::vector<std::string>& args)
{
	std::list<std::string> types;

	IPCManager->list_ipcp_types(types);

	outstream << "Supported IPC process types:" << endl;
	for (list<string>::const_iterator it = types.begin();
					it != types.end(); it++) {
		outstream << "    " << *it << endl;
	}

	return CMDRETCONT;
}

int
IPCMConsole::assign_to_dif(std::vector<string>& args)
{
	int ipcp_id;
	Promise promise;
	rinad::DIFTemplate * dif_template;

	if (args.size() < 4) {
		outstream << commands_map[args[0]].usage << endl;
		return CMDRETCONT;
	}

	rina::ApplicationProcessNamingInformation dif_name(args[2], string());

	if (string2int(args[1], ipcp_id)) {
		outstream << "Invalid IPC process id" << endl;
		return CMDRETCONT;
	}

	if (!IPCManager->ipcp_exists(ipcp_id)) {
		outstream << "No such IPC process id" << endl;
		return CMDRETCONT;
	}

	dif_template = IPCManager->dif_template_manager->get_dif_template(args[3]);
	if (!dif_template) {
		outstream << "Cannot find DIF template called " << args[3] << endl;
		return CMDRETCONT;
	}

	if (IPCManager->assign_to_dif(this, &promise, ipcp_id, dif_template, dif_name) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS){
		outstream << "DIF assignment failed" << endl;
		return CMDRETCONT;
	}

	outstream << "DIF assignment completed successfully"<< endl;

	return CMDRETCONT;
}

int
IPCMConsole::query_rib(std::vector<string>& args)
{
	int ipcp_id;
	QueryRIBPromise promise;

	if (args.size() < 2) {
		outstream << commands_map[args[0]].usage << endl;
		return CMDRETCONT;
	}

	if(string2int(args[1], ipcp_id)){
		outstream << "Invalid IPC process id" << endl;
		return CMDRETCONT;
	}

	if (!IPCManager->ipcp_exists(ipcp_id)) {
		outstream << "No such IPC process id" << endl;
		return CMDRETCONT;
	}

	if (IPCManager->query_rib(this, &promise, ipcp_id) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		outstream << "Query RIB operation failed" << endl;
		return CMDRETCONT;
	}

	outstream << promise.serialized_rib << endl;

	return CMDRETCONT;
}

int
IPCMConsole::register_at_dif(vector<string>& args)
{
	int ipcp_id;
	Promise promise;

	if (args.size() < 3) {
		outstream << commands_map[args[0]].usage << endl;
		return CMDRETCONT;
	}

	rina::ApplicationProcessNamingInformation dif_name(args[2], string());

	if (string2int(args[1], ipcp_id)){
		outstream << "Invalid IPC process id" << endl;
		return CMDRETCONT;
	}

	if (!IPCManager->ipcp_exists(ipcp_id)) {
		outstream << "No such IPC process id" << endl;
		return CMDRETCONT;
	}

	if(IPCManager->register_at_dif(this, &promise, ipcp_id, dif_name) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		outstream << "Registration failed" << endl;
		return CMDRETCONT;
	}

	outstream << "IPC process registration completed successfully" << endl;

	return CMDRETCONT;
}


int
IPCMConsole::unregister_from_dif(std::vector<std::string>& args)
{
	int ipcp_id;
	Promise promise;

	if (args.size() < 3) {
		outstream << commands_map[args[0]].usage << endl;
		return CMDRETCONT;
	}

	rina::ApplicationProcessNamingInformation dif_name(args[2], string());

	if(string2int(args[1], ipcp_id)){
		outstream << "Invalid IPC process id" << endl;
		return CMDRETCONT;
	}
	if (!IPCManager->ipcp_exists(ipcp_id)){
		outstream << "No such IPC process id" << endl;
		return CMDRETCONT;
	}

	//Call IPCManager
	if(IPCManager->unregister_ipcp_from_ipcp(this, &promise, ipcp_id,
		dif_name) == IPCM_FAILURE || promise.wait() != IPCM_SUCCESS) {
		outstream << "Unregistration failed" << endl;
		return CMDRETCONT;
	}

	outstream << "IPC process unregistration completed "
			"successfully" << endl;

	return CMDRETCONT;
}

int
IPCMConsole::update_dif_config(std::vector<std::string>& args)
{
	rina::DIFConfiguration dif_config;
	Promise promise;
	int ipcp_id;

	if (args.size() < 2) {
		outstream << commands_map[args[0]].usage << endl;
		return CMDRETCONT;
	}

	if(string2int(args[1], ipcp_id)){
		outstream << "Invalid IPC process id" << endl;
		return CMDRETCONT;
	}

	if (!IPCManager->ipcp_exists(ipcp_id)) {
		outstream << "No such IPC process id" << endl;
		return CMDRETCONT;
	}

	if(IPCManager->update_dif_configuration(this, &promise, ipcp_id,
			dif_config) == IPCM_FAILURE || promise.wait() != IPCM_SUCCESS) {
		outstream << "Configuration update failed" << endl;
		return CMDRETCONT;
	}

	outstream << "NULL configuration updated successfully"<< endl;

	return CMDRETCONT;
}

int
IPCMConsole::enroll_to_dif(std::vector<std::string>& args)
{
	NeighborData neighbor_data;
	int ipcp_id;
	Promise promise;

	if (args.size() < 6) {
		outstream << commands_map[args[0]].usage << endl;
		return CMDRETCONT;
	}

	if(string2int(args[1], ipcp_id)){
		outstream << "Invalid IPC process id" << endl;
		return CMDRETCONT;
	}

	neighbor_data.difName = rina::ApplicationProcessNamingInformation(
				args[2], string());
	neighbor_data.supportingDifName =
		rina::ApplicationProcessNamingInformation(args[3], string());
	neighbor_data.apName =
		rina::ApplicationProcessNamingInformation(args[4], args[5]);

	if (!IPCManager->ipcp_exists(ipcp_id)) {
		outstream << "No such IPC process id" << endl;
		return CMDRETCONT;
	}

	if(IPCManager->enroll_to_dif(this, &promise, ipcp_id, neighbor_data) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		outstream << "Enrollment operation failed" << endl;
		return CMDRETCONT;
	}

	outstream << "DIF enrollment succesfully completed" << endl;

	return CMDRETCONT;
}

int
IPCMConsole::select_policy_set(std::vector<std::string>& args)
{
	int ipcp_id;
	Promise promise;

	if (args.size() < 4) {
		outstream << commands_map[args[0]].usage << endl;
		return CMDRETCONT;
	}

	if( string2int(args[1], ipcp_id) ){
		outstream << "Invalid IPC process id" << endl;
		return CMDRETCONT;
	}

	if (!IPCManager->ipcp_exists(ipcp_id)) {
		outstream << "No such IPC process id" << endl;
		return CMDRETCONT;
	}

	if(IPCManager->select_policy_set(this, &promise, ipcp_id, args[2],
			args[3]) == IPCM_FAILURE  || promise.wait() != IPCM_SUCCESS) {
		outstream << "select-policy-set operation failed" << endl;
		return CMDRETCONT;
	}

	outstream << "Policy-set selection succesfully completed" << endl;

	return CMDRETCONT;
}

int
IPCMConsole::set_policy_set_param(std::vector<std::string>& args)
{
	int ipcp_id;
	Promise promise;

	if (args.size() < 5) {
		outstream << commands_map[args[0]].usage << endl;
		return CMDRETCONT;
	}

	if(string2int(args[1], ipcp_id)){
		outstream << "Invalid IPC process id" << endl;
		return CMDRETCONT;
	}

	if (!IPCManager->ipcp_exists(ipcp_id)) {
		outstream << "No such IPC process id" << endl;
		return CMDRETCONT;
	}

	if(IPCManager->set_policy_set_param(this, &promise, ipcp_id, args[2],
			args[3], args[4]) == IPCM_FAILURE || promise.wait() != IPCM_SUCCESS) {
		outstream << "set-policy-set-param operation failed"<< endl;
		return CMDRETCONT;
	}

	outstream << "Policy-set parameter succesfully set"<< endl;

	return CMDRETCONT;
}

int
IPCMConsole::plugin_load_unload(std::vector<std::string>& args, bool load)
{
	int ipcp_id;
	Promise promise;

	if (args.size() < 3) {
		outstream << commands_map[args[0]].usage << endl;
		return CMDRETCONT;
	}

	if (string2int(args[1], ipcp_id)){
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

int
IPCMConsole::plugin_load(std::vector<std::string>& args)
{
	return plugin_load_unload(args, true);
}

int
IPCMConsole::plugin_unload(std::vector<std::string>& args)
{
	return plugin_load_unload(args, false);
}

int
IPCMConsole::plugin_get_info(std::vector<std::string>& args)
{
	if (args.size() < 2) {
		outstream << commands_map[args[0]].usage << endl;
		return CMDRETCONT;
	}

        const std::string& plugin_name = args[1];
	std::list<rina::PsInfo> policy_sets;

	if (IPCManager->plugin_get_info(plugin_name, policy_sets) == IPCM_FAILURE) {
		outstream << "Failed to get information about plugin "
                                << plugin_name << endl;
		return CMDRETCONT;
	}

	for (std::list<rina::PsInfo>::iterator lit = policy_sets.begin();
					lit != policy_sets.end(); lit++) {
                outstream << "Policy set: Name='" << lit->name <<
			     "', Component='" << lit->app_entity <<
			     "', Version='" << lit->version << "'" << endl;
	}

	return CMDRETCONT;
}

int IPCMConsole::show_dif_templates(std::vector<std::string>& args)
{
	if (args.size() != 1) {
		outstream << commands_map[args[0]].usage << endl;
		return CMDRETCONT;
	}

	std::list<DIFTemplate*> dif_templates =
			IPCManager->dif_template_manager->get_all_dif_templates();

	outstream<< "CURRENT DIF TEMPLATES:" << endl;

	std::list<DIFTemplate*>::iterator it;
	for (it = dif_templates.begin(); it != dif_templates.end(); ++it) {
		outstream << (*it)->toString() << endl;
	}

	return CMDRETCONT;
}

int IPCMConsole::read_ipcp_ribobj(std::vector<std::string>& args)
{
	Promise promise;
	int ipcp_id;
	int scope;

	if (args.size() != 5) {
		outstream << commands_map[args[0]].usage << endl;
		return CMDRETCONT;
	}

	if (string2int(args[1], ipcp_id)){
		outstream << "Invalid IPC process id" << endl;
		return CMDRETCONT;
	}

	if (string2int(args[4], scope)){
		outstream << "Invalid scope" << endl;
		return CMDRETCONT;
	}

	if (!IPCManager->ipcp_exists(ipcp_id)) {
		outstream << "No such IPC process id" << endl;
		return CMDRETCONT;
	}

	// CAREFUL, DELEGATION OBJECT SET TO NULL, this function is only
	// for testing, return result is not being processed.
	if (IPCManager->delegate_ipcp_ribobj(NULL,
					 &promise,
					 ipcp_id,
					 args[2],
					 args[3],
					 scope) == IPCM_FAILURE ||
					 promise.wait() != IPCM_SUCCESS) {
		outstream << "Error occured while forwarding CDAP message to IPCP" << endl;
	} else {
		outstream << "Successfully sent M_READ request "
			  << "with object class = " << args[2]
			  << " and object name = " << args[3] << endl;
	}

	return CMDRETCONT;
}

int IPCMConsole::show_catalog(std::vector<std::string>& args)
{
	switch (args.size()) {
	case 1:
		outstream << IPCManager->catalog.toString();
		break;

	case 2:
		outstream << IPCManager->catalog.toString(args[1]);
		break;

	default:
		outstream << commands_map[args[0]].usage << endl;
		break;
	}

	return CMDRETCONT;
}

int IPCMConsole::update_catalog(std::vector<std::string>& args)
{
	IPCManager->update_catalog(this);
	outstream << "Catalog updated" << endl;

	return CMDRETCONT;
}

}//namespace rinad
