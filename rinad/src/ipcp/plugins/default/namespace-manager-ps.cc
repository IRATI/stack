//
// Defaul policy set for Namespace Manager
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

#define IPCP_MODULE "namespace-manager-ps-default"

#include "../../ipcp-logging.h"
#include <string>

#include "ipcp/components.h"

namespace rinad {

class NamespaceManagerPs: public INamespaceManagerPs {
public:
	NamespaceManagerPs(INamespaceManager * nsm);
	bool isValidAddress(unsigned int address,
			    const std::string& ipcp_name,
			    const std::string& ipcp_instance);
	unsigned int getValidAddress(const std::string& ipcp_name,
				     const std::string& ipcp_instance);
	int set_policy_set_param(const std::string& name,
				 const std::string& value);
	void set_dif_configuration(const rina::DIFConfiguration& dif_configuration);
	virtual ~NamespaceManagerPs() {}

private:
        // Data model of the namespace manager component.
        INamespaceManager * nsm;
    	unsigned int getIPCProcessAddress(const std::string& process_name,
    			const std::string& process_instance,
    			const rina::AddressingConfiguration& address_conf);
    	unsigned int getAddressPrefix(const std::string& process_name,
    				const rina::AddressingConfiguration& address_conf);
    	bool isAddressInUse(unsigned int address, const std::string& ipcp_name);
};

NamespaceManagerPs::NamespaceManagerPs(INamespaceManager * nsm_) : nsm(nsm_)
{ }

void NamespaceManagerPs::set_dif_configuration(const rina::DIFConfiguration& dif_configuration)
{ }

bool NamespaceManagerPs::isValidAddress(unsigned int address,
					const std::string& ipcp_name,
					const std::string& ipcp_instance)
{
	if (address == 0) {
		return false;
	}

	//Check if we know the remote IPC Process address
	rina::AddressingConfiguration configuration = nsm->ipcp->get_dif_information().
			dif_configuration_.nsm_configuration_.addressing_configuration_;
	unsigned int knownAddress = getIPCProcessAddress(ipcp_name, ipcp_instance, configuration);
	if (knownAddress != 0) {
		if (address == knownAddress) {
			return true;
		} else {
			return false;
		}
	}

	//Check the prefix information
	try {
		unsigned int prefix = getAddressPrefix(ipcp_name, configuration);

		//Check if the address is within the range of the prefix
		if (address < prefix || address >= prefix + rina::AddressPrefixConfiguration::MAX_ADDRESSES_PER_PREFIX){
			return false;
		}
	} catch (rina::Exception &e) {
		//We don't know the organization of the IPC Process
		return false;
	}

	return !isAddressInUse(address, ipcp_name);
}

unsigned int NamespaceManagerPs::getValidAddress(const std::string& ipcp_name,
						 const std::string& ipcp_instance)
{
	rina::AddressingConfiguration configuration = nsm->ipcp->get_dif_information().
			dif_configuration_.nsm_configuration_.addressing_configuration_;
	unsigned int candidateAddress = getIPCProcessAddress(ipcp_name,
			ipcp_instance, configuration);
	if (candidateAddress != 0) {
		return candidateAddress;
	}

	unsigned int prefix = 0;

	try {
		prefix = getAddressPrefix(ipcp_name, configuration);
	} catch (rina::Exception &e) {
		//We don't know the organization of the IPC Process
		return 0;
	}

	candidateAddress = prefix;
	while (candidateAddress < prefix +
			rina::AddressPrefixConfiguration::MAX_ADDRESSES_PER_PREFIX) {
		if (isAddressInUse(candidateAddress, ipcp_name)) {
			candidateAddress++;
		} else {
			return candidateAddress;
		}
	}

	return 0;
}

unsigned int NamespaceManagerPs::getIPCProcessAddress(const std::string& process_name,
			const std::string& process_instance, const rina::AddressingConfiguration& address_conf) {
	std::list<rina::StaticIPCProcessAddress>::const_iterator it;
	for (it = address_conf.static_address_.begin();
			it != address_conf.static_address_.end(); ++it) {
		if (it->ap_name_.compare(process_name) == 0 &&
				it->ap_instance_.compare(process_instance) == 0) {
			return it->address_;
		}
	}

	return 0;

}

unsigned int NamespaceManagerPs::getAddressPrefix(const std::string& process_name,
		const rina::AddressingConfiguration& address_conf) {
	std::list<rina::AddressPrefixConfiguration>::const_iterator it;
	for (it = address_conf.address_prefixes_.begin();
			it != address_conf.address_prefixes_.end(); ++it) {
		if (process_name.find(it->organization_) != std::string::npos) {
			return it->address_prefix_;
		}
	}

	throw rina::Exception("Unknown organization");
}

bool NamespaceManagerPs::isAddressInUse(unsigned int address,
		const std::string& ipcp_name) {
	std::list<rina::Neighbor> neighbors = nsm->ipcp->get_neighbors();

	for (std::list<rina::Neighbor>::const_iterator it = neighbors.begin();
		it != neighbors.end(); ++it) {
		if (it->address_ == address) {
			if (it->name_.processName.compare(ipcp_name) == 0) {
				return false;
			} else {
				return true;
			}
		}
	}

	return false;
}

int NamespaceManagerPs::set_policy_set_param(const std::string& name,
                                            const std::string& value)
{
        LOG_IPCP_DBG("No policy-set-specific parameters to set (%s, %s)",
                        name.c_str(), value.c_str());
        return -1;
}

// Topological address assignment: an enroller hands out addresses under its own
// prefix, so that whoever attaches to a gNB gets an address the rest of the DIF already
// reaches through the gNB's aggregate. IPC processes listed in knownIPCProcessAddresses
// keep their static address, as with the default policy set.
//
// Parameter: prefixLength, the length in bits of the prefix an enroller assigns from
// (e.g. 20 for [region 8 | node 12 | host 12]).
class TopologicalNamespaceManagerPs: public INamespaceManagerPs {
public:
	TopologicalNamespaceManagerPs(INamespaceManager * nsm) : nsm(nsm), prefix_len(32) {}
	bool isValidAddress(unsigned int address,
			    const std::string& ipcp_name,
			    const std::string& ipcp_instance);
	unsigned int getValidAddress(const std::string& ipcp_name,
				     const std::string& ipcp_instance);
	int set_policy_set_param(const std::string& name,
				 const std::string& value);
	void set_dif_configuration(const rina::DIFConfiguration& dif_configuration);
	virtual ~TopologicalNamespaceManagerPs() {}

private:
	INamespaceManager * nsm;
	unsigned int prefix_len;
	unsigned int mask() const
	{ return prefix_len == 0 ? 0 : (prefix_len >= 32 ? ~0U : ~0U << (32 - prefix_len)); }
	unsigned int static_address(const std::string& name, const std::string& instance);
	bool in_use(unsigned int address, const std::string& ipcp_name);
};

void TopologicalNamespaceManagerPs::set_dif_configuration(const rina::DIFConfiguration& dc)
{
	std::list<rina::PolicyParameter>::const_iterator p;

	for (p = dc.nsm_configuration_.policy_set_.parameters_.begin();
	     p != dc.nsm_configuration_.policy_set_.parameters_.end(); ++p)
		set_policy_set_param(p->name_, p->value_);
}

int TopologicalNamespaceManagerPs::set_policy_set_param(const std::string& name,
						     const std::string& value)
{
	if (name == "prefixLength") {
		unsigned int len = strtoul(value.c_str(), NULL, 10);
		if (len > 32) {
			LOG_IPCP_ERR("Bad prefixLength %s", value.c_str());
			return -1;
		}
		prefix_len = len;
		LOG_IPCP_INFO("Topological addressing, prefix length %u", prefix_len);
		return 0;
	}
	LOG_IPCP_DBG("Unknown parameter %s", name.c_str());
	return -1;
}

unsigned int TopologicalNamespaceManagerPs::static_address(const std::string& name,
							    const std::string& instance)
{
	const rina::AddressingConfiguration& conf = nsm->ipcp->get_dif_information().
			dif_configuration_.nsm_configuration_.addressing_configuration_;
	std::list<rina::StaticIPCProcessAddress>::const_iterator it;

	for (it = conf.static_address_.begin(); it != conf.static_address_.end(); ++it) {
		if (it->ap_name_ == name && it->ap_instance_ == instance)
			return it->address_;
	}
	return 0;
}

// In use by a neighbour other than ipcp_name, or by this IPC process itself.
bool TopologicalNamespaceManagerPs::in_use(unsigned int address,
					   const std::string& ipcp_name)
{
	std::list<rina::Neighbor> neighbors = nsm->ipcp->get_neighbors();
	std::list<rina::Neighbor>::const_iterator it;

	if (address == nsm->ipcp->get_dif_information().dif_configuration_.address_)
		return true;
	for (it = neighbors.begin(); it != neighbors.end(); ++it) {
		if (it->address_ == address && it->name_.processName != ipcp_name)
			return true;
	}
	return false;
}

bool TopologicalNamespaceManagerPs::isValidAddress(unsigned int address,
						   const std::string& ipcp_name,
						   const std::string& ipcp_instance)
{
	unsigned int known = static_address(ipcp_name, ipcp_instance);
	unsigned int own = nsm->ipcp->get_dif_information().dif_configuration_.address_;

	if (address == 0)
		return false;
	if (known != 0)
		return address == known;
	// Anything else must sit under this IPC process's prefix.
	if ((address & mask()) != (own & mask()))
		return false;
	return !in_use(address, ipcp_name);
}

unsigned int TopologicalNamespaceManagerPs::getValidAddress(const std::string& ipcp_name,
							    const std::string& ipcp_instance)
{
	unsigned int known = static_address(ipcp_name, ipcp_instance);
	unsigned int own = nsm->ipcp->get_dif_information().dif_configuration_.address_;
	unsigned int base, size, host;
	std::list<rina::Neighbor> neighbors;
	std::list<rina::Neighbor>::const_iterator it;

	if (known != 0)
		return known;
	if (own == 0 || prefix_len == 0 || prefix_len >= 32)
		return 0;

	// A neighbour that already holds an address under the prefix keeps it.
	neighbors = nsm->ipcp->get_neighbors();
	for (it = neighbors.begin(); it != neighbors.end(); ++it) {
		if (it->name_.processName == ipcp_name && it->address_ != 0 &&
		    (it->address_ & mask()) == (own & mask()))
			return it->address_;
	}

	base = own & mask();
	size = prefix_len == 0 ? 0 : 1U << (32 - prefix_len);
	for (host = 1; host < size; host++) {
		if (!in_use(base | host, ipcp_name))
			return base | host;
	}
	LOG_IPCP_ERR("No free address under prefix %u/%u", base, prefix_len);
	return 0;
}

extern "C" rina::IPolicySet *
createTopologicalNamespaceManagerPs(rina::ApplicationEntity * ctx)
{
	INamespaceManager * nsm = dynamic_cast<INamespaceManager *>(ctx);

	if (!nsm)
		return NULL;
	return new TopologicalNamespaceManagerPs(nsm);
}

extern "C" void
destroyTopologicalNamespaceManagerPs(rina::IPolicySet * ps)
{
	delete ps;
}

extern "C" rina::IPolicySet *
createNamespaceManagerPs(rina::ApplicationEntity * ctx)
{
		INamespaceManager * nsm = dynamic_cast<INamespaceManager *>(ctx);

		if (!nsm) {
			return NULL;
		}

		return new NamespaceManagerPs(nsm);
}

extern "C" void
destroyNamespaceManagerPs(rina::IPolicySet * ps)
{
        if (ps) {
                delete ps;
        }
}

}
