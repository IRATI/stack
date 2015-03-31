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

#define RINA_PREFIX "security-manager-ps-default"

#include <librina/logs.h>
#include <string>

#include "ipcp/components.h"

namespace rinad {

class SecurityManagerPs: public ISecurityManagerPs {
public:
	SecurityManagerPs(ISecurityManager * dm);
	bool isAllowedToJoinDIF(const rina::Neighbor& newMember);
	bool acceptFlow(const Flow& newFlow);
        int set_policy_set_param(const std::string& name,
                                 const std::string& value);
        virtual ~SecurityManagerPs() {}

private:
        // Data model of the security manager component.
        ISecurityManager * dm;
};

SecurityManagerPs::SecurityManagerPs(ISecurityManager * dm_) : dm(dm_)
{
	(void)dm;
}


bool SecurityManagerPs::isAllowedToJoinDIF(const rina::Neighbor& newMember)
{
	LOG_DBG("Allowing IPC Process %s to join the DIF", newMember.name_.processName.c_str());
	return true;
}

bool SecurityManagerPs::acceptFlow(const Flow& newFlow)
{
	LOG_DBG("Accepting flow from remote application %s",
			newFlow.source_naming_info.getEncodedString().c_str());
	return true;
}

int SecurityManagerPs::set_policy_set_param(const std::string& name,
                                            const std::string& value)
{
        LOG_DBG("No policy-set-specific parameters to set (%s, %s)",
                        name.c_str(), value.c_str());
        return -1;
}

extern "C" IPolicySet *
createSecurityManagerPs(IPCProcessComponent * ctx)
{
        ISecurityManager * sm = dynamic_cast<ISecurityManager *>(ctx);

        if (!sm) {
                return NULL;
        }

        return new SecurityManagerPs(sm);
}

extern "C" void
destroySecurityManagerPs(IPolicySet * ps)
{
        if (ps) {
                delete ps;
        }
}

}
