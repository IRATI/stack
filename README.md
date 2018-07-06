## Table of contents
````

1. Introduction
2. Build instructions
    2.1 Building on Ubuntu 16.04, Debian 8 and Debian 9
    2.2 Building on Arch Linux
    2.3 Building on Raspbian
3. Running and configuring IRATI
    3.1 Loading the kernel modules
    3.2 The configuration files
    3.3 Running the IPC Manager Daemon
4. Tutorials
5. Overview of the software components
````

## 1. Introduction

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

## 2. Build instructions

Build Status of the Master branch on Debian via Gitlab Runner.

[![pipeline status](https://gitlab.com/arcfire/irati/stack/badges/master/pipeline.svg)](https://gitlab.com/arcfire/irati/stack/commits/master)

Build Status of arcfire branch on Debian via Gitlab Runner.

[![pipeline status](https://gitlab.com/arcfire/irati/stack/badges/arcfire/pipeline.svg)](https://gitlab.com/arcfire/irati/stack/commits/arcfire)

Build Status of the Master branch on Ubuntu via Travis CI.

[![Build Status of the Master branch on Ubuntu](https://travis-ci.org/IRATI/stack.svg?branch=master)](https://travis-ci.org/IRATI/stack)

Build Status of arcfire branch on Ubuntu via Travis CI.

[![Build Status of arcfire branch on Ubuntu](https://travis-ci.org/IRATI/stack.svg?branch=arcfire)](https://travis-ci.org/IRATI/stack)


### 2.1. Building on Ubuntu 16.04, Debian 8 and Debian 9

**NOTE for Debian 8**: For the kernel modules, a Linux kernel with a version between 4.1 
and 4.9 (included) has to be installed in the system, with the kernel headers. Ubuntu 
16.04 already comes with Linux kernel 4.4, therefore no new kernel needs to be installed. 

Once this is done, please install user-space dependencies

    $ apt-get update
    $ apt-get install autoconf automake libtool pkg-config git g++ libssl-dev protobuf-compiler libprotobuf-dev socat python linux-headers-$(uname -r)
    $ apt-get install hostapd (if the system will be configured as an access point)
    $ apt-get install wpasupplicant (if the system will be configured as a mobile host)

Download the IRATI repo (arcfire branch) and enter the root directory

    $ git clone -b arcfire https://github.com/IRATI/stack.git
    $ cd stack

Build and install both kernel-space and user-space software

    $ sudo ./configure --prefix <path to IRATI installation folder>
    $ sudo make install

(`sudo` is not necessary in the `./configure` phase if the current user has
write permissions on the directory specified with `--prefix`).

### 2.2. Building on Arch Linux

Install user-space dependencies

    $ pacman -S autoconf automake autoconf libtool pkg-config git openssl protobuf socat python linux-headers
    $ pacman -S hostapd (if the system will be configured as an access point)
    $ pacman -S wpa_supplicant (if the system will be configured as a mobile host)

Download the IRATI repo (arcfire branch) and enter the root directory

    $ git clone -b arcfire https://github.com/IRATI/stack.git
    $ cd stack

Build and install both kernel-space and user-space software

    $ sudo ./configure --prefix <path to IRATI installation folder>
    $ sudo make install

(`sudo` is not necessary in the `./configure` phase if the current user has
write permissions on the directory specified with `--prefix`).

### 2.3. Building on Raspbian

(Tested with Raspberry Pi model 3B)

Insert the SD card into the Raspberry Pi and power it on. Log in with user 'pi' and 
password 'raspberry'.  As root ("sudo su -" or equivalent):

Check your kernel version (via uname -r), if it is not at least 4.9.24-v7+, update the distro

    $ apt-get update
    $ apt-get dist-upgrade
    $ reboot

Install dependencies

    $ apt-get install raspberrypi-kernel-headers socat
    $ apt-get install hostapd (if the system will be configured as an access point)
    $ apt-get install wpa-supplicant (if the system will be configured as a mobile host)
    $ apt-get install autoconf libtool git libssl-dev protobuf-compiler libprotobuf-dev

Download the IRATI repo (arcfire branch) and enter the root directory

    $ git clone -b arcfire https://github.com/IRATI/stack.git
    $ cd stack

Build and install both kernel-space and user-space software

    $ sudo ./configure --prefix <path to IRATI installation folder>
    $ sudo make install

(`sudo` is not necessary in the `./configure` phase if the current user has
write permissions on the directory specified with `--prefix`).

## 3. Running and configuring IRATI

### 3.1. Loading the required Kernel modules

To load the IRATI kernel modules, just call the load-rina-modules script:

    $ ./load-irati-modules

Next, the IPC Manager (IPCM) has to be started in userspace, which is the local management agent. 
The IPCM needs some configuration information.

### 3.2 The IPC Manager configuration files

#### 3.2.1 Main configuration file
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

#### 3.2.2 DIF Template configuration files
DIF template files contain the configuration of the components of a DIF. There is a mandatory DIF 
template called "default.dif" (which gets installed during the IRATI installation procedure), all 
other DIF templates extend from it (in the sense that they only need to define the JSON sections 
that are different from the default.dif file). All DIF templates have to be located in the same 
folder as the main configuration files (.conf) that use the templates.

For exmples of different JSON configuration files, you can take a look at 
https://github.com/IRATI/stack/tree/master/tests/conf.

##### 3.2.2.1 Data Transfer Constants
Customize the length of the header fields in EFCP data transfer (**address**, **cep-id**, **length**, 
**port-id**, **sequence number**, **qos-id**) and control (**rate**, **frame**, **control seq. number**) 
PDUs. Set the **maximum PDU size** and **maximum PDU lifetime** for the DIF.

    "dataTransferConstants" : {
    	"addressLength" : 2,
    	"cepIdLength" : 2,
    	"lengthLength" : 2,
    	"portIdLength" : 2,
    	"qosIdLength" : 2,
    	"rateLength" : 4,
    	"frameLength" : 4,
    	"sequenceNumberLength" : 4,
    	"ctrlSequenceNumberLength" : 4,
    	"maxPduSize" : 1470,
    	"maxPduLifetime" : 60000
    },

##### 3.2.2.2 QoS Cubes
Define what are the characteristics of the different QoS cube supported by the DIF,
as well as their associated EFCP policies: DTP policy set (data transfer policy set) 
and DTCP policy set (data transfer policy set).

    "qosCubes" : [ {
	 "name" : "unreliablewithflowcontrol",
         "id" : 1,
         "partialDelivery" : false,
         "orderedDelivery" : true,
         "efcpPolicies" : {
              "dtpPolicySet" : {
                "name" : "default",
                "version" : "0"
              },
              "initialATimer" : 300,
              "dtcpPresent" : true,
              "dtcpConfiguration" : {
                   "dtcpPolicySet" : {
                     "name" : "default",
                     "version" : "0"
                   },
                   "rtxControl" : false,
                   "flowControl" : true,
                   "flowControlConfig" : {
                       "rateBased" : false,
                       "windowBased" : true,
                       "windowBasedConfig" : {
                         "maxClosedWindowQueueLength" : 50,
                         "initialCredit" : 50
                        }
                   }
              }
          }
       }, {
       "name" : "reliablewithflowcontrol",
       ...
       } ]

   * **Name**: the name of the qos cube (a string)
   * **id**: the id of the qos cube (an unsigned integer)
   * **partialDelivery**: true/false depending if delivery of partial SDUs is supported
   * **orderedDelivery**: true/false depending if in order delivery of SDUs is required
   * **initialATimer**: initial value of the A timer, in ms
   * **dtpPolicySet**: name and version of the DTP policy set associated to this QoS cube
   * **dtcpPresent**: true if a DTCP instance is required for every DTP instance, false otherwise
   * **dtcpPolicySet**: name and verison of the DTCP policy set associated to this QoS cube
   * **rtxControl**: true if DTCP performs rtx control, false otherwise
   * **flowControl**: true if DTCP performs flow control, false otherwise
   * **rateBased**: true if DTCP performs rate-based flow control, false otherwise
   * **windowBased**: true if DTCP performs window-based flow control, false otherwise
   * **maxClosedWindowQueueLength**: maximum length of the closed window queue
   * **initialCredit**: initial credit of the window (only if window-based flow control is used)

The following DTCP policy sets are available in IRATI

###### 3.2.2.2.1 Default policy
Very simple DTCP policies that implement retransmission control and a sliding window 
flow control policy with a fixed size window (configurable via the **initialCredit** parameter).
The DTCP flow control policy does not react to congestion, packet loss or RTT delay variation.

   * **Policy name**: default.
   * **Policy version**: 0.
   * **Dependencies**: None.

Example configuration:

    "dtcpPolicySet" : {
        "name" : "default",
        "version" : "0"
    }

###### 3.2.2.2.2 DECNET binary feedback congestion control
Extends the default DTCP policies by reacting to congestion and adapting the window size, based 
on Raj Jain's binary feedback congestion control (additive increase, multiplicative decrease).
Since the policy relies on Explicit Congestion Notification (ECN), it must be used in conjunction 
with its companion RMT policy (cas-ps).

   * **Policy name**: cas-ps.
   * **Policy version**: 1.
   * **Dependencies**:
      * **RMT policy**: cas-ps.

Example configuration:

    "dtcpPolicySet" : {
        "name" : "cas-ps",
        "version" : "1",
        "parameters" : [{
           "name"  : "w_inc_a_p",
           "value" : "1"
        }]
    }

   * **w_inc_a_p**: Number of units by which the window size will be increased under "additive
increase" state (the default value is **1**)

###### 3.2.2.2.3 TCP ECN congestion control 
Extends the default DTCP policies by reacting to congestion and adapting the window size, based
on TCP Reno's congestion control strategy (but using Explicit Congestion Notification instead 
of packet loss as a sign of congestion). Since the policy relies on Explicit Congestion 
Notification (ECN), it must be used in conjunction with an ECN-marking RMT policy (such as 
cas-ps or red-ps).

   * **Policy name**: red-ps.
   * **Policy version**: 1.
   * **Dependencies**:
      * **RMT policy**: red-ps or cas-ps.

Example configuration:

    "dtcpPolicySet" : {
        "name" : "red-ps",
        "version" : "1"
    }

###### 3.2.2.2.4 Data Center TCP congestion control
Extends the default DTCP policies by reacting to congestion and adapting the window size, based
on the Data Center TCP (DCTCP) scheme. The goal of DCTCP is to achieve high burst tolerance, 
low latency, and high throughput, with commodity shallow buffered switches. DCTCP achieves these 
goals primarily by reacting to congestion in proportion to the extent of congestion. Since the 
policy relies on Explicit Congestion Notification, it must be used with its companion ECN-marking 
RMT policy (dctcp-ps).

   * **Policy name**: dctcp-ps.
   * **Policy version**: 1.
   * **Dependencies**:
      * **RMT policy**: dctcp-ps.
        
Example configuration: 
           
    "dtcpPolicySet" : {
        "name" : "dctcp-ps",
        "version" : "1",
        "parameters" : [{
           "name"  : "shift_g",
           "value" : "4"
        }]
    }

   * **shift_g**: According the DCTCP paper, the g value should be small enough and all experiments 
in the paper use `g = 0.0625 (1/16)`. Thus, the `shift_g = 4` is `2^4 = 16` (the default value is **4**)

##### 3.2.2.3 Known IPC Process addresses
IRATI only supports a very simple static address allocation policy right now. The configuration file 
defines a mapping betwenn the IPC Process application names and its address. It assumes that the 
IPC Process name is of the form *<name.Organization>*, so that if a specific entry for a name is not 
found but the configuration file defines what prefix is assgined to the *Organization*, then an address 
of the *Organization* range can still be assigned.

Example configuration:

     "knownIPCProcessAddresses" : [ {
         "apName" : "C.IRATI",
         "apInstance" : "1",
         "address" : 3
          }, {
         "apName" : "D.IRATI",
         "apInstance" : "1",
         "address" : 4
        } ],
        "addressPrefixes" : [ {
         "addressPrefix" : 0,
         "organization" : "N.Bourbaki"
          }, {
         "addressPrefix" : 16,
         "organization" : "IRATI"
      } ]

##### 3.2.2.4 Relaying and Multiplexing Task
The Relaying and Multiplexing Task (RMT) configuration. The behaviour of the RMT is controlled by 
two policies: the forwarding function policy (which specifies how PDUs are forwarded), and the 
RMT policy (which controls the number of queues at the RMT, its monitoring and how PDUs are scheduled).

     "rmtConfiguration" : {
        "pftConfiguration" : {
          "policySet" : {
            "name" : "default",
            "version" : "0"
          }
        },
        "policySet" : {
          "name" : "default",
          "version" : "1"
        }
     }

IRATI supports the following Forwarding and RMT policies

###### 3.2.2.4.1 Forwarding policy: default
The default forwarding policy is based on a forwarding table that maps the *destination address* and 
*qos_id* fields of PDUs to an N-1 port. The table lookup is based on a exact match of the destination 
address and qos_id fields (except if the qos_id value in the forwarding table is 0, then the qos_id 
value is ignored).

   * **Policy name**: default.
   * **Policy version**: 1.
   * **Dependencies**: none.

Example configuration:
    
    "pftConfiguration" : {
        "policySet" : {
            "name" : "default",
            "version" : "0"
         } 
     }

###### 3.2.2.4.2 Forwarding policy: multi-path
This policy extends the default forwarding policy with multi-path capabilities. If multiple N-1 flows 
are assigned to the same destination address and qos_id, this policy applies a hash-threshold algorithm 
(the one defined in the ECMP specification) to load-balance PDUs amongst the multiple N-1 flows. The 
algorithm preserves PDU packet ordering by forwarding PDUs belonging to the same flow through the same 
N-1 port.

   * **Policy name**: multipath.
   * **Policy version**: 1.
   * **Dependencies**: none.
   
Example configuration:
    
    "pftConfiguration" : {
        "policySet" : {
            "name" : "multipath",
            "version" : "1"
         }  
     }

###### 3.2.2.4.3 Forwarding policy: LFA
This policy extends the default forwarding policy with reliability capabilities. The routing algorithm 
populates the forwarding table computing multiple N-1 port-ids per each destination address, using the 
Loop-Free Alternates (LFA) algorithm. One port-id is the active one, and the other port-ids are the backup 
ones. If an N-1 port becames unusable, the forwarding algorithm switches to the backup port-ids for all 
destination addresses where the failing port-id was the active one. 
            
   * **Policy name**: lfa.
   * **Policy version**: 1.
   * **Dependencies**: none.

Example configuration:
     
    "pftConfiguration" : {
        "policySet" : { 
            "name" : "lfa",
            "version" : "1"
         }
     }

###### 3.2.2.4.4 RMT policy: default
The default RMT policy set implements a simple FIFO queue per N-1 port. All the EFCP data transfer and control 
PDUs that need to be forwarded through the N-1 port are put in the FIFO queue. Layer management PDUs are put 
in a separate queue (per N-1 port) that has strict priority over the data transfer FIFO queue. When the queue is 
full, the PDUs that try to be enqueued are dropped. 

   * **Policy name**: default.
   * **Policy version**: 1.
   * **Dependencies**: none.

Example configuration:

     "rmtConfiguration" : {
        "pftConfiguration" : {
            ....
	    }
        },
        "policySet" : {
          "name" : "default",
          "version" : "1",
          "parameters" : [{
             "name"  : "q_max",
             "value" : "1000"
          }]
        }
     }

   * **q_max**: The size of the FIFO queue (in PDUs). The default value is **1000** PDUs.

###### 3.2.2.4.5 RMT policy: DECNET's binary feedback congestion control
This policy extends the RMT default policy by marking queued PDUs with the ECN flag when 
the average queue size is greater than 1 PDU.

   * **Policy name**: cas-ps.
   * **Policy version**: 1.
   * **Dependencies**:
      * **DTCP policy**: cas-ps.

Example configuration:

     "rmtConfiguration" : {
        "pftConfiguration" : {
            ....
            }
        },
        "policySet" : {
          "name" : "cas-ps",
          "version" : "1",
          "parameters" : [{
             "name"  : "q_max",
             "value" : "1000"
          }]
        }
     }

   * **q_max**: The size of the FIFO queue (in PDUs). The default value is **1000** PDUs.

###### 3.2.2.4.5 RMT policy: Random Early Detection (RED)
This policy extends the RMT default policy by marking queued PDUs with the ECN flag according 
to the Random Early Detection (RED) algorithm. This policy marks probabilistically PDUs with 
the ECN flag according to the configured thresholds.

   * **Policy name**: red-ps.
   * **Policy version**: 1.
   * **Dependencies**:
      * **DTCP policy**: red-ps.

Example configuration:

     "rmtConfiguration" : {
        "pftConfiguration" : {
            ....
            }
        },
        "policySet" : {
          "name" : "red-ps",
          "version" : "1",
          "parameters" : [{
             "name"  : "q_max_p",
             "value" : "600"
             }, {
             "name"  : "qth_min_p",
             "value" : "17"
             }, {
             "name"  : "qth_max_p",
             "value" : "179"
             }, {
             "name"  : "Wlog_p",
             "value" : "7"
             }, {
             "name"  : "Plog_p",
             "value" : "12"
          }]
        }
     }

   * **q_max_p**: The size of the FIFO queue (in PDUs). 
   * **qth_min_p**: The size of the FIFO queue where probabilistic marking starts (if the size is smaller, no marking)
   * **qth_max_p**: The size of the FIFO queue where probabilistic marking ends (if the size is larger, always marking)
   * **Wlog_p**: the filter constant, controls intertia of the algorithm (decrease W to allow larger bursts)
   * **Plog_p**: related to the marking probability

###### 3.2.2.4.6 RMT policy: Data Center TCP (DCTCP)
The DCTCP policy for RMT is simple. It is possible to configure a queue size and a threshold. If queue size exceedes 
the threshold, the RMT starts to mark PDUs with explicit congestion flag (ECN).

   * **Policy name**: dctcp-ps.
   * **Policy version**: 1.
   * **Dependencies**:
      * **DTCP policy**: dctcp-ps.
   
Example configuration:

     "rmtConfiguration" : {
        "pftConfiguration" : {
            ....
            }
        },  
        "policySet" : {
          "name" : "dctcp-ps",
          "version" : "1",
          "parameters" : [{
             "name"  : "q_threshold",
             "value" : "20"
             }, {
             "name"  : "q_max",
             "value" : "500"
          }] 
        } 
     }  

   * **q_max**: The maximum size of the queue
   * **q_threshold**: Sets the queue threshold. If the queue size is exceeded, PDUs will be marked with ECN flag.

###### 3.2.2.4.7 RMT policy: QTAMux
Documented in plugins/qtamux

##### 3.2.2.5 Enrollment Task
Configuration of the enrollment task, which carries out the procedures by which an IPC Process joins a DIF. Currently 
only the default policy is supported by IRATI.

     "enrollmentTaskConfiguration" : {
        "policySet" : {
           "name" : "default",
           "version" : "1",
           "parameters" : [{
               "name"  : "enrollTimeoutInMs",
               "value" : "10000"
             },{
               "name"  : "watchdogPeriodInMs",
               "value" : "30000"
             },{
               "name"  : "declaredDeadIntervalInMs",
               "value" : "120000"
             },{
               "name"  : "useReliableNFlow",
               "value" : "false"
             },{
               "name"  : "maxEnrollmentRetries",
               "value" : "3"
             },{
               "name"  : "n1flows",
               "value" : "2:10/200:0/10000"
             }]
        }
     }

   * **enrollTimeoutInMs**: timeout to wait for response messages during enrollment procedures
   * **watchdogPeriodInMs**: period of the watchdog mechanism, to detect if the application connection with a peer IPC Process is still alive
   * **declaredDeadIntervalInMs**: if no watchdog message has been sent or received from a neighbor IPCP during this period, the 
application connection is closed and the N-1 flow deallocated
   * **useReliableNFlow**: true if a realible N-flow is to be used to communicate with the neighbor IPCP (layer management)
   * **maxEnrollmentRetries**: how many times enrollment should be retried in case of failure
   * **n1flows** (optional): how many flows will be allocated between peer IPCPs, and what are the delay/loss characteristics of each one. The first flow will be used for layer management and data transfer, the others just for data transfer. In the example configuration, two N-1 flows will be requested: one with a maximum delay of 10 ms and a maximum loss probability of 200/10000 SDUs, while the other without loss and delay guarantees.

##### 3.2.2.6 Flow Allocator
Flow allocator maps allocate flow requests to a specific QoS cube, and chooses any policies/parameters 
that the QoS cube definition has not already fixed. The default policy just distinguishes between 
reliable (with rtx control and in order-delivery) and unreliable flow requests.

     "flowAllocatorConfiguration" : {
         "policySet" : {
           "name" : "default",
           "version" : "1"
          }
     }

##### 3.2.2.7 Namespace Manager
The namespace manager maintains the distributted mapping of application process name to IPC Process address 
within a DIF, and is also responsible for IPCP address assignment. 

     "namespaceManagerConfiguration" : {
         "policySet" : {
           "name" : "default",
           "version" : "1"
           }
     }

###### 3.2.2.7.1 Default policy
The default policy keeps a fully replicated database of application names to IPCP addresses. When an application 
registers to the IPCP a new mapping is added to the RIB and disseminated to peer IPCPs via a controlled flooding 
approach. The address assignment policy is static and uses the information from the "known IPC Processes" and 
"address prefixes" configuration items.

   * **Policy name**: default.
   * **Policy version**: 1.
   * **Dependencies**: none.

Example configuration:

    "namespaceManagerConfiguration" : {
        "policySet" : {
            "name" : "default",
            "version" : "1"
         }
     }

###### 3.2.2.7.2 Address change policy
This policy extends the default policy with an address assignment policy that periodically changes the address 
of the IPC Process. It assigns and initial address based on the information form the "known IPC Processes" and 
"address prefixes" configuration items, but it multiplies it for the "range" parameter. Then, it periodically 
changes the address (randomly within a time interval), walking all the values within the "range" and starting from 
the initial value once it reaches the end of the "range". This policy must be used in conjunction with the 
**link-state** routing policy.

   * **Policy name**: address-change.
   * **Policy version**: 1.
   * **Dependencies**:
      * **routing policy**: link-state.

Example configuration:

     "namespaceManagerConfiguration" : {
        "policySet" : {
          "name" : "address-change",
          "version" : "1",
          "parameters" : [{
               "name"  : "useNewTimeout",
               "value" : "10001"
             },{
               "name"  : "deprecateOldTimeout",
               "value" : "20001"
             },{
               "name"  : "changePeriod",
               "value" : "30001"
             },{
               "name"  : "addressRange",
               "value" : "100"
          }]
       } 
     }

   * **useNewTimeout**: Time ellapsed (in ms) after the address change event upon which the new address can be used as source 
address in the header of EFCP PDUs.
   * **deprecateOldTimeout**: Time ellapsed (in ms) after the address change event upon which the old address can be discarded.
Must have the same value as the **waitUntilDeprecateAddress** parameter of the **link-state** routing policy configuration.
   * **changePeriod**: Time interval between address change events (in ms).
   * **addressRange**: Range of addresses available to a single IPC Process.

##### 3.2.2.8 Resource Allocator
Generates the PDU forwarding table based on input from the routing policy (next-hop table). Right now only the default 
policy is supported in IRATI.

     "resourceAllocatorConfiguration" : {
         "pduftgConfiguration" : {    
           "policySet" : {
             "name" : "default",
             "version" : "0",             
             "parameters" : [{
                "name"  : "1.qosid",
                "value" : "10/200"
             },{
                "name"  : "2.qosid",
                "value" : "10/200"
             },{
                "name"  : "3.qosid",
                "value" : "0/10000"
             },{
                "name"  : "4.qosid",
                "value" : "0/10000"
             }]
           }
         }
     } 

The optional parameters are used to map the qos id of the flows at this DIF to characteristics of the N-1 DIFs 
through which the flow should be forwarded (only delay/loss right now). For instance the configuration above will 
add entries to the PDU forwarding table so that entries belonging to QoS ids 1 and 2 are forwarded via an N-1 flow 
that has a maximum delay of 10 ms and a maximum loss probability of 200/10000 SDUs. Entries belonging to QoS ids 
3 and 4 will be forwarded via an N-1 flow that does not provide any guarantees on loss and delay.

##### 3.2.2.9 Routing
The routing policy is in charge of generating and maintaining the next-hop table, and passing this information to the Resource 
Allocator's PDU Forwarding Table generator policy.

###### 3.2.2.9.1 Link-state routing policy
Implements a link-state routing policy, equivalent to the behaviour of routing protocols such as IS-IS (with a single area).
 
   * **Policy name**: link-state.
   * **Policy version**: 1.
   * **Dependencies**: none.
        
Example configuration:
          
     "routingConfiguration" : {
        "policySet" : {
          "name" : "link-state",
          "version" : "1",
          "parameters" : [{
             "name"  : "objectMaximumAge",
             "value" : "10000"
           },{
             "name"  : "waitUntilReadCDAP",
             "value" : "5001"
           },{
             "name"  : "waitUntilError",
             "value" : "5001"
           },{
             "name"  : "waitUntilPDUFTComputation",
             "value" : "103"
           },{
             "name"  : "waitUntilFSODBPropagation",
             "value" : "101"
           },{
             "name"  : "waitUntilAgeIncrement",
             "value" : "997" 
           }, {
             "name" : "waitUntilDeprecateAddress",
             "value" : "20001"
           },{
             "name"  : "routingAlgorithm",
             "value" : "ECMPDijkstra"
           }]       
	} 
     }

   * **objectMaximumAge**: maximum age that an object can reach before being deprecated
   * **waitUntilPDUFTComputation**: Period (in ms) to check if the PDU Forwarding Table needs to be recomputed
   * **waitUntilFSODBPropagation**: Period (in ms) to check if the link-state objects in the RIB need to be propagated
   * **waitUntilAgeIncrement**: Period (in ms) to check if the age of an object in the RIB needs to be increased
   * **waitUntilDeprecateAddress**: Time to wait (in ms) before deprecating an address in a link-state routing object after 
an address change event (only useful if there is renumbering going on in the DIF. Must be the same as the **deprecateOldTimeout** 
parameter in the address-change namespacemanager policy)
   * **routingAlgorithm**: The routing algorithm to generate the next-hop table. Available algorithms:
      * **Dijkstra**: Computes the least-cost next hop to all destination addresses in the DIF (single next-hop per destination address)
      * **ECMPDijkstra**: Computes all the equal-cost next hops to all destination addresses in the DIF (multiple next-hops per destination address)

###### 3.2.2.9.2 Static routing policy
Implements a static routing policy, in which all entries of the next-hop table are provided at IPC Process configuration time.

   * **Policy name**: static.
   * **Policy version**: 1.
   * **Dependencies**: none.

Format of configuration:

     "routingConfiguration" : {
        "policySet" : {
          "name" : "static",
          "version" : "1",
          "parameters" : [{
             "name"  : "<dest-address (single/range/all)>",
             "value" : "<qos_id>-<cost>-<next_hop_addresss>"
           },{
             "name"  : "<dest-address (single/range/all)>",
             "value" : "<qos_id>-<cost>-<next_hop_addresss>"
           },{
               ....
           }]        
        }  
     } 

Each parameter is an entry (or multiple) of the next-hop table. The name of the parameter can refer to a 
   * Single destination address: "19"
   * A range of destination addresses: "20-31"
   * All destination addresses: "defaultNextHop"

While the value of the parameter is the next hop address for a given cost and qos_id.

Example configurations:

     "routingConfiguration" : {
        "policySet" : {
          "name" : "static",
          "version" : "1",
          "parameters" : [{
             "name"  : "defaultNextHop",
             "value" : "0-1-18"
           }]
        }
     }

     "routingConfiguration" : {
        "policySet" : { 
          "name" : "static",
          "version" : "1",
          "parameters" : [{
             "name"  : "1-16",
             "value" : "0-1-18"
           }, {
             "name"  : "15-30",
             "value" : "0-1-19"
           }, {
             "name"  : "32",
             "value" : "0-1-20"
           }]
        }
     }

##### 3.2.2.10 Security Manager
The security manager configuration contains the security manager policy and zero or more configuration 
items for "authentication and SDU protection profiles" (authSDUProfiles). Each authSDUProfile contains 
a consistent set of authentication and SDU protection that need to be used to communicate with all 
neighbor IPC Processes, or only with neighbor IPC Processes over specific N-1 DIFs.

Example format:

    "securityManager" : {
        "policySet" : {
          "name" : "default",
          "version" : "0"
        },
        "authSDUProtProfiles" : {
           "default" : {
              "authPolicy" : {
              	"name" : "PSOC_authentication-sshrsa",
          	"version" : "1",
          	"parameters" : [ {
          	   "name" : "keyExchangeAlg",
          	   "value" : "EDH"
          	}, {
                   "name" : "keystore",
                   "value" : "/usr/local/irati/etc/private_key.pem"
                }, {
                   "name" : "keystorePass",
                   "value" : "test"
                } ]

              },
              "encryptPolicy" : {
                 "name" : "default",
                 "version" : "1",
                 "parameters" : [ {
          	   "name" : "encryptAlg",
          	   "value" : "AES128"
          	}, {
          	   "name" : "macAlg",
          	   "value" : "SHA1"
          	}, {
          	   "name" : "compressAlg",
          	   "value" : "default"
                } ]
              },
              "TTLPolicy" : {
                 "name" : "default",
                 "version" : "1",
                 "parameters" : [ {
                    "name" : "initialValue",
                    "value" : "50"
                  } ]
                },
                "ErrorCheckPolicy" : {
                   "name" : "CRC32",
                   "version" : "1"
                }
           },
           "specific" : [ {
               "underlyingDIF" : "100",
               "authPolicy" : {
               	  "name" : "PSOC_authentication-none",
           	  "version" : "1"
                }
           }, {
               "underlyingDIF" : "110",
               "authPolicy" : {
               	  "name" : "PSOC_authentication-password",
           	  "version" : "1",
           	  "parameters" : [ {
           	     "name" : "password",
           	     "value" : "kf05j.a1234.af0k"
           	  } ]
                },
                "TTLPolicy" : {
                   "name" : "default",
                   "version" : "1",
                   "parameters" : [ {
                      "name" : "initialValue",
                      "value" : "50"
                   } ]
                },
                "ErrorCheckPolicy" : {
                   "name" : "CRC32",
                   "version" : "1"
                }
           } ]
        }
    }

###### 3.2.2.10.1 Authentication policy: NULL authentication policy
The policy performs no authentication, just checks policy names and versions and returns success.

   * **Policy name**: PSOC_authentication-none.
   * **Policy version**: 1.
   * **Dependencies**: none.

Example configuration:

    "authPolicy" : {
        "name" : "PSOC_authentication-none",
        "version" : "1"
    }

###### 3.2.2.10.2 Authentication policy: password-based
Authentication is carried out by hashing a random string with a shared secret (the password).

   * **Policy name**: PSOC_authentication-password.
   * **Policy version**: 1.
   * **Dependencies**: none.
            
Example configuration:
               
    "authPolicy" : {
        "name" : "PSOC_authentication-password",
        "version" : "1",
        "parameters" : [ {
             "name" : "password",
              "value" : "kf05j.a1234.af0k"
         } ]
    }

   * **password**: the shared secret

###### 3.2.2.10.3 Authentication policy: SSH2-based
Authentication policy that mimics the behaviour of the SSH2 protocol authentication mechanism.

   * **Policy name**: PSOC_authentication-ssh2.
   * **Policy version**: 1.
   * **Dependencies**:
      * **SDU Protection, crypto policy**: default
            
Example configuration:
               
    "authPolicy" : {
        "name" : "PSOC_authentication-ssh2",
        "version" : "1",
        "parameters" : [ {
            "name" : "keyExchangeAlg",
            "value" : "EDH"
        }, {
            "name" : "keystore",
            "value" : "/usr/local/irati/etc/creds"
        }, {
            "name" : "keystorePass",
            "value" : "test"
        } ]
    }

   * **keyExchangeAlg**: The key exchange algorithm. Only **EDH** is available right now (Elliptic-Curve Diffie-Hellman.
   * **keyStore**: A folder containing the credentials. The contents of the folder should be the following files:
      * **key**: The RSA private key of the IPCP in PEM format (generated for example with openssl genrsa -out key 2048)
      * **public_key**: The RSA public key of the IPCP in PEM format (extracted from the private key file, for example with openssl rsa -in key -pubout > mykey.pub)
      * For each IPCP whose public key is known, a file containing his public key in PEM format. The file must be named with the IPC Process application process name. For example, imagine the IPCP has 3 neighbours, then there would be 3 files: B.IRATI, C.IRATI and D.IRATI.
   * **keystorePass**: Password to decrypt information in the keystore (if needed)

###### 3.2.2.10.4 Authentication policy: TLS-Handhsake based
Authentication policy that mimics the behaviour of the TLS handshake protocol.

   * **Policy name**: PSOC_authentication-tlshandshake
   * **Policy version**: 1.
   * **Dependencies**:
      * **SDU Protection, crypto policy**: default
            
Example configuration:
               
    "authPolicy" : {
        "name" : "PSOC_authentication-tlshandshake",
        "version" : "1",
        "parameters" : [ {
            "name" : "keystore",
            "value" : "/usr/local/irati/etc/creds"
        }, {
            "name" : "keystorePass",
            "value" : "test"
        } ] 
    }   

   * **keyStore**: A folder containing the credentials. The contents of the folder should be the following files:
      * **key**: contains the RSA private key of the IPCP, in PEM format.
      * **cert.pem**: contains the certificate of the IPCP, in PEM format.
      * **<cert_name>.pem**: one PEM file for every certificate required to reach the (or one of the) root of trust of the DIF (root CA).
   * **keystorePass**: Password to decrypt information in the keystore (if needed)

###### 3.2.2.10.5 SDU Protection, crypto policy: default
The default SDU protection policies carries out cryptographic manipulations of the PDU before being trasmitted through 
an N-1 flow, in order to provide confidentiality and message integrity services. The policy performs the following operations:
encryption, padding, generating message auhentication (MAC) codes and compression; as well as their counterpart operations.

   * **Policy name**: default                  
   * **Policy version**: 1.
   * **Dependencies**:
      * **Authentication policy**: PSOC_authentication-tlshandshake or PSOC_authentication-ssh2

Example configuration:

    "encryptPolicy" : {
        "name" : "default",
        "version" : "1",
        "parameters" : [ {
            "name" : "encryptAlg",
            "value" : "AES128"
         }, {
            "name" : "macAlg",
            "value" : "SHA256"
         }, {
            "name" : "compressAlg",
            "value" : "deflate"
         } ]
    }

   * **encryptAlg**: The encryption algorithm to be used. Currently two are supported: AES128 and AES256.
   * **macAlg**: The algorithm to generate a MAC code. Supported algorithms are: MD5, SHA1 and SHA256.
   * **compressAlg**: The algorithm to compress/decompress PDUs. Only the "deflate" algorithm is supported.

###### 3.2.2.10.6 SDU Protection, PDU lifetime enforcement: default
The default PDU lifetime enforcement policy is a hopcount that starts on a configured initial value and 
is decremented at each hop. When it reaches 0, the PDU is dropeed.

   * **Policy name**: default
   * **Policy version**: 1
   * **Dependencies**: none
      
Example configuration:

    "TTLPolicy" : {
        "name" : "default",
        "version" : "1",
        "parameters" : [ {
           "name" : "initialValue",
           "value" : "50"
        } ]
    }
   
   * **initialValue**: Initial value of the hopcount.

###### 3.2.2.10.7 SDU Protection, error protection: CRC32
This policy protects the PDU against random errors on its bytes by appending a CRC32 field to it.

   * **Policy name**: CRC32
   * **Policy version**: 1
   * **Dependencies**: none

Example configuration:

    "ErrorCheckPolicy" : {
        "name" : "CRC32",
        "version" : "1"
    }

#### 3.2.3 Application to DIF mappings
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

### 3.3 Running the IPC Manager Daemon
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

## 4. Tutorials
Several tutorials are available at https://github.com/IRATI/stack/wiki/Tutorials

## 5. Overview of the software components
This section provides an overview of the software architecture and components of IRATI. For a more detailed 
explanation we direct the reader to FP7-IRATI's at http://irati.eu:
 
* D3.1: http://irati.eu/wp-content/uploads/2012/07/IRATI-D3.1-v1.0.pdf 
* D3.2: http://irati.eu/wp-content/uploads/2012/07/IRATI-D3.2-v1.0.pdf 
* D3.3: http://irati.eu/wp-content/uploads/2012/07/IRATI-D3.3-bundle.zip 

The FP7 PRISTINE (http://ict-pristine.eu) project enhanced the base IRATI implementation with a Software Development Kit to allow 
for programmbility of policies for the different components, and the ability to dynamically load the plugins 
containing such policies at runtime. Documentation about the SDK is available at the following URLs:

* D2.3: http://ict-pristine.eu/wp-content/uploads/2013/12/pristine-d23-sdk-v1_0.pdf
* D2.5: http://ict-pristine.eu/wp-content/uploads/2013/12/pristine-d25-draft.pdf

The software architecture of IRATI is shown in Figure 1. 

![Figure 1. Main software components of the RINA implementation by the FP7-IRATI project](https://github.com/IRATI/stack/wiki/images/irati-softarch.png)
_Figure 1. Source: [S. Vrijders et al; "Prototyping the recursive internet architecture: the IRATI project approach ", IEEE Network Vol 28 (2), pp. 20-25, March 2014](http://ieeexplore.ieee.org/xpl/articleDetails.jsp?arnumber=6786609)_ 

The main components of IRATI have been divided into four packages:

1. **Daemons** ([rinad](https://github.com/IRATI/stack/tree/master/rinad)). This package contains two types 
of daemons (OS Processes that run in the background), implemented in C++.
   * **IPC Manager Daemon** ([rinad/src/ipcm](https://github.com/IRATI/stack/tree/master/rinad/src/ipcm)). The 
IPC Manager Daemon is the core of IPC Management in the system, acting both as the manager of IPC Processes and 
a broker between applications and IPC Processes (enforcing access rights, mapping flow allocation or application 
registration requests to the right IPC Processes, etc.)
   * **IPC Process Daemon**  ([rinad/src/ipcp](https://github.com/IRATI/stack/tree/master/rinad/src/ipcp)). The 
IPC Process Daemons (one per running IPC Process in the system) implement the layer management components of an 
IPC Process (enrollment, flow allocation, PDU Forwarding table generation or distributed resource allocation functions).

2. **Librina** ([librina](https://github.com/IRATI/stack/tree/master/librina)). The librina package contains all 
IRATI libraries that have been introduced to abstract from the user all the kernel interactions (such as syscalls 
and Netlink details). Librina provides its functionalities to user-space RINA programs via scripting language 
extensions or statically/dynamically linkable libraries (i.e. for C/C++ programs).

3. **Kernel components** ([linux/net/rina](https://github.com/IRATI/stack/tree/master/linux/net/rina)). The kernel 
contains the implementation of the data transfer / data transfer control components of normal IPC Processes as well 
as the implementation of shim DIFs - which usually need to access functionality only available at the kernel. The 
Kernel IPC Manager (KIPCM) manages the lifetime (creation, destruction, monitoring) of the other component instances 
in the kernel, as well as its configuration. It also provides coordination at the boundary between the different IPC processes.

4. **Test applications and tools** ([rina-tools](https://github.com/IRATI/stack/tree/master/rina-tools)). This package 
contains test applications and tools to test and debug the RINA Prototype.


