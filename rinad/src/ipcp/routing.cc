//
// Routing component
//
//    Eduard Grasa <eduard.grasa@i2cat.net>
//    Vincenzo Maffione <v.maffione@nextworks.it>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301  USA
//

#include <assert.h>

#define IPCP_MODULE "routing"
#include "ipcp-logging.h"

#include "ipcp/components.h"

namespace rinad {

//Class RoutingComponent
void RoutingComponent::set_application_process(rina::ApplicationProcess * ap)
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

void RoutingComponent::set_dif_configuration(const rina::DIFInformation& dif_information)
{
	std::string ps_name = dif_information.dif_configuration_.routing_configuration_.policy_set_.name_;
	if (select_policy_set(std::string(), ps_name) != 0) {
		throw rina::Exception("Cannot create Routing policy-set");
	}

	IRoutingPs *rps = dynamic_cast<IRoutingPs *> (ps);
	assert(rps);
	rps->set_dif_configuration(dif_information.dif_configuration_);
}

RoutingComponent::~RoutingComponent()
{
	delete ps;
}

} //namespace rinad
