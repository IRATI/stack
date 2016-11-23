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

// Class Central Key Manager
CentralKeyManager::CentralKeyManager(const std::list<std::string>& dif_names,
		  	  	     const std::string& app_name,
				     const std::string& app_instance,
				     const std::string& creds_folder)
						: ApplicationProcess(app_name, app_instance)
{
	creds_location = creds_folder;
	etask = new CKMEnrollmentTask();
	ribd = new KMRIBDaemon(etask);
	secman = new KMSecurityManager(creds_folder);
	eventm = new SimpleInternalEventManager();
	irm = new KMIPCResourceManager();

	add_entity(eventm);
	add_entity(ribd);
	add_entity(irm);
	add_entity(secman);
	add_entity(etask);

	populate_rib();

	set_km_irm(irm);
	irm->applicationRegister(dif_names, app_name, app_instance);
}

CentralKeyManager::~CentralKeyManager()
{
	delete ribd;
	delete etask;
	delete secman;
	delete eventm;
	delete irm;
}

unsigned int CentralKeyManager::get_address() const
{
	return 0;
}

void CentralKeyManager::populate_rib()
{
	rib::RIBObj * ribo;

	try{
		ribo = new rib::RIBObj("keyContainers");
		ribd->addObjRIB("/keyContainers", &ribo);
	} catch (rina::Exception & e){
		LOG_ERR("Problems adding object to RIB");
	}
}
