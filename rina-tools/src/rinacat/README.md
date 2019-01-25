# rinacat -- A RINA Flow Redirection Utility

### Steve Bunch, 20 September 2017

## rinacat Operation:

### Overview
rinacat copies bidirectionally between a RINA flow it creates and its own standard input and standard output files or a forked command.  rinacat can be used in either client (initiator) or server (listener/target) mode.  Using standard UNIX/Linux commands, rinacat can implement a bi-directional chat, perform file copying, launch server instances, and perform many other functions without writing custom programs.  See the Examples.  To get a list of the command-line options, run __rinacat --help__

rinacat by default copies data to/from its own standard input and output, but can also instead launch programs when run in either client or server mode, forking and exec'ing a separate shell command line whose standard input and output are redirected to the RINA flow that rinacat sets up.  In server mode, rinacat can launch a single command and then terminate, or persist and launch an independent process for each incoming flow request, allowing up to a set maximum number of simultaneous processes.

### Client Mode
In client mode, the default, rinacat attempts to create a RINA flow to a specified destination RINA application.  If successful, in its simplest mode of operation, it then copies its own standard input to the flow, and simultaneously reads from the flow and copies to its own standard output file.  An EOF indication or error on any file terminates the execution.  Optionally, rather than copying its own standard input/output, the client can launch a separate command line, provided on the rinacat command line using the -c option.  rinacat will connect the flow to the command line’s standard input and output, launch it, wait for the command to complete, then terminate.

### Server Mode
In server(listen) mode, designated using the -l option, rinacat registers an application name, then awaits incoming flow requests.  When one is received, it vets it (more below on that) and if happy with the request, accepts the flow.  In its simplest mode of operation, it then copies data from the flow to its standard output, simultaneously reads from its standard input and copies it to the flow, and when EOF or error is seen on any file, terminates.  Rather than copying to/from its own standard output/input, using the -c option rinacat can instead launch a separate command line and attach the flow to the command line's standard input and output, and let that command handle the flow and terminate when the client application completes.  If persistence option -p is specified, rather than terminating after the launched command line completes, the server rinacat will instead remain operating indefinitely, launching and monitoring up to the specified maximum number of simultaneous commands as flow requests arrive.  Incoming flow requests that would cause the number of active commands to exceed the command-line-set limit will be refused.

## Misc. Details
When launching a separate Linux application in either client or server mode, rinacat does not redirect the standard error output file, so errors from the rinacat program as well as any children will NOT go over the RINA flow.  Stderr for a launched command can be redirected on its command line if desired.

Commands launched by rinacat are placed into the same process group as rinacat, so signals such as SIGINT (control-C) sent to rinacat will also be sent to any running command(s), and the launched commands have access to rinacat's controlling tty.  If this behavior is not desired, the commands are free to detach themselves from the process group (or we can in future provide an auto-detach option to rinacat.)

The command line provided to rinacat via the -c option is sent to the current default shell ($SHELL) for execution, using its -c command option.  The effect is essentially the same as:
	$SHELL -c “string provided using rinacat -c option”
Therefore, the command line can use environment variables, pipes, redirection, or any other typical shell command line function.  Some of the command line options of rinacat as well as information about incoming connections to a server are provided to commands via environment variables (see example below).  Users need to be cognizant of shell quoting conventions when specifying commands whose arguments include strings or environment variables.

rinacat does not modify default signal behavior for rinacat or launched commands.

If an application name for the rinacat instance (-A option) isn't provided, one is provided: it is the name of the host running the command, a dot, and either rinacat or the name of the command supplied in the -c option if present.

As a convenience for scripting, if an application instance is provided on the command line (-I or -i options), the name provided by the -A or -a option, respectively, is appended with "|" (the rina-api default separator), then the instance argument.  Users of the command can instead use that syntax themselves with the -A or -a option to add application instance, application entity (AE), and application entity instance to the application name.


## Examples:

***Simple two-way “chat” session between a client and server:  Copy input from standard input on each system to standard output on the other.***

_Server:_
	# rinacat -l -A chat_app  
_Client:_
	# rinacat -a chat_app 

Input typed to the standard input of the client instance will be sent over the RINA flow and sent to standard output of the server instance, and vice versa.  (The -d DIFNAME option is not generally needed unless multiple DIFs are available and the user cares which is used.)  An EOF (control-D) at either end will terminate both applications above and end the conversation.  The server could be made persistent, e.g., with -p 1, in which case it would remain active and listening for future client sessions.


***Simple echo test:  Standard input on one system is remotely echoed back to the sender's stdout.***

_Server:_
    # rinacat -l -A echo_app -c "cat"  
_Client:_
    # rinacat -a echo_app  

Anything typed at the client instance will be sent to the server, bounced back, and printed out unchanged.


***To do a file copy, just redirect the input and output to files:***

_Server:_
	# rinacat -l -A copy_app >filename  
_Client:_
	# rinacat -a copy_app <filename  


Note that the copy could have instead been done in the other direction.  (Copying files simultaneously in both directions will likely result in one file being truncated, as rinacat terminates as soon as any file read hits EOF.)


***Copy a video stream from a Raspberry Pi camera to the display of another computer (e.g., another Raspberry Pi or other Linux):***

_Server:_
	# rinacat -l -A display | mplayer -fps 10 -cache 32 -vo x11 -  
_Client:_
	# raspivid -fps 10 -w 800 -h 600 -t 0 -o - | rinacat -a display -d  


***Run a persistent video player (compared to the previous example, this pattern doesn’t buffer as much data in pipes and so improves efficiency, allows the player to be persistent, and allows destination application output to be fed back to the source, in cases where that’s useful).***

_Server (persistent, allowing one active flow at a time):_  
	# rinacat -l -A video-player -p 1 -c "mplayer -fps 11 -cache 512 -vo x11 -  2>/dev/null"  
_Client:_  
	# raspivid -fps 10 -w 800 -h 600 -t 0 -o - | rinacat -a video-player   

*(NOTE: The raspivid program does not read from its stdin file, but the mplayer program outputs status information about the video stream, so directly connecting the two by using _rinacat -c "command"_ at both ends to bidirectionally connect mplayer with raspivid will NOT work -- the unread mplayer output will build up until something breaks.  So a sink must be provided for mplayer's output.  In this example, that is provided by the rinacat client instance, whose stdout is not redirected by the shell when setting up the pipe and therefore will print the status information from the player at the client end.  That output could instead be redirected, e.g., to /dev/null, as could the mplayer stdout as is shown for stderr.)*

***Stream a video from one computer and display it in another one (stream to stdout, redirect to rinacat in one end | make video player read from stdin in the other end):***

_Sever:_
       # rinacat -l -A display | mplayer -fps 30 -cache 64 -vo x11 -

_Client:_
       # avconv -re -i <video_file_name> -codec copy -movflags frag_keyframe+empty_moov -f mp4 - | rinacat -a display

***Commands executed by rinacat have access via environment variables to information about the rinacat instance that ran them.  In particular (experimental, details subject to change):***  
  * RINACAT_A		Name of this rinacat instance (-A argument)
  * RINACAT_a		Name of other rinacat instance (-a argument, or incoming flow)
  * RINACAT_d		Name of dif to use (-d argument)
  * RINACAT_sdusize	Specified SDU size (-sdusize argument, or default)
  * RINACAT_l		If running as a server, “l”, else null (-l option)
  * RINACAT_v		Verbosity of debug information (0 = none, 1 = a lot, >=2 = even more) (-v(1) and -V(2) options, which can be repeated)
  * RINACAT_reliability	Flow reliability: "d", "r", or "u" for default, reliable, or unreliable (best effort), respectively
  * RINACAT_delimiting	Delimiting between records, "s" for stream (records run together), "r" for record/SDU mode


_Server (on razzycam4):_  
	# rinacat -l -c 'echo rinacat-args -A $RINACAT_A -a $RINACAT_a -d $RINACAT_d -sdusize $RINACAT_sdusize -l $RINACAT_l -v $RINACAT_v'

_Output at client (on razzycam2):_  
 	# rinacat -a razzycam4.echo  
rinacat-args -A razzycam4.echo -a razzycam2.rinacat -d -sdusize 4096 -l l -v 0
	# 

*(Note that in this example some parameters, such as the -A application name, have been allowed to default.)*

## Misc. Notes, as of current version:
  * rinacat now allows setting some flow properties, see --help for details.  It by default requests a reliable, in-order, stream flow for maximum compatibility with existing Linux commands.  Flow reliability can be overridden with -u (unreliable) or --defaultFS (the system-provided default, whatever it is).   
  * rinacat lets a server specify the sole application name that is allowed to connect to it (by using the normally-optional-in-server-mode -a option), and will refuse flow requests from any other app.  This is untested, and unlikely to work as expected at this time.  It will be enhanced to include some wild-card name flexibility, and could include setting requirements on incoming flow specs (e.g., reliable vs. unreliable, stream vs. record), via command-line options since the person specifying the application receiving rinacat’s output knows what it needs.  Details will be finalized when the behavior of the rina-api library with application and AE names and instances becomes clearer and we gain experience with what is found useful.  
  * Note that the --sdusize value used must be smaller than the MTU of the underlying transport(s) if fragmentation isn't in use.  The default sdusize is currently set to 1024 to make it unnecesssary to override it on the command line when transporting over Ethernet without fragmentation -- a larger value would improve performance, but could cause overly-long SDUs to be dropped.

