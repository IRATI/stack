#############################################################################
## Table of contents                                                        #
#############################################################################

* 1. Introduction
* 2. Software requirements
* 3. Build instructions
* 4. Configuration files
* 5. Tutorials
* 6. Overview of the software components
    * 6.1. Kernel modules
    * 6.2. Userspace IPCP daemons
    * 6.3. Libraries
    * 6.4. IPC Manager Daemon
    * 6.5. Other tools

#############################################################################
## 1. Introduction                                                          #
#############################################################################

IRATI is an open source implementation of the RINA architecture targeted to the OS/Linux 
system, initially developed by the FP7-IRATI project (for information about RINA please 
visit http://pouzinsociety.org).  This README file and the following 
wiki pages provide information on using IRATI and understanding its design:

* *Software Architecture Overview*: Explanation of the different components of IRATI.
See https://github.com/IRATI/stack/wiki/Software-architecture-overview.
* *Getting Started*: How to install and run IRATI. See https://github.com/IRATI/stack/wiki/Getting-Started
* *IRATI in depth*: Detailed explanation of how specific aspects of the IRATI 
implementation work. See https://github.com/IRATI/stack/wiki/IRATI-in-depth
* *Tutorials*: Step-by-step experimentation scenarios. See https://github.com/IRATI/stack/wiki/Tutorials

A public mailing list, is available here: http://www.freelists.org/list/irati. The list is to be used as a 
means for communication with the IRATI open source development community.

The current implementation is mature enough to allow small/mid scale experimentation (up to 50-70 systems, 
with each system having up to 10 IPC Processes instantiated), during relatively short periods of time (hours, 
up to a day depending on the experiment tested). We are working on improving the stability and robustness 
of IRATI in future releases.
