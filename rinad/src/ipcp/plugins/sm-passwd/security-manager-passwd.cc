#define RINA_PREFIX "security-manager-ps-passwd"

#include <string>
#include <sstream>
#include <librina/logs.h>

#include "ipcp/components.h"

namespace rinad {

class SecurityManagerPasswdPs: public ISecurityManagerPs {
public:
	SecurityManagerPasswdPs(ISecurityManager * dm);
	bool isAllowedToJoinDIF(const rina::Neighbor& newMember);
	bool acceptFlow(const Flow& newFlow);
        int set_policy_set_param(const std::string& name,
                                 const std::string& value);
        virtual ~SecurityManagerPasswdPs() {}

private:
        // Data model of the security manager component.
        ISecurityManager * dm;

        int max_retries;
};

SecurityManagerPasswdPs::SecurityManagerPasswdPs(ISecurityManager * dm_) : dm(dm_)
{
	(void)dm;
}


bool SecurityManagerPasswdPs::isAllowedToJoinDIF(const rina::Neighbor& newMember)
{
	LOG_DBG("Allowing IPC Process %s to join the DIF", newMember.name_.processName.c_str());
	return true;
}

bool SecurityManagerPasswdPs::acceptFlow(const Flow& newFlow)
{
	LOG_DBG("Accepting flow from remote application %s",
			newFlow.source_naming_info.getEncodedString().c_str());
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
                        LOG_ERR("Invalid value '%s'", value.c_str());
                        return -1;
                }

                max_retries = x;
        } else {
                LOG_ERR("Unknown parameter '%s'", name.c_str());
                return -1;
        }

        return 0;
}

extern "C" rina::IPolicySet *
createSecurityManagerPasswdPs(rina::ApplicationEntity * ctx)
{
        ISecurityManager * sm = dynamic_cast<ISecurityManager *>(ctx);

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
init(IPCProcess * ipc_process, const std::string& plugin_name)
{
        struct rina::PsFactory factory;
        int ret;

        factory.plugin_name = plugin_name;
        factory.name = "passwd";
        factory.app_entity = IPCProcessComponent::SECURITY_MANAGER_AE_NAME;
        factory.create = createSecurityManagerPasswdPs;
        factory.destroy = destroySecurityManagerPasswdPs;

        ret = ipc_process->psFactoryPublish(factory);

        return ret;
}

}   // namespace rinad
