/*
 * Classes related to RINA configuration
 *
 *    Eduard Grasa <eduard.grasa@i2cat.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef LIBRINA_RINACONF_H
#define LIBRINA_RINACONF_H

#ifdef __cplusplus

#include <string>

namespace rina {

/// A parameter of a policy configuration
class PolicyParameter {
public:
        PolicyParameter();
        PolicyParameter(const std::string& name, const std::string& value);
        bool operator==(const PolicyParameter &other) const;
        bool operator!=(const PolicyParameter &other) const;
        const std::string& get_name() const;
        void set_name(const std::string& name);
        const std::string& get_value() const;
        void set_value(const std::string& value);

private:
        /// the name of the parameter
        std::string name_;

        /// the value of the parameter
        std::string value_;
};

///The DTCP window based flow control configuration
class DTCPWindowBasedFlowControlConfig {
public:
    DTCPWindowBasedFlowControlConfig();
    unsigned int get_initial_credit() const;
    void set_initial_credit(int initial_credit);
    unsigned int get_maxclosed_window_queue_length() const;
    void set_max_closed_window_queue_length(unsigned int max_closed_window_queue_length);
    const PolicyConfig& get_rcvr_flow_control_policy() const;
    void set_rcvr_flow_control_policy(
                    const PolicyConfig& rcvr_flow_control_policy);
    const PolicyConfig& getTxControlPolicy() const;
    void set_tx_control_policy(const PolicyConfig& tx_control_policy);
    const std::string toString();

private:
        /// Integer that the number PDUs that can be put on the
        /// ClosedWindowQueue before something must be done.
        unsigned int max_closed_window_queue_length_;

        /// initial sequence number to get right window edge.
        unsigned int initial_credit_;

        /// Invoked when a Transfer PDU is received to give the receiving PM an
        /// opportunity to update the flow control allocations.
        PolicyConfig rcvr_flow_control_policy_;

        /// This policy is used when there are conditions that warrant sending
        /// fewer PDUs than allowed by the sliding window flow control, e.g.
        /// the ECN bit is set.
        PolicyConfig tx_control_policy_;
};

/// The DTCP rate-basd flow control configuration
class DTCPRateBasedFlowControlConfig {
public:
        DTCPRateBasedFlowControlConfig();
        const PolicyConfig& get_no_override_default_peak_policy() const;
        void set_no_override_default_peak_policy(
                        const PolicyConfig& no_override_default_peak_policy);
        const PolicyConfig& get_no_rate_slow_down_policy() const;
        void set_no_rate_slow_down_policy(
                        const PolicyConfig& no_rate_slow_down_policy);
        const PolicyConfig& get_rate_reduction_policy() const;
        void set_rate_reduction_policy(const PolicyConfig& rate_reduction_policy);
        unsigned int get_sending_rate() const;
        void set_sending_rate(unsigned int sending_rate);
        unsigned int get_time_period() const;
        void set_time_period(unsigned int time_period);
        const std::string toString();

private:
        /// the number of PDUs that may be sent in a TimePeriod. Used with
        /// rate-based flow control.
        unsigned int sending_rate_;

        /// length of time in microseconds for pacing rate-based flow control.
        unsigned int time_period_;

        /// used to momentarily lower the send rate below the rate allowed
        PolicyConfig no_rate_slow_down_policy_;

        /// Allows rate-based flow control to exceed its nominal rate.
        /// Presumably this would be for short periods and policies should
        /// enforce this.  Like all policies, if this returns True it creates
        /// the default action which is no override.
        PolicyConfig no_override_default_peak_policy_;

        /// Allows an alternate action when using rate-based flow control and
        /// the number of free buffers is getting low.
        PolicyConfig rate_reduction_policy_;
};

}

#endif

#endif
