//
// RINA 'cat' (reuses a few lines of code from Echo Server by Addy Bombeke <addy.bombeke@ugent.be>
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

#include <iostream>
#include <unistd.h>
#include <limits.h>
#include <cerrno>
#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>		// for fcntl()
#include <string.h>

// #include "api.h"	// RINA api
#include "rina/api.h"

#include "cat_client.h"
#include "cat_utils.h"

using namespace std;

CatClient::CatClient (int cfd,
					  int cmd_count,
					  const char **cmd_start,
					  int verbose,
					  const char *this_apn,
					  const char *other_apn,
					  const char *dif_name,
					  int sdusize)
: cfd(cfd), command_argcount(cmd_count), command_args(cmd_start), my_apname(this_apn),
dif_name(dif_name), server_name(other_apn), verbose(verbose), sdu_size(sdusize)
{
}

int CatClient::run()
{
	int ret = 0;
	int flowfd;	// file handle for flow
	struct rina_flow_spec fspec;
	
	rina_flow_spec_default (&fspec);
	fspec.in_order_delivery = 1;	// in-order delivery
	fspec.max_sdu_gap = 0;		// no loss
	fspec.msg_boundaries = 0;	// no boundaries -- streaming mode, like stdin/stdout
	
	flowfd = rina_flow_alloc(dif_name, my_apname, server_name, &fspec, 0);
	
	if (flowfd < 0) {
		cerr << "Unable to create client flow, exiting, error '" << strerror(errno) << "'" << endl;
		return (errno);
	}
	
	ret = fcntl(flowfd, F_SETFL, 0);	// not clear this is necessary, should check returned fd...
	if (ret) {
		cerr << "Failed to clear O_NONBLOCK flag" << endl;
	}
	
	// if there's a command to execute, run it, else pump data from stdin to the flow
	if (command_argcount > 0) {
		int pid;
		
		ret = exec_command (flowfd, command_argcount, command_args, &pid, verbose);
		waitforpid(pid, verbose);
	} else {
		pumpdata_bothdirections (flowfd, sdu_size, verbose);
	}
	
	close (flowfd);
	return (ret);
}

CatClient::~CatClient()
{
}

