/*
 * Common interfaces and classes for encoding/decoding RIB objects
 *
 *    Bernat Gaston <bernat.gaston@i2cat.net>
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

#include "common/encoder.h"
#include "common/encoders/DataTransferConstantsMessage.pb.h"
#include "common/encoders/DirectoryForwardingTableEntryArrayMessage.pb.h"
#include "common/encoders/QoSCubeArrayMessage.pb.h"
#include "common/encoders/WhatevercastNameArrayMessage.pb.h"
#include "common/encoders/NeighborArrayMessage.pb.h"

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

/// CLASS Encoder
Encoder::~Encoder() {
	for (std::map<std::string,rina::EncoderInterface*>::iterator it = encoders_.begin(); it != encoders_.end(); ++it) {
		delete it->second;
		it->second = 0;
	}
	encoders_.clear();
}
void Encoder::addEncoder(const std::string& object_class, rina::EncoderInterface *encoder) {
	encoders_.insert(std::pair<std::string,rina::EncoderInterface*> (object_class, encoder));
}

void Encoder::encode(const void* object, rina::CDAPMessage * cdapMessage) {
	rina::EncoderInterface* encoder = get_encoder(cdapMessage->obj_class_);
	if (!encoder) {
		throw rina::Exception("Could not find encoder");
	}

	const rina::SerializedObject * encodedObject =  encoder->encode(object);
	if (!encodedObject) {
		throw rina::Exception("The encoder returned a null pointer");
	}

	cdapMessage->obj_value_ =  new rina::ByteArrayObjectValue(
			*encodedObject);

	delete encodedObject;

	return;
}
void* Encoder::decode(const rina::CDAPMessage * cdapMessage) {
	if (!cdapMessage->obj_value_) {
		throw rina::Exception ("Object value is null");
	}

	rina::EncoderInterface* encoder = get_encoder(cdapMessage->obj_class_);
	if (!encoder) {
		throw rina::Exception("Could not find encoder");
	}
	return encoder->decode(cdapMessage->obj_value_);
}

rina::EncoderInterface * Encoder::get_encoder(const std::string& object_class) {
	std::map<std::string, rina::EncoderInterface*>::iterator it = encoders_.find(object_class);
	if (it == encoders_.end()) {
		throw rina::Exception("Could not find an Encoder associated to object class");
	}

	return it->second;
}

rina::SerializedObject * Encoder::get_serialized_object(
		const rina::ObjectValueInterface * object_value) {
	if (!object_value) {
		throw rina::Exception ("Object value is null");
	}

	rina::ByteArrayObjectValue * value =
			(rina::ByteArrayObjectValue*) object_value;

	if (!value) {
		throw rina::Exception("Object value is not of type Byte Array Object Value");
	}

	return (rina::SerializedObject *) value->get_value();
}

rina::messages::applicationProcessNamingInfo_t* Encoder::get_applicationProcessNamingInfo_t(
		const rina::ApplicationProcessNamingInformation &name) {
	rina::messages::applicationProcessNamingInfo_t *gpf_name =
			new rina::messages::applicationProcessNamingInfo_t;
	gpf_name->set_applicationprocessname(name.processName);
	gpf_name->set_applicationprocessinstance(name.processInstance);
	gpf_name->set_applicationentityname(name.entityName);
	gpf_name->set_applicationentityinstance(name.entityInstance);
	return gpf_name;
}

rina::ApplicationProcessNamingInformation* Encoder::get_ApplicationProcessNamingInformation(
		const rina::messages::applicationProcessNamingInfo_t &gpf_app) {
	rina::ApplicationProcessNamingInformation *app = new rina::ApplicationProcessNamingInformation;

	app->processName = gpf_app.applicationprocessname();
	app->processInstance = gpf_app.applicationprocessinstance();
	app->entityName = gpf_app.applicationentityname();
	app->entityInstance = gpf_app.applicationentityinstance();

	return app;
}

rina::messages::qosSpecification_t* Encoder::get_qosSpecification_t(
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

void Encoder::get_property_t(rina::messages::property_t* gpb_conf, const rina::PolicyParameter &conf) {
	gpb_conf->set_name(conf.get_name());
	gpb_conf->set_value(conf.get_value());

	return;
}

rina::PolicyParameter* Encoder::get_PolicyParameter(const rina::messages::property_t &gpf_conf) {
	rina::PolicyParameter *conf = new rina::PolicyParameter;

	conf->set_name(gpf_conf.name());
	conf->set_value(gpf_conf.value());

	return conf;
}

rina::PolicyConfig* Encoder::get_PolicyConfig(const rina::messages::policyDescriptor_t &gpf_conf) {
	rina::PolicyConfig *conf = new rina::PolicyConfig;

	conf->set_name(gpf_conf.policyname());
	conf->set_version(gpf_conf.version());
	for (int i =0; i < gpf_conf.policyparameters_size(); ++i)	{
		rina::PolicyParameter *param = Encoder::get_PolicyParameter(gpf_conf.policyparameters(i));
		conf->parameters_.push_back(*param);
		delete param;
	}

	return conf;
}

rina::messages::policyDescriptor_t* Encoder::get_policyDescriptor_t(const rina::PolicyConfig &conf) {
	rina::messages::policyDescriptor_t *gpf_conf = new rina::messages::policyDescriptor_t;

	gpf_conf->set_policyname(conf.get_name());
	gpf_conf->set_policyimplname(conf.get_name());
	gpf_conf->set_version(conf.get_version());
	for (std::list<rina::PolicyParameter>::const_iterator it = conf.get_parameters().begin();
			it != conf.get_parameters().end(); ++it) {
		rina::messages::property_t * pro = gpf_conf->add_policyparameters();
		Encoder::get_property_t(pro, *it);
	}

	return gpf_conf;
}

rina::DTCPRtxControlConfig* Encoder::get_DTCPRtxControlConfig(const rina::messages::dtcpRtxControlConfig_t &gpf_conf) {
	rina::DTCPRtxControlConfig *conf = new rina::DTCPRtxControlConfig;

	conf->set_max_time_to_retry(gpf_conf.maxtimetoretry());
	conf->set_data_rxmsn_max(gpf_conf.datarxmsnmax());

	rina::PolicyConfig *polc = Encoder::get_PolicyConfig(gpf_conf.rtxtimerexpirypolicy());
	conf->set_rtx_timer_expiry_policy(*polc);
	delete polc;
	polc = 0;

	polc = Encoder::get_PolicyConfig(gpf_conf.senderackpolicy());
	conf->set_sender_ack_policy(*polc);
	delete polc;
	polc = 0;

	polc = Encoder::get_PolicyConfig(gpf_conf.recvingacklistpolicy());
	conf->set_recving_ack_list_policy(*polc);
	delete polc;
	polc = 0;

	polc = Encoder::get_PolicyConfig(gpf_conf.rcvrackpolicy());
	conf->set_rcvr_ack_policy(*polc);
	delete polc;
	polc = 0;

	polc = Encoder::get_PolicyConfig(gpf_conf.sendingackpolicy());
	conf->set_sending_ack_policy(*polc);
	delete polc;
	polc = 0;

	polc = Encoder::get_PolicyConfig(gpf_conf.rcvrcontrolackpolicy());
	conf->set_rcvr_control_ack_policy(*polc);
	delete polc;
	polc = 0;

	conf->set_initial_rtx_time(gpf_conf.initialrtxtime());

	return conf;
}

rina::messages::dtcpRtxControlConfig_t* Encoder::get_dtcpRtxControlConfig_t(const rina::DTCPRtxControlConfig &conf) {
	rina::messages::dtcpRtxControlConfig_t *gpf_conf = new rina::messages::dtcpRtxControlConfig_t;

	gpf_conf->set_maxtimetoretry(conf.get_max_time_to_retry());
	gpf_conf->set_datarxmsnmax(conf.get_data_rxmsn_max());
	gpf_conf->set_allocated_rtxtimerexpirypolicy(Encoder::get_policyDescriptor_t(conf.get_rtx_timer_expiry_policy()));
	gpf_conf->set_allocated_senderackpolicy(Encoder::get_policyDescriptor_t(conf.get_sender_ack_policy()));
	gpf_conf->set_allocated_recvingacklistpolicy(Encoder::get_policyDescriptor_t(conf.get_recving_ack_list_policy()));
	gpf_conf->set_allocated_rcvrackpolicy(Encoder::get_policyDescriptor_t(conf.get_rcvr_ack_policy()));
	gpf_conf->set_allocated_sendingackpolicy(Encoder::get_policyDescriptor_t(conf.get_sending_ack_policy()));
	gpf_conf->set_allocated_rcvrcontrolackpolicy(Encoder::get_policyDescriptor_t(conf.get_rcvr_control_ack_policy()));
	gpf_conf->set_initialrtxtime(conf.get_initial_rtx_time());

	return gpf_conf;
}

rina::messages::dtcpWindowBasedFlowControlConfig_t* Encoder::get_dtcpWindowBasedFlowControlConfig_t(
		const rina::DTCPWindowBasedFlowControlConfig &conf) {
	rina::messages::dtcpWindowBasedFlowControlConfig_t * gpf_conf = new rina::messages::dtcpWindowBasedFlowControlConfig_t;

	gpf_conf->set_maxclosedwindowqueuelength(conf.get_maxclosed_window_queue_length());
	gpf_conf->set_initialcredit(conf.get_initial_credit());
	gpf_conf->set_allocated_rcvrflowcontrolpolicy(Encoder::get_policyDescriptor_t(conf.get_rcvr_flow_control_policy()));
	gpf_conf->set_allocated_txcontrolpolicy(Encoder::get_policyDescriptor_t(conf.getTxControlPolicy()));

	return gpf_conf;
}

rina::DTCPWindowBasedFlowControlConfig* Encoder::get_DTCPWindowBasedFlowControlConfig(
		const rina::messages::dtcpWindowBasedFlowControlConfig_t &gpf_conf) {
	rina::DTCPWindowBasedFlowControlConfig *conf = new rina::DTCPWindowBasedFlowControlConfig;

	conf->set_max_closed_window_queue_length(gpf_conf.maxclosedwindowqueuelength());
	conf->set_initial_credit(gpf_conf.initialcredit());

	rina::PolicyConfig *poli = Encoder::get_PolicyConfig(gpf_conf.rcvrflowcontrolpolicy());
	conf->set_rcvr_flow_control_policy(*poli);
	delete poli;
	poli = 0;

	poli = Encoder::get_PolicyConfig(gpf_conf.txcontrolpolicy());
	conf->set_tx_control_policy(*poli);
	delete poli;
	poli = 0;

	return conf;
}

rina::messages::dtcpRateBasedFlowControlConfig_t* Encoder::get_dtcpRateBasedFlowControlConfig_t(
		const rina::DTCPRateBasedFlowControlConfig &conf) {
	rina::messages::dtcpRateBasedFlowControlConfig_t *gpf_conf = new rina::messages::dtcpRateBasedFlowControlConfig_t;

	gpf_conf->set_sendingrate(conf.get_sending_rate());
	gpf_conf->set_timeperiod(conf.get_time_period());
	gpf_conf->set_allocated_norateslowdownpolicy(Encoder::get_policyDescriptor_t(conf.get_no_rate_slow_down_policy()));
	gpf_conf->set_allocated_nooverridedefaultpeakpolicy(Encoder::get_policyDescriptor_t(conf.get_no_override_default_peak_policy()));
	gpf_conf->set_allocated_ratereductionpolicy(Encoder::get_policyDescriptor_t(conf.get_rate_reduction_policy()));

	return gpf_conf;
}

rina::DTCPRateBasedFlowControlConfig* Encoder::get_DTCPRateBasedFlowControlConfig(
		const rina::messages::dtcpRateBasedFlowControlConfig_t &gpf_conf) {
	rina::DTCPRateBasedFlowControlConfig *conf = new rina::DTCPRateBasedFlowControlConfig;

	conf->set_sending_rate(gpf_conf.sendingrate());
	conf->set_time_period(gpf_conf.timeperiod());
	rina::PolicyConfig *poli = Encoder::get_PolicyConfig(gpf_conf.norateslowdownpolicy());
	conf->set_no_rate_slow_down_policy(*poli);
	delete poli;
	poli = 0;

	poli = Encoder::get_PolicyConfig(gpf_conf.nooverridedefaultpeakpolicy());
	conf->set_no_override_default_peak_policy(*poli);
	delete poli;
	poli = 0;

	poli = Encoder::get_PolicyConfig(gpf_conf.ratereductionpolicy());
	conf->set_rate_reduction_policy(*poli);
	delete poli;
	poli = 0;

	return conf;
}

rina::DTCPFlowControlConfig* Encoder::get_DTCPFlowControlConfig(const rina::messages::dtcpFlowControlConfig_t &gpf_conf) {
	rina::DTCPFlowControlConfig *conf = new rina::DTCPFlowControlConfig;

	conf->set_window_based(gpf_conf.windowbased());
	rina::DTCPWindowBasedFlowControlConfig *window = Encoder::get_DTCPWindowBasedFlowControlConfig(gpf_conf.windowbasedconfig());
	conf->set_window_based_config(*window);
	delete window;
	window = 0;

	conf->set_rate_based(gpf_conf.ratebased());
	rina::DTCPRateBasedFlowControlConfig *rate = Encoder::get_DTCPRateBasedFlowControlConfig(gpf_conf.ratebasedconfig());
	conf->set_rate_based_config(*rate);
	delete rate;
	rate = 0;

	conf->set_sent_bytes_threshold(gpf_conf.sentbytesthreshold());
	conf->set_sent_bytes_percent_threshold(gpf_conf.sentbytespercentthreshold());
	conf->set_sent_buffers_threshold(gpf_conf.sentbuffersthreshold());
	conf->set_rcv_bytes_threshold(gpf_conf.rcvbytesthreshold());
	conf->set_rcv_bytes_percent_threshold(gpf_conf.rcvbytespercentthreshold());
	conf->set_rcv_buffers_threshold(gpf_conf.rcvbuffersthreshold());

	rina::PolicyConfig *poli = Encoder::get_PolicyConfig(gpf_conf.closedwindowpolicy());
	conf->set_closed_window_policy(*poli);
	delete poli;
	poli = 0;

	poli = Encoder::get_PolicyConfig(gpf_conf.flowcontroloverrunpolicy());
	conf->set_flow_control_overrun_policy(*poli);
	delete poli;
	poli = 0;

	poli = Encoder::get_PolicyConfig(gpf_conf.reconcileflowcontrolpolicy());
	conf->set_reconcile_flow_control_policy(*poli);
	delete poli;
	poli = 0;

	poli = Encoder::get_PolicyConfig(gpf_conf.receivingflowcontrolpolicy());
	conf->set_receiving_flow_control_policy(*poli);
	delete poli;
	poli = 0;

	return conf;
}

rina::messages::dtcpFlowControlConfig_t* Encoder::get_dtcpFlowControlConfig_t(const rina::DTCPFlowControlConfig &conf) {
	rina::messages::dtcpFlowControlConfig_t *gpf_conf = new rina::messages::dtcpFlowControlConfig_t ;

	gpf_conf->set_windowbased(conf.is_window_based());
	gpf_conf->set_allocated_windowbasedconfig(Encoder::get_dtcpWindowBasedFlowControlConfig_t(conf.get_window_based_config()));
	gpf_conf->set_ratebased(conf.is_rate_based());
	gpf_conf->set_allocated_ratebasedconfig(Encoder::get_dtcpRateBasedFlowControlConfig_t(conf.get_rate_based_config()));
	gpf_conf->set_sentbytesthreshold(conf.get_sent_bytes_threshold());
	gpf_conf->set_sentbytespercentthreshold(conf.get_sent_bytes_percent_threshold());
	gpf_conf->set_sentbuffersthreshold(conf.get_sent_buffers_threshold());
	gpf_conf->set_rcvbytesthreshold(conf.get_rcv_bytes_threshold());
	gpf_conf->set_rcvbytespercentthreshold(conf.get_rcv_bytes_percent_threshold());
	gpf_conf->set_rcvbuffersthreshold(conf.get_rcv_buffers_threshold());
	gpf_conf->set_allocated_closedwindowpolicy(Encoder::get_policyDescriptor_t(conf.get_closed_window_policy()));
	gpf_conf->set_allocated_flowcontroloverrunpolicy(Encoder::get_policyDescriptor_t(conf.get_flow_control_overrun_policy()));
	gpf_conf->set_allocated_reconcileflowcontrolpolicy(Encoder::get_policyDescriptor_t(conf.get_reconcile_flow_control_policy()));
	gpf_conf->set_allocated_receivingflowcontrolpolicy(Encoder::get_policyDescriptor_t(conf.get_receiving_flow_control_policy()));

	return gpf_conf;
}

rina::DTCPConfig* Encoder::get_DTCPConfig(const rina::messages::dtcpConfig_t &gpf_conf) {
	rina::DTCPConfig *conf = new rina::DTCPConfig;

	conf->flow_control_ = gpf_conf.flowcontrol();
	rina::DTCPFlowControlConfig *flow_conf = Encoder::get_DTCPFlowControlConfig(gpf_conf.flowcontrolconfig());
	conf->set_flow_control_config(*flow_conf);
	delete flow_conf;
	flow_conf = 0;

	conf->set_rtx_control(gpf_conf.rtxcontrol());
	rina::DTCPRtxControlConfig *rtx_conf = Encoder::get_DTCPRtxControlConfig(gpf_conf.rtxcontrolconfig());
	conf->set_rtx_control_config(*rtx_conf);
	delete rtx_conf;
	rtx_conf = 0;

	rina::PolicyConfig * p_conf = Encoder::get_PolicyConfig(gpf_conf.lostcontrolpdupolicy());
	conf->set_lost_control_pdu_policy(*p_conf);
	delete p_conf;
	p_conf = 0;

	p_conf = Encoder::get_PolicyConfig(gpf_conf.rttestimatorpolicy());
	conf->set_rtt_estimator_policy(*p_conf);
	delete p_conf;
	p_conf = 0;

	p_conf = Encoder::get_PolicyConfig(gpf_conf.dtcppolicyset());
	conf->set_dtcp_policy_set(*p_conf);
	delete p_conf;
	p_conf = 0;

	return conf;
}

rina::messages::dtcpConfig_t* Encoder::get_dtcpConfig_t(const rina::DTCPConfig &conf) {
	rina::messages::dtcpConfig_t *gpf_conf = new rina::messages::dtcpConfig_t;

	gpf_conf->set_flowcontrol(conf.is_flow_control());
	gpf_conf->set_allocated_flowcontrolconfig(Encoder::get_dtcpFlowControlConfig_t(conf.get_flow_control_config()));
	gpf_conf->set_rtxcontrol(conf.is_rtx_control());
	gpf_conf->set_allocated_rtxcontrolconfig(Encoder::get_dtcpRtxControlConfig_t(conf.get_rtx_control_config()));
	gpf_conf->set_allocated_lostcontrolpdupolicy(Encoder::get_policyDescriptor_t((conf.get_lost_control_pdu_policy())));
	gpf_conf->set_allocated_rttestimatorpolicy(Encoder::get_policyDescriptor_t((conf.get_rtt_estimator_policy())));
	gpf_conf->set_allocated_dtcppolicyset(Encoder::get_policyDescriptor_t((conf.get_dtcp_policy_set())));

	return gpf_conf;
}

rina::messages::dtpConfig_t* Encoder::get_dtpConfig_t(const rina::DTPConfig &conf) {
	rina::messages::dtpConfig_t *gpf_conf = new rina::messages::dtpConfig_t;

	gpf_conf->set_dtcppresent(conf.is_dtcp_present());
	gpf_conf->set_allocated_initialseqnumpolicy(get_policyDescriptor_t(conf.get_initial_seq_num_policy()));
	gpf_conf->set_seqnumrolloverthreshold(conf.get_seq_num_rollover_threshold());
	gpf_conf->set_initialatimer(conf.get_initial_a_timer());
	gpf_conf->set_allocated_rcvrtimerinactivitypolicy(get_policyDescriptor_t(conf.get_rcvr_timer_inactivity_policy()));
	gpf_conf->set_allocated_sendertimerinactiviypolicy(get_policyDescriptor_t(conf.get_sender_timer_inactivity_policy()));
	gpf_conf->set_allocated_dtppolicyset(get_policyDescriptor_t(conf.get_dtp_policy_set()));

	return gpf_conf;
}

rina::DTPConfig* Encoder::get_DTPConfig(const rina::messages::dtpConfig_t &gpf_conf) {
	rina::DTPConfig *conf = new rina::DTPConfig;

	conf->set_dtcp_present(gpf_conf.dtcppresent());

	rina::PolicyConfig *p_conf = get_PolicyConfig(gpf_conf.rcvrtimerinactivitypolicy());
	conf->set_rcvr_timer_inactivity_policy(*p_conf);
	delete p_conf;

	p_conf = get_PolicyConfig(gpf_conf.sendertimerinactiviypolicy());
	conf->set_sender_timer_inactivity_policy(*p_conf);
	delete p_conf;

	p_conf = get_PolicyConfig(gpf_conf.initialseqnumpolicy());
	conf->set_initial_seq_num_policy(*p_conf);
	delete p_conf;
	p_conf = 0;

	p_conf = get_PolicyConfig(gpf_conf.dtppolicyset());
	conf->set_dtp_policy_set(*p_conf);
	delete p_conf;
	p_conf = 0;

	conf->set_seq_num_rollover_threshold(gpf_conf.seqnumrolloverthreshold());
	conf->set_initial_a_timer(gpf_conf.initialatimer());

	return conf;
}

rina::Connection* Encoder::get_Connection(const rina::messages::connectionId_t &gpf_conn) {
	rina::Connection *conn = new rina::Connection;

	conn->setQosId(gpf_conn.qosid());
	conn->setSourceCepId(gpf_conn.sourcecepid());
	conn->setDestCepId(gpf_conn.destinationcepid());

	return conn;
}

rina::FlowSpecification* Encoder::get_FlowSpecification(const rina::messages::qosSpecification_t &gpf_qos) {
	rina::FlowSpecification *qos = new rina::FlowSpecification;

	qos->averageBandwidth = gpf_qos.averagebandwidth();
	qos->averageSDUBandwidth = gpf_qos.averagesdubandwidth();
	qos->peakBandwidthDuration = gpf_qos.peakbandwidthduration();
	qos->peakSDUBandwidthDuration = gpf_qos.peaksdubandwidthduration();
	qos->undetectedBitErrorRate = gpf_qos.undetectedbiterrorrate();
	qos->partialDelivery = gpf_qos.partialdelivery();
	qos->orderedDelivery = gpf_qos.partialdelivery();
	qos->maxAllowableGap = gpf_qos.maxallowablegapsdu();
	qos->delay = gpf_qos.delay();
	qos->jitter = gpf_qos.jitter();

	return qos;
}

// CLASS DataTransferConstantsEncoder
const rina::SerializedObject* DataTransferConstantsEncoder::encode(const void* object) {
	rina::DataTransferConstants *dtc = (rina::DataTransferConstants*) object;
	rina::messages::dataTransferConstants_t gpb_dtc;

	gpb_dtc.set_addresslength(dtc->address_length_);
	gpb_dtc.set_cepidlength(dtc->cep_id_length_);
	gpb_dtc.set_difintegrity(dtc->dif_integrity_);
	gpb_dtc.set_lengthlength(dtc->length_length_);
	gpb_dtc.set_maxpdulifetime(dtc->max_pdu_lifetime_);
	gpb_dtc.set_maxpdusize(dtc->max_pdu_size_);
	gpb_dtc.set_portidlength(dtc->port_id_length_);
	gpb_dtc.set_qosidlength(dtc->qos_id_length_);
	gpb_dtc.set_sequencenumberlength(dtc->sequence_number_length_);

	int size = gpb_dtc.ByteSize();
	char *serialized_message = new char[size];
	gpb_dtc.SerializeToArray(serialized_message, size);
	rina::SerializedObject *serialized_object =  new rina::SerializedObject(serialized_message,size);

	return serialized_object;
}

void* DataTransferConstantsEncoder::decode(
	const rina::ObjectValueInterface * object_value) const {
	rina::DataTransferConstants *dtc = new rina::DataTransferConstants();
	rina::messages::dataTransferConstants_t gpb_dtc;

	rina::SerializedObject * serializedObject =
			Encoder::get_serialized_object(object_value);

	gpb_dtc.ParseFromArray(serializedObject->message_, serializedObject->size_);

	dtc->address_length_ = gpb_dtc.addresslength();
	dtc->cep_id_length_ = gpb_dtc.cepidlength();
	dtc->dif_integrity_ = gpb_dtc.difintegrity();
	dtc->length_length_ = gpb_dtc.lengthlength();
	dtc->max_pdu_lifetime_ = gpb_dtc.maxpdulifetime();
	dtc->max_pdu_size_ = gpb_dtc.maxpdusize();
	dtc->port_id_length_ = gpb_dtc.portidlength();
	dtc->qos_id_length_ = gpb_dtc.qosidlength();
	dtc->sequence_number_length_ = gpb_dtc.sequencenumberlength();

	return (void*) dtc;
}

// CLASS DirectoryForwardingTableEntryEncoder
const rina::SerializedObject* DirectoryForwardingTableEntryEncoder::encode(const void* object) {
	rina::DirectoryForwardingTableEntry *dfte = (rina::DirectoryForwardingTableEntry*) object;
	rina::messages::directoryForwardingTableEntry_t gpb_dfte;

	DirectoryForwardingTableEntryEncoder::convertModelToGPB(&gpb_dfte, dfte);

	int size = gpb_dfte.ByteSize();
	char *serialized_message = new char[size];
	gpb_dfte.SerializeToArray(serialized_message, size);
	rina::SerializedObject *serialized_object =  new rina::SerializedObject(serialized_message,size);

	return serialized_object;
}

void* DirectoryForwardingTableEntryEncoder::decode(
		const rina::ObjectValueInterface * object_value) const {
	rina::messages::directoryForwardingTableEntry_t gpb_dtfe;

	rina::SerializedObject * serializedObject =
			Encoder::get_serialized_object(object_value);

	gpb_dtfe.ParseFromArray(serializedObject->message_, serializedObject->size_);

	return (void*) DirectoryForwardingTableEntryEncoder::convertGPBToModel(gpb_dtfe);
}

void DirectoryForwardingTableEntryEncoder::convertModelToGPB(rina::messages::directoryForwardingTableEntry_t * gpb_dfte,
		rina::DirectoryForwardingTableEntry * dfte) {
	gpb_dfte->set_allocated_applicationname(
			Encoder::get_applicationProcessNamingInfo_t(dfte->ap_naming_info_));
	gpb_dfte->set_ipcprocesssynonym(dfte->address_);
	gpb_dfte->set_timestamp(dfte->timestamp_);

	return;
}

rina::DirectoryForwardingTableEntry * DirectoryForwardingTableEntryEncoder::convertGPBToModel(
			const rina::messages::directoryForwardingTableEntry_t& gpb_dfte) {
	rina::DirectoryForwardingTableEntry *dtfe = new rina::DirectoryForwardingTableEntry();

	rina::ApplicationProcessNamingInformation *app_name =
			Encoder::get_ApplicationProcessNamingInformation(gpb_dfte.applicationname());
	dtfe->ap_naming_info_ = *app_name;
	delete app_name;
	app_name = 0;
	dtfe->address_ = gpb_dfte.ipcprocesssynonym();
	dtfe->timestamp_ = gpb_dfte.timestamp();

	return dtfe;
}

// Class DirectoryForwardingTableEntryListEncoder
const rina::SerializedObject* DirectoryForwardingTableEntryListEncoder::encode(const void* object) {
	std::list<rina::DirectoryForwardingTableEntry*> * list =
			(std::list<rina::DirectoryForwardingTableEntry*> *) object;
	std::list<rina::DirectoryForwardingTableEntry*>::const_iterator it;
	rina::messages::directoryForwardingTableEntrySet_t gpb_list;

	rina::messages::directoryForwardingTableEntry_t * gpb_dfte;
	for (it = list->begin(); it != list->end(); ++it) {
		gpb_dfte = gpb_list.add_directoryforwardingtableentry();
		DirectoryForwardingTableEntryEncoder::convertModelToGPB(gpb_dfte, (*it));
	}

	int size = gpb_list.ByteSize();
	char *serialized_message = new char[size];
	gpb_list.SerializeToArray(serialized_message, size);
	rina::SerializedObject *serialized_object =  new rina::SerializedObject(serialized_message,size);

	return serialized_object;
}

void* DirectoryForwardingTableEntryListEncoder::decode(const rina::ObjectValueInterface * object_value) const {
	rina::messages::directoryForwardingTableEntrySet_t gpb_list;

	rina::SerializedObject * serializedObject =
			Encoder::get_serialized_object(object_value);

	gpb_list.ParseFromArray(serializedObject->message_, serializedObject->size_);

	std::list<rina::DirectoryForwardingTableEntry*> * list = new std::list<rina::DirectoryForwardingTableEntry*>();

	for (int i = 0; i < gpb_list.directoryforwardingtableentry_size(); ++i) {
		list->push_back(DirectoryForwardingTableEntryEncoder::convertGPBToModel(
				gpb_list.directoryforwardingtableentry(i)));
	}

	return (void *) list;
}

// CLASS QoSCubeEncoder
const rina::SerializedObject* QoSCubeEncoder::encode(const void* object) {
	rina::QoSCube *cube = (rina::QoSCube*) object;
	rina::messages::qosCube_t gpb_cube;

	QoSCubeEncoder::convertModelToGPB(&gpb_cube, cube);

	int size = gpb_cube.ByteSize();
	char *serialized_message = new char[size];
	gpb_cube.SerializeToArray(serialized_message, size);
	rina::SerializedObject *serialized_object =  new rina::SerializedObject(serialized_message,size);

	return serialized_object;
}

void* QoSCubeEncoder::decode(
		const rina::ObjectValueInterface * object_value) const {
	rina::messages::qosCube_t gpb_cube;

	rina::SerializedObject * serializedObject =
			Encoder::get_serialized_object(object_value);

	gpb_cube.ParseFromArray(serializedObject->message_, serializedObject->size_);

	return (void*) QoSCubeEncoder::convertGPBToModel(gpb_cube);
}

void QoSCubeEncoder::convertModelToGPB(rina::messages::qosCube_t * gpb_cube,
		rina::QoSCube * cube) {
	gpb_cube->set_allocated_dtpconfiguration(Encoder::get_dtpConfig_t(cube->dtp_config_));
	gpb_cube->set_allocated_dtcpconfiguration(Encoder::get_dtcpConfig_t(cube->dtcp_config_));
	gpb_cube->set_averagebandwidth(cube->average_bandwidth_);
	gpb_cube->set_averagesdubandwidth(cube->average_sdu_bandwidth_);
	gpb_cube->set_delay(cube->delay_);
	gpb_cube->set_jitter(cube->jitter_);
	gpb_cube->set_maxallowablegapsdu(cube->max_allowable_gap_);
	gpb_cube->set_name(cube->name_);
	gpb_cube->set_order(cube->ordered_delivery_);
	gpb_cube->set_partialdelivery(cube->partial_delivery_);
	gpb_cube->set_peakbandwidthduration(cube->peak_bandwidth_duration_);
	gpb_cube->set_peaksdubandwidthduration(cube->peak_sdu_bandwidth_duration_);
	gpb_cube->set_qosid(cube->id_);
	gpb_cube->set_undetectedbiterrorrate(cube->undetected_bit_error_rate_);

	return;
}

rina::QoSCube * QoSCubeEncoder::convertGPBToModel(
			const rina::messages::qosCube_t & gpb_cube) {
	rina::QoSCube *cube = new rina::QoSCube();

	rina::DTPConfig *dtpConfig =
			Encoder::get_DTPConfig(gpb_cube.dtpconfiguration());
	cube->dtp_config_ = *dtpConfig;
	delete dtpConfig;
	dtpConfig = 0;

	rina::DTCPConfig *dtcpConfig =
			Encoder::get_DTCPConfig(gpb_cube.dtcpconfiguration());
	cube->dtcp_config_ = *dtcpConfig;
	delete dtcpConfig;
	dtcpConfig = 0;

	cube->average_bandwidth_ = gpb_cube.averagebandwidth();
	cube->average_sdu_bandwidth_ = gpb_cube.averagesdubandwidth();
	cube->delay_ = gpb_cube.delay();
	cube->jitter_ = gpb_cube.jitter();
	cube->max_allowable_gap_ = gpb_cube.maxallowablegapsdu();
	cube->name_ = gpb_cube.name();
	cube->ordered_delivery_ = gpb_cube.order();
	cube->partial_delivery_ = gpb_cube.partialdelivery();
	cube->peak_bandwidth_duration_ = gpb_cube.peakbandwidthduration();
	cube->peak_sdu_bandwidth_duration_ = gpb_cube.peaksdubandwidthduration();
	cube->id_ = gpb_cube.qosid();
	cube->undetected_bit_error_rate_ = gpb_cube.undetectedbiterrorrate();

	return cube;
}

// Class QoSCubeListEncoder
const rina::SerializedObject* QoSCubeListEncoder::encode(const void* object) {
	std::list<rina::QoSCube*> * list =
			(std::list<rina::QoSCube*> *) object;
	std::list<rina::QoSCube*>::const_iterator it;
	rina::messages::qosCubes_t gpb_list;

	rina::messages::qosCube_t * gpb_cube;
	for (it = list->begin(); it != list->end(); ++it) {
		gpb_cube = gpb_list.add_qoscube();
		QoSCubeEncoder::convertModelToGPB(gpb_cube, (*it));
	}

	int size = gpb_list.ByteSize();
	char *serialized_message = new char[size];
	gpb_list.SerializeToArray(serialized_message, size);
	rina::SerializedObject *serialized_object =  new rina::SerializedObject(serialized_message,size);

	return serialized_object;
}

void* QoSCubeListEncoder::decode(const rina::ObjectValueInterface * object_value) const {
	rina::messages::qosCubes_t gpb_list;

	rina::SerializedObject * serializedObject =
			Encoder::get_serialized_object(object_value);

	gpb_list.ParseFromArray(serializedObject->message_, serializedObject->size_);

	std::list<rina::QoSCube*> * list = new std::list<rina::QoSCube*>();
	for (int i = 0; i < gpb_list.qoscube_size(); ++i) {
		list->push_back(QoSCubeEncoder::convertGPBToModel(
				gpb_list.qoscube(i)));
	}

	return (void *) list;
}

//Class WhatevercastNameEncoder
const rina::SerializedObject* WhatevercastNameEncoder::encode(const void* object) {
	rina::WhatevercastName *name = (rina::WhatevercastName*) object;
	rina::messages::whatevercastName_t gpb_name;

	WhatevercastNameEncoder::convertModelToGPB(&gpb_name, name);

	int size = gpb_name.ByteSize();
	char *serialized_message = new char[size];
	gpb_name.SerializeToArray(serialized_message, size);
	rina::SerializedObject *serialized_object =  new rina::SerializedObject(serialized_message,size);

	return serialized_object;
}

void* WhatevercastNameEncoder::decode(
		const rina::ObjectValueInterface * object_value) const {
	rina::messages::whatevercastName_t gpb_name;

	rina::SerializedObject * serializedObject =
			Encoder::get_serialized_object(object_value);

	gpb_name.ParseFromArray(serializedObject->message_, serializedObject->size_);

	return (void*) WhatevercastNameEncoder::convertGPBToModel(gpb_name);
}

void WhatevercastNameEncoder::convertModelToGPB(rina::messages::whatevercastName_t * gpb_name,
		rina::WhatevercastName * name) {
	gpb_name->set_name(name->name_);
	gpb_name->set_rule(name->rule_);
	std::list<std::string>::iterator it;
	for (it=name->set_members_.begin(); it != name->set_members_.end(); ++it) {
		gpb_name->add_setmembers((*it));
	}

	return;
}

rina::WhatevercastName * WhatevercastNameEncoder::convertGPBToModel(
			const rina::messages::whatevercastName_t & gpb_name) {
	rina::WhatevercastName *name = new rina::WhatevercastName();

	name->name_ = gpb_name.name();
	name->rule_ = gpb_name.rule();
	for(int i=0; i<gpb_name.setmembers_size(); i++) {
		name->set_members_.push_back(gpb_name.setmembers(i));
	}

	return name;
}

// Class WhatevercastNameListEncoder
const rina::SerializedObject* WhatevercastNameListEncoder::encode(const void* object) {
	std::list<rina::WhatevercastName*> * list =
			(std::list<rina::WhatevercastName*> *) object;
	std::list<rina::WhatevercastName*>::const_iterator it;
	rina::messages::whatevercastNames_t gpb_list;

	rina::messages::whatevercastName_t * gpb_name;
	for (it = list->begin(); it != list->end(); ++it) {
		gpb_name = gpb_list.add_whatevercastname();
		WhatevercastNameEncoder::convertModelToGPB(gpb_name, (*it));
	}

	int size = gpb_list.ByteSize();
	char *serialized_message = new char[size];
	gpb_list.SerializeToArray(serialized_message, size);
	rina::SerializedObject *serialized_object =  new rina::SerializedObject(serialized_message,size);

	return serialized_object;
}

void* WhatevercastNameListEncoder::decode(const rina::ObjectValueInterface * object_value) const {
	rina::messages::whatevercastNames_t gpb_list;

	rina::SerializedObject * serializedObject =
			Encoder::get_serialized_object(object_value);

	gpb_list.ParseFromArray(serializedObject->message_, serializedObject->size_);

	std::list<rina::WhatevercastName*> * list = new std::list<rina::WhatevercastName*>();

	for (int i = 0; i < gpb_list.whatevercastname_size(); ++i) {
		list->push_back(WhatevercastNameEncoder::convertGPBToModel(
				gpb_list.whatevercastname(i)));
	}

	return (void *) list;
}

//Class NeighborEncoder
const rina::SerializedObject* NeighborEncoder::encode(const void* object) {
	rina::Neighbor * nei = (rina::Neighbor*) object;
	rina::messages::neighbor_t gpb_nei;

	NeighborEncoder::convertModelToGPB(&gpb_nei, nei);

	int size = gpb_nei.ByteSize();
	char *serialized_message = new char[size];
	gpb_nei.SerializeToArray(serialized_message, size);
	rina::SerializedObject *serialized_object =  new rina::SerializedObject(serialized_message,size);

	return serialized_object;
}

void* NeighborEncoder::decode(
		const rina::ObjectValueInterface * object_value) const {
	rina::messages::neighbor_t gpb_nei;

	rina::SerializedObject * serializedObject =
			Encoder::get_serialized_object(object_value);

	gpb_nei.ParseFromArray(serializedObject->message_, serializedObject->size_);

	return (void*) NeighborEncoder::convertGPBToModel(gpb_nei);
}

void NeighborEncoder::convertModelToGPB(rina::messages::neighbor_t * gpb_nei,
		rina::Neighbor * nei) {
	gpb_nei->set_address(nei->address_);
	gpb_nei->set_applicationprocessname(nei->name_.processName);
	gpb_nei->set_applicationprocessinstance(nei->name_.processInstance);
	std::list<rina::ApplicationProcessNamingInformation>::iterator it;
	for (it = nei->supporting_difs_.begin();
			it != nei->supporting_difs_.end(); ++it) {
		gpb_nei->add_supportingdifs(it->processName);
	}

	return;
}

rina::Neighbor * NeighborEncoder::convertGPBToModel(
			const rina::messages::neighbor_t & gpb_nei) {
	rina::Neighbor *nei = new rina::Neighbor();

	nei->address_ = gpb_nei.address();
	nei->name_.processName = gpb_nei.applicationprocessname();
	nei->name_.processInstance = gpb_nei.applicationprocessinstance();
	for(int i=0; i<gpb_nei.supportingdifs_size(); i++) {
		nei->supporting_difs_.push_back(rina::ApplicationProcessNamingInformation(
				gpb_nei.supportingdifs(i), ""));
	}

	return nei;
}

// Class NeighborListEncoder
const rina::SerializedObject* NeighborListEncoder::encode(const void* object) {
	std::list<rina::Neighbor*> * list =
			(std::list<rina::Neighbor*> *) object;
	std::list<rina::Neighbor*>::const_iterator it;
	rina::messages::neighbors_t gpb_list;

	rina::messages::neighbor_t * gpb_nei;
	for (it = list->begin(); it != list->end(); ++it) {
		gpb_nei = gpb_list.add_neighbor();
		NeighborEncoder::convertModelToGPB(gpb_nei, (*it));
	}

	int size = gpb_list.ByteSize();
	char *serialized_message = new char[size];
	gpb_list.SerializeToArray(serialized_message, size);
	rina::SerializedObject *serialized_object =  new rina::SerializedObject(serialized_message,size);

	return serialized_object;
}

void* NeighborListEncoder::decode(const rina::ObjectValueInterface * object_value) const {
	rina::messages::neighbors_t gpb_list;

	rina::SerializedObject * serializedObject =
			Encoder::get_serialized_object(object_value);

	gpb_list.ParseFromArray(serializedObject->message_, serializedObject->size_);

	std::list<rina::Neighbor*> * list = new std::list<rina::Neighbor*>();

	for (int i = 0; i < gpb_list.neighbor_size(); ++i) {
		list->push_back(NeighborEncoder::convertGPBToModel(
				gpb_list.neighbor(i)));
	}

	return (void *) list;
}

// Class WatchdogEncoder
const rina::SerializedObject* WatchdogEncoder::encode(const void* object) {
	return 0;
}

void* WatchdogEncoder::decode(const rina::ObjectValueInterface * object_value) const {
	rina::messages::neighbors_t gpb_list;
	int * result = 0;

	rina::IntObjectValue * value =
				(rina::IntObjectValue*) object_value;

	if (!value) {
		LOG_ERR("Problems decoding watchdog RIB object value");
		return 0;
	}

	result = new int;
	*result = * ((int *) value->get_value());
	return result;
}

// Class AdataUnitEncoder
// CLASS DataTransferConstantsEncoder
const rina::SerializedObject* ADataObjectEncoder::encode(const void* object) {
	rina::ADataObject *adata = (rina::ADataObject*) object;
	rina::messages::a_data_t gpb_adata;

	gpb_adata.set_sourceaddress(adata->source_address_);
	gpb_adata.set_destaddress(adata->dest_address_);
	gpb_adata.set_cdapmessage(adata->encoded_cdap_message_->message_,
			adata->encoded_cdap_message_->size_);

	int size = gpb_adata.ByteSize();
	char *serialized_message = new char[size];
	gpb_adata.SerializeToArray(serialized_message, size);
	rina::SerializedObject *serialized_object =  new rina::SerializedObject(serialized_message,size);

	return serialized_object;
}

void* ADataObjectEncoder::decode(
	const rina::ObjectValueInterface * object_value) const {
	rina::ADataObject *adata = new rina::ADataObject();
	rina::messages::a_data_t gpb_adata;

	rina::SerializedObject * serializedObject =
			Encoder::get_serialized_object(object_value);

	gpb_adata.ParseFromArray(serializedObject->message_, serializedObject->size_);

	adata->source_address_ = gpb_adata.sourceaddress();
	adata->dest_address_ = gpb_adata.destaddress();
	char *cdap_message = new char[gpb_adata.cdapmessage().size()];
	rina::SerializedObject * sr_message = new rina::SerializedObject(
			cdap_message, gpb_adata.cdapmessage().size());
	memcpy(cdap_message, gpb_adata.cdapmessage().data(), gpb_adata.cdapmessage().size());
	adata->encoded_cdap_message_ = sr_message;

	return (void*) adata;
}

}
