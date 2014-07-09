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

#include <librina/common.h>

#include "encoder.h"
#include "ipcp/flow-allocator.h"

/*
bool test_ApplicationProcessNamingInfoMessage () {
	rina::ApplicationProcessNamingInformation ap_info;
	rinad::Encoder encoder;
	rinad::ApplicationRegistrationEncoder app_reg_encoder;
	encoder.addEncoder(rinad::Encoder::ApplicationRegistration, &app_reg_encoder);
	bool assert = true;

	ap_info.setEntityInstance("1");
	ap_info.setEntityName("A");
	ap_info.setProcessInstance("1");
	ap_info.setProcessName("A proc");
	rina::ApplicationRegistration app_reg(ap_info);
	for(int i=0; i < 3; i++)
	{
		rina::ApplicationProcessNamingInformation ap_aux;
		std::stringstream ss;
		ss << "DIF name "<< i<< std::endl;
		ap_aux.setProcessName(ss.str());
		app_reg.addDIFName(ap_aux);
	}
	rina::SerializedObject *encoded_object = encoder.encode((void*) &app_reg, rinad::Encoder::ApplicationRegistration);

	rina::ApplicationRegistration *decoded_object = (rina::ApplicationRegistration*)encoder.decode(*encoded_object, rinad::Encoder::ApplicationRegistration);

	if (app_reg.getApplicationName() != *decoded_object->getApplicationName())
		assert = false;
	else {
		if (app_reg.getDIFNames().size() != decoded_object->getDIFNames().size())
			assert = false;
		else {
			std::list<rina::ApplicationProcessNamingInformation>::const_iterator it2 = decoded_object->getDIFNames().begin();
			for (std::list<rina::ApplicationProcessNamingInformation>::const_iterator it = app_reg.getDIFNames().begin(); it != app_reg.getDIFNames().end(); ++it)
			{
				if (it->getProcessName() !=  it2->getProcessName())
					assert = false;
				++it2;
			}
		}

	}

	delete encoded_object;
	delete decoded_object;

	return true;
}
*/

bool test_Flow () {
	rinad::Flow flow;
	std::list<rina::Connection*> connection_list;
	rina::Connection *connection = new rina::Connection;
	rina::ConnectionPolicies connection_policies;
	rina::DTCPConfig dtcp_config;
	rina::DTCPRtxControlConfig rtx_config;
	rina::DTCPFlowControlConfig flow_config;
	rina::DTCPWindowBasedFlowControlConfig window;
	rina::DTCPRateBasedFlowControlConfig rate;

	// Set
	flow.source_naming_info_ = rina::ApplicationProcessNamingInformation("test", "1");
	flow.destination_naming_info_ = rina::ApplicationProcessNamingInformation("test2", "1");
	connection_policies.set_dtcp_present(true);
	connection_policies.set_seq_num_rollover_threshold(1234);
	connection_policies.set_initial_a_timer(14561);
	connection_policies.set_initial_seq_num_policy(rina::PolicyConfig("policy1", "23"));
	dtcp_config.set_rtx_control(true);
	rtx_config.set_data_rxmsn_max(25423);
	dtcp_config.set_rtx_control_config(rtx_config);
	dtcp_config.set_flow_control(true);
	flow_config.set_rcv_buffers_threshold(412431);
	flow_config.set_rcv_bytes_percent_threshold(134);
	flow_config.set_rcv_bytes_threshold(46236);
	flow_config.set_sent_buffers_threshold(94);
	flow_config.set_sent_bytes_percent_threshold(2562);
	flow_config.set_sent_bytes_threshold(26236);
	flow_config.set_window_based(true);
	window.set_initial_credit(62556);
	window.set_max_closed_window_queue_length(5612623);
	flow_config.set_window_based_config(window);
	flow_config.set_rate_based(true);
	rate.set_sending_rate(45125);
	rate.set_time_period(1451234);
	flow_config.set_rate_based_config(rate);
	dtcp_config.set_flow_control_config(flow_config);
	dtcp_config.set_initial_recvr_inactivity_time(34);
	dtcp_config.set_initial_sender_inactivity_time(51245);
	connection_policies.set_dtcp_configuration(dtcp_config);
	connection->setPolicies(connection_policies);
	connection_list.push_front(connection);
	flow.connections_ = connection_list;

	// Encode

	return true;
}

int main()
{
	test_Flow();

	return 0;
}
