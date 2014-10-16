#define RINA_PREFIX "security-manager-passwd"

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
};

SecurityManagerPasswdPs::SecurityManagerPasswdPs(ISecurityManager * dm_) : dm(dm_)
{ }


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
        LOG_DBG("No policy-set specific parameters to set (%s, %s)",
                        name.c_str(), value.c_str());
        return -1;
}

extern "C" IPolicySet *
createSecurityManagerPasswdPs(IPCProcessComponent * ctx)
{
        ISecurityManager * sm = dynamic_cast<ISecurityManager *>(ctx);

        if (!sm) {
                return NULL;
        }

        return new SecurityManagerPasswdPs(sm);
}

extern "C" void
destroySecurityManagerPasswdPs(IPolicySet * ps)
{
        if (ps) {
                delete ps;
        }
}

extern "C" int
init(IPCProcess * ipc_process)
{
        struct ComponentFactory factory;
        int ret;

        factory.name = "passwd";
        factory.component = "security-manager";
        factory.create = createSecurityManagerPasswdPs;
        factory.destroy = destroySecurityManagerPasswdPs;

        ret = ipc_process->componentFactoryPublish(factory);

        return ret;
}

}   // namespace rinad
