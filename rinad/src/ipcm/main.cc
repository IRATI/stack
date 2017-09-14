//
// IPC Manager
//
//    Francesco Salvestrini <f.salvestrini@nextworks.it>
//    Vincenzo Maffione     <v.maffione@nextworks.it>
//    Sander Vrijders       <sander.vrijders@intec.ugent.be>
//    Marc Sune             <marc.sune (at) bisdn.de>
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
#include <sys/wait.h>
#include <assert.h>
#include <signal.h>

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>

#define RINA_PREFIX "ipcm"
#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/logs.h>


#include "common/rina-configuration.h"
#include "common/debug.h"
#include "tclap/CmdLine.h"
#include "ipcm.h"
#include "configuration.h"

using namespace std;
using namespace TCLAP;


void handler(int signum)
{
	int pid, saved_errno;

	switch(signum){
		case SIGSEGV:
			LOG_CRIT("Got signal SIGSEGV");
			dump_backtrace();
			rinad::IPCManager->stop();
			rina::librina_finalize();
			exit(EXIT_FAILURE);
		case SIGINT:
		case SIGQUIT:
		case SIGTERM:
		case SIGHUP:
			rinad::IPCManager->stop();
			break;
		case SIGCHLD:
			saved_errno = errno;
			while (pid = waitpid(WAIT_ANY, NULL, WNOHANG), pid > 0) {
				LOG_DBG("Child IPC Process Daemon %d died, removed from process table",
					pid);
			}
			errno = saved_errno;
			break;
		default:
			LOG_CRIT("Got unknown signal %d", signum);
			assert(0);
			break;
	}
}

int wrapped_main(int argc, char * argv[])
{
	std::string addons;
	std::string conf;
	std::string logfile;
	std::string loglevel;
	bool dump;

	// Wrap everything in a try block.  Do this every time,
	// because exceptions will be thrown for problems.

	try {
		// Define the command line object.
		TCLAP::CmdLine cmd("IPC Manager", ' ', PACKAGE_VERSION);

		TCLAP::ValueArg<std::string>
			addons_arg("a",
				 "addons",
				 "Load listed addons; default \"console, scripting\"",
				 false,
				 "console, scripting",
				 "string");

		TCLAP::ValueArg<std::string>
			conf_arg("c",
				 "config",
				 "Configuration file to load; default \""
				  + std::string(__INSTALLATION_PREFIX)
				  + "/etc/ipcmanager.conf\"",
				 false,
				 std::string(__INSTALLATION_PREFIX)
				  + "/etc/ipcmanager.conf",
				 "string");
		TCLAP::ValueArg<std::string>
			loglevel_arg("l",
				     "loglevel",
				     "Log level",
				     false,
				     "INFO",
				     "string");
		TCLAP::SwitchArg
			dump_arg("d",
				     "dump",
				     "Dump configuration on startup",
				     false);

		cmd.add(addons_arg);
		cmd.add(conf_arg);
		cmd.add(loglevel_arg);
		cmd.add(dump_arg);

		// Parse the args.
		cmd.parse(argc, argv);

		// Get the value parsed by each arg.
		addons = addons_arg.getValue();
		conf     = conf_arg.getValue();
		loglevel = loglevel_arg.getValue();
		dump = dump_arg.getValue();

		LOG_DBG("Config file is: %s", conf.c_str());

	} catch (ArgException &e) {
		LOG_ERR("Error: %s for arg %d",
			e.error().c_str(),
			e.argId().c_str());
		return EXIT_FAILURE;
	}

	//Parse configuration file
	if (!rinad::parse_configuration(conf)) {
		LOG_ERR("Failed to load configuration");
		return EXIT_FAILURE;
	}

	//Initialize IPCM
	rinad::IPCManager->init(loglevel, conf);

	//Dump the config
	if(dump)
		rinad::IPCManager->dumpConfig();

	//Load addons
	rinad::IPCManager->load_addons(addons);

	//Run the loop
	rinad::IPCManager->run();

	return EXIT_SUCCESS;
}

int main(int argc, char * argv[])
{
	int retval;

	if(geteuid() != 0){
		fprintf(stderr, "\nERROR: Root permissions are required to run %s\n",
								RINA_PREFIX);
		exit(EXIT_FAILURE);
	}

	//Configure signal  traps
	if (signal(SIGSEGV, handler) == SIG_ERR) {
		LOG_ERR("Could not install SIGSEGV handler!");
		return EXIT_FAILURE;
	}
        LOG_DBG("SIGSEGV handler installed successfully");

	if (signal(SIGINT, handler) == SIG_ERR) {
		LOG_ERR("Could not install SIGINT handler!");
		return EXIT_FAILURE;
	}
        LOG_DBG("SIGINT handler installed successfully");

	if (signal(SIGQUIT, handler) == SIG_ERR) {
		LOG_ERR("Could not install SIGQUIT handler!");
		return EXIT_FAILURE;
	}
        LOG_DBG("SIGQUIT handler installed successfully");

	if (signal(SIGTERM, handler) == SIG_ERR) {
		LOG_ERR("Could not install SIGTERM handler!");
		return EXIT_FAILURE;
	}
        LOG_DBG("SIGTERM handler installed successfully");

	if (signal(SIGHUP, handler) == SIG_ERR) {
		LOG_ERR("Could not install SIGHUP handler!");
		return EXIT_FAILURE;
	}
        LOG_DBG("SIGHUP handler installed successfully");

	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
                LOG_WARN("Cannot ignore SIGPIPE, bailing out");
                return EXIT_FAILURE;
        }
        LOG_DBG("SIGPIPE handler installed successfully");

	if (signal(SIGCHLD, handler) == SIG_ERR) {
		LOG_ERR("Could not install SIGCHLD handler!");
		return EXIT_FAILURE;
	}
        LOG_DBG("SIGCHLD handler installed successfully");

	//Launch wrapped main
	try {
		retval = wrapped_main(argc, argv);
	} catch (std::exception & e) {
		LOG_ERR("Got unhandled exception (%s)", e.what());
		retval = EXIT_FAILURE;
	}

	return retval;
}
