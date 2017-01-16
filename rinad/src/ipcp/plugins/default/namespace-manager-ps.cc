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
