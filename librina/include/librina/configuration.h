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
#define RINA_NO_POLICY_NAME "nopolicy"
#define RINA_DEFAULT_POLICY_VERSION "1"

namespace rina {

/// A parameter of a policy configuration
class PolicyParameter {
 public:
    PolicyParameter();
    PolicyParameter(const std::string& name, const std::string& value);
    bool operator==(const PolicyParameter &other) const;
    bool operator!=(const PolicyParameter &other) const;
    std::string toString();
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
	static void from_c_policy(PolicyConfig & policy, struct policy * pc);
	struct policy * to_c_policy(void) const;
	bool operator==(const PolicyConfig &other) const;
	bool operator!=(const PolicyConfig &other) const;
	/// Get a parameter value as a string
	/// @throws Exception if parameter is not found
	std::string get_param_value_as_string(const std::string& name) const;
	/// Get a parameter value as long
	/// @throws Exception if parameter is not found or the conversion
	/// to long fails
	long get_param_value_as_long(const std::string& name) const;
	float get_param_value_as_float(const std::string& name) const;
	int get_param_value_as_int(const std::string& name) const;
	unsigned long get_param_value_as_ulong(const std::string& name) const;
	unsigned int get_param_value_as_uint(const std::string& name) const;
	bool get_param_value_as_bool(const std::string& name) const;
	std::string toString();
	void add_parameter(const PolicyParameter& paremeter);
#ifndef SWIG
	const std::string& get_name() const;
	void set_name(const std::string& name);
	const std::list<PolicyParameter>& get_parameters() const;
	void set_parameters(const std::list<PolicyParameter>& parameters);
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
	bool operator==(const DTCPWindowBasedFlowControlConfig &other) const;
	bool operator!=(const DTCPWindowBasedFlowControlConfig &other) const;
	static void from_c_window_config(DTCPWindowBasedFlowControlConfig & fc,
			struct window_fctrl_config * wfc);
	struct window_fctrl_config * to_c_window_config(void) const;
#ifndef SWIG
	unsigned int get_initial_credit() const;
	void set_initial_credit(int initial_credit);
	unsigned int get_maxclosed_window_queue_length() const;
	void set_max_closed_window_queue_length(
			unsigned int max_closed_window_queue_length);
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
	bool operator==(const DTCPRateBasedFlowControlConfig &other) const;
	bool operator!=(const DTCPRateBasedFlowControlConfig &other) const;
	static void from_c_rate_config(DTCPRateBasedFlowControlConfig & rf,
			struct rate_fctrl_config * rfg);
	struct rate_fctrl_config * to_c_rate_config(void) const;
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
	bool operator==(const DTCPFlowControlConfig &other) const;
	bool operator!=(const DTCPFlowControlConfig &other) const;
	static void from_c_fconfig(DTCPFlowControlConfig & fc,
			struct dtcp_fctrl_config * fcc);
	struct dtcp_fctrl_config * to_c_fconfig(void) const;
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
			const DTCPWindowBasedFlowControlConfig& window_based_config);
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
class DTCPRtxControlConfig {
public:
	DTCPRtxControlConfig();
	bool operator==(const DTCPRtxControlConfig &other) const;
	bool operator!=(const DTCPRtxControlConfig &other) const;
	static void from_c_rxconfig(DTCPRtxControlConfig & dr,
			struct dtcp_rxctrl_config * drc);
	struct dtcp_rxctrl_config * to_c_rxconfig(void) const;
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
	bool operator==(const DTCPConfig &other) const;
	bool operator!=(const DTCPConfig &other) const;
	static void from_c_dtcp_config(DTCPConfig & dt,
			struct dtcp_config* dtc);
	struct dtcp_config * to_c_dtcp_config(void) const;
#ifndef SWIG
	bool is_flow_control() const;
	void set_flow_control(bool flow_control);
	const DTCPFlowControlConfig& get_flow_control_config() const;
	void set_flow_control_config(
			const DTCPFlowControlConfig& flow_control_config);
	const PolicyConfig& get_lost_control_pdu_policy() const;
	void set_lost_control_pdu_policy(const PolicyConfig& lostcontrolpdupolicy);
	bool is_rtx_control() const;
	void set_rtx_control(bool rtx_control);
	const DTCPRtxControlConfig& get_rtx_control_config() const;
	void set_rtx_control_config(const DTCPRtxControlConfig& rtx_control_config);
	const PolicyConfig& get_dtcp_policy_set() const;
	void set_dtcp_policy_set(const PolicyConfig& dtcp_policy_set);
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

/// Policy set for DTCP.
PolicyConfig dtcp_policy_set_;

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

/// This class defines the policies paramenters for the DTP Component
class DTPConfig {
public:
	DTPConfig();
	bool operator==(const DTPConfig &other) const;
	bool operator!=(const DTPConfig &other) const;
	static void from_c_dtp_config(DTPConfig & dt, struct dtp_config * dtc);
	struct dtp_config * to_c_dtp_config(void) const;

#ifndef SWIG
	bool is_dtcp_present() const;
	void set_dtcp_present(bool dtcp_present);
	unsigned int get_seq_num_rollover_threshold() const;
	void set_seq_num_rollover_threshold(
			unsigned int seq_num_rollover_threshold);
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
	const PolicyConfig& get_dtp_policy_set() const;
	void set_dtp_policy_set(const PolicyConfig& dtp_policy_set);
	const PolicyConfig& get_dtcp_policy_set() const;
	void set_dtcp_policy_set(const PolicyConfig& dtcp_policy_set);
#endif
const std::string toString() const;

/// Indicates if DTCP is required
bool dtcp_present_;

/// Policy Set for DTP.
PolicyConfig dtp_policy_set_;

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
	static void from_c_qos_cube(QoSCube & qo,
				    struct qos_cube * qos);
	struct qos_cube * to_c_qos_cube(void) const;
	bool operator==(const QoSCube &other) const;
	bool operator!=(const QoSCube &other) const;
#ifndef SWIG
	void set_id(unsigned int id);
	unsigned int get_id() const;
	const std::string& get_name() const;
	void set_name(const std::string& name);
	const DTPConfig& get_dtp_config() const;
	void set_dtp_config(const DTPConfig& dtp_config);
	const DTCPConfig& get_dtcp_config() const;
	void set_dtcp_config(const DTCPConfig& dtcp_config);
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
	void set_peak_sdu_bandwidth_duration(
			unsigned int peak_sdu_bandwidth_duration);
	double get_undetected_bit_error_rate() const;
	void set_undetected_bit_error_rate(double undetected_bit_error_rate);
#endif
	const std::string toString();

	/// The name of the QoS cube
	std::string name_;

	/// The id of the QoS cube
	unsigned int id_;

	/// The DTP policies associated to this QoS Cube
	DTPConfig dtp_config_;

	/// The DTCP policies associated to this QoS Cube
	DTCPConfig dtcp_config_;

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

	/// In 1/10000, indicates the maximum loss probability allowed in
	/// htis flow. A value >= 10000 indicates 'do not care'
	unsigned short loss;
};

/// Contains the values of the constants for the Error and Flow Control
/// Protocol (EFCP)
class DataTransferConstants {
public:
	DataTransferConstants();
	bool operator==(const DataTransferConstants &other) const;
	bool operator!=(const DataTransferConstants &other) const;
	static void from_c_dt_cons(DataTransferConstants & dt,
				   struct dt_cons * dtc);
	struct dt_cons * to_c_dt_cons(void) const;
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
	unsigned short get_ctrl_sequence_number_length() const;
	void set_ctrl_sequence_number_length(
			unsigned short ctrl_sequence_number_length);
	unsigned short get_rate_length() const;
	void set_rate_length(unsigned short rate_length);
	unsigned short get_frame_length() const;
	void set_frame_length(unsigned short frame_length);
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
	/// The length of the control sequence number field in the DTCP PCI, in
	unsigned short ctrl_sequence_number_length_;
	/// The length of the address field in the DTP PCI, in bytes
	unsigned short address_length_;
	/// The length of the length field in the DTP PCI, in bytes
	unsigned short length_length_;
	/// The maximum length allowed for a PDU in this DIF, in bytes
	unsigned int max_pdu_size_;
	/// The maximum length of an SDU accepted by this DIF, in bytes
	unsigned int max_sdu_size_;
	/// The length of the rate field in the DTP PCI, in bytes
	unsigned short rate_length_;
	/// The length of the frame field in the DTP PCI, in bytes
	unsigned short frame_length_;
	/// True if the PDUs in this DIF have CRC, TTL, and/or encryption. Since
	/// headers are encrypted, not just user data, if any flow uses encryption,
	/// all flows within the same DIF must do so and the same encryption
	/// algorithm must be used for every PDU; we cannot identify which flow
	/// owns a particular PDU until it has been decrypted.
	bool dif_integrity_;
	/// The maximum PDU lifetime in this DIF, in milliseconds. This is MPL
	/// in delta-T
	unsigned int max_pdu_lifetime_;
	///The sequence number after which the Flow Allocator instance should
	///create a new EFCP connection
	unsigned int seq_rollover_thres_;
	//This is true if multiple SDUs can be delimited and concatenated
	/// within a single PDU
	bool dif_concatenation_;
	//This is true if multiple SDUs can be fragmented and reassembled
	///within a single PDU
	bool dif_fragmentation_;
	//The maximum time DTCP will try to keep retransmitting a PDU, before
	///discarding it. This is R in delta-T
	unsigned int max_time_to_keep_ret_;
	//The maximum time the receiving side of a DTCP connection will take to
	///ACK a PDU once it has received it. This is A in delta-T
	unsigned int max_time_to_ack_;
};

/// Contains the configuration of the Error and Flow Control Protocol for a
/// particular DIF
class EFCPConfiguration {
public:
	EFCPConfiguration();
	bool operator==(const EFCPConfiguration &other) const;
	bool operator!=(const EFCPConfiguration &other) const;
	static void from_c_efcp_conf(EFCPConfiguration &ef,
				     struct efcp_config* efc);
	struct efcp_config * to_c_efcp_conf(void) const;
	EFCPConfiguration(const EFCPConfiguration& other);
	EFCPConfiguration& operator=(const EFCPConfiguration & rhs);
	~EFCPConfiguration();
	void add_qos_cube(QoSCube* qos_cube);
#ifndef SWIG
	const DataTransferConstants& get_data_transfer_constants() const;
	void set_data_transfer_constants(
			const DataTransferConstants& data_transfer_constants);
	const std::list<QoSCube *>& get_qos_cubes() const;
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
	bool operator==(const FlowAllocatorConfiguration &other) const;
	bool operator!=(const FlowAllocatorConfiguration &other) const;
	static void from_c_fa_config(FlowAllocatorConfiguration & fa,
				     struct fa_config * fac);
	struct fa_config * to_c_fa_config(void) const;
	std::string toString();
#ifndef SWIG
	const PolicyConfig& get_policy_set() const;
	void set_policy_set(const PolicyConfig& policy_set);
	const PolicyConfig& get_allocate_notify_policy() const;
	void set_allocate_notify_policy(const PolicyConfig& allocate_notify_policy);
	const PolicyConfig& get_allocate_retry_policy() const;
	void set_allocate_retry_policy(const PolicyConfig& allocate_retry_policy);
	int get_max_create_flow_retries() const;
	void set_max_create_flow_retries(int max_create_flow_retries);
	const PolicyConfig& get_new_flow_request_policy() const;
	void set_new_flow_request_policy(
			const PolicyConfig& new_flow_request_policy);
	const PolicyConfig& get_seq_rollover_policy() const;
	void set_seq_rollover_policy(const PolicyConfig& seq_rollover_policy);
#endif
	/// Policy set for Flow Allocator
	PolicyConfig policy_set_;

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

/// Contains the configuration data of the PDUFTG, so far its policy
/// set
class PDUFTGConfiguration {
public:
	PDUFTGConfiguration();
	std::string toString();

	/// Set of policies to define the PDUFTG's behaviour.
	PolicyConfig policy_set_;
};

/// Contains the configuration data of the Resource Allocator
class ResourceAllocatorConfiguration {
public:
	ResourceAllocatorConfiguration();
	bool operator==(const ResourceAllocatorConfiguration &other) const;
	bool operator!=(const ResourceAllocatorConfiguration &other) const;
	static void from_c_rall_config(ResourceAllocatorConfiguration & res,
				       struct resall_config * resc);
	struct resall_config * to_c_rall_config(void) const;
	std::string toString();

	/// Set of policies to define the Resource Allocator's behaviour.
	PDUFTGConfiguration pduftg_conf_;
};

/// Contains the configuration data of the PDU Forwarding Table for a particular
/// DIF
class PFTConfiguration {
public:
	PFTConfiguration();
	static void from_c_pff_conf(PFTConfiguration & pf,
				    struct pff_config * pfc);
	struct pff_config * to_c_pff_conf(void) const;
	std::string toString();

	/// Set of policies to define PDU Forwarding's behaviour.
	// pft_nhop
	PolicyConfig policy_set_;
};

/// Contains the configuration data of the Relaying and Multiplexing Task for a
/// particular DIF
class RMTConfiguration {
public:
	RMTConfiguration();
	bool operator==(const RMTConfiguration &other) const;
	bool operator!=(const RMTConfiguration &other) const;
	static void from_c_rmt_config(RMTConfiguration & rm,
				      struct rmt_config * rt);
	struct rmt_config * to_c_rmt_config(void) const;

	std::string toString();

	/// Set of policies to define RMT's behaviour.
	/// QMonitor Policy, MaxQ Policy and Scheduling Policy
	PolicyConfig policy_set_;

	/// Configuration of the PFT
	PFTConfiguration pft_conf_;
};

/// Configuration of the resource allocator
class EnrollmentTaskConfiguration {
public:
	EnrollmentTaskConfiguration();
	bool operator==(const EnrollmentTaskConfiguration &other) const;
	bool operator!=(const EnrollmentTaskConfiguration &other) const;
	static void from_c_et_config(EnrollmentTaskConfiguration & et,
				     struct et_config * etc);
	struct et_config * to_c_et_config(void) const;

	/// Policy set for Enrollment Task
	PolicyConfig policy_set_;

	std::string toString();
};

/// Configuration of a static IPC Process address
class StaticIPCProcessAddress {
public:
	StaticIPCProcessAddress();
	static void from_c_stipcp_addr(StaticIPCProcessAddress & ad,
				       struct static_ipcp_addr * addr);
	struct static_ipcp_addr * t_c_stipcp_addr(void) const;

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
	static void from_c_pre_config(AddressPrefixConfiguration & ap,
				      struct address_pref_config * apc);
	struct address_pref_config * to_c_pref_config(void) const;

	/// The address prefix (it is the first valid address
	/// for the given subdomain)
	unsigned int address_prefix_;

	/// The organization whose addresses start by the prefix
	std::string organization_;
};

/// Configuration of the assignment of addresses to IPC Processes
class AddressingConfiguration {
public:
	AddressingConfiguration(){};
	bool operator==(const AddressingConfiguration &other) const;
	bool operator!=(const AddressingConfiguration &other) const;
	static void from_c_addr_config(AddressingConfiguration & a,
				       struct addressing_config * ac);
	struct addressing_config * to_c_addr_config(void) const;

	void addAddress(StaticIPCProcessAddress &addr);
	void addPrefix(AddressPrefixConfiguration &pref);
	std::list<StaticIPCProcessAddress> static_address_;
	std::list<AddressPrefixConfiguration> address_prefixes_;
};

/// Configuration of the namespace manager
class NamespaceManagerConfiguration {
public:
	NamespaceManagerConfiguration();
	bool operator==(const NamespaceManagerConfiguration &other) const;
	bool operator!=(const NamespaceManagerConfiguration &other) const;
	static void from_c_nsm_config(NamespaceManagerConfiguration & nsm,
				      struct nsm_config * nsmc);
	struct nsm_config * to_c_nsm_config(void) const;
	std::string toString();
#ifndef SWIG
	const PolicyConfig& get_policy_set() const;
	void set_policy_set(const PolicyConfig& policy_set);
#endif

	AddressingConfiguration addressing_configuration_;
	PolicyConfig policy_set_;
};

/// Configuration of
class AuthSDUProtectionProfile {
public:
	AuthSDUProtectionProfile() {};
	bool operator==(const AuthSDUProtectionProfile &other) const;
	bool operator!=(const AuthSDUProtectionProfile &other) const;
	static void from_c_auth_profile(AuthSDUProtectionProfile & as,
					struct auth_sdup_profile * asp);
	struct auth_sdup_profile * to_c_auth_profile(void) const;

	std::string to_string();

	///The authentication-encryption-compression policy set
	PolicyConfig authPolicy;

	/// The encryption policy configuration (name/version)
	PolicyConfig encryptPolicy;

	/// The CRC policy config
	PolicyConfig crcPolicy;

	/// The TTL policy config
	PolicyConfig ttlPolicy;
};

/// Configuration of the Security Manager
class SecurityManagerConfiguration {
public:
	SecurityManagerConfiguration() {};
	bool operator==(const SecurityManagerConfiguration &other) const;
	bool operator!=(const SecurityManagerConfiguration &other) const;
	static void from_c_secman_config(SecurityManagerConfiguration &s,
					 struct secman_config * sc);
	struct secman_config * to_c_secman_config(void) const;

	std::string toString();

	PolicyConfig policy_set_;
	AuthSDUProtectionProfile default_auth_profile;
	std::map<std::string, AuthSDUProtectionProfile> specific_auth_profiles;
};

class RoutingConfiguration {
public:
	RoutingConfiguration() {};
	bool operator==(const RoutingConfiguration &other) const;
	bool operator!=(const RoutingConfiguration &other) const;
	static void from_c_routing_config(RoutingConfiguration & r,
					  struct routing_config* rc);
	struct routing_config * to_c_routing_config(void) const;

	std::string toString();

	PolicyConfig policy_set_;
};

/// Contains the data about a DIF Configuration
/// (QoS cubes, policies, parameters, etc)
class DIFConfiguration {
public:
	DIFConfiguration();
	bool operator==(const DIFConfiguration &other) const;
	bool operator!=(const DIFConfiguration &other) const;
	static void from_c_dif_config(DIFConfiguration & d,
				      struct dif_config * dc);
	struct dif_config * to_c_dif_config() const;

#ifndef SWIG
	unsigned int get_address() const;
	void set_address(unsigned int address);
	const EFCPConfiguration& get_efcp_configuration() const;
	void set_efcp_configuration(const EFCPConfiguration& efcp_configuration);
	const RMTConfiguration& get_rmt_configuration() const;
	void set_rmt_configuration(const RMTConfiguration& rmt_configuration);
	const std::list<PolicyConfig>& get_policies();
	void set_policies(const std::list<PolicyConfig>& policies);
	void add_policy(const PolicyConfig& policy);
	const std::list<PolicyParameter>& get_parameters() const;
	void set_parameters(const std::list<PolicyParameter>& parameters);
	void add_parameter(const PolicyParameter& parameter);
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
	/// Flow Allocator configuration parameters of the DIF
	FlowAllocatorConfiguration fa_configuration_;
	/// Configuration of the enrollment Task
	EnrollmentTaskConfiguration et_configuration_;
	/// Configuration of the NamespaceManager
	NamespaceManagerConfiguration nsm_configuration_;
	/// Configuration of routing
	RoutingConfiguration routing_configuration_;
	/// Configuration of the Resource Allocator
	ResourceAllocatorConfiguration ra_configuration_;
	/// Configuration of the security manager
	SecurityManagerConfiguration sm_configuration_;
	/// Other configuration parameters of the DIF
	std::list<PolicyParameter> parameters_;
};

/// Contains the information about a DIF (name, type, configuration)
class DIFInformation {
public:
	DIFInformation(){};
	DIFInformation(struct dif_config * dc, struct name * name,
		       string_t * type);
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
