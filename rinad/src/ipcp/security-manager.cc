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
void SecurityManager::set_application_process(rina::ApplicationProcess * ap)
{
	if (!ap)
			return;

	app = ap;
	ipcp = dynamic_cast<IPCProcess*>(app);
	if (!ipcp) {
			LOG_ERR("Bogus instance of IPCP passed, return");
			return;
	}
}

void SecurityManager::set_dif_configuration(const rina::DIFConfiguration& dif_configuration) {
	LOG_DBG("Set dif configuration: %u", dif_configuration.address_);
}

int SecurityManager::select_policy_set(const std::string& path,
                                       const std::string& name)
{
        return select_policy_set_common(get_name(), path, name);
}

int SecurityManager::set_policy_set_param(const std::string& path,
                                          const std::string& name,
                                          const std::string& value)
{
        return set_policy_set_param_common(path, name, value);
}

}
