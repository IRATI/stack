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
	rinad::FlowStateObject fso(23, 84, 2, true, 123, 450);
	rinad::FlowStateObject recovered_obj;
	rinad::FlowStateObjectEncoder encoder;

	rina::ser_obj_t encoded_obj;

	encoder.encode(fso, encoded_obj);
	encoder.decode(encoded_obj, recovered_obj);

	if (fso.get_address() != recovered_obj.get_address()) {
		LOG_IPCP_ERR("Addresses are different; original: %u, recovered: %u",
			     fso.get_address(),
			     recovered_obj.get_address());
		return false;
	}

	if (fso.get_neighboraddress() != recovered_obj.get_neighboraddress()) {
		LOG_IPCP_ERR("Neighbor addresses are different; original: %u, recovered: %u",
			     fso.get_neighboraddress(),
			     recovered_obj.get_neighboraddress());
		return false;
	}

	if (fso.get_cost() != recovered_obj.get_cost()) {
		LOG_IPCP_ERR("Costs are different; original: %u, recovered: %u",
			     fso.get_cost(),
			     recovered_obj.get_cost());
		return false;
	}

	if (fso.get_sequencenumber() != recovered_obj.get_sequencenumber()) {
		LOG_IPCP_ERR("Sequence numbers are different; original: %d, recovered: %d",
		             fso.get_sequencenumber(),
		             recovered_obj.get_sequencenumber());
		return false;
	}

	if (fso.is_state() != recovered_obj.is_state()) {
		LOG_IPCP_ERR("States are different; original: %d, recovered: %d",
			     fso.is_state(),
			     recovered_obj.is_state());
		return false;
	}

	if (fso.get_age() != recovered_obj.get_age()) {
		LOG_IPCP_ERR("Ages are different; original: %u, recovered: %u",
			     fso.get_age(),
			     recovered_obj.get_age());
		return false;
	}

	LOG_IPCP_INFO("Flow State Object Encoder tested successfully");
	return true;
}

bool test_flow_state_object_list()
{
	std::list<rinad::FlowStateObject> fso_list;
	rinad::FlowStateObject fso1(23, 1, 2, true, 123, 434);
	rinad::FlowStateObject fso2(34, 2, 1, true, 223, 434);
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
