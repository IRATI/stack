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

#define DEFAULT_AP_NAME        "default/apname"
#define DEFAULT_AP_INSTANCE    "default/apinstance"
#define DEFAULT_AE_NAME        "default/aename"
#define DEFAULT_AE_INSTANCE    "default/aeinstance"
#define DEFAULT_POLICY_NAME    "default"
#define DEFAULT_POLICY_VERSION "1"
#define DEFAULT_DIF_NAME       "default.DIF"

using namespace rina;

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
	msg->source = s_before.to_c_name();
	msg->dest = d_before.to_c_name();
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

	s_after = ApplicationProcessNamingInformation(resp->source);
	d_after = ApplicationProcessNamingInformation(resp->dest);
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
	struct mod_pff_entry * modata, * pos, * modata_after;
	struct port_id_altlist * podata, * ppos, * podata_after;
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

	modata_after = 0;
	podata_after = 0;
        list_for_each_entry(pos, &(resp->pft_entries->pff_entries), next) {
                num_entries ++;
                modata_after = pos;
        }

        list_for_each_entry(ppos, &(modata_after->port_id_altlists), next) {
                num_entries2 ++;
                podata_after = ppos;
        }

	if (msg->result != resp->result) {
		std::cout << "Result on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (msg->mode != resp->mode) {
		std::cout << "Mode on original and recovered messages"
			   << " are different\n";
		ret = -1;
	} else if (num_entries != 1 || num_entries2 != 1) {
		std::cout << "Number of entries on original and recovered "
				<< "messages are different\n";
		ret = -1;
	} else if (modata->cost != modata_after->cost) {
		std::cout << "Cost on original and recovered "
				<< "messages are different\n";
		ret = -1;
	} else if (modata->fwd_info != modata_after->fwd_info) {
		std::cout << "Address on original and recovered "
				<< "messages are different\n";
		ret = -1;
	} else if (modata->qos_id != modata_after->qos_id) {
		std::cout << "Qos-id on original and recovered "
				<< "messages are different\n";
		ret = -1;
	} else if (podata->num_ports != podata_after->num_ports) {
		std::cout << "Num ports on original and recovered "
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

	result = test_irati_msg_base_resp(RINA_C_IPCM_DEALLOCATE_FLOW_RESPONSE);
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

	result = test_irati_msg_base(RINA_C_IPCM_IPC_MANAGER_PRESENT);
	if (result < 0) return result;

	result = test_irati_msg_base(RINA_C_IPCM_SOCKET_CLOSED_NOTIFICATION);
	if (result < 0) return result;

	result = test_irati_msg_base(RINA_C_RMT_DUMP_FT_REQUEST);
	if (result < 0) return result;

	result = test_irati_kmsg_rmt_dump_ft(RINA_C_RMT_MODIFY_FTE_REQUEST);
	if (result < 0) return result;

	result = test_irati_kmsg_rmt_dump_ft(RINA_C_RMT_DUMP_FT_REPLY);
	if (result < 0) return result;

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


	return 0;
}
