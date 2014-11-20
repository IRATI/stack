#include "ipcp/components.h"

namespace rinad {

extern "C" IPolicySet *
createSecurityManagerPs(IPCProcessComponent * context);

extern "C" void
destroySecurityManagerPs(IPolicySet * instance);

extern "C" int
init(IPCProcess * ipc_process, const std::string& plugin_name)
{
        struct PsFactory factory;
        int ret;

        factory.plugin_name = plugin_name;
        factory.name = "default";
        factory.component = "security-manager";
        factory.create = createSecurityManagerPs;
        factory.destroy = destroySecurityManagerPs;

        ret = ipc_process->psFactoryPublish(factory);

        return ret;
}

}   // namespace rinad
