//
// IPC Manager
//
//    Francesco Salvestrini <f.salvestrini@nextworks.it>
//    Vincenzo Maffione     <v.maffione@nextworks.it>
//    Sander Vrijders       <sander.vrijders@intec.ugent.be>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

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

	switch(signum){
		case SIGSEGV:
			LOG_CRIT("Got signal SIGSEGV");
			dump_backtrace();
			exit(EXIT_FAILURE);
		case SIGINT:
			rinad::IPCManager->stop();
			break;
		default:
			LOG_CRIT("Got unknown signal %d", signum);
			assert(0);
			break;
	}
}

int wrapped_main(int argc, char * argv[])
{
	std::string conf;
	std::string logfile;
	std::string loglevel;
	unsigned int wait_time;

	// Wrap everything in a try block.  Do this every time,
	// because exceptions will be thrown for problems.

	try {
		// Define the command line object.
		TCLAP::CmdLine cmd("IPC Manager", ' ', PACKAGE_VERSION);

		TCLAP::ValueArg<std::string>
			conf_arg("c",
				 "config",
				 "Configuration file to load",
				 true,
				 "ipcmanager.conf",
				 "string");
		TCLAP::ValueArg<std::string>
			loglevel_arg("l",
				     "loglevel",
				     "Log level",
				     false,
				     "INFO",
				     "string");
		TCLAP::ValueArg<unsigned int>
			wait_time_arg("w",
				     "wait-time",
				     "Maximum time (in seconds) to wait for an event response",
				     false,
				     10,
				     "unsigned int");

		cmd.add(conf_arg);
		cmd.add(loglevel_arg);
		cmd.add(wait_time_arg);

		// Parse the args.
		cmd.parse(argc, argv);

		// Get the value parsed by each arg.
		conf     = conf_arg.getValue();
		loglevel = loglevel_arg.getValue();
		wait_time = wait_time_arg.getValue();

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
	rinad::IPCManager->init(wait_time, loglevel);

	//Dump the config
	rinad::IPCManager->dumpConfig();

	//TODO make this configurable
	rinad::IPCManager->start_console_worker();
	rinad::IPCManager->start_script_worker();

	//Run the loop
	rinad::IPCManager->run();

	return EXIT_SUCCESS;
}


int main(int argc, char * argv[])
{
	int retval;

	//Configure signal  traps
	if (signal(SIGSEGV, handler) == SIG_ERR) {
		LOG_WARN("Could not install SIGSEGV handler!");
	}
	if (signal(SIGINT, handler) == SIG_ERR) {
		LOG_ERR("Could not install SIGINT handler!");
	}

	//Launch wrapped main
	try {
		retval = wrapped_main(argc, argv);
	} catch (std::exception & e) {
		LOG_ERR("Got unhandled exception (%s)", e.what());
		retval = EXIT_FAILURE;
	}

	return retval;
}
