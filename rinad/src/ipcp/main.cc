/*
 * IPC Process
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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
#include <sstream>

#define RINA_PREFIX "ipcp-main"

#include <librina/common.h>
#include <librina/logs.h>
#include "ipcp/ipc-process.h"

int main(int argc, char * argv[])
{
	if (argc != 5) {
		LOG_ERR("Wrong number of arguments: expected 5, got %d", argc);
		return EXIT_FAILURE;
	}

	rina::ApplicationProcessNamingInformation name(argv[1], argv[2]);
	LOG_INFO("IPC Process name: %s; IPC Process instance: %s", argv[1], argv[2]);
	unsigned short ipcp_id = 0;
	unsigned int ipcm_port = 0;

	std::stringstream ss(argv[3]);
	if (! (ss >> ipcp_id)) {
		LOG_ERR("Problems converting string to unsigned short");
		return EXIT_FAILURE;
	}
	LOG_INFO("IPC Process id: %u", ipcp_id);

	std::stringstream ss2(argv[4]);
	if (! (ss2 >> ipcm_port)) {
		LOG_ERR("Problems converting string to unsigned int");
		return EXIT_FAILURE;
	}
	LOG_INFO("IPC Manager port: %u", ipcm_port);

	rinad::IPCProcessImpl ipcp(name, ipcp_id, ipcm_port);
	rinad::EventLoop loop(&ipcp);

	rinad::register_handlers_all(loop);

	loop.run();

	return EXIT_SUCCESS;
}
