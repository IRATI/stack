//
// Default policy set for Resource Allocator
//
//    Eduard Grasa <eduard.grasa@i2cat.net>
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

#define RINA_PREFIX "resource-allocator-ps-default"

#include <librina/logs.h>
#include <string>

#include "ipcp/components.h"

namespace rinad {

class ResourceAllocatorPs: public IResourceAllocatorPs {
public:
		ResourceAllocatorPs(IResourceAllocator * ra);
		void routingTableUpdated(const std::list<RoutingTableEntry*>& routing_table);
		int set_policy_set_param(const std::string& name, const std::string& value);
		virtual ~ResourceAllocatorPs() {}

private:
        // Data model of the resource allocator component.
        IResourceAllocator * res_alloc;
};

ResourceAllocatorPs::ResourceAllocatorPs(IResourceAllocator * ra) : res_alloc(ra)
{ }


void ResourceAllocatorPs::routingTableUpdated(
		const std::list<RoutingTableEntry*>& rt)
{
	LOG_DBG("Got %d entries in the routing table", rt.size());
	//Compute PDU Forwarding Table
	std::list<rina::PDUForwardingTableEntry *> pduft;
	std::list<RoutingTableEntry *>::const_iterator it;
	rina::PDUForwardingTableEntry * entry;
	std::list<int> flows;
	for (it = rt.begin(); it!= rt.end(); ++it){
		entry = new rina::PDUForwardingTableEntry();
		entry->address = (*it)->address;
		entry->qosId = (*it)->qosId;

		LOG_DBG("Processing entry for destination %u", (*it)->address);
		LOG_DBG("Next hop address %u", (*it)->nextHopAddresses.front());

		flows = res_alloc->get_n_minus_one_flow_manager()->
				getNMinusOneFlowsToNeighbour((*it)->nextHopAddresses.front());

		if (flows.size() == 0) {
			delete entry;
		} else {
			LOG_DBG("N-1 port-id: %u", flows.front());
			entry->portIds.push_back(flows.front());
			pduft.push_back(entry);
		}
	}

	try {
		rina::kernelIPCProcess->modifyPDUForwardingTableEntries(pduft, 2);
	} catch (Exception & e) {
		LOG_ERR("Error setting PDU Forwarding Table in the kernel: %s",
				e.what());
	}
}

int ResourceAllocatorPs::set_policy_set_param(const std::string& name,
                                            const std::string& value)
{
        LOG_DBG("No policy-set-specific parameters to set (%s, %s)",
                        name.c_str(), value.c_str());
        return -1;
}

extern "C" IPolicySet *
createResourceAllocatorPs(IPCProcessComponent * ctx)
{
		IResourceAllocator * ra = dynamic_cast<IResourceAllocator *>(ctx);

		if (!ra) {
			return NULL;
		}

		return new ResourceAllocatorPs(ra);
}

extern "C" void
destroyResourceAllocatorPs(IPolicySet * ps)
{
        if (ps) {
                delete ps;
        }
}

}
