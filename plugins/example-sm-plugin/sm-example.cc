#include <string>
#include <sstream>

#define IPCP_MODULE "security-manager-example-ps"
#include "ipcp-logging.h"
#include "components.h"
#include "common/configuration.h"


namespace rinad {

class SecurityManagerExamplePs: public ISecurityManagerPs {
public:
	SecurityManagerExamplePs(IPCPSecurityManager * dm);
	bool isAllowedToJoinDIF(const rina::Neighbor& newMember);
	bool acceptFlow(const configs::Flow& newFlow);
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


bool SecurityManagerExamplePs::isAllowedToJoinDIF(const rina::Neighbor&
						 newMember)
{
	LOG_IPCP_DBG("Allowing IPC Process %s to join the DIF",
		     newMember.name_.processName.c_str());
	return true;
}

bool SecurityManagerExamplePs::acceptFlow(const configs::Flow& newFlow)
{
	LOG_IPCP_DBG("Accepting flow from remote application %s",
			newFlow.source_naming_info.getEncodedString().c_str());
	return true;
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
