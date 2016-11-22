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

#ifndef CKM_H_
#define CKM_H_

#include <librina/application.h>
#include <librina/rib_v2.h>
#include <librina/security-manager.h>
#include "server.h"
#include "km-common.h"

class CKMWorker : public ServerWorker {
public:
	CKMWorker(rina::ThreadAttributes * threadAttributes,
		  const std::string& creds_folder,
	          Server * serv);
	~CKMWorker() throw() { };
	int internal_run();

private:
        std::string creds_location;
        rina::Timer timer;
        CancelFlowTimerTask * last_task;
};

struct KMAData
{
	rina::ApplicationProcessNamingInformation name;
	std::string system_id;
	std::string supporting_dif;
	rina::cdap_rib::con_handle_t con;
};

class CentralKeyManager;

class CKMEnrollmentTask: public rina::cacep::AppConHandlerInterface, public rina::ApplicationEntity
{
public:
	CKMEnrollmentTask();
	~CKMEnrollmentTask(){};

	void set_application_process(rina::ApplicationProcess * ap);
	void connect(const rina::cdap::CDAPMessage& message,
		     const rina::cdap_rib::con_handle_t &con);
	void connectResult(const rina::cdap_rib::res_info_t &res,
			   const rina::cdap_rib::con_handle_t &con,
			   const rina::cdap_rib::auth_policy_t& auth);
	void release(int invoke_id,
		     const rina::cdap_rib::con_handle_t &con);
	void releaseResult(const rina::cdap_rib::res_info_t &res,
			   const rina::cdap_rib::con_handle_t &con);
	void process_authentication_message(const rina::cdap::CDAPMessage& message,
				            const rina::cdap_rib::con_handle_t &con);

private:
	rina::Lockable lock;
	std::map<std::string, KMAData> enrolled_kmas;
	CentralKeyManager * ckm;
};

class CKMRIBDaemon : public rina::rib::RIBDaemonAE
{
public:
	CKMRIBDaemon(rina::cacep::AppConHandlerInterface *app_con_callback);
	~CKMRIBDaemon(){};

	rina::rib::RIBDaemonProxy * getProxy();
        const rina::rib::rib_handle_t & get_rib_handle();
        int64_t addObjRIB(const std::string& fqn, rina::rib::RIBObj** obj);
        void removeObjRIB(const std::string& fqn);

private:
	//Handle to the RIB
	rina::rib::rib_handle_t rib;
	rina::rib::RIBDaemonProxy* ribd;
};

class CKMSecurityManager: public rina::ISecurityManager
{
public:
	CKMSecurityManager(const std::string& creds_location);
	~CKMSecurityManager();

	void set_application_process(rina::ApplicationProcess * ap);
        rina::IAuthPolicySet::AuthStatus update_crypto_state(const rina::CryptoState& state,
        						     rina::IAuthPolicySet * caller);
private:
        rina::AuthSDUProtectionProfile sec_profile;
        rina::AuthSSH2PolicySet * auth_ps;
        CentralKeyManager * ckm;
};

class CentralKeyManager: public Server, public rina::ApplicationProcess
{
public:
	CentralKeyManager(const std::list<std::string>& dif_names,
			  const std::string& app_name,
			  const std::string& app_instance,
			  const std::string& creds_folder);
        ~CentralKeyManager();
        unsigned int get_address() const;

        CKMEnrollmentTask * etask;
        CKMRIBDaemon * ribd;
        CKMSecurityManager * secman;

protected:
        ServerWorker * internal_start_worker(rina::FlowInformation flow);

private:
        std::string creds_location;
};

#endif /* CKM_H_ */
