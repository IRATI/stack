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

class BaseEnrollmentStateMachine;

class EnrollmentFailedTimerTask: public rina::TimerTask {
public:
	EnrollmentFailedTimerTask(BaseEnrollmentStateMachine * state_machine,
			const std::string& reason, bool enrollee);
	~EnrollmentFailedTimerTask() throw() {};
	void run();

	BaseEnrollmentStateMachine * state_machine_;
	std::string reason_;
	bool enrollee_;
};

/// The base class that contains the common aspects of both
/// enrollment state machines: the enroller side and the enrolle
/// side
class BaseEnrollmentStateMachine : public BaseCDAPResponseMessageHandler {
	friend class EnrollmentFailedTimerTask;
public:
	enum State {
		STATE_NULL,
		STATE_WAIT_CONNECT_RESPONSE,
		STATE_WAIT_START_ENROLLMENT_RESPONSE,
		STATE_WAIT_READ_RESPONSE,
		STATE_WAIT_START,
		STATE_ENROLLED,
		STATE_WAIT_START_ENROLLMENT,
		STATE_WAIT_STOP_ENROLLMENT_RESPONSE
	};

	/// Called by the EnrollmentTask when it got an M_RELEASE message
	/// @param cdapMessage
	/// @param cdapSessionDescriptor
	void release(rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor);

	/// Called by the EnrollmentTask when it got an M_RELEASE_R message
	/// @param cdapMessage
	/// @param cdapSessionDescriptor
	void releaseResponse(rina::CDAPMessage * cdapMessage,
				rina::CDAPSessionDescriptor * cdapSessionDescriptor);

	/// Called by the EnrollmentTask when the flow supporting the CDAP session with the remote peer
	/// has been deallocated
	/// @param cdapSessionDescriptor
	void flowDeallocated(rina::CDAPSessionDescriptor * cdapSessionDescriptor);

protected:
	BaseEnrollmentStateMachine(IRIBDaemon * rib_daemon, rina::CDAPSessionManagerInterface * cdap_session_manager,
			Encoder * encoder, const rina::ApplicationProcessNamingInformation& remote_naming_info,
			IEnrollmentTask * enrollment_task, int timeout,
			const rina::ApplicationProcessNamingInformation& supporting_dif_name);
	~BaseEnrollmentStateMachine();
	bool isValidPortId(rina::CDAPSessionDescriptor * cdapSessionDescriptor);

	/// Called by the enrollment state machine when the enrollment sequence fails
	void abortEnrollment(const rina::ApplicationProcessNamingInformation& remotePeerNamingInfo,
			int portId, const std::string& reason, bool enrollee, bool sendReleaseMessage);

	/// Create or update the neighbor information in the RIB
	/// @param enrolled true if the neighbor is enrolled, false otherwise
	void createOrUpdateNeighborInformation(bool enrolled);

	/// Sends all the DIF dynamic information
	/// @param IPC Process
	void sendDIFDynamicInformation(IPCProcess * ipcProcess);

	/// Gets the object value from the RIB and send it as a CDAP Mesage
	/// @param objectClass the class of the object to be send
	/// @param objectName the name of the object to be send
	void sendCreateInformation(const std::string& objectClass, const std::string& objectName);

	State state_;
	IRIBDaemon * rib_daemon_;
	rina::CDAPSessionManagerInterface * cdap_session_manager_;
	Encoder * encoder_;
	IEnrollmentTask * enrollment_task_;
	int timeout_;
	rina::Timer * timer_;
	rina::Lockable * lock_;
	int port_id_;
	rina::Neighbor * remote_peer_;
};

/// Handles the operations related to the "daf.management.operationalStatus" object
/*class OperationalStatusRIBObject: public BaseRIBObject {
public:

};*/

}

#endif

#endif
