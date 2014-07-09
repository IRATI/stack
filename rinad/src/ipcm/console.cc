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

#define RINA_PREFIX     "ipcm"

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/logs.h>

#include "rina-configuration.h"
#include "helpers.h"
#include "ipcm.h"

using namespace std;


namespace rinad {

static int
console_init()
{
        struct sockaddr_in server_address;
        int sfd = -1;
        int optval = 1;
        int ret;

        memset(&server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET;
        inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);
        server_address.sin_port = htons(32766);

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

void *
console_function(void *opaque)
{
        IPCManager *ipcm = static_cast<IPCManager *>(opaque);
        int sfd = console_init();

        if (sfd < 0) {
                return NULL;
        }

        cout << "Console starts: " << ipcm << endl;

        for (;;) {
                int cfd;
                struct sockaddr_in client_address;
                socklen_t address_len = sizeof(client_address);
                char buf[100];
                int n;

                cfd = accept(sfd, reinterpret_cast<struct sockaddr *>(
                             &client_address), &address_len);
                if (cfd < 0) {
                        cerr << __func__ << " Error [" << errno <<
                                "] calling accept() " << endl;
                        continue;
                }

                n = read(cfd, buf, sizeof(buf));
                if (n < 0) {
                        cerr << __func__ << " Error [" << errno <<
                                "] calling read() " << endl;
                        close(cfd);
                        continue;
                }

                n = write(cfd, buf, n);
                if (n < 0) {
                        cerr << __func__ << " Error [" << errno <<
                                "] calling write() " << endl;
                        close(cfd);
                        continue;
                }

                close(cfd);
        }

        cout << "Console stops" << endl;

        return NULL;
}

}
