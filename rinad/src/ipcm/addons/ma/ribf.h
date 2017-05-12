/*
 * RIB factory
 *
 *    Bernat Gaston         <bernat.gaston@i2cat.net>
 *    Marc Sune             <marc.sune (at) bisdn.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#ifndef __RINAD_RIBM_H__
#define __RINAD_RIBM_H__

#include <list>
#include <librina/concurrency.h>
#include <inttypes.h>
#include <librina/rib_v2.h>
#include <librina/security-manager.h>

namespace rinad{
namespace mad{

/**
 * @file ribf.h
 * @author Marc Sune<marc.sune (at) bisdn.de>
 *
 * @brief RIB Factory
 */

//
// RIB factory exceptions
//

/**
 * Duplicated RIB exception
 */
DECLARE_EXCEPTION_SUBCLASS(eDuplicatedRIB);

/**
 * RIB not found exception
 */
DECLARE_EXCEPTION_SUBCLASS(eRIBNotFound);

// Helper type
typedef std::map<uint64_t, std::list<std::string> > RIBAEassoc;

class MASecurityManager;

class RIBConHandler : public rina::cacep::AppConHandlerInterface,
		      public rina::rib::RIBOpsRespHandler
{
public:
	RIBConHandler();
	~RIBConHandler(){};

	void setSecurityManager(MASecurityManager * sec_man);

	void connect(const rina::cdap::CDAPMessage& message,
		     rina::cdap_rib::con_handle_t &con);
	void connectResult(const rina::cdap::CDAPMessage& message,
			   rina::cdap_rib::con_handle_t &con);
	void release(int message_id, const rina::cdap_rib::con_handle_t &con);
	void releaseResult(const rina::cdap_rib::res_info_t &res,
			  const rina::cdap_rib::con_handle_t &con);
	void process_authentication_message(const rina::cdap::CDAPMessage& message,
					    const rina::cdap_rib::con_handle_t &con);

	void remoteReadResult(const rina::cdap_rib::con_handle_t &con,
			      const rina::cdap_rib::obj_info_t &obj,
			      const rina::cdap_rib::res_info_t &res);

private:
	MASecurityManager * sec_man;
};

/**
 * @brief RIB manager
 */
class RIBFactory{

public:
	//Constructors
	RIBFactory(RIBAEassoc ver_assoc);
	virtual ~RIBFactory(void) throw();


	/**
	* Get the RIB provider proxy
	*/
	static inline rina::rib::RIBDaemonProxy* getProxy(){
		if(!ribd)
			ribd = rina::rib::RIBDaemonProxyFactory();
		return ribd;
	}

	/**
	* Get the handle of the specific RIB and AE name associated
	*/
	static rina::rib::rib_handle_t getRIBHandle(const uint64_t& version,
						    const std::string& AEname);

	static rina::rib::rib_handle_t getDefaultRIBHandle();

	//Process IPCP create event to all RIB versions
	void createIPCPevent(int ipcp_id);

	//Process IPCP create event to all RIB versions
	void destroyIPCPevent(int ipcp_id);

	MASecurityManager * getSecurityManager();

protected:
	//Mutex
	rina::Lockable mutex;

	//Map handle <-> version
	std::map<rina::rib::rib_handle_t, uint64_t> ribs;

	//RIBProxy instance
	static rina::rib::RIBDaemonProxy* ribd;

	MASecurityManager * sec_man;

	RIBConHandler rib_con_handler;
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

class MASDUProtectionHandler : public rina::cdap::SDUProtectionHandler
{
public:
	MASDUProtectionHandler();
	~MASDUProtectionHandler() {};

	void set_security_manager(rina::ISecurityManager * sec_man);
	void protect_sdu(rina::ser_obj_t& sdu, int port_id);
	void unprotect_sdu(rina::ser_obj_t& sdu, int port_id);

private:
	rina::ISecurityManager * secman;
};

class MASecurityManager: public rina::ISecurityManager
{
public:
	MASecurityManager(const std::string& creds_location);
	~MASecurityManager();

	void set_application_process(rina::ApplicationProcess * ap);
	void set_rib_daemon(rina::rib::RIBDaemonProxy * ribd);
        rina::IAuthPolicySet::AuthStatus update_crypto_state(const rina::CryptoState& state,
        						     rina::IAuthPolicySet * caller);
        rina::AuthSDUProtectionProfile get_sec_profile(const std::string& auth_policy_name);
private:
        rina::AuthSSH2PolicySet * ssh2_auth_ps;
        rina::AuthNonePolicySet* none_auth_ps;
        rina::AuthSDUProtectionProfile auth_ssh2_sec_profile;
        rina::AuthSDUProtectionProfile auth_none_sec_profile;
        MASDUProtectionHandler * sdup;
};

}; //namespace mad
}; //namespace rinad

#endif  /* __RINAD_RIBM_H__ */
