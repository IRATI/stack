//
// Classes related to RINA configuration
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

#include <sstream>

#define RINA_PREFIX "librina.configuration"

#include "librina/configuration.h"
#include "librina/logs.h"

namespace rina {

// CLASS POLICY PAREMETER
PolicyParameter::PolicyParameter(){
}

PolicyParameter::PolicyParameter(const std::string& name,
                const std::string& value){
        name_ = name;
        value_ = value;
}

bool PolicyParameter::operator==(const PolicyParameter &other) const {
        return other.get_name().compare(name_) == 0;
}

bool PolicyParameter::operator!=(const PolicyParameter &other) const {
        return !(*this == other);
}

const std::string& PolicyParameter::get_name() const {
        return name_;
}

void PolicyParameter::set_name(const std::string& name) {
        name_ = name;
}

const std::string& PolicyParameter::get_value() const {
        return value_;
}

void PolicyParameter::set_value(const std::string& value) {
        value_ = value;
}

// CLASS POLICY CONFIGURATION
PolicyConfig::PolicyConfig() {
        name_ = RINA_DEFAULT_POLICY_NAME;
        version_ = RINA_DEFAULT_POLICY_VERSION;
}

PolicyConfig::PolicyConfig(const std::string& name,
                const std::string& version) {
        name_ = name;
        version_ = version;
}

bool PolicyConfig::operator==(const PolicyConfig &other) const {
        return other.get_name().compare(name_) == 0 &&
                        other.get_version().compare(version_) == 0;
}

bool PolicyConfig::operator!=(const PolicyConfig &other) const {
        return !(*this == other);
}

const std::string& PolicyConfig::get_name() const {
        return name_;
}

void PolicyConfig::set_name(const std::string& name) {
        name_ = name;
}

const std::list<PolicyParameter>&
PolicyConfig::get_parameters() const {
        return parameters_;
}

void PolicyConfig::set_parameters(
                const std::list<PolicyParameter>& parameters) {
        parameters_ = parameters;
}

void PolicyConfig::add_parameter(const PolicyParameter& paremeter) {
        parameters_.push_back(paremeter);
}

const std::string& PolicyConfig::get_version() const {
        return version_;
}

void PolicyConfig::set_version(const std::string& version) {
        version_ = version;
}

// CLASS DTCP WINDOW-BASED FLOW CONTROL CONFIG
DTCPWindowBasedFlowControlConfig::DTCPWindowBasedFlowControlConfig() {
        initial_credit_ = 0;
        max_closed_window_queue_length_ = 0;
}

unsigned int DTCPWindowBasedFlowControlConfig::get_initial_credit() const {
	return initial_credit_;
}

void DTCPWindowBasedFlowControlConfig::set_initial_credit(int initial_credit){
	initial_credit_ = initial_credit;
}

unsigned int DTCPWindowBasedFlowControlConfig::get_maxclosed_window_queue_length() const {
	return max_closed_window_queue_length_;
}

void DTCPWindowBasedFlowControlConfig::set_max_closed_window_queue_length(
		unsigned int max_closed_window_queue_length) {
	max_closed_window_queue_length_ = max_closed_window_queue_length;
}

const PolicyConfig& DTCPWindowBasedFlowControlConfig::get_rcvr_flow_control_policy() const {
	return rcvr_flow_control_policy_;
}

void DTCPWindowBasedFlowControlConfig::set_rcvr_flow_control_policy(
                    const PolicyConfig& rcvr_flow_control_policy) {
	rcvr_flow_control_policy_ = rcvr_flow_control_policy;
}

const PolicyConfig& DTCPWindowBasedFlowControlConfig::getTxControlPolicy() const {
	return tx_control_policy_;
}

void DTCPWindowBasedFlowControlConfig::set_tx_control_policy(
		const PolicyConfig& tx_control_policy) {
	tx_control_policy_ = tx_control_policy;
}

const std::string DTCPWindowBasedFlowControlConfig::toString() {
        std::stringstream ss;
        ss<<"Max closed window queue length: "<<max_closed_window_queue_length_;
        ss<<"; Initial credit (PDUs): "<<initial_credit_<<std::endl;
        ss<<"Receiver flow control policy (name/version): "<<rcvr_flow_control_policy_.get_name();
        ss<<"/"<<rcvr_flow_control_policy_.get_version();
        ss<<"; Transmission control policy (name/version): "<<tx_control_policy_.get_name();
        ss<<"/"<<tx_control_policy_.get_version();
        return ss.str();
}

// CLASS DTCP RATE-BASED FLOW CONTROL CONFIG
DTCPRateBasedFlowControlConfig::DTCPRateBasedFlowControlConfig() {
        sending_rate_ = 0;
        time_period_ = 0;
}

const PolicyConfig& DTCPRateBasedFlowControlConfig::get_no_override_default_peak_policy() const {
	return no_override_default_peak_policy_;
}

void DTCPRateBasedFlowControlConfig::set_no_override_default_peak_policy(
                const PolicyConfig& no_override_default_peak_policy) {
	no_override_default_peak_policy_ = no_override_default_peak_policy;
}

const PolicyConfig& DTCPRateBasedFlowControlConfig::get_no_rate_slow_down_policy() const {
	return no_rate_slow_down_policy_;
}

void DTCPRateBasedFlowControlConfig::set_no_rate_slow_down_policy(
                const PolicyConfig& no_rate_slow_down_policy) {
	no_rate_slow_down_policy_ = no_rate_slow_down_policy;
}

const PolicyConfig& DTCPRateBasedFlowControlConfig::get_rate_reduction_policy() const {
	return rate_reduction_policy_;
}

void DTCPRateBasedFlowControlConfig::set_rate_reduction_policy(
		const PolicyConfig& rate_reduction_policy) {
	rate_reduction_policy_ = rate_reduction_policy;
}

unsigned int DTCPRateBasedFlowControlConfig::get_sending_rate() const {
	return sending_rate_;
}

void DTCPRateBasedFlowControlConfig::set_sending_rate(unsigned int sending_rate) {
	sending_rate_ = sending_rate;
}

unsigned int DTCPRateBasedFlowControlConfig::get_time_period() const {
	return time_period_;
}

void DTCPRateBasedFlowControlConfig::set_time_period(unsigned int time_period) {
	time_period_ = time_period;
}

const std::string DTCPRateBasedFlowControlConfig::toString() {
        std::stringstream ss;
        ss<<"Sending rate (PDUs/time period): "<<sending_rate_;
        ss<<"; Time period (in microseconds): "<<time_period_<<std::endl;
        ss<<"No rate slowdown policy (name/version): "<<no_rate_slow_down_policy_.get_name();
        ss<<"/"<<no_rate_slow_down_policy_.get_version();
        ss<<"; No override default peak policy (name/version): "<<no_override_default_peak_policy_.get_name();
        ss<<"/"<<no_override_default_peak_policy_.get_version()<<std::endl;
        ss<<"Rate reduction policy (name/version): "<<rate_reduction_policy_.get_name();
        ss<<"/"<<rate_reduction_policy_.get_version()<<std::endl;
        return ss.str();
}

// CLASS DTCP FLOW CONTROL CONFIG
DTCPFlowControlConfig::DTCPFlowControlConfig() {
        rate_based_ = false;
        window_based_ = false;
        rcv_buffers_threshold_ = 0;
        rcv_bytes_percent_threshold_ = 0;
        rcv_bytes_threshold_ = 0;
        sent_buffers_threshold_ = 0;
        sent_bytes_percent_threshold_ = 0;
        sent_bytes_threshold_ = 0;
}

const PolicyConfig& DTCPFlowControlConfig::get_closed_window_policy() const {
	return closed_window_policy_;
}

void DTCPFlowControlConfig::set_closed_window_policy(const PolicyConfig& closed_window_policy) {
	closed_window_policy_ = closed_window_policy;
}

const PolicyConfig& DTCPFlowControlConfig::get_flow_control_overrun_policy() const {
	return flow_control_overrun_policy_;
}

void DTCPFlowControlConfig::set_flow_control_overrun_policy(
                const PolicyConfig& flow_control_overrun_policy) {
	flow_control_overrun_policy_ = flow_control_overrun_policy;
}

bool DTCPFlowControlConfig::is_rate_based() const {
	return rate_based_;
}

void DTCPFlowControlConfig::set_rate_based(bool rate_based) {
	rate_based_ = rate_based;
}

const DTCPRateBasedFlowControlConfig& DTCPFlowControlConfig::get_rate_based_config() const {
	return rate_based_config_;
}

void DTCPFlowControlConfig::set_rate_based_config(
                const DTCPRateBasedFlowControlConfig& rate_based_config) {
	rate_based_config_ = rate_based_config;
}

int DTCPFlowControlConfig::get_rcv_buffers_threshold() const {
	return rcv_buffers_threshold_;
}

void DTCPFlowControlConfig::set_rcv_buffers_threshold(int rcv_buffers_threshold) {
	rcv_buffers_threshold_ = rcv_buffers_threshold;
}

int DTCPFlowControlConfig::get_rcv_bytes_percent_threshold() const {
	return rcv_bytes_percent_threshold_;
}

void DTCPFlowControlConfig::set_rcv_bytes_percent_threshold(int rcv_bytes_percent_threshold) {
	rcv_bytes_percent_threshold_ = rcv_bytes_percent_threshold;
}

int DTCPFlowControlConfig::get_rcv_bytes_threshold() const {
	return rcv_bytes_threshold_;
}

void DTCPFlowControlConfig::set_rcv_bytes_threshold(int rcv_bytes_threshold) {
	rcv_bytes_threshold_ = rcv_bytes_threshold;
}

const PolicyConfig& DTCPFlowControlConfig::get_reconcile_flow_control_policy() const {
	return reconcile_flow_control_policy_;
}

void DTCPFlowControlConfig::set_reconcile_flow_control_policy(
                const PolicyConfig& reconcile_flow_control_policy) {
	reconcile_flow_control_policy_ = reconcile_flow_control_policy;
}

int DTCPFlowControlConfig::get_sent_buffers_threshold() const {
	return sent_buffers_threshold_;
}

void DTCPFlowControlConfig::set_sent_buffers_threshold(int sent_buffers_threshold) {
	sent_buffers_threshold_ = sent_buffers_threshold;
}

int DTCPFlowControlConfig::get_sent_bytes_percent_threshold() const {
	return sent_bytes_percent_threshold_;
}

void DTCPFlowControlConfig::set_sent_bytes_percent_threshold(int sent_bytes_percent_threshold) {
	sent_bytes_percent_threshold_ = sent_bytes_percent_threshold;
}

int DTCPFlowControlConfig::get_sent_bytes_threshold() const {
	return sent_bytes_threshold_;
}

void DTCPFlowControlConfig::set_sent_bytes_threshold(int sent_bytes_threshold) {
	sent_bytes_threshold_ = sent_bytes_threshold;
}

bool DTCPFlowControlConfig::is_window_based() const {
	return window_based_;
}

void DTCPFlowControlConfig::set_window_based(bool window_based) {
	window_based_ = window_based;
}

const DTCPWindowBasedFlowControlConfig& DTCPFlowControlConfig::get_window_based_config() const {
	return window_based_config_;
}

void DTCPFlowControlConfig::set_window_based_config(
                const DTCPWindowBasedFlowControlConfig&
                window_based_config) {
	window_based_config_ = window_based_config;
}

const PolicyConfig& DTCPFlowControlConfig::get_receiving_flow_control_policy() const {
	return receiving_flow_control_policy_;
}

void DTCPFlowControlConfig::set_receiving_flow_control_policy(
                const PolicyConfig& receiving_flow_control_policy) {
	receiving_flow_control_policy_ = receiving_flow_control_policy;
}

const std::string DTCPFlowControlConfig::toString() {
        std::stringstream ss;
        ss<<"Sent bytes threshold: "<<sent_bytes_threshold_;
        ss<<"; Sent bytes percent threshold: "<<sent_bytes_percent_threshold_;
        ss<<"; Sent buffers threshold: "<<sent_buffers_threshold_<<std::endl;
        ss<<"Received bytes threshold: "<<rcv_bytes_threshold_;
        ss<<"; Received bytes percent threshold: "<<rcv_bytes_percent_threshold_;
        ss<<"; Received buffers threshold: "<<rcv_buffers_threshold_<<std::endl;
        ss<<"Closed window policy (name/version): "<<closed_window_policy_.get_name();
        ss<<"/"<<closed_window_policy_.get_version();
        ss<<"; Flow control overrun policy (name/version): "<<flow_control_overrun_policy_.get_name();
        ss<<"/"<<flow_control_overrun_policy_.get_version()<<std::endl;
        ss<<"Reconcile flow control policy (name/version): "<<reconcile_flow_control_policy_.get_name();
        ss<<"/"<<reconcile_flow_control_policy_.get_version();
        ss<<"; Window based? "<<window_based_<<"; Rate based? "<<rate_based_<<std::endl;
        ss<<"Receiving flow control policy (name/version): "<<receiving_flow_control_policy_.get_name();
        ss<<"/"<<receiving_flow_control_policy_.get_version()<<std::endl;
        if (window_based_) {
                ss<<"Window based flow control config: "<<window_based_config_.toString();
        }
        if (rate_based_) {
                ss<<"Rate based flow control config: "<<rate_based_config_.toString();
        }
        return ss.str();
}

// CLASS DTCP RX CONTROL CONFIG
DTCPRtxControlConfig::DTCPRtxControlConfig() {
	max_time_to_retry_ = 0;
	data_rxms_nmax_ = 0;
	initial_rtx_time_ = 0;
}

unsigned int DTCPRtxControlConfig::get_max_time_to_retry() const {
	return max_time_to_retry_;
}

void DTCPRtxControlConfig::set_max_time_to_retry(unsigned int max_time_to_retry) {
	max_time_to_retry_ = max_time_to_retry;
}

unsigned int DTCPRtxControlConfig::get_data_rxmsn_max() const {
	return data_rxms_nmax_;
}

void DTCPRtxControlConfig::set_data_rxmsn_max(unsigned int data_rxmsn_max) {
	data_rxms_nmax_ = data_rxmsn_max;
}

unsigned int DTCPRtxControlConfig::get_initial_rtx_time() const {
	return initial_rtx_time_;
}

void DTCPRtxControlConfig::set_initial_rtx_time(unsigned int initial_rtx_time) {
	initial_rtx_time_ = initial_rtx_time;
}

const PolicyConfig& DTCPRtxControlConfig::get_rcvr_ack_policy() const {
	return rcvr_ack_policy_;
}

void DTCPRtxControlConfig::set_rcvr_ack_policy(const PolicyConfig& rcvr_ack_policy) {
	rcvr_ack_policy_ = rcvr_ack_policy;
}

const PolicyConfig& DTCPRtxControlConfig::get_rcvr_control_ack_policy() const {
	return rcvr_control_ack_policy_;
}

void DTCPRtxControlConfig::set_rcvr_control_ack_policy(
                const PolicyConfig& rcvr_control_ack_policy) {
	rcvr_control_ack_policy_ = rcvr_control_ack_policy;
}

const PolicyConfig& DTCPRtxControlConfig::get_recving_ack_list_policy() const {
	return recving_ack_list_policy_;
}

void DTCPRtxControlConfig::set_recving_ack_list_policy(
                const PolicyConfig& recving_ack_list_policy) {
	recving_ack_list_policy_ = recving_ack_list_policy;
}

const PolicyConfig& DTCPRtxControlConfig::get_rtx_timer_expiry_policy() const {
	return rtx_timer_expiry_policy_;
}

void DTCPRtxControlConfig::set_rtx_timer_expiry_policy(
                const PolicyConfig& rtx_timer_expiry_policy) {
	rtx_timer_expiry_policy_ = rtx_timer_expiry_policy;
}

const PolicyConfig& DTCPRtxControlConfig::get_sender_ack_policy() const {
	return sender_ack_policy_;
}

void DTCPRtxControlConfig::set_sender_ack_policy(const PolicyConfig& sender_ack_policy) {
	sender_ack_policy_ = sender_ack_policy;
}

const PolicyConfig& DTCPRtxControlConfig::get_sending_ack_policy() const {
	return sending_ack_policy_;
}

void DTCPRtxControlConfig::set_sending_ack_policy(const PolicyConfig& sending_ack_policy) {
	sending_ack_policy_ = sending_ack_policy;
}

const std::string DTCPRtxControlConfig::toString() {
        std::stringstream ss;
        ss<<"Max time to retry the transmission of a PDU (R)"<<max_time_to_retry_;
        ss<<"; Max number of retx attempts: "<<data_rxms_nmax_;
        ss<<"; Initial retx time: "<<initial_rtx_time_<<std::endl;
        ss<<"; Rtx timer expiry policy (name/version): "<<rtx_timer_expiry_policy_.get_name();
        ss<<"/"<<rtx_timer_expiry_policy_.get_version()<<std::endl;
        ss<<"Sender ACK policy (name/version): "<<sender_ack_policy_.get_name();
        ss<<"/"<<sender_ack_policy_.get_version();
        ss<<"; Receiving ACK list policy (name/version): "<<recving_ack_list_policy_.get_name();
        ss<<"/"<<recving_ack_list_policy_.get_version()<<std::endl;
        ss<<"Reciver ACK policy (name/version): "<<rcvr_ack_policy_.get_name();
        ss<<"/"<<rcvr_ack_policy_.get_version();
        ss<<"; Sending ACK list policy (name/version): "<<sending_ack_policy_.get_name();
        ss<<"/"<<sending_ack_policy_.get_version()<<std::endl;
        ss<<"Reciver Control ACK policy (name/version): "<<rcvr_control_ack_policy_.get_name();
        ss<<"/"<<rcvr_control_ack_policy_.get_version()<<std::endl;
        return ss.str();
}

// CLASS DTCP CONFIG
DTCPConfig::DTCPConfig() {
        flow_control_ = false;
        rtx_control_ = false;
}

bool DTCPConfig::is_flow_control() const {
	return flow_control_;
}

void DTCPConfig::set_flow_control(bool flow_control) {
	flow_control_ = flow_control;
}

const DTCPFlowControlConfig& DTCPConfig::get_flow_control_config() const {
	return flow_control_config_;
}

void DTCPConfig::set_flow_control_config(
                const DTCPFlowControlConfig& flow_control_config){
	flow_control_config_ = flow_control_config;
}

const PolicyConfig& DTCPConfig::get_lost_control_pdu_policy() const {
	return lost_control_pdu_policy_;
}

void DTCPConfig::set_lost_control_pdu_policy(
                const PolicyConfig& lostcontrolpdupolicy) {
	lost_control_pdu_policy_ = lostcontrolpdupolicy;
}

bool DTCPConfig::is_rtx_control() const {
	return rtx_control_;
}

void DTCPConfig::set_rtx_control(bool rtx_control) {
	rtx_control_ = rtx_control;
}

const DTCPRtxControlConfig& DTCPConfig::get_rtx_control_config() const {
	return rtx_control_config_;
}

void DTCPConfig::set_rtx_control_config(const DTCPRtxControlConfig& rtx_control_config) {
	rtx_control_config_ = rtx_control_config;
}

const PolicyConfig& DTCPConfig::get_rtt_estimator_policy() const {
	return rtt_estimator_policy_;
}

void DTCPConfig::set_rtt_estimator_policy(const PolicyConfig& rtt_estimator_policy) {
	rtt_estimator_policy_ = rtt_estimator_policy;
}

const std::string DTCPConfig::toString() {
        std::stringstream ss;
        ss<<"Flow control? "<<flow_control_<<"; Retx control? "<<rtx_control_;
        ss<<"; Lost control PDU policy (name/version): "<<lost_control_pdu_policy_.get_name();
        ss<<"/"<<lost_control_pdu_policy_.get_version()<<std::endl;
        ss<<"RTT estimator policy (name/version): "<<rtt_estimator_policy_.get_name();
        ss<<"/"<<rtt_estimator_policy_.get_version()<<std::endl;
        if (rtx_control_) {
                ss<<"Retx control config: "<<rtx_control_config_.toString();
        }
        if (flow_control_) {
                ss<<"Flow control config: "<<flow_control_config_.toString();
        }
        return ss.str();
}

// CLASS CONNECTION POLICIES
ConnectionPolicies::ConnectionPolicies(){
	dtcp_present_ = false;
	seq_num_rollover_threshold_ = 0;
	initial_a_timer_ = 0;
	partial_delivery_ = false;
	in_order_delivery_ = false;
	incomplete_delivery_ = false;
	max_sdu_gap_ = 0;
}

const PolicyConfig& ConnectionPolicies::get_rcvr_timer_inactivity_policy() const {
	return rcvr_timer_inactivity_policy_;
}

void ConnectionPolicies::set_rcvr_timer_inactivity_policy(
                const PolicyConfig& rcvr_timer_inactivity_policy) {
	rcvr_timer_inactivity_policy_ = rcvr_timer_inactivity_policy;
}

const PolicyConfig& ConnectionPolicies::get_sender_timer_inactivity_policy() const {
	return sender_timer_inactivity_policy_;
}

void ConnectionPolicies::set_sender_timer_inactivity_policy(
                const PolicyConfig& sender_timer_inactivity_policy) {
	sender_timer_inactivity_policy_ = sender_timer_inactivity_policy;
}

const DTCPConfig& ConnectionPolicies::get_dtcp_configuration() const {
	return dtcp_configuration_;
}

void ConnectionPolicies::set_dtcp_configuration(const DTCPConfig& dtcp_configuration) {
	dtcp_configuration_ = dtcp_configuration;
}

bool ConnectionPolicies::is_dtcp_present() const {
	return dtcp_present_;
}

void ConnectionPolicies::set_dtcp_present(bool dtcp_present) {
	dtcp_present_ = dtcp_present;
}

const PolicyConfig& ConnectionPolicies::get_initial_seq_num_policy() const {
	return initial_seq_num_policy_;
}

void ConnectionPolicies::set_initial_seq_num_policy(const PolicyConfig& initial_seq_num_policy) {
	initial_seq_num_policy_ = initial_seq_num_policy;
}

unsigned int ConnectionPolicies::get_seq_num_rollover_threshold() const {
	return seq_num_rollover_threshold_;
}

void ConnectionPolicies::set_seq_num_rollover_threshold(unsigned int seq_num_rollover_threshold) {
	seq_num_rollover_threshold_ = seq_num_rollover_threshold;
}

unsigned int ConnectionPolicies::get_initial_a_timer() const {
	return initial_a_timer_;
}

void ConnectionPolicies::set_initial_a_timer(unsigned int initial_a_timer) {
	initial_a_timer_ = initial_a_timer;
}

bool ConnectionPolicies::is_in_order_delivery() const {
	return in_order_delivery_;
}

void ConnectionPolicies::set_in_order_delivery(bool in_order_delivery) {
	in_order_delivery_ = in_order_delivery;
}

int ConnectionPolicies::get_max_sdu_gap() const {
	return max_sdu_gap_;
}

void ConnectionPolicies::set_max_sdu_gap(int max_sdu_gap) {
	max_sdu_gap_ = max_sdu_gap;
}

bool ConnectionPolicies::is_partial_delivery() const {
	return partial_delivery_;
}

void ConnectionPolicies::set_partial_delivery(bool partial_delivery) {
	partial_delivery_ = partial_delivery;
}

bool ConnectionPolicies::is_incomplete_delivery() const {
	return incomplete_delivery_;
}

void ConnectionPolicies::set_incomplete_delivery(bool incomplete_delivery) {
	incomplete_delivery_ = incomplete_delivery;
}

const std::string ConnectionPolicies::toString() {
        std::stringstream ss;
        ss<<"Sder time inactivity policy (name/version): "<<sender_timer_inactivity_policy_.get_name();
        ss<<"/"<<sender_timer_inactivity_policy_.get_version();
        ss<<"; Rcvr time inactivity policy (name/version): "<<rcvr_timer_inactivity_policy_.get_name();
        ss<<"/"<<rcvr_timer_inactivity_policy_.get_version()<<std::endl;
        ss<<"DTCP present: "<<dtcp_present_;
        ss<<"; Initial A-timer: "<<initial_a_timer_;
        ss<<"; Seq. num roll. threshold: "<<seq_num_rollover_threshold_;
        ss<<"; Initial seq. num policy (name/version): ";
        ss<<initial_seq_num_policy_.get_name()<<"/"<<initial_seq_num_policy_.get_version()<<std::endl;
        ss<<"; Partial delivery: "<<partial_delivery_;
        ss<<"; In order delivery: "<<in_order_delivery_;
        ss<<"; Max allowed SDU gap: "<<max_sdu_gap_<<std::endl;
        if (dtcp_present_) {
                ss<<"DTCP Configuration: "<<dtcp_configuration_.toString();
        }
        return ss.str();
}

/* CLASS QoS CUBE */
QoSCube::QoSCube(){
        id_ = 0;
        average_bandwidth_ = 0;
        average_sdu_bandwidth_ = 0;
        peak_bandwidth_duration_ = 0;
        peak_sdu_bandwidth_duration_ = 0;
        undetected_bit_error_rate_ = 0;
        partial_delivery_ = true;
        ordered_delivery_ = false;
        max_allowable_gap_ = -1;
        jitter_ = 0;
        delay_ = 0;
}

QoSCube::QoSCube(const std::string& name, int id) {
	name_ = name;
	id_ = id;
	average_bandwidth_ = 0;
	average_sdu_bandwidth_ = 0;
	peak_bandwidth_duration_ = 0;
	peak_sdu_bandwidth_duration_ = 0;
	undetected_bit_error_rate_ = 0;
	partial_delivery_ = true;
	ordered_delivery_ = false;
	max_allowable_gap_ = -1;
	jitter_ = 0;
	delay_ = 0;
}

bool QoSCube::operator==(const QoSCube &other) const {
	return (this->id_ == other.get_id());
}

bool QoSCube::operator!=(const QoSCube &other) const {
	return !(*this == other);
}

void QoSCube::set_id(unsigned int id) {
	id_ = id;
}

unsigned int QoSCube::get_id() const {
	return id_;
}

const std::string& QoSCube::get_name() const {
	return name_;
}

void QoSCube::set_name(const std::string& name) {
	name_ = name;
}

const ConnectionPolicies& QoSCube::get_efcp_policies() const {
	return efcp_policies_;
}

void QoSCube::set_efcp_policies(const ConnectionPolicies& efcp_policies) {
	efcp_policies_ = efcp_policies;
}

unsigned int QoSCube::get_average_bandwidth() const {
	return average_bandwidth_;
}

void QoSCube::set_average_bandwidth(unsigned int average_bandwidth) {
	average_bandwidth_ = average_bandwidth;
}

unsigned int QoSCube::get_average_sdu_bandwidth() const {
	return average_sdu_bandwidth_;
}

void QoSCube::setAverageSduBandwidth(unsigned int average_sdu_bandwidth) {
	average_sdu_bandwidth_ = average_sdu_bandwidth;
}

unsigned int QoSCube::get_delay() const {
	return delay_;
}

void QoSCube::set_delay(unsigned int delay) {
	delay_ = delay;
}

unsigned int QoSCube::get_jitter() const {
	return jitter_;
}

void QoSCube::set_jitter(unsigned int jitter) {
	jitter_ = jitter;
}

int QoSCube::get_max_allowable_gap() const {
	return max_allowable_gap_;
}

void QoSCube::set_max_allowable_gap(int max_allowable_gap) {
	max_allowable_gap_ = max_allowable_gap;
}

bool QoSCube::is_ordered_delivery() const {
	return ordered_delivery_;
}

void QoSCube::set_ordered_delivery(bool ordered_delivery) {
	ordered_delivery_ = ordered_delivery;
}

bool QoSCube::is_partial_delivery() const {
	return partial_delivery_;
}

void QoSCube::set_partial_delivery(bool partial_delivery) {
	partial_delivery_ = partial_delivery;
}

unsigned int QoSCube::get_peak_bandwidth_duration() const {
	return peak_bandwidth_duration_;
}

void QoSCube::set_peak_bandwidth_duration(unsigned int peak_bandwidth_duration) {
	peak_bandwidth_duration_ = peak_bandwidth_duration;
}

unsigned int QoSCube::get_peak_sdu_bandwidth_duration() const {
	return peak_sdu_bandwidth_duration_;
}

void QoSCube::set_peak_sdu_bandwidth_duration(unsigned int peak_sdu_bandwidth_duration) {
	peak_sdu_bandwidth_duration_ = peak_sdu_bandwidth_duration;
}

double QoSCube::get_undetected_bit_error_rate() const {
	return undetected_bit_error_rate_;
}

void QoSCube::set_undetected_bit_error_rate(double undetected_bit_error_rate) {
	undetected_bit_error_rate_ = undetected_bit_error_rate;
}

const std::string QoSCube::toString() {
        std::stringstream ss;
        ss<<"Name: "<<name_<<"; Id: "<<id_;
        ss<<"; Jitter: "<<jitter_<<"; Delay: "<<delay_<<std::endl;
        ss<<"In oder delivery: "<<ordered_delivery_;
        ss<<"; Partial delivery allowed: "<<partial_delivery_<<std::endl;
        ss<<"Max allowed gap between SDUs: "<<max_allowable_gap_;
        ss<<"; Undetected bit error rate: "<<undetected_bit_error_rate_<<std::endl;
        ss<<"Average bandwidth (bytes/s): "<<average_bandwidth_;
        ss<<"; Average SDU bandwidth (bytes/s): "<<average_sdu_bandwidth_<<std::endl;
        ss<<"Peak bandwidth duration (ms): "<<peak_bandwidth_duration_;
        ss<<"; Peak SDU bandwidth duration (ms): "<<peak_sdu_bandwidth_duration_<<std::endl;
        ss<<"EFCP policies: "<<efcp_policies_.toString();
        return ss.str();
}

// CLASS DATA TRANSFER CONSTANTS
DataTransferConstants::DataTransferConstants() {
	qos_id_length_ = 0;
	port_id_length_ = 0;
	cep_id_length_ = 0;
	sequence_number_length_ = 0;
	address_length_ = 0;
	length_length_ = 0;
	max_pdu_size_ = 0;
	dif_integrity_ = false;
	max_pdu_lifetime_ = 0;
}

unsigned short DataTransferConstants::get_address_length() const {
	return address_length_;
}

void DataTransferConstants::set_address_length(unsigned short address_length) {
	address_length_ = address_length;
}

unsigned short DataTransferConstants::get_cep_id_length() const {
	return cep_id_length_;
}

void DataTransferConstants::set_cep_id_length(unsigned short cep_id_length) {
	cep_id_length_ = cep_id_length;
}

bool DataTransferConstants::is_dif_integrity() const {
	return dif_integrity_;
}

void DataTransferConstants::set_dif_integrity(bool dif_integrity) {
	dif_integrity_ = dif_integrity;
}

unsigned short DataTransferConstants::get_length_length() const {
	return length_length_;
}

void DataTransferConstants::set_length_length(unsigned short length_length) {
	length_length_ = length_length;
}

unsigned int DataTransferConstants::get_max_pdu_lifetime() const {
	return max_pdu_lifetime_;
}

void DataTransferConstants::set_max_pdu_lifetime(unsigned int max_pdu_lifetime) {
	max_pdu_lifetime_ = max_pdu_lifetime;
}

unsigned int DataTransferConstants::get_max_pdu_size() const {
	return max_pdu_size_;
}

void DataTransferConstants::set_max_pdu_size(unsigned int max_pdu_size) {
	max_pdu_size_ = max_pdu_size;
}

unsigned short DataTransferConstants::get_port_id_length() const {
	return port_id_length_;
}

void DataTransferConstants::set_port_id_length(unsigned short port_id_length) {
	port_id_length_ = port_id_length;
}

unsigned short DataTransferConstants::get_qos_id_length() const {
	return qos_id_length_;
}

void DataTransferConstants::set_qos_id_length(unsigned short qos_id_length) {
	qos_id_length_ = qos_id_length;
}

unsigned short DataTransferConstants::get_sequence_number_length() const {
	return sequence_number_length_;
}

void DataTransferConstants::set_sequence_number_length(unsigned short sequence_number_length) {
	sequence_number_length_ = sequence_number_length;
}

bool DataTransferConstants::isInitialized() {
	if (qos_id_length_ == 0 || address_length_ == 0 || cep_id_length_ == 0 ||
			port_id_length_ == 0 || length_length_ == 0 ){
		return false;
	}

	return true;
}

const std::string DataTransferConstants::toString(){
	std::stringstream ss;

	ss<<"Address length (bytes): "<<address_length_;
	ss<<"; CEP-id length (bytes): "<<cep_id_length_;
	ss<<"; Length length (bytes): "<<length_length_<<std::endl;
	ss<<"Port-id length (bytes): "<<port_id_length_;
	ss<<"; Qos-id length (bytes): "<<qos_id_length_;
	ss<<"; Seq number length(bytes): "<<sequence_number_length_<<std::endl;
	ss<<"Max PDU lifetime: "<<max_pdu_lifetime_;
	ss<<"; Max PDU size: "<<max_pdu_size_;
	ss<<"; Integrity?: "<<dif_integrity_;
	ss<<"; Initialized?: "<<isInitialized();

	return ss.str();
}

// CLASS EFCPConfiguration
EFCPConfiguration::EFCPConfiguration(){
}

EFCPConfiguration::EFCPConfiguration(const EFCPConfiguration& other) {
	copy(other);
}

EFCPConfiguration& EFCPConfiguration::operator=(const EFCPConfiguration & other) {
	if (this == &other) {
		return *this;
	}

	copy(other);

	return *this;
}

void EFCPConfiguration::copy(const EFCPConfiguration & other) {
	data_transfer_constants_ = other.data_transfer_constants_;
	unknown_flowpolicy_ = other.unknown_flowpolicy_;
	std::list<QoSCube*>::const_iterator it;
	QoSCube * cube = 0;
	for (it = other.qos_cubes_.begin(); it != other.qos_cubes_.end(); ++it) {
		cube = new QoSCube(*(*it));
		qos_cubes_.push_back(cube);
	}
}

EFCPConfiguration::~EFCPConfiguration() {
	std::list<QoSCube*>::iterator it;
	rina::QoSCube * current = 0;
	for (it=qos_cubes_.begin(); it != qos_cubes_.end(); ++it) {
		current = *it;
		if (current != 0) {
			delete current;
			*it = 0;
		}
	}
}

const std::list<QoSCube*>& EFCPConfiguration::get_qos_cubes() const {
	return qos_cubes_;
}

void EFCPConfiguration::add_qos_cube(QoSCube* qos_cube) {
        std::list<QoSCube*>::const_iterator iterator;
        for (iterator = qos_cubes_.begin(); iterator != qos_cubes_.end(); ++iterator) {
            if ((*iterator)->id_ == qos_cube->id_) {
                    return;
            }
        }

        qos_cubes_.push_back(qos_cube);
}

const DataTransferConstants&
EFCPConfiguration::get_data_transfer_constants() const {
        return data_transfer_constants_;
}

void EFCPConfiguration::set_data_transfer_constants(
                const DataTransferConstants& data_transfer_constants) {
	data_transfer_constants_ = data_transfer_constants;
}

const PolicyConfig& EFCPConfiguration::get_unknown_flow_policy() const {
        return unknown_flowpolicy_;
}

void EFCPConfiguration::set_unknown_flow_policy(
                const PolicyConfig& unknown_flowpolicy) {
	unknown_flowpolicy_ = unknown_flowpolicy;
}

// CLASS FlowAllocatorConfiguration
FlowAllocatorConfiguration::FlowAllocatorConfiguration(){
	max_create_flow_retries_ = 0;
}

const PolicyConfig&
FlowAllocatorConfiguration::get_allocate_notify_policy() const {
	return allocate_notify_policy_;
}

void FlowAllocatorConfiguration::set_allocate_notify_policy(
		const PolicyConfig& allocate_notify_policy){
	allocate_notify_policy_ = allocate_notify_policy;
}

const PolicyConfig& FlowAllocatorConfiguration::get_allocate_retry_policy() const{
	return allocate_retry_policy_;
}

void FlowAllocatorConfiguration::set_allocate_retry_policy(
		const PolicyConfig& allocate_retry_policy) {
	allocate_retry_policy_ = allocate_retry_policy;
}

int FlowAllocatorConfiguration::get_max_create_flow_retries() const {
	return max_create_flow_retries_;
}

void FlowAllocatorConfiguration::set_max_create_flow_retries(
		int max_create_flow_retries) {
	max_create_flow_retries_ = max_create_flow_retries;
}

const PolicyConfig& FlowAllocatorConfiguration::get_new_flow_request_policy() const {
	return new_flow_request_policy_;
}

void FlowAllocatorConfiguration::set_new_flow_request_policy(
		const PolicyConfig& new_flow_request_policy) {
	new_flow_request_policy_ = new_flow_request_policy;
}

const PolicyConfig& FlowAllocatorConfiguration::get_seq_rollover_policy() const {
	return seq_rollover_policy_;
}

void FlowAllocatorConfiguration::set_seq_rollover_policy(
		const PolicyConfig& seq_rollover_policy) {
	seq_rollover_policy_ = seq_rollover_policy;
}

// CLASS LinkStateRouting Configuraiton

const std::string LinkStateRoutingConfiguration::DEFAULT_ROUTING_ALGORITHM =
    "Dijkstra";

LinkStateRoutingConfiguration::LinkStateRoutingConfiguration()
{
	wait_until_read_cdap_ = WAIT_UNTIL_READ_CDAP_DEFAULT;
	wait_until_error_ = WAIT_UNTIL_ERROR_DEFAULT;
	wait_until_pduft_computation_ = WAIT_UNTIL_PDUFT_COMPUTATION_DEFAULT;
	wait_until_fsodb_propagation_ = WAIT_UNTIL_FSODB_PROPAGATION_DEFAULT;
	wait_until_age_increment_ = WAIT_UNTIL_AGE_INCREMENT_DEFAULT;
	object_maximum_age_ = PULSES_UNTIL_FSO_EXPIRATION_DEFAULT;
	routing_algorithm_ = DEFAULT_ROUTING_ALGORITHM;
}

const std::string LinkStateRoutingConfiguration::toString() {
	std::stringstream ss;

	ss<<"Timer until send a Read CDAP message (ms): " << wait_until_read_cdap_<<std::endl;
	ss<<"Timer until send CDAP error (ms): "<<wait_until_error_<<std::endl;
	ss<<"Timer between PDU forwarding table Computation (ms): "<<wait_until_pduft_computation_<<std::endl;
	ss<<"Timer between FlowStateDataBase propagation (ms): "<<wait_until_fsodb_propagation_<<std::endl;
	ss<<"Timer between age incrementation pulses (ms): "<<wait_until_age_increment_<<std::endl;
	ss<<"Number of pulses until FSO expiration: "<<object_maximum_age_<<std::endl;
	ss<<"Routing algorithm: "<<routing_algorithm_<<std::endl;

	return ss.str();
}

int LinkStateRoutingConfiguration::get_wait_until_age_increment() const {
	return wait_until_age_increment_;
}

void LinkStateRoutingConfiguration::set_wait_until_age_increment(int wait_until_age_increment) {
	wait_until_age_increment_ = wait_until_age_increment;
}

int LinkStateRoutingConfiguration::get_wait_until_error() const {
	return wait_until_error_;
}

void LinkStateRoutingConfiguration::set_wait_until_error(int wait_until_error) {
	wait_until_error_ = wait_until_error;
}

int LinkStateRoutingConfiguration::get_wait_until_fsodb_propagation() const {
	return wait_until_fsodb_propagation_;
}

void LinkStateRoutingConfiguration::set_wait_until_fsodb_propagation(int wait_until_fsodb_propagation) {
	wait_until_fsodb_propagation_ = wait_until_fsodb_propagation;
}

int LinkStateRoutingConfiguration::get_wait_until_pduft_computation() const {
	return wait_until_pduft_computation_;
}

void LinkStateRoutingConfiguration::set_wait_until_pduft_computation(int wait_until_pduft_computation) {
	wait_until_pduft_computation_ = wait_until_pduft_computation;
}

int LinkStateRoutingConfiguration::get_wait_until_read_cdap() const {
	return wait_until_read_cdap_;
}

void LinkStateRoutingConfiguration::set_wait_until_read_cdap(int wait_until_read_cdap) {
	wait_until_read_cdap_ = wait_until_read_cdap;
}

unsigned int LinkStateRoutingConfiguration::get_object_maximum_age() const {
	return object_maximum_age_;
}

void LinkStateRoutingConfiguration::set_object_maximum_age(const int object_maximum_age) {
	object_maximum_age_ = object_maximum_age;
}

const std::string& LinkStateRoutingConfiguration::get_routing_algorithm() const {
	return routing_algorithm_;
}

void LinkStateRoutingConfiguration::set_routing_algorithm(
		const std::string& routing_algorithm) {
	routing_algorithm_ = routing_algorithm;
}

// CLAS PDUFTableGeneratorConfiguration
PDUFTableGeneratorConfiguration::PDUFTableGeneratorConfiguration(){
	set_pduft_generator_policy(PolicyConfig("LinkState",
                        RINA_DEFAULT_POLICY_VERSION));
}

PDUFTableGeneratorConfiguration::PDUFTableGeneratorConfiguration(
                const PolicyConfig& pduFTGeneratorPolicy) {
	set_pduft_generator_policy(pduFTGeneratorPolicy);
}

const LinkStateRoutingConfiguration&
PDUFTableGeneratorConfiguration::get_link_state_routing_configuration() const {
        return link_state_routing_configuration_;
}

void PDUFTableGeneratorConfiguration::set_link_state_routing_configuration(
                const LinkStateRoutingConfiguration& link_state_routing_configuration){
	link_state_routing_configuration_ =
			link_state_routing_configuration;
}

const PolicyConfig&
PDUFTableGeneratorConfiguration::get_pduft_generator_policy() const {
        return pduft_generator_policy_;
}

void PDUFTableGeneratorConfiguration::set_pduft_generator_policy(
                const PolicyConfig& pduft_generator_policy){
	pduft_generator_policy_ = pduft_generator_policy;
}

// Class Enrollment Task Configuration
EnrollmentTaskConfiguration::EnrollmentTaskConfiguration() {
	enrollment_timeout_in_ms_ = 10000;
	watchdog_period_in_ms_ = 60000;
	declared_dead_interval_in_ms_ = 120000;
	neighbor_enroller_period_in_ms_ = 10000;
	max_number_of_enrollment_attempts_ = 3;
}

// Class Static IPC Process Address
StaticIPCProcessAddress::StaticIPCProcessAddress() {
	address_ = 0;
}

//Class AddressPrefixConfiguration
AddressPrefixConfiguration::AddressPrefixConfiguration() {
	address_prefix_ = 0;
}

// CLASS DIF CONFIGURATION
DIFConfiguration::DIFConfiguration(){
  address_ = 0;
}

unsigned int DIFConfiguration::get_address() const {
	return address_;
}

void DIFConfiguration::set_address(unsigned int address) {
	address_ = address;
}

const std::list<PolicyConfig>& DIFConfiguration::get_policies() {
	return policies_;
}

void DIFConfiguration::set_policies(const std::list<PolicyConfig>& policies) {
	policies_ = policies;
}

void DIFConfiguration::add_policy(const PolicyConfig& policy) {
	policies_.push_back(policy);
}

const std::list<Parameter>& DIFConfiguration::get_parameters() const {
	return parameters_;
}

void DIFConfiguration::set_parameters(const std::list<Parameter>& parameters) {
	parameters_ = parameters;
}

void DIFConfiguration::add_parameter(const Parameter& parameter){
	parameters_.push_back(parameter);
}

const EFCPConfiguration& DIFConfiguration::get_efcp_configuration() const {
	return efcp_configuration_;
}

void DIFConfiguration::set_efcp_configuration(
		const EFCPConfiguration& efcp_configuration) {
	efcp_configuration_= efcp_configuration;
}

void DIFConfiguration::set_pduft_generator_configuration(
		const PDUFTableGeneratorConfiguration& pduft_generator_configuration){
	pduft_generator_configuration_ = pduft_generator_configuration;
}

const PDUFTableGeneratorConfiguration&
DIFConfiguration::get_pduft_generator_configuration() const {
	return pduft_generator_configuration_;
}

const RMTConfiguration& DIFConfiguration::get_rmt_configuration() const {
	return rmt_configuration_;
}

void DIFConfiguration::set_rmt_configuration(
		const RMTConfiguration& rmt_configuration) {
	rmt_configuration_ = rmt_configuration;
}

const FlowAllocatorConfiguration& DIFConfiguration::get_fa_configuration() const {
	return fa_configuration_;
}

void DIFConfiguration::set_fa_configuration(
		const FlowAllocatorConfiguration& fa_configuration) {
	fa_configuration_ = fa_configuration;
}

// CLASS DIF INFORMATION
const ApplicationProcessNamingInformation& DIFInformation::get_dif_name()
const {
	return dif_name_;
}

void DIFInformation::set_dif_name(
		const ApplicationProcessNamingInformation& dif_name) {
	dif_name_ = dif_name;
}

const std::string& DIFInformation::get_dif_type() const {
	return dif_type_;
}

void DIFInformation::set_dif_type(const std::string& dif_type) {
	dif_type_ = dif_type;
}

const DIFConfiguration& DIFInformation::get_dif_configuration()
const {
	return dif_configuration_;
}

void DIFInformation::set_dif_configuration(
		const DIFConfiguration& dif_configuration) {
	dif_configuration_ = dif_configuration;
}

}
