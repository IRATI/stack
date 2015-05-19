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

#define IPCP_MODULE "security-manager"
#include "ipcp-logging.h"

#include "ipcp/components.h"

namespace rinad {

//Class SecurityManager
void IPCPSecurityManager::set_application_process(rina::ApplicationProcess * ap)
{
	if (!ap)
			return;

	app = ap;
	ipcp = dynamic_cast<IPCProcess*>(app);
	if (!ipcp) {
			LOG_IPCP_ERR("Bogus instance of IPCP passed, return");
			return;
	}
}

void IPCPSecurityManager::set_dif_configuration(const rina::DIFConfiguration& dif_configuration)
{
	config = dif_configuration.sm_configuration_;
	LOG_IPCP_DBG("Security configuration: %s", config.toString().c_str());
}

rina::AuthSDUProtectionProfile IPCPSecurityManager::get_auth_sdup_profile(const std::string& under_dif_name)
{
	std::map<std::string, rina::AuthSDUProtectionProfile>::const_iterator it =
			config.specific_auth_profiles.find(under_dif_name);
	if (it == config.specific_auth_profiles.end()) {
		return config.default_auth_profile;
	} else {
		return it->second;
	}
}

}
