/*
 * Common interfaces and classes for encoding/decoding RIB objects
 *
 *    Bernat Gaston <bernat.gaston@i2cat.net>
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

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#define RINA_PREFIX "rinad.encoder"

#include <librina/logs.h>

#include "encoder.h"
#include "encoders/DataTransferConstantsMessage.pb.h"
#include "encoders/DirectoryForwardingTableEntryArrayMessage.pb.h"
#include "encoders/QoSCubeArrayMessage.pb.h"
#include "encoders/WhatevercastNameArrayMessage.pb.h"
#include "encoders/NeighborArrayMessage.pb.h"
#include "encoders/IntType.pb.h"
#include "encoders/QoSSpecification.pb.h"
#include "encoders/FlowMessage.pb.h"
#include "encoders/EnrollmentInformationMessage.pb.h"

namespace rinad {

//Class Encoder Constants
const std::string EncoderConstants::ADDRESS = "address";
const std::string EncoderConstants::APNAME = "applicationprocessname";
const std::string EncoderConstants::CONSTANTS = "constants";
const std::string EncoderConstants::DATA_TRANSFER = "datatransfer";
const std::string EncoderConstants::DAF = "daf";
const std::string EncoderConstants::DIF = "dif";
const std::string EncoderConstants::DIF_REGISTRATIONS = "difregistrations";
const std::string EncoderConstants::DIRECTORY_FORWARDING_TABLE_ENTRIES = "directoryforwardingtableentries";
const std::string EncoderConstants::ENROLLMENT = "enrollment";
const std::string EncoderConstants::FLOWS = "flows";
const std::string EncoderConstants::FLOW_ALLOCATOR = "flowallocator";
const std::string EncoderConstants::IPC = "ipc";
const std::string EncoderConstants::DIFMANAGEMENT = "dif_management";
const std::string EncoderConstants::DAFMANAGEMENT = "daf_management";
const std::string EncoderConstants::NEIGHBORS = "neighbors";
const std::string EncoderConstants::NAMING = "naming";
const std::string EncoderConstants::NMINUSONEFLOWMANAGER = "nminusoneflowmanager";
const std::string EncoderConstants::NMINUSEONEFLOWS = "nminusoneflows";
const std::string EncoderConstants::OPERATIONAL_STATUS = "operationalStatus";
const std::string EncoderConstants::PDU_FORWARDING_TABLE = "pduforwardingtable";
const std::string EncoderConstants::QOS_CUBES = "qoscubes";
const std::string EncoderConstants::RESOURCE_ALLOCATION = "resourceallocation";
const std::string EncoderConstants::ROOT = "root";
const std::string EncoderConstants::SEPARATOR = "/";
const std::string EncoderConstants::SYNONYMS = "synonyms";
const std::string EncoderConstants::WHATEVERCAST_NAMES ="whatevercast_name";
const std::string EncoderConstants::ROUTING = "routing";
const std::string EncoderConstants::FLOWSTATEOBJECTGROUP = "flowstateobjectgroup";
const std::string EncoderConstants::LINKSTATE = "linkstate";
const std::string EncoderConstants::WATCHDOG = "watchdog";

const std::string EncoderConstants::DAF_RIB_OBJECT_CLASS = "daf";
const std::string EncoderConstants::DAF_RIB_OBJECT_NAME = SEPARATOR + DAF;
const std::string EncoderConstants::DIF_RIB_OBJECT_CLASS = "dif";
const std::string EncoderConstants::DIF_RIB_OBJECT_NAME= SEPARATOR + DIF ;
const std::string EncoderConstants::DAF_MANAGEMENT_RIB_OBJECT_CLASS = "daf_management";
const std::string EncoderConstants::DAF_MANAGEMENT_RIB_OBJECT_NAME = SEPARATOR + DAF +
    SEPARATOR + DAFMANAGEMENT;
const std::string EncoderConstants::DIF_MANAGEMENT_RIB_OBJECT_CLASS = "dif_management";
const std::string EncoderConstants::DIF_MANAGEMENT_RIB_OBJECT_NAME = SEPARATOR + DIF +
    SEPARATOR + DIFMANAGEMENT;
const std::string EncoderConstants::RESOURCE_ALLOCATION_RIB_OBJECT_CLASS =  "reource_allocation";
const std::string EncoderConstants::RESOURCE_ALLOCATION_RIB_OBJECT_NAME = SEPARATOR + DIF +
    SEPARATOR + RESOURCE_ALLOCATION;
const std::string EncoderConstants::NMINUSONEFLOWMANAGER_RIB_OBJECT_CLASS = "n_minus_one_flowmanager";
const std::string EncoderConstants::NMINUSONEFLOWMANAGER_RIB_OBJECT_NAME = SEPARATOR + DIF +
    SEPARATOR + RESOURCE_ALLOCATION + SEPARATOR + NMINUSONEFLOWMANAGER;
const std::string EncoderConstants::NAMING_RIB_OBJECT_CLASS = "naming";
const std::string EncoderConstants::NAMING_RIB_OBJECT_NAME = SEPARATOR + DAF +
    SEPARATOR + DAFMANAGEMENT + SEPARATOR + NAMING;
const std::string EncoderConstants::FLOW_ALLOCATOR_RIB_OBJECT_CLASS = "flow_allocator";
const std::string EncoderConstants::FLOW_ALLOCATOR_RIB_OBJECT_NAME = SEPARATOR + DIF + SEPARATOR
    + FLOW_ALLOCATOR;
const std::string EncoderConstants::LINKSTATE_RIB_OBJECT_CLASS = "linkstate";
const std::string EncoderConstants::LINKSTATE_RIB_OBJECT_NAME = SEPARATOR +
    DIF + SEPARATOR + RESOURCE_ALLOCATION + SEPARATOR + PDU_FORWARDING_TABLE + SEPARATOR
    + LINKSTATE;
const std::string EncoderConstants::IPC_RIB_OBJECT_CLASS = "ipc";
const std::string EncoderConstants::IPC_RIB_OBJECT_NAME = SEPARATOR + DIF +
    SEPARATOR + IPC;
const std::string EncoderConstants::DATA_TRANSFER_RIB_OBJECT_CLASS = "data_transfer";
const std::string EncoderConstants::DATA_TRANSFER_RIB_OBJECT_NAME = SEPARATOR + DIF +
    SEPARATOR + IPC + SEPARATOR + DATA_TRANSFER;


const std::string EncoderConstants::OPERATIONAL_STATUS_RIB_OBJECT_NAME = SEPARATOR + DAF +
		SEPARATOR + DAFMANAGEMENT + SEPARATOR + OPERATIONAL_STATUS;
const std::string EncoderConstants::OPERATIONAL_STATUS_RIB_OBJECT_CLASS = "operationstatus";
const std::string EncoderConstants::PDU_FORWARDING_TABLE_RIB_OBJECT_CLASS = "pdu forwarding table";
const std::string EncoderConstants::PDU_FORWARDING_TABLE_RIB_OBJECT_NAME = SEPARATOR + DIF +
		SEPARATOR + RESOURCE_ALLOCATION + SEPARATOR + PDU_FORWARDING_TABLE;
const std::string EncoderConstants::DIF_NAME_WHATEVERCAST_RULE = "any";
const std::string EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_NAME = SEPARATOR +
		DIF + SEPARATOR + DIFMANAGEMENT  + SEPARATOR + DIRECTORY_FORWARDING_TABLE_ENTRIES;
const std::string EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_CLASS = "directoryforwardingtableentry_set";
const std::string EncoderConstants::DFT_ENTRY_RIB_OBJECT_CLASS = "directoryforwardingtableentry";
const std::string EncoderConstants::FLOW_SET_RIB_OBJECT_NAME = SEPARATOR + DIF + SEPARATOR +
    FLOW_ALLOCATOR + SEPARATOR + FLOWS;
const std::string EncoderConstants::FLOW_SET_RIB_OBJECT_CLASS = "flow set";
const std::string EncoderConstants::FLOW_RIB_OBJECT_CLASS = "flow";
const std::string EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_NAME = SEPARATOR + DIF + SEPARATOR + RESOURCE_ALLOCATION
    + SEPARATOR + QOS_CUBES;
const std::string EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_CLASS = "qoscube_set";
const std::string EncoderConstants::QOS_CUBE_RIB_OBJECT_CLASS = "qoscube";
const std::string EncoderConstants::ENROLLMENT_INFO_OBJECT_NAME = SEPARATOR + DAF +
			SEPARATOR + DAFMANAGEMENT + SEPARATOR + ENROLLMENT;
const std::string EncoderConstants::ENROLLMENT_INFO_OBJECT_CLASS = "enrollment_information";
const std::string EncoderConstants::FLOW_STATE_OBJECT_RIB_OBJECT_CLASS = "flowstateobject";
const std::string EncoderConstants::FLOW_STATE_OBJECT_GROUP_RIB_OBJECT_CLASS = "flowstateobject_set";
const std::string EncoderConstants::FLOW_STATE_OBJECT_GROUP_RIB_OBJECT_NAME = SEPARATOR +
		DIF + SEPARATOR + RESOURCE_ALLOCATION + SEPARATOR + PDU_FORWARDING_TABLE + SEPARATOR
		+ LINKSTATE + SEPARATOR + FLOWSTATEOBJECTGROUP;
const std::string EncoderConstants::WATCHDOG_RIB_OBJECT_NAME = SEPARATOR + DIF + SEPARATOR + DIFMANAGEMENT +
		SEPARATOR + WATCHDOG;
const std::string EncoderConstants::WATCHDOG_RIB_OBJECT_CLASS = "watchdog_timer";
const std::string EncoderConstants::ADDRESS_RIB_OBJECT_CLASS = "address";
const std::string EncoderConstants::ADDRESS_RIB_OBJECT_NAME = SEPARATOR + DAF +
		SEPARATOR + DAFMANAGEMENT + SEPARATOR + NAMING + SEPARATOR + ADDRESS;
const std::string EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_CLASS = "datatransfercons";
const std::string EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_NAME = SEPARATOR + DIF +
		SEPARATOR + IPC + SEPARATOR + DATA_TRANSFER + SEPARATOR + CONSTANTS;
const std::string EncoderConstants::WHATEVERCAST_NAME_SET_RIB_OBJECT_NAME = SEPARATOR + DAF +
    SEPARATOR + DAFMANAGEMENT + SEPARATOR + NAMING + SEPARATOR + WHATEVERCAST_NAMES;
const std::string EncoderConstants::WHATEVERCAST_NAME_SET_RIB_OBJECT_CLASS = "whatname_set";
const std::string EncoderConstants::WHATEVERCAST_NAME_RIB_OBJECT_CLASS = "whatname";

namespace helpers{

rina::messages::applicationProcessNamingInfo_t* get_applicationProcessNamingInfo_t(
	const rina::ApplicationProcessNamingInformation &name) {
		rina::messages::applicationProcessNamingInfo_t *gpf_name =
			new rina::messages::applicationProcessNamingInfo_t;
		gpf_name->set_applicationprocessname(name.processName);
		gpf_name->set_applicationprocessinstance(name.processInstance);
		gpf_name->set_applicationentityname(name.entityName);
		gpf_name->set_applicationentityinstance(name.entityInstance);
		return gpf_name;
}

void get_ApplicationProcessNamingInformation(
	const rina::messages::applicationProcessNamingInfo_t &gpf_app,
	rina::ApplicationProcessNamingInformation &app) {
		app.processName = gpf_app.applicationprocessname();
		app.processInstance = gpf_app.applicationprocessinstance();
		app.entityName = gpf_app.applicationentityname();
		app.entityInstance = gpf_app.applicationentityinstance();
}

void get_property_t(rina::messages::property_t* gpb_conf, const rina::PolicyParameter &conf) {
	gpb_conf->set_name(conf.get_name());
	gpb_conf->set_value(conf.get_value());
}

void get_PolicyParameter(const rina::messages::property_t &gpf_conf,
	rina::PolicyParameter &conf) {

	conf.set_name(gpf_conf.name());
	conf.set_value(gpf_conf.value());
}

void get_PolicyConfig(const rina::messages::policyDescriptor_t &gpf_conf,
	rina::PolicyConfig &conf) {
	conf.set_name(gpf_conf.policyname());
	conf.set_version(gpf_conf.version());
	for (int i =0; i < gpf_conf.policyparameters_size(); ++i)	{
		rina::PolicyParameter param;
		get_PolicyParameter(gpf_conf.policyparameters(i), param);
		conf.parameters_.push_back(param);
	}
}

rina::messages::policyDescriptor_t* get_policyDescriptor_t(const rina::PolicyConfig &conf) {
	rina::messages::policyDescriptor_t *gpf_conf = new rina::messages::policyDescriptor_t;

	gpf_conf->set_policyname(conf.get_name());
	gpf_conf->set_policyimplname(conf.get_name());
	gpf_conf->set_version(conf.get_version());
	for (std::list<rina::PolicyParameter>::const_iterator it = conf.get_parameters().begin();
		it != conf.get_parameters().end(); ++it) {
			rina::messages::property_t * pro = gpf_conf->add_policyparameters();
			get_property_t(pro, *it);
	}
}
}//namespace helpers

// CLASS DataTransferConstantsEncoder
void DataTransferConstantsEncoderencode(
	const rina::DataTransferConstants &obj, 
	rina::ser_obj_t& serobj)
{
	rina::messages::dataTransferConstants_t gpb;

	gpb.set_addresslength(obj.address_length_);
	gpb.set_cepidlength(obj.cep_id_length_);
	gpb.set_difintegrity(obj.dif_integrity_);
	gpb.set_lengthlength(obj.length_length_);
	gpb.set_maxpdulifetime(obj.max_pdu_lifetime_);
	gpb.set_maxpdusize(obj.max_pdu_size_);
	gpb.set_portidlength(obj.port_id_length_);
	gpb.set_qosidlength(obj.qos_id_length_);
	gpb.set_sequencenumberlength(obj.sequence_number_length_);

	serobj.size_ = gpb.ByteSize();
	serobj.message_ = new char[serobj.size_];
	gpb.SerializeToArray(serobj.message_, serobj.size_);
}

void DataTransferConstantsEncoder::decode(
	const rina::ser_obj_t &serobj,
	rina::DataTransferConstants &des_obj)
{
	rina::messages::dataTransferConstants_t gpb;

	gpb.ParseFromArray(serobj.message_, serobj.size_);

	des_obj.address_length_ = gpb.addresslength();
	des_obj.cep_id_length_ = gpb.cepidlength();
	des_obj.dif_integrity_ = gpb.difintegrity();
	des_obj.length_length_ = gpb.lengthlength();
	des_obj.max_pdu_lifetime_ = gpb.maxpdulifetime();
	des_obj.max_pdu_size_ = gpb.maxpdusize();
	des_obj.port_id_length_ = gpb.portidlength();
	des_obj.qos_id_length_ = gpb.qosidlength();
	des_obj.sequence_number_length_ = gpb.sequencenumberlength();
}


// CLASS DirectoryForwardingTableEntryEncoder

namespace dft_helpers
{
void toGPB(const rina::DirectoryForwardingTableEntry &obj, 
	rina::messages::directoryForwardingTableEntry_t &gpb)
{
	gpb.set_allocated_applicationname(
		helpers::get_applicationProcessNamingInfo_t(obj.ap_naming_info_));
	gpb.set_ipcprocesssynonym(obj.address_);
	gpb.set_timestamp(obj.timestamp_);
}

void toModel (const rina::messages::directoryForwardingTableEntry_t &gpb,
	rina::DirectoryForwardingTableEntry &des_obj)
{
	rina::ApplicationProcessNamingInformation app_name;
	helpers::get_ApplicationProcessNamingInformation(gpb.applicationname(),
		app_name);
	des_obj.ap_naming_info_ = app_name;
	des_obj.address_ = gpb.ipcprocesssynonym();
	des_obj.timestamp_ = gpb.timestamp();
}
}// namespace dft_helpers

void DFTEEncoder::encode(
	const rina::DirectoryForwardingTableEntry &obj, 
	rina::ser_obj_t& serobj)
{
	rina::messages::directoryForwardingTableEntry_t gpb;

	dft_helpers::toGPB(obj, gpb);

	serobj.size_ = gpb.ByteSize();
	serobj.message_ = new char[serobj.size_];
	gpb.SerializeToArray(serobj.message_, serobj.size_);
}

void DFTEEncoder::decode(
	const rina::ser_obj_t &serobj, 
	rina::DirectoryForwardingTableEntry &des_obj)
{
	rina::messages::directoryForwardingTableEntry_t gpb;
	gpb.ParseFromArray(serobj.message_, serobj.size_);
	dft_helpers::toModel(gpb, des_obj);
}

// CLASS DFTEListEncoder
void DFTEListEncoder::encode(
	const std::list<rina::DirectoryForwardingTableEntry> &obj,
	rina::ser_obj_t& serobj)
{
	rina::messages::directoryForwardingTableEntrySet_t gpb;

	for (std::list<rina::DirectoryForwardingTableEntry>::const_iterator 
		it = obj.begin(); it != obj.end(); ++it) {
			rina::messages::directoryForwardingTableEntry_t *gpb_dft;
			gpb_dft = gpb.add_directoryforwardingtableentry();
			dft_helpers::toGPB((*it), *gpb_dft);
	}

	serobj.size_ = gpb.ByteSize();
	serobj.message_ = new char[serobj.size_];
	gpb.SerializeToArray(serobj.message_, serobj.size_);
}
void DFTEListEncoder::decode(const rina::ser_obj_t &serobj, 
	std::list<rina::DirectoryForwardingTableEntry> &des_obj)
{
	rina::messages::directoryForwardingTableEntrySet_t gpb;
	gpb.ParseFromArray(serobj.message_, serobj.size_);

	for (int i = 0; i < gpb.directoryforwardingtableentry_size(); i++)
	{
		rina::DirectoryForwardingTableEntry dfte;
		dft_helpers::toModel(gpb.directoryforwardingtableentry(i), dfte);
		des_obj.push_back(dfte);
	}
}

// CLASS QoSCubeEncoder
namespace cube_helpers
{
rina::messages::dtpConfig_t* get_dtpConfig_t(const rina::DTPConfig &conf) 
{
	rina::messages::dtpConfig_t *gpf_conf = new rina::messages::dtpConfig_t;
	gpf_conf->set_dtcppresent(conf.is_dtcp_present());
	gpf_conf->set_allocated_initialseqnumpolicy(
		helpers::get_policyDescriptor_t(conf.get_initial_seq_num_policy()));
	gpf_conf->set_seqnumrolloverthreshold(conf.get_seq_num_rollover_threshold());
	gpf_conf->set_initialatimer(conf.get_initial_a_timer());
	gpf_conf->set_allocated_rcvrtimerinactivitypolicy(
		helpers::get_policyDescriptor_t(
		conf.get_rcvr_timer_inactivity_policy()));
	gpf_conf->set_allocated_sendertimerinactiviypolicy(
		helpers::get_policyDescriptor_t(conf.get_sender_timer_inactivity_policy()));
	gpf_conf->set_allocated_dtppolicyset(
		helpers::get_policyDescriptor_t(conf.get_dtp_policy_set()));

	return gpf_conf;
}

void get_DTPConfig(const rina::messages::dtpConfig_t &gpf_conf, 
	rina::DTPConfig &conf) 
{
	conf.set_dtcp_present(gpf_conf.dtcppresent());
	rina::PolicyConfig p_conf;
	helpers::get_PolicyConfig(gpf_conf.rcvrtimerinactivitypolicy(), p_conf);
	conf.set_rcvr_timer_inactivity_policy(p_conf);
	helpers::get_PolicyConfig(gpf_conf.sendertimerinactiviypolicy(), p_conf);
	conf.set_sender_timer_inactivity_policy(p_conf);
	helpers::get_PolicyConfig(gpf_conf.initialseqnumpolicy(), p_conf);
	conf.set_initial_seq_num_policy(p_conf);
	helpers::get_PolicyConfig(gpf_conf.dtppolicyset(), p_conf);
	conf.set_dtp_policy_set(p_conf);
	conf.set_seq_num_rollover_threshold(
		gpf_conf.seqnumrolloverthreshold());
	conf.set_initial_a_timer(gpf_conf.initialatimer());
}


rina::messages::dtcpWindowBasedFlowControlConfig_t* 
	get_dtcpWindowBasedFlowControlConfig_t(
	const rina::DTCPWindowBasedFlowControlConfig &conf) 
{
		rina::messages::dtcpWindowBasedFlowControlConfig_t * gpf_conf = 
			new rina::messages::dtcpWindowBasedFlowControlConfig_t;

		gpf_conf->set_maxclosedwindowqueuelength(
			conf.get_maxclosed_window_queue_length());
		gpf_conf->set_initialcredit(conf.get_initial_credit());
		gpf_conf->set_allocated_rcvrflowcontrolpolicy(
			helpers::get_policyDescriptor_t(
			conf.get_rcvr_flow_control_policy()));
		gpf_conf->set_allocated_txcontrolpolicy(
			helpers::get_policyDescriptor_t(
			conf.getTxControlPolicy()));

		return gpf_conf;
}

rina::messages::dtcpRateBasedFlowControlConfig_t* 
	get_dtcpRateBasedFlowControlConfig_t(
	const rina::DTCPRateBasedFlowControlConfig &conf) {
		rina::messages::dtcpRateBasedFlowControlConfig_t *gpf_conf = 
			new rina::messages::dtcpRateBasedFlowControlConfig_t;

		gpf_conf->set_sendingrate(conf.get_sending_rate());
		gpf_conf->set_timeperiod(conf.get_time_period());
		gpf_conf->set_allocated_norateslowdownpolicy(
			helpers::get_policyDescriptor_t(conf.get_no_rate_slow_down_policy()));
		gpf_conf->set_allocated_nooverridedefaultpeakpolicy(
			helpers::get_policyDescriptor_t(
			conf.get_no_override_default_peak_policy()));
		gpf_conf->set_allocated_ratereductionpolicy(helpers::get_policyDescriptor_t(
			conf.get_rate_reduction_policy()));

		return gpf_conf;
}

rina::messages::dtcpFlowControlConfig_t* 
	get_dtcpFlowControlConfig_t(const rina::DTCPFlowControlConfig &conf)
{
	rina::messages::dtcpFlowControlConfig_t *gpf_conf = 
		new rina::messages::dtcpFlowControlConfig_t ;

	gpf_conf->set_windowbased(conf.is_window_based());
	gpf_conf->set_allocated_windowbasedconfig(
		get_dtcpWindowBasedFlowControlConfig_t(
		conf.get_window_based_config()));
	gpf_conf->set_ratebased(conf.is_rate_based());
	gpf_conf->set_allocated_ratebasedconfig(
		get_dtcpRateBasedFlowControlConfig_t(conf.get_rate_based_config()));
	gpf_conf->set_sentbytesthreshold(conf.get_sent_bytes_threshold());
	gpf_conf->set_sentbytespercentthreshold(
		conf.get_sent_bytes_percent_threshold());
	gpf_conf->set_sentbuffersthreshold(conf.get_sent_buffers_threshold());
	gpf_conf->set_rcvbytesthreshold(conf.get_rcv_bytes_threshold());
	gpf_conf->set_rcvbytespercentthreshold(
		conf.get_rcv_bytes_percent_threshold());
	gpf_conf->set_rcvbuffersthreshold(conf.get_rcv_buffers_threshold());
	gpf_conf->set_allocated_closedwindowpolicy(helpers::get_policyDescriptor_t(
		conf.get_closed_window_policy()));
	gpf_conf->set_allocated_flowcontroloverrunpolicy(
		helpers::get_policyDescriptor_t(conf.get_flow_control_overrun_policy()));
	gpf_conf->set_allocated_reconcileflowcontrolpolicy(
		helpers::get_policyDescriptor_t(
		conf.get_reconcile_flow_control_policy()));
	gpf_conf->set_allocated_receivingflowcontrolpolicy(
		helpers::get_policyDescriptor_t(
		conf.get_receiving_flow_control_policy()));

	return gpf_conf;
}

rina::messages::dtcpRtxControlConfig_t* get_dtcpRtxControlConfig_t(
	const rina::DTCPRtxControlConfig &conf) {
	rina::messages::dtcpRtxControlConfig_t *gpf_conf = 
		new rina::messages::dtcpRtxControlConfig_t;

	gpf_conf->set_maxtimetoretry(conf.get_max_time_to_retry());
	gpf_conf->set_datarxmsnmax(conf.get_data_rxmsn_max());
	gpf_conf->set_allocated_rtxtimerexpirypolicy(
		helpers::get_policyDescriptor_t(conf.get_rtx_timer_expiry_policy()));
	gpf_conf->set_allocated_senderackpolicy(helpers::get_policyDescriptor_t(
		conf.get_sender_ack_policy()));
	gpf_conf->set_allocated_recvingacklistpolicy(helpers::get_policyDescriptor_t(
		conf.get_recving_ack_list_policy()));
	gpf_conf->set_allocated_rcvrackpolicy(helpers::get_policyDescriptor_t(
		conf.get_rcvr_ack_policy()));
	gpf_conf->set_allocated_sendingackpolicy(helpers::get_policyDescriptor_t(
		conf.get_sending_ack_policy()));
	gpf_conf->set_allocated_rcvrcontrolackpolicy(
		helpers::get_policyDescriptor_t(conf.get_rcvr_control_ack_policy()));
	gpf_conf->set_initialrtxtime(conf.get_initial_rtx_time());

	return gpf_conf;
}

rina::messages::dtcpConfig_t* get_dtcpConfig_t(
	const rina::DTCPConfig &conf) {
	rina::messages::dtcpConfig_t *gpf_conf = new rina::messages::dtcpConfig_t;

	gpf_conf->set_flowcontrol(conf.is_flow_control());
	gpf_conf->set_allocated_flowcontrolconfig(get_dtcpFlowControlConfig_t(
		conf.get_flow_control_config()));
	gpf_conf->set_rtxcontrol(conf.is_rtx_control());
	gpf_conf->set_allocated_rtxcontrolconfig(get_dtcpRtxControlConfig_t(
		conf.get_rtx_control_config()));
	gpf_conf->set_allocated_lostcontrolpdupolicy(
		helpers::get_policyDescriptor_t((conf.get_lost_control_pdu_policy())));
	gpf_conf->set_allocated_rttestimatorpolicy(
		helpers::get_policyDescriptor_t((conf.get_rtt_estimator_policy())));
	gpf_conf->set_allocated_dtcppolicyset(
		helpers::get_policyDescriptor_t((conf.get_dtcp_policy_set())));

	return gpf_conf;
}

void get_DTCPWindowBasedFlowControlConfig(
	const rina::messages::dtcpWindowBasedFlowControlConfig_t &gpf_conf,
	rina::DTCPWindowBasedFlowControlConfig &conf) {

	conf.set_max_closed_window_queue_length(
		gpf_conf.maxclosedwindowqueuelength());
	conf.set_initial_credit(gpf_conf.initialcredit());
	rina::PolicyConfig poli;
	helpers::get_PolicyConfig(gpf_conf.rcvrflowcontrolpolicy(), poli);
	conf.set_rcvr_flow_control_policy(poli);

	helpers::get_PolicyConfig(gpf_conf.txcontrolpolicy(), poli);
	conf.set_tx_control_policy(poli);
}

void get_DTCPRateBasedFlowControlConfig(
	const rina::messages::dtcpRateBasedFlowControlConfig_t &gpf_conf,
	rina::DTCPRateBasedFlowControlConfig &conf) {
		rina::PolicyConfig poli;

		conf.set_sending_rate(gpf_conf.sendingrate());
		conf.set_time_period(gpf_conf.timeperiod());
		helpers::get_PolicyConfig(gpf_conf.norateslowdownpolicy(), poli);
		conf.set_no_rate_slow_down_policy(poli);

		helpers::get_PolicyConfig(gpf_conf.nooverridedefaultpeakpolicy(), poli);
		conf.set_no_override_default_peak_policy(poli);

		helpers::get_PolicyConfig(gpf_conf.ratereductionpolicy(), poli);
		conf.set_rate_reduction_policy(poli);
}

void get_DTCPFlowControlConfig(
	const rina::messages::dtcpFlowControlConfig_t &gpf_conf, 
	rina::DTCPFlowControlConfig &conf) 
{
	conf.set_window_based(gpf_conf.windowbased());
	rina::DTCPWindowBasedFlowControlConfig window;
	get_DTCPWindowBasedFlowControlConfig(gpf_conf.windowbasedconfig(),window);
	conf.set_window_based_config(window);

	conf.set_rate_based(gpf_conf.ratebased());

	rina::DTCPRateBasedFlowControlConfig rate;
	get_DTCPRateBasedFlowControlConfig(gpf_conf.ratebasedconfig(), rate);
	conf.set_rate_based_config(rate);

	conf.set_sent_bytes_threshold(gpf_conf.sentbytesthreshold());
	conf.set_sent_bytes_percent_threshold(gpf_conf.sentbytespercentthreshold());
	conf.set_sent_buffers_threshold(gpf_conf.sentbuffersthreshold());
	conf.set_rcv_bytes_threshold(gpf_conf.rcvbytesthreshold());
	conf.set_rcv_bytes_percent_threshold(gpf_conf.rcvbytespercentthreshold());
	conf.set_rcv_buffers_threshold(gpf_conf.rcvbuffersthreshold());

	rina::PolicyConfig poli;
	helpers::get_PolicyConfig(gpf_conf.closedwindowpolicy(), poli);
	conf.set_closed_window_policy(poli);

	helpers::get_PolicyConfig(gpf_conf.flowcontroloverrunpolicy(), poli);
	conf.set_flow_control_overrun_policy(poli);

	helpers::get_PolicyConfig(gpf_conf.reconcileflowcontrolpolicy(), poli);
	conf.set_reconcile_flow_control_policy(poli);

	helpers::get_PolicyConfig(gpf_conf.receivingflowcontrolpolicy(), poli);
	conf.set_receiving_flow_control_policy(poli);
}

void get_DTCPRtxControlConfig(
	const rina::messages::dtcpRtxControlConfig_t &gpf_conf, 
	rina::DTCPRtxControlConfig &conf) 
{
	rina::PolicyConfig polc;

	conf.set_max_time_to_retry(gpf_conf.maxtimetoretry());
	conf.set_data_rxmsn_max(gpf_conf.datarxmsnmax());
	helpers::get_PolicyConfig(gpf_conf.rtxtimerexpirypolicy(), polc);
	conf.set_rtx_timer_expiry_policy(polc);
	helpers::get_PolicyConfig(gpf_conf.senderackpolicy(), polc);
	conf.set_sender_ack_policy(polc);
	helpers::get_PolicyConfig(gpf_conf.recvingacklistpolicy(), polc);
	conf.set_recving_ack_list_policy(polc);
	helpers::get_PolicyConfig(gpf_conf.rcvrackpolicy(), polc);
	conf.set_rcvr_ack_policy(polc);
	helpers::get_PolicyConfig(gpf_conf.sendingackpolicy(), polc);
	conf.set_sending_ack_policy(polc);
	helpers::get_PolicyConfig(gpf_conf.rcvrcontrolackpolicy(), polc);
	conf.set_rcvr_control_ack_policy(polc);
	conf.set_initial_rtx_time(gpf_conf.initialrtxtime());
}

rina::DTCPConfig* get_DTCPConfig(
	const rina::messages::dtcpConfig_t &gpf_conf, rina::DTCPConfig &conf)
{
	conf.flow_control_ = gpf_conf.flowcontrol();
	rina::DTCPFlowControlConfig flow_conf;
	get_DTCPFlowControlConfig(gpf_conf.flowcontrolconfig(), flow_conf);
	conf.set_flow_control_config(flow_conf);
	rina::DTCPRtxControlConfig rtx_conf;
	get_DTCPRtxControlConfig(gpf_conf.rtxcontrolconfig(), rtx_conf);
	conf.set_rtx_control_config(rtx_conf);
	rina::PolicyConfig p_conf;
	helpers::get_PolicyConfig(gpf_conf.lostcontrolpdupolicy(), p_conf);
	conf.set_lost_control_pdu_policy(p_conf);
	helpers::get_PolicyConfig(gpf_conf.rttestimatorpolicy(), p_conf);
	conf.set_rtt_estimator_policy(p_conf);
	helpers::get_PolicyConfig(gpf_conf.dtcppolicyset(), p_conf);
	conf.set_dtcp_policy_set(p_conf);
}

void toGPB(const rina::QoSCube &obj, rina::messages::qosCube_t &gpb)
{
	gpb.set_allocated_dtpconfiguration(
		cube_helpers::get_dtpConfig_t(obj.dtp_config_));
	gpb.set_allocated_dtcpconfiguration(
		cube_helpers::get_dtcpConfig_t(obj.dtcp_config_));
	gpb.set_averagebandwidth(obj.average_bandwidth_);
	gpb.set_averagesdubandwidth(obj.average_sdu_bandwidth_);
	gpb.set_delay(obj.delay_);
	gpb.set_jitter(obj.jitter_);
	gpb.set_maxallowablegapsdu(obj.max_allowable_gap_);
	gpb.set_name(obj.name_);
	gpb.set_order(obj.ordered_delivery_);
	gpb.set_partialdelivery(obj.partial_delivery_);
	gpb.set_peakbandwidthduration(obj.peak_bandwidth_duration_);
	gpb.set_peaksdubandwidthduration(obj.peak_sdu_bandwidth_duration_);
	gpb.set_qosid(obj.id_);
	gpb.set_undetectedbiterrorrate(obj.undetected_bit_error_rate_);
}

void toModel (const rina::messages::qosCube_t &gpb, rina::QoSCube &des_obj)
{
	rina::DTPConfig dtpConfig;
	get_DTPConfig(gpb.dtpconfiguration(), dtpConfig);
	des_obj.dtp_config_ = dtpConfig;
	rina::DTCPConfig dtcpConfig;
	get_DTCPConfig(gpb.dtcpconfiguration(), dtcpConfig);
	des_obj.dtcp_config_ = dtcpConfig;
	des_obj.average_bandwidth_ = gpb.averagebandwidth();
	des_obj.average_sdu_bandwidth_ = gpb.averagesdubandwidth();
	des_obj.delay_ = gpb.delay();
	des_obj.jitter_ = gpb.jitter();
	des_obj.max_allowable_gap_ = gpb.maxallowablegapsdu();
	des_obj.name_ = gpb.name();
	des_obj.ordered_delivery_ = gpb.order();
	des_obj.partial_delivery_ = gpb.partialdelivery();
	des_obj.peak_bandwidth_duration_ = gpb.peakbandwidthduration();
	des_obj.peak_sdu_bandwidth_duration_ = gpb.peaksdubandwidthduration();
	des_obj.id_ = gpb.qosid();
	des_obj.undetected_bit_error_rate_ = gpb.undetectedbiterrorrate();
}
} // namespace cube_helpers

void QoSCubeEncoder::encode(const rina::QoSCube &obj, 
	rina::ser_obj_t &serobj)
{
	rina::messages::qosCube_t gpb;

	cube_helpers::toGPB(obj, gpb);

	serobj.size_ = gpb.ByteSize();
	serobj.message_ = new char[serobj.size_];
	gpb.SerializeToArray(serobj.message_, serobj.size_);
}

void QoSCubeEncoder::decode(const rina::ser_obj_t &serobj, 
	rina::QoSCube &des_obj)

{
	rina::messages::qosCube_t gpb;
	gpb.ParseFromArray(serobj.message_, serobj.size_);

	cube_helpers::toModel(gpb, des_obj);
}

/// CLASS QoSCubeListEncoder
void QoSCubeListEncoder::encode(const std::list<rina::QoSCube> &obj, 
	rina::ser_obj_t& serobj)
{
	rina::messages::qosCubes_t gpb;

	for (std::list<rina::QoSCube>::const_iterator it = obj.begin(); 
		it != obj.end(); ++it) {
		rina::messages::qosCube_t *gpb_cube;
		gpb_cube = gpb.add_qoscube();
		cube_helpers::toGPB((*it), *gpb_cube);
	}

	serobj.size_ = gpb.ByteSize();
	serobj.message_ = new char[serobj.size_];
	gpb.SerializeToArray(serobj.message_, serobj.size_);

}
void QoSCubeListEncoder::decode(const rina::ser_obj_t &serobj, 
	std::list<rina::QoSCube> &des_obj)
{
	rina::messages::qosCubes_t gpb;
	gpb.ParseFromArray(serobj.message_, serobj.size_);

	for (int i = 0; i < gpb.qoscube_size(); i++)
	{
		rina::QoSCube cube;
		cube_helpers::toModel(gpb.qoscube(i), cube);
		des_obj.push_back(cube);
	}
}

//Class WhatevercastNameEncoder
namespace whatever_helpers{
void toGPB(const rina::WhatevercastName &obj, 
	rina::messages::whatevercastName_t &gpb)
{
	gpb.set_name(obj.name_);
	gpb.set_rule(obj.rule_);
	for (std::list<std::string>::const_iterator it=obj.set_members_.begin();
		it != obj.set_members_.end(); ++it) 
	{
		gpb.add_setmembers(*it);
	}
}

void toModel(const rina::messages::whatevercastName_t &gpb, 
	rina::WhatevercastName &des_obj)
{
	des_obj.name_ = gpb.name();
	des_obj.rule_ = gpb.rule();
	for(int i=0; i<gpb.setmembers_size(); i++) {
		des_obj.set_members_.push_back(gpb.setmembers(i));
	}
}
} // namespace whatever_helpers

void WhatevercastNameEncoder::encode(const rina::WhatevercastName &obj,
	rina::ser_obj_t& serobj)
{
	rina::messages::whatevercastName_t gpb;

	whatever_helpers::toGPB(obj, gpb);

	serobj.size_ = gpb.ByteSize();
	serobj.message_ = new char[serobj.size_];
	gpb.SerializeToArray(serobj.message_, serobj.size_);
}

void WhatevercastNameEncoder::decode(const rina::ser_obj_t &serobj,
	rina::WhatevercastName &des_obj)
{
	rina::messages::whatevercastName_t gpb;
	gpb.ParseFromArray(serobj.message_, serobj.size_);

	whatever_helpers::toModel(gpb, des_obj);
}

/// Class WhatevercastNameListEncoder
void WhatevercastNameListEncoder::encode(
	const std::list<rina::WhatevercastName> &obj,
	rina::ser_obj_t& serobj)
{
	rina::messages::whatevercastNames_t gpb;

	for (std::list<rina::WhatevercastName>::const_iterator it = obj.begin();
		it != obj.end(); ++it) {
			rina::messages::whatevercastName_t *gpb_name;
			gpb_name = gpb.add_whatevercastname();
			whatever_helpers::toGPB((*it), *gpb_name);
	}

	serobj.size_ = gpb.ByteSize();
	serobj.message_ = new char[serobj.size_];
	gpb.SerializeToArray(serobj.message_, serobj.size_);
}
void WhatevercastNameListEncoder::decode(const rina::ser_obj_t &serobj, 
	std::list<rina::WhatevercastName> &des_obj)
{
	rina::messages::whatevercastNames_t gpb;
	gpb.ParseFromArray(serobj.message_, serobj.size_);

	for (int i = 0; i < gpb.whatevercastname_size(); i++)
	{
		rina::WhatevercastName name;
		whatever_helpers::toModel(gpb.whatevercastname(i), name);
		des_obj.push_back(name);
	}
}

namespace neighbor_helpers{
void toGPB(const rina::Neighbor &obj, rina::messages::neighbor_t &gpb)
{
	gpb.set_address(obj.address_);
	gpb.set_applicationprocessname(obj.name_.processName);
	gpb.set_applicationprocessinstance(obj.name_.processInstance);
	for (std::list<rina::ApplicationProcessNamingInformation>::const_iterator
		it = obj.supporting_difs_.begin(); it != obj.supporting_difs_.end();
		++it)
	{
		gpb.add_supportingdifs(it->processName);
	}
}
void toModel(const rina::messages::neighbor_t &gpb, 
	rina::Neighbor &des_obj)
{
	des_obj.address_ = gpb.address();
	des_obj.name_.processName = gpb.applicationprocessname();
	des_obj.name_.processInstance = gpb.applicationprocessinstance();
	for(int i=0; i<gpb.supportingdifs_size(); i++) {
		des_obj.supporting_difs_.push_back(
			rina::ApplicationProcessNamingInformation(gpb.supportingdifs(i),
			""));
	}
}
} // namespace neighbor_helpers
//Class NeighborEncoder
void NeighborEncoder::encode(const rina::Neighbor &obj, 
	rina::ser_obj_t& serobj)
{
	rina::messages::neighbor_t gpb;

	neighbor_helpers::toGPB(obj, gpb);

	serobj.size_ = gpb.ByteSize();
	serobj.message_ = new char[serobj.size_];
	gpb.SerializeToArray(serobj.message_, serobj.size_);
}

void NeighborEncoder::decode(const rina::ser_obj_t &serobj, 
	rina::Neighbor &des_obj)
{
	rina::messages::neighbor_t gpb;
	gpb.ParseFromArray(serobj.message_, serobj.size_);
	neighbor_helpers::toModel(gpb, des_obj);
}


/// CLASS NeighborListEncoder
void NeighborListEncoder::encode(
	const std::list<rina::Neighbor> &obj,
	rina::ser_obj_t& serobj)
{
	rina::messages::neighbors_t gpb;

	for (std::list<rina::Neighbor>::const_iterator it = obj.begin();
		it != obj.end(); ++it) {
			rina::messages::neighbor_t *gpb_neigh;
			gpb_neigh = gpb.add_neighbor();
			neighbor_helpers::toGPB((*it), *gpb_neigh);
	}

	serobj.size_ = gpb.ByteSize();
	serobj.message_ = new char[serobj.size_];
	gpb.SerializeToArray(serobj.message_, serobj.size_);
}
void NeighborListEncoder::decode(const rina::ser_obj_t &serobj, 
	std::list<rina::Neighbor> &des_obj)
{
	rina::messages::neighbors_t gpb;
	gpb.ParseFromArray(serobj.message_, serobj.size_);

	for (int i = 0; i < gpb.neighbor_size(); i++)
	{
		rina::Neighbor neigh;
		neighbor_helpers::toModel(gpb.neighbor(i), neigh);
		des_obj.push_back(neigh);
	}
}

// CLASS ADataObjectEncoder
void ADataObjectEncoder::encode(const rina::ADataObject &obj, 
	rina::ser_obj_t& serobj)
{
	rina::messages::a_data_t gpb;

	gpb.set_sourceaddress(obj.source_address_);
	gpb.set_destaddress(obj.dest_address_);
	gpb.set_cdapmessage(obj.encoded_cdap_message_->message_,
		obj.encoded_cdap_message_->size_);

	serobj.size_ = gpb.ByteSize();
	serobj.message_ = new char[serobj.size_];
	gpb.SerializeToArray(serobj.message_, serobj.size_);
}

void ADataObjectEncoder::decode(const rina::ser_obj_t &serobj, 
	rina::ADataObject &des_obj)
{
	rina::messages::a_data_t gpb;
	gpb.ParseFromArray(serobj.message_, serobj.size_);

	des_obj.source_address_ = gpb.sourceaddress();
	des_obj.dest_address_ = gpb.destaddress();
	
	rina::ser_obj_t* cdap;
	cdap->size_ = gpb.cdapmessage().size();
	cdap->message_ = new char[cdap->size_];
	memcpy(cdap->message_, gpb.cdapmessage().data(), 
		gpb.cdapmessage().size());
	des_obj.encoded_cdap_message_ = cdap;
}

// Class EnrollmentInformationRequestEncoder
void EnrollmentInformationRequestEncoder::encode(
	const EnrollmentInformationRequest &obj, rina::ser_obj_t& serobj)
{
	rina::messages::enrollmentInformation_t gpb;

	gpb.set_address(obj.address_);
	gpb.set_startearly(obj.allowed_to_start_early_);

	for(std::list<rina::ApplicationProcessNamingInformation>::const_iterator 
		it = obj.supporting_difs_.begin(); it != obj.supporting_difs_.end(); ++it) 
	{
		gpb.add_supportingdifs(it->processName);
	}

	serobj.size_ = gpb.ByteSize();
	serobj.message_ = new char[serobj.size_];
	gpb.SerializeToArray(serobj.message_, serobj.size_);
}

void decode(const rina::ser_obj_t &serobj, EnrollmentInformationRequest &des_obj)
{
	rina::messages::enrollmentInformation_t gpb;
	gpb.ParseFromArray(serobj.message_, serobj.size_);

	des_obj.address_ = gpb.address();
	//FIXME that should read gpb_eir.startearly() but always returns false
	des_obj.allowed_to_start_early_ = true;

	for (int i = 0; i < gpb.supportingdifs_size(); ++i) {
		des_obj.supporting_difs_.push_back(rina::ApplicationProcessNamingInformation(
			gpb.supportingdifs(i), ""));
	}
}

// CLASS FlowEncoder
namespace flow_helpers
{
rina::messages::qosSpecification_t* get_qosSpecification_t(
	const rina::FlowSpecification &flow_spec) {
	rina::messages::qosSpecification_t *gpf_flow_spec =
		new rina::messages::qosSpecification_t;

	gpf_flow_spec->set_averagebandwidth(flow_spec.averageBandwidth);
	gpf_flow_spec->set_averagesdubandwidth(flow_spec.averageSDUBandwidth);
	gpf_flow_spec->set_peakbandwidthduration(
		flow_spec.peakBandwidthDuration);
	gpf_flow_spec->set_peaksdubandwidthduration(
		flow_spec.peakSDUBandwidthDuration);
	gpf_flow_spec->set_undetectedbiterrorrate(
		flow_spec.undetectedBitErrorRate);
	gpf_flow_spec->set_partialdelivery(flow_spec.partialDelivery);
	gpf_flow_spec->set_order(flow_spec.orderedDelivery);
	gpf_flow_spec->set_maxallowablegapsdu(flow_spec.maxAllowableGap);
	gpf_flow_spec->set_delay(flow_spec.delay);
	gpf_flow_spec->set_jitter(flow_spec.jitter);

	return gpf_flow_spec;
}

void get_Connection(const rina::messages::connectionId_t &gpf_conn,
	rina::Connection &conn) 
{
	conn.setQosId(gpf_conn.qosid());
	conn.setSourceCepId(gpf_conn.sourcecepid());
	conn.setDestCepId(gpf_conn.destinationcepid());
}

void get_FlowSpecification(const rina::messages::qosSpecification_t &gpf_qos,
	rina::FlowSpecification &qos)
{
	qos.averageBandwidth = gpf_qos.averagebandwidth();
	qos.averageSDUBandwidth = gpf_qos.averagesdubandwidth();
	qos.peakBandwidthDuration = gpf_qos.peakbandwidthduration();
	qos.peakSDUBandwidthDuration = gpf_qos.peaksdubandwidthduration();
	qos.undetectedBitErrorRate = gpf_qos.undetectedbiterrorrate();
	qos.partialDelivery = gpf_qos.partialdelivery();
	qos.orderedDelivery = gpf_qos.partialdelivery();
	qos.maxAllowableGap = gpf_qos.maxallowablegapsdu();
	qos.delay = gpf_qos.delay();
	qos.jitter = gpf_qos.jitter();
}

}// namespace flow_enc_helpers

void FlowEncoder::encode(const Flow &obj, rina::ser_obj_t& serobj)
{
	rina::messages::Flow gpb;

	// SourceNamingInfo
	gpb.set_allocated_sourcenaminginfo(
		helpers::get_applicationProcessNamingInfo_t(
		obj.source_naming_info));
	// DestinationNamingInfo
	gpb.set_allocated_destinationnaminginfo(
		helpers::get_applicationProcessNamingInfo_t(
		obj.destination_naming_info));
	// sourcePortId
	gpb.set_sourceportid(obj.source_port_id);
	//destinationPortId
	gpb.set_destinationportid(obj.destination_port_id);
	//sourceAddress
	gpb.set_sourceaddress(obj.source_address);
	//destinationAddress
	gpb.set_destinationaddress(obj.destination_address);
	//connectionIds
	for (std::list<rina::Connection*>::const_iterator it = 
		obj.connections.begin(); it != obj.connections.end(); ++it) 
	{
			rina::messages::connectionId_t *gpf_connection = gpb
				.add_connectionids();
			//qosId
			gpf_connection->set_qosid((*it)->getQosId());
			//sourceCEPId
			gpf_connection->set_sourcecepid((*it)->getSourceCepId());
			//destinationCEPId
			gpf_connection->set_destinationcepid((*it)->getDestCepId());
	}
	//currentConnectionIdIndex
	gpb.set_currentconnectionidindex(
		obj.current_connection_index);
	//state
	gpb.set_state(obj.state);
	//qosParameters
	gpb.set_allocated_qosparameters(
		flow_helpers::get_qosSpecification_t(obj.flow_specification));
	//optional dtpConfig_t dtpConfig
	gpb.set_allocated_dtpconfig(
		cube_helpers::get_dtpConfig_t(
		obj.getActiveConnection()->getDTPConfig()));
	//optional dtpConfig_t dtpConfig
	gpb.set_allocated_dtcpconfig(
		cube_helpers::get_dtcpConfig_t(
		obj.getActiveConnection()->getDTCPConfig()));
	//accessControl
	if (obj.access_control != 0)
		gpb.set_accesscontrol(obj.access_control);
	//maxCreateFlowRetries
	gpb.set_maxcreateflowretries(obj.max_create_flow_retries);
	//createFlowRetries
	gpb.set_createflowretries(obj.create_flow_retries);
	//hopCount
	gpb.set_hopcount(obj.hop_count);

	serobj.size_ = gpb.ByteSize();
	serobj.message_ = new char[serobj.size_];
	gpb.SerializeToArray(serobj.message_, serobj.size_);
}

void FlowEncoder::decode(const rina::ser_obj_t &serobj, Flow &des_obj)
{
	rina::messages::Flow gpb;

	gpb.ParseFromArray(serobj.message_, serobj.size_);

	rina::ApplicationProcessNamingInformation src_app;
	helpers::get_ApplicationProcessNamingInformation(
		gpb.sourcenaminginfo(), src_app);
	des_obj.source_naming_info = src_app;

	rina::ApplicationProcessNamingInformation dest_app;
	helpers::get_ApplicationProcessNamingInformation(
		gpb.destinationnaminginfo(), dest_app);
	des_obj.destination_naming_info = dest_app;
	
	des_obj.source_port_id = gpb.sourceportid();
	des_obj.destination_port_id = gpb.destinationportid();
	des_obj.source_address = gpb.sourceaddress();
	des_obj.destination_address = gpb.destinationaddress();

	for (int i = 0; i < gpb.connectionids_size(); ++i) {
		rina::Connection* connection = new rina::Connection;
		flow_helpers::get_Connection(gpb.connectionids(i), *connection);
		connection->sourceAddress = gpb.sourceaddress();
		connection->destAddress = gpb.destinationaddress();
		des_obj.connections.push_back(connection);
	}
	des_obj.current_connection_index =
		gpb.currentconnectionidindex();
	des_obj.state =
		static_cast<rinad::Flow::IPCPFlowState>(gpb.state());
	rina::FlowSpecification fs;
	flow_helpers::get_FlowSpecification(
		gpb.qosparameters(), fs);
	des_obj.flow_specification = fs;

	rina::DTPConfig dtp_config;
	cube_helpers::get_DTPConfig(gpb.dtpconfig(), dtp_config);
	des_obj.getActiveConnection()->setDTPConfig(dtp_config);

	rina::DTCPConfig dtcp_config;
	cube_helpers::get_DTCPConfig(gpb.dtcpconfig(), dtcp_config);
	des_obj.getActiveConnection()->setDTCPConfig(dtcp_config);

	des_obj.access_control = const_cast<char*>(gpb.accesscontrol()
		.c_str());
	des_obj.max_create_flow_retries = gpb.maxcreateflowretries();
	des_obj.create_flow_retries = gpb.createflowretries();
	des_obj.hop_count = gpb.hopcount();
}

}// namespace rinad
