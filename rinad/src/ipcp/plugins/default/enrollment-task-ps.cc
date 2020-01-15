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
	static const std::string START_RESPONSE_IN_BAD_STATE;
	static const std::string STOP_ENROLLMENT_TIMEOUT;
	static const std::string STOP_IN_BAD_STATE;
	static const std::string STOP_WITH_NO_OBJECT_VALUE;
	static const std::string READ_RESPONSE_TIMEOUT;
	static const std::string PROBLEMS_COMMITTING_ENROLLMENT_INFO;
	static const std::string START_TIMEOUT;
	static const std::string INTERNAL_FLOW_ALLOCATION_TIMEOUT;
	static const std::string READ_RESPONSE_IN_BAD_STATE;
	static const std::string UNSUCCESSFULL_READ_RESPONSE;
	static const std::string UNSUCCESSFULL_START;
	static const std::string CONNECT_IN_NOT_NULL;
	static const std::string ENROLLMENT_NOT_ALLOWED;
	static const std::string START_ENROLLMENT_TIMEOUT;
	static const std::string STOP_ENROLLMENT_RESPONSE_TIMEOUT;
	static const std::string STOP_RESPONSE_IN_BAD_STATE;
	static const std::string AUTHENTICATION_TIMEOUT;
	static const std::string INTERNAL_FLOW_ALLOCATE_IN_BAD_STATE;

	static const std::string STATE_AUTHENTICATING;
	static const std::string STATE_WAIT_CONNECT_RESPONSE;
	static const std::string STATE_WAIT_START_ENROLLMENT_RESPONSE;
	static const std::string STATE_WAIT_READ_RESPONSE;
	static const std::string STATE_WAIT_START;
	static const std::string STATE_WAIT_INTERNAL_FLOW_ALLOCATION;
	static const std::string STATE_WAIT_START_ENROLLMENT;
	static const std::string STATE_WAIT_STOP_ENROLLMENT_RESPONSE;
	static const std::string STATE_WAIT_START_RESPONSE;

	virtual ~BaseEnrollmentStateMachine() {};
	virtual void operational_status_start(int invoke_id,
					      const rina::ser_obj_t &obj_req);
protected:
	BaseEnrollmentStateMachine(IPCProcess * ipc_process,
				   const rina::ApplicationProcessNamingInformation& remote_naming_info,
				   int timeout,
				   const rina::ApplicationProcessNamingInformation& supporting_dif_name,
				   rina::Timer * timer);

	/// Sends all the DIF dynamic information
	void sendDIFDynamicInformation();

	/// Send the entries in the DFT (if any)
	void sendDFTEntries();

	IPCProcess * ipc_process_;
	IPCPSecurityManager * sec_man_;
	std::string token;
};

//Class BaseEnrollmentStateMachine
const std::string BaseEnrollmentStateMachine::CONNECT_RESPONSE_TIMEOUT =
		"Timeout waiting for connect response";
const std::string BaseEnrollmentStateMachine::START_RESPONSE_TIMEOUT =
		"Timeout waiting for start response";
const std::string BaseEnrollmentStateMachine::START_IN_BAD_STATE =
		"Received a START message in a wrong state";
const std::string BaseEnrollmentStateMachine::START_RESPONSE_IN_BAD_STATE =
		"Received a START_RESPONSE message in a wrong state";
const std::string BaseEnrollmentStateMachine::INTERNAL_FLOW_ALLOCATE_IN_BAD_STATE =
		"Internal flow allocated while in a wrong state";
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
const std::string BaseEnrollmentStateMachine::INTERNAL_FLOW_ALLOCATION_TIMEOUT =
		"Timeout waiting for internal flow allocation";
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
const std::string BaseEnrollmentStateMachine::STATE_WAIT_INTERNAL_FLOW_ALLOCATION =
		"WAIT_INTERNAL_FLOW_ALLOCATION";
const std::string BaseEnrollmentStateMachine::STATE_WAIT_START_RESPONSE =
		"WAIT_START_RESPONSE";

BaseEnrollmentStateMachine::BaseEnrollmentStateMachine(IPCProcess * ipc_process,
						       const rina::ApplicationProcessNamingInformation& remote_naming_info,
						       int timeout,
						       const rina::ApplicationProcessNamingInformation& supporting_dif_name,
						       rina::Timer * timer) :
				IEnrollmentStateMachine(ipc_process, remote_naming_info,
						timeout, supporting_dif_name, timer)
{
	ipc_process_ = ipc_process;
	sec_man_ = ipc_process->security_manager_;
}

void BaseEnrollmentStateMachine::operational_status_start(int invoke_id,
			      	      	      	          const rina::ser_obj_t &obj_req)
{
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
			    int timeout,
			    rina::Timer * timer);
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
	void connectResponse(int result,
			     const std::string& result_reason,
			     const rina::cdap_rib::con_handle_t & con,
			     const rina::cdap_rib::auth_policy_t& auth);

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

	void operational_status_start(int invoke_id,
				      const rina::ser_obj_t &obj_req);

	void internal_flow_allocate_result(int portId,
					   int fd,
					   const std::string& result_reason);

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

	void send_start_result_and_complete();

	bool was_dif_member_before_enrollment_;
	bool allowed_to_start_early_;
	int stop_request_invoke_id_;
	int start_request_invoke_id;
};

// Class EnrolleeStateMachine
EnrolleeStateMachine::EnrolleeStateMachine(IPCProcess * ipc_process,
					   const rina::ApplicationProcessNamingInformation& remote_naming_info,
					   int timeout, rina::Timer * timer):
		BaseEnrollmentStateMachine(ipc_process,
					   remote_naming_info,
					   timeout,
					   rina::ApplicationProcessNamingInformation(), timer)
{
	was_dif_member_before_enrollment_ = false;
	last_scheduled_task_ = 0;
	allowed_to_start_early_ = false;
	stop_request_invoke_id_ = 0;
	start_request_invoke_id = 0;
}

void EnrolleeStateMachine::initiateEnrollment(const rina::EnrollmentRequest& enrollmentRequest,
					      int portId)
{
	rina::ScopedLock g(lock_);

	enr_request = enrollmentRequest;
	remote_peer_.address_ = enr_request.neighbor_.address_;
	remote_peer_.name_ = enr_request.neighbor_.name_;
	remote_peer_.supporting_dif_name_ = enr_request.neighbor_.supporting_dif_name_;
	remote_peer_.underlying_port_id_ = portId;
	remote_peer_.supporting_difs_ = enr_request.neighbor_.supporting_difs_;

	if (state_ != STATE_NULL) {
		throw rina::Exception("Enrollee state machine not in NULL state");
	}

	try{
		rina::AuthSDUProtectionProfile profile =
				sec_man_->get_auth_sdup_profile(remote_peer_.supporting_dif_name_.processName);
		auth_ps_ = ipc_process_->security_manager_->get_auth_policy_set(profile.authPolicy.name_);
		if (!auth_ps_) {
			abortEnrollment(std::string("Unsupported authentication policy set"),
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
		last_scheduled_task_ = new AbortEnrollmentTimerTask(enrollment_task_,
				       	       	       	       	    remote_peer_.name_,
								    con.port_id,
								    remote_peer_.internal_port_id,
								    CONNECT_RESPONSE_TIMEOUT,
								    true);
		timer->scheduleTask(last_scheduled_task_, timeout_);

		//Update state
		state_ = STATE_WAIT_CONNECT_RESPONSE;
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems sending M_CONNECT message: %s", e.what());
		abortEnrollment(std::string(e.what()), true);
	}
}

void EnrolleeStateMachine::process_authentication_message(const rina::cdap::CDAPMessage& message,
			   	    	    	    	  const rina::cdap_rib::con_handle_t &con_handle)
{
	rina::ScopedLock g(lock_);

	if (!auth_ps_ || state_ != STATE_WAIT_CONNECT_RESPONSE) {
		abortEnrollment("Authentication policy is null or wrong state",
				true);
		return;
	}

	int result = auth_ps_->process_incoming_message(message,
							con_handle.port_id);

	if (result == rina::IAuthPolicySet::FAILED) {
		abortEnrollment("Authentication failed", true);
	} else if (result == rina::IAuthPolicySet::SUCCESSFULL) {
		LOG_IPCP_INFO("Authentication was successful, waiting for M_CONNECT_R");
	}
}

void EnrolleeStateMachine::authentication_completed(bool success)
{
	rina::ScopedLock g(lock_);

	if (!success) {
		abortEnrollment("Authentication failed", true);
	} else {
		LOG_IPCP_INFO("Authentication was successful, waiting for M_CONNECT_R");
	}
}

void EnrolleeStateMachine::connectResponse(int result,
					   const std::string& result_reason,
					   const rina::cdap_rib::con_handle_t & con,
					   const rina::cdap_rib::auth_policy_t& auth)
{
	rina::ScopedLock g(lock_);

	if (state_ != STATE_WAIT_CONNECT_RESPONSE) {
		abortEnrollment("Message received in wrong order", true);
		return;
	}

	timer->cancelTask(last_scheduled_task_);
	if (result != 0) {
		abortEnrollment(result_reason, true);
		return;
	}

	if (remote_peer_.name_.processName != con.dest_.ap_name_) {
		//Application connection was to the DAF in general, now update
		//it with the specific DAP name
		remote_peer_.name_.processName = con.dest_.ap_name_;
		remote_peer_.name_.processInstance = con.dest_.ap_inst_;

		INamespaceManagerPs *nsmps = dynamic_cast<INamespaceManagerPs *>(ipc_process_->namespace_manager_->ps);
		assert(nsmps);
		remote_peer_.address_ = nsmps->getValidAddress(remote_peer_.name_.processName,
							       remote_peer_.name_.processInstance);
	}

	// If "auth" contents are not valid, it will return an error and
	// enrollment should be aborted. Otherwise it should store the "token" or any other
	// type of credential in "auth" - if any - and return 0.
	rina::ISecurityManagerPs *smps = dynamic_cast<rina::ISecurityManagerPs *>(sec_man_->ps);
	assert(smps);

	if (smps->storeAccessControlCreds(auth, con) != 0){
		abortEnrollment("Failed to validate and store access control credentials", true);
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
			difInformation.dif_name_ = enr_request.event_.dafName;
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
		last_scheduled_task_ = new AbortEnrollmentTimerTask(enrollment_task_,
   	       	       	       	    	    	    	    	    remote_peer_.name_,
								    con.port_id,
								    remote_peer_.internal_port_id,
								    START_RESPONSE_TIMEOUT,
								    true);
		timer->scheduleTask(last_scheduled_task_, timeout_);

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
		abortEnrollment(START_IN_BAD_STATE, true);
		return;
	}

	timer->cancelTask(last_scheduled_task_);
	if (res.code_ != rina::cdap_rib::CDAP_SUCCESS) {
		abortEnrollment(res.reason_, true);
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
	last_scheduled_task_ = new AbortEnrollmentTimerTask(enrollment_task_,
    	    	    	    	    	    	    	    remote_peer_.name_,
							    con.port_id,
							    remote_peer_.internal_port_id,
							    STOP_ENROLLMENT_TIMEOUT,
							    true);
	timer->scheduleTask(last_scheduled_task_, timeout_);

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
		abortEnrollment(STOP_IN_BAD_STATE, true);
		return;
	}

	timer->cancelTask(last_scheduled_task_);

	allowed_to_start_early_ = eiRequest.allowed_to_start_early_;
	stop_request_invoke_id_ = invoke_id;
	token = eiRequest.token;

	LOG_IPCP_DBG("Allowed to start early: %d \n Token: %s",
		     allowed_to_start_early_,
		     token.c_str());

	//Request more information or start
	try{
		requestMoreInformationOrStart();
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems requesting more information or starting: %s",
			      e.what());
		abortEnrollment(std::string(e.what()), true);
	}
}

void EnrolleeStateMachine::requestMoreInformationOrStart()
{
	if (sendNextObjectRequired()){
		//Set timer
		last_scheduled_task_ = new AbortEnrollmentTimerTask(enrollment_task_,
	    	    	    	    	    	    	    	    remote_peer_.name_,
								    con.port_id,
								    remote_peer_.internal_port_id,
								    READ_RESPONSE_TIMEOUT,
								    true);
		timer->scheduleTask(last_scheduled_task_, timeout_);

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

			abortEnrollment(PROBLEMS_COMMITTING_ENROLLMENT_INFO, true);
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

	last_scheduled_task_ = new AbortEnrollmentTimerTask(enrollment_task_,
    	    	    	    	    	    	    	    remote_peer_.name_,
							    con.port_id,
							    remote_peer_.internal_port_id,
							    START_TIMEOUT,
							    true);
	timer->scheduleTask(last_scheduled_task_, timeout_);
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
	createOrUpdateNeighborInformation(true);;

	//Send DirectoryForwardingTableEntries
	sendDFTEntries();

	enrollment_task_->enrollmentCompleted(remote_peer_, true,
					      enr_request.event_.prepare_for_handover,
					      enr_request.event_.disc_neigh_name);

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
	std::list<rina::Neighbor> neighbors;
	neighbors.push_back(remote_peer_);
	if (!enr_request.ipcm_initiated_){
		enr_request.event_.sequenceNumber = 0;
	}

	try {
		rina::extendedIPCManager->enrollToDIFResponse(enr_request.event_,
				0,
				neighbors,
				ipc_process_->get_dif_information());
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems sending message to IPC Manager: %s", e.what());
	}
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
		abortEnrollment(READ_RESPONSE_IN_BAD_STATE, true);
		return;
	}

	timer->cancelTask(last_scheduled_task_);

	if (res.code_ != rina::cdap_rib::CDAP_SUCCESS ||
			obj.value_.message_ == 0){
		abortEnrollment(res.reason_, true);
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
		for (it = neighbors.begin(); it != neighbors.end(); ++it) {
			ipc_process_->enrollment_task_->add_neighbor(*it);

			//Update remote peer address, it may have changed
			if (it->name_.processName == remote_peer_.name_.processName) {
				remote_peer_.address_ = it->address_;
			}
		}
	}else{
		LOG_IPCP_WARN("The object to be created is not required for enrollment");
	}

	//Request more information or proceed with the enrollment program
	requestMoreInformationOrStart();
}

void EnrolleeStateMachine::operational_status_start(int invoke_id,
					      	    const rina::ser_obj_t &obj_req)
{
	rina::FlowRequestEvent event;
	rina::Neighbor aux;

	rina::ScopedLock g(lock_);

	if (state_ == STATE_ENROLLED) {
		return;
	}

	if (state_ != STATE_WAIT_START) {
		abortEnrollment(START_IN_BAD_STATE, true);
		return;
	}

	timer->cancelTask(last_scheduled_task_);
	start_request_invoke_id = invoke_id;

	//Update remote peer address based on the information it provided us
	aux.name_.processName = remote_peer_.name_.processName;
	aux.name_.processInstance = remote_peer_.name_.processInstance;
	ipcp_->enrollment_task_->get_neighbor_info(aux);
	remote_peer_.address_ = aux.address_;
	remote_peer_.old_address_ = aux.old_address_;

	if (enrollment_task_->use_reliable_n_flow) {
		//Add temp entry to the PDU forwarding table, to be able to forward PDUs to neighbor
		ipcp_->resource_allocator_->add_temp_pduft_entry(remote_peer_.address_,
				remote_peer_.underlying_port_id_);

		// Allocate internal reliable flow to layer management tasks of remote peer
		try {
			event.DIFName.processName = enr_request.event_.dafName.processName;
			event.localApplicationName.processName = ipcp_->get_name();
			event.localApplicationName.processInstance = ipcp_->get_instance();
			event.localApplicationName.entityName = IPCProcess::MANAGEMENT_AE;
			event.remoteApplicationName.processName = remote_peer_.name_.processName;
			event.remoteApplicationName.processInstance = remote_peer_.name_.processInstance;
			event.remoteApplicationName.entityName = IPCProcess::MANAGEMENT_AE;
			event.flowSpecification.maxAllowableGap = 0;
			event.flowSpecification.orderedDelivery = true;
			event.flowRequestorIpcProcessId = 0;
			event.ipcProcessId = ipcp_->get_id();
			event.internal = true;
			ipc_process_->flow_allocator_->submitAllocateRequest(event,
					remote_peer_.address_);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems allocating internal flow: %s", e.what());
			abortEnrollment("Problems allocating internal flow", true);
			return;
		}

		last_scheduled_task_ = new AbortEnrollmentTimerTask(enrollment_task_,
				remote_peer_.name_,
				con.port_id,
				remote_peer_.internal_port_id,
				INTERNAL_FLOW_ALLOCATION_TIMEOUT,
				true);
		timer->scheduleTask(last_scheduled_task_, timeout_);
		state_ = STATE_WAIT_INTERNAL_FLOW_ALLOCATION;
	} else {
		send_start_result_and_complete();
	}
}

void EnrolleeStateMachine::send_start_result_and_complete()
{
	// Send M_START_R message (transition to internal flow)
	try {
		rina::cdap_rib::flags_t flags;
		rina::cdap_rib::filt_info_t filt;
		rina::cdap_rib::res_info_t res;
		rina::cdap_rib::obj_info_t obj;
		encoders::NeighborEncoder encoder;
		rina::Neighbor neighbor;
		obj.class_ = OperationalStatusRIBObject::class_name;
		obj.name_ = OperationalStatusRIBObject::object_name;
		neighbor.name_.processName = ipcp_->get_name();
		neighbor.name_.processInstance = ipcp_->get_instance();
		neighbor.name_.entityName = IPCProcess::MANAGEMENT_AE;
		neighbor.name_.entityInstance = token;
		encoder.encode(neighbor, obj.value_);
		res.code_ = rina::cdap_rib::CDAP_SUCCESS;

		rina::cdap::getProvider()->send_start_result(con,
				obj,
				flags,
				res,
				start_request_invoke_id);
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems sending CDAP message: %s", e.what());
	}

	// Complete enrollment
	try{
		commitEnrollment();
		enrollmentCompleted();
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems commiting enrollment: %s", e.what());
		abortEnrollment(PROBLEMS_COMMITTING_ENROLLMENT_INFO, true);
	}
}

void EnrolleeStateMachine::internal_flow_allocate_result(int portId,
							 int fd,
				   	   	   	 const std::string& result_reason)
{
	rina::FlowRequestEvent event;

	rina::ScopedLock g(lock_);

	if (state_ == STATE_ENROLLED) {
		return;
	}

	if (state_ != STATE_WAIT_INTERNAL_FLOW_ALLOCATION) {
		abortEnrollment(INTERNAL_FLOW_ALLOCATE_IN_BAD_STATE, true);
		return;
	}

	timer->cancelTask(last_scheduled_task_);

	if (portId < 0){
		abortEnrollment(result_reason, true);
		return;
	}

	LOG_IPCP_DBG("Internal flow with port-id %d allocated", portId);

	remote_peer_.internal_port_id = portId;

	send_start_result_and_complete();
}

/// The state machine of the party that is a member of the DIF
/// and will help the joining party (enrollee) to join the DIF
class EnrollerStateMachine: public BaseEnrollmentStateMachine {
public:
	EnrollerStateMachine(IPCProcess * ipc_process,
			     const rina::ApplicationProcessNamingInformation& remote_naming_info,
			     int timeout,
			     const rina::ApplicationProcessNamingInformation& supporting_dif_name,
			     rina::Timer * timer);
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

	// The response of the start operation has been received, check that the
	// token is the same as the one generated and consider the process enrolled
	void remoteStartResult(const rina::cdap_rib::con_handle_t &con,
			       const rina::cdap_rib::obj_info_t &obj,
			       const rina::cdap_rib::res_info_t &res);

	void internal_flow_allocate_result(int portId,
					   int fd,
					   const std::string& result_reason);

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

	INamespaceManager * namespace_manager_;
	int connect_message_invoke_id_;
};

//Class EnrollerStateMachine
EnrollerStateMachine::EnrollerStateMachine(IPCProcess * ipc_process,
					   const rina::ApplicationProcessNamingInformation& remote_naming_info,
					   int timeout,
					   const rina::ApplicationProcessNamingInformation& supporting_dif_name,
					   rina::Timer * timer):
		BaseEnrollmentStateMachine(ipc_process,
					   remote_naming_info,
					   timeout,
					   supporting_dif_name,
					   timer)
{
	namespace_manager_ = ipc_process->namespace_manager_;
	enroller_ = true;
	connect_message_invoke_id_ = 0;
}

void EnrollerStateMachine::connect(const rina::cdap::CDAPMessage& message,
				   const rina::cdap_rib::con_handle_t &con_handle)
{
	rina::ScopedLock g(lock_);

	if (state_ != STATE_NULL) {
		abortEnrollment(CONNECT_IN_NOT_NULL, true);
		return;
	}

	LOG_IPCP_DBG("Authenticating IPC process %s-%s ...",
		      con_handle.dest_.ap_name_.c_str(),
		      con_handle.dest_.ap_inst_.c_str());
	remote_peer_.name_.processName = con_handle.dest_.ap_name_;
	remote_peer_.name_.processInstance = con_handle.dest_.ap_inst_;
	connect_message_invoke_id_ = message.invoke_id_;
	con = con_handle;
	remote_peer_.underlying_port_id_ = con.port_id;

	auth_ps_ = sec_man_->get_auth_policy_set(message.auth_policy_.name);
	if (!auth_ps_) {
		abortEnrollment(std::string("Could not find auth policy set"), true);
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
		abortEnrollment(std::string("Authentication failed"), true);
		return;
	} else if (auth_status == rina::IAuthPolicySet::IN_PROGRESS) {
		state_ = STATE_AUTHENTICATING;
		last_scheduled_task_ = new AbortEnrollmentTimerTask(enrollment_task_,
	    	    	    	    	    	    	    	    remote_peer_.name_,
								    con.port_id,
								    remote_peer_.internal_port_id,
								    AUTHENTICATION_TIMEOUT,
								    true);
		LOG_IPCP_DBG("Authentication in progress");
		timer->scheduleTask(last_scheduled_task_, timeout_);
		return;
	}

	authentication_successful();
}

void EnrollerStateMachine::process_authentication_message(const rina::cdap::CDAPMessage& message,
					    	          const rina::cdap_rib::con_handle_t &con_handle)
{
	rina::ScopedLock g(lock_);

	if (!auth_ps_ || state_ != STATE_AUTHENTICATING) {
		abortEnrollment("Authentication policy is null or wrong state", true);
		return;
	}

	int result = auth_ps_->process_incoming_message(message,
							con_handle.port_id);

	if (result == rina::IAuthPolicySet::IN_PROGRESS) {
		LOG_IPCP_DBG("Authentication still in progress");
		return;
	}

	timer->cancelTask(last_scheduled_task_);

	if (result == rina::IAuthPolicySet::FAILED) {
		abortEnrollment("Authentication failed", true);
		return;
	}

	authentication_successful();
}

void EnrollerStateMachine::authentication_completed(bool success)
{
	rina::ScopedLock g(lock_);

	if (last_scheduled_task_) {
		timer->cancelTask(last_scheduled_task_);
		last_scheduled_task_ = 0;
	}

	if (!success) {
		abortEnrollment("Authentication failed", true);
	} else {
		authentication_successful();
	}
}

void EnrollerStateMachine::authentication_successful()
{
	rina::ISecurityManagerPs *smps = dynamic_cast<rina::ISecurityManagerPs *>(sec_man_->ps);
	assert(smps);
	rina::cdap_rib::auth_policy_t auth;

	LOG_IPCP_DBG("Authentication successful, deciding if new member can join the DIF...");
	rina::Time currentTime;
        int t0 = currentTime.get_current_time_in_ms();
        LOG_IPCP_DBG("START ENROLLMENT %d ms", t0);
        if (smps->isAllowedToJoinDAF(con,
				     remote_peer_,
				     auth) != 0) {
		LOG_IPCP_WARN("Security Manager rejected enrollment attempt, aborting enrollment");
		abortEnrollment(ENROLLMENT_NOT_ALLOWED, true);
		return;
	}
        
	//Send M_CONNECT_R
	try{
		rina::cdap_rib::res_info_t res;
		res.code_ = rina::cdap_rib::CDAP_SUCCESS;
		rina::cdap::getProvider()->send_open_connection_result(con,
								       res,
								       auth,
								       connect_message_invoke_id_);

		//Set timer
		last_scheduled_task_ = new AbortEnrollmentTimerTask(enrollment_task_,
	    	    	    	    	    	    	    	    remote_peer_.name_,
								    con.port_id,
								    remote_peer_.internal_port_id,
								    START_ENROLLMENT_TIMEOUT,
								    true);
		timer->scheduleTask(last_scheduled_task_, timeout_);
		LOG_IPCP_DBG("M_CONNECT_R sent to portID %d. Waiting for start enrollment request message",
			     con.port_id);

		state_ = STATE_WAIT_START_ENROLLMENT;
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems sending CDAP message: %s", e.what());
		abortEnrollment(std::string(e.what()), true);
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

		abortEnrollment(resultReason, true);
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
	std::stringstream ss;

	rina::ScopedLock g(lock_);

	INamespaceManagerPs *nsmps = dynamic_cast<INamespaceManagerPs *>(namespace_manager_->ps);
	assert(nsmps);

	if (!isValidPortId(con_handle.port_id)){
		return;
	}

	if (state_ != STATE_WAIT_START_ENROLLMENT) {
		abortEnrollment(START_IN_BAD_STATE, true);
		return;
	}

	timer->cancelTask(last_scheduled_task_);

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

	int temp = std::rand();
	ss << temp;
	token = ss.str();

	//Send the M_STOP request
	try {
		rina::cdap_rib::obj_info_t obj;
		obj.class_ = EnrollmentRIBObject::class_name;
		obj.name_ = EnrollmentRIBObject::object_name;
		encoders::EnrollmentInformationRequestEncoder encoder;
		eiRequest.allowed_to_start_early_ = false;
		eiRequest.token = token;
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
	last_scheduled_task_ = new AbortEnrollmentTimerTask(enrollment_task_,
    	    	    	    	    	    	    	    remote_peer_.name_,
							    con.port_id,
							    remote_peer_.internal_port_id,
							    STOP_ENROLLMENT_RESPONSE_TIMEOUT,
							    true);
	timer->scheduleTask(last_scheduled_task_, timeout_);

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
		abortEnrollment(STOP_RESPONSE_IN_BAD_STATE, true);
		return;
	}

	timer->cancelTask(last_scheduled_task_);
	if (res.code_ != rina::cdap_rib::CDAP_SUCCESS){
		abortEnrollment(res.reason_, true);
		return;
	}

	if (enrollment_task_->use_reliable_n_flow) {
		//Add temp entry to the PDU forwarding table, to be able to forward PDUs to neighbor
		ipcp_->resource_allocator_->add_temp_pduft_entry(remote_peer_.address_,
				remote_peer_.underlying_port_id_);
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
				this);
	} catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems sending CDAP Message: %s", e.what());
		abortEnrollment("Problems sending CDAP message", true);
		return;
	}

	if (enrollment_task_->use_reliable_n_flow) {
		//Set timer
		last_scheduled_task_ = new AbortEnrollmentTimerTask(enrollment_task_,
				remote_peer_.name_,
				con.port_id,
				remote_peer_.internal_port_id,
				INTERNAL_FLOW_ALLOCATION_TIMEOUT,
				true);
		timer->scheduleTask(last_scheduled_task_, timeout_);

		LOG_IPCP_DBG("Waiting for internal flow allocation");
		state_ = STATE_WAIT_INTERNAL_FLOW_ALLOCATION;
	} else {
		//Set timer
		last_scheduled_task_ = new AbortEnrollmentTimerTask(enrollment_task_,
				remote_peer_.name_,
				con.port_id,
				remote_peer_.internal_port_id,
				START_RESPONSE_TIMEOUT,
				true);
		timer->scheduleTask(last_scheduled_task_, timeout_);

		LOG_IPCP_DBG("Waiting for start response message");
		state_ = STATE_WAIT_START_RESPONSE;
	}
}

void EnrollerStateMachine::internal_flow_allocate_result(int portId,
				   	   	   	 int fd,
							 const std::string& result_reason)
{
	rina::FlowRequestEvent event;

	rina::ScopedLock g(lock_);

	if (state_ != STATE_WAIT_INTERNAL_FLOW_ALLOCATION) {
		abortEnrollment(INTERNAL_FLOW_ALLOCATE_IN_BAD_STATE, true);
		return;
	}

	timer->cancelTask(last_scheduled_task_);

	if (portId < 0){
		abortEnrollment(result_reason, true);
		return;
	}

	LOG_IPCP_DBG("Internal flow with port-id %d allocated", portId);

	remote_peer_.internal_port_id = portId;

	//Set timer
	last_scheduled_task_ = new AbortEnrollmentTimerTask(enrollment_task_,
    	    	    	    	    	    	    	    remote_peer_.name_,
							    con.port_id,
							    remote_peer_.internal_port_id,
							    START_RESPONSE_TIMEOUT,
							    true);
	timer->scheduleTask(last_scheduled_task_, timeout_);

	LOG_IPCP_DBG("Waiting for start response message");
	state_ = STATE_WAIT_START_RESPONSE;
}

void EnrollerStateMachine::remoteStartResult(const rina::cdap_rib::con_handle_t &con_handle,
			      	      	     const rina::cdap_rib::obj_info_t &obj,
			      	      	     const rina::cdap_rib::res_info_t &res)
{
	rina::FlowRequestEvent event;

	rina::ScopedLock g(lock_);

	if (state_ != STATE_WAIT_START_RESPONSE) {
		abortEnrollment(START_RESPONSE_IN_BAD_STATE, true);
		return;
	}

	timer->cancelTask(last_scheduled_task_);

	if (res.code_ != rina::cdap_rib::CDAP_SUCCESS) {
		abortEnrollment(res.reason_, true);
		return;
	}

	encoders::NeighborEncoder encoder;
	rina::Neighbor neighbor;
	encoder.decode(obj.value_, neighbor);
	if (neighbor.name_.processName != remote_peer_.name_.processName ||
			neighbor.name_.processInstance != remote_peer_.name_.processInstance ||
			neighbor.name_.entityName != IPCProcess::MANAGEMENT_AE ||
			neighbor.name_.entityInstance != token) {
		LOG_IPCP_ERR("Peer is not the expected one, aborting enrollment. Peer info: %s",
			     neighbor.toString().c_str());
		abortEnrollment("Unexpected partner", true);
		return;
	}

	enrollmentCompleted();
}

void EnrollerStateMachine::enrollmentCompleted()
{
	state_ = STATE_ENROLLED;

	createOrUpdateNeighborInformation(true);

	enrollment_task_->enrollmentCompleted(remote_peer_, false, false,
					      rina::ApplicationProcessNamingInformation());
}

//Class EnrollmentRIBObject
const std::string EnrollmentRIBObject::class_name = "Enrollment";
const std::string EnrollmentRIBObject::object_name = "/difm/enr";

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
	EnrollerStateMachine * stateMachine;
	stateMachine = (EnrollerStateMachine *) enrollment_task_->getEnrollmentStateMachine(con_handle.port_id,
											    false);

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
	EnrolleeStateMachine * stateMachine;
	stateMachine = (EnrolleeStateMachine *) enrollment_task_->getEnrollmentStateMachine(con_handle.port_id,
											    false);

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
        			       const rina::cdap_rib::con_handle_t &con,
				       const rina::cdap_rib::auth_policy_t& auth);
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
        rina::Timer timer;
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
	rina::cdap_rib::vers_info_t vers;
	vers.version_ = 0x1ULL;

	try {
		tmp = new EnrollmentRIBObject(ipcp);
		rib_daemon->addObjRIB(EnrollmentRIBObject::object_name, &tmp);

		tmp = new NeighborsRIBObj(ipcp);
		rib_daemon->addObjRIB(NeighborsRIBObj::object_name, &tmp);
		rib_daemon->getProxy()->addCreateCallbackSchema(vers,
								NeighborRIBObj::class_name,
								NeighborsRIBObj::object_name,
								NeighborRIBObj::create_cb);

		tmp = new OperationalStatusRIBObject(ipcp);
		rib_daemon->addObjRIB(OperationalStatusRIBObject::object_name, &tmp);

		tmp = new rina::rib::RIBObj("Naming");
		rib_daemon->addObjRIB("/difm/nam", &tmp);

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
			     	     	         const rina::cdap_rib::con_handle_t &con_handle,
						 const rina::cdap_rib::auth_policy_t &auth)
{
	rina::ScopedLock g(lock);

	EnrolleeStateMachine * stateMachine =
			(EnrolleeStateMachine*) et->getEnrollmentStateMachine(con_handle.port_id,
									      false);
	if (!stateMachine) {
		//Error getting the enrollment state machine
		try {
			rib_daemon->getProxy()->remote_close_connection(con_handle.port_id);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems closing application connection: %s",
				     e.what());
		}

		et->deallocateFlow(con_handle.port_id);
		return;
	}

	stateMachine->connectResponse(result,
				      result_reason,
				      con_handle,
				      auth);
}

void EnrollmentTaskPs::process_authentication_message(const rina::cdap::CDAPMessage& message,
						      const rina::cdap_rib::con_handle_t &con_handle)
{
	rina::ScopedLock g(lock);

	IEnrollmentStateMachine * stateMachine =
		et->getEnrollmentStateMachine(con_handle.port_id,
					      false);
	if (!stateMachine) {
		try {
			rib_daemon->getProxy()->remote_close_connection(con_handle.port_id);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems closing application connection: %s",
				     e.what());
		}
		et->deallocateFlow(con_handle.port_id);
		return;
	}

	stateMachine->process_authentication_message(message,
						     con_handle);
}

void EnrollmentTaskPs::authentication_completed(int port_id, bool success)
{
	rina::ScopedLock g(lock);

	IEnrollmentStateMachine * stateMachine =
					et->getEnrollmentStateMachine(port_id, false);

	if (!stateMachine) {
		try {
			rib_daemon->getProxy()->remote_close_connection(port_id);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems closing application connection: %s",
					e.what());
		}
		et->deallocateFlow(port_id);
		return;
	}

	stateMachine->authentication_completed(success);
}

void EnrollmentTaskPs::initiate_enrollment(const rina::NMinusOneFlowAllocatedEvent & event,
					   const rina::EnrollmentRequest& request)
{
	rina::Time currentTime;
        int t = currentTime.get_current_time_in_ms();
        LOG_IPCP_INFO("initiate_enrollment at %d", t);

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
		if (sm->enr_request.ipcm_initiated_) {
			try {
				rina::extendedIPCManager->enrollToDIFResponse(sm->enr_request.event_,
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

	if (apNamingInfo.entityName == "" ||
			apNamingInfo.entityName == IPCProcess::MANAGEMENT_AE) {
		if (enrollee){
			stateMachine = new EnrolleeStateMachine(ipcp,
								apNamingInfo,
								timeout, &timer);
		}else{
			stateMachine = new EnrollerStateMachine(ipcp,
								apNamingInfo,
								timeout,
								supportingDifName, &timer);
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
