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
#include <stdlib.h>

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

std::string PolicyParameter::toString()
{
	std::stringstream ss;
	ss << "Name: " << name_ << "; Value: " << value_ << std::endl;
	return ss.str();
}

// CLASS POLICY CONFIGURATION
PolicyConfig::PolicyConfig() {
}

PolicyConfig::PolicyConfig(const std::string& name,
                const std::string& version) {
        name_ = name;
        version_ = version;
}

PolicyConfig::PolicyConfig(struct policy * pc)
{
	PolicyParameter param;
	struct policy_parm * pos;

	if (!pc)
		return;

	name_ = pc->name;
	version_ = pc->version;

        list_for_each_entry(pos, &(pc->params), next) {
        	param.name_ = pos->name;
        	param.value_ = pos->value;
        	add_parameter(param);
        }
}

struct policy * PolicyConfig::to_c_policy() const
{
	std::list<PolicyParameter>::iterator it;
	struct policy * result;
	struct policy_parm * pp;

	result = new policy();
	INIT_LIST_HEAD(&result->params);
	result->name = name_.c_str();
	result->version = version_.c_str();
	for (it = parameters_.begin(); it != parameters_.end(); ++it) {
		pp = new policy_parm();
		INIT_LIST_HEAD(&pp->next);
		pp->name = it->name_.c_str();
		pp->value = it->value_.c_str();
		list_add_tail(&pp->next, &result->params);
	}

	return result;
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

std::string PolicyConfig::get_param_value_as_string(const std::string& name) const
{
	if (parameters_.size() == 0) {
		throw Exception("No parameters");
	}

	for (std::list<PolicyParameter>::const_iterator it = parameters_.begin();
			it != parameters_.end(); ++it) {
		if (it->name_ == name) {
			return it->value_;
		}
	}
  std::stringstream ss;
  ss << "Parameter '" << name << "' not found.";
	throw Exception(ss.str().c_str());
}

long PolicyConfig::get_param_value_as_long(const std::string& name) const
{
	long result;
	char *dummy;

	std::string value = get_param_value_as_string(name);
	result = strtol(value.c_str(), &dummy, 10);
	if (!value.size() || *dummy != '\0') {
    std::stringstream ss;
    ss << "Parameter '" << name << "' error converting value to long.";
		throw Exception(ss.str().c_str());
	}

	return result;
}

float PolicyConfig::get_param_value_as_float(const std::string& name) const
{
	float result;
	char *dummy;

	std::string value = get_param_value_as_string(name);
	result = strtof(value.c_str(), &dummy);
	if (!value.size() || *dummy != '\0') {
    std::stringstream ss;
    ss << "Parameter '" << name << "' error converting value to float.";
		throw Exception(ss.str().c_str());
	}

	return result;
}

int PolicyConfig::get_param_value_as_int(const std::string& name) const
{
	return get_param_value_as_long(name);
}

unsigned long PolicyConfig::get_param_value_as_ulong(const std::string& name) const
{
	unsigned long result;
	char *dummy;

	std::string value = get_param_value_as_string(name);
	result = strtoul(value.c_str(), &dummy, 10);
	if (!value.size() || *dummy != '\0') {
		std::stringstream ss;
		ss << "Parameter '" << name << "' error converting value to ulong.";
		throw Exception(ss.str().c_str());
	}

	return result;
}

bool PolicyConfig::get_param_value_as_bool(const std::string& name) const
{
	std::string value = get_param_value_as_string(name);
	if (value == "true") {
		return true;
	} else if (value == "false") {
		return false;
	}

	throw Exception("Wrong value for bool");
}

unsigned int PolicyConfig::get_param_value_as_uint(const std::string& name) const
{
	return get_param_value_as_ulong(name);
}

std::string PolicyConfig::toString()
{
	std::stringstream ss;
	ss << "Name: " << name_ << "; Version: " << version_ << std::endl;
	if (parameters_.size() > 0) {
		ss << "Parameters:" << std::endl;
		for (std::list<PolicyParameter>::const_iterator it = parameters_.begin();
				it != parameters_.end(); ++it) {
			ss <<"Name: " << it->name_ << "; Value: "<< it->value_ << std::endl;
		}
	}

	return ss.str();
}

// CLASS DTCP WINDOW-BASED FLOW CONTROL CONFIG
DTCPWindowBasedFlowControlConfig::DTCPWindowBasedFlowControlConfig() {
        initial_credit_ = 0;
        max_closed_window_queue_length_ = 0;
}

DTCPWindowBasedFlowControlConfig::DTCPWindowBasedFlowControlConfig(struct window_fctrl_config * wfc)
{
	if (!wfc)
		return;

	initial_credit_ = wfc->initial_credit;
	max_closed_window_queue_length_ = wfc->max_closed_winq_length;
	rcvr_flow_control_policy_(wfc->rcvr_flow_control);
	tx_control_policy_(wfc->tx_control);
}

struct window_fctrl_config * DTCPWindowBasedFlowControlConfig::to_c_window_config()
{
	struct window_fctrl_config * result;

	result = new window_fctrl_config();
	result->initial_credit = initial_credit_;
	result->max_closed_winq_length = max_closed_window_queue_length_;
	result->rcvr_flow_control = rcvr_flow_control_policy_.to_c_policy();
	result->tx_control = tx_control_policy_.to_c_policy();

	return result;
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

DTCPRateBasedFlowControlConfig::DTCPRateBasedFlowControlConfig(struct rate_fctrl_config * rfg)
{
	if (!rfg)
		return;

	sending_rate_ = rfg->sending_rate;
	time_period_ = rfg->time_period;
	no_override_default_peak_policy_(rfg->no_override_default_peak);
	no_rate_slow_down_policy_(rfg->no_rate_slow_down);
	rate_reduction_policy_(rfg->rate_reduction);
}

struct rate_fctrl_config * DTCPRateBasedFlowControlConfig::to_c_rate_config()
{
	struct rate_fctrl_config * result;

	result = new rate_fctrl_config();
	result->sending_rate = sending_rate_;
	result->time_period = time_period_;
	result->no_override_default_peak = no_override_default_peak_policy_.to_c_policy();
	result->no_rate_slow_down = no_rate_slow_down_policy_.to_c_policy();
	result->rate_reduction = rate_reduction_policy_.to_c_policy();

	return result;
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

DTCPFlowControlConfig::DTCPFlowControlConfig(struct dtcp_fctrl_config * fcc)
{
	if (!fcc)
		return;

	rate_based_ = fcc->rate_based_fctrl;
	window_based_ = fcc->window_based_fctrl;
	rcv_buffers_threshold_ = fcc->rcvd_buffers_th;
	rcv_bytes_percent_threshold_ = fcc->rcvd_bytes_percent_th;
        rcv_bytes_threshold_ = fcc->rcvd_bytes_th;
        sent_buffers_threshold_ = fcc->sent_buffers_th;
        sent_bytes_percent_threshold_ = fcc->sent_bytes_percent_th;
        sent_bytes_threshold_ = fcc->sent_bytes_th;
        window_based_config_(fcc->wfctrl_cfg);
        rate_based_config_(fcc->rfctrl_cfg);
        closed_window_policy_(fcc->closed_window);
        flow_control_overrun_policy_(fcc->flow_control_overrun);
        reconcile_flow_control_policy_(fcc->reconcile_flow_conflict);
        receiving_flow_control_policy_(fcc->receiving_flow_control);
}

struct dtcp_fctrl_config * DTCPFlowControlConfig::to_c_fconfig()
{
	struct dtcp_fctrl_config * result;

	result = new dtcp_fctrl_config();
	result->rate_based_fctrl = rate_based_;
	result->window_based_fctrl = window_based_;
	result->rcvd_buffers_th = rcv_buffers_threshold_;
	result->rcvd_bytes_percent_th = rcv_bytes_percent_threshold_;
	result->rcvd_bytes_th = rcv_bytes_threshold_;
	result->sent_buffers_th = sent_buffers_threshold_;
	result->sent_bytes_percent_th = sent_bytes_percent_threshold_;
	result->sent_bytes_th = sent_bytes_threshold_;
	result->wfctrl_cfg = window_based_config_.to_c_window_config();
	result->rfctrl_cfg = rate_based_config_.to_c_rate_config();
	result->closed_window = closed_window_policy_.to_c_policy();
	result->flow_control_overrun = flow_control_overrun_policy_.to_c_policy();
	result->reconcile_flow_conflict = reconcile_flow_control_policy_.to_c_policy();
	result->receiving_flow_control = receiving_flow_control_policy_.to_c_policy();

	return result;
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

DTCPRtxControlConfig::DTCPRtxControlConfig(struct dtcp_rxctrl_config * drc)
{
	if (!drc)
		return;

	max_time_to_retry_ = drc->max_time_retry;
	data_rxms_nmax_ = drc->data_retransmit_max;
	initial_rtx_time_ = drc->initial_tr;
	rtx_timer_expiry_policy_(drc->retransmission_timer_expiry);
	sender_ack_policy_(drc->sender_ack);
	recving_ack_list_policy_(drc->receiving_ack_list);
	rcvr_ack_policy_(drc->rcvr_ack);
	sending_ack_policy_(drc->sending_ack);
	rcvr_control_ack_policy_(drc->rcvr_control_ack);
}

struct dtcp_rxctrl_config * DTCPRtxControlConfig::to_c_rxconfig()
{
	struct dtcp_rxctrl_config * result;

	result = new dtcp_rxctrl_config();
	result->max_time_retry = max_time_to_retry_;
	result->data_retransmit_max = data_rxms_nmax_;
	result->initial_tr = initial_rtx_time_;
	result->retransmission_timer_expiry = rtx_timer_expiry_policy_.to_c_policy();
	result->sender_ack = sender_ack_policy_.to_c_policy();
	result->receiving_ack_list = recving_ack_list_policy_.to_c_policy();
	result->rcvr_ack = rcvr_ack_policy_.to_c_policy();
	result->sender_ack = sending_ack_policy_.to_c_policy();
	result->rcvr_control_ack = rcvr_control_ack_policy_.to_c_policy();

	return result;
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

DTCPConfig::DTCPConfig(struct dtcp_config* dtc)
{
	if (!dtc)
		return;

	flow_control_ = dtc->flow_ctrl;
	rtx_control_ = dtc->rtx_ctrl;
	flow_control_config_(dtc->fctrl_cfg);
	rtx_control_config_(dtc->rxctrl_cfg);
	dtcp_policy_set_(dtc->dtcp_ps);
	lost_control_pdu_policy_(dtc->lost_control_pdu);
	rtt_estimator_policy_(dtc->rtt_estimator);
}

struct dtcp_config * DTCPConfig::to_c_dtcp_config()
{
	struct dtcp_config * result;

	result = new dtcp_config();
	result->flow_ctrl = flow_control_;
	result->rtx_ctrl = rtx_control_;
	result->fctrl_cfg = flow_control_config_.to_c_fconfig();
	result->rxctrl_cfg = rtx_control_config_.to_c_rxconfig();
	result->dtcp_ps = dtcp_policy_set_.to_c_policy();
	result->lost_control_pdu = lost_control_pdu_policy_.to_c_policy();
	result->rtt_estimator = rtt_estimator_policy_.to_c_policy();

	return result;
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

const PolicyConfig& DTCPConfig::get_dtcp_policy_set() const {
        return dtcp_policy_set_;
}

void DTCPConfig::set_dtcp_policy_set(
                const PolicyConfig& dtcp_policy_set) {
	dtcp_policy_set_ = dtcp_policy_set;
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
        ss<<"DTCP Policy Set (name/version): "<<dtcp_policy_set_.get_name();
        ss<<"/"<<dtcp_policy_set_.get_version();
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

// CLASS DTP CONFIG
DTPConfig::DTPConfig(){
	dtcp_present_ = false;
	seq_num_rollover_threshold_ = 0;
	initial_a_timer_ = 0;
	partial_delivery_ = false;
	in_order_delivery_ = false;
	incomplete_delivery_ = false;
	max_sdu_gap_ = 0;
}

DTPConfig::DTPConfig(struct dtp_config * dtc)
{
	if (!dtc)
		return;

	dtcp_present_ = dtc->dtcp_present;
	seq_num_rollover_threshold_ = dtc->seq_num_ro_th;
	initial_a_timer_ = dtc->initial_a_timer;
	partial_delivery_ = dtc->partial_delivery;
	in_order_delivery_ = dtc->in_order_delivery;
	incomplete_delivery_ = dtc->incomplete_delivery;
	max_sdu_gap_ = dtc->max_sdu_gap;
	dtp_policy_set_(dtc->dtp_ps);
}

struct dtp_config * DTPConfig::to_c_dtp_config()
{
	struct dtp_config * result;

	result = new dtp_config();
	result->dtcp_present = dtcp_present_;
	result->seq_num_ro_th = seq_num_rollover_threshold_;
	result->initial_a_timer = initial_a_timer_;
	result->partial_delivery = partial_delivery_;
	result->in_order_delivery = in_order_delivery_;
	result->incomplete_delivery = incomplete_delivery_;
	result->max_sdu_gap = max_sdu_gap_;
	result->dtp_ps = dtp_policy_set_.to_c_policy();

	return result;
}

const PolicyConfig& DTPConfig::get_dtp_policy_set() const {
        return dtp_policy_set_;
}

void DTPConfig::set_dtp_policy_set(
                const PolicyConfig& dtp_policy_set) {
	dtp_policy_set_ = dtp_policy_set;
}

bool DTPConfig::is_dtcp_present() const {
	return dtcp_present_;
}

void DTPConfig::set_dtcp_present(bool dtcp_present) {
	dtcp_present_ = dtcp_present;
}

unsigned int DTPConfig::get_seq_num_rollover_threshold() const {
	return seq_num_rollover_threshold_;
}

void DTPConfig::set_seq_num_rollover_threshold(unsigned int seq_num_rollover_threshold) {
	seq_num_rollover_threshold_ = seq_num_rollover_threshold;
}

unsigned int DTPConfig::get_initial_a_timer() const {
	return initial_a_timer_;
}

void DTPConfig::set_initial_a_timer(unsigned int initial_a_timer) {
	initial_a_timer_ = initial_a_timer;
}

bool DTPConfig::is_in_order_delivery() const {
	return in_order_delivery_;
}

void DTPConfig::set_in_order_delivery(bool in_order_delivery) {
	in_order_delivery_ = in_order_delivery;
}

int DTPConfig::get_max_sdu_gap() const {
	return max_sdu_gap_;
}

void DTPConfig::set_max_sdu_gap(int max_sdu_gap) {
	max_sdu_gap_ = max_sdu_gap;
}

bool DTPConfig::is_partial_delivery() const {
	return partial_delivery_;
}

void DTPConfig::set_partial_delivery(bool partial_delivery) {
	partial_delivery_ = partial_delivery;
}

bool DTPConfig::is_incomplete_delivery() const {
	return incomplete_delivery_;
}

void DTPConfig::set_incomplete_delivery(bool incomplete_delivery) {
	incomplete_delivery_ = incomplete_delivery;
}

const std::string DTPConfig::toString() const {
        std::stringstream ss;
        ss<<"DTP Policy Set (name/version): "<<dtp_policy_set_.get_name();
        ss<<"/"<<dtp_policy_set_.get_version();
        ss<<"DTCP present: "<<dtcp_present_;
        ss<<"; Initial A-timer: "<<initial_a_timer_;
        ss<<"; Seq. num roll. threshold: "<<seq_num_rollover_threshold_;
        ss<<"; Partial delivery: "<<partial_delivery_;
        ss<<"; In order delivery: "<<in_order_delivery_;
        ss<<"; Max allowed SDU gap: "<<max_sdu_gap_<<std::endl;
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

QoSCube::QoSCube(struct qos_cube * qos)
{
	if (!qos)
		return;

	name_ = qos->name;
	id_ = qos->id;
	average_bandwidth_ = qos->avg_bw;
        average_sdu_bandwidth_ = qos->avg_sdu_bw;
        peak_bandwidth_duration_ = qos->peak_bw_duration;
        peak_sdu_bandwidth_duration_ = qos->peak_sdu_bw_duration;
        undetected_bit_error_rate_ = 0;
        partial_delivery_ = qos->partial_delivery;
        ordered_delivery_ = qos->ordered_delivery;
        max_allowable_gap_ = qos->max_allowed_gap;
        jitter_ = qos->jitter;
        delay_ = qos->delay;
        dtp_config_(qos->dtpc);
        dtcp_config_(qos->dtcpc);
}

struct qos_cube * QoSCube::to_c_qos_cube() const
{
	struct qos_cube * result;

	result = new qos_cube();
	result->name = name_.c_str();
	result->id = id_;
	result->avg_bw = average_bandwidth_;
	result->avg_sdu_bw = average_sdu_bandwidth_;
	result->peak_bw_duration = peak_bandwidth_duration_;
	result->peak_sdu_bw_duration = peak_sdu_bandwidth_duration_;
	result->partial_delivery = partial_delivery_;
	result->ordered_delivery = ordered_delivery_;
	result->max_allowed_gap = max_allowable_gap_;
	result->jitter = jitter_;
	result->delay = delay_;
	result->dtpc = dtp_config_.to_c_dtp_config();
	result->dtcpc = dtcp_config_.to_c_dtcp_config();

	return result;
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

const DTPConfig& QoSCube::get_dtp_config() const {
	return dtp_config_;
}

void QoSCube::set_dtp_config(const DTPConfig& dtp_config) {
	dtp_config_ = dtp_config;
}

const DTCPConfig& QoSCube::get_dtcp_config() const {
	return dtcp_config_;
}

void QoSCube::set_dtcp_config(const DTCPConfig& dtcp_config) {
	dtcp_config_ = dtcp_config;
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
        ss<<"DTP Configuration: "<<dtp_config_.toString();
        ss<<"DTCP Configuration: "<<dtcp_config_.toString();
        return ss.str();
}

// CLASS DATA TRANSFER CONSTANTS
DataTransferConstants::DataTransferConstants()
{
	qos_id_length_ = 0;
	port_id_length_ = 0;
	cep_id_length_ = 0;
	sequence_number_length_ = 0;
	address_length_ = 0;
	length_length_ = 0;
	max_pdu_size_ = 0;
	dif_integrity_ = false;
	max_pdu_lifetime_ = 0;
	rate_length_ = 0;
	frame_length_ = 0;
	ctrl_sequence_number_length_ = 0;
	seq_rollover_thres_ = 0;
	dif_concatenation_ = false;
	dif_fragmentation_ = false;
	max_time_to_keep_ret_ = 0;
	max_time_to_ack_ = 0;
}

DataTransferConstants::DataTransferConstants(struct dt_cons * dtc)
{
	if (!dtc)
		return;

	qos_id_length_ = dtc->qos_id_length;
	port_id_length_ = dtc->port_id_length;
	cep_id_length_ = dtc->cep_id_length;
	sequence_number_length_ = dtc->seq_num_length;
	address_length_ = dtc->address_length;
	length_length_ = dtc->length_length;
	max_pdu_size_ = dtc->max_pdu_size;
	dif_integrity_ = dtc->dif_integrity;
	max_pdu_lifetime_ = dtc->max_pdu_life;
	rate_length_ = dtc->rate_length;
	frame_length_ = dtc->frame_length;
	ctrl_sequence_number_length_ = dtc->ctrl_seq_num_length;
	seq_rollover_thres_ = dtc->seq_rollover_thres;
	dif_concatenation_ = dtc->dif_concat;
	dif_fragmentation_ = dtc->dif_frag;
	max_time_to_keep_ret_ = dtc->max_time_to_keep_ret_;
	max_time_to_ack_ = dtc->max_time_to_ack_;
}

struct dt_cons * DataTransferConstants::to_c_dt_cons() const
{
	struct dt_cons * result;

	result = new dt_cons();
	result->qos_id_length = qos_id_length_;
	result->port_id_length = port_id_length_;
	result->cep_id_length = cep_id_length_;
	result->seq_num_length = sequence_number_length_;
	result->address_length = address_length_;
	result->length_length = length_length_;
	result->max_pdu_size = max_pdu_size_;
	result->dif_integrity = dif_integrity_;
	result->max_pdu_life = max_pdu_lifetime_;
	result->rate_length = rate_length_;
	result->frame_length = frame_length_;
	result->ctrl_seq_num_length = ctrl_sequence_number_length_;
	result->seq_rollover_thres = seq_rollover_thres_;
	result->dif_concat = dif_concatenation_;
	result->dif_frag = dif_fragmentation_;
	result->max_time_to_keep_ret_ = max_time_to_keep_ret_;
	result->max_time_to_ack_ = max_time_to_ack_;

	return result;
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

unsigned short DataTransferConstants::get_rate_length() const {
	return rate_length_;
}

void DataTransferConstants::set_rate_length(unsigned short rate_length) {
	rate_length_ = rate_length;
}

unsigned short DataTransferConstants::get_frame_length() const {
	return frame_length_;
}

void DataTransferConstants::set_frame_length(unsigned short frame_length) {
	frame_length_ = frame_length;
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

unsigned short DataTransferConstants::get_ctrl_sequence_number_length() const {
	return ctrl_sequence_number_length_;
}

void DataTransferConstants::set_ctrl_sequence_number_length(unsigned short ctrl_sequence_number_length) {
	ctrl_sequence_number_length_ = ctrl_sequence_number_length;
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

EFCPConfiguration::EFCPConfiguration(struct efcp_config* efc)
{
	QoSCube * qos_cube = 0;
	struct qos_cube_entry * pos;

	if (!efc)
		return;

	data_transfer_constants_(efc->dt_cons);
	unknown_flowpolicy_(efc->unknown_flow);
	list_for_each_entry(pos, &(efc->qos_cubes), next) {
		if (pos->entry) {
			qos_cube = new QoSCube(pos->entry);
			add_qos_cube(qos_cube);
		}
	}
}

struct efcp_config * EFCPConfiguration::to_c_efcp_conf() const
{
	struct efcp_config * result;
	std::list<QoSCube *>::iterator it;
	struct qos_cube_entry * qos_entry;

	result = new efcp_config();
	INIT_LIST_HEAD(&result->qos_cubes);
	result->pci_offset_table = 0;
	result->dt_cons = data_transfer_constants_.to_c_dt_cons();
	result->unknown_flow = unknown_flowpolicy_.to_c_policy();
	for (it = qos_cubes_.begin(); it != qos_cubes_.end(); ++it) {
		qos_entry = new qos_cube_entry();
		INIT_LIST_HEAD(&qos_entry->next);
		qos_entry->entry = (*it)->to_c_qos_cube();
		list_add_tail(&qos_entry->next, &result->qos_cubes);
	}

	return result;
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

const std::list<QoSCube *>& EFCPConfiguration::get_qos_cubes() const{
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

// CLASS NamespaceManagerConfiguration
NamespaceManagerConfiguration::NamespaceManagerConfiguration(){
}

const PolicyConfig&
NamespaceManagerConfiguration::get_policy_set() const {
	return policy_set_;
}

void NamespaceManagerConfiguration::set_policy_set(
		const PolicyConfig& policy_set){
	policy_set_ = policy_set;
}

std::string NamespaceManagerConfiguration::toString()
{
	std::stringstream ss;
	ss << "Selected NamespaceManager Policy set. Name: " << policy_set_.name_ ;
	ss << "; Version: " << policy_set_.version_ << std::endl;

	return ss.str();
}

// CLASS RoutingConfiguration
std::string RoutingConfiguration::toString()
{
	std::stringstream ss;
	ss << "Routing policy set." << policy_set_.toString();

	return ss.str();
}

// CLASS PDUFTGConfiguration
PDUFTGConfiguration::PDUFTGConfiguration(){
}

std::string PDUFTGConfiguration::toString()
{
	std::stringstream ss;
	ss << "Selected PDU Forwarding Table Generator Policy Set." << policy_set_.toString() << std::endl;

	return ss.str();
}

// CLASS ResourceAllocatorConfiguration
ResourceAllocatorConfiguration::ResourceAllocatorConfiguration(){
}

ResourceAllocatorConfiguration::ResourceAllocatorConfiguration(struct resall_config * resc)
{
	if (!resc)
		return;

	pduftg_conf_.policy_set_(resc->pff_gen);
}

struct resall_config * ResourceAllocatorConfiguration::to_c_rall_config() const
{
	struct resall_config * result;

	result = new resall_config();
	result->pff_gen = pduftg_conf_.policy_set_.to_c_policy();

	return result;
}

std::string ResourceAllocatorConfiguration::toString()
{
	std::stringstream ss;
        ss << "Resource Allocator Configuration";
	ss << pduftg_conf_.toString() << std::endl;

	return ss.str();
}

// CLASS FlowAllocatorConfiguration
FlowAllocatorConfiguration::FlowAllocatorConfiguration(){
	max_create_flow_retries_ = 0;
}

FlowAllocatorConfiguration::FlowAllocatorConfiguration(struct fa_config * fac)
{
	if (!fac)
		return;

	max_create_flow_retries_ = fac->max_create_flow_retries;
	policy_set_(fac->ps);
	allocate_notify_policy_(fac->allocate_notify);
	allocate_retry_policy_(fac->allocate_retry);
	new_flow_request_policy_(fac->new_flow_req);
	seq_rollover_policy_(fac->seq_roll_over);
}

struct fa_config * FlowAllocatorConfiguration::to_c_fa_config() const
{
	struct fa_config * result;

	result = new fa_config();
	result->max_create_flow_retries = max_create_flow_retries_;
	result->ps = policy_set_.to_c_policy();
	result->allocate_notify = allocate_notify_policy_.to_c_policy();
	result->allocate_retry = allocate_retry_policy_.to_c_policy();
	result->new_flow_req = new_flow_request_policy_.to_c_policy();
	result->seq_roll_over = seq_rollover_policy_.to_c_policy();

	return result;
}

const PolicyConfig&
FlowAllocatorConfiguration::get_policy_set() const {
	return policy_set_;
}

void FlowAllocatorConfiguration::set_policy_set(
		const PolicyConfig& policy_set){
	policy_set_ = policy_set;
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

std::string FlowAllocatorConfiguration::toString()
{
	std::stringstream ss;
	ss << "Selected FlowAllocator Policy set. Name: " << policy_set_.name_ ;
	ss << "; Version: " << policy_set_.version_ << std::endl;

	return ss.str();
}

// CLASS PFTConfiguration
PFTConfiguration::PFTConfiguration(){
	policy_set_ = PolicyConfig();
}

PFTConfiguration::PFTConfiguration(struct pff_config * pfc)
{
	if (!pfc)
		return;

	policy_set_(pfc->policy_set);
}

struct pff_config * PFTConfiguration::to_c_pff_conf() const
{
	struct pff_config * result;

	result = new pff_config();
	result->policy_set = policy_set_.to_c_policy();

	return result;
}

std::string PFTConfiguration::toString()
{
	std::stringstream ss;
	ss << "Selected PFT Policy set. Name: " << policy_set_.name_ ;
	ss << "; Version: " << policy_set_.version_ << std::endl;

	return ss.str();
}

// CLASS RMTConfiguration
RMTConfiguration::RMTConfiguration(){
	policy_set_ = PolicyConfig();
}

RMTConfiguration::RMTConfiguration(struct rmt_config * rt)
{
	if (!rt)
		return;

	policy_set_(rt->policy_set);
	pft_conf_(rt->pff_conf);
}

struct rmt_config * RMTConfiguration::to_c_rmt_config() const
{
	struct rmt_config * result;

	result = new rmt_config();
	result->pff_conf = pft_conf_.to_c_pff_conf();
	result->policy_set = policy_set_.to_c_policy();

	return result;
}

std::string RMTConfiguration::toString()
{
	std::stringstream ss;
	ss << "Selected RMT Policy set. Name: " << policy_set_.name_ ;
	ss << "; Version: " << policy_set_.version_ << std::endl;
	ss << pft_conf_.toString() << std::endl;

	return ss.str();
}

// Class Enrollment Task Configuration
EnrollmentTaskConfiguration::EnrollmentTaskConfiguration()
{
}

EnrollmentTaskConfiguration::EnrollmentTaskConfiguration(struct et_config * etc)
{
	if (!etc)
		return;

	policy_set_(etc->ps);
}

struct et_config * EnrollmentTaskConfiguration::to_c_et_config() const
{
	struct et_config * result;

	result = new et_config();
	result->ps = policy_set_.to_c_policy();

	return result;
}

std::string EnrollmentTaskConfiguration::toString() {
	std::stringstream ss;
	ss << "Policy set: " << policy_set_.toString() << std::endl;
	return ss.str();
}

// Class Static IPC Process Address
StaticIPCProcessAddress::StaticIPCProcessAddress() {
	address_ = 0;
}

StaticIPCProcessAddress::StaticIPCProcessAddress(struct static_ipcp_addr * addr)
{
	if (!addr)
		return;

	ap_name_ = addr->ap_name;
	ap_instance_ = addr->ap_instance;
	address_ = addr->address;
}

struct static_ipcp_addr * StaticIPCProcessAddress::t_c_stipcp_addr() const
{
	struct static_ipcp_addr * result;

	result = new static_ipcp_addr();
	result->ap_name = ap_name_.c_str();
	result->ap_instance = ap_instance_.c_str();
	result->address = address_;

	return result;
}

//Class AddressPrefixConfiguration
AddressPrefixConfiguration::AddressPrefixConfiguration() {
	address_prefix_ = 0;
}

AddressPrefixConfiguration::AddressPrefixConfiguration(struct address_pref_config * apc)
{
	if (!apc)
		return;

	address_prefix_ = apc->prefix;
	organization_ = apc->org;
}

struct address_pref_config * AddressPrefixConfiguration::to_c_pref_config() const
{
	struct address_pref_config * result;

	result = new address_pref_config();
	result->prefix = address_prefix_;
	result->org = organization_.c_str();

	return result;
}

// Class AddressingConfiguration
AddressingConfiguration::AddressingConfiguration(struct addressing_config * ac)
{
	struct static_ipcp_addr_entry * addr_pos;
	struct address_pref_config_entry * pref_pos;

	if (!ac)
		return;

        list_for_each_entry(addr_pos, &(ac->static_ipcp_addrs), next) {
        	static_address_.push_back(StaticIPCProcessAddress(addr_pos));
        }

        list_for_each_entry(pref_pos, &(ac->address_prefixes), next) {
        	address_prefixes_.push_back(AddressPrefixConfiguration(pref_pos));
        }
}

struct addressing_config * AddressingConfiguration::to_c_addr_config() const
{
	struct addressing_config * result;
	struct static_ipcp_addr_entry * addr_pos;
	struct address_pref_config_entry * pref_pos;
	std::list<StaticIPCProcessAddress>::iterator addr_it;
	std::list<AddressPrefixConfiguration>::iterator prefix_it;

	result = new addressing_config();
	INIT_LIST_HEAD(&result->address_prefixes);
	INIT_LIST_HEAD(&result->static_ipcp_addrs);

	for(addr_it = static_address_.begin();
			addr_it != static_address_.end(); ++addr_it) {
		addr_pos = new static_ipcp_addr_entry();
		INIT_LIST_HEAD(&addr_pos->next);
		addr_pos->entry = new static_ipcp_addr();
		addr_pos->entry->address = addr_it->address_;
		addr_pos->entry->ap_name = addr_it->ap_name_.c_str();
		addr_pos->entry->ap_instance = addr_it->ap_instance_.c_str();
		list_add_tail(&addr_pos->next, &result->static_ipcp_addrs);
	}

	for(prefix_it = address_prefixes_.begin();
			prefix_it != address_prefixes_.end(); ++addr_it) {
		pref_pos = new static_ipcp_addr_entry();
		INIT_LIST_HEAD(&pref_pos->next);
		pref_pos->entry = new address_pref_config();
		pref_pos->entry->prefix = prefix_it->address_prefix_;
		pref_pos->entry->org = prefix_it->organization_.c_str();
		list_add_tail(&pref_pos->next, &result->address_prefixes);
	}

	return result;
}

void AddressingConfiguration::addAddress(StaticIPCProcessAddress &addr)
{
	static_address_.push_back(addr);
}
void AddressingConfiguration::addPrefix(AddressPrefixConfiguration &pref)
{
	address_prefixes_.push_back(pref);
}
//Class AuthSDUProtectionProfile
std::string AuthSDUProtectionProfile::to_string()
{
	std::stringstream ss;
	ss << "Auth policy" << std::endl;
	ss << authPolicy.toString() << std::endl;
	ss << "Encrypt policy" << std::endl;
	ss << encryptPolicy.toString() << std::endl;
	ss << "Error check policy" << std::endl;
	ss << crcPolicy.toString() <<std::endl;
	ss << "TTL policy" << std::endl;
	ss << ttlPolicy.toString() << std::endl;

	return ss.str();
}

//Class SecurityManagerConfiguration
std::string SecurityManagerConfiguration::toString()
{
	std::stringstream ss;
	ss << "Security Manager policy set" << std::endl;
	ss << policy_set_.toString() << std::endl;
	ss << "Default auth-sdup profile" << std::endl;
	ss << default_auth_profile.to_string() << std::endl;
	if (specific_auth_profiles.size () > 0) {
		ss << "Specific auth-sdup profiles" << std::endl;
		for (std::map<std::string, AuthSDUProtectionProfile>::iterator it = specific_auth_profiles.begin();
				it != specific_auth_profiles.end(); ++it) {
			ss << "N-1 DIF name: " << it->first << "; Profile: " <<std::endl;
			ss << it->second.to_string() << std::endl;
		}
	}

	return ss.str();
}

// CLASS DIF CONFIGURATION
DIFConfiguration::DIFConfiguration(){
	address_ = 0;
}

DIFConfiguration::DIFConfiguration(struct dif_config * dc)
{
	address_ = dc->address;
}

struct dif_config * DIFConfiguration::to_c_dif_config() const
{
	struct dif_config * result = new dif_config();

	result->address = address_;

	//TODO

	return result;
}

unsigned int DIFConfiguration::get_address() const {
	return address_;
}

void DIFConfiguration::set_address(unsigned int address) {
	address_ = address;
}

const std::list<PolicyParameter>& DIFConfiguration::get_parameters() const {
	return parameters_;
}

void DIFConfiguration::set_parameters(const std::list<PolicyParameter>& parameters) {
	parameters_ = parameters;
}

void DIFConfiguration::add_parameter(const PolicyParameter& parameter){
	parameters_.push_back(parameter);
}

const EFCPConfiguration& DIFConfiguration::get_efcp_configuration() const {
	return efcp_configuration_;
}

void DIFConfiguration::set_efcp_configuration(
		const EFCPConfiguration& efcp_configuration) {
	efcp_configuration_= efcp_configuration;
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
DIFInformation::DIFInformation(struct dif_config * dc, struct name * name,
			       string_t * type)
{
	dif_name_.processName = name->process_name;
	dif_type_ = type;
	dif_configuration_(dc);
}

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

} //namespace rina
