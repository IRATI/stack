//
// Static routing policy set
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

#define IPCP_MODULE "routing-ps-static"
#include "../../ipcp-logging.h"
#include <string>
#include <climits>

#include "ipcp/components.h"

namespace rinad {

class StaticRoutingPs: public IRoutingPs, public rina::InternalEventListener {
public:
	static const std::string DEFAULT_NEXT_HOP;
	enum ParamType {DEFAULT, RANGE, SINGLE};

	StaticRoutingPs(IRoutingComponent * dm);
	virtual ~StaticRoutingPs() {};
	void set_dif_configuration(const rina::DIFConfiguration& dif_configuration);
	int set_policy_set_param(const std::string& name,
				 const std::string& value);
	void eventHappened(rina::InternalEvent * event);

private:
	void subscribeToEvents(void);
	void parse_policy_param(rina::PolicyParameter pm,
				const rina::DIFConfiguration& dif_configuration);
	void split(std::vector<std::string> & result,
	           const char *str, char c);
	void get_rt_entries_as_list(std::list<rina::RoutingTableEntry *> & result);
	void update_forwarding_table(void);

        // Data model of the security manager component.
	IRoutingComponent * rc;
	std::map<std::string, rina::RoutingTableEntry *> rt_entries;
	rina::Lockable lock;
};

const std::string StaticRoutingPs::DEFAULT_NEXT_HOP = "defaultNextHop";

StaticRoutingPs::StaticRoutingPs(IRoutingComponent * dm) {
	rc = dm;
}

void StaticRoutingPs::subscribeToEvents()
{
	rc->ipcp->internal_event_manager_->
		subscribeToEvent(rina::InternalEvent::APP_N_MINUS_1_FLOW_DEALLOCATED, this);
	rc->ipcp->internal_event_manager_->
		subscribeToEvent(rina::InternalEvent::APP_N_MINUS_1_FLOW_ALLOCATED, this);
	rc->ipcp->internal_event_manager_->
		subscribeToEvent(rina::InternalEvent::APP_NEIGHBOR_ADDED, this);
	rc->ipcp->internal_event_manager_->
		subscribeToEvent(rina::InternalEvent::APP_CONNECTIVITY_TO_NEIGHBOR_LOST, this);
}

void StaticRoutingPs::update_forwarding_table(void)
{
	std::list<rina::RoutingTableEntry *> current_entries;

	get_rt_entries_as_list(current_entries);
	rc->ipcp->resource_allocator_->pduft_gen_ps->routingTableUpdated(current_entries);
}

void StaticRoutingPs::eventHappened(rina::InternalEvent * event)
{
	if (!event)
		return;

	rina::ScopedLock g(lock);

	update_forwarding_table();
}

int StaticRoutingPs::set_policy_set_param(const std::string& name,
			 	 	  const std::string& value)
{
	LOG_INFO("Ignoring set policy set parm, name: %s, value: %s",
		  name.c_str(), value.c_str());
	return 0;
}

void StaticRoutingPs::set_dif_configuration(const rina::DIFConfiguration& dif_configuration)
{
	rina::PolicyConfig psconf;
	std::list<rina::PolicyParameter>::const_iterator it;

	rina::ScopedLock g(lock);

	subscribeToEvents();

	psconf = dif_configuration.routing_configuration_.policy_set_;
	for (it = psconf.parameters_.begin();
			it != psconf.parameters_.end(); ++it) {
		parse_policy_param(*it, dif_configuration);
	}

	update_forwarding_table();
}

//Parameter name is the destination address,
//Parameter value is <qos_id>-<cost>-<nhop_address>
void StaticRoutingPs::parse_policy_param(rina::PolicyParameter pm,
					 const rina::DIFConfiguration& dif_configuration)
{
	rina::RoutingTableEntry * entry;
	std::stringstream ss;
	unsigned int qos_id, cost, address, range_start, range_end;
	char *dummy;
	std::vector<std::string> result;
	std::vector<std::string> range;
	rina::NHopAltList nhop_alt;
	rina::IPCPNameAddresses ipcpna;
	ParamType param_type;
	std::list<rina::StaticIPCProcessAddress> addresses;
	std::list<rina::StaticIPCProcessAddress>::iterator it;
	unsigned int counter;

	if (pm.name_.c_str() == DEFAULT_NEXT_HOP) {
		param_type = DEFAULT;
	} else {
		split(range, pm.name_.c_str(), '-');
		if (range.size() == 2) {
			param_type = RANGE;
		} else {
			param_type = SINGLE;
		}
	}

	//Tokenize parameter value
	split(result, pm.value_.c_str(), '-');
	if (result.size() != 3) {
		LOG_ERR("Parameter value does not have the right format: %s",
				pm.value_.c_str());
	}

	//Parse qos_id
	qos_id = strtoul(result[0].c_str(), &dummy, 10);
	if (!result[0].c_str() || *dummy != '\0') {
		LOG_ERR("Error converting qos-id to ulong: %s",
				result[0].c_str());
		return;
	}

	//Parse cost
	cost = strtoul(result[1].c_str(), &dummy, 10);
	if (!result[1].c_str() || *dummy != '\0') {
		LOG_ERR("Error converting cost to ulong: %s",
				result[1].c_str());
		return;
	}

	//Parse nhop address
	address = strtoul(result[2].c_str(), &dummy, 10);
	if (!result[1].c_str() || *dummy != '\0') {
		LOG_ERR("Error converting nhop address to ulong: %s",
				result[2].c_str());
		return;
	}
	ipcpna.addresses.push_back(address);
	nhop_alt.alts.push_back(ipcpna);

	switch (param_type) {
	case SINGLE:
		address = strtoul(pm.name_.c_str(), &dummy, 10);
		if (!pm.name_.size() || *dummy != '\0') {
			LOG_ERR("Error converting dest. address to ulong: %s",
					pm.name_.c_str());
			return;
		}
		entry = new rina::RoutingTableEntry();
		entry->qosId = qos_id;
		entry->cost = cost;
		entry->destination.addresses.push_back(address);
		entry->nextHopNames.push_back(nhop_alt);
		ss << pm.name_ << "-" << pm.value_;
		rt_entries[ss.str()] = entry;
		break;
	case RANGE:
		range_start = strtoul(range[0].c_str(), &dummy, 10);
		if (!range[0].size() || *dummy != '\0') {
			LOG_ERR("Error converting range start to ulong: %s",
					pm.name_.c_str());
			return;
		}

		range_end = strtoul(range[1].c_str(), &dummy, 10);
		if (!range[1].size() || *dummy != '\0') {
			LOG_ERR("Error converting range end to ulong: %s",
					pm.name_.c_str());
			return;
		}

		for(counter = range_start; counter <= range_end; counter ++ ) {
			entry = new rina::RoutingTableEntry();
			entry->qosId = qos_id;
			entry->cost = cost;
			entry->destination.addresses.push_back(counter);
			entry->nextHopNames.push_back(nhop_alt);
			ss << counter << "-" << pm.value_;
			rt_entries[ss.str()] = entry;
		}
		break;
	case DEFAULT:
		addresses = dif_configuration.nsm_configuration_.addressing_configuration_.static_address_;
		for (it = addresses.begin(); it != addresses.end(); ++it) {
			if (it->address_ == dif_configuration.address_)
				continue;

			entry = new rina::RoutingTableEntry();
			entry->qosId = qos_id;
			entry->cost = cost;
			entry->destination.addresses.push_back(it->address_);
			entry->nextHopNames.push_back(nhop_alt);
			ss << it->address_ << "-" << pm.value_;
			rt_entries[ss.str()] = entry;
		}
		break;
	default:
		LOG_ERR("Wrong state, cannot parse param");

	}
}

void StaticRoutingPs::split(std::vector<std::string> & result,
			    const char *str, char c)
{
	do
	{
		const char *begin = str;

		while(*str != c && *str)
			str++;

		result.push_back(std::string(begin, str));
	} while (0 != *str++);
}

void StaticRoutingPs::get_rt_entries_as_list(std::list<rina::RoutingTableEntry *> & result)
{
	std::map<std::string, rina::RoutingTableEntry *>::iterator it;
	rina::RoutingTableEntry * rte;

	for (it = rt_entries.begin(); it != rt_entries.end(); ++it) {
		rte = new rina::RoutingTableEntry();
		rte->cost = it->second->cost;
		rte->qosId = it->second->qosId;
		rte->destination = it->second->destination;
		rte->nextHopNames = it->second->nextHopNames;
		result.push_back(rte);
	}
}

extern "C" rina::IPolicySet *
createStaticRoutingComponentPs(rina::ApplicationEntity * ctx)
{
	IRoutingComponent * rc = dynamic_cast<IRoutingComponent *>(ctx);

	if (!rc) {
		return NULL;
	}

	return new StaticRoutingPs(rc);
}

extern "C" void
destroyStaticRoutingComponentPs(rina::IPolicySet * ps)
{
        if (ps) {
                delete ps;
        }
}

}
