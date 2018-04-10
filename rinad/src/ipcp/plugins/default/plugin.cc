#include "ipcp/components.h"

namespace rinad {

extern "C" rina::IPolicySet *
createSecurityManagerPs(rina::ApplicationEntity * context);

extern "C" void
destroySecurityManagerPs(rina::IPolicySet * instance);

extern "C" rina::IPolicySet *
createAuthNonePs(rina::ApplicationEntity * context);

extern "C" void
destroyAuthNonePs(rina::IPolicySet * instance);

extern "C" rina::IPolicySet *
createFlowAllocatorPs(rina::ApplicationEntity * context);

extern "C" void
destroyFlowAllocatorPs(rina::IPolicySet * instance);

extern "C" rina::IPolicySet *
createFlowAllocatorRoundRobinPs(rina::ApplicationEntity * context);

extern "C" void
destroyFlowAllocatorRoundRobinPs(rina::IPolicySet * instance);

extern "C" rina::IPolicySet *
createFlowAllocatorQTAPs(rina::ApplicationEntity * context);

extern "C" void
destroyFlowAllocatorQTAPs(rina::IPolicySet * instance);

extern "C" rina::IPolicySet *
createNamespaceManagerPs(rina::ApplicationEntity * context);

extern "C" void
destroyNamespaceManagerPs(rina::IPolicySet * instance);

extern "C" rina::IPolicySet *
createPDUFTGenPs(rina::ApplicationEntity * ctx);

extern "C" void
destroyPDUFTGenPs(rina::IPolicySet * ps);

extern "C" rina::IPolicySet *
createEnrollmentTaskPs(rina::ApplicationEntity * context);

extern "C" void
destroyEnrollmentTaskPs(rina::IPolicySet * instance);

extern "C" rina::IPolicySet *
createRoutingComponentPs(rina::ApplicationEntity * context);

extern "C" void
destroyRoutingComponentPs(rina::IPolicySet * instance);

extern "C" rina::IPolicySet *
createStaticRoutingComponentPs(rina::ApplicationEntity * context);

extern "C" void
destroyStaticRoutingComponentPs(rina::IPolicySet * instance);

extern "C" int
get_factories(std::vector<struct rina::PsFactory>& factories)
{
        struct rina::PsFactory sm_factory;
        struct rina::PsFactory auth_none_factory;
        struct rina::PsFactory fa_factory;
        struct rina::PsFactory farr_factory;
        struct rina::PsFactory nsm_factory;
        struct rina::PsFactory pduft_gen_factory;
        struct rina::PsFactory et_factory;
        struct rina::PsFactory rc_factory;
        struct rina::PsFactory src_factory;

        sm_factory.info.name = rina::IPolicySet::DEFAULT_PS_SET_NAME;
        sm_factory.info.app_entity = rina::ApplicationEntity::SECURITY_MANAGER_AE_NAME;
        sm_factory.create = createSecurityManagerPs;
        sm_factory.destroy = destroySecurityManagerPs;
	factories.push_back(sm_factory);

        auth_none_factory.info.name = rina::IAuthPolicySet::AUTH_NONE;
        auth_none_factory.info.app_entity = rina::ApplicationEntity::SECURITY_MANAGER_AE_NAME;
        auth_none_factory.create = createAuthNonePs;
        auth_none_factory.destroy = destroyAuthNonePs;
	factories.push_back(auth_none_factory);

        fa_factory.info.name = rina::IPolicySet::DEFAULT_PS_SET_NAME;
        fa_factory.info.app_entity = IFlowAllocator::FLOW_ALLOCATOR_AE_NAME;
        fa_factory.create = createFlowAllocatorPs;
        fa_factory.destroy = destroyFlowAllocatorPs;
	factories.push_back(fa_factory);

        farr_factory.info.name = "RoundRobin";
        farr_factory.info.app_entity = IFlowAllocator::FLOW_ALLOCATOR_AE_NAME;
        farr_factory.create = createFlowAllocatorRoundRobinPs;
        farr_factory.destroy = destroyFlowAllocatorRoundRobinPs;
        factories.push_back(farr_factory);

        farr_factory.info.name = "qta-ps";
        farr_factory.info.app_entity = IFlowAllocator::FLOW_ALLOCATOR_AE_NAME;
        farr_factory.create = createFlowAllocatorQTAPs;
        farr_factory.destroy = destroyFlowAllocatorQTAPs;
        factories.push_back(farr_factory);

        nsm_factory.info.name = rina::IPolicySet::DEFAULT_PS_SET_NAME;
        nsm_factory.info.app_entity = INamespaceManager::NAMESPACE_MANAGER_AE_NAME;
        nsm_factory.create = createNamespaceManagerPs;
        nsm_factory.destroy = destroyNamespaceManagerPs;
	factories.push_back(nsm_factory);

        pduft_gen_factory.info.name = rina::IPolicySet::DEFAULT_PS_SET_NAME;
        pduft_gen_factory.info.app_entity =
				IResourceAllocator::RESOURCE_ALLOCATOR_AE_NAME;
        pduft_gen_factory.create = createPDUFTGenPs;
        pduft_gen_factory.destroy = destroyPDUFTGenPs;
	factories.push_back(pduft_gen_factory);

        et_factory.info.name = rina::IPolicySet::DEFAULT_PS_SET_NAME;
        et_factory.info.app_entity = rina::ApplicationEntity::ENROLLMENT_TASK_AE_NAME;
        et_factory.create = createEnrollmentTaskPs;
        et_factory.destroy = destroyEnrollmentTaskPs;
	factories.push_back(et_factory);

        rc_factory.info.name = "link-state";
        rc_factory.info.app_entity = IRoutingComponent::ROUTING_COMPONENT_AE_NAME;
        rc_factory.create = createRoutingComponentPs;
        rc_factory.destroy = destroyRoutingComponentPs;
	factories.push_back(rc_factory);

        src_factory.info.name = "static";
        src_factory.info.app_entity = IRoutingComponent::ROUTING_COMPONENT_AE_NAME;
        src_factory.create = createStaticRoutingComponentPs;
        src_factory.destroy = destroyStaticRoutingComponentPs;
	factories.push_back(src_factory);

	return 0;
}

}   // namespace rinad
