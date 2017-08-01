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
#include <sys/wait.h> // for waitpid()
#include <string.h>
#include <sys/select.h>
#include <stdlib.h>

// #include "api.h"	// RINA api
#include "rina/api.h"

#include "cat_utils.h"

using namespace std;

int exec_command (int flowfd, int argc, const char **argv, int *ret_pid, int verbose)
{
	if (verbose)
		cerr << "Running command" << endl;
	int pid = vfork();
	int pgrp = getpgrp();

	if (pid > 0) { // parent
		*ret_pid = pid;
		if (verbose) {
			cerr << "Fork successful, pid = " << pid << endl;
		}
	} else if (pid == 0) { // child
		close(0); dup (flowfd);
		close (1); dup (flowfd);
		for (int i = 3; i < 100; i++)	// disconnect from control fd or any other irrelevant fds
			close(i);
		setpgid(0, pgrp);	// inherit signals behavior from parent
		execvp(argv[0], (char * const *)argv);
		cerr << "Exec of file " << argv[0] << " failed." << endl;
		exit (errno);
	} else { // error
		cerr << "Fork to run " << argv[0] << " failed." << endl;
		return (-1);
	}
	return (0);
}

static fd_set readbits, writebits;

// 0-length write is treated as EOF, produces a non-zero return, but with errno set to 0 (not an error).
static int condwrite (int outfd, int &count, char *buffer)
{
	if (count > 0 && FD_ISSET (outfd, &writebits)) {
		int result = (int)write (outfd, buffer, count);
		if (result >= 0 || (result < 0 && errno == EWOULDBLOCK)) {
			if (result == 0) { // EOF
				errno = 0;
				return (-1);
			}
			if (result < 0) // error
				return (errno);
			// else short write
			if (result < count) // short write - probably needn't bother, but what the heck.
				memmove(buffer, &buffer[result], count - result);
			count -= result;
		}
	}
	return (0);
}

// Allow a short time for the network or stdout to accept buffered data after the source end EOFs.
// Rather than wait forever with a blocking write, we use a timed select but use a modest wait
// time.  Blocking stdout or a long network outage can cause this strategy to fail, so we print a
// warning to stderr if there's still unbuffered data when we exit.
const int DRAIN_SECONDS = 3;

void drain_buffer (int fd_to_drain, int &count, char *buffer, int verbose)
{
	struct timeval draintime;
	
	if (verbose > 1)
		cerr << "Draining fd " << fd_to_drain << " of " << count << " Bytes" << endl;
	
	if (count <= 0)
		return;
	draintime.tv_usec = 0;
	draintime.tv_sec = DRAIN_SECONDS;
	FD_ZERO(&writebits);
	
	FD_SET (fd_to_drain, &writebits);
	select(fd_to_drain + 1, NULL, &writebits, NULL, &draintime);

	if (write(fd_to_drain, buffer, count) != count) {
		cerr << "WARNING: Failed to drain final output buffer!! Incomplete final write to file " << fd_to_drain << endl;
	}
}

// A zero return is "normal".  EOF on input or output is "normal"; errors aren't, errno is returned.
int pumpdata_bothdirections (int flowfd, int sdu_size, int verbose)
{
	char stdinbuf[sdu_size];
	int stdincount = 0;
	char flowbuf[sdu_size];
	int flowcount = 0;
	
	FD_ZERO(&readbits); FD_ZERO(&writebits);
	
	for (;;) {
		if (stdincount > 0)
			FD_SET(flowfd, &writebits);
		else
			FD_SET(0, &readbits);
		if (flowcount > 0)
			FD_SET (1, &writebits);
		else
			FD_SET(flowfd, &readbits);
		select(flowfd + 1, &readbits, &writebits, NULL, NULL);
		
		// Try getting rid of buffered data
		if (condwrite(flowfd, stdincount, stdinbuf))
			return (errno);
		if (condwrite(1, flowcount, flowbuf))
			return (errno);

		// try reading
		if (stdincount == 0 && FD_ISSET(0, &readbits)) {
			stdincount = (int)read(0, stdinbuf, sdu_size);
			if (stdincount < 0 && errno != EWOULDBLOCK)
				return (errno);
			if (stdincount == 0) {
				drain_buffer(flowfd, stdincount, stdinbuf, verbose);
				drain_buffer(1, flowcount, flowbuf, verbose);
				break;
			}
		}
		if (flowcount == 0 && FD_ISSET(flowfd, &readbits)) {
			flowcount = (int)read(flowfd, flowbuf, sdu_size);
			if (flowcount < 0 && errno != EWOULDBLOCK)
				return (errno);
			if (flowcount == 0) {
				drain_buffer(1, flowcount, flowbuf, verbose);
				drain_buffer(flowfd, stdincount, stdinbuf, verbose);
				break;
			}
		}
	}
	return (0);
}

int waitforpid_internal (int pid, int options, int verbose)
{
	int status;
	int returned_pid;
	
	returned_pid = waitpid (pid, &status, options);
	if (verbose) {
		if (returned_pid < 0) {
			if (verbose)
				cerr << "Waitpid returned -1.  errno: " << strerror(errno) << endl;
		} else {
			if (WEXITSTATUS(status)) {
				cerr << "Pid " << returned_pid << " returned non-normal status " << WEXITSTATUS(status) << endl;
			} else {
				cerr << "Pid " << returned_pid << " exited with normal (0) status" << endl;
			}
			if (WIFSIGNALED(status)) {
				cerr << "Pid " << returned_pid << " terminated by signal "<< WIFSIGNALED(status) << endl;
			}
		}
	}
	return (returned_pid);
}

int waitforpid (int pid, int verbose)
{
	return (waitforpid_internal(pid, 0, verbose));
}

int waitforanypid_nohang (int verbose)
{
	return (waitforpid_internal(-1, WNOHANG, verbose));
}

void reap_children()
{
	while (waitforpid_internal(-1, 0, 0) >= 0)
		sleep(1);
}

