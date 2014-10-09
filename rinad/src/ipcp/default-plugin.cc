#include "ipcp/component-factory.h"

namespace rinad {

extern "C" IPCProcessComponent *
createSecurityManager(IPCProcess * ipc_process);

extern "C" void
destroySecurityManager(IPCProcessComponent *component);

extern "C" void
init(IPCProcess * ipc_process)
{
        struct ComponentFactory factory;

        factory.name = "default";
        factory.component = "security-manager";
        factory.create = createSecurityManager;
        factory.destroy = destroySecurityManager;

        (void) factory;
        (void) ipc_process;
}

}   // namespace rinad
