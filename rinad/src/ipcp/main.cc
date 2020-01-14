//
// IPC Process
//
//    Francesco Salvestrini <f.salvestrini@nextworks.it>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301  USA
//

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include <cstdlib>
#include <sstream>

//No ipcp module
#include "ipcp-logging.h"

#include <librina/common.h>
#include "ipcp/ipc-process.h"
#include "common/debug.h"

//IPCP id
int ipcp_id;

class Main{
public:
        int wrapped_main(int argc, char * argv[]);
        static rinad::AbstractIPCProcessImpl* ipcp;
};

rinad::AbstractIPCProcessImpl* Main::ipcp;

int Main::wrapped_main(int argc, char * argv[])
{
	std::string install_dir = argv[0];
	std::string log_level = argv[5];
	std::string log_file = argv[6];
	std::string ipcp_type = argv[7];

	rina::ApplicationProcessNamingInformation name(argv[1], argv[2]);

	unsigned int   ipcm_port = 0;

	std::stringstream ss2(argv[4]);
	if (! (ss2 >> ipcm_port)) {
		LOG_IPCP_ERR("Problems converting string to unsigned int");
		return EXIT_FAILURE;
	}

	ipcp = rinad::IPCPFactory::createIPCP(ipcp_type, name, ipcp_id,
				ipcm_port, log_level, log_file, install_dir);
	if (!ipcp) {
		LOG_IPCP_ERR("Problems creating IPCP");
		return EXIT_FAILURE;
	}

	LOG_IPCP_INFO("IPC Process name:     %s", argv[1]);
	LOG_IPCP_INFO("IPC Process instance: %s", argv[2]);
	LOG_IPCP_INFO("IPC Process id:       %u", ipcp_id);
	LOG_IPCP_INFO("IPC Manager port:     %u", ipcm_port);
	LOG_IPCP_INFO("IPC Process type:     %s", ipcp_type.c_str());

	LOG_IPCP_INFO("IPC Process initialized, executing event loop...");

	try {
		ipcp->event_loop();
	} catch (...) {
		LOG_IPCP_ERR("Unhandled exception!!!");
	}

	LOG_IPCP_DBG("Exited event loop");

	rinad::IPCPFactory::destroyIPCP(ipcp_type);

	return EXIT_SUCCESS;
}

void sighandler_segv(int signum)
{
	LOG_IPCP_CRIT("Got signal %d", signum);

	if (signum == SIGSEGV) {
		dump_backtrace();
		rina::librina_finalize();
		exit(EXIT_FAILURE);
	}
}

void sighandler_sigint(int signum)
{
        LOG_IPCP_CRIT("Got signal %d", signum);

        if (signum == SIGINT && Main::ipcp->keep_running) {
                Main::ipcp->keep_running = false;
                rina::librina_finalize();
        } else {
        	LOG_DBG("Ignoring signal, IPCP is already exiting");
        }
}


int main(int argc, char * argv[])
{
	int retval;
	Main main;

	//Check first things first
	if(geteuid() != 0){
		fprintf(stderr, "\nERROR: Root permissions are required to run an IPCP\n");
		exit(EXIT_FAILURE);
	}

	if (argc != 8) {
		LOG_IPCP_ERR("Wrong number of arguments: expected 8, got %d", argc);
		return EXIT_FAILURE;
	}

	//Parse ipcp-id asap
	std::stringstream ss(argv[3]);
	if (! (ss >> ipcp_id)) {
		LOG_IPCP_ERR("Problems converting string to unsigned short");
		return EXIT_FAILURE;
	}

	if (signal(SIGSEGV, sighandler_segv) == SIG_ERR)
		LOG_IPCP_WARN("Cannot install SIGSEGV handler!");

	LOG_IPCP_DBG("SIGSEGV handler installed successfully");

	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR){
		LOG_IPCP_WARN("Cannot ignore SIGPIPE, bailing out");
		return EXIT_FAILURE;
	}
	LOG_IPCP_DBG("SIGPIPE handler installed successfully");

	if (signal(SIGINT, sighandler_sigint) == SIG_ERR)
		LOG_IPCP_WARN("Cannot ignore SIGINT!");

	LOG_IPCP_DBG("SIGINT handler installed successfully");


	try {
		retval = main.wrapped_main(argc, argv);
	} catch (std::exception & e) {
		LOG_IPCP_ERR("Got unhandled exception (%s)", e.what());
		rina::librina_finalize();
		retval = EXIT_FAILURE;
	}

	return retval;
}
