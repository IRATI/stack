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

#include "common/concurrency.h"
#include "ipcp/components.h"
#include <librina/cdap.h>
#include <librina/internal-events.h>

namespace rinad {

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

class WatchdogRIBObject: public BaseIPCPRIBObject, public rina::BaseCDAPResponseMessageHandler {
public:
	WatchdogRIBObject(IPCProcess * ipc_process, const rina::DIFConfiguration& dif_configuration);
	~WatchdogRIBObject();
	const void* get_value() const;
	void remoteReadObject(int invoke_id, rina::CDAPSessionDescriptor * session_descriptor);

	/// Send watchdog messages to the IPC processes that are our neighbors and we're enrolled to
	void sendMessages();

	/// Take advantadge of the watchdog message responses to measure the RTT,
	/// and store it in the neighbor object (average of the last 4 RTTs)
	void readResponse(int result, const std::string& result_reason,
			void * object_value, const std::string& object_name,
			rina::CDAPSessionDescriptor * session_descriptor);

private:
	rina::CDAPSessionManagerInterface * cdap_session_manager_;
	rina::Timer * timer_;
	int wathchdog_period_;
	int declared_dead_interval_;
	std::map<std::string, int> neighbor_statistics_;
	rina::Lockable * lock_;
};

/// Handles the operations related to the "daf.management.naming.currentsynonym" objects
class AddressRIBObject: public BaseIPCPRIBObject {
public:
	AddressRIBObject(IPCProcess * ipc_process);
	~AddressRIBObject();
	const void* get_value() const;
	void writeObject(const void* object_value);
	std::string get_displayable_value();

private:
	unsigned int address_;
};

class EnrollmentTask: public IPCPEnrollmentTask {
public:
	EnrollmentTask();
	~EnrollmentTask();
	void set_application_process(rina::ApplicationProcess * ap);
	void set_dif_configuration(const rina::DIFConfiguration& dif_configuration);
	void processEnrollmentRequestEvent(rina::EnrollToDAFRequestEvent * event);
	void initiateEnrollment(rina::EnrollmentRequest * request);
	void connect(const rina::CDAPMessage& message,
		     rina::CDAPSessionDescriptor * session_descriptor);
	void connectResponse(int result, const std::string& result_reason,
			rina::CDAPSessionDescriptor * session_descriptor);
	void process_authentication_message(const rina::CDAPMessage& message,
			rina::CDAPSessionDescriptor * session_descriptor);
	void enrollmentFailed(const rina::ApplicationProcessNamingInformation& remotePeerNamingInfo,
			int portId, const std::string& reason, bool sendReleaseMessage);

private:
	void populateRIB();

	/// Called when a new N-1 flow has been allocated
	// @param portId
	void nMinusOneFlowAllocated(rina::NMinusOneFlowAllocatedEvent * flowEvent);

	/// Called when a new N-1 flow allocation has failed
	/// @param portId
	/// @param flowService
	/// @param resultReason
	void nMinusOneFlowAllocationFailed(rina::NMinusOneFlowAllocationFailedEvent * event);

	INamespaceManager * namespace_manager_;
	rina::Thread * neighbors_enroller_;
};

/// Handles the operations related to the "daf.management.operationalStatus" object
class OperationalStatusRIBObject: public BaseIPCPRIBObject {
public:
	OperationalStatusRIBObject(IPCProcess * ipc_process);
	const void* get_value() const;
	void remoteStartObject(void * object_value, int invoke_id,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor);
	void startObject(const void* object);
	void stopObject(const void* object);
	std::string get_displayable_value();

private:
	void sendErrorMessage(const rina::CDAPSessionDescriptor * cdapSessionDescriptor);

	EnrollmentTask * enrollment_task_;
	rina::CDAPSessionManagerInterface * cdap_session_manager_;
};

/// Encoder of a list of EnrollmentInformationRequest
class EnrollmentInformationRequestEncoder: public rina::EncoderInterface {
public:
	const rina::SerializedObject* encode(const void* object);
	void* decode(const rina::ObjectValueInterface * object_value) const;
};

}

#endif

#endif
