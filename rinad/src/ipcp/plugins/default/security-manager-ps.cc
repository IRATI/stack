//
// Defaul policy set for Security Manager
//
//    Eduard Grasa <eduard.grasa@i2cat.net>
//    Vincenzo Maffione <v.maffione@nextworks.it>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#define IPCP_MODULE "security-manager-ps-default"
#include "../../ipcp-logging.h"

#include <string>
#include <librina/security-manager.h>

#include "ipcp/components.h"

namespace rinad {

class SecurityManagerPs: public ISecurityManagerPs {
public:
	SecurityManagerPs(IPCPSecurityManager * dm);
	bool isAllowedToJoinDIF(const rina::Neighbor& newMember);
	bool acceptFlow(const Flow& newFlow);
        int set_policy_set_param(const std::string& name,
                                 const std::string& value);
        virtual ~SecurityManagerPs() {}

private:
        // Data model of the security manager component.
        IPCPSecurityManager * dm;
};

SecurityManagerPs::SecurityManagerPs(IPCPSecurityManager * dm_) : dm(dm_)
{
}


bool SecurityManagerPs::isAllowedToJoinDIF(const rina::Neighbor& newMember)
{
	LOG_IPCP_DBG("Allowing IPC Process %s to join the DIF", newMember.name_.processName.c_str());
	return true;
}

bool SecurityManagerPs::acceptFlow(const Flow& newFlow)
{
	LOG_IPCP_DBG("Accepting flow from remote application %s",
			newFlow.source_naming_info.getEncodedString().c_str());
	return true;
}

int SecurityManagerPs::set_policy_set_param(const std::string& name,
                                            const std::string& value)
{
        LOG_IPCP_DBG("No policy-set-specific parameters to set (%s, %s)",
                        name.c_str(), value.c_str());
        return -1;
}

extern "C" rina::IPolicySet *
createSecurityManagerPs(rina::ApplicationEntity * ctx)
{
	IPCPSecurityManager * sm = dynamic_cast<IPCPSecurityManager *>(ctx);

        if (!sm) {
                return NULL;
        }

        return new SecurityManagerPs(sm);
}

extern "C" void
destroySecurityManagerPs(rina::IPolicySet * ps)
{
        if (ps) {
                delete ps;
        }
}

extern "C" rina::IPolicySet *
createAuthNonePs(rina::ApplicationEntity * ctx)
{
	IPCPSecurityManager * sm = dynamic_cast<IPCPSecurityManager *>(ctx);

        if (!sm) {
                return NULL;
        }

        return new rina::AuthNonePolicySet(sm);
}

extern "C" void
destroyAuthNonePs(rina::IPolicySet * ps)
{
        if (ps) {
                delete ps;
        }
}

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

}
