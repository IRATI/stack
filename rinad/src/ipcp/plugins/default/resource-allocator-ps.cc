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
	void set_dif_configuration(const rina::DIFConfiguration& dif_configuration);
	virtual ~DefaultPDUFTGeneratorPs() {}

private:
	void parse_qosid_map_entry(const rina::PolicyParameter& param);

        // Data model of the resource allocator component.
        IResourceAllocator * res_alloc;

        // Stores qos-id to N-1 flow characteristics mappings
        std::map<int, rina::FlowSpecification> qosid_map;
};

DefaultPDUFTGeneratorPs::DefaultPDUFTGeneratorPs(IResourceAllocator * ra) : res_alloc(ra)
{ }

void DefaultPDUFTGeneratorPs::parse_qosid_map_entry(const rina::PolicyParameter& param)
{
	int qos_id, loss, delay;
	std::string eqos_id, edelay, eloss;
	rina::FlowSpecification fspec;

	eqos_id = param.name_.substr(0, param.name_.find("."));
	if (rina::string2int(eqos_id, qos_id)) {
		LOG_WARN("Could not parse qos_id: %s", eqos_id.c_str());
		return;
	}

	edelay = param.value_.substr(0, param.value_.find("/"));
	if (rina::string2int(edelay, delay)) {
		LOG_WARN("Could not parse delay: %s", edelay.c_str());
		return;
	}

	eloss = param.value_.substr(param.value_.find("/") +1);
	if (rina::string2int(eloss, loss)) {
		LOG_WARN("Could not parse loss: %s", eloss.c_str());
		return;
	}

	fspec.loss = loss;
	fspec.delay = delay;
	qosid_map[qos_id] = fspec;

	LOG_DBG("Added mapping of qos-id %d to N-1 flow with delay %d and loss %d",
		 qos_id, delay, loss);
}

void DefaultPDUFTGeneratorPs::set_dif_configuration(const rina::DIFConfiguration& dif_configuration)
{
	std::list<rina::PolicyParameter>::const_iterator it;
	rina::PolicyConfig psconf;

	psconf = dif_configuration.ra_configuration_.pduftg_conf_.policy_set_;
	for (it = psconf.parameters_.begin();
			it != psconf.parameters_.end(); ++it) {
		if (it->name_.find("qosid") != std::string::npos) {
			parse_qosid_map_entry(*it);
		}
	}
}

void DefaultPDUFTGeneratorPs::routingTableUpdated(const std::list<rina::RoutingTableEntry*>& rt)
{
	LOG_IPCP_DBG("Got %d entries in the routing table", rt.size());
	//Compute PDU Forwarding Table
	std::list<rina::PDUForwardingTableEntry *> pduft;
	std::list<rina::PDUForwardingTableEntry *>::iterator pfit;
	std::list<rina::PDUForwardingTableEntry *>::iterator pfjt;
	std::list<rina::RoutingTableEntry *>::const_iterator it;
	std::list<rina::NHopAltList>::const_iterator jt;
	std::list<rina::IPCPNameAddresses>::const_iterator kt;
	std::map<int, rina::FlowSpecification>::iterator qosmap_it;
	std::list<unsigned int>::iterator at;
	rina::PDUForwardingTableEntry * entry;
	rina::PDUForwardingTableEntry * candidate;
	bool increment = true;
	INMinusOneFlowManager * n1fm;
	int port_id = 0;

	n1fm = res_alloc->get_n_minus_one_flow_manager();

	for (it = rt.begin(); it!= rt.end(); it++) {
		for (at = (*it)->destination.addresses.begin(); at != (*it)->destination.addresses.end(); ++at) {
			if (qosid_map.size() == 0) {
				entry = new rina::PDUForwardingTableEntry();
				entry->address = *at;
				entry->qosId = (*it)->qosId;
				entry->cost = (*it)->cost;

				LOG_IPCP_DBG("Processing entry for destination %u and qos-id %u", *at, entry->qosId);

				for (jt = (*it)->nextHopNames.begin();
						jt != (*it)->nextHopNames.end(); jt++) {
					rina::PortIdAltlist portid_altlist;

					for (kt = jt->alts.begin();
							kt != jt->alts.end(); kt++) {
						if (kt->name != "") {
							port_id = n1fm->getManagementFlowToNeighbour(kt->name);
						} else {
							port_id = n1fm->getManagementFlowToNeighbour(kt->addresses.front());
						}
						if (port_id == -1)
							continue;

						LOG_IPCP_DBG("NHOP %s qos-id %u --> N-1 port-id: %u",
								kt->name.c_str(), entry->qosId, port_id);
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
			} else {
				for (qosmap_it = qosid_map.begin(); qosmap_it != qosid_map.end(); qosmap_it ++) {
					entry = new rina::PDUForwardingTableEntry();
					entry->address = *at;
					entry->qosId = qosmap_it->first;
					entry->cost = (*it)->cost;

					LOG_IPCP_DBG("Processing entry for destination %u and qos-id %u", *at, entry->qosId);

					for (jt = (*it)->nextHopNames.begin();
							jt != (*it)->nextHopNames.end(); jt++) {
						rina::PortIdAltlist portid_altlist;

						for (kt = jt->alts.begin();
								kt != jt->alts.end(); kt++) {
							port_id = n1fm->get_n1flow_to_neighbor(qosmap_it->second, kt->name);
							if (port_id == -1)
								port_id = n1fm->getManagementFlowToNeighbour(kt->name);

							LOG_IPCP_DBG("NHOP %s qos-id %u --> N-1 port-id: %u",
									kt->name.c_str(), entry->qosId, port_id);
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
			}
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
