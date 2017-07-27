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
	StaticRoutingPs(IRoutingComponent * dm);
	virtual ~StaticRoutingPs() {};
	void set_dif_configuration(const rina::DIFConfiguration& dif_configuration);
	int set_policy_set_param(const std::string& name,
				 const std::string& value);
	void eventHappened(rina::InternalEvent * event);

private:
	void subscribeToEvents(void);
	void parse_policy_param(rina::PolicyParameter pm);
	void split(std::vector<std::string> & result,
	           const char *str, char c);
	void get_rt_entries_as_list(std::list<rina::RoutingTableEntry *> & result);
	void update_forwarding_table(void);

        // Data model of the security manager component.
	IRoutingComponent * rc;
	std::map<std::string, rina::RoutingTableEntry *> rt_entries;
	rina::Lockable lock;
};

StaticRoutingPs::StaticRoutingPs(IRoutingComponent * dm) {
	rc = dm;
}

void StaticRoutingPs::subscribeToEvents()
{
	rc->ipcp->internal_event_manager_->
		subscribeToEvent(rina::InternalEvent::APP_N_MINUS_1_FLOW_DEALLOCATED, this);
	rc->ipcp->internal_event_manager_->
		subscribeToEvent(rina::InternalEvent::APP_N_MINUS_1_FLOW_ALLOCATED, this);
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
		parse_policy_param(*it);
	}

	update_forwarding_table();
}

//Parameter name is the destination address,
//Parameter value is <qos_id>-<cost>-<nhop_address>
void StaticRoutingPs::parse_policy_param(rina::PolicyParameter pm)
{
	rina::RoutingTableEntry * entry;
	std::stringstream ss;
	unsigned int aux;
	char *dummy;
	std::vector<std::string> result;
	rina::NHopAltList nhop_alt;
	rina::IPCPNameAddresses ipcpna;

	//Tokenize parameter value
	split(result, pm.value_.c_str(), '-');
	if (result.size() != 3) {
		LOG_ERR("Parameter value does not have the right format: %s",
				pm.value_.c_str());
	}

	//Parse destination address
	entry = new rina::RoutingTableEntry();
	aux = strtoul(pm.name_.c_str(), &dummy, 10);
	if (!pm.name_.size() || *dummy != '\0') {
		LOG_ERR("Error converting dest. address to ulong: %s",
				pm.name_.c_str());
		delete entry;
		return;
	}
	entry->destination.addresses.push_back(aux);

	//Parse qos_id
	entry->qosId = strtoul(result[0].c_str(), &dummy, 10);
	if (!result[0].c_str() || *dummy != '\0') {
		LOG_ERR("Error converting qos-id to ulong: %s",
				result[0].c_str());
		delete entry;
		return;
	}

	//Parse cost
	entry->cost = strtoul(result[1].c_str(), &dummy, 10);
	if (!result[1].c_str() || *dummy != '\0') {
		LOG_ERR("Error converting cost to ulong: %s",
				result[1].c_str());
		delete entry;
		return;
	}

	//Parse nhop address
	aux = strtoul(result[2].c_str(), &dummy, 10);
	if (!result[1].c_str() || *dummy != '\0') {
		LOG_ERR("Error converting nhop address to ulong: %s",
				result[2].c_str());
		delete entry;
		return;
	}
	ipcpna.addresses.push_back(aux);
	nhop_alt.alts.push_back(ipcpna);
	entry->nextHopNames.push_back(nhop_alt);

	ss << pm.name_ << "-" << pm.value_;
	rt_entries[ss.str()] = entry;
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

	for (it = rt_entries.begin(); it != rt_entries.end(); ++it) {
		result.push_back(it->second);
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
