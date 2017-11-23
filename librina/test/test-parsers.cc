//
// Test parsers
//
//    Eduard Grasa          <eduard.grasa@i2cat.net>
//    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#include <iostream>

#include "irati/serdes-utils.h"
#include "irati/kernel-msg.h"
#include "core.h"
#include "ctrl.h"
#include "librina/configuration.h"
#include "librina/security-manager.h"
#include "librina/ipc-daemons.h"

#define DEFAULT_AP_NAME        "default/apname"
#define DEFAULT_AP_INSTANCE    "default/apinstance"
#define DEFAULT_AE_NAME        "default/aename"
#define DEFAULT_AE_INSTANCE    "default/aeinstance"
#define DEFAULT_POLICY_NAME    "default"
#define DEFAULT_POLICY_VERSION "1"
#define DEFAULT_DIF_NAME       "default.DIF"

using namespace rina;

void populate_dtp_config(DTPConfig & dtp_config)
{
	PolicyParameter param;

	param.name_ = DEFAULT_POLICY_NAME;
	param.value_ = DEFAULT_POLICY_VERSION;

	dtp_config.dtcp_present_ = true;
	dtp_config.dtp_policy_set_.name_ = DEFAULT_POLICY_NAME;
	dtp_config.dtp_policy_set_.version_ = DEFAULT_POLICY_VERSION;
	dtp_config.dtp_policy_set_.parameters_.push_back(param);
	dtp_config.in_order_delivery_ = true;
	dtp_config.incomplete_delivery_ = true;
	dtp_config.initial_a_timer_ = 232;
	dtp_config.max_sdu_gap_ = 65;
	dtp_config.partial_delivery_ = true;
	dtp_config.seq_num_rollover_threshold_ = 513412341;
}

void populate_dtcp_config(DTCPConfig & dtcp_config)
{
	PolicyParameter param;

	param.name_ = DEFAULT_POLICY_NAME;
	param.value_ = DEFAULT_POLICY_VERSION;

	dtcp_config.dtcp_policy_set_.name_ = DEFAULT_POLICY_NAME;
	dtcp_config.dtcp_policy_set_.version_ = DEFAULT_POLICY_VERSION;
	dtcp_config.dtcp_policy_set_.parameters_.push_back(param);
	dtcp_config.flow_control_ = true;
	dtcp_config.rtx_control_ = true;
	dtcp_config.lost_control_pdu_policy_.name_ = DEFAULT_POLICY_NAME;
	dtcp_config.lost_control_pdu_policy_.version_ = DEFAULT_POLICY_VERSION;
	dtcp_config.lost_control_pdu_policy_.parameters_.push_back(param);
	dtcp_config.rtt_estimator_policy_.name_ = DEFAULT_POLICY_NAME;
	dtcp_config.rtt_estimator_policy_.version_ = DEFAULT_POLICY_VERSION;
	dtcp_config.rtt_estimator_policy_.parameters_.push_back(param);
	dtcp_config.rtx_control_config_.data_rxms_nmax_ = 232;
	dtcp_config.rtx_control_config_.initial_rtx_time_ = 2;
	dtcp_config.rtx_control_config_.max_time_to_retry_ = 232;
	dtcp_config.rtx_control_config_.rcvr_ack_policy_.name_ = DEFAULT_POLICY_NAME;
	dtcp_config.rtx_control_config_.rcvr_ack_policy_.version_ = DEFAULT_POLICY_VERSION;
	dtcp_config.rtx_control_config_.rcvr_ack_policy_.parameters_.push_back(param);
	dtcp_config.rtx_control_config_.rcvr_control_ack_policy_.name_ = DEFAULT_POLICY_NAME;
	dtcp_config.rtx_control_config_.rcvr_control_ack_policy_.version_ = DEFAULT_POLICY_VERSION;
	dtcp_config.rtx_control_config_.rcvr_control_ack_policy_.parameters_.push_back(param);
	dtcp_config.rtx_control_config_.recving_ack_list_policy_.name_ = DEFAULT_POLICY_NAME;
	dtcp_config.rtx_control_config_.recving_ack_list_policy_.version_ = DEFAULT_POLICY_VERSION;
	dtcp_config.rtx_control_config_.recving_ack_list_policy_.parameters_.push_back(param);
	dtcp_config.rtx_control_config_.rtx_timer_expiry_policy_.name_ = DEFAULT_POLICY_NAME;
	dtcp_config.rtx_control_config_.rtx_timer_expiry_policy_.version_ = DEFAULT_POLICY_VERSION;
	dtcp_config.rtx_control_config_.rtx_timer_expiry_policy_.parameters_.push_back(param);
	dtcp_config.rtx_control_config_.sender_ack_policy_.name_ = DEFAULT_POLICY_NAME;
	dtcp_config.rtx_control_config_.sender_ack_policy_.version_ = DEFAULT_POLICY_VERSION;
	dtcp_config.rtx_control_config_.sender_ack_policy_.parameters_.push_back(param);
	dtcp_config.rtx_control_config_.sending_ack_policy_.name_ = DEFAULT_POLICY_NAME;
	dtcp_config.rtx_control_config_.sending_ack_policy_.version_ = DEFAULT_POLICY_VERSION;
	dtcp_config.rtx_control_config_.sending_ack_policy_.parameters_.push_back(param);
	dtcp_config.flow_control_config_.closed_window_policy_.name_ = DEFAULT_POLICY_NAME;
	dtcp_config.flow_control_config_.closed_window_policy_.version_ = DEFAULT_POLICY_VERSION;
	dtcp_config.flow_control_config_.closed_window_policy_.parameters_.push_back(param);
	dtcp_config.flow_control_config_.flow_control_overrun_policy_.name_ = DEFAULT_POLICY_NAME;
	dtcp_config.flow_control_config_.flow_control_overrun_policy_.version_ = DEFAULT_POLICY_VERSION;
	dtcp_config.flow_control_config_.flow_control_overrun_policy_.parameters_.push_back(param);
	dtcp_config.flow_control_config_.receiving_flow_control_policy_.name_ = DEFAULT_POLICY_NAME;
	dtcp_config.flow_control_config_.receiving_flow_control_policy_.version_ = DEFAULT_POLICY_VERSION;
	dtcp_config.flow_control_config_.receiving_flow_control_policy_.parameters_.push_back(param);
	dtcp_config.flow_control_config_.reconcile_flow_control_policy_.name_ = DEFAULT_POLICY_NAME;
	dtcp_config.flow_control_config_.reconcile_flow_control_policy_.version_ = DEFAULT_POLICY_VERSION;
	dtcp_config.flow_control_config_.reconcile_flow_control_policy_.parameters_.push_back(param);
	dtcp_config.flow_control_config_.rate_based_ = true;
	dtcp_config.flow_control_config_.rcv_buffers_threshold_ = 23;
	dtcp_config.flow_control_config_.rcv_bytes_percent_threshold_ = 2323;
	dtcp_config.flow_control_config_.rcv_bytes_threshold_ = 62623;
	dtcp_config.flow_control_config_.sent_buffers_threshold_ = 34232;
	dtcp_config.flow_control_config_.sent_bytes_percent_threshold_ = 452345;
	dtcp_config.flow_control_config_.sent_bytes_threshold_ = 412434;
	dtcp_config.flow_control_config_.window_based_ = true;
	dtcp_config.flow_control_config_.rate_based_config_.no_override_default_peak_policy_.name_ = DEFAULT_POLICY_NAME;
	dtcp_config.flow_control_config_.rate_based_config_.no_override_default_peak_policy_.version_ = DEFAULT_POLICY_VERSION;
	dtcp_config.flow_control_config_.rate_based_config_.no_override_default_peak_policy_.parameters_.push_back(param);
	dtcp_config.flow_control_config_.rate_based_config_.no_rate_slow_down_policy_.name_ = DEFAULT_POLICY_NAME;
	dtcp_config.flow_control_config_.rate_based_config_.no_rate_slow_down_policy_.version_ = DEFAULT_POLICY_VERSION;
	dtcp_config.flow_control_config_.rate_based_config_.no_rate_slow_down_policy_.parameters_.push_back(param);
	dtcp_config.flow_control_config_.rate_based_config_.rate_reduction_policy_.name_ = DEFAULT_POLICY_NAME;
	dtcp_config.flow_control_config_.rate_based_config_.rate_reduction_policy_.version_ = DEFAULT_POLICY_VERSION;
	dtcp_config.flow_control_config_.rate_based_config_.rate_reduction_policy_.parameters_.push_back(param);
	dtcp_config.flow_control_config_.rate_based_config_.sending_rate_ = 232;
	dtcp_config.flow_control_config_.rate_based_config_.time_period_ = 2324;
	dtcp_config.flow_control_config_.window_based_config_.initial_credit_ = 32323;
	dtcp_config.flow_control_config_.window_based_config_.max_closed_window_queue_length_ = 2323;
	dtcp_config.flow_control_config_.window_based_config_.rcvr_flow_control_policy_.name_ = DEFAULT_POLICY_NAME;
	dtcp_config.flow_control_config_.window_based_config_.rcvr_flow_control_policy_.version_ = DEFAULT_POLICY_VERSION;
	dtcp_config.flow_control_config_.window_based_config_.rcvr_flow_control_policy_.parameters_.push_back(param);
	dtcp_config.flow_control_config_.window_based_config_.tx_control_policy_.name_ = DEFAULT_POLICY_NAME;
	dtcp_config.flow_control_config_.window_based_config_.tx_control_policy_.version_ = DEFAULT_POLICY_VERSION;
	dtcp_config.flow_control_config_.window_based_config_.tx_control_policy_.parameters_.push_back(param);
}

void populate_qos_cube(QoSCube & qos_cube)
{
	qos_cube.average_bandwidth_ = 232323;
	qos_cube.average_sdu_bandwidth_ = 34232;
	qos_cube.delay_ = 34;
	populate_dtp_config(qos_cube.dtp_config_);
	populate_dtcp_config(qos_cube.dtcp_config_);
	qos_cube.id_ = 1;
	qos_cube.jitter_ = 23;
	qos_cube.max_allowable_gap_ = 2;
	qos_cube.name_ = DEFAULT_POLICY_NAME;
	qos_cube.ordered_delivery_ = true;
	qos_cube.partial_delivery_ = true;
	qos_cube.peak_bandwidth_duration_ = 323;
	qos_cube.peak_sdu_bandwidth_duration_ = 23;
}

void populate_dif_config(DIFConfiguration & dif_config)
{
	PolicyParameter param;

	param.name_ = DEFAULT_POLICY_NAME;
	param.value_ = DEFAULT_POLICY_VERSION;

	dif_config.address_ = 24;
	dif_config.efcp_configuration_.data_transfer_constants_.address_length_ = 2;
	dif_config.efcp_configuration_.data_transfer_constants_.cep_id_length_ = 3;
	dif_config.efcp_configuration_.data_transfer_constants_.ctrl_sequence_number_length_ = 3;
	dif_config.efcp_configuration_.data_transfer_constants_.dif_concatenation_ = true;
	dif_config.efcp_configuration_.data_transfer_constants_.dif_fragmentation_ = true;
	dif_config.efcp_configuration_.data_transfer_constants_.dif_integrity_ = true;
	dif_config.efcp_configuration_.data_transfer_constants_.frame_length_ = 2;
	dif_config.efcp_configuration_.data_transfer_constants_.length_length_ = 2;
	dif_config.efcp_configuration_.data_transfer_constants_.max_pdu_lifetime_ = 25;
	dif_config.efcp_configuration_.data_transfer_constants_.max_pdu_size_ = 32;
	dif_config.efcp_configuration_.data_transfer_constants_.max_time_to_ack_ = 22;
	dif_config.efcp_configuration_.data_transfer_constants_.max_time_to_keep_ret_ = 12;
	dif_config.efcp_configuration_.data_transfer_constants_.port_id_length_ = 2;
	dif_config.efcp_configuration_.data_transfer_constants_.qos_id_length_ = 3;
	dif_config.efcp_configuration_.data_transfer_constants_.rate_length_ = 2;
	dif_config.efcp_configuration_.data_transfer_constants_.seq_rollover_thres_ = 2;
	dif_config.efcp_configuration_.data_transfer_constants_.sequence_number_length_ = 25;
	dif_config.efcp_configuration_.unknown_flowpolicy_.name_ = DEFAULT_POLICY_NAME;
	dif_config.efcp_configuration_.unknown_flowpolicy_.version_ = DEFAULT_POLICY_VERSION;
	dif_config.et_configuration_.policy_set_.name_ = DEFAULT_POLICY_NAME;
	dif_config.et_configuration_.policy_set_.version_ = DEFAULT_POLICY_NAME;
	dif_config.et_configuration_.policy_set_.parameters_.push_back(param);
	dif_config.fa_configuration_.allocate_notify_policy_.name_ = DEFAULT_POLICY_NAME;
	dif_config.fa_configuration_.allocate_notify_policy_.version_ = DEFAULT_POLICY_VERSION;
	dif_config.fa_configuration_.allocate_notify_policy_.parameters_.push_back(param);
	dif_config.fa_configuration_.allocate_retry_policy_.name_ = DEFAULT_POLICY_NAME;
	dif_config.fa_configuration_.allocate_retry_policy_.version_ = DEFAULT_POLICY_VERSION;
	dif_config.fa_configuration_.allocate_retry_policy_.parameters_.push_back(param);
	dif_config.fa_configuration_.new_flow_request_policy_.name_ = DEFAULT_POLICY_NAME;
	dif_config.fa_configuration_.new_flow_request_policy_.version_ = DEFAULT_POLICY_VERSION;
	dif_config.fa_configuration_.new_flow_request_policy_.parameters_.push_back(param);
	dif_config.fa_configuration_.policy_set_.name_ = DEFAULT_POLICY_NAME;
	dif_config.fa_configuration_.policy_set_.version_ = DEFAULT_POLICY_VERSION;
	dif_config.fa_configuration_.policy_set_.parameters_.push_back(param);
	dif_config.fa_configuration_.seq_rollover_policy_.name_ = DEFAULT_POLICY_NAME;
	dif_config.fa_configuration_.seq_rollover_policy_.version_ = DEFAULT_POLICY_VERSION;
	dif_config.fa_configuration_.seq_rollover_policy_.parameters_.push_back(param);
	dif_config.fa_configuration_.max_create_flow_retries_ = 3;
	dif_config.nsm_configuration_.policy_set_.name_ = DEFAULT_POLICY_NAME;
	dif_config.nsm_configuration_.policy_set_.version_ = DEFAULT_POLICY_VERSION;
	dif_config.nsm_configuration_.policy_set_.parameters_.push_back(param);
	dif_config.ra_configuration_.pduftg_conf_.policy_set_.name_ = DEFAULT_POLICY_NAME;
	dif_config.ra_configuration_.pduftg_conf_.policy_set_.version_ = DEFAULT_POLICY_VERSION;
	dif_config.ra_configuration_.pduftg_conf_.policy_set_.parameters_.push_back(param);
	dif_config.rmt_configuration_.policy_set_.name_ = DEFAULT_POLICY_NAME;
	dif_config.rmt_configuration_.policy_set_.version_ = DEFAULT_POLICY_VERSION;
	dif_config.rmt_configuration_.policy_set_.parameters_.push_back(param);
	dif_config.rmt_configuration_.pft_conf_.policy_set_.name_ = DEFAULT_POLICY_NAME;
	dif_config.rmt_configuration_.pft_conf_.policy_set_.version_ = DEFAULT_POLICY_VERSION;
	dif_config.rmt_configuration_.pft_conf_.policy_set_.parameters_.push_back(param);
	dif_config.routing_configuration_.policy_set_.name_ = DEFAULT_POLICY_NAME;
	dif_config.routing_configuration_.policy_set_.version_ = DEFAULT_POLICY_VERSION;
	dif_config.routing_configuration_.policy_set_.parameters_.push_back(param);
	dif_config.sm_configuration_.policy_set_.name_ = DEFAULT_POLICY_NAME;
	dif_config.sm_configuration_.policy_set_.version_ = DEFAULT_POLICY_VERSION;
	dif_config.sm_configuration_.policy_set_.parameters_.push_back(param);
	dif_config.sm_configuration_.default_auth_profile.authPolicy.name_ = DEFAULT_POLICY_NAME;
	dif_config.sm_configuration_.default_auth_profile.authPolicy.version_ = DEFAULT_POLICY_VERSION;
	dif_config.sm_configuration_.default_auth_profile.authPolicy.parameters_.push_back(param);
	dif_config.sm_configuration_.default_auth_profile.crcPolicy.name_ = DEFAULT_POLICY_NAME;
	dif_config.sm_configuration_.default_auth_profile.crcPolicy.version_ = DEFAULT_POLICY_VERSION;
	dif_config.sm_configuration_.default_auth_profile.crcPolicy.parameters_.push_back(param);
	dif_config.sm_configuration_.default_auth_profile.encryptPolicy.name_ = DEFAULT_POLICY_NAME;
	dif_config.sm_configuration_.default_auth_profile.encryptPolicy.version_ = DEFAULT_POLICY_VERSION;
	dif_config.sm_configuration_.default_auth_profile.encryptPolicy.parameters_.push_back(param);
	dif_config.sm_configuration_.default_auth_profile.ttlPolicy.name_ = DEFAULT_POLICY_NAME;
	dif_config.sm_configuration_.default_auth_profile.ttlPolicy.version_ = DEFAULT_POLICY_VERSION;
	dif_config.sm_configuration_.default_auth_profile.ttlPolicy.parameters_.push_back(param);
}

void populate_uchar_array(UcharArray & array)
{
	array.length = 10;
	array.data = new unsigned char[10];
	memset(array.data, 23, array.length);
}

void populate_crypto_state(CryptoState & cs)
{
	cs.compress_alg = "compress-alg";
	cs.enable_crypto_rx = true;
	cs.enable_crypto_rx = true;
	cs.encrypt_alg = "encrypt-alg";
	cs.mac_alg = "mac-alg";
	populate_uchar_array(cs.encrypt_key_rx);
	populate_uchar_array(cs.encrypt_key_tx);
	populate_uchar_array(cs.iv_rx);
	populate_uchar_array(cs.iv_tx);
	populate_uchar_array(cs.mac_key_rx);
	populate_uchar_array(cs.mac_key_tx);
}

void populate_neighbor(Neighbor & nei)
{
	nei.address_ = 23;
	nei.average_rtt_in_ms_ = 232;
	nei.enrolled_ = true;
	nei.internal_port_id = 534;
	nei.last_heard_from_time_in_ms_ = 3232;
	nei.name_.processName = "test1.IRATI";
	nei.name_.processInstance = "1";
	nei.number_of_enrollment_attempts_ = 3;
	nei.old_address_ = 34;
	nei.supporting_dif_name_.processName = "test.DIF";
}

int test_irati_kmsg_ipcm_assign_to_dif()
{
	struct irati_kmsg_ipcm_assign_to_dif * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;
	ApplicationProcessNamingInformation before;
	ApplicationProcessNamingInformation after;
	DIFConfiguration d_before;
	DIFConfiguration d_after;
	std::string s_before;
	std::string s_after;

	std::cout << "TESTING KMSG IPCM ASSIGN TO DIF" << std::endl;

	before.processName = "/test.DIF";

	populate_dif_config(d_before);

	msg = new irati_kmsg_ipcm_assign_to_dif();
	msg->msg_type = RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST;
	msg->dif_name = before.to_c_name();
	msg->type = stringToCharArray("normal-ipcp");
	msg->dif_config = d_before.to_c_dif_config();

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_kmsg_ipcm_assign_to_dif message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_kmsg_ipcm_assign_to_dif *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	    	       serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_kmsg_ipcm_assign_to_dif message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	s_before = msg->type;
	s_after = resp->type;
	after = ApplicationProcessNamingInformation(resp->dif_name);
	DIFConfiguration::from_c_dif_config(d_after, resp->dif_config);

	if (s_before != s_after) {
		std::cout << "Type on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (before != after) {
		std::cout << "Source application name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (d_before != d_after) {
		std::cout << "Configuration on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return 0;
}

int test_irati_msg_base_resp(irati_msg_t msg_t)
{
	struct irati_msg_base_resp * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;

	std::cout << "TESTING IRATI MSG BASE RESP (" << msg_t << ")" << std::endl;

	msg = new irati_msg_base_resp();
	msg->msg_type = msg_t;
	msg->result = 32;

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_msg_base_resp message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_msg_base_resp *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	             serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_msg_base_resp message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (msg->result != resp->result) {
		std::cout << "Result on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_kmsg_ipcm_update_config()
{
	struct irati_kmsg_ipcm_update_config * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;
	DIFConfiguration d_before;
	DIFConfiguration d_after;

	std::cout << "TESTING KMSG IPCM UPDATE DIF CONFIG" << std::endl;

	populate_dif_config(d_before);

	msg = new irati_kmsg_ipcm_update_config();
	msg->msg_type = RINA_C_IPCM_UPDATE_DIF_CONFIG_REQUEST;
	msg->dif_config = d_before.to_c_dif_config();

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_kmsg_ipcm_update_config message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_kmsg_ipcm_update_config *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	    	       serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_kmsg_ipcm_update_config message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	DIFConfiguration::from_c_dif_config(d_after, resp->dif_config);

	if (d_before != d_after) {
		std::cout << "Configuration on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_kmsg_ipcp_dif_reg_not(irati_msg_t msg_t)
{
	struct irati_kmsg_ipcp_dif_reg_not * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;
	ApplicationProcessNamingInformation i_before, i_after, d_before, d_after;

	std::cout << "TESTING KMSG IPCP DIF REGISTRATION NOTIFICATION (" << msg_t
		   << ")" << std::endl;

	i_before.processName = "/test/ipcp";
	i_before.processInstance = "1";
	d_before.processName = "/test/normal.DIF",

	msg = new irati_kmsg_ipcp_dif_reg_not();
	msg->msg_type = msg_t;
	msg->ipcp_name = i_before.to_c_name();
	msg->dif_name = d_before.to_c_name();
	msg->is_registered = true;

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_kmsg_ipcp_dif_reg_not message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_kmsg_ipcp_dif_reg_not *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	    	       serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_kmsg_ipcp_dif_reg_not message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	i_after = ApplicationProcessNamingInformation(resp->ipcp_name);
	d_after = ApplicationProcessNamingInformation(resp->dif_name);

	if (msg->is_registered != resp->is_registered) {
		std::cout << "Is registered on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (i_before != i_after) {
		std::cout << "IPCP name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (d_before != d_after) {
		std::cout << "DIF name on original and recovered "
				<< "messages are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_kmsg_ipcm_allocate_flow(irati_msg_t msg_t)
{
	struct irati_kmsg_ipcm_allocate_flow * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;
	ApplicationProcessNamingInformation s_before, s_after, d_before, d_after;
	ApplicationProcessNamingInformation dif_before, dif_after;

	std::cout << "TESTING KMSG IPCM ALOCATE FLOW (" << msg_t
		   << ")" << std::endl;

	s_before.processName = "/apps/source";
	s_before.processInstance = "12";
	s_before.entityName = "database";
	s_before.entityInstance = "232";
	d_before.processName = "/apps/dest";
	d_before.processInstance = "12345";
	d_before.entityName = "printer";
	d_before.entityInstance = "1212313";
	dif_before.processName = "test.DIF";

	msg = new irati_kmsg_ipcm_allocate_flow();
	msg->msg_type = msg_t;
	msg->port_id = 25;
	msg->local = s_before.to_c_name();
	msg->remote = d_before.to_c_name();
	msg->fspec = new flow_spec();
	msg->dif_name = dif_before.to_c_name();

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_kmsg_ipcm_allocate_flow message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_kmsg_ipcm_allocate_flow *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	    	       serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_kmsg_ipcm_allocate_flow message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	s_after = ApplicationProcessNamingInformation(resp->local);
	d_after = ApplicationProcessNamingInformation(resp->remote);
	dif_after = ApplicationProcessNamingInformation(resp->dif_name);

	if (msg->port_id != resp->port_id) {
		std::cout << "Port-id on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (s_before != s_after) {
		std::cout << "Source application name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (d_before != d_after) {
		std::cout << "Destination application name on original and recovered "
				<< "messages are different\n";
		ret = -1;
	} else if (dif_before != dif_after) {
		std::cout << "DIF name on original and recovered "
				<< "messages are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_kmsg_ipcm_allocate_flow_resp()
{
	struct irati_kmsg_ipcm_allocate_flow_resp * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;

	std::cout << "TESTING KMSG IPCM ALLOCATE FLOW RESP" << std::endl;

	msg = new irati_kmsg_ipcm_allocate_flow_resp();
	msg->msg_type = RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE;
	msg->result = 23;
	msg->notify_src = true;
	msg->id = 54;

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_kmsg_ipcm_allocate_flow_resp message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_kmsg_ipcm_allocate_flow_resp *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	             serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_kmsg_ipcm_allocate_flow_resp message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (msg->result != resp->result) {
		std::cout << "Result on original and recovered messages"
			   << " are different\n";
		ret = 0;
	} else if (msg->notify_src != resp->notify_src) {
		std::cout << "Notify source on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (msg->id != resp->id) {
		std::cout << "Port-id on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_kmsg_ipcm_reg_app()
{
	struct irati_kmsg_ipcm_reg_app * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;
	ApplicationProcessNamingInformation a_before, a_after, da_before, da_after,
					   di_before, di_after;

	std::cout << "TESTING KMSG IPCM REGISTER APPLICATION" << std::endl;

	a_before.processName = "/test/app";
	a_before.processInstance = "1";
	a_before.entityName = "database";
	a_before.entityInstance = "1212";
	da_before.processName = "/daf/test";
	di_before.processInstance = "test.DIF";

	msg = new irati_kmsg_ipcm_reg_app();
	msg->msg_type = RINA_C_IPCM_REGISTER_APPLICATION_REQUEST;
	msg->app_name = a_before.to_c_name();
	msg->daf_name = da_before.to_c_name();
	msg->dif_name = di_before.to_c_name();

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_kmsg_ipcm_reg_app message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_kmsg_ipcm_reg_app *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	    	       serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_kmsg_ipcm_reg_app message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	a_after = ApplicationProcessNamingInformation(resp->app_name);
	da_after = ApplicationProcessNamingInformation(resp->daf_name);
	di_after = ApplicationProcessNamingInformation(resp->dif_name);

	if (a_before != a_after) {
		std::cout << "App name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (da_before != da_after) {
		std::cout << "DAF name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (di_before != di_after) {
		std::cout << "DIF name on original and recovered "
				<< "messages are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_kmsg_ipcm_unreg_app()
{
	struct irati_kmsg_ipcm_unreg_app * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;
	ApplicationProcessNamingInformation a_before, a_after, di_before, di_after;

	std::cout << "TESTING KMSG IPCM UNREGISTER APPLICATION" << std::endl;

	a_before.processName = "/test/app";
	a_before.processInstance = "1";
	a_before.entityName = "database";
	a_before.entityInstance = "1212";
	di_before.processInstance = "test.DIF";

	msg = new irati_kmsg_ipcm_unreg_app();
	msg->msg_type = RINA_C_IPCM_UNREGISTER_APPLICATION_REQUEST;
	msg->app_name = a_before.to_c_name();
	msg->dif_name = di_before.to_c_name();

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_kmsg_ipcm_unreg_app message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_kmsg_ipcm_unreg_app *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	    	       serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_kmsg_ipcm_unreg_app message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	a_after = ApplicationProcessNamingInformation(resp->app_name);
	di_after = ApplicationProcessNamingInformation(resp->dif_name);

	if (a_before != a_after) {
		std::cout << "App name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (di_before != di_after) {
		std::cout << "DIF name on original and recovered "
				<< "messages are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_kmsg_ipcm_query_rib()
{
	struct irati_kmsg_ipcm_query_rib * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;
	std::string f_before, f_after, oc_before, oc_after, on_before, on_after;

	std::cout << "TESTING KMSG IPCM QUERY RIB" << std::endl;

	f_before = "test filter";
	on_before = "/dif/naming/address";
	oc_before = "o_class";

	msg = new irati_kmsg_ipcm_query_rib();
	msg->msg_type = RINA_C_IPCM_QUERY_RIB_REQUEST;
	msg->object_instance = 23;
	msg->scope = 4;
	msg->filter = stringToCharArray(f_before);
	msg->object_name = stringToCharArray(on_before);
	msg->object_class = stringToCharArray(oc_before);

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_kmsg_ipcm_query_rib message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_kmsg_ipcm_query_rib *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	    	       serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_kmsg_ipcm_query_rib message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	f_after = resp->filter;
	on_after = resp->object_name;
	oc_after = resp->object_class;

	if (f_before != f_after) {
		std::cout << "Filter on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (on_before != on_after) {
		std::cout << "Object name on original and recovered "
				<< "messages are different\n";
		ret = -1;
	} else if (oc_before != oc_after) {
		std::cout << "Object class on original and recovered "
				<< "messages are different\n";
		ret = -1;
	} else if (msg->object_instance != resp->object_instance) {
		std::cout << "Object instance on original and recovered "
				<< "messages are different\n";
		ret = -1;
	} else if (msg->scope != resp->scope) {
		std::cout << "Scope on original and recovered "
				<< "messages are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_kmsg_ipcm_query_rib_resp()
{
	struct irati_kmsg_ipcm_query_rib_resp * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;
	struct rib_object_data * rodata, * pos, * rodata_after;
	int num_entries = 0;

	std::cout << "TESTING KMSG IPCM QUERY RIB RESP" << std::endl;

	msg = new irati_kmsg_ipcm_query_rib_resp();
	msg->msg_type = RINA_C_IPCM_QUERY_RIB_RESPONSE;
	msg->result = 14;
	msg->rib_entries = query_rib_resp_create();
	rodata = rib_object_data_create();
	rodata->instance = 25;
	rodata->clazz = stringToCharArray("myclass");
	rodata->name = stringToCharArray("myname");
	rodata->disp_value = stringToCharArray("this is my value");
	list_add_tail(&rodata->next, &msg->rib_entries->rib_object_data_entries);

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_kmsg_ipcm_query_rib_resp message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_kmsg_ipcm_query_rib_resp *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	    	       serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_kmsg_ipcm_query_rib_resp message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

        list_for_each_entry(pos, &(resp->rib_entries->rib_object_data_entries), next) {
                num_entries ++;
                rodata_after = pos;
        }

	if (msg->result != resp->result) {
		std::cout << "Filter on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (num_entries != 1) {
		std::cout << "Number of RIB entries on original and recovered "
				<< "messages are different\n";
		ret = -1;
	} else if (strcmp(rodata->clazz, rodata_after->clazz)) {
		std::cout << "Object class on original and recovered "
				<< "messages are different\n";
		ret = -1;
	} else if (rodata->instance != rodata_after->instance) {
		std::cout << "Object instance on original and recovered "
				<< "messages are different\n";
		ret = -1;
	} else if (strcmp(rodata->name, rodata_after->name)) {
		std::cout << "Object name on original and recovered "
				<< "messages are different\n";
		ret = -1;
	} else if (strcmp(rodata->disp_value, rodata_after->disp_value)) {
		std::cout << "Displayable value on original and recovered "
				<< "messages are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_msg_base(irati_msg_t msg_t)
{
	struct irati_msg_base * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;

	std::cout << "TESTING IRATI MSG BASE (" << msg_t << ")" << std::endl;

	msg = new irati_msg_base();
	msg->msg_type = msg_t;
	msg->dest_ipcp_id = 23;
	msg->dest_port = 243;
	msg->event_id = 342323;
	msg->msg_type = msg_t;
	msg->src_ipcp_id = 232;
	msg->src_port = 334;

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_msg_base message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_msg_base *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	             serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_msg_base message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (msg->dest_ipcp_id != resp->dest_ipcp_id) {
		std::cout << "Dest IPCP id on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (msg->dest_port != resp->dest_port) {
		std::cout << "Dest port on original and recovered messages"
				<< " are different\n";
		ret = -1;
	} else if (msg->event_id != resp->event_id) {
		std::cout << "Event id on original and recovered messages"
				<< " are different\n";
		ret = -1;
	} else if (msg->src_ipcp_id != resp->src_ipcp_id) {
		std::cout << "Src IPCP id on original and recovered messages"
				<< " are different\n";
		ret = -1;
	} else if (msg->src_port != resp->src_port) {
		std::cout << "Src port on original and recovered messages"
				<< " are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_kmsg_rmt_dump_ft(irati_msg_t msg_t)
{
	struct irati_kmsg_rmt_dump_ft * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;
	struct mod_pff_entry * modata, * pos;
	struct port_id_altlist * podata;
	int num_entries = 0, num_entries2 = 0;

	std::cout << "TESTING KMSG RMT DUMP FT (" << msg_t << ")" << std::endl;

	msg = new irati_kmsg_rmt_dump_ft();
	msg->msg_type = msg_t;
	msg->result = 14;
	msg->mode = 5;
	msg->pft_entries = pff_entry_list_create();
	modata = mod_pff_entry_create();
	modata->cost = 123;
	modata->fwd_info = 54;
	modata->qos_id = 2;
	podata = port_id_altlist_create();
	podata->num_ports = 2;
	podata->ports = (port_id_t *) malloc(sizeof(port_id_t) * podata->num_ports);
	*(podata->ports) = 3;
	*(podata->ports + sizeof(port_id_t)) = 4;
	list_add_tail(&podata->next, &modata->port_id_altlists);
	list_add_tail(&modata->next, &msg->pft_entries->pff_entries);
	modata = mod_pff_entry_create();
	modata->cost = 123;
	modata->fwd_info = 54;
	modata->qos_id = 2;
	podata = port_id_altlist_create();
	podata->num_ports = 2;
	podata->ports = (port_id_t *) malloc(sizeof(port_id_t) * podata->num_ports);
	*(podata->ports) = 3;
	*(podata->ports + sizeof(port_id_t)) = 4;
	list_add_tail(&podata->next, &modata->port_id_altlists);
	list_add_tail(&modata->next, &msg->pft_entries->pff_entries);

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_kmsg_rmt_dump_ft message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_kmsg_rmt_dump_ft *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	    	       serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_kmsg_rmt_dump_ft message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

        list_for_each_entry(pos, &(msg->pft_entries->pff_entries), next) {
                num_entries ++;
        }

        list_for_each_entry(pos, &(resp->pft_entries->pff_entries), next) {
                num_entries2 ++;
        }

	if (msg->result != resp->result) {
		std::cout << "Result on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (msg->mode != resp->mode) {
		std::cout << "Mode on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (num_entries != 2 || num_entries2 != 2) {
		std::cout << "Number of entries on original and recovered "
				<< "messages are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_kmsg_ipcp_conn_create_arrived(irati_msg_t msg_t)
{
	struct irati_kmsg_ipcp_conn_create_arrived * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;
	DTPConfig dtp_before, dtp_after;
	DTCPConfig dtcp_before, dtcp_after;

	std::cout << "TESTING KMSG IPCP CONN CREATE ARRIVED (" << msg_t << ")" << std::endl;

	populate_dtp_config(dtp_before);
	populate_dtcp_config(dtcp_before);

	msg = new irati_kmsg_ipcp_conn_create_arrived();
	msg->msg_type = msg_t;
	msg->dst_addr = 23;
	msg->dst_cep = 13;
	msg->flow_user_ipc_process_id = 2;
	msg->port_id = 32;
	msg->qos_id = 23;
	msg->src_cep = 65;
	msg->src_addr = 34;
	msg->dtp_cfg = dtp_before.to_c_dtp_config();
	msg->dtcp_cfg = dtcp_before.to_c_dtcp_config();

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_kmsg_ipcp_conn_create_arrived message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_kmsg_ipcp_conn_create_arrived *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	             serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_kmsg_ipcp_conn_create_arrived message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	DTPConfig::from_c_dtp_config(dtp_after, resp->dtp_cfg);
	DTCPConfig::from_c_dtcp_config(dtcp_after, resp->dtcp_cfg);

	if (msg->port_id != resp->port_id) {
		std::cout << "Port-id on original and recovered messages"
			   << " are different\n";
		ret = 0;
	} else if (msg->dst_cep != resp->dst_cep) {
		std::cout << "Dest cep on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (msg->src_cep != resp->src_cep) {
		std::cout << "Src CEP on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (msg->qos_id != resp->qos_id) {
		std::cout << "Qos id on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (msg->src_addr != resp->src_addr) {
		std::cout << "Src address on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (msg->dst_addr != resp->dst_addr) {
		std::cout << "Dest address on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (msg->flow_user_ipc_process_id != resp->flow_user_ipc_process_id) {
		std::cout << "Flow user IPCP id on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (dtp_before != dtp_after) {
		std::cout << "DTP Config on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (dtcp_before != dtcp_after) {
		std::cout << "DTCP Config on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_kmsg_ipcp_conn_update(irati_msg_t msg_t)
{
	struct irati_kmsg_ipcp_conn_update * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;

	std::cout << "TESTING KMSG IPCP CONN UPDATE (" << msg_t << ")" << std::endl;

	msg = new irati_kmsg_ipcp_conn_update();
	msg->msg_type = msg_t;
	msg->port_id = 25;
	msg->dst_cep = 13;
	msg->src_cep = 2;

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_kmsg_ipcp_conn_update message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_kmsg_ipcp_conn_update *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	             serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_kmsg_ipcp_conn_update message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (msg->port_id != resp->port_id) {
		std::cout << "Port-id on original and recovered messages"
			   << " are different\n";
		ret = 0;
	} else if (msg->dst_cep != resp->dst_cep) {
		std::cout << "Dest cep on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (msg->src_cep != resp->src_cep) {
		std::cout << "Src CEP on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_kmsg_ipcp_select_ps_param()
{
	struct irati_kmsg_ipcp_select_ps_param * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;
	std::string p_before, p_after, n_before, n_after, v_before, v_after;

	std::cout << "TESTING KMSG IPCP SELECT PS PARAM" << std::endl;

	p_before = "/path/to/ps/param";
	n_before = "name";
	v_before = "value";

	msg = new irati_kmsg_ipcp_select_ps_param();
	msg->msg_type = RINA_C_IPCP_SET_POLICY_SET_PARAM_REQUEST;
	msg->path = stringToCharArray(p_before);
	msg->name = stringToCharArray(n_before);
	msg->value = stringToCharArray(v_before);

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_kmsg_ipcp_select_ps_param message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_kmsg_ipcp_select_ps_param *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	             serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_kmsg_ipcp_select_ps_param message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	p_after = resp->path;
	n_after = resp->name;
	v_after = resp->value;

	if (p_before != p_after) {
		std::cout << "Path on original and recovered messages"
			   << " are different\n";
		ret = 0;
	} else if (n_before != n_after) {
		std::cout << "Name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (v_before != v_after) {
		std::cout << "Value on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_kmsg_ipcp_select_ps()
{
	struct irati_kmsg_ipcp_select_ps * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;
	std::string p_before, p_after, n_before, n_after;

	std::cout << "TESTING KMSG IPCP SELECT PS" << std::endl;

	p_before = "/path/to/ps/param";
	n_before = "name";

	msg = new irati_kmsg_ipcp_select_ps();
	msg->msg_type = RINA_C_IPCP_SELECT_POLICY_SET_REQUEST;
	msg->path = stringToCharArray(p_before);
	msg->name = stringToCharArray(n_before);

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_kmsg_ipcp_select_ps message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_kmsg_ipcp_select_ps *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	             serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_kmsg_ipcp_select_ps message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	p_after = resp->path;
	n_after = resp->name;

	if (p_before != p_after) {
		std::cout << "Path on original and recovered messages"
			   << " are different\n";
		ret = 0;
	} else if (n_before != n_after) {
		std::cout << "Name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_kmsg_ipcp_update_crypto_state()
{
	struct irati_kmsg_ipcp_update_crypto_state * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;
	CryptoState before, after;

	std::cout << "TESTING KMSG IPCP UPDATE CRYPTO STATE" << std::endl;

	populate_crypto_state(before);

	msg = new irati_kmsg_ipcp_update_crypto_state();
	msg->msg_type = RINA_C_IPCP_UPDATE_CRYPTO_STATE_REQUEST;
	msg->port_id = 32;
	msg->state = before.to_c_crypto_state();

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_kmsg_ipcp_update_crypto_state message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_kmsg_ipcp_update_crypto_state *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	             	     serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_kmsg_ipcp_update_crypto_state message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	CryptoState::from_c_crypto_state(after, resp->state);

	if (msg->port_id != resp->port_id) {
		std::cout << "Port-id on original and recovered messages"
			   << " are different\n";
		ret = 0;
	} else if (before != after) {
		std::cout << "Crypto state on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_kmsg_ipcp_address_change()
{
	struct irati_kmsg_ipcp_address_change * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;

	std::cout << "TESTING KMSG IPCP ADDRESS CHANGE" << std::endl;

	msg = new irati_kmsg_ipcp_address_change();
	msg->msg_type = RINA_C_IPCP_ADDRESS_CHANGE_REQUEST;
	msg->new_address = 54;
	msg->old_address = 13;
	msg->use_new_timeout = 22;
	msg->deprecate_old_timeout = 3;

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_kmsg_ipcp_address_change message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_kmsg_ipcp_address_change *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	      serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_kmsg_ipcp_address_change message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (msg->new_address != resp->new_address) {
		std::cout << "New address on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (msg->old_address != resp->old_address) {
		std::cout << "Old address on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (msg->use_new_timeout != resp->use_new_timeout) {
		std::cout << "Use new timeout on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (msg->deprecate_old_timeout != resp->deprecate_old_timeout) {
		std::cout << "Deprecate old timeout on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_kmsg_ipcp_allocate_port()
{
	struct irati_kmsg_ipcp_allocate_port * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;
	ApplicationProcessNamingInformation before, after;

	std::cout << "TESTING KMSG IPCP ALLOCATE PORT" << std::endl;

	before.processName = "test/app";
	before.processInstance = "123";
	before.entityName = "database";
	before.entityInstance = "121";

	msg = new irati_kmsg_ipcp_allocate_port();
	msg->msg_type = RINA_C_IPCP_ALLOCATE_PORT_REQUEST;
	msg->app_name = before.to_c_name();

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_kmsg_ipcp_allocate_port message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_kmsg_ipcp_allocate_port *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	      serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_kmsg_ipcp_allocate_port message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	after = ApplicationProcessNamingInformation(resp->app_name);

	if (before != after) {
		std::cout << "App name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_kmsg_multi_msg(irati_msg_t msg_t)
{
	struct irati_kmsg_multi_msg * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;

	std::cout << "TESTING KMSG MULTI_MSG (" << msg_t << ")" << std::endl;

	msg = new irati_kmsg_multi_msg();
	msg->msg_type = msg_t;
	msg->port_id = 25;
	msg->cep_id = 13;
	msg->result = 2;
	msg->event_id = 13;

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_kmsg_ipcm_allocate_flow message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_kmsg_multi_msg *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	      serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_kmsg_multi_msg message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (msg->port_id != resp->port_id) {
		std::cout << "Port-id on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (msg->cep_id != resp->cep_id) {
		std::cout << "Cep-id on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (msg->result != resp->result) {
		std::cout << "Result on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (msg->event_id != resp->event_id) {
		std::cout << "Event id on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int compare_byte_arrays(unsigned char * a, unsigned char * b, uint32_t size)
{
	unsigned int i;

	for (i = 0; i< size; i++) {
		std::cout << "Comparing a[" << i << "] and b["
			  << i << "]: " << a[i]
			  << "; " << b[i] << std::endl;
		if (a[i] != b[i])
			return -1;
	}

	return 0;
}

int test_irati_kmsg_ipcp_mgmt_sdu(irati_msg_t msg_t)
{
	struct irati_kmsg_ipcp_mgmt_sdu * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;

	std::cout << "TESTING KMSG IPCP MGMT SDU (" << msg_t << ")" << std::endl;

	msg = new irati_kmsg_ipcp_mgmt_sdu();
	msg->msg_type = msg_t;
	msg->port_id = 25;
	msg->sdu = new buffer();
	msg->sdu->size = 19;
	msg->sdu->data = new unsigned char[19];
	msg->sdu->data[0] = 12;
	msg->sdu->data[1] = 1;
	msg->sdu->data[2] = 2;
	msg->sdu->data[3] = 34;
	msg->sdu->data[4] = 5;
	msg->sdu->data[5] = 87;
	msg->sdu->data[6] = 32;
	msg->sdu->data[7] = 98;
	msg->sdu->data[8] = 6;
	msg->sdu->data[9] = 99;
	msg->sdu->data[10] = 32;
	msg->sdu->data[11] = 65;
	msg->sdu->data[12] = 23;
	msg->sdu->data[13] = 71;
	msg->sdu->data[14] = 98;
	msg->sdu->data[15] = 15;
	msg->sdu->data[16] = 11;
	msg->sdu->data[17] = 22;
	msg->sdu->data[18] = 55;

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_kmsg_ipcp_mgmt_sdu message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_kmsg_ipcp_mgmt_sdu *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	      serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_kmsg_ipcp_mgmt_sdu message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (msg->port_id != resp->port_id) {
		std::cout << "Port-id on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (msg->sdu->size != resp->sdu->size) {
		std::cout << "SDU size on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (compare_byte_arrays(msg->sdu->data, resp->sdu->data, msg->sdu->size)) {
		std::cout << "Data on the byte arrays"
			   << " is different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_kmsg_ipcm_create_ipcp()
{
	struct irati_kmsg_ipcm_create_ipcp * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;
	ApplicationProcessNamingInformation n_before, n_after;
	std::string t_before, t_after;

	std::cout << "TESTING KMSG IPCM CREATE IPCP" << std::endl;

	n_before.processName = "test.irati";
	n_before.processInstance = "1";
	t_before = "normal-ipcp";

	msg = new irati_kmsg_ipcm_create_ipcp();
	msg->msg_type = RINA_C_IPCM_CREATE_IPCP_REQUEST;
	msg->ipcp_id = 23;
	msg->irati_port_id = 54;
	msg->ipcp_name = n_before.to_c_name();
	msg->dif_type = stringToCharArray(t_before);

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_kmsg_ipcm_create_ipcp message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_kmsg_ipcm_create_ipcp *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	      serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_kmsg_ipcm_create_ipcp message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	n_after = ApplicationProcessNamingInformation(resp->ipcp_name);
	t_after = resp->dif_type;

	if (msg->ipcp_id != resp->ipcp_id) {
		std::cout << "IPCP-id on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (msg->irati_port_id != resp->irati_port_id) {
		std::cout << "IRATI port-id on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (t_before != t_after) {
		std::cout << "DIF type on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (n_before != n_after) {
		std::cout << "IPCP name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_kmsg_ipcm_destroy_ipcp()
{
	struct irati_kmsg_ipcm_destroy_ipcp * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;

	std::cout << "TESTING KMSG IPCM DESTROY IPCP" << std::endl;

	msg = new irati_kmsg_ipcm_destroy_ipcp();
	msg->msg_type = RINA_C_IPCM_DESTROY_IPCP_REQUEST;
	msg->ipcp_id = 23;

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_kmsg_ipcm_destroy_ipcp message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_kmsg_ipcm_destroy_ipcp *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	      serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_kmsg_ipcm_destroy_ipcp message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (msg->ipcp_id != resp->ipcp_id) {
		std::cout << "IPCP-id on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_msg_ipcm_enroll_to_dif()
{
	struct irati_msg_ipcm_enroll_to_dif * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;
	ApplicationProcessNamingInformation dn_before, dn_after, sdn_before,
			sdn_after, nn_before, nn_after, dnn_before, dnn_after;

	std::cout << "TESTING KMSG IPCM ENROLL TO DIF" << std::endl;

	dn_before.processName = "test.DIF";
	sdn_before.processName = "shim.DIF";
	nn_before.processName = "test1.IRATI";
	nn_before.processInstance = "1";
	dnn_before.processName = "test2.IRATI";
	dnn_before.processInstance = "1";

	msg = new irati_msg_ipcm_enroll_to_dif();
	msg->msg_type = RINA_C_IPCM_ENROLL_TO_DIF_REQUEST;
	msg->prepare_for_handover = true;
	msg->dif_name = dn_before.to_c_name();
	msg->sup_dif_name = sdn_before.to_c_name();
	msg->neigh_name = nn_before.to_c_name();
	msg->disc_neigh_name = dnn_before.to_c_name();

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_msg_ipcm_enroll_to_dif message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_msg_ipcm_enroll_to_dif *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	      serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_msg_ipcm_enroll_to_dif message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	dn_after = ApplicationProcessNamingInformation(resp->dif_name);
	sdn_after = ApplicationProcessNamingInformation(resp->sup_dif_name);
	nn_after = ApplicationProcessNamingInformation(resp->neigh_name);
	dnn_after = ApplicationProcessNamingInformation(resp->disc_neigh_name);

	if (msg->prepare_for_handover != resp->prepare_for_handover) {
		std::cout << "Prepare for handover on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (dn_before != dn_after) {
		std::cout << "DIF name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (sdn_before != sdn_after) {
		std::cout << "Supporting DIF name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (nn_before != nn_after) {
		std::cout << "Neighbor name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (dnn_before != dnn_after) {
		std::cout << "Disconnect neighbor name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_msg_ipcm_enroll_to_dif_resp()
{
	struct irati_msg_ipcm_enroll_to_dif_resp * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;
	ApplicationProcessNamingInformation dn_before, dn_after;
	DIFConfiguration c_before, c_after;
	std::string t_before, t_after;
	Neighbor n_before, n_after;
	struct ipcp_neighbor_entry * ndata, * pos;
	int num_entries = 0;

	std::cout << "TESTING KMSG IPCM ENROLL TO DIF RESP" << std::endl;

	dn_before.processName = "test.DIF";
	populate_dif_config(c_before);
	t_before = "normal-ipcp";
	populate_neighbor(n_before);

	msg = new irati_msg_ipcm_enroll_to_dif_resp();
	msg->msg_type = RINA_C_IPCM_ENROLL_TO_DIF_RESPONSE;
	msg->result = 23;
	msg->dif_name = dn_before.to_c_name();
	msg->dif_type = stringToCharArray(t_before);
	msg->dif_config = c_before.to_c_dif_config();
	msg->neighbors = ipcp_neigh_list_create();
	ndata = new ipcp_neighbor_entry();
	INIT_LIST_HEAD(&ndata->next);
	ndata->entry = n_before.to_c_neighbor();
	list_add_tail(&ndata->next, &msg->neighbors->ipcp_neighbors);

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_msg_ipcm_enroll_to_dif_resp message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_msg_ipcm_enroll_to_dif_resp *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	      serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_msg_ipcm_enroll_to_dif_resp message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	dn_after = ApplicationProcessNamingInformation(resp->dif_name);
	DIFConfiguration::from_c_dif_config(c_after, resp->dif_config);
	t_after = resp->dif_type;

	ndata = 0;
        list_for_each_entry(pos, &(resp->neighbors->ipcp_neighbors), next) {
                num_entries ++;
                ndata = pos;
        }

        if (ndata)
        	Neighbor::from_c_neighbor(n_after, ndata->entry);

	if (msg->result != resp->result) {
		std::cout << "Result on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (dn_before != dn_after) {
		std::cout << "DIF name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (c_before != c_after) {
		std::cout << "DIF configuration on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (t_before != t_after) {
		std::cout << "Type on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (num_entries != 1) {
		std::cout << "Number of neighbors on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (n_before != n_after) {
		std::cout << "Neighbor on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_msg_with_name(irati_msg_t msg_t)
{
	struct irati_msg_with_name * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;
	ApplicationProcessNamingInformation n_before, n_after;

	std::cout << "TESTING MSG WITH NAME (" << msg_t << ")" << std::endl;

	n_before.processName = "test.irati";
	n_before.processInstance = "1";
	n_before.entityName = "fa";
	n_before.entityInstance = "23";

	msg = new irati_msg_with_name();
	msg->msg_type = msg_t;
	msg->name = n_before.to_c_name();

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_msg_with_name message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_msg_with_name *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	      serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_msg_with_name message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	n_after = ApplicationProcessNamingInformation(resp->name);

	if (n_before != n_after) {
		std::cout << "IPCP name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_msg_app_alloc_flow_result()
{
	struct irati_msg_app_alloc_flow_result * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;
	ApplicationProcessNamingInformation san_before, san_after,
					    dn_before, dn_after;
	std::string e_before, e_after;

	std::cout << "TESTING KMSG APP ALLOCATE FLOW RESULT" << std::endl;

	dn_before.processName = "test.DIF";
	san_before.processName = "test1.IRATI";
	san_before.processInstance = "1";
	san_before.entityName = "fa";
	san_before.entityInstance = "1";
	e_before = "my error";

	msg = new irati_msg_app_alloc_flow_result();
	msg->msg_type = RINA_C_APP_ALLOCATE_FLOW_REQUEST_RESULT;
	msg->port_id = 31;
	msg->dif_name = dn_before.to_c_name();
	msg->source_app_name = san_before.to_c_name();
	msg->error_desc = stringToCharArray(e_before);

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_msg_app_alloc_flow_result message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_msg_app_alloc_flow_result *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	      serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_msg_app_alloc_flow_result message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	dn_after = ApplicationProcessNamingInformation(resp->dif_name);
	san_after = ApplicationProcessNamingInformation(resp->source_app_name);
	e_after = resp->error_desc;

	if (msg->port_id != resp->port_id) {
		std::cout << "Port-id on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (dn_before != dn_after) {
		std::cout << "DIF name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (san_before != san_after) {
		std::cout << "Source app name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (e_before != e_after) {
		std::cout << "Error description on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_msg_app_alloc_flow_response()
{
	struct irati_msg_app_alloc_flow_response * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;

	std::cout << "TESTING KMSG APP ALLOCATE FLOW RESPONSE" << std::endl;

	msg = new irati_msg_app_alloc_flow_response();
	msg->msg_type = RINA_C_APP_ALLOCATE_FLOW_RESPONSE;
	msg->result = 43;
	msg->not_source = true;

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_msg_app_alloc_flow_response message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_msg_app_alloc_flow_response *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	      serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_msg_app_alloc_flow_response message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (msg->result != resp->result) {
		std::cout << "Result on  original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (msg->not_source != resp->not_source) {
		std::cout << "Notify source on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_msg_app_dealloc_flow(irati_msg_t msg_t)
{
	struct irati_msg_app_dealloc_flow * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;

	std::cout << "TESTING KMSG APP DEALLOCATE FLOW (" << msg_t << ")" << std::endl;

	msg = new irati_msg_app_dealloc_flow();
	msg->msg_type = msg_t;
	msg->port_id = 31;
	msg->result = 35;

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_msg_app_dealloc_flow message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_msg_app_dealloc_flow *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	      serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_msg_app_dealloc_flow message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (msg->port_id != resp->port_id) {
		std::cout << "Port-id on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (msg->result != resp->result) {
		std::cout << "Result on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_msg_app_reg_app()
{
	struct irati_msg_app_reg_app * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;
	ApplicationProcessNamingInformation a_before, a_after, da_before,
					da_after, d_before, d_after;

	std::cout << "TESTING KMSG APP REGISTER APP" << std::endl;

	a_before.processName = "test1.IRATI";
	a_before.processInstance = "1";
	a_before.entityName = "fa";
	a_before.entityInstance = "1";
	da_before.processName = "mydaf";
	d_before.processName = "test.DIF";

	msg = new irati_msg_app_reg_app();
	msg->msg_type = RINA_C_APP_REGISTER_APPLICATION_REQUEST;
	msg->ipcp_id = 31;
	msg->reg_type = 2;
	msg->app_name = a_before.to_c_name();
	msg->daf_name = da_before.to_c_name();
	msg->dif_name = d_before.to_c_name();

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_msg_app_reg_app message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_msg_app_reg_app *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	      serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_msg_app_reg_app message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	a_after = ApplicationProcessNamingInformation(resp->app_name);
	da_after = ApplicationProcessNamingInformation(resp->daf_name);
	d_after = ApplicationProcessNamingInformation(resp->dif_name);

	if (msg->ipcp_id != resp->ipcp_id) {
		std::cout << "IPCP id on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (msg->reg_type != resp->reg_type) {
		std::cout << "Registration type on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (a_before != a_after) {
		std::cout << "App name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (da_before != da_after) {
		std::cout << "DAF name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (d_before != d_after) {
		std::cout << "DIF name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_msg_app_reg_app_resp(irati_msg_t msg_t)
{
	struct irati_msg_app_reg_app_resp * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;
	ApplicationProcessNamingInformation a_before, a_after,
					d_before, d_after;

	std::cout << "TESTING KMSG APP REGISTER APP RESP ("<<msg_t<<")" << std::endl;

	a_before.processName = "test1.IRATI";
	a_before.processInstance = "1";
	a_before.entityName = "fa";
	a_before.entityInstance = "1";
	d_before.processName = "test.DIF";

	msg = new irati_msg_app_reg_app_resp();
	msg->msg_type = msg_t;
	msg->result = 89;
	msg->app_name = a_before.to_c_name();
	msg->dif_name = d_before.to_c_name();

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_msg_app_reg_app_resp message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_msg_app_reg_app_resp *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	      serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_msg_app_reg_app_resp message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	a_after = ApplicationProcessNamingInformation(resp->app_name);
	d_after = ApplicationProcessNamingInformation(resp->dif_name);

	if (msg->result != resp->result) {
		std::cout << "Result on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (a_before != a_after) {
		std::cout << "App name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (d_before != d_after) {
		std::cout << "DIF name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_msg_app_reg_cancel()
{
	struct irati_msg_app_reg_cancel * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;
	ApplicationProcessNamingInformation a_before, a_after,
					d_before, d_after;
	std::string res_before, res_after;

	std::cout << "TESTING KMSG APP REGISTER APP CANCEL" << std::endl;

	a_before.processName = "test1.IRATI";
	a_before.processInstance = "1";
	a_before.entityName = "fa";
	a_before.entityInstance = "1";
	d_before.processName = "test.DIF";
	res_before = "my reason";

	msg = new irati_msg_app_reg_cancel();
	msg->msg_type = RINA_C_APP_APPLICATION_REGISTRATION_CANCELED_NOTIFICATION;
	msg->code = 89;
	msg->app_name = a_before.to_c_name();
	msg->dif_name = d_before.to_c_name();
	msg->reason = stringToCharArray(res_before);

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_msg_app_reg_cancel message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_msg_app_reg_cancel *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	      serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_msg_app_reg_cancel message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	a_after = ApplicationProcessNamingInformation(resp->app_name);
	d_after = ApplicationProcessNamingInformation(resp->dif_name);
	res_after = resp->reason;

	if (msg->code != resp->code) {
		std::cout << "Code on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (a_before != a_after) {
		std::cout << "App name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (d_before != d_after) {
		std::cout << "DIF name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (res_before != res_after) {
		std::cout << "Reason on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_msg_get_dif_prop()
{
	struct irati_msg_get_dif_prop * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;
	ApplicationProcessNamingInformation a_before, a_after,
					d_before, d_after ,d2_after;
	struct dif_properties_entry * data, * pos;
	int num_entries = 0;

	std::cout << "TESTING KMSG APP GET DIF PROPERTIES" << std::endl;

	a_before.processName = "test1.IRATI";
	a_before.processInstance = "1";
	a_before.entityName = "fa";
	a_before.entityInstance = "1";
	d_before.processName = "test.DIF";

	msg = new irati_msg_get_dif_prop();
	msg->msg_type = RINA_C_APP_GET_DIF_PROPERTIES_RESPONSE;
	msg->code = 89;
	msg->app_name = a_before.to_c_name();
	msg->dif_name = d_before.to_c_name();
	msg->dif_props = get_dif_prop_resp_create();
	data = dif_properties_entry_create();
	data->dif_name = d_before.to_c_name();
	data->max_sdu_size = 15;
	list_add_tail(&data->next, &msg->dif_props->dif_propery_entries);

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_msg_get_dif_prop message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_msg_get_dif_prop *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	      serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_msg_get_dif_prop message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	a_after = ApplicationProcessNamingInformation(resp->app_name);
	d_after = ApplicationProcessNamingInformation(resp->dif_name);
	data = 0;
        list_for_each_entry(pos, &(resp->dif_props->dif_propery_entries), next) {
                num_entries ++;
                data = pos;
        }

        if (data)
        	d2_after = ApplicationProcessNamingInformation(data->dif_name);

	if (msg->code != resp->code) {
		std::cout << "Code on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (a_before != a_after) {
		std::cout << "App name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (d_before != d_after) {
		std::cout << "DIF name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (num_entries != 1) {
		std::cout << "Number of entries on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (d_before != d2_after) {
		std::cout << "DIF name property on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_msg_ipcm_plugin_load()
{
	struct irati_msg_ipcm_plugin_load * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;
	std::string before, after;

	std::cout << "TESTING KMSG IPCM PLUGIN LOAD" << std::endl;

	before = "my plugin name";

	msg = new irati_msg_ipcm_plugin_load();
	msg->msg_type = RINA_C_IPCM_PLUGIN_LOAD_REQUEST;
	msg->load = true;
	msg->plugin_name = stringToCharArray(before);

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_msg_ipcm_plugin_load message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_msg_ipcm_plugin_load *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	      serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_msg_ipcm_plugin_load message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	after = resp->plugin_name;

	if (msg->load != resp->load) {
		std::cout << "Load on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (before != after) {
		std::cout << "Plugin name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_msg_ipcm_fwd_cdap_msg(irati_msg_t msg_t)
{
	struct irati_msg_ipcm_fwd_cdap_msg * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;

	std::cout << "TESTING KMSG IPCM FWD CDAP MSG (" << msg_t << ")" << std::endl;

	msg = new irati_msg_ipcm_fwd_cdap_msg();
	msg->msg_type = msg_t;
	msg->result = 25;
	msg->cdap_msg = new buffer();
	msg->cdap_msg->size = 19;
	msg->cdap_msg->data = new unsigned char[19];
	memset(msg->cdap_msg->data, 34, 19);

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_msg_ipcm_fwd_cdap_msg message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_msg_ipcm_fwd_cdap_msg *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	      serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_msg_ipcm_fwd_cdap_msg message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (msg->result != resp->result) {
		std::cout << "Result on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (msg->cdap_msg->size != resp->cdap_msg->size) {
		std::cout << "CDAP msg size on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int test_irati_msg_ipcm_media_report()
{
	struct irati_msg_ipcm_media_report * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;
	MediaReport m_before, m_after;
	MediaDIFInfo md_before, md_after;
	BaseStationInfo bs_before, bs_after;

	std::cout << "TESTING KMSG IPCM MEDIA REPORT" << std::endl;

	bs_before.ipcp_address = "3434";
	bs_before.signal_strength = 5343;
	md_before.dif_name = "test.DIF";
	md_before.security_policies = "my security policies";
	md_before.available_bs_ipcps.push_back(bs_before);
	m_before.bs_ipcp_address = "451412";
	m_before.current_dif_name = "test.DIF";
	m_before.ipcp_id = 3;
	m_before.available_difs["test.DIF"] = md_before;

	msg = new irati_msg_ipcm_media_report();
	msg->msg_type = RINA_C_IPCM_MEDIA_REPORT;
	msg->report = m_before.to_c_media_report();

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_msg_ipcm_media_report message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_msg_ipcm_media_report *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	      serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_msg_ipcm_media_report message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	MediaReport::from_c_media_report(m_after, resp->report);
	md_after = m_after.available_difs["test.DIF"];
	bs_after = md_after.available_bs_ipcps.front();

	if (m_before != m_after) {
		std::cout << "Media report on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (md_after != md_before) {
		std::cout << "Media DIF Info on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (bs_before != bs_after) {
		std::cout << "Base station info name on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else {
		std::cout << "Test ok!" << std::endl;
		ret = 0;
	}

	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return ret;
}

int main()
{
	int result;

	std::cout << "TESTING LIBRINA-PARSERS\n";

	result = test_irati_kmsg_ipcm_assign_to_dif();
	if (result < 0) return result;

	result = test_irati_kmsg_ipcp_mgmt_sdu(RINA_C_IPCP_MANAGEMENT_SDU_WRITE_REQUEST);
	if (result < 0) return result;

	result = test_irati_kmsg_ipcp_mgmt_sdu(RINA_C_IPCP_MANAGEMENT_SDU_READ_NOTIF);
	if (result < 0) return result;

	result = test_irati_msg_base_resp(RINA_C_IPCM_PLUGIN_LOAD_RESPONSE);
	if (result < 0) return result;

	result = test_irati_msg_base_resp(RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_RESPONSE);
	if (result < 0) return result;

	result = test_irati_msg_base_resp(RINA_C_IPCM_DESTROY_IPCP_RESPONSE);
	if (result < 0) return result;

	result = test_irati_msg_base_resp(RINA_C_IPCM_CREATE_IPCP_RESPONSE);
	if (result < 0) return result;

	result = test_irati_msg_base_resp(RINA_C_IPCP_MANAGEMENT_SDU_WRITE_RESPONSE);
	if (result < 0) return result;

	result = test_irati_msg_base_resp(RINA_C_IPCP_SET_POLICY_SET_PARAM_RESPONSE);
	if (result < 0) return result;

	result = test_irati_msg_base_resp(RINA_C_IPCP_SELECT_POLICY_SET_RESPONSE);
	if (result < 0) return result;

	result = test_irati_msg_base_resp(RINA_C_IPCM_UNREGISTER_APPLICATION_RESPONSE);
	if (result < 0) return result;

	result = test_irati_msg_base_resp(RINA_C_IPCM_REGISTER_APPLICATION_RESPONSE);
	if (result < 0) return result;

	result = test_irati_msg_base_resp(RINA_C_IPCM_UPDATE_DIF_CONFIG_RESPONSE);
	if (result < 0) return result;

	result = test_irati_msg_base_resp(RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE);
	if (result < 0) return result;

	result = test_irati_kmsg_ipcm_update_config();
	if (result < 0) return result;

	result = test_irati_kmsg_ipcp_dif_reg_not(RINA_C_IPCM_IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION);
	if (result < 0) return result;

	result = test_irati_kmsg_ipcp_dif_reg_not(RINA_C_IPCM_IPC_PROCESS_DIF_UNREGISTRATION_NOTIFICATION);
	if (result < 0) return result;

	result = test_irati_kmsg_ipcm_allocate_flow(RINA_C_IPCM_ALLOCATE_FLOW_REQUEST);
	if (result < 0) return result;

	result = test_irati_kmsg_ipcm_allocate_flow(RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_ARRIVED);
	if (result < 0) return result;

	result = test_irati_kmsg_ipcm_allocate_flow(RINA_C_APP_ALLOCATE_FLOW_REQUEST);
	if (result < 0) return result;

	result = test_irati_kmsg_ipcm_allocate_flow(RINA_C_APP_ALLOCATE_FLOW_REQUEST_ARRIVED);
	if (result < 0) return result;

	result = test_irati_kmsg_ipcm_allocate_flow_resp();
	if (result < 0) return result;

	result = test_irati_kmsg_ipcm_reg_app();
	if (result < 0) return result;

	result = test_irati_kmsg_ipcm_unreg_app();
	if (result < 0) return result;

	result =  test_irati_kmsg_ipcm_query_rib();
	if (result < 0) return result;

	result = test_irati_kmsg_ipcm_query_rib_resp();
	if (result < 0) return result;

	result = test_irati_kmsg_multi_msg(RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_RESULT);
	if (result < 0) return result;

	result = test_irati_kmsg_multi_msg(RINA_C_IPCM_DEALLOCATE_FLOW_REQUEST);
	if (result < 0) return result;

	result = test_irati_kmsg_multi_msg(RINA_C_IPCM_FLOW_DEALLOCATED_NOTIFICATION);
	if (result < 0) return result;

	result = test_irati_kmsg_multi_msg(RINA_C_IPCP_CONN_UPDATE_RESULT);
	if (result < 0) return result;

	result = test_irati_kmsg_multi_msg(RINA_C_IPCP_CONN_DESTROY_REQUEST);
	if (result < 0) return result;

	result = test_irati_kmsg_multi_msg(RINA_C_IPCP_CONN_DESTROY_RESULT);
	if (result < 0) return result;

	result = test_irati_kmsg_multi_msg(RINA_C_IPCP_UPDATE_CRYPTO_STATE_RESPONSE);
	if (result < 0) return result;

	result = test_irati_kmsg_multi_msg(RINA_C_IPCP_ALLOCATE_PORT_RESPONSE);
	if (result < 0) return result;

	result = test_irati_kmsg_multi_msg(RINA_C_IPCP_DEALLOCATE_PORT_REQUEST);
	if (result < 0) return result;

	result = test_irati_kmsg_multi_msg(RINA_C_IPCP_DEALLOCATE_PORT_RESPONSE);
	if (result < 0) return result;

	result = test_irati_msg_base(RINA_C_IPCM_FINALIZE_REQUEST);
	if (result < 0) return result;

	result = test_irati_msg_base(RINA_C_RMT_DUMP_FT_REQUEST);
	if (result < 0) return result;

	result = test_irati_kmsg_rmt_dump_ft(RINA_C_RMT_MODIFY_FTE_REQUEST);
	if (result < 0) return result;

	/*result = test_irati_kmsg_rmt_dump_ft(RINA_C_RMT_DUMP_FT_REPLY);
	if (result < 0) return result;*/

	result = test_irati_kmsg_ipcp_conn_create_arrived(RINA_C_IPCP_CONN_CREATE_REQUEST);
	if (result < 0) return result;

	result = test_irati_kmsg_ipcp_conn_create_arrived(RINA_C_IPCP_CONN_CREATE_ARRIVED);
	if (result < 0) return result;

	result = test_irati_kmsg_ipcp_conn_update(RINA_C_IPCP_CONN_CREATE_RESPONSE);
	if (result < 0) return result;

	result = test_irati_kmsg_ipcp_conn_update(RINA_C_IPCP_CONN_CREATE_RESULT);
	if (result < 0) return result;

	result = test_irati_kmsg_ipcp_conn_update(RINA_C_IPCP_CONN_UPDATE_REQUEST);
	if (result < 0) return result;

	result = test_irati_kmsg_ipcp_select_ps_param();
	if (result < 0) return result;

	result = test_irati_kmsg_ipcp_select_ps();
	if (result < 0) return result;

	result = test_irati_kmsg_ipcp_update_crypto_state();
	if (result < 0) return result;

	result = test_irati_kmsg_ipcp_address_change();
	if (result < 0) return result;

	result = test_irati_kmsg_ipcp_allocate_port();
	if (result < 0) return result;

	result = test_irati_kmsg_ipcm_create_ipcp();
	if (result < 0) return result;

	result = test_irati_kmsg_ipcm_destroy_ipcp();
	if (result < 0) return result;

	result = test_irati_msg_ipcm_enroll_to_dif();
	if (result < 0) return result;

	result = test_irati_msg_ipcm_enroll_to_dif_resp();
	if (result < 0) return result;

	result = test_irati_msg_with_name(RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_REQUEST);
	if (result < 0) return result;

	result = test_irati_msg_with_name(RINA_C_IPCM_IPC_PROCESS_INITIALIZED);
	if (result < 0) return result;

	result = test_irati_msg_app_alloc_flow_result();
	if (result < 0) return result;

	result = test_irati_msg_app_alloc_flow_response();
	if (result < 0) return result;

	result = test_irati_msg_app_dealloc_flow(RINA_C_APP_DEALLOCATE_FLOW_REQUEST);
	if (result < 0) return result;

	result = test_irati_msg_app_reg_app();
	if (result < 0) return result;

	result = test_irati_msg_app_reg_app_resp(RINA_C_APP_REGISTER_APPLICATION_RESPONSE);
	if (result < 0) return result;

	result = test_irati_msg_app_reg_app_resp(RINA_C_APP_UNREGISTER_APPLICATION_REQUEST);
	if (result < 0) return result;

	result = test_irati_msg_app_reg_app_resp(RINA_C_APP_UNREGISTER_APPLICATION_RESPONSE);
	if (result < 0) return result;

	result = test_irati_msg_app_reg_app_resp(RINA_C_APP_GET_DIF_PROPERTIES_REQUEST);
	if (result < 0) return result;

	result = test_irati_msg_app_reg_cancel();
	if (result < 0) return result;

	result = test_irati_msg_get_dif_prop();
	if (result < 0) return result;

	result = test_irati_msg_ipcm_plugin_load();
	if (result < 0) return result;

	result = test_irati_msg_ipcm_fwd_cdap_msg(RINA_C_IPCM_FWD_CDAP_MSG_REQUEST);
	if (result < 0) return result;

	result = test_irati_msg_ipcm_fwd_cdap_msg(RINA_C_IPCM_FWD_CDAP_MSG_RESPONSE);
	if (result < 0) return result;

	result = test_irati_msg_ipcm_media_report();
	if (result < 0) return result;

	return 0;
}
