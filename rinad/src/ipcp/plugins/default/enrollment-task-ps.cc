//
// Default policy set for Enrollment Task
//
//    Eduard Grasa <eduard.grasa@i2cat.net>
//    Bernat Gaston <bernat.gaston@i2cat.net>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301  USA
//

#define IPCP_MODULE "enrollment-task-ps-default"
#include "../../ipcp-logging.h"
#include <string>
#include <climits>
#include <assert.h>

#include "ipcp/ipc-process.h"
#include "ipcp/enrollment-task.h"
#include "ipcp/flow-allocator.h"
#include "ipcp/namespace-manager.h"
#include "ipcp/resource-allocator.h"

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
	sec_man_ = ipc_process->security_manager_;
}

void BaseEnrollmentStateMachine::sendDIFDynamicInformation()
{
	//Send DirectoryForwardingTableEntries
	sendDFTEntries();

	//Send neighbors (including myself)
	sendNeighbors();
}

void BaseEnrollmentStateMachine::sendDFTEntries()
{
	std::list<rina::DirectoryForwardingTableEntry> dftEntries =
			ipc_process_->namespace_manager_->getDFTEntries();

	if (dftEntries.size() == 0) {
		LOG_IPCP_DBG("No DFT entries to be sent");
		return;
	}

	try {
		encoders::DFTEListEncoder encoder;
		rina::cdap_rib::obj_info_t obj;
		obj.class_ = DFTRIBObj::class_name;
		obj.name_ = DFTRIBObj::object_name;
		encoder.encode(dftEntries, obj.value_);
		rina::cdap_rib::filt_info_t filt;
		rina::cdap_rib::flags_t flags;

		rib_daemon_->getProxy()->remote_create(con,
						       obj,
						       flags,
						       filt,
						       NULL);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems sending DFT entries: %s",
			     e.what());
	}
}

/// Handles the operations related to the "daf.management.enrollment" objects
class EnrollmentRIBObject: public IPCPRIBObj {
public:
	EnrollmentRIBObject(IPCProcess * ipc_process);
	const std::string& get_class() const {
		return class_name;
	};

	void start(const rina::cdap_rib::con_handle_t &con,
		   const std::string& fqn,
		   const std::string& class_,
		   const rina::cdap_rib::filt_info_t &filt,
		   const int invoke_id,
		   const rina::ser_obj_t &obj_req,
		   rina::ser_obj_t &obj_reply,
		   rina::cdap_rib::res_info_t& res);

	void stop(const rina::cdap_rib::con_handle_t &con,
		  const std::string& fqn,
		  const std::string& class_,
		  const rina::cdap_rib::filt_info_t &filt,
		  const int invoke_id,
		  const rina::ser_obj_t &obj_req,
		  rina::ser_obj_t &obj_reply,
		  rina::cdap_rib::res_info_t& res);

	const static std::string class_name;
	const static std::string object_name;

private:
	void sendErrorMessage(unsigned int port_id);

	IPCPEnrollmentTask * enrollment_task_;
};

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
	void initiateEnrollment(const rina::EnrollmentRequest& enrollmentRequest,
				int portId);

	void process_authentication_message(const rina::cdap::CDAPMessage& message,
				   	    const rina::cdap_rib::con_handle_t &con);

	void authentication_completed(bool success);

	/// Called by the EnrollmentTask when it got an M_CONNECT_R message
	/// @param result
	/// @param result_reason
	void connectResponse(int result, const std::string& result_reason);

	void remoteStartResult(const rina::cdap_rib::con_handle_t &con,
			       const rina::cdap_rib::obj_info_t &obj,
			       const rina::cdap_rib::res_info_t &res);

	/// Stop enrollment request received. Check if I have enough information, if not
	/// ask for more with M_READs.
	/// Have to check if I can start operating (if not wait
	/// until M_START operationStatus). If I can start and have enough information,
	/// create or update all the objects received during the enrollment phase.
	void stop(const configs::EnrollmentInformationRequest& eiRequest,
		  int invoke_id,
		  rina::cdap_rib::con_handle_t con_handle);

	/// See if the response is valid and contains an object. See if more objects
	/// are required. If not, start
	void remoteReadResult(const rina::cdap_rib::con_handle_t &con,
			      const rina::cdap_rib::obj_info_t &obj,
			      const rina::cdap_rib::res_info_t &res);

	void start(int result,
		   const std::string& result_reason,
		   rina::cdap_rib::con_handle_t con_handle);

	rina::EnrollmentRequest enrollment_request_;

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
					   int timeout):
		BaseEnrollmentStateMachine(ipc_process,
					   remote_naming_info,
					   timeout,
					   0)
{
	was_dif_member_before_enrollment_ = false;
	last_scheduled_task_ = 0;
	allowed_to_start_early_ = false;
	stop_request_invoke_id_ = 0;
}

void EnrolleeStateMachine::initiateEnrollment(const rina::EnrollmentRequest& enrollmentRequest,
					      int portId)
{
	rina::ScopedLock g(lock_);

	enrollment_request_ = enrollmentRequest;
	remote_peer_.address_ = enrollment_request_.neighbor_.address_;
	remote_peer_.name_ = enrollment_request_.neighbor_.name_;
	remote_peer_.supporting_dif_name_ = enrollment_request_.neighbor_.supporting_dif_name_;
	remote_peer_.underlying_port_id_ = portId;
	remote_peer_.supporting_difs_ = enrollment_request_.neighbor_.supporting_difs_;

	if (state_ != STATE_NULL) {
		throw rina::Exception("Enrollee state machine not in NULL state");
	}

	try{
		rina::AuthSDUProtectionProfile profile =
				sec_man_->get_auth_sdup_profile(remote_peer_.supporting_dif_name_.processName);
		auth_ps_ = ipc_process_->security_manager_->get_auth_policy_set(profile.authPolicy.name_);
		if (!auth_ps_) {
			abortEnrollment(remote_peer_.name_,
					con.port_id,
					std::string("Unsupported authentication policy set"),
					true);
			return;
		}

		rina::cdap_rib::ep_info_t src_ep;
		rina::cdap_rib::ep_info_t dest_ep;
		rina::cdap_rib::vers_info_t vers;

		src_ep.ap_name_ = ipc_process_->get_name();
		src_ep.ap_inst_ = ipc_process_->get_instance();
		src_ep.ae_name_ = IPCProcess::MANAGEMENT_AE;

		dest_ep.ap_name_ = remote_peer_.name_.processName;
		dest_ep.ap_inst_ = remote_peer_.name_.processInstance;
		dest_ep.ae_name_ = IPCProcess::MANAGEMENT_AE;

		rina::cdap_rib::auth_policy_t auth = auth_ps_->get_auth_policy(portId,
									       dest_ep,
									       profile);

		vers.version_ = 0x01;

		rib_daemon_->getProxy()->remote_open_connection(vers,
								src_ep,
								dest_ep,
								auth,
								portId);
		con.port_id = portId;

		//Set timer
		last_scheduled_task_ = new EnrollmentFailedTimerTask(this, CONNECT_RESPONSE_TIMEOUT);
		timer_.scheduleTask(last_scheduled_task_, timeout_);

		//Update state
		state_ = STATE_WAIT_CONNECT_RESPONSE;
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems sending M_CONNECT message: %s", e.what());
		abortEnrollment(remote_peer_.name_,
				con.port_id,
				std::string(e.what()),
				true);
	}
}

void EnrolleeStateMachine::process_authentication_message(const rina::cdap::CDAPMessage& message,
			   	    	    	    	  const rina::cdap_rib::con_handle_t &con_handle)
{
	lock_.lock();

	if (!auth_ps_ || state_ != STATE_WAIT_CONNECT_RESPONSE) {
		lock_.unlock();
		abortEnrollment(remote_peer_.name_,
				con.port_id,
				"Authentication policy is null or wrong state",
				true);
		return;
	}

	int result = auth_ps_->process_incoming_message(message,
							con_handle.port_id);

	lock_.unlock();

	if (result == rina::IAuthPolicySet::FAILED) {
		abortEnrollment(remote_peer_.name_,
				con.port_id,
				"Authentication failed",
				true);
	} else if (result == rina::IAuthPolicySet::SUCCESSFULL) {
		LOG_IPCP_INFO("Authentication was successful, waiting for M_CONNECT_R");
	}
}

void EnrolleeStateMachine::authentication_completed(bool success)
{
	if (!success) {
		abortEnrollment(remote_peer_.name_,
				con.port_id,
				"Authentication failed",
				true);
	} else {
		LOG_IPCP_INFO("Authentication was successful, waiting for M_CONNECT_R");
	}
}

void EnrolleeStateMachine::connectResponse(int result,
					   const std::string& result_reason)
{
	rina::ScopedLock g(lock_);

	if (state_ != STATE_WAIT_CONNECT_RESPONSE) {
		abortEnrollment(remote_peer_.name_,
				con.port_id,
				"Message received in wrong order",
				true);
		return;
	}

	timer_.cancelTask(last_scheduled_task_);
	if (result != 0) {
		state_ = STATE_NULL;
		enrollment_task_->enrollmentFailed(remote_peer_.name_,
						   con.port_id,
						   result_reason,
						   true);
		return;
	}

	//Send M_START with EnrollmentInformation object
	try{
		configs::EnrollmentInformationRequest eiRequest;
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
			difInformation.dif_name_ = enrollment_request_.event_.dafName;
			ipc_process_->set_dif_information(difInformation);
		}

		encoders::EnrollmentInformationRequestEncoder encoder;
		rina::cdap_rib::obj_info_t obj;
		obj.class_ = EnrollmentRIBObject::class_name;
		obj.name_ = EnrollmentRIBObject::object_name;
		encoder.encode(eiRequest, obj.value_);
		rina::cdap_rib::flags_t flags;
		rina::cdap_rib::filt_info_t filt;

		rib_daemon_->getProxy()->remote_start(con,
						      obj,
						      flags,
						      filt,
						      this);

		LOG_IPCP_DBG("Sent a M_START Message to portid: %d",
			     con.port_id);

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

void EnrolleeStateMachine::remoteStartResult(const rina::cdap_rib::con_handle_t &con_handle,
					     const rina::cdap_rib::obj_info_t &obj,
					     const rina::cdap_rib::res_info_t &res)
{
	rina::ScopedLock g(lock_);

	if (!isValidPortId(con_handle.port_id)){
		return;
	}

	if (state_ != STATE_WAIT_START_ENROLLMENT_RESPONSE) {
		abortEnrollment(remote_peer_.name_,
				con.port_id,
				START_IN_BAD_STATE,
				true);
		return;
	}

	timer_.cancelTask(last_scheduled_task_);
	if (res.code_ != rina::cdap_rib::CDAP_SUCCESS) {
		state_ = STATE_NULL;
		enrollment_task_->enrollmentFailed(remote_peer_.name_,
						   con.port_id,
						   res.reason_,
						   true);
		return;
	}

	//Update address
	encoders::EnrollmentInformationRequestEncoder encoder;
	configs::EnrollmentInformationRequest eiResp;
	encoder.decode(obj.value_, eiResp);
	if (eiResp.address_ != 0) {
		ipc_process_->get_dif_information().dif_configuration_.address_ = eiResp.address_;
	}

	//Set timer
	last_scheduled_task_ = new EnrollmentFailedTimerTask(this, STOP_ENROLLMENT_TIMEOUT);
	timer_.scheduleTask(last_scheduled_task_, timeout_);

	//Update state
	state_ = STATE_WAIT_STOP_ENROLLMENT_RESPONSE;
}

void EnrolleeStateMachine::stop(const configs::EnrollmentInformationRequest& eiRequest,
				int invoke_id,
				rina::cdap_rib::con_handle_t con_handle)
{
	rina::ScopedLock g(lock_);

	if (!isValidPortId(con_handle.port_id)){
		return;
	}

	if (state_ != STATE_WAIT_STOP_ENROLLMENT_RESPONSE) {
		abortEnrollment(remote_peer_.name_,
				con.port_id,
				STOP_IN_BAD_STATE,
				true);
		return;
	}

	timer_.cancelTask(last_scheduled_task_);
	//Check if I'm allowed to start early
	if (!eiRequest.allowed_to_start_early_){
		abortEnrollment(remote_peer_.name_,
				con.port_id,
				STOP_WITH_NO_OBJECT_VALUE,
				true);
		return;
	}

	allowed_to_start_early_ = eiRequest.allowed_to_start_early_;
	stop_request_invoke_id_ = invoke_id;

	//Request more information or start
	try{
		requestMoreInformationOrStart();
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems requesting more information or starting: %s",
			      e.what());
		abortEnrollment(remote_peer_.name_,
				con.port_id,
				std::string(e.what()),
				true);
	}
}

void EnrolleeStateMachine::requestMoreInformationOrStart()
{
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
	if (allowed_to_start_early_){
		try{
			commitEnrollment();

			rina::cdap_rib::flags_t flags;
			rina::cdap_rib::filt_info_t filt;
			rina::cdap_rib::res_info_t res;
			res.code_ = rina::cdap_rib::CDAP_SUCCESS;

			rina::cdap::getProvider()->send_stop_result(con,
								    flags,
								    res,
								    stop_request_invoke_id_);
			enrollmentCompleted();
		}catch(rina::Exception &e){
			LOG_IPCP_ERR("Problems sending CDAP message: %s",
				     e.what());

			rina::cdap_rib::flags_t flags;
			rina::cdap_rib::filt_info_t filt;
			rina::cdap_rib::res_info_t res;
			res.code_ = rina::cdap_rib::CDAP_ERROR;
			res.reason_ = PROBLEMS_COMMITTING_ENROLLMENT_INFO;

			rina::cdap::getProvider()->send_stop_result(con,
								    flags,
								    res,
								    stop_request_invoke_id_);

			abortEnrollment(remote_peer_.name_,
					con.port_id,
					PROBLEMS_COMMITTING_ENROLLMENT_INFO,
					true);
		}

		return;
	}

	try {
		rina::cdap_rib::flags_t flags;
		rina::cdap_rib::filt_info_t filt;
		rina::cdap_rib::res_info_t res;
		res.code_ = rina::cdap_rib::CDAP_SUCCESS;

		rina::cdap::getProvider()->send_stop_result(con,
							    flags,
							    res,
							    stop_request_invoke_id_);
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems sending CDAP message: %s", e.what());
	}

	last_scheduled_task_ = new EnrollmentFailedTimerTask(this, START_TIMEOUT);
	timer_.scheduleTask(last_scheduled_task_, timeout_);
	state_ = STATE_WAIT_START;
}

bool EnrolleeStateMachine::sendNextObjectRequired()
{
	bool result = false;
	rina::DIFInformation difInformation = ipc_process_->get_dif_information();

	rina::cdap_rib::obj_info_t obj;
	if (!difInformation.dif_configuration_.efcp_configuration_.data_transfer_constants_.isInitialized()) {
		obj.class_ = DataTransferRIBObj::class_name;
		obj.name_ = DataTransferRIBObj::object_name;
		result = true;
	} else if (difInformation.dif_configuration_.efcp_configuration_.qos_cubes_.size() == 0){
		obj.class_ = QoSCubesRIBObject::class_name;
		obj.name_ = QoSCubesRIBObject::object_name;
		result = true;
	}else if (ipc_process_->get_neighbors().size() == 0){
		obj.class_ = NeighborsRIBObj::class_name;
		obj.name_ = NeighborsRIBObj::object_name;
		result = true;
	}

	if (result) {
		try{
			rina::cdap_rib::flags_t flags;
			rina::cdap_rib::filt_info_t filt;
			rib_daemon_->getProxy()->remote_read(con,
							     obj,
							     flags,
							     filt,
							     this);
		} catch (rina::Exception &e) {
			LOG_IPCP_WARN("Problems executing remote operation: %s", e.what());
		}
	}

	return result;
}

void EnrolleeStateMachine::commitEnrollment()
{
	//TODO: Do nothing? check if this method is required
}

void EnrolleeStateMachine::enrollmentCompleted()
{
	state_ = STATE_ENROLLED;

	//Create or update the neighbor information in the RIB
	createOrUpdateNeighborInformation(true);

	//Send DirectoryForwardingTableEntries
	sendDFTEntries();

	enrollment_task_->enrollmentCompleted(remote_peer_, true);

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
	if (enrollment_request_.ipcm_initiated_){
		try {
			std::list<rina::Neighbor> neighbors;
			neighbors.push_back(remote_peer_);
			rina::extendedIPCManager->enrollToDIFResponse(enrollment_request_.event_,
								      0,
								      neighbors,
								      ipc_process_->get_dif_information());
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems sending message to IPC Manager: %s", e.what());
		}
	}

	LOG_IPCP_INFO("Remote IPC Process enrolled!");
}

void EnrolleeStateMachine::remoteReadResult(const rina::cdap_rib::con_handle_t &con_handle,
		      	      	      	    const rina::cdap_rib::obj_info_t &obj,
		      	      	      	    const rina::cdap_rib::res_info_t &res)
{
	rina::ScopedLock g(lock_);

	if (!isValidPortId(con_handle.port_id)){
		return;
	}

	if (state_ != STATE_WAIT_READ_RESPONSE) {
		abortEnrollment(remote_peer_.name_,
				con_handle.port_id,
				READ_RESPONSE_IN_BAD_STATE,
				true);
		return;
	}

	timer_.cancelTask(last_scheduled_task_);

	if (res.code_ != rina::cdap_rib::CDAP_SUCCESS ||
			obj.value_.message_ == 0){
		abortEnrollment(remote_peer_.name_,
				con_handle.port_id,
				res.reason_,
				true);
		return;
	}

	if (obj.name_.compare(DataTransferRIBObj::object_name) == 0) {
		encoders::DataTransferConstantsEncoder encoder;
		rina::DataTransferConstants constants;
		encoder.decode(obj.value_, constants);
		ipc_process_->get_dif_information().dif_configuration_.efcp_configuration_.data_transfer_constants_ = constants;
	}else if (obj.name_.compare(QoSCubesRIBObject::object_name) == 0) {
		encoders::QoSCubeListEncoder encoder;
		std::list<rina::QoSCube> cubes;
		encoder.decode(obj.value_, cubes);
		std::list<rina::QoSCube>::const_iterator it;
		for (it = cubes.begin(); it != cubes.end(); ++it) {
			ipc_process_->resource_allocator_->addQoSCube(*it);
		}
	}else if (obj.name_.compare(NeighborsRIBObj::object_name) == 0) {
		encoders::NeighborListEncoder encoder;
		std::list<rina::Neighbor> neighbors;
		encoder.decode(obj.value_, neighbors);

		std::list<rina::Neighbor>::const_iterator it;
		for (it = neighbors.begin(); it != neighbors.end(); ++it)
			ipc_process_->enrollment_task_->add_neighbor(*it);

	}else{
		LOG_IPCP_WARN("The object to be created is not required for enrollment");
	}

	//Request more information or proceed with the enrollment program
	requestMoreInformationOrStart();
}

void EnrolleeStateMachine::start(int result,
				 const std::string& result_reason,
				 rina::cdap_rib::con_handle_t con_handle)
{
	rina::ScopedLock g(lock_);

	if (!isValidPortId(con_handle.port_id)){
		return;
	}

	if (state_ == STATE_ENROLLED) {
		return;
	}

	if (state_ != STATE_WAIT_START) {
		abortEnrollment(remote_peer_.name_,
				con_handle.port_id,
				START_IN_BAD_STATE,
				true);
		return;
	}

	timer_.cancelTask(last_scheduled_task_);

	if (result != 0){
		abortEnrollment(remote_peer_.name_,
				con_handle.port_id,
				result_reason,
				true);
		return;
	}

	try{
		commitEnrollment();
		enrollmentCompleted();
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems commiting enrollment: %s", e.what());
		abortEnrollment(remote_peer_.name_,
				con_handle.port_id,
				PROBLEMS_COMMITTING_ENROLLMENT_INFO,
				true);
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
	void connect(const rina::cdap::CDAPMessage& message,
		     const rina::cdap_rib::con_handle_t &con);

	void process_authentication_message(const rina::cdap::CDAPMessage& message,
					    const rina::cdap_rib::con_handle_t &con);

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
	void start(configs::EnrollmentInformationRequest& eiRequest,
		   int invoke_id, const rina::cdap_rib::con_handle_t &con);

	/// The response of the stop operation has been received, send M_START operation without
	/// waiting for an answer and consider the process enrolled
	/// @param result
	/// @param result_reason
	/// @param object_value
	/// @param cdapSessionDescriptor
	void remoteStopResult(const rina::cdap_rib::con_handle_t &con,
			      const rina::cdap_rib::obj_info_t &obj,
			      const rina::cdap_rib::res_info_t &res);

private:
	/// Send a negative response to the M_START enrollment message
	/// @param result the error code
	/// @param resultReason the reason of the bad result
	/// @param requestMessage the received M_START enrollment message
	void sendNegativeStartResponseAndAbortEnrollment(rina::cdap_rib::res_code_t result,
							 const std::string&
							 resultReason,
							 int invoke_id);

        void authentication_successful();

        /// Send all the information required to start operation to
        /// the IPC process that is enrolling to me
        void sendDIFStaticInformation();

        void enrollmentCompleted();

	IPCPSecurityManager * security_manager_;
	INamespaceManager * namespace_manager_;
	int connect_message_invoke_id_;
	rina::cdap_rib::con_handle_t con_handle_;
};

//Class EnrollerStateMachine
EnrollerStateMachine::EnrollerStateMachine(IPCProcess * ipc_process,
					   const rina::ApplicationProcessNamingInformation& remote_naming_info,
					   int timeout,
					   rina::ApplicationProcessNamingInformation * supporting_dif_name):
		BaseEnrollmentStateMachine(ipc_process,
					   remote_naming_info,
					   timeout,
					   supporting_dif_name)
{
	security_manager_ = ipc_process->security_manager_;
	namespace_manager_ = ipc_process->namespace_manager_;
	enroller_ = true;
	connect_message_invoke_id_ = 0;
}

void EnrollerStateMachine::connect(const rina::cdap::CDAPMessage& message,
				   const rina::cdap_rib::con_handle_t &con_handle)
{
	lock_.lock();

	if (state_ != STATE_NULL) {
		lock_.unlock();
		abortEnrollment(remote_peer_.name_,
				con_handle.port_id,
				CONNECT_IN_NOT_NULL,
				true);
		return;
	}

	LOG_IPCP_DBG("Authenticating IPC process %s-%s ...",
		      con_handle.dest_.ap_name_.c_str(),
		      con_handle.dest_.ap_inst_.c_str());
	remote_peer_.name_.processName = con_handle.dest_.ap_name_;
	remote_peer_.name_.processInstance = con_handle.dest_.ap_inst_;
	connect_message_invoke_id_ = message.invoke_id_;
	con.port_id = con_handle.port_id;
	con_handle_ = con_handle;

	auth_ps_ = security_manager_->get_auth_policy_set(message.auth_policy_.name);
	if (!auth_ps_) {
		lock_.unlock();
		abortEnrollment(remote_peer_.name_,
				con.port_id,
				std::string("Could not find auth policy set"),
				true);
		return;
	}

	//TODO pass auth_value and auth_type in the function interface
	rina::AuthSDUProtectionProfile profile =
			sec_man_->get_auth_sdup_profile(remote_peer_.supporting_dif_name_.processName);
	rina::IAuthPolicySet::AuthStatus auth_status =
			auth_ps_->initiate_authentication(message.auth_policy_,
							  profile,
							  con_handle.dest_,
							  con.port_id);
	if (auth_status == rina::IAuthPolicySet::FAILED) {
		lock_.unlock();
		abortEnrollment(remote_peer_.name_,
				con.port_id,
				std::string("Authentication failed"),
				true);
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

void EnrollerStateMachine::process_authentication_message(const rina::cdap::CDAPMessage& message,
					    	          const rina::cdap_rib::con_handle_t &con_handle)
{
	lock_.lock();

	if (!auth_ps_ || state_ != STATE_AUTHENTICATING) {
		lock_.unlock();
		abortEnrollment(remote_peer_.name_,
				con.port_id,
				"Authentication policy is null or wrong state",
				true);
		return;
	}

	int result = auth_ps_->process_incoming_message(message,
							con_handle.port_id);

	if (result == rina::IAuthPolicySet::IN_PROGRESS) {
		LOG_IPCP_DBG("Authentication still in progress");
		lock_.unlock();
		return;
	}

	timer_.cancelTask(last_scheduled_task_);

	lock_.unlock();

	if (result == rina::IAuthPolicySet::FAILED) {
		abortEnrollment(remote_peer_.name_,
				con.port_id,
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
		abortEnrollment(remote_peer_.name_,
				con.port_id,
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
	if (!smps->isAllowedToJoinDIF(remote_peer_)) {
		LOG_IPCP_WARN("Security Manager rejected enrollment attempt, aborting enrollment");
		abortEnrollment(remote_peer_.name_,
				con.port_id,
				ENROLLMENT_NOT_ALLOWED,
				true);
		return;
	}

	//Send M_CONNECT_R
	try{
		rina::cdap_rib::res_info_t res;
		res.code_ = rina::cdap_rib::CDAP_SUCCESS;
		rina::cdap::getProvider()->send_open_connection_result(con_handle_,
								       res,
								       connect_message_invoke_id_);

		//Set timer
		last_scheduled_task_ = new EnrollmentFailedTimerTask(this, START_ENROLLMENT_TIMEOUT);
		timer_.scheduleTask(last_scheduled_task_, timeout_);
		LOG_IPCP_DBG("M_CONNECT_R sent to portID %d. Waiting for start enrollment request message",
			     con.port_id);

		state_ = STATE_WAIT_START_ENROLLMENT;
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems sending CDAP message: %s", e.what());
		abortEnrollment(remote_peer_.name_,
				con.port_id,
				std::string(e.what()),
				true);
	}
}

void EnrollerStateMachine::sendNegativeStartResponseAndAbortEnrollment(rina::cdap_rib::res_code_t result,
								       const std::string&
								       resultReason,
								       int invoke_id)
{
	try{
		rina::cdap_rib::res_info_t res;
		res.code_ = result;
		res.reason_ = resultReason;
		rina::cdap_rib::obj_info_t obj;
		rina::cdap_rib::flags_t flags;

		rina::cdap::getProvider()->send_start_result(con,
							     obj,
							     flags,
							     res,
							     invoke_id);

		abortEnrollment(remote_peer_.name_,
				con.port_id,
				resultReason,
				true);
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems sending CDAP message: %s", e.what());
	}
}

void EnrollerStateMachine::sendDIFStaticInformation()
{
	std::list<rina::WhatevercastName> names =
			ipc_process_->namespace_manager_->get_whatevercast_names();

	if (names.size() > 0) {
		try {
			encoders::WhatevercastNameListEncoder encoder;
			rina::cdap_rib::obj_info_t obj;
			obj.class_ = WhateverCastNamesRIBObj::class_name;
			obj.name_ = WhateverCastNamesRIBObj::object_name;
			encoder.encode(names, obj.value_);
			rina::cdap_rib::filt_info_t filt;
			rina::cdap_rib::flags_t flags;

			rib_daemon_->getProxy()->remote_create(con,
							       obj,
							       flags,
							       filt,
							       NULL);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems sending Whatevercast names: %s",
				     e.what());
		}
	}

	try {
		encoders::DataTransferConstantsEncoder encoder;
		rina::cdap_rib::obj_info_t obj;
		obj.class_ = DataTransferRIBObj::class_name;
		obj.name_ = DataTransferRIBObj::object_name;
		encoder.encode(ipc_process_->get_dif_information().dif_configuration_.
			efcp_configuration_.data_transfer_constants_, obj.value_);
		rina::cdap_rib::filt_info_t filt;
		rina::cdap_rib::flags_t flags;

		rib_daemon_->getProxy()->remote_create(con,
						       obj,
						       flags,
						       filt,
						       NULL);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems sending DataTransfer constants: %s",
			     e.what());
	}

	std::list<rina::QoSCube*> cubes =
			ipc_process_->resource_allocator_->getQoSCubes();

	if (cubes.size() > 0) {
		try {
			encoders::QoSCubeListEncoder encoder;
			rina::cdap_rib::obj_info_t obj;
			obj.class_ = QoSCubesRIBObject::class_name;
			obj.name_ = QoSCubesRIBObject::object_name;
			encoder.encodePointers(cubes, obj.value_);
			rina::cdap_rib::filt_info_t filt;
			rina::cdap_rib::flags_t flags;

			rib_daemon_->getProxy()->remote_create(con,
							       obj,
							       flags,
							       filt,
							       NULL);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems sending QoS cubes: %s",
				     e.what());
		}
	}
}

void EnrollerStateMachine::start(configs::EnrollmentInformationRequest& eiRequest,
				 int invoke_id,
				 const rina::cdap_rib::con_handle_t &con_handle)
{
	rina::ScopedLock g(lock_);

	INamespaceManagerPs *nsmps = dynamic_cast<INamespaceManagerPs *>(namespace_manager_->ps);
	assert(nsmps);

	if (!isValidPortId(con_handle.port_id)){
		return;
	}

	if (state_ != STATE_WAIT_START_ENROLLMENT) {
		abortEnrollment(remote_peer_.name_,
				con_handle.port_id,
				START_IN_BAD_STATE,
				true);
		return;
	}

	timer_.cancelTask(last_scheduled_task_);

	bool requiresInitialization = false;

	LOG_IPCP_DBG("Remote IPC Process address: %u",
		     eiRequest.address_);

	if (eiRequest.address_ == 0) {
		requiresInitialization = true;
	} else {
		try {
			if (!nsmps->isValidAddress(eiRequest.address_,
						   remote_peer_.name_.processName,
						   remote_peer_.name_.processInstance)) {
				requiresInitialization = true;
			}

			std::list<rina::ApplicationProcessNamingInformation>::const_iterator it;
			for (it = eiRequest.supporting_difs_.begin();
					it != eiRequest.supporting_difs_.end(); ++it) {
				remote_peer_.supporting_difs_.push_back(*it);
			}
		}catch (rina::Exception &e) {
			LOG_IPCP_ERR("%s", e.what());
			sendNegativeStartResponseAndAbortEnrollment(rina::cdap_rib::CDAP_ERROR,
								    std::string(e.what()),
								    invoke_id);
			return;
		}
	}

	if (requiresInitialization){
		unsigned int address = nsmps->getValidAddress(remote_peer_.name_.processName,
							      remote_peer_.name_.processInstance);

		if (address == 0){
			sendNegativeStartResponseAndAbortEnrollment(rina::cdap_rib::CDAP_ERROR,
								    "Could not assign a valid address",
								    invoke_id);
			return;
		}

		LOG_IPCP_DBG("Remote IPC Process requires initialization, assigning address %u", address);
		eiRequest.address_ = address;
	}

	try {
		rina::cdap_rib::obj_info_t obj;
		obj.class_ = EnrollmentRIBObject::class_name;
		obj.name_ = EnrollmentRIBObject::object_name;
		if (requiresInitialization) {
			encoders::EnrollmentInformationRequestEncoder encoder;
			encoder.encode(eiRequest, obj.value_);
		}
		rina::cdap_rib::flags_t flags;
		rina::cdap_rib::res_info_t res;
		res.code_ = rina::cdap_rib::CDAP_SUCCESS;

		rina::cdap::getProvider()->send_start_result(con,
							     obj,
							     flags,
							     res,
							     invoke_id);

		remote_peer_.address_ = eiRequest.address_;
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems sending CDAP message: %s", e.what());
		sendNegativeStartResponseAndAbortEnrollment(rina::cdap_rib::CDAP_ERROR,
							    std::string(e.what()),
							    invoke_id);
		return;
	}

	//If initialization is required send the M_CREATEs
	if (requiresInitialization){
		sendDIFStaticInformation();
	}

	sendDIFDynamicInformation();

	//Send the M_STOP request
	try {
		rina::cdap_rib::obj_info_t obj;
		obj.class_ = EnrollmentRIBObject::class_name;
		obj.name_ = EnrollmentRIBObject::object_name;
		encoders::EnrollmentInformationRequestEncoder encoder;
		encoder.encode(eiRequest, obj.value_);
		rina::cdap_rib::flags_t flags;
		rina::cdap_rib::filt_info_t filt;

		rib_daemon_->getProxy()->remote_stop(con,
						     obj,
						     flags,
						     filt,
						     this);
	} catch(rina::Exception &e) {
		LOG_IPCP_ERR("Problems sending CDAP message: %s", e.what());
		sendNegativeStartResponseAndAbortEnrollment(rina::cdap_rib::CDAP_ERROR,
							    std::string(e.what()),
							    invoke_id);
		return;
	}

	//Set timer
	last_scheduled_task_ = new EnrollmentFailedTimerTask(this,
						   	     STOP_ENROLLMENT_RESPONSE_TIMEOUT);
	timer_.scheduleTask(last_scheduled_task_, timeout_);

	LOG_IPCP_DBG("Waiting for stop enrollment response message");
	state_ = STATE_WAIT_STOP_ENROLLMENT_RESPONSE;
}

void EnrollerStateMachine::remoteStopResult(const rina::cdap_rib::con_handle_t &con_handle,
			      	      	    const rina::cdap_rib::obj_info_t &obj,
			      	      	    const rina::cdap_rib::res_info_t &res)
{
	rina::ScopedLock g(lock_);

	if (!isValidPortId(con_handle.port_id)){
		return;
	}

	if (state_ != STATE_WAIT_STOP_ENROLLMENT_RESPONSE) {
		abortEnrollment(remote_peer_.name_,
				con_handle_.port_id,
				STOP_RESPONSE_IN_BAD_STATE,
				true);
		return;
	}

	timer_.cancelTask(last_scheduled_task_);
	if (res.code_ != rina::cdap_rib::CDAP_SUCCESS){
		state_ = STATE_NULL;
		enrollment_task_->enrollmentFailed(remote_peer_.name_,
						   con_handle.port_id,
						   res.reason_,
						   true);
		return;
	}

	try{
		rina::cdap_rib::obj_info_t obj;
		obj.class_ = OperationalStatusRIBObject::class_name;
		obj.name_ = OperationalStatusRIBObject::object_name;
		rina::cdap_rib::flags_t flags;
		rina::cdap_rib::filt_info_t filt;

		rib_daemon_->getProxy()->remote_start(con,
						      obj,
						      flags,
						      filt,
						      NULL);
	} catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems sending CDAP Message: %s", e.what());
	}

	enrollmentCompleted();
}

void EnrollerStateMachine::enrollmentCompleted()
{
	state_ = STATE_ENROLLED;

	createOrUpdateNeighborInformation(true);

	enrollment_task_->enrollmentCompleted(remote_peer_, false);

	LOG_IPCP_INFO("Remote IPC Process enrolled!");
}

//Class EnrollmentRIBObject
const std::string EnrollmentRIBObject::class_name = "Enrollment";
const std::string EnrollmentRIBObject::object_name = "/difManagement/enrollment";

EnrollmentRIBObject::EnrollmentRIBObject(IPCProcess * ipc_process) :
	IPCPRIBObj(ipc_process, class_name)
{
	enrollment_task_ = (IPCPEnrollmentTask *) ipc_process->enrollment_task_;
}

void EnrollmentRIBObject::start(const rina::cdap_rib::con_handle_t &con_handle,
				const std::string& fqn,
				const std::string& class_,
				const rina::cdap_rib::filt_info_t &filt,
				const int invoke_id,
				const rina::ser_obj_t &obj_req,
				rina::ser_obj_t &obj_reply,
				rina::cdap_rib::res_info_t& res)
{
	EnrollerStateMachine * stateMachine = 0;

	try {
		stateMachine = (EnrollerStateMachine *) enrollment_task_->getEnrollmentStateMachine(con_handle.port_id,
												    false);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems retrieving state machine: %s", e.what());
		sendErrorMessage(con_handle.port_id);
		return;
	}

	if (!stateMachine) {
		LOG_IPCP_ERR("Got a CDAP message that is not for me ");
		return;
	}

	configs::EnrollmentInformationRequest eiRequest;
	eiRequest.address_ = 0;
	if (obj_req.message_) {
		encoders::EnrollmentInformationRequestEncoder encoder;
		encoder.decode(obj_req, eiRequest);
	}
	stateMachine->start(eiRequest,
			    invoke_id,
			    con_handle);

	res.code_ = rina::cdap_rib::CDAP_PENDING;
}

void EnrollmentRIBObject::stop(const rina::cdap_rib::con_handle_t &con_handle,
			       const std::string& fqn,
			       const std::string& class_,
			       const rina::cdap_rib::filt_info_t &filt,
			       const int invoke_id,
			       const rina::ser_obj_t &obj_req,
			       rina::ser_obj_t &obj_reply,
			       rina::cdap_rib::res_info_t& res)
{
	EnrolleeStateMachine * stateMachine = 0;

	try {
		stateMachine = (EnrolleeStateMachine *) enrollment_task_->getEnrollmentStateMachine(
				con_handle.port_id, false);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems retrieving state machine: %s", e.what());
		sendErrorMessage(con_handle.port_id);
		return;
	}

	if (!stateMachine) {
		LOG_IPCP_ERR("Got a CDAP message that is not for me");
		return;
	}

	configs::EnrollmentInformationRequest eiRequest;
	eiRequest.address_ = 0;
	if (obj_req.message_) {
		encoders::EnrollmentInformationRequestEncoder encoder;
		encoder.decode(obj_req, eiRequest);
	}
	stateMachine->stop(eiRequest,
			   invoke_id ,
			   con_handle);

	res.code_ = rina::cdap_rib::CDAP_PENDING;
}

void EnrollmentRIBObject::sendErrorMessage(unsigned int port_id)
{
	try{
		rib_daemon_->getProxy()->remote_close_connection(port_id);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems sending CDAP message: %s", e.what());
	}
}

class EnrollmentTaskPs: public IPCPEnrollmentTaskPS {
public:
	EnrollmentTaskPs(IPCProcess * ipcp_);
        virtual ~EnrollmentTaskPs() {};
        void connect_received(const rina::cdap::CDAPMessage& cdapMessage,
        		     const rina::cdap_rib::con_handle_t &con);
        void connect_response_received(int result,
        			       const std::string& result_reason,
        			       const rina::cdap_rib::con_handle_t &con);
        void process_authentication_message(const rina::cdap::CDAPMessage& message,
        				    const rina::cdap_rib::con_handle_t &con);
	void authentication_completed(int port_id, bool success);
        void initiate_enrollment(const rina::NMinusOneFlowAllocatedEvent & event,
        			 const rina::EnrollmentRequest& request);
        void inform_ipcm_about_failure(IEnrollmentStateMachine * state_machine);
        void set_dif_configuration(const rina::DIFConfiguration& dif_configuration);
        int set_policy_set_param(const std::string& name,
        		         const std::string& value);

private:
        void populate_rib();
        IEnrollmentStateMachine * createEnrollmentStateMachine(const rina::ApplicationProcessNamingInformation& naming_info,
        						       int portId,
        						       bool enrollee,
        						       const rina::ApplicationProcessNamingInformation& supportingDifName);

private:
        IPCProcess * ipcp;
        IPCPEnrollmentTask * et;
        int timeout;
        rina::Lockable lock;
        rina::IPCResourceManager * irm;
        IPCPRIBDaemon * rib_daemon;
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
	rina::rib::RIBObj* tmp;

	try {
		tmp = new EnrollmentRIBObject(ipcp);
		rib_daemon->addObjRIB(EnrollmentRIBObject::object_name, &tmp);

		tmp = new NeighborsRIBObj(ipcp);
		rib_daemon->addObjRIB(NeighborsRIBObj::object_name, &tmp);

		tmp = new OperationalStatusRIBObject(ipcp);
		rib_daemon->addObjRIB(OperationalStatusRIBObject::object_name, &tmp);

		tmp = new rina::rib::RIBObj("Naming");
		rib_daemon->addObjRIB("/difManagement/naming", &tmp);

		tmp = new AddressRIBObject(ipcp);
		rib_daemon->addObjRIB(AddressRIBObject::object_name, &tmp);
	} catch (rina::Exception &e) {
		LOG_ERR("Problems adding object to the RIB : %s", e.what());
	}
}

void EnrollmentTaskPs::connect_received(const rina::cdap::CDAPMessage& cdapMessage,
		     	     	        const rina::cdap_rib::con_handle_t &con_handle)
{
	rina::ScopedLock g(lock);

	try{
		rina::FlowInformation flowInformation =
			irm->getNMinus1FlowInformation(con_handle.port_id);
		rina::ApplicationProcessNamingInformation apNamingInfo;
		apNamingInfo.processName = con_handle.dest_.ap_name_;
		apNamingInfo.processInstance = con_handle.dest_.ap_inst_;
		apNamingInfo.entityName = con_handle.dest_.ae_name_;
		apNamingInfo.entityInstance = con_handle.dest_.ae_inst_;
		EnrollerStateMachine * enrollmentStateMachine =
			(EnrollerStateMachine *) createEnrollmentStateMachine(apNamingInfo,
									      con_handle.port_id,
									      false,
									      flowInformation.difName);
		enrollmentStateMachine->connect(cdapMessage, con_handle);
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems: %s", e.what());

		try {
			rina::cdap_rib::res_info_t res;
			res.code_ = rina::cdap_rib::CDAP_APP_CONNECTION_REJECTED;
			res.reason_ = std::string(e.what());

			rina::cdap::getProvider()->send_open_connection_result(con_handle,
									       res,
									       cdapMessage.invoke_id_);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems sending CDAP message: %s", e.what());
		}

		et->deallocateFlow(con_handle.port_id);
	}
}

void EnrollmentTaskPs::connect_response_received(int result,
			     	     	         const std::string& result_reason,
			     	     	         const rina::cdap_rib::con_handle_t &con_handle)
{
	rina::ScopedLock g(lock);

	try{
		EnrolleeStateMachine * stateMachine =
			(EnrolleeStateMachine*) et->getEnrollmentStateMachine(con_handle.port_id,
									      false);
		stateMachine->connectResponse(result, result_reason);
	}catch(rina::Exception &e){
		//Error getting the enrollment state machine
		LOG_IPCP_ERR("Problems getting enrollment state machine: %s",
				e.what());
		try {
			rib_daemon->getProxy()->remote_close_connection(con_handle.port_id);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems closing application connection: %s",
				     e.what());
		}

		et->deallocateFlow(con_handle.port_id);
	}
}

void EnrollmentTaskPs::process_authentication_message(const rina::cdap::CDAPMessage& message,
						      const rina::cdap_rib::con_handle_t &con_handle)
{
	rina::ScopedLock g(lock);

	try {
		IEnrollmentStateMachine * stateMachine =
			et->getEnrollmentStateMachine(con_handle.port_id,
						      false);
		stateMachine->process_authentication_message(message,
							     con_handle);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems getting enrollment state machine: %s",
			     e.what());
		try {
			rib_daemon->getProxy()->remote_close_connection(con_handle.port_id);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems closing application connection: %s",
				     e.what());
		}
		et->deallocateFlow(con_handle.port_id);
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
			rib_daemon->getProxy()->remote_close_connection(port_id);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems closing application connection: %s",
					e.what());
		}
		et->deallocateFlow(port_id);
	}
}

void EnrollmentTaskPs::initiate_enrollment(const rina::NMinusOneFlowAllocatedEvent & event,
					   const rina::EnrollmentRequest& request)
{
	rina::ScopedLock g(lock);

	EnrolleeStateMachine * enrollmentStateMachine = 0;

	//1 Tell the enrollment task to create a new Enrollment state machine
	try{
		enrollmentStateMachine =
				(EnrolleeStateMachine *) createEnrollmentStateMachine(request.neighbor_.name_,
										      event.flow_information_.portId,
										      true,
										      event.flow_information_.difName);
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problem retrieving enrollment state machine: %s", e.what());
		return;
	}

	//2 Tell the enrollment state machine to initiate the enrollment
	// (will require an M_CONNECT message and a port Id)
	try{
		enrollmentStateMachine->initiateEnrollment(request,
							   event.flow_information_.portId);
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems initiating enrollment: %s", e.what());
	}
}

void EnrollmentTaskPs::inform_ipcm_about_failure(IEnrollmentStateMachine * state_machine)
{
	EnrolleeStateMachine * sm = dynamic_cast<EnrolleeStateMachine *>(state_machine);
	if (sm) {
		if (sm->enrollment_request_.ipcm_initiated_) {
			try {
				rina::extendedIPCManager->enrollToDIFResponse(sm->enrollment_request_.event_,
									      -1,
									      std::list<rina::Neighbor>(),
									      ipcp->get_dif_information());
			} catch (rina::Exception &e) {
				LOG_IPCP_ERR("Problems sending message to IPC Manager: %s",
						e.what());
			}
		}
	}
}

IEnrollmentStateMachine * EnrollmentTaskPs::createEnrollmentStateMachine(const rina::ApplicationProcessNamingInformation& apNamingInfo,
									 int portId,
									 bool enrollee,
									 const rina::ApplicationProcessNamingInformation& supportingDifName)
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
