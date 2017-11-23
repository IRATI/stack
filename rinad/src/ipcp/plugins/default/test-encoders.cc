//
// test-encoders
//
//    Bernat Gaston <bernat.gaston@i2cat.net>
//    Eduard Grasa  <eduard.grasa@i2cat.net>
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

#include <list>
#include <iostream>

#define IPCP_MODULE "encoders-tests"
#include "../../ipcp-logging.h"

#include <librina/configuration.h>
#include "common/encoder.h"
#include "routing-ps.h"

int ipcp_id = 1;

bool test_flow_state_object()
{
	rinad::FlowStateObject fso("test1.IRATI", "test2.IRATI", 2, true, 123, 450);
	fso.add_address(23);
	fso.add_address(2300);
	fso.add_neighboraddress(18);
	fso.add_neighboraddress(1800);

	LOG_IPCP_INFO("%s", fso.toString().c_str());

	rinad::FlowStateObject recovered_obj;
	rinad::FlowStateObjectEncoder encoder;

	rina::ser_obj_t encoded_obj;
	encoder.encode(fso, encoded_obj);
	encoder.decode(encoded_obj, recovered_obj);

	if (fso.name != recovered_obj.name) {
		LOG_IPCP_ERR("Names are different; original: %s, recovered: %s",
			     fso.name.c_str(),
			     recovered_obj.name.c_str());
		return false;
	}

	if (fso.neighbor_name != recovered_obj.neighbor_name) {
		LOG_IPCP_ERR("Neighbor names are different; original: %s, recovered: %s",
			     fso.neighbor_name.c_str(),
			     recovered_obj.neighbor_name.c_str());
		return false;
	}

	if (fso.addresses.size() != recovered_obj.addresses.size()) {
		LOG_IPCP_ERR("Address sizes are different");
		return false;
	}

	if (!fso.contains_address(23) || !fso.contains_address(2300)) {
		LOG_IPCP_ERR("Recovered object doesn't contain the right addresses");
		return false;
	}

	if (fso.neighbor_addresses.size() != recovered_obj.neighbor_addresses.size()) {
		LOG_IPCP_ERR("Neighbor address sizes are different");
		return false;
	}

	if (!fso.contains_neighboraddress(18) || !fso.contains_neighboraddress(1800)) {
		LOG_IPCP_ERR("Recovered object doesn't contain the right neighbor addresses");
		return false;
	}

	if (fso.cost != recovered_obj.cost) {
		LOG_IPCP_ERR("Costs are different; original: %u, recovered: %u",
			     fso.cost, recovered_obj.cost);
		return false;
	}

	if (fso.seq_num != recovered_obj.seq_num) {
		LOG_IPCP_ERR("Sequence numbers are different; original: %d, recovered: %d",
		             fso.seq_num, recovered_obj.seq_num);
		return false;
	}

	if (fso.state_up != recovered_obj.state_up) {
		LOG_IPCP_ERR("States are different; original: %d, recovered: %d",
			     fso.state_up, recovered_obj.state_up);
		return false;
	}

	if (fso.age != recovered_obj.age) {
		LOG_IPCP_ERR("Ages are different; original: %u, recovered: %u",
			     fso.age, recovered_obj.age);
		return false;
	}

	LOG_IPCP_INFO("Flow State Object Encoder tested successfully");
	return true;
}

bool test_flow_state_object_list()
{
	std::list<rinad::FlowStateObject> fso_list;
	rinad::FlowStateObject fso1("aa", "bb", 2, true, 123, 434);
	fso1.add_address(17);
	fso1.add_address(1700);
	fso1.add_neighboraddress(23);
	fso1.add_neighboraddress(2300);
	rinad::FlowStateObject fso2("cc", "dd", 1, true, 223, 434);
	fso2.add_address(19);
	fso2.add_address(1900);
	fso2.add_neighboraddress(25);
	fso2.add_neighboraddress(2500);
	std::list<rinad::FlowStateObject> recovered_obj;
	rinad::FlowStateObjectListEncoder encoder;
	rina::ser_obj_t encoded_obj;

	fso_list.push_back(fso1);
	fso_list.push_back(fso2);

	encoder.encode(fso_list, encoded_obj);
	encoder.decode(encoded_obj, recovered_obj);

	if (fso_list.size() != recovered_obj.size()) {
		return false;
	}

	LOG_IPCP_INFO("Flow State Object List Encoder tested successfully");
	return true;
}

int main()
{
	bool result = test_flow_state_object();
	if (!result) {
		LOG_IPCP_ERR("Problems testing Flow State Object Encoder");
		return -1;
	}

	result = test_flow_state_object_list();
	if (!result) {
		LOG_IPCP_ERR("Problems testing Flow State Object List Encoder");
		return -1;
	}

	return 0;
}
