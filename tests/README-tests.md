Testing procedures
==================

This document describes the tests that have to succeed before integrating code
from one branch in the other.

On the following, we refer with the term 'testbeds' assuming that they are the
IRATI's vWall and EXPERIMENTA testbeds. Target platforms are Debian wheezy and
the 32/64 bits x86 based machines available on the testbeds.

'be' is merged into 'wip' when:
  * 'be' succeeds in the following tests without crashing on the project's
     testbeds:

     General tests: 
       - The kernel reaches 'init' and the modules can be loaded
       - The modules can be unloaded
       - The modules can be loaded and unloaded multiple times
       - The IPC Manager Daemon initializes
       - Kill the IPC Manager Daemon
       - No major performance decreases are noticed

     For each available IPC process:
       - Creation of single IPC Process (using a configuration file)
       - Creation of multiple IPC Processes (using a configuration file)
       - Creation of multiple IPC Processes (using CLI commands)
       - Destroy single IPC Process
       - Destroy multiple IPC Processes
       - Assign the IPC Process to a DIF (using a configuration file)
       - Assign the IPC Process to a DIF (using CLI commands)
       - If it is a normal IPC process, enrol with another IPC process 
         (through both a shim and normal IPC process) using CLI commands
       - If it is a normal IPC process, forwarding is working
       - Destroy the IPC Process assigned to a DIF
       - Destroy the IPC Process enrolled to a DIF

      For each available application:
       - Register the application to a DIF
       - Register more than one instance of the application to an
         IPC Process
       - Run the application once on top of an IPC process
       - Run the application multiple times on top of an IPC process
       - Kill the application

'wip' is merged into 'irati' when: 
  * 'wip' succeeds in the following tests without crashing on the project's
    testbeds:

     General tests: 
        - The stack can be loaded
        - The stack can be unloaded
        - The stack can be loaded and unloaded multiple times
        - The IPC Manager Daemon initializes
        - Kill the IPC Manager Daemon
        - No major performance decreases are noticed

     For each available IPC process:
        - Creation of single IPC Process (using a configuration file)
        - Creation of multiple IPC Processes (using a configuration file)
        - Creation of multiple IPC Processes (using CLI commands)
        - Destroy single IPC Process
        - Destroy multiple IPC Processes
        - Assign the IPC Process to a DIF (using a configuration file)
        - Assign the IPC Process to a DIF (using CLI commands)
        - If it is a normal IPC process, enrol with another IPC process 
          (through both a shim and normal IPC process) using CLI commands
        - If it is a normal IPC process, forwarding is working
        - Destroy the IPC Process assigned to a DIF
        - Destroy the IPC Process enrolled to a DIF

      For each available application:
        - Register the application to a DIF
        - Register more than one instance of the application to an
          IPC Process
        - Run the application once on top of an IPC process
        - Run the application multiple times on top of an IPC process
        - Kill the application
