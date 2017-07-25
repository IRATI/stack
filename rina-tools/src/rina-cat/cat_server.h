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

#ifndef CAT_SERVER_H
#define CAT_SERVER_H

#define MAX_REPS	1024	// arbitrary sanity bound on how many servers we're willing to launch at once

class CatServer
{
public:
	CatServer(int cfd,
			  int cmd_count = 0,
			  const char **cmd_start = 0,
			  int repetitions = 0,
			  int verbose = 0,
			  const char *this_apn = 0,
			  const char *dif_name = 0,
			  const char *other_apn = 0,
			  int sdusize = 4096);
	
	int register_apn(const char *this_apn, const char *dif_name);
	void unregister_apn();
	void set_remote_apn (const char *remote_apn);
	int set_repetitions (int reps);
	int run();
	~CatServer();

private:
	int controlfd;
	int command_argcount;
	const char **command_args;
	int repetitions;
	const char *my_apname;
	const char *dif_name;
	const char *client_name;
	int verbose;
	int sdu_size;
	
	int reg_returnval;
	int nchildren;
};

#endif
