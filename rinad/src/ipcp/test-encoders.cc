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
#include "ipcp/enrollment-task.h"
#include "ipcp/flow-allocator.h"

bool test_flow (rinad::Encoder * encoder) {
	rinad::Flow flow_to_encode;
	rinad::Flow *pflow_to_encode;
	std::list<rina::Connection*> connection_list;
	rina::Connection *pconnection_to_encode = new rina::Connection;
	rina::ConnectionPolicies connection_policies_to_encode;
	rina::DTCPConfig dtcp_config_to_encode;
	rina::DTCPRtxControlConfig rtx_config;
	rina::DTCPFlowControlConfig flow_config_to_encode;
	rina::DTCPWindowBasedFlowControlConfig window_to_encode;
	rina::DTCPRateBasedFlowControlConfig rate_to_encode;

	// Set
	flow_to_encode.source_naming_info_ = rina::ApplicationProcessNamingInformation("test", "1");
	flow_to_encode.destination_naming_info_ = rina::ApplicationProcessNamingInformation("test2", "1");
	connection_policies_to_encode.set_dtcp_present(true);
	connection_policies_to_encode.set_seq_num_rollover_threshold(1234);
	connection_policies_to_encode.set_initial_a_timer(14561);
	connection_policies_to_encode.set_initial_seq_num_policy(rina::PolicyConfig("policy1", "23"));
	dtcp_config_to_encode.set_rtx_control(true);
	rtx_config.set_data_rxmsn_max(25423);
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
	connection_policies_to_encode.set_dtcp_configuration(dtcp_config_to_encode);
	pconnection_to_encode->setPolicies(connection_policies_to_encode);
	connection_list.push_front(pconnection_to_encode);
	flow_to_encode.connections_ = connection_list;

	// Encode
	pflow_to_encode = &flow_to_encode;
	const rina::SerializedObject *serialized_object = encoder->encode((void*)pflow_to_encode,
			rinad::EncoderConstants::FLOW_RIB_OBJECT_CLASS);
	// Decode
	rinad::Flow *pflow_decoded = (rinad::Flow*) encoder->decode(*serialized_object,
			rinad::EncoderConstants::FLOW_RIB_OBJECT_CLASS);

	// Assert
	if (pflow_to_encode->source_naming_info_.processName != pflow_decoded->source_naming_info_.processName)
		return false;
	if (pflow_to_encode->source_naming_info_.processInstance != pflow_decoded->source_naming_info_.processInstance)
		return false;

	rina::Connection *pconnection_decoded = pflow_decoded->connections_.front();
	rina::ConnectionPolicies connection_policies_decoded = pconnection_decoded->getPolicies();
	if ( connection_policies_to_encode.get_seq_num_rollover_threshold() != connection_policies_decoded.get_seq_num_rollover_threshold())
		return false;
	if ( connection_policies_to_encode.get_initial_a_timer() != connection_policies_decoded.get_initial_a_timer())
		return false;
	if ( connection_policies_to_encode.get_initial_seq_num_policy() != connection_policies_decoded.get_initial_seq_num_policy())
		return false;

	if ( connection_policies_to_encode.is_dtcp_present() != connection_policies_decoded.is_dtcp_present())
		return false;
	else {

		rina::DTCPConfig dtcp_config_decoded = connection_policies_decoded.get_dtcp_configuration();
		if(dtcp_config_to_encode.is_rtx_control() != dtcp_config_decoded.is_rtx_control())
			return false;
		if(dtcp_config_to_encode.get_rtx_control_config().get_data_rxmsn_max() != dtcp_config_decoded.get_rtx_control_config().get_data_rxmsn_max())
			return false;
		if(dtcp_config_to_encode.get_rtx_control_config().get_max_time_to_retry() != dtcp_config_decoded.get_rtx_control_config().get_max_time_to_retry())
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

	LOG_INFO("Flow Encoder tested successfully");
	return true;
}

bool test_data_transfer_constants(rinad::Encoder * encoder) {
	rina::DataTransferConstants dtc;
	rina::DataTransferConstants * recovered_obj = 0;
	const rina::SerializedObject * encoded_object;

	dtc.address_length_ = 23;
	dtc.cep_id_length_ = 15;
	dtc.dif_integrity_ = true;
	dtc.length_length_ = 12;
	dtc.max_pdu_lifetime_ = 45243;
	dtc.max_pdu_size_ = 1233;
	dtc.port_id_length_ = 541;
	dtc.qos_id_lenght_ = 3414;
	dtc.sequence_number_length_ = 123;

	encoded_object = encoder->encode(&dtc,
			rinad::EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_CLASS);
	recovered_obj = (rina::DataTransferConstants*) encoder->decode(*encoded_object,
			rinad::EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_CLASS);

	if (dtc.address_length_ != recovered_obj->address_length_) {
		return false;
	}

	if (dtc.cep_id_length_ != recovered_obj->cep_id_length_) {
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

	if (dtc.qos_id_lenght_ != recovered_obj->qos_id_lenght_) {
		return false;
	}

	if (dtc.sequence_number_length_ != recovered_obj->sequence_number_length_) {
		return false;
	}

	delete recovered_obj;
	delete encoded_object;

	LOG_INFO("Data Transfer Constants Encoder tested successfully");
	return true;
}

bool test_directory_forwarding_table_entry(rinad::Encoder * encoder) {
	rina::DirectoryForwardingTableEntry dfte;
	rina::DirectoryForwardingTableEntry * recovered_obj = 0;
	const rina::SerializedObject * encoded_object;

	dfte.address_ = 232;
	dfte.timestamp_ = 5265235;
	dfte.ap_naming_info_.processName = "test";
	dfte.ap_naming_info_.processInstance = "1";
	dfte.ap_naming_info_.entityName = "ae";
	dfte.ap_naming_info_.entityInstance = "1";

	encoded_object = encoder->encode(&dfte,
			rinad::EncoderConstants::DFT_ENTRY_RIB_OBJECT_CLASS);
	recovered_obj = (rina::DirectoryForwardingTableEntry*) encoder->decode(*encoded_object,
			rinad::EncoderConstants::DFT_ENTRY_RIB_OBJECT_CLASS);

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
	delete encoded_object;

	LOG_INFO("Directory Forwarding Table Entry Encoder tested successfully");
	return true;
}

bool test_directory_forwarding_table_entry_list(rinad::Encoder * encoder) {
	std::list<rina::DirectoryForwardingTableEntry*> dfte_list;
	rina::DirectoryForwardingTableEntry dfte1;
	rina::DirectoryForwardingTableEntry dfte2;
	std::list<rina::DirectoryForwardingTableEntry*> * recovered_obj = 0;
	const rina::SerializedObject * encoded_object;

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

	encoded_object = encoder->encode(&dfte_list,
			rinad::EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_CLASS);
	recovered_obj = (std::list<rina::DirectoryForwardingTableEntry*> *) encoder->decode(*encoded_object,
			rinad::EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_CLASS);

	if (dfte_list.size() != recovered_obj->size()) {
		return false;
	}

	delete recovered_obj;
	delete encoded_object;

	LOG_INFO("Directory Forwarding Table Entry List Encoder tested successfully");
	return true;
}

bool test_enrollment_information_request(rinad::Encoder * encoder) {
	rinad::EnrollmentInformationRequest request;
	rina::ApplicationProcessNamingInformation name1;
	rina::ApplicationProcessNamingInformation name2;
	rinad::EnrollmentInformationRequest * recovered_obj = 0;
	const rina::SerializedObject * encoded_object;

	name1.processName = "dif1";
	name2.processName = "dif2";
	request.supporting_difs_.push_back(name1);
	request.supporting_difs_.push_back(name2);
	request.address_ = 141234;

	encoded_object = encoder->encode(&request,
			rinad::EncoderConstants::ENROLLMENT_INFO_OBJECT_CLASS);
	recovered_obj = (rinad::EnrollmentInformationRequest *) encoder->decode(*encoded_object,
			rinad::EncoderConstants::ENROLLMENT_INFO_OBJECT_CLASS);

	if (request.address_ != recovered_obj->address_) {
		return false;
	}

	if (request.supporting_difs_.size() != recovered_obj->supporting_difs_.size()) {
		return false;
	}

	delete recovered_obj;
	delete encoded_object;

	LOG_INFO("Enrollment Information Request Encoder tested successfully");
	return true;
}

int main()
{
	rinad::Encoder encoder;
	encoder.addEncoder(rinad::EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_CLASS,
			new rinad::DataTransferConstantsEncoder());
	encoder.addEncoder(rinad::EncoderConstants::FLOW_RIB_OBJECT_CLASS,
			new rinad::FlowEncoder());
	encoder.addEncoder(rinad::EncoderConstants::DFT_ENTRY_RIB_OBJECT_CLASS,
			new rinad::DirectoryForwardingTableEntryEncoder());
	encoder.addEncoder(rinad::EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_CLASS,
			new rinad::DirectoryForwardingTableEntryListEncoder());
	encoder.addEncoder(rinad::EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_CLASS,
			new rinad::DirectoryForwardingTableEntryListEncoder());
	encoder.addEncoder(rinad::EncoderConstants::ENROLLMENT_INFO_OBJECT_CLASS,
			new rinad::EnrollmentInformationRequestEncoder());

	bool result = test_flow(&encoder);
	if (!result) {
		LOG_ERR("Problems testing Flow Encoder");
		return -1;
	}

	result = test_data_transfer_constants(&encoder);
	if (!result) {
		LOG_ERR("Problems testing Data Transfer Constants Encoder");
		return -1;
	}

	result = test_directory_forwarding_table_entry(&encoder);
	if (!result) {
		LOG_ERR("Problems testing Directory Forwarding Table Entry Encoder");
		return -1;
	}

	result = test_directory_forwarding_table_entry_list(&encoder);
	if (!result) {
		LOG_ERR("Problems testing Directory Forwarding Table Entry List Encoder");
		return -1;
	}

	result = test_enrollment_information_request(&encoder);
	if (!result) {
		LOG_ERR("Problems testing Enrollment Information Request Encoder");
		return -1;
	}

	return 0;
}
