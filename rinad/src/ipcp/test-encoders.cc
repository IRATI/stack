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

#include "ipcp-logging.h"

#include <librina/configuration.h>
#include "common/encoder.h"
#include "ipcp/enrollment-task.h"
#include "ipcp/flow-allocator.h"

int ipcp_id = 1;

bool test_flow (rinad::Encoder * encoder) {
	rinad::Flow flow_to_encode;
	rinad::Flow *pflow_to_encode;
	std::list<rina::Connection*> connection_list;
	rina::Connection *pconnection_to_encode = new rina::Connection;
	rina::DTPConfig dtp_config_to_encode;
	rina::DTCPConfig dtcp_config_to_encode;
	rina::DTCPRtxControlConfig rtx_config;
	rina::DTCPFlowControlConfig flow_config_to_encode;
	rina::DTCPWindowBasedFlowControlConfig window_to_encode;
	rina::DTCPRateBasedFlowControlConfig rate_to_encode;

	// Set
	flow_to_encode.source_naming_info = rina::ApplicationProcessNamingInformation("test", "1");
	flow_to_encode.destination_naming_info = rina::ApplicationProcessNamingInformation("test2", "1");
	dtp_config_to_encode.set_dtcp_present(true);
	dtp_config_to_encode.set_seq_num_rollover_threshold(1234);
	dtp_config_to_encode.set_initial_a_timer(14561);
	dtp_config_to_encode.set_initial_seq_num_policy(rina::PolicyConfig("policy1", "23"));
	dtp_config_to_encode.set_dtp_policy_set(rina::PolicyConfig("policy2", "26"));
	dtcp_config_to_encode.set_dtcp_policy_set(rina::PolicyConfig("policy3", "27"));
	dtcp_config_to_encode.set_rtx_control(true);
	rtx_config.set_data_rxmsn_max(25423);
	rtx_config.set_initial_rtx_time(100);
	dtcp_config_to_encode.set_rtx_control_config(rtx_config);
	dtcp_config_to_encode.set_flow_control(true);
	flow_config_to_encode.set_rcv_buffers_threshold(412431);
	flow_config_to_encode.set_rcv_bytes_percent_threshold(134);
	flow_config_to_encode.set_rcv_bytes_threshold(46236);
	flow_config_to_encode.set_sent_buffers_threshold(94);
	flow_config_to_encode.set_sent_bytes_percent_threshold(2562);
	flow_config_to_encode.set_sent_bytes_threshold(26236);
	flow_config_to_encode.set_window_based(true);
	window_to_encode.set_initial_credit(62556);
	window_to_encode.set_max_closed_window_queue_length(5612623);
	flow_config_to_encode.set_window_based_config(window_to_encode);
	flow_config_to_encode.set_rate_based(true);
	rate_to_encode.set_sending_rate(45125);
	rate_to_encode.set_time_period(1451234);
	flow_config_to_encode.set_rate_based_config(rate_to_encode);
	dtcp_config_to_encode.set_flow_control_config(flow_config_to_encode);
	pconnection_to_encode->setDTPConfig(dtp_config_to_encode);
	pconnection_to_encode->setDTCPConfig(dtcp_config_to_encode);
	connection_list.push_front(pconnection_to_encode);
	flow_to_encode.connections = connection_list;

	// Encode
	pflow_to_encode = &flow_to_encode;
	rina::CDAPMessage cdapMessage = rina::CDAPMessage();
		cdapMessage.obj_class_ = rinad::EncoderConstants::FLOW_RIB_OBJECT_CLASS;
	encoder->encode((void*)pflow_to_encode, &cdapMessage);

	// Decode
	rinad::Flow *pflow_decoded = (rinad::Flow*) encoder->decode(&cdapMessage);

	// Assert
	if (pflow_to_encode->source_naming_info.processName != pflow_decoded->source_naming_info.processName)
		return false;
	if (pflow_to_encode->source_naming_info.processInstance != pflow_decoded->source_naming_info.processInstance)
		return false;

	rina::Connection *pconnection_decoded = pflow_decoded->connections.front();
	rina::DTPConfig dtp_config_decoded = pconnection_decoded->getDTPConfig();
	if ( dtp_config_to_encode.get_seq_num_rollover_threshold() != dtp_config_decoded.get_seq_num_rollover_threshold())
		return false;
	if ( dtp_config_to_encode.get_initial_a_timer() != dtp_config_decoded.get_initial_a_timer())
		return false;
	if ( dtp_config_to_encode.get_initial_seq_num_policy() != dtp_config_decoded.get_initial_seq_num_policy())
		return false;
	if ( dtp_config_to_encode.get_dtp_policy_set() != dtp_config_decoded.get_dtp_policy_set())
		return false;

	if ( dtp_config_to_encode.is_dtcp_present() != dtp_config_decoded.is_dtcp_present())
		return false;
	else {
		rina::DTCPConfig dtcp_config_decoded = pconnection_decoded->getDTCPConfig();
	        if (dtcp_config_to_encode.get_dtcp_policy_set() != dtcp_config_decoded.get_dtcp_policy_set())
		       return false;
		if(dtcp_config_to_encode.is_rtx_control() != dtcp_config_decoded.is_rtx_control())
			return false;
		if(dtcp_config_to_encode.get_rtx_control_config().get_data_rxmsn_max() != dtcp_config_decoded.get_rtx_control_config().get_data_rxmsn_max())
			return false;
		if(dtcp_config_to_encode.get_rtx_control_config().get_max_time_to_retry() != dtcp_config_decoded.get_rtx_control_config().get_max_time_to_retry())
			return false;
		if(dtcp_config_to_encode.get_rtx_control_config().get_initial_rtx_time() != dtcp_config_decoded.get_rtx_control_config().get_initial_rtx_time())
			return false;
		if(dtcp_config_to_encode.is_flow_control() != dtcp_config_decoded.is_flow_control())
			return false;
		else {
			rina::DTCPFlowControlConfig flow_config_decoded = dtcp_config_decoded.get_flow_control_config();
			if (flow_config_to_encode.get_rcv_buffers_threshold() != flow_config_decoded.get_rcv_buffers_threshold())
				return false;
			if (flow_config_to_encode.get_rcv_bytes_percent_threshold() != flow_config_decoded.get_rcv_bytes_percent_threshold())
				return false;
			if (flow_config_to_encode.get_rcv_bytes_threshold() != flow_config_decoded.get_rcv_bytes_threshold())
				return false;
			if (flow_config_to_encode.get_sent_buffers_threshold() != flow_config_decoded.get_sent_buffers_threshold())
				return false;
			if (flow_config_to_encode.get_sent_bytes_percent_threshold() != flow_config_decoded.get_sent_bytes_percent_threshold())
				return false;
			if (flow_config_to_encode.get_sent_bytes_threshold() != flow_config_decoded.get_sent_bytes_threshold())
				return false;

			if (flow_config_to_encode.is_window_based() != flow_config_decoded.is_window_based())
				return false;
			else{
				rina::DTCPWindowBasedFlowControlConfig window_decoded = flow_config_decoded.get_window_based_config();
				if (window_to_encode.get_initial_credit() != window_decoded.get_initial_credit())
					return false;
				if (window_to_encode.get_maxclosed_window_queue_length() != window_decoded.get_maxclosed_window_queue_length())
					return false;
			}

			if (flow_config_to_encode.is_rate_based() != flow_config_decoded.is_rate_based())
				return false;
			else {
				rina::DTCPRateBasedFlowControlConfig rate_decoded = flow_config_decoded.get_rate_based_config();
				if (rate_to_encode.get_sending_rate() != rate_decoded.get_sending_rate())
					return false;
				if (rate_to_encode.get_time_period() != rate_decoded.get_time_period())
					return false;
			}
		}
	}

	LOG_IPCP_INFO("Flow Encoder tested successfully");

	delete pflow_decoded;

	return true;
}

bool test_data_transfer_constants(rinad::Encoder * encoder) {
	rina::DataTransferConstants dtc;
	rina::DataTransferConstants * recovered_obj = 0;

	dtc.address_length_ = 23;
	dtc.cep_id_length_ = 15;
	dtc.dif_integrity_ = true;
	dtc.length_length_ = 12;
	dtc.max_pdu_lifetime_ = 45243;
	dtc.max_pdu_size_ = 1233;
	dtc.port_id_length_ = 541;
	dtc.qos_id_length_ = 3414;
	dtc.sequence_number_length_ = 123;

	rina::CDAPMessage cdapMessage = rina::CDAPMessage();
	cdapMessage.obj_class_ = rinad::EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_CLASS;
	encoder->encode(&dtc, &cdapMessage);
	recovered_obj = (rina::DataTransferConstants*) encoder->decode(&cdapMessage);

	if (dtc.address_length_ != recovered_obj->address_length_) {
		return false;
	}

	if (dtc.cep_id_length_ != recovered_obj->cep_id_length_) {
		LOG_IPCP_DBG("Aqui");
		return false;
	}

	if (dtc.dif_integrity_ != recovered_obj->dif_integrity_) {
		return false;
	}

	if (dtc.length_length_ != recovered_obj->length_length_) {
		return false;
	}

	if (dtc.max_pdu_lifetime_ != recovered_obj->max_pdu_lifetime_) {
		return false;
	}

	if (dtc.max_pdu_size_ != recovered_obj->max_pdu_size_) {
		return false;
	}

	if (dtc.port_id_length_ != recovered_obj->port_id_length_) {
		return false;
	}

	if (dtc.qos_id_length_ != recovered_obj->qos_id_length_) {
		return false;
	}

	if (dtc.sequence_number_length_ != recovered_obj->sequence_number_length_) {
		return false;
	}

	delete recovered_obj;

	LOG_IPCP_INFO("Data Transfer Constants Encoder tested successfully");

	return true;
}

bool test_directory_forwarding_table_entry(rinad::Encoder * encoder) {
	rina::DirectoryForwardingTableEntry dfte;
	rina::DirectoryForwardingTableEntry * recovered_obj = 0;

	dfte.address_ = 232;
	dfte.timestamp_ = 5265235;
	dfte.ap_naming_info_.processName = "test";
	dfte.ap_naming_info_.processInstance = "1";
	dfte.ap_naming_info_.entityName = "ae";
	dfte.ap_naming_info_.entityInstance = "1";

	rina::CDAPMessage cdapMessage = rina::CDAPMessage();
	cdapMessage.obj_class_ = rinad::EncoderConstants::DFT_ENTRY_RIB_OBJECT_CLASS;

	encoder->encode(&dfte, &cdapMessage);
	recovered_obj = (rina::DirectoryForwardingTableEntry*) encoder->decode(&cdapMessage);

	if (dfte.address_ != recovered_obj->address_) {
		return false;
	}

	if (dfte.timestamp_ != recovered_obj->timestamp_) {
		return false;
	}

	if (dfte.ap_naming_info_.processName.compare(
			recovered_obj->ap_naming_info_.processName) != 0) {
		return false;
	}

	if (dfte.ap_naming_info_.processInstance.compare(
			recovered_obj->ap_naming_info_.processInstance) != 0) {
		return false;
	}

	if (dfte.ap_naming_info_.entityName.compare(
			recovered_obj->ap_naming_info_.entityName) != 0) {
		return false;
	}

	if (dfte.ap_naming_info_.entityInstance.compare(
			recovered_obj->ap_naming_info_.entityInstance) != 0) {
		return false;
	}

	delete recovered_obj;

	LOG_IPCP_INFO("Directory Forwarding Table Entry Encoder tested successfully");
	return true;
}

bool test_directory_forwarding_table_entry_list(rinad::Encoder * encoder) {
	std::list<rina::DirectoryForwardingTableEntry*> dfte_list;
	rina::DirectoryForwardingTableEntry dfte1;
	rina::DirectoryForwardingTableEntry dfte2;
	std::list<rina::DirectoryForwardingTableEntry*> * recovered_obj = 0;

	dfte1.address_ = 232;
	dfte1.timestamp_ = 5265235;
	dfte1.ap_naming_info_.processName = "test";
	dfte1.ap_naming_info_.processInstance = "1";
	dfte1.ap_naming_info_.entityName = "ae";
	dfte1.ap_naming_info_.entityInstance = "1";
	dfte_list.push_back(&dfte1);
	dfte2.address_ = 2312;
	dfte2.timestamp_ = 52265235;
	dfte2.ap_naming_info_.processName = "atest";
	dfte2.ap_naming_info_.processInstance = "2";
	dfte2.ap_naming_info_.entityName = "adfs";
	dfte2.ap_naming_info_.entityInstance = "2";
	dfte_list.push_back(&dfte2);

	rina::CDAPMessage cdapMessage = rina::CDAPMessage();
	cdapMessage.obj_class_ = rinad::EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_CLASS;

	encoder->encode(&dfte_list, &cdapMessage);

	recovered_obj = (std::list<rina::DirectoryForwardingTableEntry*> *) encoder->decode(&cdapMessage);

	if (dfte_list.size() != recovered_obj->size()) {
		return false;
	}

	delete recovered_obj;

	LOG_IPCP_INFO("Directory Forwarding Table Entry List Encoder tested successfully");
	return true;
}

bool test_enrollment_information_request(rinad::Encoder * encoder) {
	rinad::EnrollmentInformationRequest request;
	rina::ApplicationProcessNamingInformation name1;
	rina::ApplicationProcessNamingInformation name2;
	rinad::EnrollmentInformationRequest * recovered_obj = 0;

	name1.processName = "dif1";
	name2.processName = "dif2";
	request.supporting_difs_.push_back(name1);
	request.supporting_difs_.push_back(name2);
	request.address_ = 141234;


	rina::CDAPMessage cdapMessage = rina::CDAPMessage();
	cdapMessage.obj_class_ = rinad::EncoderConstants::ENROLLMENT_INFO_OBJECT_CLASS;

	encoder->encode(&request, &cdapMessage);

	recovered_obj = (rinad::EnrollmentInformationRequest *) encoder->decode(&cdapMessage);

	if (request.address_ != recovered_obj->address_) {
		return false;
	}

	if (request.supporting_difs_.size() != recovered_obj->supporting_difs_.size()) {
		return false;
	}

	delete recovered_obj;

	LOG_IPCP_INFO("Enrollment Information Request Encoder tested successfully");
	return true;
}

bool test_qos_cube(rinad::Encoder * encoder) {
	rina::QoSCube cube;
	rina::QoSCube * recovered_obj = 0;

	cube.average_bandwidth_ = 12134;
	cube.average_sdu_bandwidth_ = 3141234;
	cube.delay_ = 314;
	cube.jitter_ = 252345;
	cube.id_ = 12;
	cube.max_allowable_gap_ = 1243;
	cube.name_ = "test";
	cube.ordered_delivery_ = true;
	cube.partial_delivery_ = true;
	cube.peak_bandwidth_duration_ = 1443;
	cube.peak_sdu_bandwidth_duration_ = 1341;
	cube.undetected_bit_error_rate_ = 2;
	cube.dtp_config_.dtcp_present_ = true;
	cube.dtp_config_.initial_a_timer_ = 23;
	cube.dtp_config_.initial_seq_num_policy_.name_ = "test-policy";

	rina::CDAPMessage cdapMessage = rina::CDAPMessage();
	cdapMessage.obj_class_ = rinad::EncoderConstants::QOS_CUBE_RIB_OBJECT_CLASS;

	encoder->encode(&cube, &cdapMessage);

	recovered_obj = (rina::QoSCube *) encoder->decode(&cdapMessage);

	if (cube.average_bandwidth_ != recovered_obj->average_bandwidth_) {
		return false;
	}

	if (cube.average_sdu_bandwidth_ != recovered_obj->average_sdu_bandwidth_) {
		return false;
	}

	if (cube.delay_ != recovered_obj->delay_) {
		return false;
	}

	if (cube.jitter_ != recovered_obj->jitter_) {
		return false;
	}

	if (cube.id_ != recovered_obj->id_) {
		return false;
	}

	if (cube.max_allowable_gap_ != recovered_obj->max_allowable_gap_) {
		return false;
	}

	if (cube.name_.compare(recovered_obj->name_)!= 0 ){
		return false;
	}

	if (cube.ordered_delivery_ != recovered_obj->ordered_delivery_) {
		return false;
	}

	if (cube.partial_delivery_ != recovered_obj->partial_delivery_) {
		return false;
	}

	if (cube.peak_bandwidth_duration_ != recovered_obj->peak_bandwidth_duration_) {
		return false;
	}

	if (cube.peak_sdu_bandwidth_duration_ != recovered_obj->peak_sdu_bandwidth_duration_) {
		return false;
	}

	if (cube.undetected_bit_error_rate_ != recovered_obj->undetected_bit_error_rate_) {
		return false;
	}

	if (cube.dtp_config_.dtcp_present_ != recovered_obj->dtp_config_.dtcp_present_) {
		return false;
	}

	if (cube.dtp_config_.initial_a_timer_ != recovered_obj->dtp_config_.initial_a_timer_) {
		return false;
	}

	if (cube.dtp_config_.initial_seq_num_policy_.name_.compare(
			recovered_obj->dtp_config_.initial_seq_num_policy_.name_) != 0) {
		return false;
	}

	delete recovered_obj;

	LOG_IPCP_INFO("QoS Cube Encoder tested successfully");
	return true;
}

bool test_qos_cube_list(rinad::Encoder * encoder) {
	std::list<rina::QoSCube*> cube_list;
	rina::QoSCube cube1;
	rina::QoSCube cube2;
	std::list<rina::QoSCube*> * recovered_obj = 0;

	cube_list.push_back(&cube1);
	cube_list.push_back(&cube2);

	rina::CDAPMessage cdapMessage = rina::CDAPMessage();
	cdapMessage.obj_class_ = rinad::EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_CLASS;

	encoder->encode(&cube_list, &cdapMessage);

	recovered_obj = (std::list<rina::QoSCube*> *) encoder->decode(&cdapMessage);

	if (cube_list.size() != recovered_obj->size()) {
		return false;
	}

	delete recovered_obj;

	LOG_IPCP_INFO("QoS Cube List Encoder tested successfully");
	return true;
}

bool test_whatevercast_name(rinad::Encoder * encoder) {
	rina::WhatevercastName name;
	rina::WhatevercastName * recovered_obj = 0;

	name.name_ = "all members";
	name.rule_ = "fancy rule";
	name.set_members_.push_back("member1");
	name.set_members_.push_back("member2");

	rina::CDAPMessage cdapMessage = rina::CDAPMessage();
	cdapMessage.obj_class_ = rinad::EncoderConstants::WHATEVERCAST_NAME_RIB_OBJECT_CLASS;

	encoder->encode(&name, &cdapMessage);

	recovered_obj = (rina::WhatevercastName *) encoder->decode(&cdapMessage);

	if (name.name_.compare(recovered_obj->name_) != 0) {
		return false;
	}

	if (name.rule_.compare(recovered_obj->rule_) != 0) {
		return false;
	}

	if (name.set_members_.size() != recovered_obj->set_members_.size()) {
		return false;
	}

	delete recovered_obj;

	LOG_IPCP_INFO("Whatevercast Name Encoder tested successfully");
	return true;
}

bool test_whatevercast_name_list(rinad::Encoder * encoder) {
	std::list<rina::WhatevercastName*> name_list;
	rina::WhatevercastName name1;
	rina::WhatevercastName name2;
	std::list<rina::WhatevercastName*> * recovered_obj = 0;

	name_list.push_back(&name1);
	name_list.push_back(&name2);

	rina::CDAPMessage cdapMessage = rina::CDAPMessage();
	cdapMessage.obj_class_ = rinad::EncoderConstants::WHATEVERCAST_NAME_SET_RIB_OBJECT_CLASS;

	encoder->encode(&name_list, &cdapMessage);

	recovered_obj = (std::list<rina::WhatevercastName*> *) encoder->decode(&cdapMessage);

	if (name_list.size() != recovered_obj->size()) {
		return false;
	}

	delete recovered_obj;

	LOG_IPCP_INFO("Whatevercast Name List Encoder tested successfully");
	return true;
}

bool test_neighbor(rinad::Encoder * encoder) {
	rina::Neighbor nei;
	rina::Neighbor * recovered_obj = 0;

	nei.name_.processName = "test1.IRATI",
	nei.name_.processInstance = "1";
	nei.address_ = 25;
	nei.supporting_difs_.push_back(rina::ApplicationProcessNamingInformation("test2.IRATI", "1"));
	nei.supporting_difs_.push_back(rina::ApplicationProcessNamingInformation("Castefa.i2CAT", "1"));

	rina::CDAPMessage cdapMessage = rina::CDAPMessage();
	cdapMessage.obj_class_ = rina::NeighborSetRIBObject::NEIGHBOR_RIB_OBJECT_CLASS;

	encoder->encode(&nei, &cdapMessage);

	recovered_obj = (rina::Neighbor *) encoder->decode(&cdapMessage);

	if (nei.name_.processName.compare(recovered_obj->name_.processName) != 0) {
		return false;
	}

	if (nei.name_.processInstance.compare(recovered_obj->name_.processInstance) != 0) {
		return false;
	}

	if (nei.address_ != recovered_obj->address_) {
		return false;
	}

	if (nei.supporting_difs_.size() != recovered_obj->supporting_difs_.size()) {
		return false;
	}

	delete recovered_obj;

	LOG_IPCP_INFO("Neighbor Encoder tested successfully");
	return true;
}

bool test_neighbor_list(rinad::Encoder * encoder) {
	std::list<rina::Neighbor*> nei_list;
	rina::Neighbor nei1;
	rina::Neighbor nei2;
	std::list<rina::Neighbor*> * recovered_obj = 0;

	nei_list.push_back(&nei1);
	nei_list.push_back(&nei2);

	rina::CDAPMessage cdapMessage = rina::CDAPMessage();
	cdapMessage.obj_class_ = rina::NeighborSetRIBObject::NEIGHBOR_SET_RIB_OBJECT_CLASS;

	encoder->encode(&nei_list, &cdapMessage);

	recovered_obj = (std::list<rina::Neighbor*> *) encoder->decode(&cdapMessage);

	if (nei_list.size() != recovered_obj->size()) {
		return false;
	}

	delete recovered_obj;

	LOG_IPCP_INFO("Neighbor List Encoder tested successfully");
	return true;
}

bool test_watchdog(rinad::Encoder * encoder) {
	int address = 23;
	int * recovered_obj = 0;

	rina::CDAPMessage cdapMessage = rina::CDAPMessage();
	cdapMessage.obj_class_ = rinad::EncoderConstants::WATCHDOG_RIB_OBJECT_CLASS;

	rina::IntObjectValue * int_value = new rina::IntObjectValue(address);
	cdapMessage.obj_value_ = int_value;

	recovered_obj = (int*) encoder->decode(&cdapMessage);

	if (*recovered_obj != address ) {
		return false;
	}

	delete recovered_obj;

	LOG_IPCP_INFO("Watchdog Encoder tested successfully");
	return true;
}

int main()
{
	rinad::Encoder encoder;
	encoder.addEncoder(rinad::EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_CLASS,
			new rinad::DataTransferConstantsEncoder());
	encoder.addEncoder(rinad::EncoderConstants::DFT_ENTRY_RIB_OBJECT_CLASS,
			new rinad::DirectoryForwardingTableEntryEncoder());
	encoder.addEncoder(rinad::EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_CLASS,
			new rinad::DirectoryForwardingTableEntryListEncoder());
	encoder.addEncoder(rinad::EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_CLASS,
			new rinad::DirectoryForwardingTableEntryListEncoder());
	encoder.addEncoder(rinad::EncoderConstants::ENROLLMENT_INFO_OBJECT_CLASS,
			new rinad::EnrollmentInformationRequestEncoder());
	encoder.addEncoder(rinad::EncoderConstants::FLOW_RIB_OBJECT_CLASS,
			new rinad::FlowEncoder());
	encoder.addEncoder(rina::NeighborSetRIBObject::NEIGHBOR_RIB_OBJECT_CLASS,
			new rinad::NeighborEncoder());
	encoder.addEncoder(rina::NeighborSetRIBObject::NEIGHBOR_SET_RIB_OBJECT_CLASS,
			new rinad::NeighborListEncoder());
	encoder.addEncoder(rinad::EncoderConstants::QOS_CUBE_RIB_OBJECT_CLASS,
			new rinad::QoSCubeEncoder());
	encoder.addEncoder(rinad::EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_CLASS,
			new rinad::QoSCubeListEncoder());
	encoder.addEncoder(rinad::EncoderConstants::WHATEVERCAST_NAME_RIB_OBJECT_CLASS,
			new rinad::WhatevercastNameEncoder());
	encoder.addEncoder(rinad::EncoderConstants::WHATEVERCAST_NAME_SET_RIB_OBJECT_CLASS,
			new rinad::WhatevercastNameListEncoder());
	encoder.addEncoder(rinad::EncoderConstants::WATCHDOG_RIB_OBJECT_CLASS,
			new rinad::WatchdogEncoder());

	bool result = test_data_transfer_constants(&encoder);
	if (!result) {
		LOG_IPCP_ERR("Problems testing Data Transfer Constants Encoder");
		return -1;
	}

	result = test_directory_forwarding_table_entry(&encoder);
	if (!result) {
		LOG_IPCP_ERR("Problems testing Directory Forwarding Table Entry Encoder");
		return -1;
	}

	result = test_directory_forwarding_table_entry_list(&encoder);
	if (!result) {
		LOG_IPCP_ERR("Problems testing Directory Forwarding Table Entry List Encoder");
		return -1;
	}

	result = test_enrollment_information_request(&encoder);
	if (!result) {
		LOG_IPCP_ERR("Problems testing Enrollment Information Request Encoder");
		return -1;
	}

	result = test_flow(&encoder);
	if (!result) {
		LOG_IPCP_ERR("Problems testing Flow Encoder");
		return -1;
	}

	result = test_neighbor(&encoder);
	if (!result) {
		LOG_IPCP_ERR("Problems testing Neighbor Encoder");
		return -1;
	}

	result = test_neighbor_list(&encoder);
	if (!result) {
		LOG_IPCP_ERR("Problems testing Neighbor List Encoder");
		return -1;
	}

	result = test_qos_cube(&encoder);
	if (!result) {
		LOG_IPCP_ERR("Problems testing QoS Cube Encoder");
		return -1;
	}

	result = test_qos_cube_list(&encoder);
	if (!result) {
		LOG_IPCP_ERR("Problems testing QoS Cube List Encoder");
		return -1;
	}

	result = test_whatevercast_name(&encoder);
	if (!result) {
		LOG_IPCP_ERR("Problems testing Whatevercast Name Encoder");
		return -1;
	}

	result = test_whatevercast_name_list(&encoder);
	if (!result) {
		LOG_IPCP_ERR("Problems testing Whatevercast Name List Encoder");
		return -1;
	}

	result = test_watchdog(&encoder);
	if (!result) {
		LOG_IPCP_ERR("Problems testing Watchdog Encoder");
		return -1;
	}

	return 0;
}
