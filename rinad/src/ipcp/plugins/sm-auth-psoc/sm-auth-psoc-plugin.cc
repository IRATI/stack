#define IPCP_MODULE "security-manager-ps-auth-psoc"
#include "../../ipcp-logging.h"

#include "ipcp/components.h"

#include <librina/tlshand-authp.h>

namespace rinad {

extern "C" rina::IPolicySet *
createAuthPasswordPs(rina::ApplicationEntity * ctx)
{
	IPCPSecurityManager * sm = dynamic_cast<IPCPSecurityManager *>(ctx);
        if (!sm || !sm->get_application_process()) {
                return NULL;
        }

        IPCPRIBDaemon * rib_daemon =
        		dynamic_cast<IPCPRIBDaemon *>(sm->get_application_process()->get_rib_daemon());
        if (!rib_daemon) {
        	return NULL;
        }

        return new rina::AuthPasswordPolicySet(rib_daemon->getProxy(), sm);
}

extern "C" void
destroyAuthPasswordPs(rina::IPolicySet * ps)
{
        if (ps) {
                delete ps;
        }
}

extern "C" rina::IPolicySet *
createAuthSSH2Ps(rina::ApplicationEntity * ctx)
{
	IPCPSecurityManager * sm = dynamic_cast<IPCPSecurityManager *>(ctx);
        if (!sm || !sm->get_application_process()) {
                return NULL;
        }

        IPCPRIBDaemon * rib_daemon =
        		dynamic_cast<IPCPRIBDaemon *>(sm->get_application_process()->get_rib_daemon());
        if (!rib_daemon) {
        	return NULL;
        }

        return new rina::AuthSSH2PolicySet(rib_daemon->getProxy(), sm);
}

extern "C" void
destroyAuthSSH2Ps(rina::IPolicySet * ps)
{
        if (ps) {
                delete ps;
        }
}


extern "C" rina::IPolicySet *
createAuthTLSHandPs(rina::ApplicationEntity * ctx)
{
	IPCPSecurityManager * sm = dynamic_cast<IPCPSecurityManager *>(ctx);
        if (!sm || !sm->get_application_process()) {
                return NULL;
        }

        IPCPRIBDaemon * rib_daemon =
        		dynamic_cast<IPCPRIBDaemon *>(sm->get_application_process()->get_rib_daemon());
        if (!rib_daemon) {
        	return NULL;
        }

        return new rina::AuthTLSHandPolicySet(rib_daemon->getProxy(), sm);
}

extern "C" void
destroyAuthTLSHandPs(rina::IPolicySet * ps)
{
        if (ps) {
                delete ps;
        }
}

extern "C" int
get_factories(std::vector<struct rina::PsFactory>& factories)
{
        struct rina::PsFactory auth_password_factory;
        struct rina::PsFactory auth_ssh2_factory;
        struct rina::PsFactory auth_tls_hand_factory;

        auth_password_factory.info.name = rina::IAuthPolicySet::AUTH_PASSWORD;
        auth_password_factory.info.app_entity = rina::ApplicationEntity::SECURITY_MANAGER_AE_NAME;
        auth_password_factory.create = createAuthPasswordPs;
        auth_password_factory.destroy = destroyAuthPasswordPs;
	factories.push_back(auth_password_factory);

        auth_ssh2_factory.info.name = rina::IAuthPolicySet::AUTH_SSH2;
        auth_ssh2_factory.info.app_entity = rina::ApplicationEntity::SECURITY_MANAGER_AE_NAME;
        auth_ssh2_factory.create = createAuthSSH2Ps;
        auth_ssh2_factory.destroy = destroyAuthSSH2Ps;
	factories.push_back(auth_ssh2_factory);

	auth_tls_hand_factory.info.name = rina::IAuthPolicySet::AUTH_TLSHAND;
	auth_tls_hand_factory.info.app_entity = rina::ApplicationEntity::SECURITY_MANAGER_AE_NAME;
	auth_tls_hand_factory.create = createAuthTLSHandPs;
	auth_tls_hand_factory.destroy = destroyAuthTLSHandPs;
	factories.push_back(auth_tls_hand_factory);

	return 0;
}

}   // namespace rinad
