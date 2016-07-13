#define IPCP_MODULE "security-manager-ps-cbac"
#include "../../ipcp-logging.h"

#include "ipcp/components.h"
#include "security-manager-cbac.h"

namespace rinad {

extern "C" rina::IPolicySet *
createCBACPs(rina::ApplicationEntity * ctx)
{
        IPCPSecurityManager * sm = dynamic_cast<IPCPSecurityManager *>(ctx);
        if (!sm || !sm->get_application_process()) {
                return NULL;
        }
        IPCPRIBDaemon * rib_daemon =
        		dynamic_cast<IPCPRIBDaemon *>(sm->get_application_process()->get_rib_daemon());
        if (!rib_daemon) {
        	return NULL;
        }
        // instantiate policy set here
        return new SecurityManagerCBACPs(sm);
        //return NULL;
}

extern "C" void
destroyCBACPs(rina::IPolicySet * ps)
{
        if (ps) {
                delete ps;
        }
}

extern "C" int
get_factories(std::vector<struct rina::PsFactory>& factories)
{
        struct rina::PsFactory cbac_factory;

        cbac_factory.info.name = "cbac";
        cbac_factory.info.app_entity = rina::ApplicationEntity::SECURITY_MANAGER_AE_NAME;
        cbac_factory.create = createCBACPs;
        cbac_factory.destroy = destroyCBACPs;
	factories.push_back(cbac_factory);

	return 0;
}

}   // namespace rinad
