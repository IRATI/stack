#include "ipcp/components.h"

namespace rinad {

extern "C" IPolicySet *
createSecurityManagerPs(IPCProcessComponent * context);

extern "C" void
destroySecurityManagerPs(IPolicySet * instance);

extern "C" int
init(IPCProcess * ipc_process)
{
        struct PsFactory factory;
        int ret;

        factory.name = "default";
        factory.component = "security-manager";
        factory.create = createSecurityManagerPs;
        factory.destroy = destroySecurityManagerPs;

        ret = ipc_process->psFactoryPublish(factory);

        return ret;
}

}   // namespace rinad
