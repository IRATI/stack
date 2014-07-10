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

#define RINA_PREFIX     "ipcm"

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/logs.h>

#include "rina-configuration.h"
#include "helpers.h"
#include "ipcm.h"
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

IPCMConsole::IPCMConsole(IPCManager& r) :
                rina::Thread(new rina::ThreadAttributes(),
                             console_function, this),
                ipcm(r)
{
        commands_map["help"] = &IPCMConsole::help;
        commands_map["quit"] = &IPCMConsole::quit;
        commands_map["exit"] = &IPCMConsole::quit;
}

IPCMConsole::~IPCMConsole() throw()
{
}

int
IPCMConsole::init()
{
        struct sockaddr_in server_address;
        int sfd = -1;
        int optval = 1;
        int ret;

        memset(&server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET;
        inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);
        server_address.sin_port = htons(ipcm.config.local.consolePort);

        sfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sfd < 0) {
                cerr << __func__ << " Error [" << errno <<
                        "] calling socket() " << endl;
                goto err;
        }

        ret = setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR,
                         &optval, sizeof(optval));
        if (ret < 0) {
                cerr << __func__ << " Error [" << errno <<
                        "] calling setsockopt(SOL_SOCKET, "
                        "SO_REUSEADDR, 1)" << endl;
        }

        ret = bind(sfd, reinterpret_cast<struct sockaddr *>(&server_address),
                   sizeof(server_address));
        if (ret < 0) {
                cerr << __func__ << " Error [" << errno <<
                        "] calling bind() " << endl;
                goto err;
        }

        ret = listen(sfd, 5);
        if (ret < 0) {
                cerr << __func__ << " Error [" << errno <<
                        "] calling listen() " << endl;
                goto err;
        }

        return sfd;
err:
        if (sfd >= 0) {
                close(sfd);
        }

        return sfd;
}

void IPCMConsole::body()
{
        int sfd = init();

        if (sfd < 0) {
                return;
        }

        cout << "Console starts: " << sfd << endl;

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
                        cerr << __func__ << " Error [" << errno <<
                                "] calling accept() " << endl;
                        continue;
                }

                for (;;) {
                        outstream << "IPCM >>> ";
                        flush_output(cfd);

                        n = read(cfd, cmdbuf, sizeof(cmdbuf));
                        if (n < 0) {
                                cerr << __func__ << " Error [" << errno <<
                                        "] calling read() " << endl;
                                close(cfd);
                                continue;
                        }

                        cmdret = process_command(cfd, cmdbuf, n);
                        if (cmdret == CMDRETSTOP) {
                                close(cfd);
                                break;
                        }
                }
        }

        cout << "Console stops" << endl;

}

int
IPCMConsole::flush_output(int cfd)
{
        int n;
        const string& str = outstream.str();

        n = write(cfd, str.c_str(), str.size());
        if (n < 0) {
                cerr << __func__ << " Error [" << errno <<
                        "] calling write() " << endl;
                return -1;
        }

        // Make the stringstream empty
        outstream.str(string());

        return 0;
}

int
IPCMConsole::process_command(int cfd, char *cmdbuf, int size)
{
        map<string, ConsoleCmdFunction>::iterator mit;
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
                flush_output(cfd);
                return 0;
        }

        fun = mit->second;
        ret = (this->*fun)(args);

        outstream << endl;
        flush_output(cfd);

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

        outstream << "Available commands:" << endl;
        for (map<string, ConsoleCmdFunction>::iterator mit =
                commands_map.begin(); mit != commands_map.end(); mit++) {
                outstream << "    " << mit->first << endl;
        }

        return CMDRETCONT;
}

}
