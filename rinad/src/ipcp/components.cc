//
// Common interfaces and constants of the IPC Process components
//
//    Bernat Gaston <bernat.gaston@i2cat.net>
//    Eduard Grasa <eduard.grasa@i2cat.net>
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

#include <sstream>
#include <iostream>

#define IPCP_MODULE "components"
#include "ipcp-logging.h"
#include "components.h"

namespace rinad {

const std::string IResourceAllocator::RESOURCE_ALLOCATOR_AE_NAME = "resource-allocator";
const std::string IResourceAllocator::PDUFT_GEN_COMPONENT_NAME = "pduft-generator";
const std::string INamespaceManager::NAMESPACE_MANAGER_AE_NAME = "namespace-manager";
const std::string IRoutingComponent::ROUTING_COMPONENT_AE_NAME = "routing";
const std::string IFlowAllocator::FLOW_ALLOCATOR_AE_NAME = "flow-allocator";

// Class IResourceAllocator
int IResourceAllocator::set_pduft_gen_policy_set(const std::string& name)
{
	if (!ipcp) {
		LOG_IPCP_ERR("IPCP is null");
		return -1;
	}

	if (pduft_gen_ps) {
		LOG_IPCP_ERR("PDUFT Generator policy set already present");
		return -1;
	}

	std::stringstream ss;
	ss << RESOURCE_ALLOCATOR_AE_NAME;
        pduft_gen_ps = (IPDUFTGeneratorPs *) ipcp->psCreate(ss.str(),
        					     	    name,
        					     	    this);
        if (!pduft_gen_ps) {
                LOG_IPCP_ERR("failed to allocate instance of policy set %s",
                	     name.c_str());
                return -1;
        }

        LOG_INFO("PDUFT Generator policy-set %s added to the resource-allocator",
                 name.c_str());
        return 0;
}

// Class BaseRIBObject
IPCPRIBObj::IPCPRIBObj(IPCProcess * ipc_process,
		       const std::string& object_class):
				       rina::rib::RIBObj(object_class)
{
	ipc_process_ = ipc_process;
	if (ipc_process)
		rib_daemon_ =  ipc_process->rib_daemon_;
	else
		rib_daemon_ = 0;
}

// CLASS IPC Process
const std::string IPCProcess::MANAGEMENT_AE = "Management";
const std::string IPCProcess::DATA_TRANSFER_AE = "Data Transfer";
const int IPCProcess::DEFAULT_MAX_SDU_SIZE_IN_BYTES = 10000;

//Class IPCProcess
IPCProcess::IPCProcess(const std::string& name, const std::string& instance)
			: rina::ApplicationProcess(name, instance)
{
	delimiter_ = 0;
	internal_event_manager_ = 0;
	enrollment_task_ = 0;
	flow_allocator_ = 0;
	namespace_manager_ = 0;
	resource_allocator_ = 0;
	security_manager_ = 0;
	rib_daemon_ = 0;
	routing_component_ = 0;
}

} //namespace rinad
