//
//  rinacat.c
//  rinacat
//
//  Created by Steve Bunch on 8/2/17.
//  Copyright © 2017 Steve Bunch. All rights reserved.
//

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>	// for gethostname
#include <libgen.h>	// for basename
#include <fcntl.h>
//#include <limits.h>
#include <sys/wait.h> // for waitpid()
#include <sys/select.h>
#include <getopt.h>
#include <signal.h>	// for sigaction

#include "rina/api.h"

#define NON_PERSISTENT	0		// value that indicates to do the command once, then terminate, >0 == persist
#define MAX_REPS		100		// arbitrary sanity bound on simultaneous servers we'll launch
#define DEFAULT_SDUSIZE 1024	// default should be less than expected MTU of any reasonable underlying transport
#define DRAIN_SECONDS	3		// seconds for a blocked write to be allowed to drain before exiting anyway
#define MAXHOSTNAME		256		// longest host name we will use -- gethostname() result truncated if necessary
#define APN_API_SEPARATOR_CHAR_STRING "|"	// separator between APN and API in appname in the rina/api API

#define VERBOSE(args...)		if (verbose) { fprintf (stderr, args);  } else
#define VVERBOSE(args...)		if (verbose > 1) { fprintf (stderr, args);  } else
#define V3VERBOSE(args...)		if (verbose > 2) { fprintf (stderr, args);  } else
#define V4VERBOSE(args...)		if (verbose > 3) { fprintf (stderr, args);  } else
#define PRINTERRORMSG(args...)	fprintf (stderr, args)
#define EMPTYSTRING(s)			(s == NULL || *s == '\0')	// test safely for an empty string

int cfd = -1;					// RINA control fd
int regfd = -1;					// RINA registration fd

// Results of command argument parsing.
unsigned int repetitions = 0;	// 0 == non-persistent, >0 == persist server, run up to this many simultaneous commands
int verbose = 0;				// 1 == verbose, >1 == more verbose
int listenflag = 0;				// non-zero means we are in "listen"/server/target mode
int sdusize = DEFAULT_SDUSIZE;	// used when rinacat does the data pump, ignored by commands
const char *this_apn = "";
const char *difname = NULL;
const char *other_apn = "";
const char *command = "";		// command string will be executed by a shell -c, so anything goes (redirection, etc.)

int getver = 0;					// set to 1 if all that's desired is the version information
#ifndef _VERSION
#define _VERSION "20170921"	// Override if desired by providing -D_VERSION=xx at compile time on command line
#endif

#define UNRELIABLE_FLOW	0		// request an unreliable flow (client side only - ignored by server)
#define RELIABLE_FLOW	1		// request a reliable flow (client side only - ignored by server)
#define DEFAULT_FLOW	2		// DO NOT OVERRIDE the default flow spec produced by rina_flow_spec_default()
#define USE_AS_DEFAULT_FLOW	RELIABLE_FLOW	// this is the default if no command line option overrides it
int flow_reliability = USE_AS_DEFAULT_FLOW;	// user can override the default on command line

#define STREAM_FLOW	1		// request a stream-oriented flow (client)
#define MESSAGE_FLOW	0		// request a message/SDU-oriented flow (client)
int flow_boundaries = MESSAGE_FLOW;	// default to a message-oriented flow, allow override on command line

int command_argc = 0;
const char *command_argv[4];	// argv for command to launch (leave a NULL at the end for exec)

int CatClient();				// forward -- client (connector)
int CatServer();				// forward -- server (listener/target of connector)
void usage(int error, const char *command);	// forward -- print usage information
char *cat3 (const char *left, const char *middle, const char *right);	// forward -- string concatenation
char *safe_ltoa (long int value);	// safely convert an integer to a character string, mallocs its answer
const char *pick_good_apn (const char *, const char *);	// try to find the command name in the second arg, if failure, use first arg
void abrt_handler (int, siginfo_t *, void *);	// SIGABRT handler to deal with failed assert()'s in RINA libraries

static struct option cmd_options[] =
{
	{ "listen",		no_argument,		NULL,		'l' },
	{ "verbose",	no_argument,		NULL,		'v' },
	{ "very-verbose", no_argument,		NULL,		'V' },
	{ "other-apn",	required_argument,	NULL,		'a' },
	{ "this-apn",	required_argument,	NULL,		'A' },
	{ "other-api",	required_argument,	NULL,		'i' },
	{ "this-api",	required_argument,	NULL,		'I' },
	{ "persist",	required_argument,	NULL,		'p' },
	{ "difname",	required_argument,	NULL,		'd' },
	{ "sdusize",	required_argument,	NULL,		's' },
	{ "command",	required_argument,	NULL,		'c' },
	{ "help",		required_argument,	NULL,		'h' },
	{ "version",	no_argument,		&getver,	1 },
	{ "unreliable", no_argument,		NULL,		'u' },
	{ "reliable",	no_argument,		NULL,		'r' },
	{ "defaultFS",	no_argument,		&flow_reliability, DEFAULT_FLOW },
	{ "stream",	no_argument,		&flow_boundaries, STREAM_FLOW },
	{ NULL, 0, NULL, 0 }
};

int main (int argc, char **argv)
{
	int result = EXIT_SUCCESS;
	int ch;
	char *this_api = NULL, *other_api = NULL;	// optional application instances
	const char *commandname = basename(argv[0]);		// for error messages and for setting default apn
	
	while ((ch = getopt_long(argc, argv, "la:A:p:vVI:i:d:s:c:h", cmd_options, NULL)) != -1) {
		switch (ch) {
			case 'l': listenflag = 1; break;
			case 'v': verbose++; break;
			case 'V': verbose += 2; break;
			case 'u': flow_reliability = UNRELIABLE_FLOW; break;
			case 'r': flow_reliability = RELIABLE_FLOW; break;
				
			case 'a': other_apn = optarg; break;
			case 'A': this_apn = optarg; break;
			case 'i': other_api = optarg; break;
			case 'I': this_api = optarg; break;
			case 'p': repetitions = atoi(optarg); break;
			case 'd': difname = optarg; break;
			case 's': sdusize = atoi(optarg); break;
			case 'c': command = optarg; break;	// set command arg as shell command string

			case 'h': usage(0, commandname); return (EXIT_SUCCESS);
				
			case 0: if (getver) { PRINTERRORMSG("Version: %s\n", _VERSION); return (EXIT_SUCCESS); }
				
			case '?':
			default: usage(1, commandname); return (EXIT_FAILURE);
		}
	}
	if (optind < argc) {
		PRINTERRORMSG("ERROR: Argument(s) starting with %s not understood.\n", argv[optind]);
		usage(0,commandname);
		return (EXIT_FAILURE);
	}
	
	// We can't persist without a command
	if (EMPTYSTRING(command) && repetitions) {
		PRINTERRORMSG("WARNING: -p option meaningless without command option (-c).  Ignored.\n");
		repetitions = 0;
	}
	if (repetitions > MAX_REPS) {
		PRINTERRORMSG("WARNING: Repetition count must be from 0 to %d, %d assumed \n", MAX_REPS, MAX_REPS);
		repetitions = MAX_REPS;
	}
	
	if (!listenflag && EMPTYSTRING(other_apn)) {
		PRINTERRORMSG("ERROR: Client (initiator) mode requires a destination listener/server application name.\n");
		return (EXIT_FAILURE);
	}
	
	if (listenflag && (flow_reliability != USE_AS_DEFAULT_FLOW || flow_boundaries != MESSAGE_FLOW)) {
		PRINTERRORMSG("WARNING: --unreliable and --stream arguments only meaningful for client.  Ignored.\n");
	}
	
	// if there is a command, attempt to locate the basename of the command so we can use that in the default apname
	if (!EMPTYSTRING(command)) {
		commandname = pick_good_apn (commandname, command);
	}

	// we have to have a name for this app to be able to register it and be found, but it's also nice for the other end
	// to be able to report who is connecting to it (or even initiate an authorization exchange based on that name)
	if (EMPTYSTRING(this_apn)) {
		// manufacture a name for this app.  Has form: hostname.commandname or simply commandname
		char thishost[MAXHOSTNAME+1];
		
		thishost[MAXHOSTNAME] = '\0';	// gethostname will not null-terminate its result if it truncates
		if (gethostname(thishost, MAXHOSTNAME) < 0) {
			this_apn = commandname;
		} else {
			this_apn = cat3(thishost, ".", commandname);
		}
		PRINTERRORMSG("WARNING: Default application name will be used: '%s'\n", this_apn);
	}

	// Adjust application names if user provided the optional instance id(s)
	if (this_api)
		this_apn = cat3(this_apn, APN_API_SEPARATOR_CHAR_STRING, this_api);
	if (other_api)
		other_apn = cat3(other_apn, APN_API_SEPARATOR_CHAR_STRING, other_api);

	// We will launch the command string with equivalent of: $SHELL -c "command string", so build it
	if (!EMPTYSTRING(command)) {
		command_argc = 3;
		command_argv[0] = getenv("SHELL");
		if (!command_argv[0] || access(command_argv[0], X_OK)) {
			// oops - no shell defined in environment, or can't execute it.  Fail now rather than when we try to exec it later.
			PRINTERRORMSG("ERROR: SHELL environment variable not defined or not an executable file, can't launch commands, exiting.\n");
			return (EXIT_FAILURE);
		}
		command_argv[1] = "-c";
		command_argv[2] = command;
		
		// Pass information about the command line to any forked commands via environment variables
		setenv("RINACAT_A", (this_apn ? this_apn : ""), 1);
		setenv("RINACAT_a", (other_apn ? other_apn : ""), 1);
		setenv("RINACAT_d", (difname ? difname : ""), 1);
		setenv("RINACAT_sdusize", safe_ltoa((long)sdusize), 1);
		setenv("RINACAT_l", (listenflag ? "l" : ""), 1);
		setenv("RINACAT_v", (safe_ltoa((long)verbose)), 1);
		setenv("RINACAT_reliability", (flow_reliability == DEFAULT_FLOW ? "d" : (flow_reliability == RELIABLE_FLOW ? "r" : "u")), 1);
		setenv("RINACAT_delimiting", (flow_boundaries == STREAM_FLOW ? "s" : "r"), 1);
	}
	
	// Intercept SIGABRT signal that arises from an assert() failure in a library
	struct sigaction sact;
	sact.sa_flags = SA_SIGINFO;
	memset(&sact.sa_mask, 0, sizeof (sact.sa_mask));
	sact.sa_sigaction = abrt_handler;
	sigaction(SIGABRT, &sact, (NULL));
	
	// Initialize the control fd
	cfd = rina_open();
	if (cfd < 0) {
		PRINTERRORMSG("ERROR: Unable to initialize RINA infrastructure, error: %s\n", strerror(errno));
		return (EXIT_FAILURE);
	}
	VERBOSE("Opened control fd %d\n", cfd);

	if (listenflag)
		result = CatServer();
	else
		result = CatClient();
	return result;
}

char *cat3 (const char *left, const char *middle, const char *right)
{
	char *result = malloc (strlen(left) + strlen (middle) + strlen (right) + 1);
	
	*result = '\0';
	strcat (result, left);
	strcat (result, middle);
	strcat (result, right);
	return (result);
}

char *safe_ltoa (long int value)
{
	int length = snprintf(NULL, 0,"%ld",value) + 1;
	char *result = malloc(length);
	
	snprintf(result, length, "%ld", value);
	return (result);	// don't forget to free it eventually
}

const char *pick_good_apn (const char *oldname, const char *candidate)
{
	int c, first_space, first_nonslash = 0;
	int length;
	
	if (EMPTYSTRING(candidate))
		return (oldname);
	
	// isolate first token in candidate string, assuming no leading spaces (hint: add a space intentionally to force use of oldname):
	for (first_space = 0; (c = candidate[first_space]) && !isspace(c);first_space++) {
		if (c == '/')
			first_nonslash = first_space + 1;
	}

	length = first_space - first_nonslash;
	if (length < 1)
		return (oldname);

	// malloc space, strncopy in the name, null terminate it, return it
	char *newname = malloc (length + 1);
	strncpy(newname, &candidate[first_nonslash], length);
	newname[length] = '\0';
	return (newname);
}

// print usage information.
void usage (int error, const char *command)
{
	if (error) {
		PRINTERRORMSG("ERROR: Command line error.\n");
	}
	PRINTERRORMSG("Usage:\
%s [-l] [-a <string>] [-A <string>] [-c <string>] [-p <integer>]\
    [-v] [-V] [-I <string>] [-i <string>] [-d <string>] [-s <integer>] [-h] [--]\n", command);
	PRINTERRORMSG("Where:\n\
	\n\
  -l,  --listen\n\
	Register this app, run in server mode awaiting incoming flow\n\
	request(s)\n\
	\n\
  -a <string>,  --other-apn <string>\n\
	Application process name for the other end\n\
	\n\
  -A <string>,  --this-apn <string>\n\
	Application process name for this application\n\
	\n\
  -c <string>,  --command <string>\n\
	Command to execute to process the flow\n\
	\n\
  -p <integer>,  --persist-servers <integer>\n\
	Run persistently, up to this many simultaneous commands - server\n\
	requests over this number will be refused\n\
	\n\
  -v,  --verbose\n\
	Increase debug and informational output\n\
	\n\
  -V,  --very-verbose\n\
	Maximize debug and informational output\n\
	\n\
  -I <string>,  --this-api <string>\n\
	Application process instance for this application\n\
	\n\
  -i <string>,  --other-api <string>\n\
	Application process instance for the other end\n\
	\n\
  -d <string>,  --dif <string>\n\
	The name of the DIF to use or register at (may be ignored)\n\
	\n\
  -s <integer>, --sdusize <integer>\n\
	Default size for read/write transfers on RINA flow (ignored by exec'ed commands\n\
	\n\
  --\n\
	Ignores the rest of the arguments following this flag.\n\
	\n\
  --version\n\
	Displays version information and exits.\n\
	\n\
  --u, --unreliable\n\
	Force client to use an unreliable flow instead of the (unspecified) default\n\
	\n\
  --r, --reliable\n\
	Force client to use a reliable flow instead of the (unspecified) default\n\
  --defaultFS\n\
	Force client to use the default Flow Spec provided by the implementation\n\
  --stream\n\
	Force client to use a stream (boundary-less) flow instead of the default record/SDU\n\
	\n\
  -h,  --help\n\
	Displays usage information and exits.\n");
}


int exec_command (int flowfd, int argc, const char **argv, int *ret_pid)
{
	VERBOSE ("Running command '%s'\n", argv[2]);
	int pgrp = getpgrp();
	int pid;
	
	pid = vfork();
	if (pid > 0) { // parent
		*ret_pid = pid;
		VERBOSE("Fork successful, pid %d\n", pid);
	} else if (pid == 0) { // child
		int newfd, i;
		close(0);
		newfd = dup(flowfd);
		if (newfd) {
			PRINTERRORMSG("ERROR: Dup(flowfd) returned %d, not expected 0, error %s\n", newfd, strerror(errno));
			return (-1);
		}
		close (1);
		newfd = dup (flowfd);
		if (newfd != 1) {
			PRINTERRORMSG("ERROR: Dup(flowfd) returned %d, not expected 1, error %s\n", newfd, strerror(errno));
			return (-1);
		}
		for (i = 3; i < 10; i++)	// disconnect from control fd or any other irrelevant fds
			close(i);
		setpgid(0, pgrp);	// ensure that child sees parent's signals, can access parent's controlling terminal
		execvp(argv[0], (char * const *)argv);
		PRINTERRORMSG("ERROR: Exec of file %s failed, error %s.  Exiting.\n", argv[0], strerror(errno));
		exit (errno);
	} else { // error
		PRINTERRORMSG("ERROR: Fork to run %s failed, error %s\n", argv[0], strerror(errno));
		return (-1);
	}
	return (0);
}

static fd_set readbits, writebits;

// 0-length write is treated as EOF, produces a non-zero return, but with errno set to 0 (not an error).
static int condwrite (int outfd, int *count, char *buffer)
{
	if (*count > 0 && FD_ISSET (outfd, &writebits)) {
		int result = (int)write (outfd, buffer, *count);
		if (result >= 0 || (result < 0 && errno != EWOULDBLOCK)) {
			if (result == 0) { // EOF
				errno = 0;
				return (-1);
			}
			if (result < 0) // error
				return (-1);
			// else short write
			if (result < *count) // short write - probably needn't bother, but what the heck.
				memmove(buffer, &buffer[result], *count - result);
			*count -= result;
		}
	}
	return (0);
}

// Allow a short time for the network or stdout to accept buffered data after the source end EOFs.
// Rather than wait forever with a blocking write, we use a timed select but use a modest wait
// time.  Blocking stdout or a long network outage can cause this strategy to fail, so we print a
// warning to stderr if there's still unbuffered data when we exit.

void drain_buffer (int fd_to_drain, int *count, char *buffer)
{
	struct timeval draintime;
	
	VVERBOSE("Draining fd %d of %d bytes\n", fd_to_drain, *count);
	
	if (*count <= 0)
		return;
	draintime.tv_usec = 0;
	draintime.tv_sec = DRAIN_SECONDS;
	FD_ZERO(&writebits);
	
	FD_SET (fd_to_drain, &writebits);
	select(fd_to_drain + 1, NULL, &writebits, NULL, &draintime);
	
	if (write(fd_to_drain, buffer, *count) != *count) {
		PRINTERRORMSG("WARNING: Failed to drain final output buffer!! Incomplete final write to file %d\n", fd_to_drain);
	}
}

// Bi-directional data pump.  Read to stdin/write to flow, read from flow/write to stdout.
// A zero return is "normal".  EOF on input or output is "normal"; errors aren't, errno is returned.
int pumpdata_bothdirections (int flowfd, int sdu_size)
{
	char stdinbuf[sdu_size];
	int stdincount = 0;
	char flowbuf[sdu_size];
	int flowcount = 0;
	
	V3VERBOSE("Pumping data in rinacat.\n");
	for (;;) {
		FD_ZERO(&readbits); FD_ZERO(&writebits);
		
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
		if (condwrite(flowfd, &stdincount, stdinbuf))
			return (errno);
		if (condwrite(1, &flowcount, flowbuf))
			return (errno);
		
		// try reading
		if (stdincount == 0 && FD_ISSET(0, &readbits)) {
			stdincount = (int)read(0, stdinbuf, sdu_size);
			if (stdincount < 0 && errno != EWOULDBLOCK)
				return (errno);
			if (stdincount == 0) {
				drain_buffer(flowfd, &stdincount, stdinbuf);
				drain_buffer(1, &flowcount, flowbuf);
				break;
			}
		}
		if (flowcount == 0 && FD_ISSET(flowfd, &readbits)) {
			flowcount = (int)read(flowfd, flowbuf, sdu_size);
			if (flowcount < 0 && errno != EWOULDBLOCK)
				return (errno);
			if (flowcount == 0) {
				drain_buffer(1, &flowcount, flowbuf);
				drain_buffer(flowfd, &stdincount, stdinbuf);
				break;
			}
		}
	}
	return (0);
}

int waitforpid_internal (int pid, int options)
{
	int status;
	int returned_pid;
	
	returned_pid = waitpid (pid, &status, options);
	// We assume the user doesn't normally care how the launched server(s) end...
	if (returned_pid < 0) {
		VERBOSE("Waitpid returned -1.  error: %s\n", strerror(errno));
	} else {
		if (WEXITSTATUS(status)) {
			VERBOSE("Pid %d returned non-normal status %d\n", returned_pid, WEXITSTATUS(status));
		} else {
			VERBOSE("Pid %d exited with normal (0) status\n", returned_pid);
		}
		if (WIFSIGNALED(status)) {
			VERBOSE("Pid %d terminated by signal %d\n", returned_pid, WIFSIGNALED(status));
		}
	}
	return (returned_pid);
}

int waitforpid (int pid)
{
	return (waitforpid_internal(pid, 0));
}

int waitforanypid_nohang ()
{
	return (waitforpid_internal(-1, WNOHANG));
}

void reap_children()
{
	while (waitforpid_internal(-1, 0) >= 0)
		sleep(1);
}

// Called mainly if the underlying libraries do an abort(), usually from an assert() failing.
void abrt_handler (int sig, siginfo_t *siginfo, void *stuff)
{
	PRINTERRORMSG("ERROR: SIGABRT signal handler invoked.  Likely an assert() failure.  Save logs, get help.  Waiting for any children.\n");
	reap_children();
	exit (SIGABRT);
}

int CatClient ()
{
	int ret = 0;
	int flowfd;	// file handle for flow
	struct rina_flow_spec fspec;
	
	VERBOSE("Running in Client mode, attempting to connect out to apn %s\n", other_apn);
	
	rina_flow_spec_default (&fspec);

	// Adjust default fspec based on command line options and program defaults
	fspec.msg_boundaries = flow_boundaries;		// r/w boundaries -- STREAM_FLOW or MESSAGE_FLOW
	if (flow_reliability == RELIABLE_FLOW) {
		fspec.in_order_delivery = 1;	// in-order delivery
		fspec.max_sdu_gap = 0;		// no loss
	}
	if (flow_reliability == UNRELIABLE_FLOW) {
		fspec.max_sdu_gap = -1;
	}
	
	flowfd = rina_flow_alloc(difname, this_apn, other_apn, &fspec, 0);
	
	if (flowfd < 0) {
		PRINTERRORMSG("ERROR: Unable to create client flow, exiting, error %s\n", strerror(errno));
		return (errno);
	}
	
	// if there's a command to execute, run it, else pump data from stdin to the flow
	if (command_argc > 0) {
		int pid;
		
		ret = exec_command (flowfd, command_argc, command_argv, &pid);
		V3VERBOSE("Waiting for launched command, pid %d, to complete.\n", pid);
		waitforpid(pid);
	} else {
		pumpdata_bothdirections (flowfd, sdusize);
	}
	
	close (flowfd);
	return (ret);
}

// Register, await flow(s), pump or launch command to serve it.  Non-zero return means something bad
int CatServer()
{
	int result = EXIT_SUCCESS;
	int nchildren = 0;	// number of forked commands that haven't completed
	
	VERBOSE("Running in Server mode, registering as %s in DIF %s\n", this_apn, difname);
	
	regfd = rina_register(cfd, difname, this_apn, 0);
	if (regfd < 0) {
		PRINTERRORMSG("Registration failed, app name '%s', DIF name requested '%s', error %s\n", this_apn, difname, strerror(errno));
		return (errno ? errno : EXIT_FAILURE);
	}
	
	do {
		int incomingfd;
		int flowfd;
		char *incomingapn = NULL;
		struct rina_flow_spec fspec;
		
		fspec.version = RINA_FLOW_SPEC_VERSION;
		incomingfd = rina_flow_accept(cfd, &incomingapn, &fspec, RINA_F_NORESP);
		if (incomingfd < 0) {
			PRINTERRORMSG("WARNING: Unexpected flow accept failure, ignoring.  Error: %s\n", strerror(errno));
			if (incomingapn) {
				PRINTERRORMSG("    APN: %s\n", incomingapn);
				free(incomingapn);
			}
			continue;
			// result = EXIT_FAILURE; break;
		}
		if (incomingapn) {
			VERBOSE("Received incoming flow request from app '%s'\n", incomingapn);
			setenv("RINACAT_a", incomingapn, 1);
			free (incomingapn);
		} else {
			VERBOSE("Received incoming flow request, no apn supplied\n");
			setenv("RINACAT_a", "", 1);
		}
		
		VVERBOSE("    with flowspec:\n");
		VVERBOSE("    max_sdu_gap (in SDUs): %ld\n", (long)fspec.max_sdu_gap);
		VVERBOSE("    avg_bandwidth (in bits/second): %ld\n", (long)fspec.avg_bandwidth);
		VVERBOSE("    max_delay (in microseconds): %d\n", fspec.max_delay);
		VVERBOSE("    max_loss (percentage): %d\n", fspec.max_loss);
		VVERBOSE("    max_jitter (in microseconds): %d\n", fspec.max_jitter);
		VVERBOSE("    in_order_delivery (boolean): %d\n", (int)fspec.in_order_delivery);
		VVERBOSE("    msg_boundaries (boolean, 0 = stream): %d\n", (int)fspec.msg_boundaries);

		// see if any of our children have exited (carefully, we don't want to hang here forever...)
		if (nchildren > 0) {
			int retpid;
			while ((retpid = waitforanypid_nohang())) {
				if (retpid > 0)
					nchildren--;
				else if (retpid < 0 && errno == ECHILD) {
					nchildren = 0;
					break;
				} else {
					PRINTERRORMSG("WARNING: Waiting for pid, got -1 or 0 return but errno isn't ECHILD.  Error %s\n", strerror(errno));
					result = EXIT_FAILURE;	// debatable, as 0 return can just be from tracing a child...
					break;
				}
			}
		}
		
		// if persisting and below the limit or aren't persisting, accept it, else refuse it, and
		// is the requested flow coming from the app we're expecting, if we're expecting one?
		//
		// POSSIBLE ENHANCEMENT: reject flows whose flowspec doesn't match parameters explicitly specified in arguments
		// to server.  (Note that you have to be able to distinguish explicit arguments vs. a don't-care default).
		if ((repetitions > 0 && nchildren < repetitions) ||
			EMPTYSTRING(other_apn) ||
			strcmp(incomingapn, other_apn) == 0) {
			flowfd = rina_flow_respond(cfd, incomingfd, 0);	// 0 == accept
			VERBOSE("Accepting flow, fd %d\n", flowfd);
			if (flowfd < 0) {
				PRINTERRORMSG("WARNING: Responding to flow request did not result in a flow!  Error: %s\n", strerror(errno));
				continue;
			}
		} else {
			VERBOSE("Refusing flow, too many flows or mismatch of incoming APN with goal apn\n");
			rina_flow_respond(cfd, incomingfd, -1);	// refuse the flow
			continue;
		}
		
		// if there's a command to execute, run it, else pump data from stdin to the flow
		if (command_argc > 0) {
			int pid;
			
			nchildren++;
			if (exec_command (flowfd, command_argc, command_argv, &pid) < 0)
				nchildren--;	// oops - forking off the command must have failed
			V3VERBOSE("After fork, number of children now %d/%d\n", nchildren, repetitions);
		} else {
			pumpdata_bothdirections (flowfd, sdusize);
			// Note that if we're the data pump, repetitions are zero (if you change that, close(flowfd); break; here)
		}
		close (flowfd);
	} while (repetitions > 0);
	
	V3VERBOSE("Done. Closing registration fd\n");
	close (regfd);
	V3VERBOSE("Unregistering apn %s \n", this_apn);
	if (rina_unregister(cfd, difname, this_apn, 0) < 0) {
		PRINTERRORMSG("WARNING: Unregistering failed, error %s\n", strerror(errno));
		result = EXIT_FAILURE;
	}
	V3VERBOSE("Children still alive: %d, if >0 will wait for them all to complete\n", nchildren);
	if (nchildren > 0) {
		reap_children();	// waiting debatable, but we wouldn't be here with children unless something bad happened
	}
	return (result);
}
