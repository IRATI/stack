//
// Classes related to RINA configuration
//
//    Eduard Grasa <eduard.grasa@i2cat.net>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include <sstream>

#define RINA_PREFIX "rinaconf"

#include "librina/rinaconf.h"

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
        ss<<"Receiver flow control policy (name/version): "<<rcvr_flow_control_policy_.getName();
        ss<<"/"<<rcvr_flow_control_policy_.getVersion();
        ss<<"; Transmission control policy (name/version): "<<tx_control_policy_.getName();
        ss<<"/"<<tx_control_policy_.getVersion();
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
        ss<<"No rate slowdown policy (name/version): "<<no_rate_slow_down_policy_.getName();
        ss<<"/"<<no_rate_slow_down_policy_.getVersion();
        ss<<"; No override default peak policy (name/version): "<<no_override_default_peak_policy_.getName();
        ss<<"/"<<no_override_default_peak_policy_.getVersion()<<std::endl;
        ss<<"Rate reduction policy (name/version): "<<rate_reduction_policy_.getName();
        ss<<"/"<<rate_reduction_policy_.getVersion()<<std::endl;
        return ss.str();
}

}
