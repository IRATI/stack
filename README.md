#############################################################################
## Table of contents                                                        #
#############################################################################

* 1. Introduction
* 2. Software requirements
* 3. Build instructions
* 4. Running and configuring IRATI
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

* **Software Architecture Overview**: Explanation of the different components of IRATI.
See https://github.com/IRATI/stack/wiki/Software-architecture-overview.
* **Getting Started**: How to install and run IRATI. See https://github.com/IRATI/stack/wiki/Getting-Started
* **IRATI in depth**: Detailed explanation of how specific aspects of the IRATI 
implementation work. See https://github.com/IRATI/stack/wiki/IRATI-in-depth
* **Tutorials**: Step-by-step experimentation scenarios. See https://github.com/IRATI/stack/wiki/Tutorials

A public mailing list, is available here: http://www.freelists.org/list/irati. The list is to be used as a 
means for communication with the IRATI open source development community.

The current implementation is mature enough to allow small/mid scale experimentation (up to 50-70 systems, 
with each system having up to 10 IPC Processes instantiated), during relatively short periods of time (hours, 
up to a day depending on the experiment tested). We are working on improving the stability and robustness 
of IRATI in future releases.

#############################################################################
## 2. Software requirements                                                 #
#############################################################################

This section lists the software packages required to build and run *IRATI* on
Linux-based operating systems. Only Debian 7 is explicitly
indicated here, but using other distributions should be equally
straightforward.

### Debian 7
#############################################################################

For the kernel parts, the following packages are required:

* kernel-package
* libncurses5-dev (needed for make menuconfig)

For the user-space parts, the following packages from the default repository are required:

* autoconf
* automake
* libtool
* pkg-config
* git
* g++
* libssl-dev
* openjdk-6-jdk (only if generation of Java bindings is required)
* maven (only if generation of Java bindings is required)

Required packages from the testing repository:

* Add the testing repository
* Add deb http://ftp.de.debian.org/debian jessie main in /etc/apt/sources.list
* Run apt-get update
* protobuf-compiler, libprotobuf-dev (version >= 2.5.0 required)
* libnl-genl-3-dev and libnl-3-dev (version >= 3.2.14 required):

Required packages to be build from source (only if generation of Java bindings is required)

* SWIG (version >= 2.0.8 required, 2.0.12 is known to be working fine) from http://swig.org

To use the shim-eth-vlan module you also need to install the *vlan* package.


#############################################################################
## 3. Build instructions                                                       #
#############################################################################

Download the repo and enter the root directory

    $ git clone https://github.com/IRATI/stack.git
    $ cd stack

Enter the linux folder and copy default kernel build config file to *.config* 
(you can further customize the kernel build config by running *make menuconfig* from 
the same folder)

    $ cd linux
    $ copy config-IRATI .config

Build and install both kernel-space and user-space software

    $ ./install-from-scratch --prefix=<path to IRATI installation folder>

Once all the build process has finalized (it will take some time, since a whole 
kernel has to be compiled), reboot your computer.


#############################################################################
## 4. Running and configuring IRATI                                         #
#############################################################################

### 4.1. Loading the required Kernel modules
#############################################################################
First the appropriate kernel space components have to be modprobed. If you want to use a 
normal IPC process use:

    $ modprobe normal-ipcp

Currently, there are 3 shim IPC processes available in IRATI:

* **The shim IPC process for 802.1Q (VLANs)**, which requires a virtual interface to be 
created on a VLAN before being inserted as a Linux kernel module. The VLAN id is a synonym 
for the DIF name. **NOTE**: If you are installing IRATI in a VirtualBox VM, please do not 
use the Intel PRO/1000 net card adaptor since it strips the VLAN tags.
* **The shim IPC process for TCP/UDP**.
* **The shim IPC process for Hypervisos**, which supports communication between a virtual 
machine and its host.

Each one can be loaded with the correct command:

    $ modprobe shim-eth-vlan
    $ modprobe shim-tcp-udp
    $ modprobe shim-hv

**NOTE**: Please notice that before loading the shim-eth-vlan module, the VLAN interface 
must be created and up.

Next, the IPC Manager (IPCM) has to be started in userspace, which is the local management agent. 
The IPCM needs some configuration information.

### 4.2 The IPC Manager configuration files
#############################################################################

#### 4.2.1 Main configuration file
The main configuration file is located in your `INSTALLATION_PATH/etc/ipcmanager.conf`. It contains 
instructions to optionally instantiate and configure a number of IPC Processes when the IPC Manager 
Daemon starts its execution.

**Local configuration**. The first part of the configuration file contains the settings for IRATI, 
such as the paths to the  UNIX socket for the local console or the paths where to search for 
user-space or kernel plugins.

      "configFileVersion" : "1.4.1",
      "localConfiguration" : {
        "installationPath" : "/usr/bin",
        "libraryPath" : "/usr/lib",
        "logPath" : "/var/log",
        "consoleSocket" : "/var/run/ipcm-console.sock",
        "pluginsPaths" : ["/usr/lib/rinad/ipcp"]
      },

**IPC Processes to create**. The next section specifies which IPC processes should be created. 
It requires for each IPC process the type, which can be either a normal IPC process, or a certain 
shim IPC process. The names of the IPC process and the DIF then have to be specified. The name of 
the DIF the IPC process should register with is also supplied. Enrollment however, will have to be 
done manually from the local management console.

    "ipcProcessesToCreate" : [ {
      "type" : "shim-eth-vlan",
      "apName" : "test-eth-vlan",
      "apInstance" : "1",
      "difName" : "110"
     }, {
      "type" : "normal-ipc",
      "apName" : "test1.IRATI",
      "apInstance" : "1",
      "difName" : "normal.DIF",
      "difsToRegisterAt" : ["110"]
     } ],

**DIF Configurations**. This only specifies what DIFs to create, but it does not yet explain how 
the DIFs should be configured. Thats why there is a section called difConfigurations, which specifies 
what is the DIF template file for each of the DIF names in the main configuration file (the same 
template file can be used for multiple DIFs). DIF template files contain the actual configuration 
of the DIF, including its policies.

    "difConfigurations" : [ {
        "name" : "110",
        "template" : "shim-eth-vlan.dif"
    }, {
        "name" : "normal.DIF",
        "template" : "default.dif"
    } ]

#### 4.2.2 DIF Template configuration files
DIF template files contain the configuration of the components of a DIF. There is a mandatory DIF 
template called "default.dif" (which gets installed during the IRATI installation procedure), all 
other DIF templates extend from it (in the sense that they only need to define the JSON sections 
that are different from the default.dif file). All DIF templates have to be located in the same 
folder as the main configuration files (.conf) that use the templates.

For exmples of different JSON configuration files, you can take a look at 
https://github.com/IRATI/stack/tree/master/tests/conf.

#### 4.2.3 Application to DIF mappings
The da.map file contains the preferences for which DIFs should be used to register and to allocate 
flows to/from specific applications. If no mapping is provided by a certain application, it will try 
to randomly select a _normal DIF_ first; if there is non available a _shim DIF_ and if there is none 
it will fail. The contents of the da.map file can be modified while the IPC Manager Daemon is running.

    "applicationToDIFMappings": [
        {
            "encodedAppName": "rina.apps.echotime.server-1--",
            "difName": "dcfabric.DIF"
        },
        {
            "encodedAppName": "rina.apps.echotime.client-1--",
            "difName": "dcfabric.DIF"
        },
        {
            "encodedAppName": "rina.apps.echotime-2--",
            "difName": "vpn1.DIF"
        },
        {
            "encodedAppName": "rina.apps.echotime.client-2--",
            "difName": "vpn1.DIF"
        }
    ],

### 4.3 Running the IPC Manager Daemon
#############################################################################
Once the configuration file is ready you can un the IPC Manager Daemon. To do so go to the 
INSTALLATION_PATH/bin folder and type:

    $ ./ipcm -c <PATH TO THE CONFIGURATION FILE>

There are more options for the ipcm launch script, to see them just type

    $ ./ipcm --help

IRATI has a local management console to interact with the IPC Manager Daemon and get information 
on the state of the software (number of IPCPs, type, status, DIF information, etc.). To access the 
management console type

    $ socat - UNIX:/<INSTALLATION_PATH>/var/run/ipcm-console.sock

Type help to get an overview of all available commands:

* **help**: Show the list of available commands or the usage of a specific command.
* **exit or quit**: Exit the console.
* **create-ipcp**: Create a new IPC process.
* **destroy-ipcp**: Destroy an existing IPC process.
* **list-ipcps**: List the existing IPC processes with associated information.
* **list-ipcp-types**: List the IPC process types currently available in the system.
* **assign-to-dif**: Assign an IPC process to a DIF.
* **register-at-dif**: Register an IPC process within a DIF. 
* **unregister-from-dif**: Unregister an IPC process from a DIF.
* **enroll-to-dif**: Enroll an IPC process to a DIF.
* **query-rib**. Display the information of the objects present at RIB of an IPC Processs.
* **query-ma-rib**. Display the information of the objects present at the RIB of a management agent.
* **show-dif-templates**. Display the DIF templates known by the IPC Manager.
* **show-catalog**. Show the catalog of plugins available in the system, with their policy-sets.
* **update-catalog**. Tell the IPC Manager to refresh its list of known plugins.
* **plugin-get-info**. Get information about a plugin (policy-sets, versions)
* **plugin-load**. Load a plugin.
* **plugin-unload**. Unload a plugin.
* **select-policy-set**. Select a policy set for a specific component of a specific IPC Process in the system.
* **set-policy-set-param**. Select a parameter of a specific policy-set for a specific component of a specific IPC Process in the system.

Example of IPCM console output:

    IPCM >>> list-ipcps
    Management Agent not started

    Current IPC processes (id | name | type | state | Registered applications | Port-ids of flows provided)
        1 | eth.1.IPCP:1:: | shim-eth-vlan | ASSIGNED TO DIF 2003 | renumber.19.IPCP-1-- | 2
        2 | eth.2.IPCP:1:: | shim-eth-vlan | ASSIGNED TO DIF 2005 | renumber.19.IPCP-1-- | 1
        3 | eth.3.IPCP:1:: | shim-eth-vlan | ASSIGNED TO DIF 2006 | renumber.19.IPCP-1-- | 4
        4 | eth.4.IPCP:1:: | shim-eth-vlan | ASSIGNED TO DIF 2007 | renumber.19.IPCP-1-- | 3
        5 | renumber.19.IPCP:1:: | normal-ipc | ASSIGNED TO DIF renumber.DIF | - | -

Now applications can be run that use the IPC API. Look at the Tutorials section for some step-by-step 
examples on how to use the rina-echo-time test application to experiment with IRATI.

#############################################################################
## 5. Tutorials                                                             #
#############################################################################
Several tutorials are available at https://github.com/IRATI/stack/wiki/Tutorials

#############################################################################
## 6. Overview of the software components                                   #
#############################################################################
This section provides an overview of the software architecture and components of IRATI. For a more detailed 
explanation we direct the reader to FP7-IRATI's at http://irati.eu:
 
* D3.1: http://irati.eu/wp-content/uploads/2012/07/IRATI-D3.1-v1.0.pdf 
* D3.2: http://irati.eu/wp-content/uploads/2012/07/IRATI-D3.2-v1.0.pdf 
* D3.3: http://irati.eu/wp-content/uploads/2012/07/IRATI-D3.3-bundle.zip 

The software architecture of IRATI is shown in Figure 1. 

![Figure 1. Main software components of the RINA implementation by the FP7-IRATI project](https://github.com/IRATI/stack/wiki/images/irati-softarch.png)
_Figure 1. Source: [S. Vrijders et al; "Prototyping the recursive internet architecture: the IRATI project approach ", IEEE Network Vol 28 (2), pp. 20-25, March 2014](http://ieeexplore.ieee.org/xpl/articleDetails.jsp?arnumber=6786609)_
