#define IPCP_MODULE "security-manager-ps-passwd"
#include "../../ipcp-logging.h"

#include <string>
#include <sstream>

#include "ipcp/components.h"


namespace rinad {

class SecurityManagerPasswdPs: public IPCPSecurityManagerPs {
public:
	SecurityManagerPasswdPs(IPCPSecurityManager * dm);
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
	bool acceptFlow(const configs::Flow& newFlow);
	int set_policy_set_param(const std::string& name,
			const std::string& value);
	virtual ~SecurityManagerPasswdPs() {}

private:
	// Data model of the security manager component.
	IPCPSecurityManager * dm;

	int max_retries;
};

SecurityManagerPasswdPs::SecurityManagerPasswdPs(IPCPSecurityManager * dm_)
						: dm(dm_)
{
	max_retries = 0;
}

int SecurityManagerPasswdPs::isAllowedToJoinDAF(const rina::cdap_rib::con_handle_t & con,
						const rina::Neighbor &newMember,
						rina::cdap_rib::auth_policy_t& auth)
{
	(void) con;
	(void) auth;
	LOG_IPCP_DBG("Allowing IPC Process %s to join the DIF",
		     newMember.name_.processName.c_str());
	return 0;
}

int SecurityManagerPasswdPs::storeAccessControlCreds(const rina::cdap_rib::auth_policy_t & auth,
			    	    	    	     const rina::cdap_rib::con_handle_t & con)
{
	(void) auth;
	(void) con;
	return 0;
}

int SecurityManagerPasswdPs::getAccessControlCreds(rina::cdap_rib::auth_policy_t & auth,
		          	  	  	   const rina::cdap_rib::con_handle_t & con)
{
	(void) auth;
	(void) con;
	return 0;
}

void SecurityManagerPasswdPs::checkRIBOperation(const rina::cdap_rib::auth_policy_t & auth,
		      	      	      	        const rina::cdap_rib::con_handle_t & con,
					        const rina::cdap::cdap_m_t::Opcode opcode,
					        const std::string obj_name,
						rina::cdap_rib::res_info_t& res)
{
	(void) auth;
	(void) con;
	(void) opcode;
	(void) obj_name;

	res.code_ = rina::cdap_rib::CDAP_SUCCESS;
}

bool SecurityManagerPasswdPs::acceptFlow(const configs::Flow& newFlow)
{
	LOG_IPCP_DBG("Accepting flow from remote application %s",
			newFlow.remote_naming_info.getEncodedString().c_str());
	return true;
}

int SecurityManagerPasswdPs::set_policy_set_param(const std::string& name,
						  const std::string& value)
{
	if (name == "max_retries") {
		int x;
		std::stringstream ss;

		ss << value;
		ss >> x;
		if (ss.fail()) {
			LOG_IPCP_ERR("Invalid value '%s'", value.c_str());
			return -1;
		}

		max_retries = x;
	} else {
		LOG_IPCP_ERR("Unknown parameter '%s'", name.c_str());
		return -1;
	}

	return 0;
}

extern "C" rina::IPolicySet *
createSecurityManagerPasswdPs(rina::ApplicationEntity * ctx)
{
	IPCPSecurityManager * sm = dynamic_cast<IPCPSecurityManager *>(ctx);

	if (!sm) {
		return NULL;
	}

	return new SecurityManagerPasswdPs(sm);
}

extern "C" void
destroySecurityManagerPasswdPs(rina::IPolicySet * ps)
{
	if (ps) {
		delete ps;
	}
}

extern "C" int
get_factories(std::vector<struct rina::PsFactory>& factories)
{
	struct rina::PsFactory factory;
	int ret;

	factory.info.name = "passwd";
	factory.info.app_entity = rina::ApplicationEntity::SECURITY_MANAGER_AE_NAME;
	factory.create = createSecurityManagerPasswdPs;
	factory.destroy = destroySecurityManagerPasswdPs;
	factories.push_back(factory);

	return 0;
}

}   // namespace rinad
