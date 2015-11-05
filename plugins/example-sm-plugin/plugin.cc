#include <string>
#include <sstream>

#define IPCP_MODULE "security-manager-example-ps"
#include "ipcp-logging.h"
#include "components.h"


namespace rinad {

extern "C" rina::IPolicySet *
createSecurityManagerExamplePs(rina::ApplicationEntity * ctx);

extern "C" void
destroySecurityManagerExamplePs(rina::IPolicySet * ps);

extern "C" int
get_factories(std::vector<struct rina::PsFactory>& factories)
{
	struct rina::PsFactory factory;

	factory.info.name = "example";
	factory.info.app_entity = rina::ApplicationEntity::SECURITY_MANAGER_AE_NAME;
	factory.create = createSecurityManagerExamplePs;
	factory.destroy = destroySecurityManagerExamplePs;
	factories.push_back(factory);

	return 0;
}

}   // namespace rinad
