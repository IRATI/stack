//
// Central Key Manager, just for demo purposes (does not interact with
// Manager yet)
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

#include <iostream>
#include <time.h>
#include <signal.h>
#include <time.h>

#define RINA_PREFIX     "central-key-manager"
#include <librina/logs.h>
#include <librina/ipc-api.h>

#include "ckm.h"

using namespace std;
using namespace rina;

// Class Central Key Manager Enrollment Task
CKMEnrollmentTask::CKMEnrollmentTask() : ApplicationEntity(ApplicationEntity::ENROLLMENT_TASK_AE_NAME)
{
	ckm = 0;
}

void CKMEnrollmentTask::set_application_process(rina::ApplicationProcess * ap)
{
	ckm = static_cast<CentralKeyManager *>(ap);
}

void CKMEnrollmentTask::connect(const rina::cdap::CDAPMessage& message,
				rina::cdap_rib::con_handle_t &con)
{
	std::map<std::string, KMAData *>::iterator it;
	KMAData * data = 0;

	//1 Find out if the sender is really connecting to us
	if(con.src_.ap_name_.compare(ckm->get_name())!= 0){
		LOG_ERR("an M_CONNECT message whose destination was not this IPC Process, ignoring it");
		ckm->irm->deallocate_flow(con.port_id);
		return;
	}

	rina::ScopedLock g(lock);

	//2 Check we are not enrolled yet
	it = enrolled_kmas.find(con.dest_.ap_name_);
	if (it != enrolled_kmas.end()) {
		LOG_ERR("I am already enrolled to Key Management Agent %s",
			con.src_.ap_name_.c_str());
		ckm->irm->deallocate_flow(con.port_id);
		return;
	}

	//3 Send connectResult
	data = new KMAData();
	data->con = con;
	data->invoke_id = message.invoke_id_;

	data->auth_ps_ = ckm->secman->get_auth_policy_set(message.auth_policy_.name);
	if (!data->auth_ps_) {
		LOG_ERR("Could not find authentication policy %s",
			message.auth_policy_.name.c_str());
		ckm->irm->deallocate_flow(con.port_id);
		delete data;
		return;
	}

	LOG_INFO("Authenticating Key Management Agent %s-%s ...",
		  con.dest_.ap_name_.c_str(),
		  con.dest_.ap_inst_.c_str());

	rina::IAuthPolicySet::AuthStatus auth_status =
			data->auth_ps_->initiate_authentication(message.auth_policy_,
							  	ckm->secman->sec_profile,
								con.dest_,
								con.port_id);
	if (auth_status == rina::IAuthPolicySet::FAILED) {
		LOG_ERR("Authentication failed");
		ckm->irm->deallocate_flow(con.port_id);
		delete data;
		return;
	} else if (auth_status == rina::IAuthPolicySet::IN_PROGRESS) {
		LOG_DBG("Authentication in progress");
	}

	enrolled_kmas[con.dest_.ap_name_] = data;
}

void CKMEnrollmentTask::connectResult(const rina::cdap::CDAPMessage& message,
		   	   	      rina::cdap_rib::con_handle_t &con)
{
}

void CKMEnrollmentTask::release(int invoke_id,
		     	     	const rina::cdap_rib::con_handle_t &con)
{
}

void CKMEnrollmentTask::releaseResult(const rina::cdap_rib::res_info_t &res,
		   	   	      const rina::cdap_rib::con_handle_t &con)
{
}

void CKMEnrollmentTask::process_authentication_message(const rina::cdap::CDAPMessage& message,
			            	    	       const rina::cdap_rib::con_handle_t &con)
{
	int result = 0;
	KMAData * kma_data = 0;
	cdap_rib::res_info_t res;

	ScopedLock g(lock);

	kma_data = get_kma_data(con.port_id);

	if (!kma_data) {
		LOG_ERR("Could not find KMA data associated to port-id %d",
			con.port_id);
		ckm->irm->deallocate_flow(con.port_id);
		return;
	}

	result = kma_data->auth_ps_->process_incoming_message(message,
							      con.port_id);

	if (result == rina::IAuthPolicySet::IN_PROGRESS) {
		LOG_INFO("Authentication still in progress");
		return;
	}

	if (result == rina::IAuthPolicySet::FAILED) {
		LOG_ERR("Authentication failed");
		ckm->irm->deallocate_flow(con.port_id);
		return;
	}

	LOG_INFO("Authentication of KMA %s successful",
		 kma_data->con.dest_.ap_name_.c_str());


	//Send M_CONNECT_R
	try{
		res.code_ = rina::cdap_rib::CDAP_SUCCESS;
		rina::cdap::getProvider()->send_open_connection_result(kma_data->con,
								       res,
								       kma_data->con.auth_,
								       kma_data->invoke_id);
	}catch(rina::Exception &e){
		LOG_ERR("Problems sending CDAP message: %s", e.what());
		ckm->irm->deallocate_flow(con.port_id);
		return;
	}

	//Initialise credentials for KMA
	ckm->initialise_creds_for_kma(kma_data->con.dest_.ap_name_,
				      kma_data->con);
}

KMAData * CKMEnrollmentTask::get_kma_data(unsigned int port_id)
{
	std::map<std::string, KMAData *>::iterator it;

	for (it = enrolled_kmas.begin(); it != enrolled_kmas.end(); ++it) {
		if (it->second->con.port_id == port_id)
			return it->second;
	}

	return 0;
}

// Class Central Key Manager
CentralKeyManager::CentralKeyManager(const std::list<std::string>& dif_names,
		  	  	     const std::string& app_name,
				     const std::string& app_instance,
				     const std::string& creds_folder)
						: AbstractKM(app_name, app_instance)
{
	creds_location = creds_folder;
	etask = new CKMEnrollmentTask();
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

	irm->applicationRegister(dif_names, app_name, app_instance);
}

CentralKeyManager::~CentralKeyManager()
{
	if (etask) {
		delete etask;
		etask = 0;
	}
}

std::string CentralKeyManager::get_name()
{
	return name_;
}

std::string CentralKeyManager::get_instance()
{
	return instance_;
}

void CentralKeyManager::populate_rib()
{
	rib::RIBObj * ribo;

	try{
		ribo = new KeyContainersRIBObject();
		ribd->addObjRIB(KeyContainersRIBObject::object_name, &ribo);
	} catch (rina::Exception & e){
		LOG_ERR("Problems adding object to RIB");
	}
}

///
/// Hardcoded to only generate credentials for 3 values of KMA name
/// It will generate a credential for 1 IPCP for each system
///
void CentralKeyManager::initialise_creds_for_kma(const std::string& kma_name,
						 const cdap_rib::con_handle_t& con)
{
	struct key_container * kc = new key_container();
	rib::RIBObj * kc_ribo = 0;
	std::stringstream ss;

	if (kma_name == "pristine.local.key.ma")
		kc->key_id = "A.kmdemo";
	else if (kma_name == "pristine.local.key.ma2")
		kc->key_id = "B.kmdemo";
	else if (kma_name == "pristine.local.key.ma3" )
		kc->key_id = "C.kmdemo";
	else {
		LOG_ERR("Unrecognised KMA, not creating credentials for its IPCPs");
		return;
	}

	kcm->generate_rsa_key_pair(kc);
	kc->state = "Active";
	kcm->add_key_container(kc);

	//Add new object to the RIB
	try {
		kc_ribo = new KeyContainerRIBObject(kcm,
						    kc->key_id);
		ss << KeyContainerRIBObject::object_name_prefix << kc->key_id;
		ribd->addObjRIB(ss.str(), &kc_ribo);
	} catch (rina::Exception &e) {
		LOG_ERR("Problems adding object %s to RIB: %s",
				ss.str().c_str(), e.what());
	}

	//Create object on KMA
	try {
		cdap_rib::flags_t flags;
		cdap_rib::filt_info_t filt;
		cdap_rib::obj_info_t obj_info;
		cdap::StringEncoder encoder;

		obj_info.class_ = KeyContainerRIBObject::class_name;
		obj_info.name_ = ss.str();
		obj_info.inst_ = 0;
		KeyContainerManager::encode_key_container_message(*kc,
								  obj_info.value_);

		ribd->getProxy()->remote_create(con,
				    	        obj_info,
						flags,
						filt,
						NULL);
	} catch (Exception &e) {
		LOG_ERR("Problems encoding and sending CDAP message to KMA %s: %s",
			kma_name.c_str(),
			e.what());
	}

	LOG_INFO("Generated Key Container with ID %s for Key Management Agent %s",
		  kc->key_id.c_str(), kma_name.c_str());
}
