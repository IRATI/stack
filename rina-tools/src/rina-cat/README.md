# rina-cat -- A RINA Flow Redirection Utility

### Steve Bunch, 25 July 2017 — Alpha Version

## rina-cat Operation:

### Overview
By default, rina-cat copies bidirectionally between its own standard input and standard output files to/from a RINA flow.  rina-cat can be used in either client (initiator) or server (listener/target) mode.  

Rather than performing its own copying, rina-cat can also launch applications when run in either client or server mode, exec’ing a separate process whose standard input and output are connected to the RINA flow that it sets up.  In server mode, it can launch an independent process for each incoming flow request.  Using standard UNIX/Linux commands, rina-cat can implement a bi-directional chat, perform file copying, launch server instances, and perform many other functions without writing custom programs.  See the Examples.  To get a list of the command-line options, run __rina-cat --help__

### Client Mode
In client mode, the default, rina-cat attempts to create a RINA flow to a specified destination RINA application.  If successful, in its simplest mode of operation, it then copies its own standard input to the flow, and simultaneously reads from the flow and copies to its standard output file.  An EOF indication or error on any file terminates the execution.  Optionally, rather than copying its own standard input/output, the client can launch a separate command, provided on the rina-cat command line using the -c option.  rina-cat will connect the flow to the command’s standard input and output, wait for it to complete, then terminate.

### Server Mode
In server(listen) mode, designated using the -l option, rina-cat registers an application name, then awaits incoming flow requests.  When one is received, it vets it (more below on that) and if happy with the request, accepts the flow.  In its simplest mode of operation, it then copies data from the flow to its standard output, simultaneously reads from its standard input and copies it to the flow, and when EOF or error is seen on any file, terminates.  In server mode, rather than copying to/from its own standard output/input, rina-cat can instead launch a separate application and attach the flow to its standard input and output, and let that application handle the flow and terminate when the client application completes.  If persistence option -p is specified, the server rina-cat will instead remain operating indefinitely, launching and monitoring up to the specified maximum number of simultaneous applications as flow requests arrive.  Requests that would cause the number of active commands to exceed the command-line-set limit will be refused.

## Misc. Details
When launching a separate Linux application in either client or server mode, rina-cat does not redirect the standard error output file, so errors from the rina-cat program as well as any children will NOT go over the RINA flow.  Stderr for a launched command can be redirected on its command line if desired.

Commands launched by rina-cat are placed into the same process group as rina-cat, so signals such as SIGTERM (control-C) sent to rina-cat will also be sent to any running command(s).  If this behavior is not desired, the commands are free to detach themselves from the process group (or we can in future provide an auto-detach option to rina-cat.)

The command line provided to rina-cat via the -c option is sent to the current default shell ($SHELL) for execution, using its -c command option.  The effect is essentially the same as:
	$SHELL -c “string provided using rina-cat -c option”
Therefore, the command line can use environment variables, pipes, redirection, or any other typical shell command line function.  Some of the command line options of rina-cat as well as information about incoming connections to a server are provided to commands via environment variables (see example below).  Users need to be careful of shell quoting conventions when specifying commands whose arguments include strings or environment variables.


## Examples:

***Simple two-way “chat” session between a client and server:  Copy input from standard input on each system to standard output on the other.***

_Client:_
	# rina-cat -a dest_app -d normal.DIF  
_Server:_
	# rina-cat -l -A dest-app -d normal.DIF

Input typed to the standard input of the client instance will be sent over the RINA flow and sent to standard output of the server instance, and vice versa.  (The -d DIFNAME option may not be needed.)  An EOF (control-D) at either end will terminate both applications above and end the conversation.  The server could be made persistent, e.g., with -p 1, in which case it would remain active and listening for future client sessions.


***To do a file copy, just redirect the input and output to files:***

_Client:_
	# rina-cat -a dest_app -d normal.DIF <filename  
_Server:_
	# rina-cat -l -A dest-app -d normal.DIF >filename

Note that the copy could have instead been done in the other direction.  (Copying files simultaneously in both directions will likely result in one file being truncated, as rina-cat terminates when any file read hits EOF.)


***Copy a video stream from a Raspberry Pi camera to the display of another Raspberry Pi:***

_Client:_
	# raspivid -fps 10 -w 800 -h 600 -t 0 -o - | rina-cat -a display -d normal.DIF  
_Server:_
	# rina-cat -l -A display -d normal.DIF | mplayer -fps 10 -cache 32 -vo x11 -


***Run a persistent video server (in some ways an improvement over the last example, as it doesn’t buffer data in a pipe, and allows player commands to be fed back to the source, in cases where that’s useful).***

_Client:_
	# rina-cat -a server_app -c “mplayer -fps 10 -cache 32 -vo x11 -“  
_Server (persistent, allowing one active flow at a time):_
	# rina-cat -l -A server_app -p 1 -c “raspivid -fps 10 -w 800 -h 600 -t 0 -o -“

*(NOTE: we've noticed that the raspivid program sometimes enters a state where it quits sending video but doesn't quit, probably when the network can’t keep up.  The pipe version above, or running it over a network with more buffering, can avoid this problem somewhat, but it’s a bug in raspivid that needs to be worked around at this time.)*


***Commands executed by rina-cat have access via environment variables to information about the rina-cat instance that ran them.  In particular (experimental, subject to change):***  
  * RINACAT_A		Name of this rina-cat instance (-A argument)
  * RINACAT_a		Name of other rina-cat instance (-a argument, or incoming flow)
  * RINACAT_d		Name of dif to use (-d argument)
  * RINACAT_sdusize	Specified SDU size (-sdusize argument)
  * RINACAT_l		If running as a server, “l”, else null (-l option)
  * RINACAT_v		Verbosity (0 = none, 1 = most, 2 = all) (-v and -V options)

_Server (on razzycam4):_  
	# rina-cat -l -c 'echo -A $RINACAT_A -a $RINACAT_a -d $RINACAT_d -sdusize $RINACAT_sdusize -l $RINACAT_l -v $RINACAT_v'

_Output at client (on razzycam2):_  
 	# rina-cat -a razzycam4.rina-cat  
-A razzycam4.rina-cat -a razzycam2.rina-cat -d -sdusize 4096 -l l -v 0  
	# 

*(Note that in this example some parameters, such as the -A application name, have been allowed to default.)*

## Misc. Notes, as of current version:
  * rina-cat has been tested with TCP sockets, but for now is not heavily tested with RINA flows.  If you would like to build it with TCP support instead of RINA, let me know and I’ll provide you the substitute i/o library.  
  * rina-cat doesn’t yet allow setting flow properties — it requests a reliable, in-order, stream flow for maximum compatibility with existing Linux commands.  
  * rina-cat lets a server specify the sole application name that is allowed to connect to it (by using the normally-optional-in-server-mode -a option), and will refuse flow requests from any other app.  This is untested.  It should include some wild-card name flexibility, and include setting requirements on incoming flow specs (e.g., reliable vs unreliable), via command-line options since only the person specifying the application receiving rina-cat’s output knows what it needs.  Details will be finalized when the behavior of the rina-api library with application and AE names and instances becomes clearer.  
  * rina-cat does not modify default signal behavior for rina-cat or launched commands.  
  * Exception handling is minimal, so any exceptions thrown by internal libraries used by the RINA API will be minimally documented in error output.  
  * If an application name for the rina-ccat instance (-A option) isn't provided, one is invented: it is the name of the host running the command, a dot, and the name of the command (rina-cat).
  * As a convenience in scripting, if an application instance is provided on the command line (-I or -i options), the name provided by the -A or -a option, respectively, is appended with "|" (the rina-api default separator), then the instance argument.  Users of the command can instead use that syntax themselves with the -A or -a option to add application instance, application entity (AE), and application entity instance to the application name.
## The tar file:
 * The tar file contents can be installed in …/stack/rina-tools/src/rina-cat, a new directory, where it can become part of the normal user-level build process, or can be built outside the tree using the included makefile if desired.  The contents include:
   * .cc and .h source files
   * build-changes: A git diff showing the two one-line edits to add rina-cat to the build process.
   * Makefile.am: The config file to build rina-cat as part of the normal build.
   * makefile: A quick-and-dirty makefile to allow building rina-cat outside the irati build process. You may have to edit it to locate the tclap header files, and/or may have to run as root to reach them (comments included should be self-explanatory).  (Not needed if installed in the tree.)

