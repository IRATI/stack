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

#ifndef CAT_CLIENT_H
#define CAT_CLIENT_H

#include <iostream>
#include <string>

class CatClient  {
public:
	// Initiates a flow to the designated target app.
	// Once successful, if command count > 0, fork and attach child stdin/stdout to the flow,
	// exec the command, wait for result.  If not, datapump (stdin, connectionfd), terminate on eof.
	
	CatClient (int cfd,
			   int cmd_count,
			   const char **cmd_start,
			   int verbose,
			   const char *this_apn,
			   const char *other_apn,
			   const char *dif_name,
			   int sdusize);
	int run();
	~CatClient();
	
private:
	int cfd;
	int command_argcount;
	const char **command_args;
	const char *my_apname;
	const char *dif_name;
	const char *server_name;
	bool verbose;
	int sdu_size;
};

#endif//CAT_CLIENT_H
