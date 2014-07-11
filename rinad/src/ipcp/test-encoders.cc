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

#include <librina/common.h>

#include "encoder.h"
#include "ipcp/flow-allocator.h"

bool test_Flow () {
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
	rinad::FlowEncoder encoder;
	bool ret = true;

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
	dtcp_config_to_encode.set_initial_recvr_inactivity_time(34);
	dtcp_config_to_encode.set_initial_sender_inactivity_time(51245);
	connection_policies_to_encode.set_dtcp_configuration(dtcp_config_to_encode);
	pconnection_to_encode->setPolicies(connection_policies_to_encode);
	connection_list.push_front(pconnection_to_encode);
	flow_to_encode.connections_ = connection_list;

	// Encode
	pflow_to_encode = &flow_to_encode;
	const rina::SerializedObject *serialized_object = encoder.encode((void*)pflow_to_encode);
	// Decode
	rinad::Flow *pflow_decoded = (rinad::Flow*)encoder.decode(*serialized_object);

	// Assert
	if (pflow_to_encode->source_naming_info_.processName != pflow_decoded->source_naming_info_.processName)
		ret = false;
	if (pflow_to_encode->source_naming_info_.processInstance != pflow_decoded->source_naming_info_.processInstance)
		ret = false;

	rina::Connection *pconnection_decoded = pflow_decoded->connections_.front();
	rina::ConnectionPolicies connection_policies_decoded = pconnection_decoded->getPolicies();
	if ( connection_policies_to_encode.get_seq_num_rollover_threshold() != connection_policies_decoded.get_seq_num_rollover_threshold())
		ret = false;
	if ( connection_policies_to_encode.get_initial_a_timer() != connection_policies_decoded.get_initial_a_timer())
		ret = false;
	if ( connection_policies_to_encode.get_initial_seq_num_policy() != connection_policies_decoded.get_initial_seq_num_policy())
		ret = false;

	if ( connection_policies_to_encode.is_dtcp_present() != connection_policies_decoded.is_dtcp_present())
		ret = false;
	else {

		rina::DTCPConfig dtcp_config_decoded = connection_policies_decoded.get_dtcp_configuration();
		if(dtcp_config_to_encode.is_rtx_control() != dtcp_config_decoded.is_rtx_control())
			ret = false;
		if(dtcp_config_to_encode.get_rtx_control_config().get_data_rxmsn_max() != dtcp_config_decoded.get_rtx_control_config().get_data_rxmsn_max())
			ret = false;
		if(dtcp_config_to_encode.get_initial_recvr_inactivity_time() != dtcp_config_decoded.get_initial_recvr_inactivity_time())
			ret = false;
		if(dtcp_config_to_encode.get_initial_sender_inactivity_time() != dtcp_config_decoded.get_initial_sender_inactivity_time())
			ret = false;

		if(dtcp_config_to_encode.is_flow_control() != dtcp_config_decoded.is_flow_control())
			ret = false;
		else {
			rina::DTCPFlowControlConfig flow_config_decoded = dtcp_config_decoded.get_flow_control_config();
			if (flow_config_to_encode.get_rcv_buffers_threshold() != flow_config_decoded.get_rcv_buffers_threshold())
				ret = false;
			if (flow_config_to_encode.get_rcv_bytes_percent_threshold() != flow_config_decoded.get_rcv_bytes_percent_threshold())
				ret = false;
			if (flow_config_to_encode.get_rcv_bytes_threshold() != flow_config_decoded.get_rcv_bytes_threshold())
				ret = false;
			if (flow_config_to_encode.get_sent_buffers_threshold() != flow_config_decoded.get_sent_buffers_threshold())
				ret = false;
			if (flow_config_to_encode.get_sent_bytes_percent_threshold() != flow_config_decoded.get_sent_bytes_percent_threshold())
				ret = false;
			if (flow_config_to_encode.get_sent_bytes_threshold() != flow_config_decoded.get_sent_bytes_threshold())
				ret = false;

			if (flow_config_to_encode.is_window_based() != flow_config_decoded.is_window_based())
				ret = false;
			else{
				rina::DTCPWindowBasedFlowControlConfig window_decoded = flow_config_decoded.get_window_based_config();
				if (window_to_encode.get_initial_credit() != window_decoded.get_initial_credit())
					ret = false;
				if (window_to_encode.get_maxclosed_window_queue_length() != window_decoded.get_maxclosed_window_queue_length())
					ret = false;
			}

			if (flow_config_to_encode.is_rate_based() != flow_config_decoded.is_rate_based())
				ret = false;
			else {
				rina::DTCPRateBasedFlowControlConfig rate_decoded = flow_config_decoded.get_rate_based_config();
				if (rate_to_encode.get_sending_rate() != rate_decoded.get_sending_rate())
					ret = false;
				if (rate_to_encode.get_time_period() != rate_decoded.get_time_period())
					ret = false;
			}
		}
	}

	return ret;
}

int main()
{
	bool result = test_Flow();

	if (result)
		return 0;
	else
		return -1;
}
