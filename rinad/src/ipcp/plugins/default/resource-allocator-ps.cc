//
// Default policy set for Resource Allocator
//
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

#define IPCP_MODULE "resource-allocator-ps-default"
#include "../../ipcp-logging.h"

#include <string>

#include "ipcp/components.h"

namespace rinad {

class DefaultPDUFTGeneratorPs: public IPDUFTGeneratorPs {
public:
	DefaultPDUFTGeneratorPs(IResourceAllocator * ra);
	void routingTableUpdated(const std::list<rina::RoutingTableEntry*>& routing_table);
	int set_policy_set_param(const std::string& name, const std::string& value);
	virtual ~DefaultPDUFTGeneratorPs() {}

private:
        // Data model of the resource allocator component.
        IResourceAllocator * res_alloc;
};

DefaultPDUFTGeneratorPs::DefaultPDUFTGeneratorPs(IResourceAllocator * ra) : res_alloc(ra)
{ }

void DefaultPDUFTGeneratorPs::routingTableUpdated(const std::list<rina::RoutingTableEntry*>& rt)
{
	LOG_IPCP_DBG("Got %d entries in the routing table", rt.size());
	//Compute PDU Forwarding Table
	std::list<rina::PDUForwardingTableEntry *> pduft;
	std::list<rina::RoutingTableEntry *>::const_iterator it;
	std::list<rina::NHopAltList>::const_iterator jt;
	std::list<unsigned int>::const_iterator kt;
	rina::PDUForwardingTableEntry * entry;
	INMinusOneFlowManager * n1fm;
	int port_id = 0;

	n1fm = res_alloc->get_n_minus_one_flow_manager();

	for (it = rt.begin(); it!= rt.end(); it++) {
		entry = new rina::PDUForwardingTableEntry();
		entry->address = (*it)->address;
		entry->qosId = (*it)->qosId;

		LOG_IPCP_DBG("Processing entry for destination %u",
			     (*it)->address);

		for (jt = (*it)->nextHopAddresses.begin();
				jt != (*it)->nextHopAddresses.end(); jt++) {
			rina::PortIdAltlist portid_altlist;

			for (kt = jt->alts.begin();
					kt != jt->alts.end(); kt++) {
				port_id = n1fm->
					  getManagementFlowToNeighbour(*kt);
				if (port_id == -1)
					continue;

				LOG_IPCP_DBG("NHOP %u --> N-1 port-id: %u",
					     *kt, port_id);
				portid_altlist.add_alt(port_id);
			}

			if (portid_altlist.alts.size()) {
				entry->portIdAltlists.push_back(portid_altlist);
			}
		}

		if (entry->portIdAltlists.size()) {
			pduft.push_back(entry);
		} else {
			delete entry;
		}
	}

	try {
		rina::kernelIPCProcess->modifyPDUForwardingTableEntries(pduft, 2);
	} catch (rina::Exception & e) {
		LOG_IPCP_ERR("Error setting PDU Forwarding Table in the kernel: %s",
				e.what());
	}

	//Update resource allocator
	res_alloc->set_rt_entries(rt);
	res_alloc->set_pduft_entries(pduft);
}

int DefaultPDUFTGeneratorPs::set_policy_set_param(const std::string& name,
                                            	  const std::string& value)
{
        LOG_IPCP_DBG("No policy-set-specific parameters to set (%s, %s)",
                        name.c_str(), value.c_str());
        return -1;
}

extern "C" rina::IPolicySet *
createPDUFTGenPs(rina::ApplicationEntity * ctx)
{
	IResourceAllocator * ra = dynamic_cast<IResourceAllocator *>(ctx);

	if (!ra) {
		return NULL;
	}

	return new DefaultPDUFTGeneratorPs(ra);
}

extern "C" void
destroyPDUFTGenPs(rina::IPolicySet * ps)
{
        if (ps) {
                delete ps;
        }
}

}
