irati-user-space
================

It will be holding all the user-space level code, apps, tools etc. No sync this time since this repository will hold IRATI only code for the moment. The master branch will hold all the work (from all-partners)

The RINA components in user-space are mainly represented by the following elements:
• The IPC Manager (implemented as an OS daemon): Handles the configuration, creation and deletion of an IPC Processes (in both user and kernel spaces), acts as the management agent for a remote DIF Management system and provides Inter DIF Directory functionalities (such as providing an application-name to DIF resolution service, including the capability to collaborate with other instances of the IDD running in other systems to create new DIFs).
•	The IPC Process (implemented as an OS daemon): There will be one IPC Process daemon for each IPC Process in the system. This daemon contains the user-space components of an IPC Process (the management layer components): the RIB and RIB daemon, CDAP parser/generator, the flow allocator, the Enrollment task, the resource allocator tasks in user space and the PDU forwarding table generator.
•	The Application Process (implemented as an OS process): Applications that make use of the IPC services provided by RINA. These include user applications such as web browsers, servers, IM messaging clients/servers, etc.
•	RINA Libraries (implemented as a set of static or dynamic linkable libraries): RINA related functionalities should be abstracted to the user-space components by libraries; such libraries will be implementing RINA functionalities, shared by the components in the system.
