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

CKMWorker::CKMWorker(rina::ThreadAttributes * threadAttributes,
		     const std::string& creds_folder,
		     Server * serv) : ServerWorker(threadAttributes, serv)
{
	creds_location = creds_folder;
	last_task = 0;
}

int CKMWorker::internal_run()
{
        LOG_INFO("Doing nothing yet!");
        return 0;
}

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
				const rina::cdap_rib::con_handle_t &con)
{
}

void CKMEnrollmentTask::connectResult(const rina::cdap_rib::res_info_t &res,
		   	   	      const rina::cdap_rib::con_handle_t &con,
				      const rina::cdap_rib::auth_policy_t& auth)
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
}

// Class Central Key Manager RIB Daemon
CKMRIBDaemon::CKMRIBDaemon(rina::cacep::AppConHandlerInterface *app_con_callback)
{
	rina::cdap_rib::cdap_params params;
	rina::cdap_rib::vers_info_t vers;
	//rina::rib::RIBObj* robj;

	//Initialize the RIB library and cdap
	params.is_IPCP_ = false;
	rina::rib::init(app_con_callback, params);
	ribd = rina::rib::RIBDaemonProxyFactory();

	//Create schema
	vers.version_ = 0x1ULL;
	ribd->createSchema(vers);

	//TODO create callbacks

	//Create RIB
	rib = ribd->createRIB(vers);
	ribd->associateRIBtoAE(rib, "");

	//TODO populate RIB with some objects

	LOG_INFO("RIB Daemon Initialized");
}

rina::rib::RIBDaemonProxy * CKMRIBDaemon::getProxy()
{
	if (ribd)
		return ribd;

	return NULL;
}

const rina::rib::rib_handle_t & CKMRIBDaemon::get_rib_handle()
{
	return rib;
}

int64_t CKMRIBDaemon::addObjRIB(const std::string& fqn,
				rina::rib::RIBObj** obj)
{
	return ribd->addObjRIB(rib, fqn, obj);
}

void CKMRIBDaemon::removeObjRIB(const std::string& fqn)
{
	ribd->removeObjRIB(rib, fqn);
}

// Class Central Key Manager Security Manager
CKMSecurityManager::CKMSecurityManager(const std::string& creds_location)
{
	ckm = 0;
	auth_ps = 0;

	PolicyParameter parameter;
	sec_profile.authPolicy.name_ = IAuthPolicySet::AUTH_SSH2;
	sec_profile.authPolicy.version_ = "1";
	parameter.name_ = "keyExchangeAlg";
	parameter.value_ = "EDH";
	sec_profile.authPolicy.parameters_.push_back(parameter);
	parameter.name_ = "keystore";
	parameter.value_ = creds_location;
	sec_profile.authPolicy.parameters_.push_back(parameter);
	parameter.name_ = "keystorePass";
	parameter.value_ = "test";
	sec_profile.authPolicy.parameters_.push_back(parameter);
	sec_profile.encryptPolicy.name_ = "default";
	sec_profile.encryptPolicy.version_ = "1";
	parameter.name_ = "encryptAlg";
	parameter.value_ = "AES128";
	sec_profile.encryptPolicy.parameters_.push_back(parameter);
	parameter.name_ = "macAlg";
	parameter.value_ = "SHA256";
	sec_profile.encryptPolicy.parameters_.push_back(parameter);
	parameter.name_ = "compressAlg";
	parameter.value_ = "deflate";
	sec_profile.encryptPolicy.parameters_.push_back(parameter);

	LOG_INFO("CKM Security Manager initialised");
}

CKMSecurityManager::~CKMSecurityManager()
{
	if (ps)
		delete ps;

	if (auth_ps)
		delete auth_ps;
}

void CKMSecurityManager::set_application_process(rina::ApplicationProcess * ap)
{
	if (!ap)
		return;

	app = ap;
	ckm = dynamic_cast<CentralKeyManager*>(ap);
	if (!ckm) {
		LOG_ERR("Bogus instance of Central Key Manager passed, return");
		return;
	}

	ckm->ribd->set_security_manager(this);

	auth_ps = new AuthSSH2PolicySet(ckm->ribd->getProxy(), this);
	ps = new DummySecurityManagerPs();
}

rina::IAuthPolicySet::AuthStatus CKMSecurityManager::update_crypto_state(const rina::CryptoState& state,
						     	     	     	 rina::IAuthPolicySet * caller)
{
	return IAuthPolicySet::SUCCESSFULL;
}

// Class Central Key Manager
CentralKeyManager::CentralKeyManager(const std::list<std::string>& dif_names,
		  	  	     const std::string& app_name,
				     const std::string& app_instance,
				     const std::string& creds_folder)
						: Server(dif_names, app_name, app_instance)
						, ApplicationProcess(app_name, app_instance)
{
	creds_location = creds_folder;
	etask = new CKMEnrollmentTask();
	ribd = new CKMRIBDaemon(etask);
	secman = new CKMSecurityManager(creds_folder);

	add_entity(ribd);
	add_entity(etask);
	add_entity(secman);
}

CentralKeyManager::~CentralKeyManager()
{
	delete ribd;
	delete etask;
}

unsigned int CentralKeyManager::get_address() const
{
	return 0;
}

ServerWorker * CentralKeyManager::internal_start_worker(rina::FlowInformation flow)
{
	ThreadAttributes threadAttributes;
	CKMWorker * worker = new CKMWorker(&threadAttributes,
        		    	    	   creds_location,
        		    	    	   this);
        worker->start();
        worker->detach();
        return worker;
}
