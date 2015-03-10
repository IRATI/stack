/*
 * IPC Manager console
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
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <sstream>

#define RINA_PREFIX "ipcm.console"
#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/logs.h>

#include "rina-configuration.h"
#include "helpers.h"
#include "../ipcm.h"
#include "console.h"

using namespace std;


namespace rinad {

void *
console_function(void *opaque)
{
        IPCMConsole *console = static_cast<IPCMConsole *>(opaque);

        console->body();

        return NULL;
}

IPCMConsole::IPCMConsole(rina::ThreadAttributes &ta,
					const unsigned int port_) :
                port(port_)
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
                                "<dif-name>");
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

        worker = new rina::Thread(&ta, console_function, this);
}

IPCMConsole::~IPCMConsole() throw()
{
        delete worker;
}

//TODO use unix sockets!
int
IPCMConsole::init()
{
        ostringstream ss;
        struct sockaddr_in server_address;
        int sfd = -1;
        int optval = 1;
        int ret;

        memset(&server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET;
        inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);
        server_address.sin_port = htons(port);

        try {
                sfd = socket(AF_INET, SOCK_STREAM, 0);
                if (sfd < 0) {
                        ss  << " Error [" << errno <<
                                "] calling socket() " << endl;
                        throw Exception();
                }

                ret = setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR,
                                &optval, sizeof(optval));
                if (ret < 0) {
                        ss  << " Error [" << errno <<
                                "] calling setsockopt(SOL_SOCKET, "
                                "SO_REUSEADDR, 1)" << endl;
                        FLUSH_LOG(WARN, ss);
                }

                ret = bind(sfd, reinterpret_cast<struct sockaddr *>(&server_address),
                                sizeof(server_address));
                if (ret < 0) {
                        ss  << " Error [" << errno <<
                                "] calling bind() " << endl;
                        throw Exception();
                }

                ret = listen(sfd, 5);
                if (ret < 0) {
                        ss  << " Error [" << errno <<
                                "] calling listen() " << endl;
                        throw Exception();
                }
        } catch (Exception) {
                FLUSH_LOG(ERR, ss);
                if (sfd >= 0) {
                        close(sfd);
                        sfd = -1;
                }
        }

        return sfd;
}

void IPCMConsole::body()
{
        ostringstream ss;
        int sfd = init();

        if (sfd < 0) {
                return;
        }

        LOG_DBG("Console starts [fd=%d]\n", sfd);

        for (;;) {
                int cfd;
                struct sockaddr_in client_address;
                socklen_t address_len = sizeof(client_address);
                char cmdbuf[CMDBUFSIZE];
                int cmdret;
                int n;

                cfd = accept(sfd, reinterpret_cast<struct sockaddr *>(
                             &client_address), &address_len);
                if (cfd < 0) {
                        ss  << " Error [" << errno <<
                                "] calling accept() " << endl;
                        FLUSH_LOG(ERR, ss);
                        continue;
                }

                for (;;) {
                        outstream << "IPCM >>> ";
                        if (flush_output(cfd)) {
                        	close(cfd);
                        	break;
                        }

                        n = read(cfd, cmdbuf, sizeof(cmdbuf));
                        if (n < 0) {
                                ss  << " Error [" << errno <<
                                        "] calling read() " << endl;
                                FLUSH_LOG(ERR, ss);
                                close(cfd);
                                break;
                        }

                        cmdret = process_command(cfd, cmdbuf, n);
                        if (cmdret == CMDRETSTOP) {
                                close(cfd);
                                break;
                        }
                }
        }

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
        (void) args;

        return CMDRETSTOP;
}

int IPCMConsole::help(vector<string>& args)
{
        (void) args;

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
        if (args.size() < 4) {
                outstream << commands_map[args[0]].usage << endl;
                return CMDRETCONT;
        }

        rina::ApplicationProcessNamingInformation ipcp_name(args[1], args[2]);

        int ipcp_id = IPCManager->create_ipcp(ipcp_name, args[3]);
        if (ipcp_id < 0) {
                outstream << "Error while creating IPC process" << endl;
        } else {
                outstream << "IPC process created successfully [id = "
                              << ipcp_id << ", name="<<args[1]<<"]" << endl;
        }

        return CMDRETCONT;
}

int
IPCMConsole::destroy_ipcp(vector<string>& args)
{
        int ipcp_id;
        int ret;

        if (args.size() < 2) {
                outstream << commands_map[args[0]].usage << endl;
                return CMDRETCONT;
        }

        ret = string2int(args[1], ipcp_id);
        if (ret) {
                outstream << "Invalid IPC process id" << endl;
                return CMDRETCONT;
        }

        ret = IPCManager->destroy_ipcp(ipcp_id);
        if (ret) {
                outstream << "Destroy operation failed" << endl;
        } else {
                outstream << "IPC process successfully destroyed" << endl;
        }

        return CMDRETCONT;
}

int
IPCMConsole::list_ipcps(vector<string>&args)
{
        (void) args;

        IPCManager->list_ipcps(outstream);

        return CMDRETCONT;
}

int
IPCMConsole::list_ipcp_types(std::vector<std::string>& args)
{
        (void) args;

        IPCManager->list_ipcp_types(outstream);

        return CMDRETCONT;
}

int
IPCMConsole::assign_to_dif(std::vector<string>& args)
{
        int ipcp_id;
        int ret;

        if (args.size() < 3) {
                outstream << commands_map[args[0]].usage << endl;
                return CMDRETCONT;
        }

        rina::ApplicationProcessNamingInformation dif_name(args[2], string());

        ret = string2int(args[1], ipcp_id);
        if (ret) {
                outstream << "Invalid IPC process id" << endl;
                return CMDRETCONT;
        }

        if (!IPCManager->ipcp_exists(ipcp_id)) {
                outstream << "No such IPC process id" << endl;
        } else {
                ret = IPCManager->assign_to_dif(ipcp_id, dif_name);
                if (ret) {
                        outstream << "DIF assignment failed" << endl;
                } else {
                        outstream << "DIF assignment completed successfully"
                                        << endl;
                }
        }

        return CMDRETCONT;
}

int
IPCMConsole::query_rib(std::vector<string>& args)
{
        int ipcp_id;
        int ret;

        if (args.size() < 2) {
                outstream << commands_map[args[0]].usage << endl;
                return CMDRETCONT;
        }

        ret = string2int(args[1], ipcp_id);
        if (ret) {
                outstream << "Invalid IPC process id" << endl;
                return CMDRETCONT;
        }

        if (!IPCManager->ipcp_exists(ipcp_id)) {
                outstream << "No such IPC process id" << endl;
        } else {
        		outstream << IPCManager->query_rib(ipcp_id) << endl;
        }

        return CMDRETCONT;
}

int
IPCMConsole::register_at_dif(vector<string>& args)
{
        int ipcp_id;
        int ret;

        if (args.size() < 3) {
                outstream << commands_map[args[0]].usage << endl;
                return CMDRETCONT;
        }

        rina::ApplicationProcessNamingInformation dif_name(args[2], string());

        ret = string2int(args[1], ipcp_id);
        if (ret) {
                outstream << "Invalid IPC process id" << endl;
                return CMDRETCONT;
        }

        if (!IPCManager->ipcp_exists(ipcp_id)) {
                outstream << "No such IPC process id" << endl;
        } else {
                ret = IPCManager->register_at_dif(ipcp_id, dif_name);
                if (ret) {
                        outstream << "Registration failed" << endl;
                } else {
                        outstream << "IPC process registration completed "
                                        "successfully" << endl;
                }
        }

        return CMDRETCONT;
}


int
IPCMConsole::unregister_from_dif(std::vector<std::string>& args)
{
        int ipcp_id, slave_ipcp_id;
        int ret;

        if (args.size() < 3) {
                outstream << commands_map[args[0]].usage << endl;
                return CMDRETCONT;
        }

        std::string dif_name(args[2]);

        ret = string2int(args[1], ipcp_id);
        if (ret) {
                outstream << "Invalid IPC process id" << endl;
                return CMDRETCONT;
        }
        if (!IPCManager->ipcp_exists(ipcp_id)){
                outstream << "No such IPC process id" << endl;
                return CMDRETCONT;
	}

        slave_ipcp_id = IPCManager->get_ipcp_by_dif_name(dif_name);
	if (!IPCManager->ipcp_exists(slave_ipcp_id) ) {
                outstream << "No IPC process in that DIF" << endl;
                return CMDRETCONT;
        }

	//Call IPCManager
	ret = IPCManager->unregister_ipcp_from_ipcp(ipcp_id,
							slave_ipcp_id);
	if (ret) {
		outstream << "Unregistration failed" << endl;
	} else {
		outstream << "IPC process unregistration completed "
			"successfully" << endl;
	}

        return CMDRETCONT;
}

int
IPCMConsole::update_dif_config(std::vector<std::string>& args)
{
        rina::DIFConfiguration dif_config;
        int ipcp_id;
        int ret;

        if (args.size() < 2) {
                outstream << commands_map[args[0]].usage << endl;
                return CMDRETCONT;
        }

        ret = string2int(args[1], ipcp_id);
        if (ret) {
                outstream << "Invalid IPC process id" << endl;
                return CMDRETCONT;
        }

        if (!IPCManager->ipcp_exists(ipcp_id)) {
                outstream << "No such IPC process id" << endl;
        } else {
                ret = IPCManager->update_dif_configuration(ipcp_id, dif_config);
                if (ret) {
                        outstream << "Configuration update failed" << endl;
                } else {
                        outstream << "NULL configuration updated successfully"
                                        << endl;
                }
        }

        return CMDRETCONT;
}

int
IPCMConsole::enroll_to_dif(std::vector<std::string>& args)
{
        NeighborData neighbor_data;
        int ipcp_id;
        int ret;

        if (args.size() < 6) {
                outstream << commands_map[args[0]].usage << endl;
                return CMDRETCONT;
        }

        ret = string2int(args[1], ipcp_id);
        if (ret) {
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
        } else {
                ret = IPCManager->enroll_to_dif(ipcp_id, neighbor_data, true);
                if (ret) {
                        outstream << "Enrollment operation failed" << endl;
                } else {
                        outstream << "DIF enrollment succesfully completed" << endl;
                }
        }

        return CMDRETCONT;
}

int
IPCMConsole::select_policy_set(std::vector<std::string>& args)
{
        int ipcp_id;
        int ret;

        if (args.size() < 4) {
                outstream << commands_map[args[0]].usage << endl;
                return CMDRETCONT;
        }

        ret = string2int(args[1], ipcp_id);
        if (ret) {
                outstream << "Invalid IPC process id" << endl;
                return CMDRETCONT;
        }

        if (!IPCManager->ipcp_exists(ipcp_id)) {
                outstream << "No such IPC process id" << endl;
        } else {
                ret = IPCManager->select_policy_set(ipcp_id, args[2], args[3]);
                if (ret) {
                        outstream << "select-policy-set operation failed"
                                        << endl;
                } else {
                        outstream << "Policy-set selection succesfully "
                                        "completed" << endl;
                }
        }

        return CMDRETCONT;
}

int
IPCMConsole::set_policy_set_param(std::vector<std::string>& args)
{
        int ipcp_id;
        int ret;

        if (args.size() < 5) {
                outstream << commands_map[args[0]].usage << endl;
                return CMDRETCONT;
        }

        ret = string2int(args[1], ipcp_id);
        if (ret) {
                outstream << "Invalid IPC process id" << endl;
                return CMDRETCONT;
        }

        if (!IPCManager->ipcp_exists(ipcp_id)) {
                outstream << "No such IPC process id" << endl;
        } else {
                ret = IPCManager->set_policy_set_param(ipcp_id, args[2],
								args[3],
        	                                        	args[4]);
                if (ret) {
                        outstream << "set-policy-set-param operation failed"
                                        << endl;
                } else {
                        outstream << "Policy-set parameter succesfully set"
                                        << endl;
                }
        }

        return CMDRETCONT;
}

int
IPCMConsole::plugin_load_unload(std::vector<std::string>& args, bool load)
{
        int ipcp_id;
        int ret;

        if (args.size() < 3) {
                outstream << commands_map[args[0]].usage << endl;
                return CMDRETCONT;
        }

        ret = string2int(args[1], ipcp_id);
        if (ret) {
                outstream << "Invalid IPC process id" << endl;
                return CMDRETCONT;
        }

        if (!IPCManager->ipcp_exists(ipcp_id)) {
                outstream << "No such IPC process id" << endl;
        } else {
                string un;

                if (!load) {
                        un = "un";
                }

                ret = IPCManager->plugin_load(ipcp_id, args[2], load);
                if (ret) {
                        outstream << "Plugin " << un <<
                                "loading failed" << endl;
                } else {
                        outstream << "Plugin " << un <<
                                "loaded succesfully" << endl;
                }
        }

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

}//namespace rinad
