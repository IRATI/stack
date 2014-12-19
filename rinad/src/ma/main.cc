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

#include "tclap/CmdLine.h"
#include "common/event-loop.h"
#include "agent.h"

#define DEFAULT_LOG_FILE ""
#define DEFAULT_LOG_LEVEL "INFO"
#define DEFAULT_CONF_FILE "/etc/rina/mad.conf"


using namespace std;

static
void parse_args(int argc, char * argv[], std::string& conf,
					 std::string& logfile,
					 std::string& loglevel){

        try {
                // Define the command line object.
                TCLAP::CmdLine cmd("Management Agent Daemon",
							' ', PACKAGE_VERSION);

                TCLAP::ValueArg<std::string>
                        conf_arg("c",
                                 "config",
                                 "Configuration file to be loaded",
                                 false,
                                 DEFAULT_CONF_FILE,
                                 "string");
                cmd.add(conf_arg);

		TCLAP::ValueArg<std::string>
                        logfile_arg("f",
                                    "logfile",
                                    "Log file",
                                    false,
                                    DEFAULT_LOG_FILE,
                                    "string");
                cmd.add(logfile_arg);

                TCLAP::ValueArg<std::string>
                        loglevel_arg("l",
                                     "loglevel",
                                     "Log level",
                                     false,
                                     DEFAULT_LOG_LEVEL,
                                     "string");
                cmd.add(loglevel_arg);
		/*
                TCLAP::ValueArg<unsigned int>
                        wait_time_arg("w",
                                     "wait-time",
                                     "Maximum time (in seconds) to wait for an event response",
                                     false,
                                     10,
                                     "unsigned int");
                cmd.add(wait_time_arg);
		*/

                // Parse the args.
                cmd.parse(argc, argv);

                // Get the value parsed by each arg.
                conf     = conf_arg.getValue();
                logfile  = logfile_arg.getValue();
                loglevel = loglevel_arg.getValue();
                //wait_time = wait_time_arg.getValue();

                LOG_DBG("Opening config file: %s", conf.c_str());

        } catch (TCLAP::ArgException &e) {
                LOG_ERR("Error: %s for arg %d",
                        e.error().c_str(),
                        e.argId().c_str());
			exit(EXIT_FAILURE);
        }
}

int main(int argc, char * argv[])
{
	//Recover initial basic parameters
	std::string logfile, loglevel, conf;
	
	//Parse arguments
	parse_args(argc, argv, conf, logfile, loglevel);

	try {
		//Initialize Agent class
		rinad::mad::ManagementAgent::init(conf, logfile, loglevel);

		//Destroy agent and the rest of subsystems
		rinad::mad::ManagementAgent::destroy();
	} catch (std::exception & e) {
		LOG_ERR("Got unhandled exception (%s)", e.what());
		throw e;
	}


	return EXIT_SUCCESS;
}
