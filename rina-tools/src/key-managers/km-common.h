//
// Common utilities for Key Manager and Key Management Agent
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

#ifndef KM_COMMON_HPP
#define KM_COMMON_HPP

#include <string>
#include <librina/concurrency.h>
#include <librina/irm.h>
#include <librina/rib_v2.h>
#include <librina/security-manager.h>

class AppEventLoop
{
public:
	AppEventLoop();
	virtual ~AppEventLoop() {};

	void stop(void);
	void event_loop(void);
	virtual void register_application_response_handler(const rina::RegisterApplicationResponseEvent& event) = 0;
	virtual void unregister_application_response_handler(const rina::UnregisterApplicationResponseEvent& event) = 0;
	virtual void allocate_flow_request_result_handler(const rina::AllocateFlowRequestResultEvent& event) = 0;
	virtual void flow_allocation_requested_handler(const rina::FlowRequestEvent& event) = 0;
	virtual void deallocate_flow_response_handler(const rina::DeallocateFlowResponseEvent& event) = 0;
	virtual void flow_deallocated_event_handler(const rina::FlowDeallocatedEvent& event) = 0;

protected:
	bool keep_running;
	rina::Lockable lock;
};

class DummySecurityManagerPs : public rina::ISecurityManagerPs
{
public:
	DummySecurityManagerPs() {};
	~DummySecurityManagerPs() {};

	int isAllowedToJoinDAF(const rina::cdap_rib::con_handle_t & con,
			       const rina::Neighbor& newMember,
			       rina::cdap_rib::auth_policy_t & auth);
	int storeAccessControlCreds(const rina::cdap_rib::auth_policy_t & auth,
				    const rina::cdap_rib::con_handle_t & con);
	int getAccessControlCreds(rina::cdap_rib::auth_policy_t & auth,
			          const rina::cdap_rib::con_handle_t & con);
	void checkRIBOperation(const rina::cdap_rib::auth_policy_t & auth,
			       const rina::cdap_rib::con_handle_t & con,
			       const rina::cdap::cdap_m_t::Opcode opcode,
			       const std::string obj_name,
			       rina::cdap_rib::res_info_t& res);
	int set_policy_set_param(const std::string& name,
				 const std::string& value);
};

class KMRIBDaemon : public rina::rib::RIBDaemonAE
{
public:
	KMRIBDaemon(rina::cacep::AppConHandlerInterface *app_con_callback);
	~KMRIBDaemon(){};

	rina::rib::RIBDaemonProxy * getProxy();
        const rina::rib::rib_handle_t & get_rib_handle();
        int64_t addObjRIB(const std::string& fqn, rina::rib::RIBObj** obj);
        void removeObjRIB(const std::string& fqn);

private:
	//Handle to the RIB
	rina::rib::rib_handle_t rib;
	rina::rib::RIBDaemonProxy* ribd;
};

class KMSecurityManager: public rina::ISecurityManager
{
public:
	KMSecurityManager(const std::string& creds_location);
	~KMSecurityManager();

	void set_application_process(rina::ApplicationProcess * ap);
        rina::IAuthPolicySet::AuthStatus update_crypto_state(const rina::CryptoState& state,
        						     rina::IAuthPolicySet * caller);
private:
        rina::AuthSDUProtectionProfile sec_profile;
        rina::AuthSSH2PolicySet * auth_ps;
};

class KMIPCResourceManager: public rina::IPCResourceManager
{
public:
	KMIPCResourceManager() {};
	~KMIPCResourceManager() {};

	void set_application_process(rina::ApplicationProcess * ap);
	void register_application_response(const rina::RegisterApplicationResponseEvent& event);
	void unregister_application_response(const rina::UnregisterApplicationResponseEvent& event);
	void applicationRegister(const std::list<std::string>& dif_names,
				 const std::string& app_name,
				 const std::string& app_instance);
	void allocate_flow(const rina::ApplicationProcessNamingInformation & local_app_name,
			  const rina::ApplicationProcessNamingInformation & remote_app_name);
	void deallocate_flow(int port_id);
};

class KMEventLoop : public AppEventLoop
{
public:
	KMEventLoop() { kirm = 0; };
	~KMEventLoop() {};

	void set_km_irm(KMIPCResourceManager * irm);
	void register_application_response_handler(const rina::RegisterApplicationResponseEvent& event);
	void unregister_application_response_handler(const rina::UnregisterApplicationResponseEvent& event);
	void allocate_flow_request_result_handler(const rina::AllocateFlowRequestResultEvent& event);
	void flow_allocation_requested_handler(const rina::FlowRequestEvent& event);
	void deallocate_flow_response_handler(const rina::DeallocateFlowResponseEvent& event);
	void flow_deallocated_event_handler(const rina::FlowDeallocatedEvent& event);

private:
	KMIPCResourceManager * kirm;
};

#endif//KM_COMMON_HPP
