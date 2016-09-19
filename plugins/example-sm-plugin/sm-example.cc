#include <string>
#include <sstream>

#define IPCP_MODULE "security-manager-example-ps"
#include "ipcp-logging.h"
#include "components.h"
#include "librina/security-manager.h"
#include "common/configuration.h"


using namespace rina;

namespace rinad {

class SecurityManagerExamplePs: public rina::ISecurityManagerPs {
public:
	SecurityManagerExamplePs(IPCPSecurityManager * dm);


	int isAllowedToJoinDAF(const cdap_rib::con_handle_t & con,
				       const Neighbor& newMember,
				       cdap_rib::auth_policy_t & auth);

	int storeAccessControlCreds(const cdap_rib::auth_policy_t & auth,
				    const cdap_rib::con_handle_t & con);

	int getAccessControlCreds(cdap_rib::auth_policy_t & auth,
				  const cdap_rib::con_handle_t & con);

	void checkRIBOperation(const cdap_rib::auth_policy_t & auth,
                               const cdap_rib::con_handle_t & con,
                               const cdap::cdap_m_t::Opcode opcode,
                               const std::string obj_name,
                               cdap_rib::res_info_t& res);

	int set_policy_set_param(const std::string& name,
			const std::string& value);
	virtual ~SecurityManagerExamplePs() {}

private:
	// Data model of the security manager component.
	IPCPSecurityManager * dm;

	int max_retries;
};

SecurityManagerExamplePs::SecurityManagerExamplePs(IPCPSecurityManager * dm_)
						: dm(dm_)
{
}


int SecurityManagerExamplePs::isAllowedToJoinDAF(
                            const cdap_rib::con_handle_t & con,
                            const Neighbor& newMember,
                            cdap_rib::auth_policy_t & auth)
{
	(void) con;
	(void) auth;
	LOG_IPCP_DBG("Allowing IPC Process %s to join the DIF",
		     newMember.name_.processName.c_str());
	return 0;
}

int SecurityManagerExamplePs::storeAccessControlCreds(
                            const cdap_rib::auth_policy_t & auth,
                            const cdap_rib::con_handle_t & con)
{
	(void) auth;
	(void) con;

	return 0;
}

int SecurityManagerExamplePs::getAccessControlCreds(
                                cdap_rib::auth_policy_t & auth,
                                const cdap_rib::con_handle_t & con)
{
	(void) auth;
	(void) con;

	return 0;
}

void SecurityManagerExamplePs::checkRIBOperation(
                            const cdap_rib::auth_policy_t & auth,
                            const cdap_rib::con_handle_t & con,
                            const cdap::cdap_m_t::Opcode opcode,
                            const std::string obj_name,
                            cdap_rib::res_info_t& res)
{
	(void) auth;
	(void) con;
	(void) opcode;
	(void) obj_name;

	res.code_ = rina::cdap_rib::CDAP_SUCCESS;
}

int SecurityManagerExamplePs::set_policy_set_param(const std::string& name,
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
createSecurityManagerExamplePs(rina::ApplicationEntity * ctx)
{
	IPCPSecurityManager * sm = dynamic_cast<IPCPSecurityManager *>(ctx);

	if (!sm) {
		return NULL;
	}

	return new SecurityManagerExamplePs(sm);
}

extern "C" void
destroySecurityManagerExamplePs(rina::IPolicySet * ps)
{
	if (ps) {
		delete ps;
	}
}

}   // namespace rinad
