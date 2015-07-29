//
// Default policy set for Enrollment Task
//
//    Eduard Grasa <eduard.grasa@i2cat.net>
//    Bernat Gaston <bernat.gaston@i2cat.net>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#define IPCP_MODULE "enrollment-task-ps-default"
#include "../../ipcp-logging.h"
#include <string>
#include <climits>
#include <assert.h>

#include "ipcp/ipc-process.h"
#include "ipcp/enrollment-task.h"

namespace rinad {

/// The base class that contains the common aspects of both
/// enrollment state machines: the enroller side and the enrolle
/// side
class BaseEnrollmentStateMachine : public IEnrollmentStateMachine {
public:
	static const std::string CONNECT_RESPONSE_TIMEOUT;
	static const std::string START_RESPONSE_TIMEOUT;
	static const std::string START_IN_BAD_STATE;
	static const std::string STOP_ENROLLMENT_TIMEOUT;
	static const std::string STOP_IN_BAD_STATE;
	static const std::string STOP_WITH_NO_OBJECT_VALUE;
	static const std::string READ_RESPONSE_TIMEOUT;
	static const std::string PROBLEMS_COMMITTING_ENROLLMENT_INFO;
	static const std::string START_TIMEOUT;
	static const std::string READ_RESPONSE_IN_BAD_STATE;
	static const std::string UNSUCCESSFULL_READ_RESPONSE;
	static const std::string UNSUCCESSFULL_START;
	static const std::string CONNECT_IN_NOT_NULL;
	static const std::string ENROLLMENT_NOT_ALLOWED;
	static const std::string START_ENROLLMENT_TIMEOUT;
	static const std::string STOP_ENROLLMENT_RESPONSE_TIMEOUT;
	static const std::string STOP_RESPONSE_IN_BAD_STATE;
	static const std::string AUTHENTICATION_TIMEOUT;

	static const std::string STATE_AUTHENTICATING;
	static const std::string STATE_WAIT_CONNECT_RESPONSE;
	static const std::string STATE_WAIT_START_ENROLLMENT_RESPONSE;
	static const std::string STATE_WAIT_READ_RESPONSE;
	static const std::string STATE_WAIT_START;
	static const std::string STATE_WAIT_START_ENROLLMENT;
	static const std::string STATE_WAIT_STOP_ENROLLMENT_RESPONSE;

	virtual ~BaseEnrollmentStateMachine() { };

protected:
	BaseEnrollmentStateMachine(IPCProcess * ipc_process,
			const rina::ApplicationProcessNamingInformation& remote_naming_info,
			int timeout, rina::ApplicationProcessNamingInformation * supporting_dif_name);

	/// Sends all the DIF dynamic information
	void sendDIFDynamicInformation();

	/// Send the entries in the DFT (if any)
	void sendDFTEntries();

	IPCProcess * ipc_process_;
	rina::CDAPSessionManagerInterface * cdap_session_manager_;
	rina::IMasterEncoder * encoder_;
	IPCPSecurityManager * sec_man_;
};

//Class BaseEnrollmentStateMachine
const std::string BaseEnrollmentStateMachine::CONNECT_RESPONSE_TIMEOUT =
		"Timeout waiting for connect response";
const std::string BaseEnrollmentStateMachine::START_RESPONSE_TIMEOUT =
		"Timeout waiting for start response";
const std::string BaseEnrollmentStateMachine::START_IN_BAD_STATE =
		"Received a START message in a wrong state";
const std::string BaseEnrollmentStateMachine::STOP_ENROLLMENT_TIMEOUT =
		"Timeout waiting for stop enrolment response";
const std::string BaseEnrollmentStateMachine::STOP_IN_BAD_STATE =
		"Received a STOP message in a wrong state";
const std::string BaseEnrollmentStateMachine::STOP_WITH_NO_OBJECT_VALUE =
		"Received STOP message with null object value";
const std::string BaseEnrollmentStateMachine::READ_RESPONSE_TIMEOUT =
		"Timeout waiting for read response";
const std::string BaseEnrollmentStateMachine::PROBLEMS_COMMITTING_ENROLLMENT_INFO =
		"Problems commiting enrollment information";
const std::string BaseEnrollmentStateMachine::START_TIMEOUT =
		"Timeout waiting for start";
const std::string BaseEnrollmentStateMachine::READ_RESPONSE_IN_BAD_STATE =
		"Received a READ_RESPONSE message in a wrong state";
const std::string BaseEnrollmentStateMachine::UNSUCCESSFULL_READ_RESPONSE =
		"Received an unsuccessful read response or a read response with a null object value";
const std::string BaseEnrollmentStateMachine::UNSUCCESSFULL_START =
		"Received unsuccessful start request";
const std::string BaseEnrollmentStateMachine::CONNECT_IN_NOT_NULL =
		"Received a CONNECT message while not in NULL state";
const std::string BaseEnrollmentStateMachine::ENROLLMENT_NOT_ALLOWED =
		"Enrollment rejected by security manager";
const std::string BaseEnrollmentStateMachine::START_ENROLLMENT_TIMEOUT =
		"Timeout waiting for start enrollment request";
const std::string BaseEnrollmentStateMachine::STOP_ENROLLMENT_RESPONSE_TIMEOUT =
		"Timeout waiting for stop enrollment response";
const std::string BaseEnrollmentStateMachine::STOP_RESPONSE_IN_BAD_STATE =
		"Received a STOP Response message in a wrong state";
const std::string BaseEnrollmentStateMachine::AUTHENTICATION_TIMEOUT =
		"Timeout waiting for authentication to complete";

const std::string BaseEnrollmentStateMachine::STATE_AUTHENTICATING =
		"AUTHENTICATING";
const std::string BaseEnrollmentStateMachine::STATE_WAIT_CONNECT_RESPONSE =
		"WAIT_CONNECT_RESPONSE";
const std::string BaseEnrollmentStateMachine::STATE_WAIT_START_ENROLLMENT_RESPONSE =
		"WAIT_START_ENROLLMENT_RESPONSE";
const std::string BaseEnrollmentStateMachine::STATE_WAIT_READ_RESPONSE =
		"WAIT_READ_RESPONSE";
const std::string BaseEnrollmentStateMachine::STATE_WAIT_START =
		"WAIT_START";
const std::string BaseEnrollmentStateMachine::STATE_WAIT_START_ENROLLMENT =
		"WAIT_START_ENROLLMENT";
const std::string BaseEnrollmentStateMachine::STATE_WAIT_STOP_ENROLLMENT_RESPONSE =
		"WAIT_STOP_ENROLLMENT_RESPONSE";

BaseEnrollmentStateMachine::BaseEnrollmentStateMachine(IPCProcess * ipc_process,
			const rina::ApplicationProcessNamingInformation& remote_naming_info,
			int timeout, rina::ApplicationProcessNamingInformation * supporting_dif_name) :
				IEnrollmentStateMachine(ipc_process, remote_naming_info,
						timeout, supporting_dif_name)
{
	ipc_process_ = ipc_process;
	cdap_session_manager_ = ipc_process->cdap_session_manager_;
	encoder_ = ipc_process->encoder_;
	sec_man_ = ipc_process->security_manager_;
}

void BaseEnrollmentStateMachine::sendDIFDynamicInformation() {
	//Send DirectoryForwardingTableEntries
	sendDFTEntries();

	//Send neighbors (including myself)
	sendNeighbors();
}

void BaseEnrollmentStateMachine::sendDFTEntries() {
	rina::BaseRIBObject * dftEntrySet;
	std::list<rina::BaseRIBObject *>::const_iterator it;
	std::list<rina::DirectoryForwardingTableEntry *> dftEntries;

	try {
		dftEntrySet = rib_daemon_->readObject(EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_CLASS,
				EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_NAME);
		for (it = dftEntrySet->get_children().begin();
				it != dftEntrySet->get_children().end(); ++it) {
			dftEntries.push_back((rina::DirectoryForwardingTableEntry*) (*it)->get_value());
		}

		if (dftEntries.size() == 0) {
			LOG_IPCP_DBG("No DFT entries to be sent");
			return;
		}

		rina::RIBObjectValue robject_value;
		robject_value.type_ = rina::RIBObjectValue::complextype;
		robject_value.complex_value_ = &dftEntries;
		rina::RemoteProcessId remote_id;
		remote_id.port_id_ = port_id_;

		rib_daemon_->remoteCreateObject(EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_CLASS,
				EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_NAME, robject_value, 0, remote_id, 0);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems sending DFT entries: %s", e.what());
	}
}

/// The state machine of the party that wants to
/// become a new member of the DIF.
class EnrolleeStateMachine: public BaseEnrollmentStateMachine {
public:
	EnrolleeStateMachine(IPCProcess * ipc_process,
			const rina::ApplicationProcessNamingInformation& remote_naming_info,
			int timeout);
	~EnrolleeStateMachine() { };

	/// Called by the DIFMembersSetObject to initiate the enrollment sequence
	/// with a remote IPC Process
	/// @param enrollment request contains information on the neighbor and on the
	/// enrollment request event
	/// @param portId
	void initiateEnrollment(rina::EnrollmentRequest * enrollmentRequest, int portId);

	void process_authentication_message(const rina::CDAPMessage& message,
				            rina::CDAPSessionDescriptor * session_descriptor);

	void authentication_completed(bool success);

	/// Called by the EnrollmentTask when it got an M_CONNECT_R message
	/// @param result
	/// @param result_reason
	void connectResponse(int result, const std::string& result_reason);

	void startResponse(int result, const std::string& result_reason,
			void * object_value, rina::CDAPSessionDescriptor * session_descriptor);

	/// Stop enrollment request received. Check if I have enough information, if not
	/// ask for more with M_READs.
	/// Have to check if I can start operating (if not wait
	/// until M_START operationStatus). If I can start and have enough information,
	/// create or update all the objects received during the enrollment phase.
	void stop(EnrollmentInformationRequest * eiRequest, int invoke_id,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor);

	/// See if the response is valid and contains an object. See if more objects
	/// are required. If not, start
	void readResponse(int result, const std::string& result_reason,
			void * object_value, const std::string& object_name,
			rina::CDAPSessionDescriptor * session_descriptor);

	void start(int result, const std::string& result_reason,
			rina::CDAPSessionDescriptor * session_descriptor);

	rina::EnrollmentRequest * enrollment_request_;

private:
	/// See if more information is required for enrollment, or if we can
	/// start or if we have to wait for the start message
	void requestMoreInformationOrStart();

	/// Checks if more information is required for enrollment
	/// (At least there must be DataTransferConstants, a QoS cube and a DAF Member). If there is,
	/// it sends a read request message and returns tru
	bool sendNextObjectRequired();

	/// Create the objects in the RIB
	void commitEnrollment();

	void enrollmentCompleted();

	bool was_dif_member_before_enrollment_;
	bool allowed_to_start_early_;
	int stop_request_invoke_id_;
};

// Class EnrolleeStateMachine
EnrolleeStateMachine::EnrolleeStateMachine(IPCProcess * ipc_process,
		const rina::ApplicationProcessNamingInformation& remote_naming_info,
		int timeout): BaseEnrollmentStateMachine(ipc_process, remote_naming_info,
				timeout, 0) {
	was_dif_member_before_enrollment_ = false;
	enrollment_request_ = 0;
	last_scheduled_task_ = 0;
	allowed_to_start_early_ = false;
	stop_request_invoke_id_ = 0;
}

void EnrolleeStateMachine::initiateEnrollment(rina::EnrollmentRequest * enrollmentRequest,
					      int portId)
{
	rina::ScopedLock g(lock_);

	enrollment_request_ = enrollmentRequest;
	remote_peer_->address_ = enrollment_request_->neighbor_->address_;
	remote_peer_->name_ = enrollment_request_->neighbor_->name_;
	remote_peer_->supporting_dif_name_ = enrollment_request_->neighbor_->supporting_dif_name_;
	remote_peer_->underlying_port_id_ = portId;
	remote_peer_->supporting_difs_ = enrollment_request_->neighbor_->supporting_difs_;

	if (state_ != STATE_NULL) {
		throw rina::Exception("Enrollee state machine not in NULL state");
	}

	try{
		rina::RemoteProcessId remote_id;
		remote_id.port_id_ = portId;

		rina::AuthSDUProtectionProfile profile =
				sec_man_->get_auth_sdup_profile(remote_peer_->supporting_dif_name_.processName);
		auth_ps_ = ipc_process_->security_manager_->get_auth_policy_set(profile.authPolicy.name_);
		if (!auth_ps_) {
			abortEnrollment(remote_peer_->name_, port_id_,
					std::string("Unsupported authentication policy set"), true);
			return;
		}

		rina::AuthPolicy auth_policy = auth_ps_->get_auth_policy(portId, profile);

		rib_daemon_->openApplicationConnection(auth_policy, "",
				IPCProcess::MANAGEMENT_AE, remote_peer_->name_.processInstance,
				remote_peer_->name_.processName, "", IPCProcess::MANAGEMENT_AE,
				ipc_process_->get_instance(), ipc_process_->get_name(), remote_id);

		port_id_ = portId;

		//Set timer
		last_scheduled_task_ = new EnrollmentFailedTimerTask(this, CONNECT_RESPONSE_TIMEOUT);
		timer_.scheduleTask(last_scheduled_task_, timeout_);

		//Update state
		state_ = STATE_WAIT_CONNECT_RESPONSE;
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems sending M_CONNECT message: %s", e.what());
		abortEnrollment(remote_peer_->name_, port_id_, std::string(e.what()), true);
	}
}

void EnrolleeStateMachine::process_authentication_message(const rina::CDAPMessage& message,
					    	          rina::CDAPSessionDescriptor * session_descriptor)
{
	lock_.lock();

	if (!auth_ps_ || state_ != STATE_WAIT_CONNECT_RESPONSE) {
		lock_.unlock();
		abortEnrollment(remote_peer_->name_,
				port_id_,
				"Authentication policy is null or wrong state",
				true);
		return;
	}

	int result = auth_ps_->process_incoming_message(message,
							session_descriptor->port_id_);

	lock_.unlock();

	if (result == rina::IAuthPolicySet::FAILED) {
		abortEnrollment(remote_peer_->name_,
				port_id_,
				"Authentication failed",
				true);
	} else if (result == rina::IAuthPolicySet::SUCCESSFULL) {
		LOG_IPCP_INFO("Authentication was successful, waiting for M_CONNECT_R");
	}
}

void EnrolleeStateMachine::authentication_completed(bool success)
{
	if (!success) {
		abortEnrollment(remote_peer_->name_,
				port_id_,
				"Authentication failed",
				true);
	} else {
		LOG_IPCP_INFO("Authentication was successful, waiting for M_CONNECT_R");
	}
}

void EnrolleeStateMachine::connectResponse(int result,
		const std::string& result_reason) {
	rina::ScopedLock g(lock_);

	if (state_ != STATE_WAIT_CONNECT_RESPONSE) {
		abortEnrollment(remote_peer_->name_, port_id_,
				"Message received in wrong order", true);
		return;
	}

	timer_.cancelTask(last_scheduled_task_);
	if (result != 0) {
		state_ = STATE_NULL;
		enrollment_task_->enrollmentFailed(remote_peer_->name_, port_id_,
				result_reason, true);
		return;
	}

	//Send M_START with EnrollmentInformation object
	try{
		EnrollmentInformationRequest eiRequest;
		std::list<rina::ApplicationProcessNamingInformation>::const_iterator it;
		std::vector<rina::ApplicationRegistration *> registrations =
				rina::extendedIPCManager->getRegisteredApplications();
		for(unsigned int i=0; i<registrations.size(); i++) {
			for(it = registrations[i]->DIFNames.begin();
					it != registrations[i]->DIFNames.end(); ++it) {;
				eiRequest.supporting_difs_.push_back(*it);
			}
		}

		if (ipc_process_->get_address() != 0) {
			was_dif_member_before_enrollment_ = true;
			eiRequest.address_ = ipc_process_->get_address();
		} else {
			rina::DIFInformation difInformation;
			difInformation.dif_name_ = enrollment_request_->event_.dafName;
			ipc_process_->set_dif_information(difInformation);
		}

		rina::RIBObjectValue object_value;
		object_value.type_ = rina::RIBObjectValue::complextype;
		object_value.complex_value_ = &eiRequest;
		rina::RemoteProcessId remote_id;
		remote_id.port_id_ = port_id_;

		rib_daemon_->remoteStartObject(EncoderConstants::ENROLLMENT_INFO_OBJECT_CLASS,
				EncoderConstants::ENROLLMENT_INFO_OBJECT_NAME, object_value, 0, remote_id, this);

		LOG_IPCP_DBG("Sent a M_START Message to portid: %d", port_id_);

		//Set timer
		last_scheduled_task_ = new EnrollmentFailedTimerTask(this, START_RESPONSE_TIMEOUT);
		timer_.scheduleTask(last_scheduled_task_, timeout_);

		//Update state
		state_ = STATE_WAIT_START_ENROLLMENT_RESPONSE;
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems sending M_START request message: %s", e.what());
		//TODO what to do?
	}
}

void EnrolleeStateMachine::startResponse(int result, const std::string& result_reason,
		void * object_value, rina::CDAPSessionDescriptor * session_descriptor) {
	rina::ScopedLock g(lock_);

	if (!isValidPortId(session_descriptor->port_id_)){
		return;
	}

	if (state_ != STATE_WAIT_START_ENROLLMENT_RESPONSE) {
		abortEnrollment(remote_peer_->name_, port_id_,
				START_IN_BAD_STATE, true);
		return;
	}

	timer_.cancelTask(last_scheduled_task_);
	if (result != 0) {
		state_ = STATE_NULL;
		enrollment_task_->enrollmentFailed(remote_peer_->name_, port_id_,
				result_reason, true);
		return;
	}

	//Update address
	if (object_value) {
		EnrollmentInformationRequest * response =
				(EnrollmentInformationRequest *) object_value;

		unsigned int address = response->address_;
		delete response;

		try {
			rib_daemon_->writeObject(EncoderConstants::ADDRESS_RIB_OBJECT_CLASS,
					EncoderConstants::ADDRESS_RIB_OBJECT_NAME, &address);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems writing RIB object: %s", e.what());
		}
	}

	//Set timer
	last_scheduled_task_ = new EnrollmentFailedTimerTask(this, STOP_ENROLLMENT_TIMEOUT);
	timer_.scheduleTask(last_scheduled_task_, timeout_);

	//Update state
	state_ = STATE_WAIT_STOP_ENROLLMENT_RESPONSE;
}

void EnrolleeStateMachine::stop(EnrollmentInformationRequest * eiRequest, int invoke_id,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	rina::ScopedLock g(lock_);

	if (!isValidPortId(cdapSessionDescriptor->port_id_)){
		return;
	}

	if (state_ != STATE_WAIT_STOP_ENROLLMENT_RESPONSE) {
		abortEnrollment(remote_peer_->name_, port_id_,
				STOP_IN_BAD_STATE, true);
		return;
	}

	timer_.cancelTask(last_scheduled_task_);
	//Check if I'm allowed to start early
	if (!eiRequest->allowed_to_start_early_){
		abortEnrollment(remote_peer_->name_, port_id_,
				STOP_WITH_NO_OBJECT_VALUE, true);
		return;
	}

	allowed_to_start_early_ = eiRequest->allowed_to_start_early_;
	stop_request_invoke_id_ = invoke_id;

	//If the enrollee is also a member of the DIF, send dynamic information to the enroller as well
	if (ipc_process_->get_operational_state() == ASSIGNED_TO_DIF) {
		sendDFTEntries();
	}

	//Request more information or start
	try{
		requestMoreInformationOrStart();
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems requesting more information or starting: %s", e.what());
		abortEnrollment(remote_peer_->name_, port_id_, std::string(e.what()), true);
	}
}

void EnrolleeStateMachine::requestMoreInformationOrStart() {
	if (sendNextObjectRequired()){
		//Set timer
		last_scheduled_task_ = new EnrollmentFailedTimerTask(this,
							 	     READ_RESPONSE_TIMEOUT);
		timer_.scheduleTask(last_scheduled_task_, timeout_);

		//Update state
		state_ = STATE_WAIT_READ_RESPONSE;
		return;
	}

	//No more information is required, if I'm allowed to start early,
	//commit the enrollment information, set operational status to true
	//and send M_STOP_R. If not, just send M_STOP_R
	rina::RIBObjectValue object_value;
	rina::RemoteProcessId remote_id;
	remote_id.port_id_ = port_id_;

	if (allowed_to_start_early_){
		try{
			commitEnrollment();

			rib_daemon_->remoteStopObjectResponse("", "", object_value, 0, "", stop_request_invoke_id_, remote_id);

			enrollmentCompleted();
		}catch(rina::Exception &e){
			LOG_IPCP_ERR("Problems sending CDAP message: %s", e.what());

			rib_daemon_->remoteStopObjectResponse("", "", object_value, -1,
					PROBLEMS_COMMITTING_ENROLLMENT_INFO, stop_request_invoke_id_, remote_id);

			abortEnrollment(remote_peer_->name_, port_id_,
					PROBLEMS_COMMITTING_ENROLLMENT_INFO, true);
		}

		return;
	}

	try {
		rib_daemon_->remoteStopObjectResponse("", "", object_value, 0, "", stop_request_invoke_id_, remote_id);
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems sending CDAP message: %s", e.what());
	}

	last_scheduled_task_ = new EnrollmentFailedTimerTask(this, START_TIMEOUT);
	timer_.scheduleTask(last_scheduled_task_, timeout_);
	state_ = STATE_WAIT_START;
}

bool EnrolleeStateMachine::sendNextObjectRequired() {
	bool result = false;
	rina::DIFInformation difInformation = ipc_process_->get_dif_information();

	rina::RemoteProcessId remote_id;
	remote_id.port_id_ = port_id_;
	std::string object_class;
	std::string object_name;
	if (!difInformation.dif_configuration_.efcp_configuration_.data_transfer_constants_.isInitialized()) {
		object_class = EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_CLASS;
		object_name = EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_CLASS;
		result = true;
	} else if (difInformation.dif_configuration_.efcp_configuration_.qos_cubes_.size() == 0){
		object_class = EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_CLASS;
		object_name = EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_NAME;
		result = true;
	}else if (ipc_process_->get_neighbors().size() == 0){
		object_class = rina::NeighborSetRIBObject::NEIGHBOR_SET_RIB_OBJECT_CLASS;
		object_name = rina::NeighborSetRIBObject::NEIGHBOR_SET_RIB_OBJECT_NAME;
		result = true;
	}

	if (result) {
		try{
			rib_daemon_->remoteReadObject(object_class, object_name, 0, remote_id, this);
		} catch (rina::Exception &e) {
			LOG_IPCP_WARN("Problems executing remote operation: %s", e.what());
		}
	}

	return result;
}

void EnrolleeStateMachine::commitEnrollment() {
	try {
		rib_daemon_->startObject(EncoderConstants::OPERATIONAL_STATUS_RIB_OBJECT_CLASS,
				EncoderConstants::OPERATIONAL_STATUS_RIB_OBJECT_NAME, 0);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems starting RIB object: %s", e.what());
	}
}

void EnrolleeStateMachine::enrollmentCompleted()
{
	state_ = STATE_ENROLLED;

	//Create or update the neighbor information in the RIB
	createOrUpdateNeighborInformation(true);

	//Send DirectoryForwardingTableEntries
	sendCreateInformation(EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_CLASS,
			EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_NAME);

	enrollment_task_->enrollmentCompleted(*remote_peer_, true);

	//Notify the kernel
	if (!was_dif_member_before_enrollment_) {
		try {
			rina::kernelIPCProcess->assignToDIF(ipc_process_->get_dif_information());
		} catch(rina::Exception &e) {
			LOG_IPCP_ERR("Problems communicating with the Kernel components of the IPC Processs: %s",
					e.what());
		}
	}

	//Notify the IPC Manager
	if (enrollment_request_){
		try {
			std::list<rina::Neighbor> neighbors;
			neighbors.push_back(*remote_peer_);
			rina::extendedIPCManager->enrollToDIFResponse(enrollment_request_->event_,
					0, neighbors, ipc_process_->get_dif_information());
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems sending message to IPC Manager: %s", e.what());
		}
	}

	LOG_IPCP_INFO("Remote IPC Process enrolled!");

	delete enrollment_request_;
}

void EnrolleeStateMachine::readResponse(int result, const std::string& result_reason,
		void * object_value, const std::string& object_name,
		rina::CDAPSessionDescriptor * session_descriptor) {
	rina::ScopedLock g(lock_);

	if (!isValidPortId(session_descriptor->port_id_)){
		return;
	}

	if (state_ != STATE_WAIT_READ_RESPONSE) {
		abortEnrollment(remote_peer_->name_, port_id_,
				READ_RESPONSE_IN_BAD_STATE, true);
		return;
	}

	timer_.cancelTask(last_scheduled_task_);

	if (result != 0 || object_value == 0){
		abortEnrollment(remote_peer_->name_, port_id_,
				result_reason, true);
		return;
	}

	if (object_name.compare(EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_NAME) == 0){
		try{
			rina::DataTransferConstants * constants =
					(rina::DataTransferConstants *) object_value;
			rib_daemon_->createObject(EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_CLASS,
					EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_NAME, constants, 0);
		}catch(rina::Exception &e){
			LOG_IPCP_ERR("Problems creating RIB object: %s", e.what());
		}
	}else if (object_name.compare(EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_NAME) == 0){
		try{
			std::list<rina::QoSCube *> * cubes =
					(std::list<rina::QoSCube *> *) object_value;
			rib_daemon_->createObject(EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_CLASS,
					EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_NAME, cubes, 0);
		}catch(rina::Exception &e){
			LOG_IPCP_ERR("Problems creating RIB object: %s", e.what());
		}
	}else if (object_name.compare(rina::NeighborSetRIBObject::NEIGHBOR_SET_RIB_OBJECT_NAME) == 0){
		try{
			std::list<rina::Neighbor *> * neighbors =
					(std::list<rina::Neighbor *> *) object_value;
			rib_daemon_->createObject(rina::NeighborSetRIBObject::NEIGHBOR_SET_RIB_OBJECT_CLASS,
					rina::NeighborSetRIBObject::NEIGHBOR_SET_RIB_OBJECT_NAME,
					neighbors, 0);
		}catch(rina::Exception &e){
			LOG_IPCP_ERR("Problems creating RIB object: %s", e.what());
		}
	}else{
		LOG_IPCP_WARN("The object to be created is not required for enrollment");
	}

	//Request more information or proceed with the enrollment program
	requestMoreInformationOrStart();
}

void EnrolleeStateMachine::start(int result, const std::string& result_reason,
		rina::CDAPSessionDescriptor * session_descriptor) {
	rina::ScopedLock g(lock_);

	if (!isValidPortId(session_descriptor->port_id_)){
		return;
	}

	if (state_ == STATE_ENROLLED) {
		return;
	}

	if (state_ != STATE_WAIT_START) {
		abortEnrollment(remote_peer_->name_, port_id_,
				START_IN_BAD_STATE, true);
		return;
	}

	timer_.cancelTask(last_scheduled_task_);

	if (result != 0){
		abortEnrollment(remote_peer_->name_, port_id_,
				result_reason, true);
		return;
	}

	try{
		commitEnrollment();
		enrollmentCompleted();
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems commiting enrollment: %s", e.what());
		abortEnrollment(remote_peer_->name_, port_id_,
				PROBLEMS_COMMITTING_ENROLLMENT_INFO, true);
	}
}

/// The state machine of the party that is a member of the DIF
/// and will help the joining party (enrollee) to join the DIF
class EnrollerStateMachine: public BaseEnrollmentStateMachine {
public:
	EnrollerStateMachine(IPCProcess * ipc_process,
		const rina::ApplicationProcessNamingInformation& remote_naming_info, int timeout,
		rina::ApplicationProcessNamingInformation * supporting_dif_name);
	~EnrollerStateMachine() { };

	/// An M_CONNECT message has been received.  Handle the transition from the
	/// NULL to the WAIT_START_ENROLLMENT state.
	/// Authenticate the remote peer and issue a connect response
	/// @param invoke_id
	/// @param portId
	void connect(const rina::CDAPMessage& cdapMessage,
		     rina::CDAPSessionDescriptor * session_descriptor);

	void process_authentication_message(const rina::CDAPMessage& message,
				            rina::CDAPSessionDescriptor * session_descriptor);

	void authentication_completed(bool success);

	/// Called by the Enrollment object when it receives an M_START message from
	/// the enrolling member. Have to look at the enrollment information request,
	/// from that deduce if the IPC process requesting to enroll with me is already
	/// a member of the DIF and if its address is valid. If it is not a member of
	/// the DIF, send a new address with the M_START_R, send the M_CREATEs to provide
	/// the DIF initialization information and state, and send M_STOP_R. If it is a
	/// valid member, just send M_START_R with no address and M_STOP_R
	/// @param eiRequest
	/// @param invoke_id to reply to the message
	/// @param cdapSessionDescriptor
	void start(EnrollmentInformationRequest * eiRequest, int invoke_id,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor);

	/// The response of the stop operation has been received, send M_START operation without
	/// waiting for an answer and consider the process enrolled
	/// @param result
	/// @param result_reason
	/// @param object_value
	/// @param cdapSessionDescriptor
	void stopResponse(int result, const std::string& result_reason,
			void * object_value, rina::CDAPSessionDescriptor * session_descriptor);

private:
    /// Send a negative response to the M_START enrollment message
    /// @param result the error code
    /// @param resultReason the reason of the bad result
    /// @param requestMessage the received M_START enrollment message
    void sendNegativeStartResponseAndAbortEnrollment(int result, const std::string&
    		resultReason, int invoke_id);

        void authentication_successful();

    /// Send all the information required to start operation to
    /// the IPC process that is enrolling to me
    void sendDIFStaticInformation();

    void enrollmentCompleted();

	IPCPSecurityManager * security_manager_;
	INamespaceManager * namespace_manager_;
	rina::CDAPSessionDescriptor * session_descriptor_;
	int connect_message_invoke_id_;
};

//Class EnrollerStateMachine
EnrollerStateMachine::EnrollerStateMachine(IPCProcess * ipc_process,
		const rina::ApplicationProcessNamingInformation& remote_naming_info, int timeout,
		rina::ApplicationProcessNamingInformation * supporting_dif_name):
		BaseEnrollmentStateMachine(ipc_process, remote_naming_info,
					timeout, supporting_dif_name){
	security_manager_ = ipc_process->security_manager_;
	namespace_manager_ = ipc_process->namespace_manager_;
	enroller_ = true;
	session_descriptor_ = 0;
	connect_message_invoke_id_ = 0;
}

void EnrollerStateMachine::connect(const rina::CDAPMessage& cdapMessage,
			           rina::CDAPSessionDescriptor * session_descriptor)
{
	lock_.lock();

	if (state_ != STATE_NULL) {
		lock_.unlock();
		abortEnrollment(remote_peer_->name_, session_descriptor->port_id_,
				CONNECT_IN_NOT_NULL, true);
		return;
	}

	LOG_IPCP_DBG("Authenticating IPC process %s-%s ...", session_descriptor->dest_ap_name_.c_str(),
			session_descriptor->dest_ap_inst_.c_str());
	remote_peer_->name_.processName = session_descriptor->dest_ap_name_;
	remote_peer_->name_.processInstance = session_descriptor->dest_ap_inst_;
	session_descriptor_ = session_descriptor;
	connect_message_invoke_id_ = cdapMessage.invoke_id_;
	port_id_ = session_descriptor_->port_id_;

	auth_ps_ = security_manager_->get_auth_policy_set(cdapMessage.auth_policy_.name_);
	if (!auth_ps_) {
		lock_.unlock();
		abortEnrollment(remote_peer_->name_, port_id_,
				std::string("Could not find auth policy set"), true);
		return;
	}

	//TODO pass auth_value and auth_type in the function interface
	rina::AuthSDUProtectionProfile profile =
			sec_man_->get_auth_sdup_profile(remote_peer_->supporting_dif_name_.processName);
	rina::IAuthPolicySet::AuthStatus auth_status =
			auth_ps_->initiate_authentication(cdapMessage.auth_policy_,
							  profile, port_id_);
	if (auth_status == rina::IAuthPolicySet::FAILED) {
		lock_.unlock();
		abortEnrollment(remote_peer_->name_, port_id_,
				std::string("Authentication failed"), true);
		return;
	} else if (auth_status == rina::IAuthPolicySet::IN_PROGRESS) {
		state_ = STATE_AUTHENTICATING;
		last_scheduled_task_ = new EnrollmentFailedTimerTask(this,
								     AUTHENTICATION_TIMEOUT);
		LOG_IPCP_DBG("Authentication in progress");
		timer_.scheduleTask(last_scheduled_task_, timeout_);
		lock_.unlock();
		return;
	}

	lock_.unlock();
	authentication_successful();
}

void EnrollerStateMachine::process_authentication_message(const rina::CDAPMessage& message,
					    	          rina::CDAPSessionDescriptor * session_descriptor)
{
	lock_.lock();

	if (!auth_ps_ || state_ != STATE_AUTHENTICATING) {
		lock_.unlock();
		abortEnrollment(remote_peer_->name_,
				port_id_,
				"Authentication policy is null or wrong state",
				true);
		return;
	}

	int result = auth_ps_->process_incoming_message(message,
							session_descriptor->port_id_);

	if (result == rina::IAuthPolicySet::IN_PROGRESS) {
		LOG_IPCP_DBG("Authentication still in progress");
		lock_.unlock();
		return;
	}

	timer_.cancelTask(last_scheduled_task_);

	lock_.unlock();

	if (result == rina::IAuthPolicySet::FAILED) {
		abortEnrollment(remote_peer_->name_,
				port_id_,
				"Authentication failed",
				true);
		return;
	}

	authentication_successful();
}

void EnrollerStateMachine::authentication_completed(bool success)
{
	if (last_scheduled_task_) {
		timer_.cancelTask(last_scheduled_task_);
		last_scheduled_task_ = 0;
	}

	if (!success) {
		abortEnrollment(remote_peer_->name_,
				port_id_,
				"Authentication failed",
				true);
	} else {
		authentication_successful();
	}
}

void EnrollerStateMachine::authentication_successful()
{
	rina::ScopedLock scopedLock(lock_);

	ISecurityManagerPs *smps = dynamic_cast<ISecurityManagerPs *>(security_manager_->ps);
	assert(smps);

	LOG_IPCP_DBG("Authentication successful, deciding if new member can join the DIF...");
	if (!smps->isAllowedToJoinDIF(*remote_peer_)) {
		LOG_IPCP_WARN("Security Manager rejected enrollment attempt, aborting enrollment");
		abortEnrollment(remote_peer_->name_, port_id_,
				ENROLLMENT_NOT_ALLOWED, true);
		return;
	}

	//Send M_CONNECT_R
	try{
		rina::RemoteProcessId remote_id;
		remote_id.port_id_ = port_id_;

		rib_daemon_->openApplicationConnectionResponse(
				rina::AuthPolicy(), session_descriptor_->dest_ae_inst_,
				IPCProcess::MANAGEMENT_AE, session_descriptor_->dest_ap_inst_,
				session_descriptor_->dest_ap_name_, 0, "", session_descriptor_->src_ae_inst_,
				IPCProcess::MANAGEMENT_AE, session_descriptor_->src_ap_inst_,
				session_descriptor_->src_ap_name_, connect_message_invoke_id_, remote_id);

		//Set timer
		last_scheduled_task_ = new EnrollmentFailedTimerTask(this, START_ENROLLMENT_TIMEOUT);
		timer_.scheduleTask(last_scheduled_task_, timeout_);
		LOG_IPCP_DBG("M_CONNECT_R sent to portID %d. Waiting for start enrollment request message", port_id_);

		state_ = STATE_WAIT_START_ENROLLMENT;
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems sending CDAP message: %s", e.what());
		abortEnrollment(remote_peer_->name_, port_id_,
				std::string(e.what()), true);
	}
}

void EnrollerStateMachine::sendNegativeStartResponseAndAbortEnrollment(int result, const std::string&
		resultReason, int invoke_id) {
	try{
		rina::RIBObjectValue robject_value;
		rina::RemoteProcessId remote_id;
		remote_id.port_id_ = port_id_;

		rib_daemon_->remoteStartObjectResponse("", "", robject_value, result,
				resultReason, invoke_id, remote_id);

		abortEnrollment(remote_peer_->name_, port_id_, resultReason, true);
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems sending CDAP message: %s", e.what());
	}
}

void EnrollerStateMachine::sendDIFStaticInformation() {
	sendCreateInformation(EncoderConstants::WHATEVERCAST_NAME_SET_RIB_OBJECT_CLASS,
			EncoderConstants::WHATEVERCAST_NAME_SET_RIB_OBJECT_NAME);

	sendCreateInformation(EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_CLASS,
			EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_NAME);

	sendCreateInformation(EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_CLASS,
			EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_NAME);
}

void EnrollerStateMachine::start(EnrollmentInformationRequest * eiRequest, int invoke_id,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	rina::ScopedLock g(lock_);

	INamespaceManagerPs *nsmps = dynamic_cast<INamespaceManagerPs *>(namespace_manager_->ps);
	assert(nsmps);

	if (!isValidPortId(cdapSessionDescriptor->port_id_)){
		return;
	}

	if (state_ != STATE_WAIT_START_ENROLLMENT) {
		abortEnrollment(remote_peer_->name_, port_id_,
				START_IN_BAD_STATE, true);
		return;
	}

	timer_.cancelTask(last_scheduled_task_);

	bool requiresInitialization = false;

	LOG_IPCP_DBG("Remote IPC Process address: %u", eiRequest->address_);

	if (!eiRequest) {
		requiresInitialization = true;
	} else {
		try {
			if (!nsmps->isValidAddress(eiRequest->address_, remote_peer_->name_.processName,
					remote_peer_->name_.processInstance)) {
				requiresInitialization = true;
			}

			std::list<rina::ApplicationProcessNamingInformation>::const_iterator it;
			for (it = eiRequest->supporting_difs_.begin();
					it != eiRequest->supporting_difs_.end(); ++it) {
				remote_peer_->supporting_difs_.push_back(*it);
			}
		}catch (rina::Exception &e) {
			LOG_IPCP_ERR("%s", e.what());
			sendNegativeStartResponseAndAbortEnrollment(-1, std::string(e.what()), invoke_id);
			return;
		}
	}

	if (requiresInitialization){
		unsigned int address = nsmps->getValidAddress(remote_peer_->name_.processName,
				remote_peer_->name_.processInstance);

		if (address == 0){
			sendNegativeStartResponseAndAbortEnrollment(-1, "Could not assign a valid address", invoke_id);
			return;
		}

		LOG_IPCP_DBG("Remote IPC Process requires initialization, assigning address %u", address);
		eiRequest->address_ = address;
	}

	try {
		rina::RemoteProcessId remote_id;
		remote_id.port_id_ = port_id_;
		rina::RIBObjectValue object_value;

		if (requiresInitialization) {
			object_value.type_ = rina::RIBObjectValue::complextype;
			object_value.complex_value_ = eiRequest;
		}

		rib_daemon_->remoteStartObjectResponse(EncoderConstants::ENROLLMENT_INFO_OBJECT_CLASS,
				EncoderConstants::ENROLLMENT_INFO_OBJECT_NAME, object_value, 0, "",
				invoke_id, remote_id);

		remote_peer_->address_ = eiRequest->address_;
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems sending CDAP message: %s", e.what());
		delete eiRequest;
		sendNegativeStartResponseAndAbortEnrollment(-1, std::string(e.what()), invoke_id);
		return;
	}

	//If initialization is required send the M_CREATEs
	if (requiresInitialization){
		sendDIFStaticInformation();
	}

	sendDIFDynamicInformation();

	//Send the M_STOP request
	try {
		rina::RemoteProcessId remote_id;
		remote_id.port_id_ = port_id_;

		eiRequest->allowed_to_start_early_ = true;
		rina::RIBObjectValue object_value;
		object_value.type_ = rina::RIBObjectValue::complextype;
		object_value.complex_value_ = eiRequest;

		rib_daemon_->remoteStopObject(EncoderConstants::ENROLLMENT_INFO_OBJECT_CLASS,
				EncoderConstants::ENROLLMENT_INFO_OBJECT_NAME, object_value, 0, remote_id, this);

		delete eiRequest;
	} catch(rina::Exception &e) {
		LOG_IPCP_ERR("Problems sending CDAP message: %s", e.what());
		delete eiRequest;
		sendNegativeStartResponseAndAbortEnrollment(-1, std::string(e.what()), invoke_id);
		return;
	}

	//Set timer
	last_scheduled_task_ = new EnrollmentFailedTimerTask(this,
						   	     STOP_ENROLLMENT_RESPONSE_TIMEOUT);
	timer_.scheduleTask(last_scheduled_task_, timeout_);

	LOG_IPCP_DBG("Waiting for stop enrollment response message");
	state_ = STATE_WAIT_STOP_ENROLLMENT_RESPONSE;
}

void EnrollerStateMachine::stopResponse(int result, const std::string& result_reason,
		void * object_value, rina::CDAPSessionDescriptor * session_descriptor) {
	rina::ScopedLock g(lock_);

	if (!isValidPortId(session_descriptor->port_id_)){
		return;
	}

	if (state_ != STATE_WAIT_STOP_ENROLLMENT_RESPONSE) {
		abortEnrollment(remote_peer_->name_, port_id_,
				STOP_RESPONSE_IN_BAD_STATE, true);
		return;
	}

	timer_.cancelTask(last_scheduled_task_);
	if (result != 0){
		state_ = STATE_NULL;
		enrollment_task_->enrollmentFailed(remote_peer_->name_, port_id_,
				result_reason, true);
		return;
	}

	try{
		rina::RIBObjectValue robject_value;
		rina::RemoteProcessId remote_id;
		remote_id.port_id_ = port_id_;

		rib_daemon_->remoteStartObject(EncoderConstants::OPERATIONAL_STATUS_RIB_OBJECT_CLASS,
				EncoderConstants::OPERATIONAL_STATUS_RIB_OBJECT_NAME, robject_value, 0, remote_id, 0);
	} catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems sending CDAP Message: %s", e.what());
	}

	enrollmentCompleted();
}

void EnrollerStateMachine::enrollmentCompleted()
{
	state_ = STATE_ENROLLED;

	createOrUpdateNeighborInformation(true);

	enrollment_task_->enrollmentCompleted(*remote_peer_, false);

	LOG_IPCP_INFO("Remote IPC Process enrolled!");
}

/// Handles the operations related to the "daf.management.enrollment" objects
class EnrollmentRIBObject: public BaseIPCPRIBObject {
public:
	EnrollmentRIBObject(IPCProcess * ipc_process);
	const void* get_value() const;
	void remoteStartObject(void * object_value, int invoke_id,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor);
	void remoteStopObject(void * object_value, int invoke_id,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor);

private:
	void sendErrorMessage(const rina::CDAPSessionDescriptor * cdapSessionDescriptor);

	IPCPEnrollmentTask * enrollment_task_;
	rina::CDAPSessionManagerInterface * cdap_session_manager_;
};

//Class EnrollmentRIBObject
EnrollmentRIBObject::EnrollmentRIBObject(IPCProcess * ipc_process) :
	BaseIPCPRIBObject(ipc_process, EncoderConstants::ENROLLMENT_INFO_OBJECT_CLASS,
			rina::objectInstanceGenerator->getObjectInstance(), EncoderConstants::ENROLLMENT_INFO_OBJECT_NAME) {
	enrollment_task_ = (IPCPEnrollmentTask *) ipc_process->enrollment_task_;
	cdap_session_manager_ = ipc_process->cdap_session_manager_;
}

const void* EnrollmentRIBObject::get_value() const {
	return 0;
}

void EnrollmentRIBObject::remoteStartObject(void * object_value, int invoke_id,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	EnrollerStateMachine * stateMachine = 0;

	try {
		stateMachine = (EnrollerStateMachine *) enrollment_task_->getEnrollmentStateMachine(
				cdapSessionDescriptor->port_id_, false);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems retrieving state machine: %s", e.what());
		sendErrorMessage(cdapSessionDescriptor);
		return;
	}

	if (!stateMachine) {
		LOG_IPCP_ERR("Got a CDAP message that is not for me ");
		return;
	}

	EnrollmentInformationRequest * eiRequest = 0;
	if (object_value) {
		eiRequest = (EnrollmentInformationRequest *) object_value;
	}
	stateMachine->start(eiRequest, invoke_id, cdapSessionDescriptor);
}

void EnrollmentRIBObject::remoteStopObject(void * object_value, int invoke_id,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	EnrolleeStateMachine * stateMachine = 0;

	try {
		stateMachine = (EnrolleeStateMachine *) enrollment_task_->getEnrollmentStateMachine(
				cdapSessionDescriptor->port_id_, false);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems retrieving state machine: %s", e.what());
		sendErrorMessage(cdapSessionDescriptor);
		return;
	}

	if (!stateMachine) {
		LOG_IPCP_ERR("Got a CDAP message that is not for me");
		return;
	}

	EnrollmentInformationRequest * eiRequest = 0;
	if (object_value) {
		eiRequest = (EnrollmentInformationRequest *) object_value;
	}
	stateMachine->stop(eiRequest, invoke_id , cdapSessionDescriptor);
}

void EnrollmentRIBObject::sendErrorMessage(const rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	try{
		rina::RemoteProcessId remote_id;
		remote_id.port_id_ = cdapSessionDescriptor->port_id_;

		rib_daemon_->closeApplicationConnection(remote_id, 0);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems sending CDAP message: %s", e.what());
	}
}

class EnrollmentTaskPs: public IPCPEnrollmentTaskPS {
public:
	EnrollmentTaskPs(IPCProcess * ipcp_);
        virtual ~EnrollmentTaskPs() {};
        void connect_received(const rina::CDAPMessage& cdapMessage,
        		      rina::CDAPSessionDescriptor * session_descriptor);
        void connect_response_received(int result,
        			       const std::string& result_reason,
        			       rina::CDAPSessionDescriptor * session_descriptor);
        void process_authentication_message(const rina::CDAPMessage& message,
        			rina::CDAPSessionDescriptor * session_descriptor);
	void authentication_completed(int port_id, bool success);
        void initiate_enrollment(const rina::NMinusOneFlowAllocatedEvent & event,
        			 rina::EnrollmentRequest * request);
        void inform_ipcm_about_failure(IEnrollmentStateMachine * state_machine);
        void set_dif_configuration(const rina::DIFConfiguration& dif_configuration);
        int set_policy_set_param(const std::string& name,
        		         const std::string& value);

private:
        void populate_rib();
        IEnrollmentStateMachine * createEnrollmentStateMachine(
        		const rina::ApplicationProcessNamingInformation& apNamingInfo, int portId,
                	bool enrollee, const rina::ApplicationProcessNamingInformation& supportingDifName);

private:
        IPCProcess * ipcp;
        IPCPEnrollmentTask * et;
        int timeout;
        rina::Lockable lock;
        rina::IPCResourceManager * irm;
        rina::IRIBDaemon * rib_daemon;
};

EnrollmentTaskPs::EnrollmentTaskPs(IPCProcess * ipcp_) :
		ipcp(ipcp_), et (ipcp_->enrollment_task_), timeout(0),
		irm(ipcp->resource_allocator_->get_n_minus_one_flow_manager()),
		rib_daemon(ipcp->rib_daemon_)
{
	populate_rib();
}

void EnrollmentTaskPs::populate_rib()
{
	try{
		rina::BaseRIBObject * ribObject = new EnrollmentRIBObject(ipcp);
		rib_daemon->addRIBObject(ribObject);
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems adding object to RIB Daemon: %s", e.what());
	}
}

void EnrollmentTaskPs::connect_received(const rina::CDAPMessage& cdapMessage,
        		 	        rina::CDAPSessionDescriptor * session_descriptor)
{
	rina::ScopedLock g(lock);

	try{
		rina::FlowInformation flowInformation =
			irm->getNMinus1FlowInformation(session_descriptor->port_id_);
		EnrollerStateMachine * enrollmentStateMachine =
			(EnrollerStateMachine *) createEnrollmentStateMachine(session_descriptor->get_destination_application_process_naming_info(),
									      session_descriptor->port_id_,
									      false,
									      flowInformation.difName);
		enrollmentStateMachine->connect(cdapMessage, session_descriptor);
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems: %s", e.what());

		try {
			rina::RemoteProcessId remote_id;
			remote_id.port_id_ = session_descriptor->port_id_;

			rib_daemon->openApplicationConnectionResponse(rina::AuthPolicy(),
								      session_descriptor->dest_ae_inst_,
								      session_descriptor->dest_ae_name_,
								      session_descriptor->dest_ap_inst_,
								      session_descriptor->dest_ap_name_,
								      rina::CDAPErrorCodes::CONNECTION_REJECTED_ERROR,
								      std::string(e.what()),
								      session_descriptor->src_ae_inst_,
								      session_descriptor->src_ae_name_,
								      session_descriptor->src_ap_inst_,
								      session_descriptor->src_ap_name_,
								      cdapMessage.invoke_id_,
								      remote_id);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems sending CDAP message: %s", e.what());
		}

		et->deallocateFlow(session_descriptor->port_id_);
	}
}

void EnrollmentTaskPs::connect_response_received(int result,
			     	     	         const std::string& result_reason,
			     	     	         rina::CDAPSessionDescriptor * session_descriptor)
{
	rina::ScopedLock g(lock);

	try{
		EnrolleeStateMachine * stateMachine =
			(EnrolleeStateMachine*) et->getEnrollmentStateMachine(session_descriptor->port_id_,
									     false);
		stateMachine->connectResponse(result, result_reason);
	}catch(rina::Exception &e){
		//Error getting the enrollment state machine
		LOG_IPCP_ERR("Problems getting enrollment state machine: %s",
				e.what());
		try {
			rina::RemoteProcessId remote_id;
			remote_id.port_id_ = session_descriptor->port_id_;

			rib_daemon->closeApplicationConnection(remote_id, 0);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems closing application connection: %s",
				     e.what());
		}

		et->deallocateFlow(session_descriptor->port_id_);
	}
}

void EnrollmentTaskPs::process_authentication_message(const rina::CDAPMessage& message,
			rina::CDAPSessionDescriptor * session_descriptor)
{
	rina::ScopedLock g(lock);

	try {
		IEnrollmentStateMachine * stateMachine =
			et->getEnrollmentStateMachine(session_descriptor->port_id_, false);
		stateMachine->process_authentication_message(message,
							     session_descriptor);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems getting enrollment state machine: %s",
			     e.what());
		try {
			rina::RemoteProcessId remote_id;
			remote_id.port_id_ = session_descriptor->port_id_;

			rib_daemon->closeApplicationConnection(remote_id, 0);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems closing application connection: %s",
				     e.what());
		}
		et->deallocateFlow(session_descriptor->port_id_);
	}
}

void EnrollmentTaskPs::authentication_completed(int port_id, bool success)
{
	rina::ScopedLock g(lock);

	try {
		IEnrollmentStateMachine * stateMachine =
				et->getEnrollmentStateMachine(port_id, false);
		stateMachine->authentication_completed(success);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems getting enrollment state machine: %s",
				e.what());
		try {
			rina::RemoteProcessId remote_id;
			remote_id.port_id_ = port_id;

			rib_daemon->closeApplicationConnection(remote_id, 0);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems closing application connection: %s",
					e.what());
		}
		et->deallocateFlow(port_id);
	}
}

void EnrollmentTaskPs::initiate_enrollment(const rina::NMinusOneFlowAllocatedEvent & event,
					      rina::EnrollmentRequest * request)
{
	rina::ScopedLock g(lock);

	EnrolleeStateMachine * enrollmentStateMachine = 0;

	//1 Tell the enrollment task to create a new Enrollment state machine
	try{
		enrollmentStateMachine = (EnrolleeStateMachine *) createEnrollmentStateMachine(
				request->neighbor_->name_, event.flow_information_.portId, true,
				event.flow_information_.difName);
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problem retrieving enrollment state machine: %s", e.what());
		delete request;
		return;
	}

	//2 Tell the enrollment state machine to initiate the enrollment
	// (will require an M_CONNECT message and a port Id)
	try{
		enrollmentStateMachine->initiateEnrollment(
				request, event.flow_information_.portId);
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems initiating enrollment: %s", e.what());
	}
}

void EnrollmentTaskPs::inform_ipcm_about_failure(IEnrollmentStateMachine * state_machine)
{
	EnrolleeStateMachine * sm = dynamic_cast<EnrolleeStateMachine *>(state_machine);
	if (sm) {
		rina::EnrollmentRequest * request = sm->enrollment_request_;
		if (request) {
			if (request->ipcm_initiated_) {
				try {
					rina::extendedIPCManager->enrollToDIFResponse(request->event_, -1,
							std::list<rina::Neighbor>(), ipcp->get_dif_information());
				} catch (rina::Exception &e) {
					LOG_IPCP_ERR("Problems sending message to IPC Manager: %s", e.what());
				}
			}
		}
	}
}

IEnrollmentStateMachine * EnrollmentTaskPs::createEnrollmentStateMachine(
        	const rina::ApplicationProcessNamingInformation& apNamingInfo, int portId,
        	bool enrollee, const rina::ApplicationProcessNamingInformation& supportingDifName)
{
	IEnrollmentStateMachine * stateMachine = 0;

	if (apNamingInfo.entityName.compare("") == 0 ||
			apNamingInfo.entityName.compare(IPCProcess::MANAGEMENT_AE) == 0) {
		if (enrollee){
			stateMachine = new EnrolleeStateMachine(ipcp,
					apNamingInfo, timeout);
		}else{
			rina::ApplicationProcessNamingInformation * sdname =
					new rina::ApplicationProcessNamingInformation(supportingDifName.processName,
							supportingDifName.processInstance);
			stateMachine = new EnrollerStateMachine(ipcp,
					apNamingInfo, timeout, sdname);
		}

		et->add_enrollment_state_machine(portId, stateMachine);

		LOG_IPCP_DBG("Created a new Enrollment state machine for remote IPC process: %s",
				apNamingInfo.getEncodedString().c_str());
		return stateMachine;
	}

	throw rina::Exception("Unknown application entity for enrollment");
}

void EnrollmentTaskPs::set_dif_configuration(const rina::DIFConfiguration& dif_configuration)
{
	rina::PolicyConfig psconf = dif_configuration.et_configuration_.policy_set_;
	timeout = psconf.get_param_value_as_int(EnrollmentTask::ENROLL_TIMEOUT_IN_MS);
}

int EnrollmentTaskPs::set_policy_set_param(const std::string& name,
                                            const std::string& value)
{
        LOG_IPCP_DBG("No policy-set-specific parameters to set (%s, %s)",
                        name.c_str(), value.c_str());
        return -1;
}

extern "C" rina::IPolicySet *
createEnrollmentTaskPs(rina::ApplicationEntity * ctx)
{
	IPCProcess * ipcp = dynamic_cast<IPCProcess *>(ctx->get_application_process());

        if (!ipcp) {
                return NULL;
        }

        return new EnrollmentTaskPs(ipcp);
}

extern "C" void
destroyEnrollmentTaskPs(rina::IPolicySet * ps)
{
        if (ps) {
                delete ps;
        }
}

}
