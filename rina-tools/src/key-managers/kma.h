//
// Local Key Management Agent
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

#ifndef KMA_HPP
#define KMA_HPP

#include <string>

#include <librina/application.h>
#include <librina/concurrency.h>
#include <librina/timer.h>

#include "km-common.h"

struct CKMEnrollmentState
{
	bool enrolled;
	rina::IAuthPolicySet * auth_ps_;
	rina::cdap_rib::con_handle_t con;
};

class KeyManagementAgent;

class KMAEnrollmentTask: public rina::cacep::AppConHandlerInterface,
			 public rina::ApplicationEntity,
			 public rina::InternalEventListener
{
public:
	KMAEnrollmentTask(const std::string& ckm_name);
	~KMAEnrollmentTask();

	void set_application_process(rina::ApplicationProcess * ap);
	void eventHappened(rina::InternalEvent * event);
	void connect(const rina::cdap::CDAPMessage& message,
		     rina::cdap_rib::con_handle_t &con);
	void connectResult(const rina::cdap::CDAPMessage& message,
			   rina::cdap_rib::con_handle_t &con);
	void release(int invoke_id,
		     const rina::cdap_rib::con_handle_t &con);
	void releaseResult(const rina::cdap_rib::res_info_t &res,
			   const rina::cdap_rib::con_handle_t &con);
	void process_authentication_message(const rina::cdap::CDAPMessage& message,
				            const rina::cdap_rib::con_handle_t &con);

private:
	void initiateEnrollmentWithCKM(const rina::NMinusOneFlowAllocatedEvent& event);

	rina::Lockable lock;
	std::string ckm_ap_name;
	KeyManagementAgent * kma;
	CKMEnrollmentState ckm_state;
	CKMEnrollmentState ma_state;
	int ma_invoke_id;
};

class KeyManagementAgent: public AbstractKM {
public:
	KeyManagementAgent(const std::string& creds_folder,
			   const std::list<std::string>& dif_name,
			   const std::string& apn,
			   const std::string& api,
			   const std::string& ckm_apn,
			   const std::string& ckm_api,
			   bool  quiet);
	~KeyManagementAgent();

        KMAEnrollmentTask * etask;

private:
        void populate_rib();

        std::string creds_location;
        std::string dif_name;
        std::string ckm_name;
        std::string ckm_instance;
        bool quiet;
        rina::Timer timer;
};
#endif//KMA_HPP
