/*
 * Common interfaces and classes for encoding/decoding RIB objects
 *
 *    Bernat Gaston <bernat.gaston@i2cat.net>
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

#include <cstring>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#define RINA_PREFIX "rinad.encoder"

#include <librina/logs.h>

#include "encoder.h"
#include "encoders/DataTransferConstantsMessage.pb.h"
#include "encoders/DirectoryForwardingTableEntryArrayMessage.pb.h"
#include "encoders/DIFAllocatorMessages.pb.h"
#include "encoders/QoSCubeArrayMessage.pb.h"
#include "encoders/WhatevercastNameArrayMessage.pb.h"
#include "encoders/NeighborArrayMessage.pb.h"
#include "encoders/QoSSpecification.pb.h"
#include "encoders/FlowMessage.pb.h"
#include "encoders/EnrollmentInformationMessage.pb.h"
#include "encoders/RoutingForwarding.pb.h"
#include "encoders/MA-IPCP.pb.h"
#include "encoders/RIBObjectData.pb.h"

namespace rinad {
namespace encoders {
namespace helpers {

void get_applicationProcessNamingInfo_t(
                const rina::ApplicationProcessNamingInformation &name,
                rina::messages::applicationProcessNamingInfo_t &gpb)
{
        gpb.set_applicationprocessname(name.processName);
        gpb.set_applicationprocessinstance(name.processInstance);
        gpb.set_applicationentityname(name.entityName);
        gpb.set_applicationentityinstance(name.entityInstance);
}

rina::messages::applicationProcessNamingInfo_t* get_applicationProcessNamingInfo_t(
                const rina::ApplicationProcessNamingInformation &name)
{
        rina::messages::applicationProcessNamingInfo_t *gpb =
                        new rina::messages::applicationProcessNamingInfo_t;
        get_applicationProcessNamingInfo_t(name, *gpb);
        return gpb;
}

void get_ApplicationProcessNamingInformation(
                const rina::messages::applicationProcessNamingInfo_t &gpf_app,
                rina::ApplicationProcessNamingInformation &app)
{
        app.processName = gpf_app.applicationprocessname();
        app.processInstance = gpf_app.applicationprocessinstance();
        app.entityName = gpf_app.applicationentityname();
        app.entityInstance = gpf_app.applicationentityinstance();
}

void get_property_t(rina::messages::property_t* gpb_conf,
                    const rina::PolicyParameter &conf)
{
        gpb_conf->set_name(conf.get_name());
        gpb_conf->set_value(conf.get_value());
}


void get_PolicyParameter(const rina::messages::property_t &gpf_conf,
                         rina::PolicyParameter &conf)
{

        conf.set_name(gpf_conf.name());
        conf.set_value(gpf_conf.value());
}

void get_PolicyConfig(const rina::messages::policyDescriptor_t &gpf_conf,
                      rina::PolicyConfig &conf)
{
        conf.set_name(gpf_conf.policyname());
        conf.set_version(gpf_conf.version());
        for (int i = 0; i < gpf_conf.policyparameters_size(); ++i)
        {
                rina::PolicyParameter param;
                get_PolicyParameter(gpf_conf.policyparameters(i), param);
                conf.parameters_.push_back(param);
        }
}

rina::messages::policyDescriptor_t* get_policyDescriptor_t(
                const rina::PolicyConfig &conf)
{
        rina::messages::policyDescriptor_t *gpb =
                        new rina::messages::policyDescriptor_t;

        gpb->set_policyname(conf.get_name());
        gpb->set_policyimplname(conf.get_name());
        gpb->set_version(conf.get_version());
        for (std::list<rina::PolicyParameter>::const_iterator it = conf
                        .get_parameters().begin();
                        it != conf.get_parameters().end(); ++it)
        {
                rina::messages::property_t * pro = gpb->add_policyparameters();
                get_property_t(pro, *it);
        }
        return gpb;
}

}  //namespace helpers

// CLASS DataTransferConstantsEncoder
namespace dt_const_helpers {
void toGPB(const rina::DataTransferConstants &obj,
           rina::messages::dataTransferConstants_t &gpb)
{
        gpb.set_maxpdulifetime(obj.max_pdu_lifetime_);
        gpb.set_addresslength(obj.address_length_);
        gpb.set_portidlength(obj.port_id_length_);
        gpb.set_cepidlength(obj.cep_id_length_);
        gpb.set_qosidlength(obj.qos_id_length_);
        gpb.set_sequencenumberlength(obj.sequence_number_length_);
        gpb.set_lengthlength(obj.length_length_);
        gpb.set_seqrolloverthreshold(obj.seq_rollover_thres_);
        gpb.set_difintegrity(obj.dif_integrity_);
        gpb.set_maxpdusize(obj.max_pdu_size_);
        gpb.set_ratelength(obj.rate_length_);
        gpb.set_framelength(obj.frame_length_);
        gpb.set_ctrlsequencenumberlength(obj.ctrl_sequence_number_length_);
        gpb.set_difconcatenation(obj.dif_concatenation_);
        gpb.set_diffragmentation(obj.dif_fragmentation_);
        gpb.set_maxtimetokeepretransmitting(obj.max_time_to_keep_ret_);
        gpb.set_maxtimetoack(obj.max_time_to_ack_);
}

void toModel(const rina::messages::dataTransferConstants_t &gpb,
             rina::DataTransferConstants &des_obj)
{
        des_obj.address_length_ = gpb.addresslength();
        des_obj.cep_id_length_ = gpb.cepidlength();
        des_obj.dif_integrity_ = gpb.difintegrity();
        des_obj.length_length_ = gpb.lengthlength();
        des_obj.max_pdu_lifetime_ = gpb.maxpdulifetime();
        des_obj.max_pdu_size_ = gpb.maxpdusize();
        des_obj.port_id_length_ = gpb.portidlength();
        des_obj.qos_id_length_ = gpb.qosidlength();
        des_obj.sequence_number_length_ = gpb.sequencenumberlength();
        des_obj.seq_rollover_thres_ = gpb.seqrolloverthreshold();
        des_obj.rate_length_ = gpb.ratelength();
        des_obj.frame_length_ = gpb.framelength();
        des_obj.ctrl_sequence_number_length_ = gpb.ctrlsequencenumberlength();
        des_obj.dif_concatenation_ = gpb.difconcatenation();
        des_obj.dif_fragmentation_ = gpb.diffragmentation();
        des_obj.max_time_to_keep_ret_ = gpb.maxtimetokeepretransmitting();
        des_obj.max_time_to_ack_ = gpb.maxtimetoack();
}
}  // namespace dt_const_helpers
void DataTransferConstantsEncoder::encode(
                const rina::DataTransferConstants &obj, rina::ser_obj_t& serobj)
{
        rina::messages::dataTransferConstants_t gpb;

        dt_const_helpers::toGPB(obj, gpb);
        serobj.size_ = gpb.ByteSize();
        serobj.message_ = new unsigned char[serobj.size_];
        gpb.SerializeToArray(serobj.message_, serobj.size_);
}

void DataTransferConstantsEncoder::decode(const rina::ser_obj_t &serobj,
                                          rina::DataTransferConstants &des_obj)
{
        rina::messages::dataTransferConstants_t gpb;

        gpb.ParseFromArray(serobj.message_, serobj.size_);
        dt_const_helpers::toModel(gpb, des_obj);
}

// CLASS DirectoryForwardingTableEntryEncoder

namespace dft_helpers {
void toGPB(const rina::DirectoryForwardingTableEntry &obj,
           rina::messages::directoryForwardingTableEntry_t &gpb)
{
        gpb.set_allocated_applicationname(
                        helpers::get_applicationProcessNamingInfo_t(
                                        obj.ap_naming_info_));
        gpb.set_ipcprocesssynonym(obj.address_);
        gpb.set_seqnum(obj.seqnum_);
}

void toModel(const rina::messages::directoryForwardingTableEntry_t &gpb,
             rina::DirectoryForwardingTableEntry &des_obj)
{
        rina::ApplicationProcessNamingInformation app_name;
        helpers::get_ApplicationProcessNamingInformation(gpb.applicationname(),
                                                         app_name);
        des_obj.ap_naming_info_ = app_name;
        des_obj.address_ = gpb.ipcprocesssynonym();
        des_obj.seqnum_ = gpb.seqnum();
}
}  // namespace dft_helpers

void DFTEEncoder::encode(const rina::DirectoryForwardingTableEntry &obj,
                         rina::ser_obj_t& serobj)
{
        rina::messages::directoryForwardingTableEntry_t gpb;

        dft_helpers::toGPB(obj, gpb);

        serobj.size_ = gpb.ByteSize();
        serobj.message_ = new unsigned char[serobj.size_];
        gpb.SerializeToArray(serobj.message_, serobj.size_);
}

void DFTEEncoder::decode(const rina::ser_obj_t &serobj,
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
	serobj.message_ = new unsigned char[serobj.size_];
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

namespace dif_alloc_helpers {
void toGPB(const AppToDIFMapping &obj,
           rina::messages::app_name_to_dif_mapping_t &gpb)
{
        gpb.set_allocated_applicationname(
                        helpers::get_applicationProcessNamingInfo_t(
                                        obj.app_name));
        gpb.set_difname(obj.dif_names.front());
}

void toModel(const rina::messages::app_name_to_dif_mapping_t &gpb,
             AppToDIFMapping &des_obj)
{
        helpers::get_ApplicationProcessNamingInformation(gpb.applicationname(),
                                                         des_obj.app_name);
        des_obj.dif_names.push_back(gpb.difname());
}
}  // namespace dft_helpers

//CLASS AppDIFMapping
void AppDIFMappingEncoder::encode(const AppToDIFMapping &obj, rina::ser_obj_t& serobj)
{
        rina::messages::app_name_to_dif_mapping_t gpb;

        dif_alloc_helpers::toGPB(obj, gpb);

        serobj.size_ = gpb.ByteSize();
        serobj.message_ = new unsigned char[serobj.size_];
        gpb.SerializeToArray(serobj.message_, serobj.size_);
}

void AppDIFMappingEncoder::decode(const rina::ser_obj_t &serobj, AppToDIFMapping &des_obj)
{
        rina::messages::app_name_to_dif_mapping_t gpb;
        gpb.ParseFromArray(serobj.message_, serobj.size_);
        dif_alloc_helpers::toModel(gpb, des_obj);
}

void AppDIFMappingListEncoder::encode(const std::list<AppToDIFMapping> &obj,
				      rina::ser_obj_t& serobj)
{
	rina::messages::app_name_to_dif_mapping_set_t gpb;

	for (std::list<AppToDIFMapping>::const_iterator it = obj.begin();
			it != obj.end(); ++it) {
		rina::messages::app_name_to_dif_mapping_t *gpb_dft;
		gpb_dft = gpb.add_appdifmapping();
		dif_alloc_helpers::toGPB((*it), *gpb_dft);
	}

	serobj.size_ = gpb.ByteSize();
	serobj.message_ = new unsigned char[serobj.size_];
	gpb.SerializeToArray(serobj.message_, serobj.size_);
}

void AppDIFMappingListEncoder::decode(const rina::ser_obj_t &serobj,
				      std::list<AppToDIFMapping> &des_obj)
{
	rina::messages::app_name_to_dif_mapping_set_t gpb;
	gpb.ParseFromArray(serobj.message_, serobj.size_);

	for (int i = 0; i < gpb.appdifmapping_size(); i++)
	{
		AppToDIFMapping admap;
		dif_alloc_helpers::toModel(gpb.appdifmapping(i), admap);
		des_obj.push_back(admap);
	}
}

// CLASS QoSCubeEncoder
namespace cube_helpers {
rina::messages::dtpConfig_t* get_dtpConfig_t(const rina::DTPConfig &conf)
{
        rina::messages::dtpConfig_t *gpf_conf = new rina::messages::dtpConfig_t;
        gpf_conf->set_dtcppresent(conf.is_dtcp_present());
        gpf_conf->set_seqnumrolloverthreshold(
                        conf.get_seq_num_rollover_threshold());
        gpf_conf->set_initialatimer(conf.get_initial_a_timer());
        gpf_conf->set_allocated_dtppolicyset(
                        helpers::get_policyDescriptor_t(
                                        conf.get_dtp_policy_set()));

        return gpf_conf;
}

void get_DTPConfig(const rina::messages::dtpConfig_t &gpf_conf,
                   rina::DTPConfig &conf)
{
        conf.set_dtcp_present(gpf_conf.dtcppresent());
        rina::PolicyConfig p_conf;
        helpers::get_PolicyConfig(gpf_conf.dtppolicyset(), p_conf);
        conf.set_dtp_policy_set(p_conf);
        conf.set_seq_num_rollover_threshold(gpf_conf.seqnumrolloverthreshold());
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
                const rina::DTCPRateBasedFlowControlConfig &conf)
{
        rina::messages::dtcpRateBasedFlowControlConfig_t *gpf_conf =
                        new rina::messages::dtcpRateBasedFlowControlConfig_t;

        gpf_conf->set_sendingrate(conf.get_sending_rate());
        gpf_conf->set_timeperiod(conf.get_time_period());
        gpf_conf->set_allocated_norateslowdownpolicy(
                        helpers::get_policyDescriptor_t(
                                        conf.get_no_rate_slow_down_policy()));
        gpf_conf->set_allocated_nooverridedefaultpeakpolicy(
                        helpers::get_policyDescriptor_t(
                                        conf.get_no_override_default_peak_policy()));
        gpf_conf->set_allocated_ratereductionpolicy(
                        helpers::get_policyDescriptor_t(
                                        conf.get_rate_reduction_policy()));

        return gpf_conf;
}

rina::messages::dtcpFlowControlConfig_t*
get_dtcpFlowControlConfig_t(const rina::DTCPFlowControlConfig &conf)
{
        rina::messages::dtcpFlowControlConfig_t *gpf_conf =
                        new rina::messages::dtcpFlowControlConfig_t;

        gpf_conf->set_windowbased(conf.is_window_based());
        gpf_conf->set_allocated_windowbasedconfig(
                        get_dtcpWindowBasedFlowControlConfig_t(
                                        conf.get_window_based_config()));
        gpf_conf->set_ratebased(conf.is_rate_based());
        gpf_conf->set_allocated_ratebasedconfig(
                        get_dtcpRateBasedFlowControlConfig_t(
                                        conf.get_rate_based_config()));
        gpf_conf->set_sentbytesthreshold(conf.get_sent_bytes_threshold());
        gpf_conf->set_sentbytespercentthreshold(
                        conf.get_sent_bytes_percent_threshold());
        gpf_conf->set_sentbuffersthreshold(conf.get_sent_buffers_threshold());
        gpf_conf->set_rcvbytesthreshold(conf.get_rcv_bytes_threshold());
        gpf_conf->set_rcvbytespercentthreshold(
                        conf.get_rcv_bytes_percent_threshold());
        gpf_conf->set_rcvbuffersthreshold(conf.get_rcv_buffers_threshold());
        gpf_conf->set_allocated_closedwindowpolicy(
                        helpers::get_policyDescriptor_t(
                                        conf.get_closed_window_policy()));
        gpf_conf->set_allocated_flowcontroloverrunpolicy(
                        helpers::get_policyDescriptor_t(
                                        conf.get_flow_control_overrun_policy()));
        gpf_conf->set_allocated_reconcileflowcontrolpolicy(
                        helpers::get_policyDescriptor_t(
                                        conf.get_reconcile_flow_control_policy()));
        gpf_conf->set_allocated_receivingflowcontrolpolicy(
                        helpers::get_policyDescriptor_t(
                                        conf.get_receiving_flow_control_policy()));

        return gpf_conf;
}

rina::messages::dtcpRtxControlConfig_t* get_dtcpRtxControlConfig_t(
                const rina::DTCPRtxControlConfig &conf)
{
        rina::messages::dtcpRtxControlConfig_t *gpf_conf =
                        new rina::messages::dtcpRtxControlConfig_t;

        gpf_conf->set_maxtimetoretry(conf.get_max_time_to_retry());
        gpf_conf->set_datarxmsnmax(conf.get_data_rxmsn_max());
        gpf_conf->set_allocated_rtxtimerexpirypolicy(
                        helpers::get_policyDescriptor_t(
                                        conf.get_rtx_timer_expiry_policy()));
        gpf_conf->set_allocated_senderackpolicy(
                        helpers::get_policyDescriptor_t(
                                        conf.get_sender_ack_policy()));
        gpf_conf->set_allocated_recvingacklistpolicy(
                        helpers::get_policyDescriptor_t(
                                        conf.get_recving_ack_list_policy()));
        gpf_conf->set_allocated_rcvrackpolicy(
                        helpers::get_policyDescriptor_t(
                                        conf.get_rcvr_ack_policy()));
        gpf_conf->set_allocated_sendingackpolicy(
                        helpers::get_policyDescriptor_t(
                                        conf.get_sending_ack_policy()));
        gpf_conf->set_allocated_rcvrcontrolackpolicy(
                        helpers::get_policyDescriptor_t(
                                        conf.get_rcvr_control_ack_policy()));
        gpf_conf->set_initialrtxtime(conf.get_initial_rtx_time());

        return gpf_conf;
}

rina::messages::dtcpConfig_t* get_dtcpConfig_t(const rina::DTCPConfig &conf)
{
        rina::messages::dtcpConfig_t *gpf_conf =
                        new rina::messages::dtcpConfig_t;

        gpf_conf->set_flowcontrol(conf.is_flow_control());
        gpf_conf->set_allocated_flowcontrolconfig(
                        get_dtcpFlowControlConfig_t(
                                        conf.get_flow_control_config()));
        gpf_conf->set_rtxcontrol(conf.is_rtx_control());
        gpf_conf->set_allocated_rtxcontrolconfig(
                        get_dtcpRtxControlConfig_t(
                                        conf.get_rtx_control_config()));
        gpf_conf->set_allocated_lostcontrolpdupolicy(
                        helpers::get_policyDescriptor_t(
                                        (conf.get_lost_control_pdu_policy())));
        gpf_conf->set_allocated_rttestimatorpolicy(
                        helpers::get_policyDescriptor_t(
                                        (conf.get_rtt_estimator_policy())));
        gpf_conf->set_allocated_dtcppolicyset(
                        helpers::get_policyDescriptor_t(
                                        (conf.get_dtcp_policy_set())));

        return gpf_conf;
}

void get_DTCPWindowBasedFlowControlConfig(
                const rina::messages::dtcpWindowBasedFlowControlConfig_t &gpf_conf,
                rina::DTCPWindowBasedFlowControlConfig &conf)
{

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
                rina::DTCPRateBasedFlowControlConfig &conf)
{
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
        get_DTCPWindowBasedFlowControlConfig(gpf_conf.windowbasedconfig(),
                                             window);
        conf.set_window_based_config(window);

        conf.set_rate_based(gpf_conf.ratebased());

        rina::DTCPRateBasedFlowControlConfig rate;
        get_DTCPRateBasedFlowControlConfig(gpf_conf.ratebasedconfig(), rate);
        conf.set_rate_based_config(rate);

        conf.set_sent_bytes_threshold(gpf_conf.sentbytesthreshold());
        conf.set_sent_bytes_percent_threshold(
                        gpf_conf.sentbytespercentthreshold());
        conf.set_sent_buffers_threshold(gpf_conf.sentbuffersthreshold());
        conf.set_rcv_bytes_threshold(gpf_conf.rcvbytesthreshold());
        conf.set_rcv_bytes_percent_threshold(
                        gpf_conf.rcvbytespercentthreshold());
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

rina::DTCPConfig* get_DTCPConfig(const rina::messages::dtcpConfig_t &gpf_conf,
                                 rina::DTCPConfig &conf)
{
        conf.flow_control_ = gpf_conf.flowcontrol();
        conf.rtx_control_ = gpf_conf.rtxcontrol();
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
        gpb.set_loss(obj.loss);
        gpb.set_maxallowablegapsdu(obj.max_allowable_gap_);
        gpb.set_name(obj.name_);
        gpb.set_order(obj.ordered_delivery_);
        gpb.set_partialdelivery(obj.partial_delivery_);
        gpb.set_peakbandwidthduration(obj.peak_bandwidth_duration_);
        gpb.set_peaksdubandwidthduration(obj.peak_sdu_bandwidth_duration_);
        gpb.set_qosid(obj.id_);
        gpb.set_undetectedbiterrorrate(obj.undetected_bit_error_rate_);
}

void toModel(const rina::messages::qosCube_t &gpb, rina::QoSCube &des_obj)
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
        des_obj.loss = gpb.loss();
        des_obj.max_allowable_gap_ = gpb.maxallowablegapsdu();
        des_obj.name_ = gpb.name();
        des_obj.ordered_delivery_ = gpb.order();
        des_obj.partial_delivery_ = gpb.partialdelivery();
        des_obj.peak_bandwidth_duration_ = gpb.peakbandwidthduration();
        des_obj.peak_sdu_bandwidth_duration_ = gpb.peaksdubandwidthduration();
        des_obj.id_ = gpb.qosid();
        des_obj.undetected_bit_error_rate_ = gpb.undetectedbiterrorrate();
}
}  // namespace cube_helpers

void QoSCubeEncoder::encode(const rina::QoSCube &obj, rina::ser_obj_t &serobj)
{
        rina::messages::qosCube_t gpb;

        cube_helpers::toGPB(obj, gpb);

        serobj.size_ = gpb.ByteSize();
        serobj.message_ = new unsigned char[serobj.size_];
        gpb.SerializeToArray(serobj.message_, serobj.size_);
}

void QoSCubeEncoder::decode(const rina::ser_obj_t &serobj,
                            rina::QoSCube &des_obj)

{
        rina::messages::qosCube_t gpb;
        gpb.ParseFromArray(serobj.message_, serobj.size_);

        cube_helpers::toModel(gpb, des_obj);
}

void QoSCubeListEncoder::encodePointers(const std::list<rina::QoSCube*> &obj,
                                        rina::ser_obj_t& serobj)
{
        rina::messages::qosCubes_t gpb;

        for (std::list<rina::QoSCube*>::const_iterator it = obj.begin();
                        it != obj.end(); ++it)
        {
                rina::messages::qosCube_t *gpb_cube;
                gpb_cube = gpb.add_qoscube();
                cube_helpers::toGPB(*(*it), *gpb_cube);
        }

        serobj.size_ = gpb.ByteSize();
        serobj.message_ = new unsigned char[serobj.size_];
        gpb.SerializeToArray(serobj.message_, serobj.size_);
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
	serobj.message_ = new unsigned char[serobj.size_];
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
namespace whatever_helpers {
void toGPB(const rina::WhatevercastName &obj,
           rina::messages::whatevercastName_t &gpb)
{
        gpb.set_name(obj.name_);
        gpb.set_rule(obj.rule_);
        for (std::list<std::string>::const_iterator it =
                        obj.set_members_.begin(); it != obj.set_members_.end();
                        ++it)
        {
                gpb.add_setmembers(*it);
        }
}

void toModel(const rina::messages::whatevercastName_t &gpb,
             rina::WhatevercastName &des_obj)
{
        des_obj.name_ = gpb.name();
        des_obj.rule_ = gpb.rule();
        for (int i = 0; i < gpb.setmembers_size(); i++)
        {
                des_obj.set_members_.push_back(gpb.setmembers(i));
        }
}
}  // namespace whatever_helpers

void WhatevercastNameEncoder::encode(const rina::WhatevercastName &obj,
                                     rina::ser_obj_t& serobj)
{
        rina::messages::whatevercastName_t gpb;

        whatever_helpers::toGPB(obj, gpb);

        serobj.size_ = gpb.ByteSize();
        serobj.message_ = new unsigned char[serobj.size_];
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
	serobj.message_ = new unsigned char[serobj.size_];
	gpb.SerializeToArray(serobj.message_, serobj.size_);
}
void WhatevercastNameListEncoder::decode(
                const rina::ser_obj_t &serobj,
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

namespace neighbor_helpers {
void toGPB(const rina::Neighbor &obj, rina::messages::neighbor_t &gpb)
{
        gpb.set_address(obj.address_);
        gpb.set_apname(obj.name_.processName);
        gpb.set_apinstance(obj.name_.processInstance);
        gpb.set_aename(obj.name_.entityName);
        gpb.set_aeinstance(obj.name_.entityInstance);
        for (std::list<rina::ApplicationProcessNamingInformation>::const_iterator it =
                        obj.supporting_difs_.begin();
                        it != obj.supporting_difs_.end(); ++it)
        {
                gpb.add_supportingdifs(it->processName);
        }
}
void toModel(const rina::messages::neighbor_t &gpb, rina::Neighbor &des_obj)
{
        des_obj.address_ = gpb.address();
        des_obj.name_.processName = gpb.apname();
        des_obj.name_.processInstance = gpb.apinstance();
        des_obj.name_.entityName = gpb.aename();
        des_obj.name_.entityInstance = gpb.aeinstance();
        for (int i = 0; i < gpb.supportingdifs_size(); i++)
        {
                des_obj.supporting_difs_.push_back(
                                rina::ApplicationProcessNamingInformation(
                                                gpb.supportingdifs(i), ""));
        }
}
}  // namespace neighbor_helpers
//Class NeighborEncoder
void NeighborEncoder::encode(const rina::Neighbor &obj, rina::ser_obj_t& serobj)
{
        rina::messages::neighbor_t gpb;

        neighbor_helpers::toGPB(obj, gpb);

        serobj.size_ = gpb.ByteSize();
        serobj.message_ = new unsigned char[serobj.size_];
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
void NeighborListEncoder::encode(const std::list<rina::Neighbor> &obj,
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
	serobj.message_ = new unsigned char[serobj.size_];
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
void ADataObjectEncoder::encode(const rina::cdap::ADataObject &obj,
                                rina::ser_obj_t& serobj)
{
        rina::messages::a_data_t gpb;

        gpb.set_sourceaddress(obj.source_address_);
        gpb.set_destaddress(obj.dest_address_);
        gpb.set_cdapmessage(obj.encoded_cdap_message_.message_,
                            obj.encoded_cdap_message_.size_);

        serobj.size_ = gpb.ByteSize();
        serobj.message_ = new unsigned char[serobj.size_];
        gpb.SerializeToArray(serobj.message_, serobj.size_);
}

void ADataObjectEncoder::decode(const rina::ser_obj_t &serobj,
                                rina::cdap::ADataObject &des_obj)
{
        rina::messages::a_data_t gpb;
        gpb.ParseFromArray(serobj.message_, serobj.size_);

        des_obj.source_address_ = gpb.sourceaddress();
        des_obj.dest_address_ = gpb.destaddress();

        des_obj.encoded_cdap_message_.size_ = gpb.cdapmessage().size();
        des_obj.encoded_cdap_message_.message_ = new unsigned char[des_obj
                        .encoded_cdap_message_.size_];
        memcpy(des_obj.encoded_cdap_message_.message_, gpb.cdapmessage().data(),
               gpb.cdapmessage().size());
}

// Class EnrollmentInformationRequestEncoder
void EnrollmentInformationRequestEncoder::encode(
                const configs::EnrollmentInformationRequest &obj,
                rina::ser_obj_t& serobj)
{
        rina::messages::enrollmentInformation_t gpb;

        gpb.set_address(obj.address_);
        gpb.set_startearly(obj.allowed_to_start_early_);
        gpb.set_token(obj.token);

        for (std::list<rina::ApplicationProcessNamingInformation>::const_iterator it =
                        obj.supporting_difs_.begin();
                        it != obj.supporting_difs_.end(); ++it)
        {
                gpb.add_supportingdifs(it->processName);
        }

        serobj.size_ = gpb.ByteSize();
        serobj.message_ = new unsigned char[serobj.size_];
        gpb.SerializeToArray(serobj.message_, serobj.size_);
}

void EnrollmentInformationRequestEncoder::decode(
                const rina::ser_obj_t &serobj,
                configs::EnrollmentInformationRequest &des_obj)
{
        rina::messages::enrollmentInformation_t gpb;
        gpb.ParseFromArray(serobj.message_, serobj.size_);

        des_obj.address_ = gpb.address();
        des_obj.allowed_to_start_early_ = gpb.startearly();
        des_obj.token = gpb.token();

        for (int i = 0; i < gpb.supportingdifs_size(); ++i)
        {
                des_obj.supporting_difs_.push_back(
                                rina::ApplicationProcessNamingInformation(
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
	gpf_flow_spec->set_loss(flow_spec.loss);
	gpf_flow_spec->set_msg_boundaries(flow_spec.msg_boundaries);

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
        qos.loss = gpf_qos.loss();
        qos.msg_boundaries = gpf_qos.msg_boundaries();
}
}  // namespace flow_helpers

void FlowEncoder::encode(const configs::Flow &obj, rina::ser_obj_t& serobj)
{
        rina::messages::Flow gpb;

        // SourceNamingInfo
        gpb.set_allocated_sourcenaminginfo(
                        helpers::get_applicationProcessNamingInfo_t(
                                        obj.local_naming_info));
        // DestinationNamingInfo
        gpb.set_allocated_destinationnaminginfo(
                        helpers::get_applicationProcessNamingInfo_t(
                                        obj.remote_naming_info));
        // sourcePortId
        gpb.set_sourceportid(obj.local_port_id);
        //destinationPortId
        gpb.set_destinationportid(obj.remote_port_id);
        //sourceAddress
        gpb.set_sourceaddress(obj.local_address);
        //destinationAddress
        gpb.set_destinationaddress(obj.remote_address);
        //connectionIds
        for (std::list<rina::Connection*>::const_iterator it = obj.connections
                        .begin(); it != obj.connections.end(); ++it)
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
        gpb.set_currentconnectionidindex(obj.current_connection_index);
        //state
        gpb.set_state(obj.state);
        //qosParameters
        gpb.set_allocated_qosparameters(
                        flow_helpers::get_qosSpecification_t(
                                        obj.flow_specification));
        //optional dtpConfig_t dtpConfig
        gpb.set_allocated_dtpconfig(
                        cube_helpers::get_dtpConfig_t(
                                        obj.getActiveConnection()->getDTPConfig()));
        //optional dtcpConfig_t dtpConfig
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
        serobj.message_ = new unsigned char[serobj.size_];
        gpb.SerializeToArray(serobj.message_, serobj.size_);
}

void FlowEncoder::decode(const rina::ser_obj_t &serobj, configs::Flow &des_obj)
{
        rina::messages::Flow gpb;

        gpb.ParseFromArray(serobj.message_, serobj.size_);

        rina::ApplicationProcessNamingInformation src_app;
        helpers::get_ApplicationProcessNamingInformation(gpb.sourcenaminginfo(),
                                                         src_app);
        des_obj.local_naming_info = src_app;

        rina::ApplicationProcessNamingInformation dest_app;
        helpers::get_ApplicationProcessNamingInformation(
                        gpb.destinationnaminginfo(), dest_app);
        des_obj.remote_naming_info = dest_app;

        des_obj.local_port_id = gpb.sourceportid();
        des_obj.remote_port_id = gpb.destinationportid();
        des_obj.local_address = gpb.sourceaddress();
        des_obj.remote_address = gpb.destinationaddress();

        rina::DTPConfig dtp_config;
        cube_helpers::get_DTPConfig(gpb.dtpconfig(), dtp_config);

        rina::DTCPConfig dtcp_config;
        cube_helpers::get_DTCPConfig(gpb.dtcpconfig(), dtcp_config);

        for (int i = 0; i < gpb.connectionids_size(); ++i)
        {
                rina::Connection* connection = new rina::Connection;
                flow_helpers::get_Connection(gpb.connectionids(i), *connection);
                connection->sourceAddress = gpb.sourceaddress();
                connection->destAddress = gpb.destinationaddress();
                connection->dtpConfig = dtp_config;
                connection->dtcpConfig = dtcp_config;
                des_obj.connections.push_back(connection);
        }
        des_obj.current_connection_index = gpb.currentconnectionidindex();
        des_obj.state = static_cast<configs::Flow::IPCPFlowState>(gpb.state());
        rina::FlowSpecification fs;
        flow_helpers::get_FlowSpecification(gpb.qosparameters(), fs);
        des_obj.flow_specification = fs;

        des_obj.access_control = const_cast<char*>(gpb.accesscontrol().c_str());
        des_obj.max_create_flow_retries = gpb.maxcreateflowretries();
        des_obj.create_flow_retries = gpb.createflowretries();
        des_obj.hop_count = gpb.hopcount();
}

// CLASS RoutingTableEntryEncoder
void RoutingTableEntryEncoder::encode(const rina::RoutingTableEntry &obj,
                                      rina::ser_obj_t& serobj)
{
        rina::messages::rt_entry_t gpb;

        gpb.set_address(obj.destination.addresses.front());
        gpb.set_qos_id(obj.qosId);
        gpb.set_cost(obj.cost);

        std::list<rina::NHopAltList>::const_iterator it;
        std::list<rina::IPCPNameAddresses>::const_iterator it2;
        for (it = obj.nextHopNames.begin();
        		it != obj.nextHopNames.end(); ++it)
        {
                rina::messages::uint_list_t *gpf_list = gpb.add_next_hops();
                for (it2 = it->alts.begin(); it2 != it->alts.end(); ++it2)
                {
                	gpf_list->add_member(it2->addresses.front());
                }
        }

        serobj.size_ = gpb.ByteSize();
        serobj.message_ = new unsigned char[serobj.size_];
        gpb.SerializeToArray(serobj.message_, serobj.size_);
}

void RoutingTableEntryEncoder::decode(const rina::ser_obj_t &serobj,
                                      rina::RoutingTableEntry &des_obj)
{
        rina::messages::rt_entry_t gpb;

        gpb.ParseFromArray(serobj.message_, serobj.size_);

        des_obj.destination.addresses.push_back(gpb.address());
        des_obj.qosId = gpb.qos_id();
        des_obj.cost = gpb.cost();

        for (int i = 0; i < gpb.next_hops_size(); ++i)
        {
                rina::NHopAltList list;
                rina::messages::uint_list_t gpb_list = gpb.next_hops(i);
                for (int j = 0; j < gpb_list.member_size(); j++)
                {
                	rina::IPCPNameAddresses ipcpna;
                	ipcpna.addresses.push_back(gpb_list.member(j));
                	list.alts.push_back(ipcpna);
                }

                des_obj.nextHopNames.push_back(list);
        }
}

// CLASS PDUForwardingTableEntry
void PDUForwardingTableEntryEncoder::encode(
                const rina::PDUForwardingTableEntry &obj,
                rina::ser_obj_t& serobj)
{
        rina::messages::pduft_entry_t gpb;

        gpb.set_address(obj.address);
        gpb.set_qos_id(obj.qosId);
        gpb.set_cost(obj.cost);

        std::list<rina::PortIdAltlist>::const_iterator it;
        std::list<unsigned int>::const_iterator it2;
        for (it = obj.portIdAltlists.begin(); it != obj.portIdAltlists.end();
                        ++it)
        {
                rina::messages::uint_list_t *gpf_list = gpb.add_port_ids();
                for (it2 = it->alts.begin(); it2 != it->alts.end(); ++it2)
                {
                        gpf_list->add_member(*it2);
                }
        }

        serobj.size_ = gpb.ByteSize();
        serobj.message_ = new unsigned char[serobj.size_];
        gpb.SerializeToArray(serobj.message_, serobj.size_);
}

void PDUForwardingTableEntryEncoder::decode(
                const rina::ser_obj_t &serobj,
                rina::PDUForwardingTableEntry &des_obj)
{
        rina::messages::pduft_entry_t gpb;

        gpb.ParseFromArray(serobj.message_, serobj.size_);

        des_obj.address = gpb.address();
        des_obj.qosId = gpb.qos_id();
        des_obj.cost = gpb.cost();

        for (int i = 0; i < gpb.port_ids_size(); ++i)
        {
                rina::PortIdAltlist list;
                rina::messages::uint_list_t gpb_list = gpb.port_ids(i);
                for (int j = 0; j < gpb_list.member_size(); j++)
                {
                        list.alts.push_back(gpb_list.member(j));
                }

                des_obj.portIdAltlists.push_back(list);
        }
}

void DTPInformationEncoder::encode(const rina::DTPInformation &obj,
                                   rina::ser_obj_t& serobj)
{
	rina::messages::connection_t gpb;

	gpb.set_src_cep_id(obj.src_cep_id);
	gpb.set_dest_cep_id(obj.dest_cep_id);
	gpb.set_qos_id(obj.qos_id);
	gpb.set_src_address(obj.src_address);
	gpb.set_dest_address(obj.dest_address);
	gpb.set_port_id(obj.port_id);
	gpb.set_allocated_dtp_config(
		cube_helpers::get_dtpConfig_t(obj.dtp_config));
	gpb.set_pdus_tx(obj.stats.tx_pdus);
	gpb.set_pdus_rx(obj.stats.rx_pdus);
	gpb.set_bytes_tx(obj.stats.tx_bytes);
	gpb.set_bytes_rx(obj.stats.rx_bytes);

	serobj.size_ = gpb.ByteSize();
	serobj.message_ = new unsigned char[serobj.size_];
	gpb.SerializeToArray(serobj.message_, serobj.size_);
}

void DTPInformationEncoder::decode(const rina::ser_obj_t &serobj,
                                   rina::DTPInformation &des_obj)
{
	rina::messages::connection_t gpb;

	gpb.ParseFromArray(serobj.message_, serobj.size_);

	des_obj.src_cep_id = gpb.src_cep_id();
	des_obj.dest_cep_id = gpb.dest_cep_id();
	des_obj.src_address = gpb.src_address();
	des_obj.dest_address = gpb.dest_address();
	des_obj.port_id = gpb.port_id();
	cube_helpers::get_DTPConfig(gpb.dtp_config(), des_obj.dtp_config);
	des_obj.stats.tx_pdus = gpb.pdus_tx();
	des_obj.stats.rx_pdus = gpb.pdus_rx();
	des_obj.stats.tx_bytes = gpb.bytes_tx();
	des_obj.stats.rx_bytes = gpb.bytes_rx();
}

void DTCPInformationEncoder::encode(const rina::DTCPConfig &obj,
                                    rina::ser_obj_t& serobj)
{
        rina::messages::dtcpConfig_t * gpb = NULL;

        gpb = cube_helpers::get_dtcpConfig_t(obj);

        serobj.size_ = gpb->ByteSize();
        serobj.message_ = new unsigned char[serobj.size_];
        gpb->SerializeToArray(serobj.message_, serobj.size_);

        delete gpb;
}

void DTCPInformationEncoder::decode(const rina::ser_obj_t &serobj,
                                    rina::DTCPConfig &des_obj)
{
        rina::messages::dtcpConfig_t gpb;

        gpb.ParseFromArray(serobj.message_, serobj.size_);

        cube_helpers::get_DTCPConfig(gpb, des_obj);
}

namespace ipcp_conf_helpers {
rina::messages::efcp_config_t* get_efcp_config_t(
                const rina::EFCPConfiguration &config)
{
        rina::messages::efcp_config_t* gpb = new rina::messages::efcp_config_t;
        rina::messages::dataTransferConstants_t* consta =
                        new rina::messages::dataTransferConstants_t;

        // data transfer constants
        dt_const_helpers::toGPB(config.data_transfer_constants_, *consta);
        gpb->set_allocated_data_transfer_constants(consta);

        // flow policy
        gpb->set_allocated_unknown_flowpolicy(
                        helpers::get_policyDescriptor_t(
                                        config.unknown_flowpolicy_));
        for (std::list<rina::QoSCube*>::const_iterator it = config.qos_cubes_
                        .begin(); it != config.qos_cubes_.end(); ++it)
        {
                rina::messages::qosCube_t* qos = gpb->add_qos_cubes();
                cube_helpers::toGPB(**it, *qos);
        }

        return gpb;
}

rina::messages::rmt_config_t* get_rmt_config_t(
                const rina::RMTConfiguration &config)
{
        rina::messages::rmt_config_t* gpb = new rina::messages::rmt_config_t;
        gpb->set_allocated_policy_set(
                        helpers::get_policyDescriptor_t(config.policy_set_));
        gpb->set_allocated_pft_conf(
                        helpers::get_policyDescriptor_t(
                                        config.pft_conf_.policy_set_));

        return gpb;
}

rina::messages::fa_config_t* get_fa_config_t(
                const rina::FlowAllocatorConfiguration &config)
{
        rina::messages::fa_config_t* gpb = new rina::messages::fa_config_t;
        gpb->set_allocated_policy_set(
                        helpers::get_policyDescriptor_t(config.policy_set_));
        gpb->set_max_create_flow_retries(config.max_create_flow_retries_);
        gpb->set_allocated_allocate_notify_policy(
                        helpers::get_policyDescriptor_t(
                                        config.allocate_notify_policy_));
        gpb->set_allocated_allocate_retry_policy(
                        helpers::get_policyDescriptor_t(
                                        config.allocate_retry_policy_));
        gpb->set_allocated_new_flow_request_policy(
                        helpers::get_policyDescriptor_t(
                                        config.new_flow_request_policy_));
        gpb->set_allocated_seq_rollover_policy(
                        helpers::get_policyDescriptor_t(
                                        config.seq_rollover_policy_));

        return gpb;
}

rina::messages::addre_pref_config_t* get_addre_pref_config_t(
                const rina::AddressPrefixConfiguration &config)
{
        rina::messages::addre_pref_config_t *gpb =
                        new rina::messages::addre_pref_config_t;
        gpb->set_address_prefix(config.address_prefix_);
        gpb->set_organization(config.organization_);
        return gpb;
}

rina::messages::static_addr_config_t* get_static_addr_config_t(
                const rina::StaticIPCProcessAddress &config)
{
        rina::messages::static_addr_config_t* gpb =
                        new rina::messages::static_addr_config_t;
        gpb->set_ap_name(config.ap_name_);
        gpb->set_ap_instance(config.ap_instance_);
        gpb->set_address(config.address_);
        return gpb;
}

void get_static_addr_config_t(const rina::StaticIPCProcessAddress &addr,
                              rina::messages::static_addr_config_t& gpb)
{
        gpb.set_ap_name(addr.ap_name_);
        gpb.set_ap_instance(addr.ap_instance_);
        gpb.set_address(addr.address_);
}

void get_addr_config_t(const rina::AddressPrefixConfiguration &addr,
                       rina::messages::addre_pref_config_t& gpb)
{
        gpb.set_address_prefix(addr.address_prefix_);
        gpb.set_organization(addr.organization_);
}

rina::messages::addr_config_t* get_addr_config_t(
                const rina::AddressingConfiguration &config)
{
        rina::messages::addr_config_t* gpb = new rina::messages::addr_config_t;
        for (std::list<rina::StaticIPCProcessAddress>::const_iterator it =
                        config.static_address_.begin();
                        it != config.static_address_.end(); ++it)
        {
                rina::messages::static_addr_config_t *st_gpb =
                                gpb->add_address();
                get_static_addr_config_t(*it, *st_gpb);
        }
        for (std::list<rina::AddressPrefixConfiguration>::const_iterator it =
                        config.address_prefixes_.begin();
                        it != config.address_prefixes_.end(); ++it)
        {
                rina::messages::addre_pref_config_t *pre_gpb =
                                gpb->add_prefixes();
                get_addr_config_t(*it, *pre_gpb);
        }
        return gpb;
}

rina::messages::nsm_config_t* get_nsm_config_t(
                const rina::NamespaceManagerConfiguration &config)
{
        rina::messages::nsm_config_t* gpb = new rina::messages::nsm_config_t;
        gpb->set_allocated_addressing_config(
                        get_addr_config_t(config.addressing_configuration_));
        gpb->set_allocated_policy_set(
                        helpers::get_policyDescriptor_t(config.policy_set_));
        return gpb;
}

void get_sduprotection_config_t(const std::string &dif_name,
                                const rina::AuthSDUProtectionProfile config,
                                rina::messages::sduprotection_config_t &gpb)
{
        gpb.set_allocated_authpolicy(
                        helpers::get_policyDescriptor_t(config.authPolicy));
        gpb.set_allocated_encryptpolicy(
                        helpers::get_policyDescriptor_t(config.encryptPolicy));
        gpb.set_allocated_crcpolicy(
                        helpers::get_policyDescriptor_t(config.crcPolicy));
        gpb.set_allocated_ttlpolicy(
                        helpers::get_policyDescriptor_t(config.ttlPolicy));
        gpb.set_dif_name(dif_name);
}

rina::messages::security_config_t* get_security_config_t(
                const rina::SecurityManagerConfiguration &config)
{
        rina::messages::security_config_t* gpb =
                        new rina::messages::security_config_t;
        gpb->set_allocated_policy_set(
                        helpers::get_policyDescriptor_t(config.policy_set_));
        rina::messages::sduprotection_config_t* sdu_gpb =
                        new rina::messages::sduprotection_config_t;
        // default dif name is not important
        std::string dif_name;
        get_sduprotection_config_t(dif_name, config.default_auth_profile,
                                   *sdu_gpb);
        gpb->set_allocated_default_auth_profile(sdu_gpb);
        for (std::map<std::string, rina::AuthSDUProtectionProfile>::const_iterator it =
                        config.specific_auth_profiles.begin();
                        it != config.specific_auth_profiles.end(); ++it)
        {
                sdu_gpb = gpb->add_specific_auth_profiles();
                get_sduprotection_config_t(it->first, it->second, *sdu_gpb);
        }
        return gpb;
}

rina::messages::dif_config_t* get_dif_config_t(
                const rina::DIFConfiguration &config)
{
        rina::messages::dif_config_t* gpb = new rina::messages::dif_config_t;
        gpb->set_address(config.address_);
        gpb->set_allocated_efcp_config(
                        get_efcp_config_t(config.efcp_configuration_));
        gpb->set_allocated_rmt_config(
                        get_rmt_config_t(config.rmt_configuration_));
        gpb->set_allocated_fa_config(get_fa_config_t(config.fa_configuration_));
        gpb->set_allocated_et_config(
                        helpers::get_policyDescriptor_t(
                                        config.et_configuration_.policy_set_));
        gpb->set_allocated_nsm_config(
                        get_nsm_config_t(config.nsm_configuration_));
        gpb->set_allocated_routing_config(
                        helpers::get_policyDescriptor_t(
                                        config.routing_configuration_
                                                        .policy_set_));
        gpb->set_allocated_ra_config(
                        helpers::get_policyDescriptor_t(
                                        config.ra_configuration_.pduftg_conf_
                                                        .policy_set_));
        gpb->set_allocated_sm_config(
                        get_security_config_t(config.sm_configuration_));
        for (std::list<rina::PolicyParameter>::const_iterator it = config
                        .parameters_.begin(); it != config.parameters_.end();
                        ++it)
        {
                rina::messages::property_t* pro = gpb->add_parameters();
                helpers::get_property_t(pro, *it);
        }

        return gpb;
}

rina::messages::dif_info_t* get_dif_info_t(const rina::DIFInformation &dif_info)
{
        rina::messages::dif_info_t *gpb = new rina::messages::dif_info_t;
        gpb->set_allocated_dif_name(
                        helpers::get_applicationProcessNamingInfo_t(
                                        dif_info.dif_name_));
        gpb->set_dif_type(dif_info.dif_type_);
        gpb->set_allocated_dif_config(
                        get_dif_config_t(dif_info.dif_configuration_));
        return gpb;
}

void get_neighbor_config_t(const configs::neighbor_config_t &neigh,
                           rina::messages::neighbor_config_t& gpb)
{
        gpb.set_allocated_neighbor_name(
                        helpers::get_applicationProcessNamingInfo_t(
                                        neigh.neighbor_name));
        gpb.set_allocated_under_dif(
                        helpers::get_applicationProcessNamingInfo_t(
                                        neigh.under_dif));
        gpb.set_allocated_dif(
                        helpers::get_applicationProcessNamingInfo_t(neigh.dif));
}

void get_EFCPConfiguration(const rina::messages::efcp_config_t &gpb,
                           rina::EFCPConfiguration &config)
{
        dt_const_helpers::toModel(gpb.data_transfer_constants(),
                                  config.data_transfer_constants_);
        helpers::get_PolicyConfig(gpb.unknown_flowpolicy(),
                                  config.unknown_flowpolicy_);
        for (int i = 0; i < gpb.qos_cubes_size(); i++)
        {
                rina::QoSCube *c_conf = new rina::QoSCube;
                cube_helpers::toModel(gpb.qos_cubes(i), *c_conf);
                config.qos_cubes_.push_back(c_conf);
        }

}

void get_AddressPrefixConfiguration(
                const rina::messages::addre_pref_config_t &gpb,
                rina::AddressPrefixConfiguration &config)
{
        config.address_prefix_ = gpb.address_prefix();
        config.organization_ = gpb.organization();
}

void get_StaticIPCProcessAddress(
                const rina::messages::static_addr_config_t &gpb,
                rina::StaticIPCProcessAddress &config)
{
        config.address_ = gpb.address();
        config.ap_instance_ = gpb.ap_instance();
        config.ap_name_ = gpb.ap_name();
}

void get_AddressingConfiguration(const rina::messages::addr_config_t &gpb,
                                 rina::AddressingConfiguration &config)
{
        for (int i = 0; i < gpb.address_size(); i++)
        {
                rina::StaticIPCProcessAddress p_conf;
                get_StaticIPCProcessAddress(gpb.address(i), p_conf);
                config.static_address_.push_back(p_conf);
        }
        for (int i = 0; i < gpb.prefixes_size(); i++)
        {
                rina::AddressPrefixConfiguration p_conf;
                get_AddressPrefixConfiguration(gpb.prefixes(i), p_conf);
                config.address_prefixes_.push_back(p_conf);
        }
}

void get_SDUProtConfiguration(const rina::messages::sduprotection_config_t &gpb,
                              std::string &dif_name,
                              rina::AuthSDUProtectionProfile &config)
{
        helpers::get_PolicyConfig(gpb.authpolicy(), config.authPolicy);
        helpers::get_PolicyConfig(gpb.encryptpolicy(), config.encryptPolicy);
        helpers::get_PolicyConfig(gpb.crcpolicy(), config.crcPolicy);
        helpers::get_PolicyConfig(gpb.ttlpolicy(), config.ttlPolicy);
        dif_name = gpb.dif_name();
}

void get_SecurityConfiguration(const rina::messages::security_config_t &gpb,
                               rina::SecurityManagerConfiguration &config)
{
        std::string dif_name;
        helpers::get_PolicyConfig(gpb.policy_set(), config.policy_set_);
        get_SDUProtConfiguration(gpb.default_auth_profile(), dif_name,
                                 config.default_auth_profile);
        for (int i = 0; i < gpb.specific_auth_profiles_size(); i++)
        {
                rina::AuthSDUProtectionProfile p_conf;
                get_SDUProtConfiguration(gpb.specific_auth_profiles(i),
                                         dif_name, p_conf);
                config.specific_auth_profiles[dif_name] = p_conf;
        }
}
void get_NSMConfiguration(const rina::messages::nsm_config_t &gpb,
                          rina::NamespaceManagerConfiguration &config)
{
        get_AddressingConfiguration(gpb.addressing_config(),
                                    config.addressing_configuration_);
        helpers::get_PolicyConfig(gpb.policy_set(), config.policy_set_);
}

void get_FAConfiguration(const rina::messages::fa_config_t &gpb,
                         rina::FlowAllocatorConfiguration &config)
{
        helpers::get_PolicyConfig(gpb.policy_set(), config.policy_set_);
        config.max_create_flow_retries_ = gpb.max_create_flow_retries();
        helpers::get_PolicyConfig(gpb.allocate_notify_policy(),
                                  config.allocate_notify_policy_);
        helpers::get_PolicyConfig(gpb.allocate_retry_policy(),
                                  config.allocate_retry_policy_);
        helpers::get_PolicyConfig(gpb.new_flow_request_policy(),
                                  config.new_flow_request_policy_);
        helpers::get_PolicyConfig(gpb.seq_rollover_policy(),
                                  config.seq_rollover_policy_);
}
void get_RMTConfiguration(const rina::messages::rmt_config_t &gpb,
                          rina::RMTConfiguration &config)
{
        helpers::get_PolicyConfig(gpb.policy_set(), config.policy_set_);
        helpers::get_PolicyConfig(gpb.pft_conf(), config.pft_conf_.policy_set_);
}

void get_DIFConfiguration(const rina::messages::dif_config_t & gpb,
                          rina::DIFConfiguration &config)
{
        config.address_ = gpb.address();
        get_EFCPConfiguration(gpb.efcp_config(), config.efcp_configuration_);
        get_RMTConfiguration(gpb.rmt_config(), config.rmt_configuration_);
        get_FAConfiguration(gpb.fa_config(), config.fa_configuration_);
        helpers::get_PolicyConfig(gpb.et_config(),
                                  config.et_configuration_.policy_set_);
        get_NSMConfiguration(gpb.nsm_config(), config.nsm_configuration_);
        helpers::get_PolicyConfig(gpb.routing_config(),
                                  config.routing_configuration_.policy_set_);
        helpers::get_PolicyConfig(
                        gpb.ra_config(),
                        config.ra_configuration_.pduftg_conf_.policy_set_);
        get_SecurityConfiguration(gpb.sm_config(), config.sm_configuration_);

        for (int i = 0; i < gpb.parameters_size(); i++)
        {
                rina::PolicyParameter p_conf;
                helpers::get_PolicyParameter(gpb.parameters(i), p_conf);
                config.parameters_.push_back(p_conf);
        }
}

void get_DIFInformation(const rina::messages::dif_info_t &gpb,
                        rina::DIFInformation &config)
{
        config.dif_type_ = gpb.dif_type();
        helpers::get_ApplicationProcessNamingInformation(gpb.dif_name(),
                                                         config.dif_name_);
        get_DIFConfiguration(gpb.dif_config(), config.dif_configuration_);
}

void get_Neighbors(const rina::messages::neighbor_config_t &gpb,
                   configs::neighbor_config_t& config)
{
        helpers::get_ApplicationProcessNamingInformation(gpb.neighbor_name(),
                                                         config.neighbor_name);
        helpers::get_ApplicationProcessNamingInformation(gpb.under_dif(),
                                                         config.under_dif);
        helpers::get_ApplicationProcessNamingInformation(gpb.dif(), config.dif);
}
}  //namespace ipcp_conf_helpers
// CLASS IPCPConfigEncoder
void IPCPConfigEncoder::encode(const configs::ipcp_config_t &obj,
                               rina::ser_obj_t& ser_obj)
{
        rina::messages::ipcp_config_t gpb;

        // name
        gpb.set_allocated_process_name(
                        helpers::get_applicationProcessNamingInfo_t(obj.name));
        // diff to assign
        gpb.set_allocated_dif_to_assign(
                        ipcp_conf_helpers::get_dif_info_t(obj.dif_to_assign));
        // Difs to register
        for (std::list<rina::ApplicationProcessNamingInformation>::const_iterator it =
                        obj.difs_to_register.begin();
                        it != obj.difs_to_register.end(); ++it)
        {
                rina::messages::applicationProcessNamingInfo_t *ap = gpb
                                .add_difs_to_register();
                helpers::get_applicationProcessNamingInfo_t(*it, *ap);
        }
        // Neighbors
        for (std::list<configs::neighbor_config_t>::const_iterator it = obj
                        .neighbors.begin(); it != obj.neighbors.end(); ++it)
        {
                rina::messages::neighbor_config_t* neigh_gpb =
                                gpb.add_neighbors();
                ipcp_conf_helpers::get_neighbor_config_t(*it, *neigh_gpb);
        }

        gpb.set_dif_template(obj.dif_template);

        //Allocate memory
        ser_obj.size_ = gpb.ByteSize();
        ser_obj.message_ = new unsigned char[ser_obj.size_];

        if (!ser_obj.message_)
                throw rina::Exception("out of memory");  //TODO improve this

        gpb.SerializeToArray(ser_obj.message_, ser_obj.size_);
}

void IPCPConfigEncoder::decode(const rina::ser_obj_t& ser_obj,
                               configs::ipcp_config_t& obj)
{
        rina::messages::ipcp_config_t gpb;
        gpb.ParseFromArray(ser_obj.message_, ser_obj.size_);
        //name
        helpers::get_ApplicationProcessNamingInformation(gpb.process_name(),
                                                         obj.name);
        // dif to assign
        ipcp_conf_helpers::get_DIFInformation(gpb.dif_to_assign(),
                                              obj.dif_to_assign);
        // difs to register
        for (int i = 0; i < gpb.difs_to_register_size(); i++)
        {
                rina::ApplicationProcessNamingInformation name;
                helpers::get_ApplicationProcessNamingInformation(
                                gpb.difs_to_register(i), name);
                obj.difs_to_register.push_back(name);
        }
        // neighbors
        for (int i = 0; i < gpb.neighbors_size(); i++)
        {
                configs::neighbor_config_t nei;
                ipcp_conf_helpers::get_Neighbors(gpb.neighbors(i), nei);
                obj.neighbors.push_back(nei);
        }

        obj.dif_template = gpb.dif_template();
}

//IPCP encoder
void IPCPEncoder::encode(const configs::ipcp_t& obj, rina::ser_obj_t& serobj)
{
        rina::messages::ipcp gpb;
        rina::messages::applicationProcessNamingInfo_t gpb_ap_name;

        gpb_ap_name.set_applicationentityinstance(
                        obj.process.processInstance.c_str());
        gpb_ap_name.set_applicationentityname(obj.process.processName.c_str());
        gpb.set_allocated_process(&gpb_ap_name);
        //TODO add name

        //Allocate memory
        serobj.size_ = gpb_ap_name.ByteSize();
        serobj.message_ = new unsigned char[serobj.size_];

        if (!serobj.message_)
                throw rina::Exception("out of memory");  //TODO improve this

        //Serialize and return
        gpb.SerializeToArray(serobj.message_, serobj.size_);
}

void IPCPEncoder::decode(const rina::ser_obj_t& serobj,
                         configs::ipcp_t& des_obj)
{

        rina::messages::ipcp gpb;
        gpb.ParseFromArray(serobj.message_, serobj.size_);

        des_obj.process.processName = gpb.process().applicationprocessname();
        des_obj.process.processInstance = gpb.process()
                        .applicationprocessinstance();
}


//CLASS RIBObjectDataListEncoder
void RIBObjectDataListEncoder::encode(
                const std::list<rina::rib::RIBObjectData> &obj,
                rina::ser_obj_t& serobj)
{
        rina::messages::RIBObjectDataList_t gpb;

        for (std::list<rina::rib::RIBObjectData>::const_iterator it = obj.begin();
                        it != obj.end(); ++it)
        {
                rina::messages::RIBObjectData_t *gpb_obj = gpb.add_objects();
                gpb_obj->set_class_(it->class_);
                gpb_obj->set_name(it->name_);
                gpb_obj->set_instance(it->instance_);
                gpb_obj->set_displayable_value(it->displayable_value_);
        }

        //Allocate memory
        serobj.size_ = gpb.ByteSize();
        serobj.message_ = new unsigned char[serobj.size_];

        if (!serobj.message_)
                throw rina::Exception("out of memory");  //TODO improve this

        //Serialize and return
        gpb.SerializeToArray(serobj.message_, serobj.size_);
}

void RIBObjectDataListEncoder::decode(const rina::ser_obj_t &serobj,
            std::list<rina::rib::RIBObjectData>& des_obj)
{
        rina::messages::RIBObjectDataList_t gpb;
        gpb.ParseFromArray(serobj.message_, serobj.size_);

        for (int i = 0; i < gpb.objects_size(); i++)
        {
                rina::rib::RIBObjectData obj;
                obj.class_ = gpb.objects(i).class_();
                obj.name_ = gpb.objects(i).name();
                obj.instance_ = gpb.objects(i).instance();
                obj.displayable_value_ = gpb.objects(i).displayable_value();
                des_obj.push_back(obj);
        }
}

}  //namespace encoders
}  // namespace rinad
