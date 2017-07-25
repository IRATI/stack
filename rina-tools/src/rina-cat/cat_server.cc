//
// RINA 'cat' (Steve Bunch, (C) 2017)
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

#include <iostream>
#include <cerrno>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

// #include "api.h"
#include "rina/api.h"

#include "cat_server.h"
#include "cat_utils.h"

using namespace std;

CatServer::CatServer (int cfd,
					  int cmd_count,
					  const char **cmd_start,
					  int repetitions,
					  int verbose,
					  const char *this_apn,
					  const char *difname,
					  const char *other_apn,
					  int sdusize)
: controlfd (cfd), command_argcount(cmd_count), command_args(cmd_start),
my_apname(this_apn), verbose(verbose), sdu_size(sdusize), nchildren(0)
{
	if (this_apn)
		reg_returnval = register_apn(this_apn, difname);
	else {
		reg_returnval = -1;
		dif_name = difname;
	}
	client_name = other_apn;
	set_repetitions(repetitions);
}

int CatServer::register_apn (const char *apn, const char *dif)
{
	reg_returnval = rina_register(controlfd, dif, apn, 0);
	if (reg_returnval < 0) {
		cerr << "Registration Failure, app name '" << apn << "', DIF name requested '" << dif << "' error '" << strerror(errno) << "'" << endl;
	}
	return (reg_returnval);
}

void CatServer::unregister_apn()
{
	if (reg_returnval >= 0) {
		rina_unregister(controlfd, dif_name, my_apname, 0);
	}
}

void CatServer::set_remote_apn (const char *remote_apn)
{
	client_name = remote_apn;
}

int CatServer::set_repetitions(int reps)
{
	if (reps >= 0 && reps <= MAX_REPS)
		repetitions = reps;
	return (reps);
}

int CatServer::run()
{
	int ret = 0;
	
	if (reg_returnval < 0) {
		cerr << "Not Registered, can't Listen!" << endl;
		return (errno ? errno : EXIT_FAILURE);
	}
	
	do {
		int incomingfd;
		int flowfd;
		char *incomingapn;
		struct rina_flow_spec fspec;
		
		
		incomingfd = rina_flow_accept(controlfd, &incomingapn, &fspec, RINA_F_NORESP);
		if (incomingfd < 0) {
			cerr << "rina_flow_accept -- unexpected failure.  '" << strerror(errno) << "'" << endl;
			return (EXIT_FAILURE);
		}
		if (verbose) {
			cerr << "Received incoming flow request from app '" << incomingapn << "' with flowspec:" << endl;
		}
		setenv("RINACAT_a", incomingapn, 1);	// publish other app name to any command we launch
		if (verbose > 1) {
			cerr << "    max_sdu_gap (in SDUs): " << fspec.max_sdu_gap << endl;
			cerr << "    avg_bandwidth (in bits/second: " << fspec.avg_bandwidth << endl;
			cerr << "    max_delay (in microseconds): " << fspec.max_delay << endl;
			cerr << "    max_loss (percentage): " << fspec.max_loss << endl;
			cerr << "    max_jitter (in microseconds): " << fspec.max_jitter << endl;
			cerr << "    in_order_delivery (boolean): " << (int)fspec.in_order_delivery << endl;
			cerr << "    msg_boundaries (boolean, 0 = stream): " << (int)fspec.msg_boundaries << endl;
		}
		// see if any of our children have exited (carefully, we don't want to hang here forever...)
		if (nchildren > 0) {
			int retpid;
			while ((retpid = waitforanypid_nohang(verbose))) {
				if (retpid > 0)
					nchildren--;
				else if (retpid < 0 && errno == ECHILD) {
					nchildren = 0;
					break;
				}
				else {
					cerr << "Waiting for pid, got -1 or 0 return but errno isn't ECHILD: " << strerror(errno) << endl;
					return (EXIT_FAILURE);
				}
			}
		}
		free (incomingapn);
		// if persisting and below the limit or aren't persisting, accept it, else refuse it, and
		// is the requested flow coming from the app we're expecting, if we're expecting one?
		if ((repetitions > 0 && nchildren < repetitions) ||
			!client_name || client_name[0] == '\0' ||
			strcmp(incomingapn, client_name) == 0) {
			flowfd = rina_flow_respond(controlfd, incomingfd, 0);	// accept
			if (verbose)
				cerr << "Accepting flow" << endl;
			if (flowfd < 0) {
				cerr << "Responding to flow request did not result in a flow!  Error: '" << strerror(errno) << "'" << endl;
				continue;
			}
		} else {
			if (verbose)
				cerr << "Refusing flow" << endl;
			rina_flow_respond(controlfd, incomingfd, -1);	// refuse the flow
			continue;
		}
		
		// if there's a command to execute, run it, else pump data from stdin to the flow
		if (command_argcount > 0) {
			int pid;
			
			nchildren++;
			ret = exec_command (flowfd, command_argcount, command_args, &pid, verbose);
			if (ret < 0)
				nchildren--;	// oops - forking off the command must have failed
		} else {
			pumpdata_bothdirections (flowfd, sdu_size, verbose);
			return (EXIT_SUCCESS);
		}
		close (flowfd);
	} while (repetitions > 0);

	return (EXIT_SUCCESS);
}

CatServer::~CatServer()
{
	unregister_apn();
	reap_children(); // by default, we wait for all our children before exiting.  Can always control-C
}
