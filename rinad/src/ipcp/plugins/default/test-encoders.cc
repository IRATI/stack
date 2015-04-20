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

#define RINA_PREFIX "encoders-tests"

#include <librina/logs.h>

#include <librina/configuration.h>
#include "common/encoder.h"
#include "routing-ps.h"

bool test_flow_state_object(rinad::Encoder * encoder) {
	rinad::FlowStateObject fso = rinad::FlowStateObject(23, 84, 2, true, 123, 450);
	rinad::FlowStateObject * recovered_obj = 0;

	rina::CDAPMessage cdapMessage = rina::CDAPMessage();
	cdapMessage.obj_class_ = rinad::EncoderConstants::FLOW_STATE_OBJECT_RIB_OBJECT_CLASS;

	encoder->encode(&fso, &cdapMessage);

	recovered_obj = (rinad::FlowStateObject *) encoder->decode(&cdapMessage);

	if (fso.address_ != recovered_obj->address_) {
		LOG_ERR("Addresses are different; original: %u, recovered: %u",
				fso.address_, recovered_obj->address_);
		return false;
	}

	if (fso.neighbor_address_ != recovered_obj->neighbor_address_) {
		LOG_ERR("Neighbor addresses are different; original: %u, recovered: %u",
				fso.neighbor_address_, recovered_obj->neighbor_address_);
		return false;
	}

	if (fso.cost_ != recovered_obj->cost_) {
		LOG_ERR("Costs are different; original: %u, recovered: %u",
				fso.cost_, recovered_obj->cost_);
		return false;
	}

	if (fso.sequence_number_ != recovered_obj->sequence_number_) {
		LOG_ERR("Sequence numbers are different; original: %d, recovered: %d",
				fso.sequence_number_, recovered_obj->sequence_number_);
		return false;
	}

	if (fso.up_ != recovered_obj->up_) {
		LOG_ERR("States are different; original: %d, recovered: %d",
				fso.up_, recovered_obj->up_);
		return false;
	}

	if (fso.age_ != recovered_obj->age_) {
		LOG_ERR("Ages are different; original: %u, recovered: %u",
				fso.age_, recovered_obj->age_);
		return false;
	}

	delete recovered_obj;

	LOG_INFO("Flow State Object Encoder tested successfully");
	return true;
}

bool test_flow_state_object_list(rinad::Encoder * encoder) {
	std::list<rinad::FlowStateObject *> fso_list;
	rinad::FlowStateObject fso1 = rinad::FlowStateObject(23, 1, 2, true, 123, 434);
	rinad::FlowStateObject fso2 = rinad::FlowStateObject(34, 2, 1, true, 223, 434);
	std::list<rinad::FlowStateObject*> * recovered_obj = 0;

	fso_list.push_back(&fso1);
	fso_list.push_back(&fso2);

	rina::CDAPMessage cdapMessage = rina::CDAPMessage();
	cdapMessage.obj_class_ = rinad::EncoderConstants::FLOW_STATE_OBJECT_GROUP_RIB_OBJECT_CLASS;

	encoder->encode(&fso_list, &cdapMessage);

	recovered_obj = (std::list<rinad::FlowStateObject*> *) encoder->decode(&cdapMessage);

	if (fso_list.size() != recovered_obj->size()) {
		return false;
	}

	delete recovered_obj;

	LOG_INFO("Flow State Object List Encoder tested successfully");
	return true;
}

int main()
{
	rinad::Encoder encoder;
	encoder.addEncoder(rinad::EncoderConstants::FLOW_STATE_OBJECT_GROUP_RIB_OBJECT_CLASS,
			new rinad::FlowStateObjectListEncoder());
	encoder.addEncoder(rinad::EncoderConstants::FLOW_STATE_OBJECT_RIB_OBJECT_CLASS,
			new rinad::FlowStateObjectEncoder());

	bool result = test_flow_state_object(&encoder);
	if (!result) {
		LOG_ERR("Problems testing Flow State Object Encoder");
		return -1;
	}

	result = test_flow_state_object_list(&encoder);
	if (!result) {
		LOG_ERR("Problems testing Flow State Object List Encoder");
		return -1;
	}

	return 0;
}
