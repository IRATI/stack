/*
 * Classes related to RINA configuration
 *
 *    Eduard Grasa <eduard.grasa@i2cat.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#ifndef LIBRINA_CONFIGURATION_H
#define LIBRINA_CONFIGURATION_H

#ifdef __cplusplus

#include "librina/common.h"

#define RINA_DEFAULT_POLICY_NAME "default"
#define RINA_DEFAULT_POLICY_VERSION "0"

namespace rina {

/// A parameter of a policy configuration
class PolicyParameter {
public:
        PolicyParameter();
        PolicyParameter(const std::string& name, const std::string& value);
        bool operator==(const PolicyParameter &other) const;
        bool operator!=(const PolicyParameter &other) const;
#ifndef SWIG
        const std::string& get_name() const;
        void set_name(const std::string& name);
        const std::string& get_value() const;
        void set_value(const std::string& value);
#endif

        /// the name of the parameter
        std::string name_;

        /// the value of the parameter
        std::string value_;
};

/// Configuration of a policy (name/version/parameters)
class PolicyConfig {
public:
        PolicyConfig();
        PolicyConfig(const std::string& name, const std::string& version);
        bool operator==(const PolicyConfig &other) const;
        bool operator!=(const PolicyConfig &other) const;
#ifndef SWIG
        const std::string& get_name() const;
        void set_name(const std::string& name);
        const std::list<PolicyParameter>& get_parameters() const;
        void set_parameters(const std::list<PolicyParameter>& parameters);
        void add_parameter(const PolicyParameter& paremeter);
        const std::string& get_version() const;
        void set_version(const std::string& version);
#endif

        /// the name of policy
        std::string name_;

        /// the version of the policy
        std::string version_;

        /// optional name/value parameters to configure the policy
        std::list<PolicyParameter> parameters_;
};

///The DTCP window based flow control configuration
class DTCPWindowBasedFlowControlConfig {
public:
    DTCPWindowBasedFlowControlConfig();
#ifndef SWIG
    unsigned int get_initial_credit() const;
    void set_initial_credit(int initial_credit);
    unsigned int get_maxclosed_window_queue_length() const;
    void set_max_closed_window_queue_length(unsigned int max_closed_window_queue_length);
    const PolicyConfig& get_rcvr_flow_control_policy() const;
    void set_rcvr_flow_control_policy(
                    const PolicyConfig& rcvr_flow_control_policy);
    const PolicyConfig& getTxControlPolicy() const;
    void set_tx_control_policy(const PolicyConfig& tx_control_policy);
#endif
    const std::string toString();

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
#ifndef SWIG
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
#endif
        const std::string toString();

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

/// The flow control configuration of a DTCP instance
class DTCPFlowControlConfig {
public:
        DTCPFlowControlConfig();
#ifndef SWIG
        const PolicyConfig& get_closed_window_policy() const;
        void set_closed_window_policy(const PolicyConfig& closed_window_policy);
        const PolicyConfig& get_flow_control_overrun_policy() const;
        void set_flow_control_overrun_policy(
                        const PolicyConfig& flow_control_overrun_policy);
        bool is_rate_based() const;
        void set_rate_based(bool rate_based);
        const DTCPRateBasedFlowControlConfig& get_rate_based_config() const;
        void set_rate_based_config(
                        const DTCPRateBasedFlowControlConfig& rate_based_config);
        int get_rcv_buffers_threshold() const;
        void set_rcv_buffers_threshold(int rcv_buffers_threshold);
        int get_rcv_bytes_percent_threshold() const;
        void set_rcv_bytes_percent_threshold(int rcv_bytes_percent_threshold);
        int get_rcv_bytes_threshold() const;
        void set_rcv_bytes_threshold(int rcv_bytes_threshold);
        const PolicyConfig& get_reconcile_flow_control_policy() const;
        void set_reconcile_flow_control_policy(
                        const PolicyConfig& reconcile_flow_control_policy);
        int get_sent_buffers_threshold() const;
        void set_sent_buffers_threshold(int sent_buffers_threshold);
        int get_sent_bytes_percent_threshold() const;
        void set_sent_bytes_percent_threshold(int sent_bytes_percent_threshold);
        int get_sent_bytes_threshold() const;
        void set_sent_bytes_threshold(int sent_bytes_threshold);
        bool is_window_based() const;
        void set_window_based(bool window_based);
        const DTCPWindowBasedFlowControlConfig& get_window_based_config() const;
        void set_window_based_config(
                        const DTCPWindowBasedFlowControlConfig&
                        window_based_config);
        const PolicyConfig& get_receiving_flow_control_policy() const;
        void set_receiving_flow_control_policy(
                        const PolicyConfig& receiving_flow_control_policy);
#endif
        const std::string toString();

        ///indicates whether window-based flow control is in use
        bool window_based_;

        /// the window-based flow control configuration
        DTCPWindowBasedFlowControlConfig window_based_config_;

        /// indicates whether rate-based flow control is in use
        bool rate_based_;

        /// the rate-based flow control configuration
        DTCPRateBasedFlowControlConfig rate_based_config_;

        /// The number of free bytes below which flow control should slow or
        /// block the user from doing any more Writes.
        int sent_bytes_threshold_;

        /// The percent of free bytes below, which flow control should slow or
        /// block the user from doing any more Writes.
        int sent_bytes_percent_threshold_;

        /// The number of free buffers below which flow control should slow or
        /// block the user from doing any more Writes.
        int sent_buffers_threshold_;

        /// The number of free bytes below which flow control does not move or
        /// decreases the amount the Right Window Edge is moved.
        int rcv_bytes_threshold_;

        /// The number of free buffers at which flow control does not advance
        /// or decreases the amount the Right Window Edge is moved.
        int rcv_bytes_percent_threshold_;

        /// The percent of free buffers below which flow control should not
        /// advance or decreases the amount the Right Window Edge is moved.
        int rcv_buffers_threshold_;

        /// Used with flow control to determine the action to be taken when the
        /// receiver has not extended more credit to allow the sender to send more
        /// PDUs. Typically, the action will be to queue the PDUs until credit is
        /// extended. This action is taken by DTCP, not DTP.
        PolicyConfig closed_window_policy_;

        /// Determines what action to take if the receiver receives PDUs but the
        /// credit or rate has been exceeded
        PolicyConfig flow_control_overrun_policy_;

        /// Invoked when both Credit and Rate based flow control are in use and
        /// they disagree on whether the PM can send or receive data. If it
        /// returns True, then the PM can send or receive; if False, it cannot.
        PolicyConfig reconcile_flow_control_policy_;

        /// Allows some discretion in when to send a Flow Control PDU when there
        /// is no Retransmission Control.
        PolicyConfig receiving_flow_control_policy_;
};

/// The configuration of the retransmission control functions of a
/// DTCP instance
class DTCPRtxControlConfig{
public:
        DTCPRtxControlConfig();
#ifndef SWIG
        unsigned int get_data_rxmsn_max() const;
        void set_data_rxmsn_max(unsigned int data_rxmsn_max);
        unsigned int get_initial_rtx_time() const;
        void set_initial_rtx_time(unsigned int initial_rtx_time);
        const PolicyConfig& get_rcvr_ack_policy() const;
        void set_rcvr_ack_policy(const PolicyConfig& rcvr_ack_policy);
        const PolicyConfig& get_rcvr_control_ack_policy() const;
        void set_rcvr_control_ack_policy(
                        const PolicyConfig& rcvr_control_ack_policy);
        const PolicyConfig& get_recving_ack_list_policy() const;
        void set_recving_ack_list_policy(
                        const PolicyConfig& recving_ack_list_policy);
        void set_rtx_timer_expiry_policy(
                        const PolicyConfig& rtx_timer_expiry_policy);
        const PolicyConfig& get_rtx_timer_expiry_policy() const;
        const PolicyConfig& get_sender_ack_policy() const;
        void set_sender_ack_policy(const PolicyConfig& sender_ack_policy);
        const PolicyConfig& get_sending_ack_policy() const;
        void set_sending_ack_policy(const PolicyConfig& sending_ack_policy);
        unsigned int get_max_time_to_retry() const;
        void set_max_time_to_retry(unsigned int max_time_to_retry);
#endif
        const std::string toString();

        ///  Maximum time to attempt the retransmission of a packet, this is R.
        unsigned int max_time_to_retry_;

        /// the number of times the retransmission of a PDU will be attempted
        /// before some other action must be taken.
        unsigned int data_rxms_nmax_;

        /// Initial retransmission time: Tr. R = tr*data_rxms_max_;
        unsigned int initial_rtx_time_;

        /// Executed by the sender when a Retransmission Timer Expires. If this
        /// policy returns True, then all PDUs with sequence number less than
        /// or equal to the sequence number of the PDU associated with this
        /// timeout are retransmitted; otherwise the procedure must determine
        /// what action to take. This policy must be executed in less than the
        /// maximum time to Ack
        PolicyConfig rtx_timer_expiry_policy_;

        /// Executed by the sender and provides the Sender with some discretion
        /// on when PDUs may be deleted from the ReTransmissionQ. This is useful
        /// for multicast and similar situations where one might want to delay
        /// discarding PDUs from the retransmission queue.
        PolicyConfig sender_ack_policy_;

        /// Executed by the Sender and provides the Sender with some discretion
        ///  on when PDUs may be deleted from the ReTransmissionQ. This policy
        ///  is used in conjunction with the selective acknowledgement aspects
        ///  of the mechanism and may be useful for multicast and similar
        ///  situations where there may be a requirement to delay discarding PDUs
        ///  from the retransmission queue
        PolicyConfig recving_ack_list_policy_;

        /// Executed by the receiver of the PDU and provides some discretion in
        /// the action taken.  The default action is to either Ack immediately
        /// or to start the A-Timer and Ack the LeftWindowEdge when it expires.
        PolicyConfig rcvr_ack_policy_;

        /// This policy allows an alternate action when the A-Timer expires when
        /// DTCP is present.
        PolicyConfig sending_ack_policy_;

        /// Allows an alternate action when a Control Ack PDU is received.
        PolicyConfig rcvr_control_ack_policy_;
};

/// Configuration of the DTCP instance, including policies and its parameters
class DTCPConfig {
public:
        DTCPConfig();
#ifndef SWIG
        bool is_flow_control() const;
        void set_flow_control(bool flow_control);
        const DTCPFlowControlConfig& get_flow_control_config() const;
        void set_flow_control_config(
                        const DTCPFlowControlConfig& flow_control_config);
        const PolicyConfig& get_lost_control_pdu_policy() const;
        void set_lost_control_pdu_policy(
                        const PolicyConfig& lostcontrolpdupolicy);
        bool is_rtx_control() const;
        void set_rtx_control(bool rtx_control);
        const DTCPRtxControlConfig& get_rtx_control_config() const;
        void set_rtx_control_config(const DTCPRtxControlConfig& rtx_control_config);
        const PolicyConfig& get_rtt_estimator_policy() const;
        void set_rtt_estimator_policy(const PolicyConfig& rtt_estimator_policy);
#endif
        const std::string toString();

        /// True if flow control is required
        bool flow_control_;

        /// The flow control configuration of a DTCP instance
        DTCPFlowControlConfig flow_control_config_;

        /// True if rtx control is required
        bool rtx_control_;

        /// the rtx control configuration of a DTCP instance
        DTCPRtxControlConfig rtx_control_config_;

        /// This policy determines what action to take when the PM detects that
        /// a control PDU (Ack or Flow Control) may have been lost.  If this
        /// procedure returns True, then the PM will send a Control Ack and an
        /// empty Transfer PDU.  If it returns False, then any action is determined
        /// by the policy
        PolicyConfig lost_control_pdu_policy_;

        /// Executed by the sender to estimate the duration of the retx timer.
        /// This policy will be based on an estimate of round-trip time and the
        /// Ack or Ack List policy in use
        PolicyConfig rtt_estimator_policy_;
};

/// This class defines the policies paramenters for an EFCP connection
class ConnectionPolicies {
public:
        ConnectionPolicies();
#ifndef SWIG
        const DTCPConfig& get_dtcp_configuration() const;
        void set_dtcp_configuration(const DTCPConfig& dtcp_configuration);
        bool is_dtcp_present() const;
        void set_dtcp_present(bool dtcp_present);
        const PolicyConfig& get_initial_seq_num_policy() const;
        void set_initial_seq_num_policy(const PolicyConfig& initial_seq_num_policy);
        unsigned int get_seq_num_rollover_threshold() const;
        void set_seq_num_rollover_threshold(unsigned int seq_num_rollover_threshold);
        unsigned int get_initial_a_timer() const;
        void set_initial_a_timer(unsigned int initial_a_timer);
        bool is_in_order_delivery() const;
        void set_in_order_delivery(bool in_order_delivery);
        int get_max_sdu_gap() const;
        void set_max_sdu_gap(int max_sdu_gap);
        bool is_partial_delivery() const;
        void set_partial_delivery(bool partial_delivery);
        bool is_incomplete_delivery() const;
        void set_incomplete_delivery(bool incomplete_delivery);
        const PolicyConfig& get_rcvr_timer_inactivity_policy() const;
        void set_rcvr_timer_inactivity_policy(
        		const PolicyConfig& rcvr_timer_inactivity_policy);
        const PolicyConfig& get_sender_timer_inactivity_policy() const;
        void set_sender_timer_inactivity_policy(
        		const PolicyConfig& sender_timer_inactivity_policy);
#endif
        const std::string toString();

        /// Indicates if DTCP is required
        bool dtcp_present_;

        /// The configuration of the DTCP instance
        DTCPConfig dtcp_configuration_;

        /// used when DTCP is in use. If no PDUs arrive in this time period,
        /// the receiver should expect a DRF in the next Transfer PDU. If not,
        /// something is very wrong. The timeout value should generally be set
        /// to 3(MPL+R+A).
        PolicyConfig rcvr_timer_inactivity_policy_;

        /// used when DTCP is in use. This timer is used to detect long periods
        /// of no traffic, indicating that a DRF should be sent. If not,
        /// something is very wrong. The timeout value should generally be set
        /// to 2(MPL+R+A).
        PolicyConfig sender_timer_inactivity_policy_;

        /// This policy allows some discretion in selecting the initial sequence
        /// number, when DRF is going to be sent.
        PolicyConfig initial_seq_num_policy_;

        /// When the sequence number is increasing beyond this value, the
        /// sequence number space is close to rolling over, a new connection
        /// should be instantiated and bound to the same port-ids, so that new
        /// PDUs can be sent on the new connection.
        unsigned int seq_num_rollover_threshold_;

        /// indicates the maximum time that a receiver will wait before sending
        /// an Ack. Some DIFs may wish to set a maximum value for the DIF.
        unsigned int initial_a_timer_;

        /// True if partial delivery of an SDU is allowed, false otherwise
        bool partial_delivery_;

        /// True if incomplete delivery is allowed (one fragment of SDU
        /// delivered is the same as all the SDU delivered), false otherwise
        bool incomplete_delivery_;

        /// True if in order delivery of SDUs is mandatory, false otherwise
        bool in_order_delivery_;

        /// The maximum gap of SDUs allowed
        int max_sdu_gap_;
};

/// Defines the properties that a QoSCube is able to provide
class QoSCube {
public:
	QoSCube();
	QoSCube(const std::string& name, int id);
	bool operator==(const QoSCube &other) const;
	bool operator!=(const QoSCube &other) const;
#ifndef SWIG
	void set_id(unsigned int id);
	unsigned int get_id() const;
	const std::string& get_name() const;
	void set_name(const std::string& name);
	const ConnectionPolicies& get_efcp_policies() const;
	void set_efcp_policies(const ConnectionPolicies& efcp_policies);
	unsigned int get_average_bandwidth() const;
	void set_average_bandwidth(unsigned int average_bandwidth);
	unsigned int get_average_sdu_bandwidth() const;
	void setAverageSduBandwidth(unsigned int average_sdu_bandwidth);
	unsigned int get_delay() const;
	void set_delay(unsigned int delay);
	unsigned int get_jitter() const;
	void set_jitter(unsigned int jitter);
	int get_max_allowable_gap() const;
	void set_max_allowable_gap(int max_allowable_gap);
	bool is_ordered_delivery() const;
	void set_ordered_delivery(bool ordered_delivery);
	bool is_partial_delivery() const;
	void set_partial_delivery(bool partial_delivery);
	unsigned int get_peak_bandwidth_duration() const;
	void set_peak_bandwidth_duration(unsigned int peak_bandwidth_duration);
	unsigned int get_peak_sdu_bandwidth_duration() const;
	void set_peak_sdu_bandwidth_duration(unsigned int peak_sdu_bandwidth_duration);
	double get_undetected_bit_error_rate() const;
	void set_undetected_bit_error_rate(double undetected_bit_error_rate);
#endif
	const std::string toString();

	/// The name of the QoS cube
	std::string name_;

	/// The id of the QoS cube
	unsigned int id_;

	/// The EFCP policies associated to this QoS Cube
	ConnectionPolicies efcp_policies_;

	/// Average bandwidth in bytes/s. A value of 0 means don't care.
	unsigned int average_bandwidth_;

	/// Average bandwidth in SDUs/s. A value of 0 means don't care
	unsigned int average_sdu_bandwidth_;

	/// In milliseconds. A value of 0 means don't care
	unsigned int peak_bandwidth_duration_;

	/// In milliseconds. A value of 0 means don't care
	unsigned int peak_sdu_bandwidth_duration_;

	/// A value of 0 indicates 'do not care'
	double undetected_bit_error_rate_;

	/// Indicates if partial delivery of SDUs is allowed or not
	bool partial_delivery_;

	/// Indicates if SDUs have to be delivered in order
	bool ordered_delivery_;

	/// Indicates the maximum gap allowed among SDUs, a gap of N SDUs
	/// is considered the same as all SDUs delivered. A value of -1
	// indicates 'Any'
	int max_allowable_gap_;

	/// In milliseconds, indicates the maximum delay allowed in this
	/// flow. A value of 0 indicates 'do not care'
	unsigned int delay_;

	/// In milliseconds, indicates the maximum jitter allowed in this
    /// flow. A value of 0 indicates 'do not care'
	unsigned int jitter_;
};

/// Contains the values of the constants for the Error and Flow Control
/// Protocol (EFCP)
class DataTransferConstants {
public:
        DataTransferConstants();
#ifndef SWIG
        unsigned short get_address_length() const;
        void set_address_length(unsigned short address_length);
        unsigned short get_cep_id_length() const;
        void set_cep_id_length(unsigned short cep_id_length);
        bool is_dif_integrity() const;
        void set_dif_integrity(bool dif_integrity);
        unsigned short get_length_length() const;
        void set_length_length(unsigned short length_length);
        unsigned int get_max_pdu_lifetime() const;
        void set_max_pdu_lifetime(unsigned int max_pdu_lifetime);
        unsigned int get_max_pdu_size() const;
        void set_max_pdu_size(unsigned int max_pdu_size);
        unsigned short get_port_id_length() const;
        void set_port_id_length(unsigned short port_id_length);
        unsigned short get_qos_id_length() const;
        void set_qos_id_length(unsigned short qos_id_length);
        unsigned short get_sequence_number_length() const;
        void set_sequence_number_length(unsigned short sequence_number_length);
#endif
        bool isInitialized();
        const std::string toString();

        /// The length of QoS-id field in the DTP PCI, in bytes
        unsigned short qos_id_length_;

        /// The length of the Port-id field in the DTP PCI, in bytes
        unsigned short port_id_length_;

        /// The length of the CEP-id field in the DTP PCI, in bytes
        unsigned short cep_id_length_;

        /// The length of the sequence number field in the DTP PCI, in bytes
        unsigned short sequence_number_length_;

        /// The length of the address field in the DTP PCI, in bytes
        unsigned short address_length_;

        /// The length of the length field in the DTP PCI, in bytes
        unsigned short length_length_;

        /// The maximum length allowed for a PDU in this DIF, in bytes
        unsigned int max_pdu_size_;

        /// True if the PDUs in this DIF have CRC, TTL, and/or encryption. Since
        /// headers are encrypted, not just user data, if any flow uses encryption,
        /// all flows within the same DIF must do so and the same encryption
        /// algorithm must be used for every PDU; we cannot identify which flow
        /// owns a particular PDU until it has been decrypted.
        bool dif_integrity_;

        /// The maximum PDU lifetime in this DIF, in milliseconds. This is MPL
        /// in delta-T
        unsigned int max_pdu_lifetime_;
};

/// Contains the configuration of the Error and Flow Control Protocol for a
/// particular DIF
class EFCPConfiguration {
public:
        EFCPConfiguration();
        EFCPConfiguration(const EFCPConfiguration& other);
        EFCPConfiguration& operator=(const EFCPConfiguration & rhs);
        ~EFCPConfiguration();
#ifndef SWIG
        const DataTransferConstants& get_data_transfer_constants() const;
        void set_data_transfer_constants(
                        const DataTransferConstants& data_transfer_constants);
        const std::list<QoSCube *>& get_qos_cubes() const;
        void add_qos_cube(QoSCube* qos_cube);
        const PolicyConfig& get_unknown_flow_policy() const;
        void set_unknown_flow_policy(const PolicyConfig& unknown_flow_policy);
#endif

        /// DIF-wide parameters that define the concrete syntax of EFCP for this
        /// DIF and other DIF-wide values
        DataTransferConstants data_transfer_constants_;

        /// When a PDU arrives for a Data Transfer Flow terminating in this
        /// IPC-Process and there is no active DTSV, this policy consults the
        /// ResourceAllocator to determine what to do.
        PolicyConfig unknown_flowpolicy_;

        /// The QoS cubes supported by the DIF, and its associated EFCP policies
        std::list<QoSCube*> qos_cubes_;

private:
        void copy(const EFCPConfiguration & other);
};

/// Configuration of the Flow Allocator
class FlowAllocatorConfiguration {
public:
        FlowAllocatorConfiguration();
#ifndef SWIG
        const PolicyConfig& get_allocate_notify_policy() const;
        void set_allocate_notify_policy(const PolicyConfig& allocate_notify_policy);
        const PolicyConfig& get_allocate_retry_policy() const;
        void set_allocate_retry_policy(const PolicyConfig& allocate_retry_policy);
        int get_max_create_flow_retries() const;
        void set_max_create_flow_retries(int max_create_flow_retries);
        const PolicyConfig& get_new_flow_request_policy() const;
        void set_new_flow_request_policy(const PolicyConfig& new_flow_request_policy);
        const PolicyConfig& get_seq_rollover_policy() const;
        void set_seq_rollover_policy(const PolicyConfig& seq_rollover_policy);
#endif

        /// Maximum number of attempts to retry the flow allocation
        int max_create_flow_retries_;

        /// This policy determines when the requesting application is given
        /// an Allocate_Response primitive. In general, the choices are once
        /// the request is determined to be well-formed and a create_flow
        /// request has been sent, or withheld until a create_flow response has
        /// been received and MaxCreateRetires has been exhausted.
        PolicyConfig allocate_notify_policy_;

        /// This policy is used when the destination has refused the create_flow
        /// request, and the FAI can overcome the cause for refusal and try
        /// again. This policy should re-formulate the request. This policy
        /// should formulate the contents of the reply.
        PolicyConfig allocate_retry_policy_;

        /// This policy is used to convert an allocate request to a create flow
        /// request. Its primary task is to translate the request into the
        /// proper QoS-class set, flow set and access control capabilities.
        PolicyConfig new_flow_request_policy_;

        /// This policy is used when the SeqRollOverThres event occurs and
        /// action may be required by the Flow Allocator to modify the bindings
        /// between connection-endpoint-ids and port-ids.
        PolicyConfig seq_rollover_policy_;
};

/// Contains the configuration data of the Relaying and Multiplexing Task for a
/// particular DIF
class RMTConfiguration {
public:
        RMTConfiguration() { };

        PolicyConfig pdu_forwarding_policy_;

        /// Three parameters are provided to monitor the queues. This policy
        /// can be invoked whenever a PDU is placed in a queue and may keep
        /// additional variables that may be of use to the decision process of
        /// the RMT-Scheduling Policy and the MaxQPolicy.
        PolicyConfig q_monitor_policy_;

        /// This policy is invoked when a queue reaches or crosses the threshold
        /// or maximum queue lengths allowed for this queue. Note that maximum
        /// length may be exceeded.
        PolicyConfig max_q_policy_;

        /// This is the meat of the RMT. This is the scheduling algorithm that
        /// determines the order input and output queues are serviced. We have
        /// not distinguished inbound from outbound. That is left to the policy.
        /// To do otherwise, would impose a policy. This policy may implement
        /// any of the standard scheduling algorithms, FCFS, LIFO,
        /// longestQfirst, priorities, etc.
        PolicyConfig scheduling_policy_;
};

/// Link State routing configuration
class LinkStateRoutingConfiguration {
public:
        LinkStateRoutingConfiguration();
        const std::string toString();
#ifndef SWIG
        int get_wait_until_age_increment() const;
        void set_wait_until_age_increment(const int wait_until_age_increment);
        int get_wait_until_error() const;
        void set_wait_until_error(const int wait_until_error);
        int get_wait_until_fsodb_propagation() const;
        void set_wait_until_fsodb_propagation(const int wait_until_fsodb_propagation);
        int get_wait_until_pduft_computation() const;
        void set_wait_until_pduft_computation(const int wait_until_pduft_computation);
        int get_wait_until_read_cdap() const;
        void set_wait_until_read_cdap(const int wait_until_read_cdap);
        unsigned int get_object_maximum_age() const;
        void set_object_maximum_age(const int object_maximum_age);
        const std::string& get_routing_algorithm() const;
        void set_routing_algorithm(const std::string& routing_algorithm);
#endif

        static const int PULSES_UNTIL_FSO_EXPIRATION_DEFAULT = 100000;
        static const int WAIT_UNTIL_READ_CDAP_DEFAULT = 5001;
        static const int WAIT_UNTIL_ERROR_DEFAULT = 5001;
        static const int WAIT_UNTIL_PDUFT_COMPUTATION_DEFAULT = 103;
        static const int WAIT_UNTIL_FSODB_PROPAGATION_DEFAULT = 101;
        static const int WAIT_UNTIL_AGE_INCREMENT_DEFAULT = 997;
        static const std::string DEFAULT_ROUTING_ALGORITHM;

        unsigned int object_maximum_age_;
        int wait_until_read_cdap_;
        int wait_until_error_;
        int wait_until_pduft_computation_;
        int wait_until_fsodb_propagation_;
        int wait_until_age_increment_;
        std::string routing_algorithm_;
};

/// PDU F Table Generator Configuration
class PDUFTableGeneratorConfiguration {
public:
        PDUFTableGeneratorConfiguration();
        PDUFTableGeneratorConfiguration(const PolicyConfig& pduft_generator_policy);
#ifndef SWIG
        const PolicyConfig& get_pduft_generator_policy() const;
        void set_pduft_generator_policy(const PolicyConfig& pduft_generator_policy);
        const LinkStateRoutingConfiguration& get_link_state_routing_configuration() const;
        void set_link_state_routing_configuration(
                        const LinkStateRoutingConfiguration& link_state_routing_configuration);
#endif

        /// Name, version and configuration of the PDU FT Generator policy
        PolicyConfig pduft_generator_policy_;

        /// Link state routing configuration parameters - only relevant if a
        /// link-state routing PDU FT Generation policy is used
        LinkStateRoutingConfiguration link_state_routing_configuration_;
};

/// Configuration of the resource allocator
class EnrollmentTaskConfiguration {
public:
	EnrollmentTaskConfiguration();

	// The maximum time to wait between steps of the enrollment program,
	// before considering enrollment failed
	int enrollment_timeout_in_ms_;

	// The period of the watchdog in ms (monitors status of neighbors)
	int watchdog_period_in_ms_;

	//The time to declare a neighbor dead (in ms)
	int declared_dead_interval_in_ms_;

	//The period of the neighbor enroller (in ms)
	int neighbor_enroller_period_in_ms_;

	//The maximum number of enrollment attempts
	unsigned int max_number_of_enrollment_attempts_;
};

/// Configuration of a static IPC Process address
class StaticIPCProcessAddress {
public:
	StaticIPCProcessAddress();

	std::string ap_name_;
	std::string ap_instance_;
	unsigned int address_;
};

/// Assigns an address prefix to a certain substring (the organization)
/// that is part of the application process name
class AddressPrefixConfiguration {
public:
	static const int MAX_ADDRESSES_PER_PREFIX = 16;

	AddressPrefixConfiguration();

	/// The address prefix (it is the first valid address
	/// for the given subdomain)
	unsigned int address_prefix_;

	/// The organization whose addresses start by the prefix
	std::string organization_;
};

/// Configuration of the assignment of addresses to IPC Processes
class AddressingConfiguration {
public:
	std::list<StaticIPCProcessAddress> static_address_;
	std::list<AddressPrefixConfiguration> address_prefixes_;
};

/// Configuration of the namespace manager
class NamespaceManagerConfiguration {
public:
	AddressingConfiguration addressing_configuration_;
};

/// Configuration of the Security Manager
class SecurityManagerConfiguration {
public:
	/// Access control policy for allowing new members into a DIF
	PolicyConfig difMemberAccessControlPolicy;

	/// Access control policy for accepting flows
	PolicyConfig newFlowAccessControlPolicy;

	/// The authentication policy for new members of the DIF
	PolicyConfig authenticationPolicy;
};

/// Contains the data about a DIF Configuration
/// (QoS cubes, policies, parameters, etc)
class DIFConfiguration {
public:
#ifndef SWIG
  DIFConfiguration();
	unsigned int get_address() const;
	void set_address(unsigned int address);
	const EFCPConfiguration& get_efcp_configuration() const;
	void set_efcp_configuration(const EFCPConfiguration& efcp_configuration);
	const PDUFTableGeneratorConfiguration&
	get_pduft_generator_configuration() const;
	void set_pduft_generator_configuration(
			const PDUFTableGeneratorConfiguration& pduft_generator_configuration);
	const RMTConfiguration& get_rmt_configuration() const;
	void set_rmt_configuration(const RMTConfiguration& rmt_configuration);
	const std::list<PolicyConfig>& get_policies();
	void set_policies(const std::list<PolicyConfig>& policies);
	void add_policy(const PolicyConfig& policy);
	const std::list<Parameter>& get_parameters() const;
	void set_parameters(const std::list<Parameter>& parameters);
	void add_parameter(const Parameter& parameter);
	const FlowAllocatorConfiguration& get_fa_configuration() const;
	void set_fa_configuration(
			const FlowAllocatorConfiguration& fa_configuration);
#endif

	/// The address of the IPC Process in the DIF
	unsigned int address_;

	/// Configuration of the Error and Flow Control Protocol
	EFCPConfiguration efcp_configuration_;

	/// Configuration of the Relaying and Multiplexing Task
	RMTConfiguration rmt_configuration_;

	/// PDUFT Configuration parameters of the DIF
	PDUFTableGeneratorConfiguration pduft_generator_configuration_;

	/// Flow Allocator configuration parameters of the DIF
	FlowAllocatorConfiguration fa_configuration_;

	/// Configuration of the enrollment Task
	EnrollmentTaskConfiguration et_configuration_;

	/// Configuration of the NamespaceManager
	NamespaceManagerConfiguration nsm_configuration_;

	/// Configuration of the security manager
	SecurityManagerConfiguration sm_configuration_;

        /// Policy sets
        std::list<Parameter> policy_sets;

	/// Other configuration parameters of the DIF
	std::list<Parameter> parameters_;

	/// Other policies of the DIF
	std::list<PolicyConfig> policies_;
};

/// Contains the information about a DIF (name, type, configuration)
class DIFInformation{
public:
	const ApplicationProcessNamingInformation& get_dif_name() const;
#ifndef SWIG
	void set_dif_name(const ApplicationProcessNamingInformation& dif_name);
	const std::string& get_dif_type() const;
	void set_dif_type(const std::string& dif_type);
	const DIFConfiguration& get_dif_configuration() const;
	void set_dif_configuration(const DIFConfiguration& dif_configuration);
#endif

	/// The type of DIF
	std::string dif_type_;

	/// The name of the DIF
	ApplicationProcessNamingInformation dif_name_;

	/// The DIF Configuration (qoscubes, policies, parameters, etc)
	DIFConfiguration dif_configuration_;
};

}

#endif

#endif
