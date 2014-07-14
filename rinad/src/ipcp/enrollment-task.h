/*
 * Enrollment Task
 *
 *    Bernat Gaston <bernat.gaston@i2cat.net>
 *    Eduard Grasa <eduard.grasa@i2cat.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef IPCP_ENROLLMENT_TASK_HH
#define IPCP_ENROLLMENT_TASK_HH

#ifdef __cplusplus

#include <map>

#include "ipcp/components.h"
#include <librina/cdap.h>
#include <librina/timer.h>

namespace rinad {

/// The object that contains all the information
/// that is required to initiate an enrollment
/// request (send as the objectvalue of a CDAP M_START
/// message, as specified by the Enrollment spec)
class EnrollmentInformationRequest {
public:
	EnrollmentInformationRequest();
	unsigned int get_address() const;
	void set_address(unsigned int address);
	const std::list<rina::ApplicationProcessNamingInformation>& get_supporting_difs() const;
	void set_supporting_difs(const std::list<rina::ApplicationProcessNamingInformation> &supporting_difs);

private:
	/// The address of the IPC Process that requests
	///to join a DIF
	unsigned int address_;
	std::list<rina::ApplicationProcessNamingInformation> supporting_difs_;
};

class WatchdogRIBObject;

/// Sends a CDAP message to check that the CDAP connection
/// is still working ok
class WatchdogTimerTask: public rina::TimerTask {
public:
	WatchdogTimerTask(WatchdogRIBObject * watchdog, rina::Timer * timer, int delay);
	void run();

private:
	WatchdogRIBObject * watchdog_;
	rina::Timer * timer_;
	int delay_;
};

class WatchdogRIBObject: public BaseRIBObject, public BaseCDAPResponseMessageHandler {
public:
	WatchdogRIBObject(IPCProcess * ipc_process, const rina::DIFConfiguration& dif_configuration);
	~WatchdogRIBObject();
	const void* get_value() const;
	void remoteReadObject(const rina::CDAPMessage * cdapMessage,
				rina::CDAPSessionDescriptor * cdapSessionDescriptor);

	/// Send watchdog messages to the IPC processes that are our neighbors and we're enrolled to
	void sendMessages();

	/// Take advantadge of the watchdog message responses to measure the RTT,
	/// and store it in the neighbor object (average of the last 4 RTTs)
	void readResponse(const rina::CDAPMessage * cdapMessage,
				rina::CDAPSessionDescriptor * cdapSessionDescriptor);

private:
	rina::CDAPSessionManagerInterface * cdap_session_manager_;
	rina::Timer * timer_;
	int wathchdog_period_;
	int declared_dead_interval_;
	std::map<std::string, int> neighbor_statistics_;
	rina::Lockable * lock_;
};

class NeighborSetRIBObject: public BaseRIBObject {
public:
	NeighborSetRIBObject(IPCProcess * ipc_process);
	~NeighborSetRIBObject();
	const void* get_value() const;
	void remoteCreateObject(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor);
	void createObject(const std::string& objectClass,
			const std::string& objectName,
			const void* objectValue);

private:
	void populateNeighborsToCreateList(rina::Neighbor * neighbor,
			std::list<rina::Neighbor *> * list);
	void createNeighbor(rina::Neighbor * neighbor);

	rina::Lockable * lock_;
};

/// Handles the operations related to the "daf.management.naming.currentsynonym" objects
class AddressRIBObject: public BaseRIBObject {
public:
	AddressRIBObject(IPCProcess * ipc_process);
	~AddressRIBObject();
	const void* get_value() const;
	void writeObject(const void* object_value);

private:
	unsigned int address_;
};

}

#endif

#endif
