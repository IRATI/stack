//
// Managment Agent DaemonIPC Manager
//
//	Marc Sune <marc.sune@bisdn.de>
//
//TODO: license

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>

#include <librina/common.h>
#include <librina/ipc-manager.h>
#define RINA_PREFIX "mad"
#include <librina/logs.h>


#include "common/event-loop.h"
#include "agent.h"

using namespace std;

int main(int argc, char * argv[])
{
	(void)argc;
	(void)argv;

	//TODO: Recover logfile loglevel
	std::string logfile, loglevel;

	try {
		//Initialize 
		rinad::mad::ManagementAgent::init(logfile, loglevel);
		
		//The mad is about to be stopped
		rinad::mad::ManagementAgent::destroy();
	} catch (std::exception & e) {
		LOG_ERR("Got unhandled exception (%s)", e.what());
		throw e;
	}

	return EXIT_SUCCESS;
}
