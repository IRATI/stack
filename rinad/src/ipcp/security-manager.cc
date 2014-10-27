//
// Security Manager
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

#define RINA_PREFIX "security-manager"

#include <librina/logs.h>

#include "ipcp/components.h"

namespace rinad {

//Class SecurityManager
SecurityManager::SecurityManager() {
	ipcp = 0;
        ps = 0;
}

void SecurityManager::set_ipc_process(IPCProcess * ipc_process)
{
	ipcp = ipc_process;
}

void SecurityManager::set_dif_configuration(const rina::DIFConfiguration& dif_configuration) {
	LOG_DBG("Set dif configuration: %u", dif_configuration.address_);
}

int SecurityManager::select_policy_set(const std::string& path,
                                       const std::string& name)
{
        IPolicySet *candidate = NULL;

        if (path != std::string()) {
                LOG_ERR("No subcomponents to address");
                return -1;
        }

        if (!ipcp) {
                LOG_ERR("bug: NULL ipcp reference");
                return -1;
        }

        if (name == selected_ps_name) {
                LOG_INFO("policy set %s already selected", name.c_str());
                return 0;
        }

        candidate = ipcp->psCreate("security-manager", name, this);
        if (!candidate) {
                LOG_ERR("failed to allocate instance of policy set %s", name.c_str());
                return -1;
        }

        if (ps) {
                // Remove the old one.
                ipcp->psDestroy("security-manager", selected_ps_name, ps);
        }

        // Install the new one.
        ps = dynamic_cast<ISecurityManagerPs *>(candidate);
        selected_ps_name = name;
        LOG_INFO("Policy-set %s selected", name.c_str());

        return ps ? 0 : -1;
}

int SecurityManager::set_policy_set_param(const std::string& path,
                                          const std::string& name,
                                          const std::string& value)
{
        LOG_DBG("set_policy_set_param(%s, %s) called",
                name.c_str(), value.c_str());

        if (!ipcp) {
                LOG_ERR("bug: NULL ipcp reference");
                return -1;
        }

        if (path == selected_ps_name) {
                // This request is for the currently selected
                // policy set, forward to it
                return ps->set_policy_set_param(name, value);
        } else if (path != std::string()) {
                LOG_ERR("Invalid component address '%s'", path.c_str());
                return -1;
        }

        // This request is for the Security Manager itself
        LOG_ERR("No such parameter '%s' exists", name.c_str());

        return -1;
}

}
