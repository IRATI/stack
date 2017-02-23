//
// Key Management Agent
//
// Eduard Grasa <eduard.grasa@i2cat.net>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//   1. Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//   2. Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
//

#include <cstring>
#include <iostream>
#include <sstream>
#include <time.h>

#define RINA_PREFIX "key-management-agent"
#include <librina/ipc-api.h>
#include <librina/logs.h>

#include "kma.h"

using namespace std;
using namespace rina;

// Class Key Management Agent Enrollment Task
KMAEnrollmentTask::KMAEnrollmentTask(const std::string& ckm_name) :
		ApplicationEntity(ApplicationEntity::ENROLLMENT_TASK_AE_NAME)
{
	ckm_ap_name = ckm_name;
	kma = 0;
	ma_invoke_id = 0;

	ckm_state.auth_ps_ = 0;
	ckm_state.enrolled = false;
	ckm_state.con.port_id = 0;

	ma_state.auth_ps_ = 0;
	ma_state.enrolled = false;
	ma_state.con.port_id = 0;
}

KMAEnrollmentTask::~KMAEnrollmentTask()
{
	if (ckm_state.auth_ps_ != 0) {
		delete ckm_state.auth_ps_;
		ckm_state.auth_ps_ = 0;
	}

	if (ma_state.auth_ps_ != 0) {
		delete ma_state.auth_ps_;
		ma_state.auth_ps_ = 0;
	}
}

void KMAEnrollmentTask::set_application_process(rina::ApplicationProcess * ap)
{
	InternalEventManager * ema = 0;

	if (!ap) {
		LOG_ERR("Bogus app passed");
		return;
	}

	ema = static_cast<InternalEventManager *>(ap->get_internal_event_manager());
	if (!ema) {
		LOG_ERR("App does not contain internal event manager");
		return;
	}
	ema->subscribeToEvent(InternalEvent::APP_N_MINUS_1_FLOW_ALLOCATED, this);
	ema->subscribeToEvent(InternalEvent::APP_N_MINUS_1_FLOW_DEALLOCATED, this);

	kma = static_cast<KeyManagementAgent *>(ap);
}

void KMAEnrollmentTask::eventHappened(rina::InternalEvent * event)
{
	rina::NMinusOneFlowAllocatedEvent * fa_event = 0;
	rina::NMinusOneFlowDeallocatedEvent * fd_event = 0;

	if (event->type == InternalEvent::APP_N_MINUS_1_FLOW_ALLOCATED) {
		fa_event = static_cast<rina::NMinusOneFlowAllocatedEvent *>(event);
		initiateEnrollmentWithCKM(*fa_event);
	} else if (event->type == InternalEvent::APP_N_MINUS_1_FLOW_DEALLOCATED) {
		fd_event = static_cast<rina::NMinusOneFlowDeallocatedEvent *>(event);
		rina::cdap::getProvider()->get_session_manager()->removeCDAPSession(fd_event->port_id_);
	}
}

void KMAEnrollmentTask::initiateEnrollmentWithCKM(const rina::NMinusOneFlowAllocatedEvent& event)
{
	if (event.flow_information_.remoteAppName.processName != ckm_ap_name)
		return;

	LOG_INFO("Starting enrollment with %s over port-id %d",
		  event.flow_information_.remoteAppName.getEncodedString().c_str(),
		  event.flow_information_.portId);

	ScopedLock g(lock);

	ckm_state.con.dest_.ap_name_ = event.flow_information_.remoteAppName.processName;
	ckm_state.con.dest_.ap_inst_ = event.flow_information_.remoteAppName.processInstance;
	ckm_state.con.dest_.ae_name_ = event.flow_information_.remoteAppName.entityName;
	ckm_state.con.dest_.ae_inst_ = event.flow_information_.remoteAppName.entityInstance;
	ckm_state.con.src_.ap_name_ = event.flow_information_.localAppName.processName;
	ckm_state.con.src_.ap_inst_ = event.flow_information_.localAppName.processInstance;
	ckm_state.con.src_.ae_name_ = event.flow_information_.localAppName.entityName;
	ckm_state.con.src_.ae_inst_ = event.flow_information_.localAppName.entityInstance;
	ckm_state.con.port_id = event.flow_information_.portId;
	ckm_state.con.version_.version_ = 0x01;

	ckm_state.auth_ps_ = kma->secman->get_auth_policy_set(kma->secman->sec_profile.authPolicy.name_);
	if (!ckm_state.auth_ps_) {
		LOG_ERR("Could not %s authentication policy set, aborting",
			kma->secman->sec_profile.authPolicy.name_.c_str());
		kma->irm->deallocate_flow(event.flow_information_.portId);
		return;
	}

	try {
		rina::cdap_rib::auth_policy_t auth = ckm_state.auth_ps_->get_auth_policy(event.flow_information_.portId,
				  	  	  	  	  	  	  	 ckm_state.con.dest_,
											 kma->secman->sec_profile);
		kma->ribd->getProxy()->remote_open_connection(ckm_state.con.version_,
							      ckm_state.con.src_,
							      ckm_state.con.dest_,
							      auth,
							      ckm_state.con.port_id);
	} catch (rina::Exception &e) {
		LOG_ERR("Problems opening CDAP connection to %s: %s",
				event.flow_information_.remoteAppName.getEncodedString().c_str(),
				e.what());
	}
}

void KMAEnrollmentTask::connect(const rina::cdap::CDAPMessage& message,
				rina::cdap_rib::con_handle_t &con)
{
	rina::ScopedLock g(lock);
	if (ma_state.enrolled || ma_state.auth_ps_){
		LOG_ERR("I'm already enrolled or enrolling to the Management Agent");
		return;
	}

	ma_state.auth_ps_ = kma->secman->get_auth_policy_set(message.auth_policy_.name);
	if (!ma_state.auth_ps_) {
		LOG_ERR("Could not find authentication policy %s",
			message.auth_policy_.name.c_str());
		kma->irm->deallocate_flow(con.port_id);
		return;
	}

	LOG_INFO("Authenticating Management Agent %s-%s ...",
		  con.dest_.ap_name_.c_str(),
		  con.dest_.ap_inst_.c_str());

	rina::IAuthPolicySet::AuthStatus auth_status =
			ma_state.auth_ps_->initiate_authentication(message.auth_policy_,
							  	   kma->secman->sec_profile,
								   con.dest_,
								   con.port_id);
	if (auth_status == rina::IAuthPolicySet::FAILED) {
		LOG_ERR("Authentication failed");
		kma->irm->deallocate_flow(con.port_id);
		delete ma_state.auth_ps_;
		ma_state.auth_ps_ = 0;
		return;
	} else if (auth_status == rina::IAuthPolicySet::IN_PROGRESS) {
		LOG_DBG("Authentication in progress");
	}

	ma_state.con = con;
	ma_invoke_id = message.invoke_id_;
}

void KMAEnrollmentTask::connectResult(const rina::cdap::CDAPMessage& message,
		   	   	      rina::cdap_rib::con_handle_t &con)
{
}

void KMAEnrollmentTask::release(int invoke_id,
		     	     	const rina::cdap_rib::con_handle_t &con)
{
}

void KMAEnrollmentTask::releaseResult(const rina::cdap_rib::res_info_t &res,
		   	   	      const rina::cdap_rib::con_handle_t &con)
{
}

void KMAEnrollmentTask::process_authentication_message(const rina::cdap::CDAPMessage& message,
			            	    	       const rina::cdap_rib::con_handle_t &con)
{
	int result = 0;
	cdap_rib::res_info_t res;

	ScopedLock g(lock);

	if (con.port_id != ckm_state.con.port_id &&
			con.port_id != ma_state.con.port_id) {
		LOG_ERR("Received authentication message through wrong port-id, aborting");
		kma->irm->deallocate_flow(con.port_id);
		return;
	}

	if (ckm_state.enrolled && ma_state.enrolled) {
		LOG_ERR("Received an authentication message and I'm already enrolled");
		kma->irm->deallocate_flow(con.port_id);
		return;
	}

	result = ckm_state.auth_ps_->process_incoming_message(message,
							      con.port_id);

	if (result == rina::IAuthPolicySet::IN_PROGRESS) {
		LOG_INFO("Authentication still in progress");
		return;
	}

	if (result == rina::IAuthPolicySet::FAILED) {
		LOG_ERR("Authentication failed");
		kma->irm->deallocate_flow(con.port_id);
		return;
	}

	if (con.port_id == ckm_state.con.port_id) {
		LOG_INFO("Authentication was successful, waiting for M_CONNECT_R");
		return;
	}

	LOG_INFO("Authentication was successful, sending M_CONNECT_R");

	//Send M_CONNECT_R
	try{
		res.code_ = rina::cdap_rib::CDAP_SUCCESS;
		rina::cdap::getProvider()->send_open_connection_result(ma_state.con,
								       res,
								       ma_state.con.auth_,
								       ma_invoke_id);
	}catch(rina::Exception &e){
		LOG_ERR("Problems sending CDAP message: %s", e.what());
		kma->irm->deallocate_flow(con.port_id);
		return;
	}
}

// Class Key Management Agent
KeyManagementAgent::KeyManagementAgent(const std::string& creds_folder,
		   const std::list<std::string>& dif_names,
		   const std::string& apn,
		   const std::string& api,
		   const std::string& ckm_apn,
		   const std::string& ckm_api,
		   bool  q) : AbstractKM(apn, api)
{
	creds_location = creds_folder;
	dif_name = dif_names.front();
	ckm_name = ckm_apn;
	ckm_instance = ckm_api;
	quiet = q;

	creds_location = creds_folder;
	etask = new KMAEnrollmentTask(ckm_apn);
	ribd = new KMRIBDaemon(etask);
	secman = new KMSecurityManager(creds_folder);
	eventm = new SimpleInternalEventManager();
	irm = new KMIPCResourceManager();
	kcm = new KeyContainerManager();

	add_entity(eventm);
	add_entity(ribd);
	add_entity(irm);
	add_entity(secman);
	add_entity(etask);
	add_entity(kcm);

	populate_rib();

	//Register so that the local Management Agent can connect to me
	irm->applicationRegister(dif_names, apn, api);

	//Allocate a flow to the Central Key Manager
	irm->allocate_flow(ApplicationProcessNamingInformation(apn, api),
			   ApplicationProcessNamingInformation(ckm_name, ckm_instance));
}

KeyManagementAgent::~KeyManagementAgent()
{
	if (etask) {
		delete etask;
		etask = 0;
	}
}

void KeyManagementAgent::populate_rib()
{
	rib::RIBObj * ribo;

	try{
		ribo = new KeyContainersRIBObject();
		ribd->addObjRIB(KeyContainersRIBObject::object_name, &ribo);
	} catch (rina::Exception & e){
		LOG_ERR("Problems adding object to RIB");
	}
}
