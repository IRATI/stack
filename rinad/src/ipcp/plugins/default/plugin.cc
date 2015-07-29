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
createAuthPasswordPs(rina::ApplicationEntity * context);

extern "C" void
destroyAuthPasswordPs(rina::IPolicySet * instance);

extern "C" rina::IPolicySet *
createAuthSSH2Ps(rina::ApplicationEntity * context);

extern "C" void
destroyAuthSSH2Ps(rina::IPolicySet * instance);

extern "C" rina::IPolicySet *
createFlowAllocatorPs(rina::ApplicationEntity * context);

extern "C" void
destroyFlowAllocatorPs(rina::IPolicySet * instance);

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

extern "C" int
init(IPCProcess * ipc_process, const std::string& plugin_name)
{
        struct rina::PsFactory sm_factory;
        struct rina::PsFactory auth_none_factory;
        struct rina::PsFactory auth_password_factory;
        struct rina::PsFactory auth_ssh2_factory;
        struct rina::PsFactory fa_factory;
        struct rina::PsFactory nsm_factory;
        struct rina::PsFactory pduft_gen_factory;
        struct rina::PsFactory et_factory;
        struct rina::PsFactory rc_factory;
        int ret;

        sm_factory.plugin_name = plugin_name;
        sm_factory.info.name = rina::IPolicySet::DEFAULT_PS_SET_NAME;
        sm_factory.info.app_entity = rina::ApplicationEntity::SECURITY_MANAGER_AE_NAME;
        sm_factory.create = createSecurityManagerPs;
        sm_factory.destroy = destroySecurityManagerPs;

        ret = ipc_process->psFactoryPublish(sm_factory);
        if (ret) {
                return ret;
        }

        auth_none_factory.plugin_name = plugin_name;
        auth_none_factory.info.name = rina::IAuthPolicySet::AUTH_NONE;
        auth_none_factory.info.app_entity = rina::ApplicationEntity::SECURITY_MANAGER_AE_NAME;
        auth_none_factory.create = createAuthNonePs;
        auth_none_factory.destroy = destroyAuthNonePs;

        ret = ipc_process->psFactoryPublish(auth_none_factory);
        if (ret) {
                return ret;
        }

        auth_password_factory.plugin_name = plugin_name;
        auth_password_factory.info.name = rina::IAuthPolicySet::AUTH_PASSWORD;
        auth_password_factory.info.app_entity = rina::ApplicationEntity::SECURITY_MANAGER_AE_NAME;
        auth_password_factory.create = createAuthPasswordPs;
        auth_password_factory.destroy = destroyAuthPasswordPs;

        ret = ipc_process->psFactoryPublish(auth_password_factory);
        if (ret) {
                return ret;
        }

        auth_ssh2_factory.plugin_name = plugin_name;
        auth_ssh2_factory.info.name = rina::IAuthPolicySet::AUTH_SSH2;
        auth_ssh2_factory.info.app_entity = rina::ApplicationEntity::SECURITY_MANAGER_AE_NAME;
        auth_ssh2_factory.create = createAuthSSH2Ps;
        auth_ssh2_factory.destroy = destroyAuthSSH2Ps;

        ret = ipc_process->psFactoryPublish(auth_ssh2_factory);
        if (ret) {
                return ret;
        }

        fa_factory.plugin_name = plugin_name;
        fa_factory.info.name = rina::IPolicySet::DEFAULT_PS_SET_NAME;
        fa_factory.info.app_entity = IFlowAllocator::FLOW_ALLOCATOR_AE_NAME;
        fa_factory.create = createFlowAllocatorPs;
        fa_factory.destroy = destroyFlowAllocatorPs;

        ret = ipc_process->psFactoryPublish(fa_factory);
        if (ret) {
                return ret;
        }

        nsm_factory.plugin_name = plugin_name;
        nsm_factory.info.name = rina::IPolicySet::DEFAULT_PS_SET_NAME;
        nsm_factory.info.app_entity = INamespaceManager::NAMESPACE_MANAGER_AE_NAME;
        nsm_factory.create = createNamespaceManagerPs;
        nsm_factory.destroy = destroyNamespaceManagerPs;

        ret = ipc_process->psFactoryPublish(nsm_factory);
        if (ret) {
                return ret;
        }

        pduft_gen_factory.plugin_name = plugin_name;
        pduft_gen_factory.info.name = rina::IPolicySet::DEFAULT_PS_SET_NAME;
        pduft_gen_factory.info.app_entity =
				IResourceAllocator::RESOURCE_ALLOCATOR_AE_NAME;
        pduft_gen_factory.create = createPDUFTGenPs;
        pduft_gen_factory.destroy = destroyPDUFTGenPs;

        ret = ipc_process->psFactoryPublish(pduft_gen_factory);
        if (ret) {
                return ret;
        }

        et_factory.plugin_name = plugin_name;
        et_factory.info.name = rina::IPolicySet::DEFAULT_PS_SET_NAME;
        et_factory.info.app_entity = rina::ApplicationEntity::ENROLLMENT_TASK_AE_NAME;
        et_factory.create = createEnrollmentTaskPs;
        et_factory.destroy = destroyEnrollmentTaskPs;

        ret = ipc_process->psFactoryPublish(et_factory);
        if (ret) {
        	return ret;
        }

        rc_factory.plugin_name = plugin_name;
        rc_factory.info.name = "link-state";
        rc_factory.info.app_entity = IRoutingComponent::ROUTING_COMPONENT_AE_NAME;
        rc_factory.create = createRoutingComponentPs;
        rc_factory.destroy = destroyRoutingComponentPs;

        ret = ipc_process->psFactoryPublish(rc_factory);
        if (ret) {
        	return ret;
        }

        return 0;
}

}   // namespace rinad
