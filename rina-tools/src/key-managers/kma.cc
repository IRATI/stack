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
KMAEnrollmentTask::KMAEnrollmentTask() : ApplicationEntity(ApplicationEntity::ENROLLMENT_TASK_AE_NAME)
{
	kma = 0;
}

void KMAEnrollmentTask::set_application_process(rina::ApplicationProcess * ap)
{
	kma = static_cast<KeyManagementAgent *>(ap);
}

void KMAEnrollmentTask::connect(const rina::cdap::CDAPMessage& message,
				const rina::cdap_rib::con_handle_t &con)
{
}

void KMAEnrollmentTask::connectResult(const rina::cdap_rib::res_info_t &res,
		   	   	      const rina::cdap_rib::con_handle_t &con,
				      const rina::cdap_rib::auth_policy_t& auth)
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
}

// Class Key Management Agent
KeyManagementAgent::KeyManagementAgent(const std::string& creds_folder,
		   const std::list<std::string>& dif_names,
		   const std::string& apn,
		   const std::string& api,
		   const std::string& ckm_apn,
		   const std::string& ckm_api,
		   bool  q) : ApplicationProcess(apn, api)
{
	creds_location = creds_folder;
	dif_name = dif_names.front();
	ckm_name = ckm_apn;
	ckm_instance = ckm_api;
	quiet = q;

	creds_location = creds_folder;
	etask = new KMAEnrollmentTask();
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

	irm->allocate_flow(ApplicationProcessNamingInformation(apn, api),
			   ApplicationProcessNamingInformation(ckm_name, ckm_instance));
}

KeyManagementAgent::~KeyManagementAgent()
{
	delete ribd;
	delete etask;
	delete secman;
	delete eventm;
	delete irm;
}

unsigned int KeyManagementAgent::get_address() const
{
	return 0;
}

void KeyManagementAgent::populate_rib()
{
	rib::RIBObj * ribo;

	try{
		ribo = new rib::RIBObj("keyContainers");
		ribd->addObjRIB("/keyContainers", &ribo);
	} catch (rina::Exception & e){
		LOG_ERR("Problems adding object to RIB");
	}
}
