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
#include <librina/cdap_v2.h>
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

class KMSDUProtectionHandler : public rina::cdap::SDUProtectionHandler
{
public:
	KMSDUProtectionHandler();
	~KMSDUProtectionHandler() {};

	void set_security_manager(rina::ISecurityManager * sec_man);
	void protect_sdu(rina::ser_obj_t& sdu, int port_id);
	void unprotect_sdu(rina::ser_obj_t& sdu, int port_id);

private:
	rina::ISecurityManager * secman;
};

class KMSecurityManager: public rina::ISecurityManager
{
public:
	KMSecurityManager(const std::string& creds_location);
	~KMSecurityManager();

	void set_application_process(rina::ApplicationProcess * ap);
        rina::IAuthPolicySet::AuthStatus update_crypto_state(const rina::CryptoState& state,
        						     rina::IAuthPolicySet * caller);

        rina::AuthSDUProtectionProfile sec_profile;
private:
        rina::AuthSSH2PolicySet * auth_ps;
        KMSDUProtectionHandler * sdup;
};

class SDUReader : public rina::SimpleThread
{
public:
	SDUReader(int port_id, int fd_);
	~SDUReader() throw() {};
	int run();

private:
	int portid;
	int fd;
};

class KMIPCResourceManager: public rina::IPCResourceManager,
			    public rina::InternalEventListener
{
public:
	KMIPCResourceManager() {};
	~KMIPCResourceManager();

	void set_application_process(rina::ApplicationProcess * ap);
	void eventHappened(rina::InternalEvent * event);
	void register_application_response(const rina::RegisterApplicationResponseEvent& event);
	void unregister_application_response(const rina::UnregisterApplicationResponseEvent& event);
	void applicationRegister(const std::list<std::string>& dif_names,
				 const std::string& app_name,
				 const std::string& app_instance);
	void allocate_flow(const rina::ApplicationProcessNamingInformation & local_app_name,
			  const rina::ApplicationProcessNamingInformation & remote_app_name);
	void deallocate_flow(int port_id);

private:
	void start_flow_reader(int port_id, int fd);
	void stop_flow_reader(int port_id);

	rina::Lockable lock;
	std::map<int, SDUReader *> sdu_readers;
};

struct key_container
{
	rina::ser_obj_t private_key;
	rina::ser_obj_t public_key;
	std::string key_id;
	std::string state;
};

class KeyContainerManager : public rina::ApplicationEntity
{
public:
	KeyContainerManager();
	~KeyContainerManager();

	void set_application_process(rina::ApplicationProcess * ap);
	int generate_rsa_key_pair(struct key_container * kc);
	void add_key_container(struct key_container * kc);
	struct key_container* find_key_container(const std::string& kc);
	struct key_container* remove_key_container(const std::string& kc);
	static void encode_key_container_message(const struct key_container& kc,
				          	 rina::ser_obj_t& result);
	static void decode_key_container_message(const rina::ser_obj_t &message,
						 struct key_container& kc);

private:
	rina::Lockable lock;
	std::map<std::string, struct key_container *> key_containers;
};

class AbstractKM : public AppEventLoop, public rina::ApplicationProcess
{
public:
	AbstractKM(const std::string& ap_name,
		   const std::string& ap_instance);
	~AbstractKM();

	unsigned int get_address() const;
	void register_application_response_handler(const rina::RegisterApplicationResponseEvent& event);
	void unregister_application_response_handler(const rina::UnregisterApplicationResponseEvent& event);
	void allocate_flow_request_result_handler(const rina::AllocateFlowRequestResultEvent& event);
	void flow_allocation_requested_handler(const rina::FlowRequestEvent& event);
	void flow_deallocated_event_handler(const rina::FlowDeallocatedEvent& event);

        KMRIBDaemon * ribd;
        KMSecurityManager * secman;
        KMIPCResourceManager * irm;
        rina::SimpleInternalEventManager * eventm;
        KeyContainerManager * kcm;
};

class KMFactory{

public:
	/**
	* Create CKM
	*/
	static AbstractKM * create_central_key_manager(const std::list<std::string>& dif_names,
			  	  	  	       const std::string& app_name,
						       const std::string& app_instance,
						       const std::string& creds_folder);
	/**
	* Create KMA
	*/
	static AbstractKM * create_key_management_agent(const std::string& creds_folder,
			   	   	   	   	const std::list<std::string>& dif_names,
							const std::string& apn,
							const std::string& api,
							const std::string& ckm_apn,
							const std::string& ckm_api,
							bool  q);
	static AbstractKM * get_instance();
};


class KeyContainersRIBObject: public rina::rib::RIBObj {
public:
	KeyContainersRIBObject();
	const std::string& get_class() const {
		return class_name;
	};

	const static std::string class_name;
	const static std::string object_name;
};

class KeyContainerRIBObject: public rina::rib::RIBObj {
public:
	KeyContainerRIBObject(KeyContainerManager * kcm,
			      const std::string& id);
	const std::string get_displayable_value() const;

	const std::string& get_class() const {
		return class_name;
	};

	bool delete_(const rina::cdap_rib::con_handle_t &con,
		     const std::string& fqn,
		     const std::string& class_,
		     const rina::cdap_rib::filt_info_t &filt,
		     const int invoke_id,
		     rina::cdap_rib::res_info_t& res);

	void read(const rina::cdap_rib::con_handle_t &con,
		  const std::string& fqn,
		  const std::string& class_,
		  const rina::cdap_rib::filt_info_t &filt,
		  const int invoke_id,
		  rina::cdap_rib::obj_info_t &obj_reply,
		  rina::cdap_rib::res_info_t& res);

	static void create_cb(const rina::rib::rib_handle_t rib,
			      const rina::cdap_rib::con_handle_t &con,
			      const std::string& fqn,
			      const std::string& class_,
			      const rina::cdap_rib::filt_info_t &filt,
			      const int invoke_id,
			      const rina::ser_obj_t &obj_req,
			      rina::cdap_rib::obj_info_t &obj_reply,
			      rina::cdap_rib::res_info_t& res);

	const static std::string class_name;
	const static std::string object_name_prefix;

private:
	KeyContainerManager * kcm;
	std::string kc_id;
};

#endif//KM_COMMON_HPP
