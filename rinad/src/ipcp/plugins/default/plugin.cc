#include "ipcp/components.h"

namespace rinad {

extern "C" rina::IPolicySet *
createSecurityManagerPs(rina::ApplicationEntity * context);

extern "C" void
destroySecurityManagerPs(rina::IPolicySet * instance);

extern "C" rina::IPolicySet *
createFlowAllocatorPs(rina::ApplicationEntity * context);

extern "C" void
destroyFlowAllocatorPs(rina::IPolicySet * instance);

extern "C" rina::IPolicySet *
createNamespaceManagerPs(rina::ApplicationEntity * context);

extern "C" void
destroyNamespaceManagerPs(rina::IPolicySet * instance);

extern "C" rina::IPolicySet *
createResourceAllocatorPs(rina::ApplicationEntity * context);

extern "C" void
destroyResourceAllocatorPs(rina::IPolicySet * instance);

extern "C" rina::IPolicySet *
createRoutingComponentPs(rina::ApplicationEntity * context);

extern "C" void
destroyRoutingComponentPs(rina::IPolicySet * instance);

extern "C" int
init(IPCProcess * ipc_process, const std::string& plugin_name)
{
        struct rina::PsFactory sm_factory;
        struct rina::PsFactory fa_factory;
        struct rina::PsFactory nsm_factory;
        struct rina::PsFactory ra_factory;
        struct rina::PsFactory rc_factory;
        int ret;

        sm_factory.plugin_name = plugin_name;
        sm_factory.name = rina::IPolicySet::DEFAULT_PS_SET_NAME;
        sm_factory.app_entity = ISecurityManager::SECURITY_MANAGER_AE_NAME;
        sm_factory.create = createSecurityManagerPs;
        sm_factory.destroy = destroySecurityManagerPs;

        ret = ipc_process->psFactoryPublish(sm_factory);
        if (ret) {
                return ret;
        }

        fa_factory.plugin_name = plugin_name;
        fa_factory.name = rina::IPolicySet::DEFAULT_PS_SET_NAME;
        fa_factory.app_entity = IFlowAllocator::FLOW_ALLOCATOR_AE_NAME;
        fa_factory.create = createFlowAllocatorPs;
        fa_factory.destroy = destroyFlowAllocatorPs;

        ret = ipc_process->psFactoryPublish(fa_factory);
        if (ret) {
                return ret;
        }

        nsm_factory.plugin_name = plugin_name;
        nsm_factory.name = rina::IPolicySet::DEFAULT_PS_SET_NAME;
        nsm_factory.app_entity = INamespaceManager::NAMESPACE_MANAGER_AE_NAME;
        nsm_factory.create = createNamespaceManagerPs;
        nsm_factory.destroy = destroyNamespaceManagerPs;

        ret = ipc_process->psFactoryPublish(nsm_factory);
        if (ret) {
                return ret;
        }

        ra_factory.plugin_name = plugin_name;
        ra_factory.name = rina::IPolicySet::DEFAULT_PS_SET_NAME;
        ra_factory.app_entity = IResourceAllocator::RESOURCE_ALLOCATOR_AE_NAME;
        ra_factory.create = createResourceAllocatorPs;
        ra_factory.destroy = destroyResourceAllocatorPs;

        ret = ipc_process->psFactoryPublish(ra_factory);
        if (ret) {
                return ret;
        }

        rc_factory.plugin_name = plugin_name;
        rc_factory.name = "link-state";
        rc_factory.app_entity = IRoutingComponent::ROUTING_COMPONENT_AE_NAME;
        rc_factory.create = createRoutingComponentPs;
        rc_factory.destroy = destroyRoutingComponentPs;

        ret = ipc_process->psFactoryPublish(rc_factory);
        if (ret) {
        	return ret;
        }

        return 0;
}

}   // namespace rinad
