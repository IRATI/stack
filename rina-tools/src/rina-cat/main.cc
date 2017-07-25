//
//
// RINA 'cat' (reuses some code from Echo Server by Addy Bombeke <addy.bombeke@ugent.be>
// and Vincenzo Maffione <v.maffione@nextworks.it>)
// (C) 2017 Steve Bunch <srb@trianetworksystems.com>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//   1. Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//   2. Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
//

#include <cstdlib>
#include <string>
#include <cerrno>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include <tclap/CmdLine.h>

#include "rina/api.h"
// #include "api.h"

#include "cat_client.h"
#include "cat_server.h"

#include <unistd.h>	// for gethostname
#include <libgen.h>	// for basename

#define NON_PERSISTENT 0	// value that indicates to do the command once, then terminate, >0 == persist
#define MAX_SERVERS    100	// arbitrary limit on number of server processes we'll launch simultaneously

using namespace std;

int wrapped_main(int argc, char** argv)
{
	bool listen;
	int verbose = 0;
	int repetitions;
	string this_apn;
	string this_api;
	string other_apn;
	string other_api;
	string dif_name_arg;
	string command;
	int sdusize;	// SDU size to use if data pumping - has no effect on exec'ed commands
	char hname[128];			// return area for (possibly truncated) host name
	string default_this_apn;	// name of this app, in case one is not supplied on the command line
	int cfd;	// rina api open return result - control file fd
	
	// compute a name for this app, in case none is supplied:  hostname.commandname or commandname
	if (gethostname(hname,sizeof(hname)) < 0) {
		default_this_apn = basename(argv[0]); // in spec, basename may modify argv string; we assume not.
	} else {
		hname[sizeof(hname)-1] = '\0';	// termination of gethostname unspecified if hostname is truncated
		default_this_apn = string(hname) + "." + string(basename(argv[0]));
	}
	
	try {
		TCLAP::CmdLine cmd("rina-cat", ' ', "alpha 7/21/2017");
		TCLAP::SwitchArg listen_arg("l",
									"listen",
									"Register this app, run in server mode awaiting incoming flow request(s)",
									false);
		TCLAP::SwitchArg verbose_arg("v",
									 "verbose",
									 "increase debug and informational output",
									 false);
		TCLAP::SwitchArg very_verbose_arg("V",
										  "very-verbose",
										  "maximize debug and informational output",
										  false);
		TCLAP::ValueArg<int> repeats_arg ("p",
										  "persist-servers",
										  "Run persistently, up to this many simultaneous commands - server requests over this number will be refused",
										  false,
										  NON_PERSISTENT,
										  "integer");
		TCLAP::ValueArg<string> command_arg("c",
											 "command",
											 "Command to execute to process the flow",
											 false,
											 "",
											 "string");
		TCLAP::ValueArg<string> this_apn_arg("A",
											 "this-apn",
											 "Application process name for this application",
											 false,
											 default_this_apn,
											 "string");
		TCLAP::ValueArg<string> this_api_arg("I",
											 "this-api",
											 "Application process instance for this application",
											 false,
											 "",
											 "string");
		TCLAP::ValueArg<string> other_apn_arg("a",
											  "other-apn",
											  "Application process name for the other end",
											  false,
											  "",
											  "string");
		TCLAP::ValueArg<string> other_api_arg("i",
											  "other-api",
											  "Application process instance for the other end",
											  false,
											  "",
											  "string");
		TCLAP::ValueArg<string> dif_arg("d",
										"dif",
										"The name of the DIF to use or register at (may be ignored)",
										false,
										"",
										"string");
		TCLAP::ValueArg<int> sdusize_arg("",
									  "sdusize",
									  "Default size for read/write transfers on RINA flow (ignored by exec'ed commands)",
									  false,
									  4096,
									  "integer");
		
		cmd.add(sdusize_arg);
		cmd.add(dif_arg);
		cmd.add(other_api_arg);
		cmd.add(this_api_arg);
		cmd.add(very_verbose_arg);
		cmd.add(verbose_arg);
		cmd.add(repeats_arg);
		cmd.add(command_arg);
		cmd.add(this_apn_arg);
		cmd.add(other_apn_arg);
		cmd.add(listen_arg);
		
		cmd.parse(argc, argv);
		
		listen = listen_arg.getValue();
		verbose = (verbose_arg.getValue() ? 1 : 0);
		if (very_verbose_arg.getValue())
			verbose = 2;
		this_apn = this_apn_arg.getValue();
		this_api = this_api_arg.getValue();
		other_apn = other_apn_arg.getValue();
		other_api = other_api_arg.getValue();
		repetitions = repeats_arg.getValue();
		dif_name_arg = dif_arg.getValue().c_str();
		sdusize = sdusize_arg.getValue();
		command = command_arg.getValue();
		
		// rina/api.h convention for apnames: apname | app instance | AEname | AEinstance
		// Allow user to specify the instances explicitly and we'll manufacture that, or they cn
		// supply the apn per the api and omit the instances and we'll just use that.
		if (!other_api.empty()) {
			other_apn = other_apn + "|" + other_api;
		}
		if (!this_api.empty()) {
			this_apn = this_apn + "|" + this_api;
		}
		
	} catch (TCLAP::ArgException &e) {
		cerr << "Error: " << e.error().c_str() << " for arg " << e.argId().c_str() << endl;
		return EXIT_FAILURE;
	}
	
	if (verbose) {
		cerr << "Arguments:" << endl;
		cerr << "    Verbosity: " << verbose << endl;
		cerr << "    Default appname(if none was supplied using -A): " << default_this_apn << endl;
		cerr << "    Listen: " << listen << endl;
		cerr << "    persist-servers: " << repetitions << endl;
		cerr << "    This app name: " << this_apn << endl;
		cerr << "    Other app name: " << other_apn << endl;
		cerr << "    DIF names: " << dif_name_arg << endl;
		cerr << "    sdusize: " << sdusize << endl;
		cerr << "    Command: \"" << command << "\"" << endl;
	}
	if ((!listen && repetitions > 0) || (listen && (repetitions < 0 || repetitions > MAX_SERVERS)))
		cerr << "Attempt to set persist-servers to " << repetitions << " ignored; must be listening, allowed range 0 to " << MAX_SERVERS << endl;
	
	if (!listen && other_apn.empty()) {
		cerr << "In client mode (no -l option), you must supply a remote application name (-a) argument" << endl;
		return (EXIT_FAILURE);
	}
	
	/*
	 * Set up for executing a client or server command.  It will execute the -c string command
	 * using the current $SHELL, or /bin/sh if empty, with -c.  Environment variables will be set
	 * to allow the command access to the command line options and incoming app name.
	 */
#define NUM_ARGS 4
	int command_argc = NUM_ARGS - 1;
	char str[20];
	const char *command_argv[NUM_ARGS] = { getenv("SHELL"), "-c", command.c_str(), NULL };
	if (command.empty())
		command_argc = 0;
	else {
		setenv("RINACAT_A", this_apn.c_str(), 1);
		setenv("RINACAT_a", other_apn.c_str(), 1);
		setenv("RINACAT_d", dif_name_arg.c_str(), 1);
		sprintf(str, "%d", sdusize);
		setenv("RINACAT_sdusize", str, 1);
		setenv("RINACAT_l", (listen ? "l" : ""), 1);
		sprintf(str, "%d", verbose);
		setenv("RINACAT_v", str, 1);
	}
	
	/* Ready to run the RINA connectivity portion */
	cfd = rina_open ();
	if (cfd < 0) {
		cerr << "Unable to reach RINA infrastructure.  error '" << strerror(errno) << "'" << endl;
		return (EXIT_FAILURE);
	} else {
		if (verbose)
			cerr << "Established control fd " << cfd << endl;
	}
	
	if (listen) {
		// Server mode
		if (verbose)
			cerr << "Running in Server mode" << endl;
		CatServer server (cfd, command_argc, command_argv, repetitions, verbose,
						  this_apn.c_str(), dif_name_arg.c_str(),
						  other_apn.c_str(), sdusize);
		return(server.run());
		
	} else {
		// Client mode
		if (verbose)
			cerr << "Running in Client mode" << endl;
		CatClient client (cfd, command_argc, command_argv, verbose,
						  this_apn.c_str(), other_apn.c_str(),
						  dif_name_arg.c_str(), sdusize);
		return (client.run());
	}
}

int main(int argc, char * argv[])
{
        int retval;

        try {
                retval = wrapped_main(argc, argv);
        } catch (std::exception& e) {
                cerr << "Uncaught exception" << endl;
                return EXIT_FAILURE;
        }

        return retval;
}
