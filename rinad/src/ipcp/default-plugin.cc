#include "ipcp/components.h"

namespace rinad {

extern "C" IPCProcessComponent *
createSecurityManager(void * context);

extern "C" void
destroySecurityManager(IPCProcessComponent *component);

extern "C" int
init(IPCProcess * ipc_process)
{
        struct ComponentFactory factory;
        int ret;

        factory.name = "default";
        factory.component = "security-manager";
        factory.create = createSecurityManager;
        factory.destroy = destroySecurityManager;

        ret = ipc_process->componentFactoryPublish(factory);

        return ret;
}

}   // namespace rinad
