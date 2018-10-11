//
// Address change policy set for Namespace Manager
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

#define IPCP_MODULE "namespace-manager-ps-address-change"

#include "../../ipcp-logging.h"
#include <string>
#include <stdlib.h>

#include "ipcp/components.h"

namespace rinad {

class AddressChangeNamespaceManagerPs: public INamespaceManagerPs {
public:
	AddressChangeNamespaceManagerPs(INamespaceManager * nsm);
	bool isValidAddress(unsigned int address,
			    const std::string& ipcp_name,
			    const std::string& ipcp_instance);
	unsigned int getValidAddress(const std::string& ipcp_name,
				     const std::string& ipcp_instance);
	int set_policy_set_param(const std::string& name,
				 const std::string& value);
	void set_dif_configuration(const rina::DIFConfiguration& dif_configuration);
	virtual ~AddressChangeNamespaceManagerPs() {}
	void change_address(void);

	static const std::string POLICY_PARAM_USE_NEW_TIMEOUT;
	static const std::string POLICY_PARAM_DEPRECATE_OLD_TIMEOUT;
	static const std::string POLICY_PARAM_CHANGE_PERIOD;
	static const std::string POLICY_PARAM_ADDRESS_RANGE;
	static const std::string POLICY_PARAM_START_DELAY;

private:
    	unsigned int getIPCProcessAddress(const std::string& process_name,
    			const std::string& process_instance,
    			const rina::AddressingConfiguration& address_conf);
    	bool isAddressInUse(unsigned int address, const std::string& ipcp_name);

    	// Data model of the namespace manager component.
    	INamespaceManager * nsm;
    	rina::Timer timer;
    	unsigned int use_new_timeout;
    	unsigned int deprecate_old_timeout;
    	unsigned int change_period;
    	unsigned int min_address;
    	unsigned int max_address;
    	unsigned int current_address;
    	unsigned int start_delay;
    	bool first_time;
};

class NSMChangeAddressTimerTask : public rina::TimerTask {
public:
	NSMChangeAddressTimerTask(AddressChangeNamespaceManagerPs *ps);
	~NSMChangeAddressTimerTask() throw(){};
	void run();
	std::string name() const {
		return "nsm-change-address";
	}

private:
	AddressChangeNamespaceManagerPs* nsm_ps;
};

NSMChangeAddressTimerTask::NSMChangeAddressTimerTask(AddressChangeNamespaceManagerPs *ps)
{
	nsm_ps = ps;
}

void NSMChangeAddressTimerTask::run()
{
	nsm_ps->change_address();
}

const std::string AddressChangeNamespaceManagerPs::POLICY_PARAM_USE_NEW_TIMEOUT = "useNewTimeout";
const std::string AddressChangeNamespaceManagerPs::POLICY_PARAM_DEPRECATE_OLD_TIMEOUT = "deprecateOldTimeout";
const std::string AddressChangeNamespaceManagerPs::POLICY_PARAM_CHANGE_PERIOD = "changePeriod";
const std::string AddressChangeNamespaceManagerPs::POLICY_PARAM_ADDRESS_RANGE = "addressRange";
const std::string AddressChangeNamespaceManagerPs::POLICY_PARAM_START_DELAY = "startDelay";

AddressChangeNamespaceManagerPs::AddressChangeNamespaceManagerPs(INamespaceManager * nsm_) : nsm(nsm_)
{
	use_new_timeout = 0;
	deprecate_old_timeout = 0;
	change_period = 0;
	min_address = 0;
	max_address = 0;
	current_address = 0;
	start_delay = 0;
	first_time = true;
}

void AddressChangeNamespaceManagerPs::set_dif_configuration(const rina::DIFConfiguration& dif_configuration)
{
	NSMChangeAddressTimerTask * task;
	long delay = 0;
	unsigned int address_range = 0;

	try {
		current_address = getIPCProcessAddress(nsm->ipcp->get_name(),
						       nsm->ipcp->get_instance(),
						       dif_configuration.nsm_configuration_.addressing_configuration_);
		use_new_timeout = dif_configuration.nsm_configuration_.policy_set_.
				get_param_value_as_uint(POLICY_PARAM_USE_NEW_TIMEOUT);
		deprecate_old_timeout = dif_configuration.nsm_configuration_.policy_set_.
				get_param_value_as_uint(POLICY_PARAM_DEPRECATE_OLD_TIMEOUT);
		change_period = dif_configuration.nsm_configuration_.policy_set_.
				get_param_value_as_uint(POLICY_PARAM_CHANGE_PERIOD);
		address_range = dif_configuration.nsm_configuration_.policy_set_.
				get_param_value_as_uint(POLICY_PARAM_ADDRESS_RANGE);
		start_delay = dif_configuration.nsm_configuration_.policy_set_.
				get_param_value_as_uint(POLICY_PARAM_START_DELAY);
		min_address = current_address * address_range;
		max_address = min_address + address_range - 1;
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Could not properly initialize address change NSM policy, will not change addresses: %s",
			      e.what());
		return;
	}

	LOG_IPCP_DBG("Use new timeout (ms): %u", use_new_timeout);
	LOG_IPCP_DBG("Deprecate old timeout (ms): %u", deprecate_old_timeout);
	LOG_IPCP_DBG("Change period (ms): %u", change_period);
	LOG_IPCP_DBG("Minimum address: %u", min_address);
	LOG_IPCP_DBG("Maximum address: %u", max_address);
	LOG_IPCP_DBG("Start delay: %u", start_delay);

	task = new NSMChangeAddressTimerTask(this);
	delay = start_delay + deprecate_old_timeout + use_new_timeout + (rand() % (change_period + 1));
	LOG_IPCP_DBG("Will update address again in %ld ms", delay);
	timer.scheduleTask(task, delay);
}

void AddressChangeNamespaceManagerPs::change_address(void)
{
	NSMChangeAddressTimerTask * task;
	unsigned int new_address = 0;
	long delay = 0;

	// 1 Compute new address
	if (current_address + 1 > max_address || first_time) {
		new_address = min_address;
		first_time = false;
	} else {
		new_address = current_address + 1;
	}

	//2 Deliver address changed event
	LOG_IPCP_INFO("Computed new address: %u, disseminating the event!", new_address);
	rina::AddressChangeEvent * event = new rina::AddressChangeEvent(new_address,
									current_address,
									use_new_timeout,
									deprecate_old_timeout);
	current_address = new_address;

	nsm->ipcp->internal_event_manager_->deliverEvent(event);

	//3 Re-schedule timer
	task = new NSMChangeAddressTimerTask(this);
	delay = deprecate_old_timeout + use_new_timeout + (rand() % (change_period + 1));
	LOG_IPCP_DBG("Will update address again in %ld ms", delay);
	timer.scheduleTask(task, delay);
}

bool AddressChangeNamespaceManagerPs::isValidAddress(unsigned int address,
						     const std::string& ipcp_name,
						     const std::string& ipcp_instance)
{
	if (address == 0)
		return false;

	return true;
}

unsigned int AddressChangeNamespaceManagerPs::getValidAddress(const std::string& ipcp_name,
							      const std::string& ipcp_instance)
{
	rina::AddressingConfiguration configuration = nsm->ipcp->get_dif_information().
			dif_configuration_.nsm_configuration_.addressing_configuration_;
	unsigned int candidateAddress = getIPCProcessAddress(ipcp_name,
			ipcp_instance, configuration);
	if (candidateAddress != 0) {
		return candidateAddress;
	}

	return 0;
}

unsigned int AddressChangeNamespaceManagerPs::getIPCProcessAddress(const std::string& process_name,
								  const std::string& process_instance,
								  const rina::AddressingConfiguration& address_conf)
{
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

bool AddressChangeNamespaceManagerPs::isAddressInUse(unsigned int address,
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

int AddressChangeNamespaceManagerPs::set_policy_set_param(const std::string& name,
                                            	    	  const std::string& value)
{
        LOG_IPCP_DBG("No policy-set-specific parameters to set (%s, %s)",
                        name.c_str(), value.c_str());
        return -1;
}

extern "C" rina::IPolicySet *
createAddressChangeNamespaceManagerPs(rina::ApplicationEntity * ctx)
{
		INamespaceManager * nsm = dynamic_cast<INamespaceManager *>(ctx);

		if (!nsm) {
			return NULL;
		}

		return new AddressChangeNamespaceManagerPs(nsm);
}

extern "C" void
destroyAddressChangeNamespaceManagerPs(rina::IPolicySet * ps)
{
        if (ps) {
                delete ps;
        }
}

extern "C" int
get_factories(std::vector<struct rina::PsFactory>& factories)
{
	struct rina::PsFactory factory;
	int ret;

	factory.info.name = "address-change";
	factory.info.app_entity = INamespaceManager::NAMESPACE_MANAGER_AE_NAME;
	factory.create = createAddressChangeNamespaceManagerPs;
	factory.destroy = destroyAddressChangeNamespaceManagerPs;
	factories.push_back(factory);

	return 0;
}

}
