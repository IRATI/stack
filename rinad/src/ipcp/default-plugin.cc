#include "ipcp/components.h"

namespace rinad {

extern "C" IPolicySet *
createSecurityManagerPs(IPCProcessComponent * context);

extern "C" void
destroySecurityManagerPs(IPolicySet * instance);

extern "C" IPolicySet *
createFlowAllocatorPs(IPCProcessComponent * context);

extern "C" void
destroyFlowAllocatorPs(IPolicySet * instance);

extern "C" int
init(IPCProcess * ipc_process, const std::string& plugin_name)
{
        struct PsFactory sm_factory;
        struct PsFactory fa_factory;
        int ret;

        sm_factory.plugin_name = plugin_name;
        sm_factory.name = "default";
        sm_factory.component = "security-manager";
        sm_factory.create = createSecurityManagerPs;
        sm_factory.destroy = destroySecurityManagerPs;

        ret = ipc_process->psFactoryPublish(sm_factory);
        if (ret) {
                return ret;
        }

        fa_factory.plugin_name = plugin_name;
        fa_factory.name = "default";
        fa_factory.component = "flow-allocator";
        fa_factory.create = createFlowAllocatorPs;
        fa_factory.destroy = destroyFlowAllocatorPs;

        ret = ipc_process->psFactoryPublish(fa_factory);
        if (ret) {
                return ret;
        }

        return 0;
}

}   // namespace rinad
