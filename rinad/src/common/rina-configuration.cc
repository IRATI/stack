/*
 * RINA configuration tree
 *
 *    Vincenzo Maffione <v.maffione@nextworks.it>
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

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <list>
#include <map>

#define RINA_PREFIX "rinad.rina-configuration"

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/json/json.h>
#include <librina/logs.h>
#include "rina-configuration.h"

using namespace std;

namespace rinad {

// Return the name of the template of the DIF named "difName" if it is known
// @param difName
// @result
bool RINAConfiguration::lookup_dif_template_mappings(
                const rina::ApplicationProcessNamingInformation& dif_name,
                DIFTemplateMapping& result) const
{
        for (list<DIFTemplateMapping>::const_iterator it = difConfigurations.begin();
                                        it != difConfigurations.end(); it++) {
                if (it->dif_name == dif_name) {
                        result = *it;
                        return true;
                }
        }

        return false;
}

#if 0
// Return the address of the IPC process named "name" if it is known,
// 0 otherwise
// @param dif_name
// @param ipcp_name
// @return
bool RINAConfiguration::lookup_ipcp_address(const string dif_name,
                const rina::ApplicationProcessNamingInformation& ipcp_name,
                unsigned int& result)
{
        DIFProperties dif_props;
        bool found;

        found = lookup_dif_properties(dif_name, dif_props);
        if (!found) {
                return false;
        }

        for (list<KnownIPCProcessAddress>::const_iterator
                it = dif_props.knownIPCProcessAddresses.begin();
                        it != dif_props.knownIPCProcessAddresses.end(); it++) {
                        if (it->name == ipcp_name) {
                                result = it->address;
                                return true;
                        }
        }

        return false;
}
#endif

string LocalConfiguration::toString() const
{
        ostringstream ss;

        ss << "Local Configuration" << endl;
        ss << "\tInstallation path: " << installationPath << endl;
        ss << "\tLibrary path: " << libraryPath << endl;
        ss << "\tLog path: " << logPath << endl;
        ss << "\tConsole socket: " << consoleSocket << endl;

	ss << "\tPlugins paths:" <<endl;
	for (list<string>::const_iterator lit = pluginsPaths.begin();
					lit != pluginsPaths.end(); lit++) {
		ss << "\t\t" << *lit << endl;
	}

        return ss.str();
}

static void
dump_map(stringstream& ss, const map<string, string>& m,
         unsigned int level)
{
        for (map<string, string>::const_iterator mit = m.begin();
                                mit != m.end(); mit++) {
                for (unsigned int i = 0; i < level; i++) {
                        ss << "\t";
                }
                ss << mit->first << ": " << mit->second << endl;
        }
}

string RINAConfiguration::toString() const
{
        stringstream ss;

        for (list<IPCProcessToCreate>::const_iterator it =
                        ipcProcessesToCreate.begin();
                                it != ipcProcessesToCreate.end(); it++) {
                ss << "IPC process to create:" << endl;

                ss << "\tName: " << it->name.toString() << endl;
                ss << "\tDIF Name: " << it->difName.toString() << endl;

                for (list<NeighborData>::const_iterator nit =
                                it->neighbors.begin();
                                        nit != it->neighbors.end(); nit++) {
                        ss << "\tNeighbor:" << endl;
                        ss << "\t\tName: " << nit->apName.toString() << endl;
                        ss << "\t\tDIF: " << nit->difName.toString() << endl;
                        ss << "\t\tSupporting DIF: " <<
                                nit->supportingDifName.toString() << endl;
                }

                ss << "\tDIFs to register at:" << endl;
                for (list<rina::ApplicationProcessNamingInformation>::
                        const_iterator nit = it->difsToRegisterAt.begin();
                                nit != it->difsToRegisterAt.end(); nit++) {
                        ss << "\t\t" << nit->toString() << endl;
                }
                ss << "\tHost name: " << it->hostname << endl;
                ss << "\tSDU protection options:" << endl;
                dump_map(ss, it->sduProtectionOptions, 2);
                ss << "\tParameters:" << endl;
                dump_map(ss, it->parameters, 2);
        }

        for (list<DIFTemplateMapping>::const_iterator it = difConfigurations.begin();
                                it != difConfigurations.end(); it++) {
                ss << "\tDIF name: " << it->dif_name.toString() << endl;
                ss << "\tTemplate name: " << it->template_name << endl;
        }

        return local.toString() + ss.str();
}

bool DIFTemplate::lookup_ipcp_address(
                const rina::ApplicationProcessNamingInformation& ipcp_name,
                unsigned int& result)
{
        for (list<KnownIPCProcessAddress>::const_iterator
                it = knownIPCProcessAddresses.begin();
                        it != knownIPCProcessAddresses.end(); it++) {
                        if (it->name.processName.compare(ipcp_name.processName) == 0 &&
                        		it->name.processInstance.compare(ipcp_name.processInstance) == 0) {
                                result = it->address;
                                return true;
                        }
        }

        return false;
}

std::string DIFTemplate::toString()
{
	std::stringstream ss;
	ss << "* TEMPLATE " << templateName << std::endl;
	ss << std::endl;
	ss << "** DIF type: " << difType << std::endl;
	ss << std::endl;
	if (difType == rina::NORMAL_IPC_PROCESS) {
		ss << "** DATA TRANSFER CONSTANTS **"<<std::endl;
		ss << dataTransferConstants.toString()<<std::endl;
		ss << std::endl;

		if (qosCubes.size() != 0) {
			ss << "** QOS CUBES **" << std::endl;
			ss << std::endl;
			std::list<rina::QoSCube>::iterator it;
			for (it = qosCubes.begin(); it != qosCubes.end(); ++ it) {
				ss << "*** QOS CUBE " << it->name_ << " ***" <<std::endl;
				ss << it->toString() << std::endl;
				ss << std::endl;
			}
		}

		ss << "** ENROLLMENT TASK **"<<std::endl;
		ss << etConfiguration.toString();
		ss << std::endl;

		ss << "** RELAYING AND MULTIPLEXING TASK ** "<<std::endl;
		ss <<rmtConfiguration.toString();
		ss << std::endl;

		ss << "** FLOW ALLOCATOR ** "<<std::endl;
		ss <<faConfiguration.toString();
		ss << std::endl;

		ss << "** ROUTING ** "<<std::endl;
		ss <<routingConfiguration.toString();
		ss << std::endl;

		ss << "** RESOURCE ALLOCATOR ** "<<std::endl;
		ss <<raConfiguration.toString();
		ss << std::endl;

		ss << "** NAMESPACE MANAGER ** "<<std::endl;
		ss <<nsmConfiguration.toString();
		ss << std::endl;

		if (knownIPCProcessAddresses.size() > 0) {
			ss << "** KNOWN IPCP ADDRESSES **" <<std::endl;
			std::list<KnownIPCProcessAddress>::iterator it;
			for (it = knownIPCProcessAddresses.begin();
					it != knownIPCProcessAddresses.end(); ++it) {
				ss << "   IPCP name: " << it->name.getProcessNamePlusInstance();
				ss << " address: " << it->address << std::endl;
			}
			ss << std::endl;
		}

		if (addressPrefixes.size() > 0) {
			ss << "** ADDRESS PREFIXES **" <<std::endl;
			std::list<AddressPrefixConfiguration>::iterator it;
			for (it = addressPrefixes.begin();
					it != addressPrefixes.end(); ++it) {
				ss << "   Organization: " << it->organization;
				ss << " address prefix: " << it->addressPrefix << std::endl;
			}
			ss << std::endl;
		}

		ss << "** SECURITY MANAGER **"<< std::endl;
		ss << secManConfiguration.toString() <<std::endl;
	}

	if (configParameters.size() != 0) {
		ss << "** CONFIG PARAMETERS **" << std::endl;
		std::map<std::string, std::string>::iterator it;
		for (it = configParameters.begin(); it != configParameters.end(); ++ it) {
			ss << "   Name: " << it->first;
			ss << "   Value: " << it->second << std::endl;
		}
		ss << std::endl;
	}

	ss << std::endl;

	return ss.str();
}

void parse_name(const Json::Value &root,
                rina::ApplicationProcessNamingInformation &name)
{
        name.processName     = root.get("apName",     string()).asString();
        name.processInstance = root.get("apInstance", string()).asString();
        name.entityName      = root.get("aeName",     string()).asString();
        name.entityInstance  = root.get("aeInstance", string()).asString();
}

void parse_policy(const Json::Value  & root,
                  const string       & name,
                  rina::PolicyConfig & pol)
{
        Json::Value p = root[name];

        if (p == 0)
        	return;

        pol.name_    = p.get("name", string()).asString();
        pol.version_ = p.get("version", string()).asString();

        Json::Value par = p["parameters"];
        if (par != 0) {
                for (unsigned int i = 0; i < par.size(); i++) {
                        rina::PolicyParameter pp;

                        pp.name_  = par[i].get("name",  string()).asString();
                        pp.value_ = par[i].get("value", string()).asString();
                        pol.parameters_.push_back(pp);
                }
        }
}

void parse_flow_ctrl(const Json::Value  root,
                     rina::DTCPConfig & dc)
{
        Json::Value flow_ctrl = root["flowControlConfig"];

        if (flow_ctrl != 0) {
                rina::DTCPFlowControlConfig fc;

                fc.window_based_ =
                        flow_ctrl.get("windowBased",fc.window_based_).asBool();

                // window_based_config_
                Json::Value w_flow_ctrl = flow_ctrl["windowBasedConfig"];
                if (w_flow_ctrl != 0) {
                        rina::DTCPWindowBasedFlowControlConfig wfc;

                        wfc.max_closed_window_queue_length_ = w_flow_ctrl
                                .get("maxClosedWindowQueueLength",
                                     wfc.max_closed_window_queue_length_)
                                .asUInt();

                        wfc.initial_credit_ = w_flow_ctrl
                                .get("initialCredit",
                                     wfc.initial_credit_)
                                .asUInt();

                        parse_policy(w_flow_ctrl,
                                     "rcvrFlowControlPolicy",
                                     wfc.rcvr_flow_control_policy_);

                        parse_policy(w_flow_ctrl,
                                     "txControlPolicy",
                                     wfc.tx_control_policy_);

                        fc.window_based_config_ = wfc;
                }

                fc.rate_based_ =
                        flow_ctrl.get("rateBased",
                                      fc.rate_based_).asBool();
                // rate_based_config_
                Json::Value r_flow_ctrl = flow_ctrl["rateBasedConfig"];
                if (r_flow_ctrl != 0) {
                        rina::DTCPRateBasedFlowControlConfig rfc;

                        rfc.sending_rate_ = r_flow_ctrl
                                .get("sendingRate", rfc.sending_rate_)
                                .asUInt();

                        rfc.time_period_ = r_flow_ctrl
                                .get("timePeriod", rfc.time_period_)
                                .asUInt();

                        parse_policy(r_flow_ctrl,
                                     "noRateSlowDownPolicy",
                                     rfc.no_rate_slow_down_policy_);

                        parse_policy(r_flow_ctrl,
                                     "noOverrideDefaultPeakPolicy",
                                     rfc.no_override_default_peak_policy_);

                        parse_policy(r_flow_ctrl,
                                     "rateReductionPolicy",
                                     rfc.rate_reduction_policy_);

                        fc.rate_based_config_ = rfc;

                }

                fc.sent_bytes_threshold_ = flow_ctrl
                        .get("sentBytesThreshold", fc.sent_bytes_threshold_)
                        .asInt();

                fc.sent_bytes_percent_threshold_ = flow_ctrl
                        .get("sentBytesPercentThreshold",
                             fc.sent_bytes_percent_threshold_)
                        .asInt();

                fc.sent_buffers_threshold_ = flow_ctrl
                        .get("sentBuffersThreshold",
                             fc.sent_buffers_threshold_)
                        .asInt();

                fc.rcv_bytes_threshold_ = flow_ctrl
                        .get("rcvBytesThreshold", fc.rcv_bytes_threshold_)
                        .asInt();

                fc.rcv_bytes_percent_threshold_ = flow_ctrl
                        .get("rcvBytesThreshold",
                             fc.rcv_bytes_percent_threshold_)
                        .asInt();

                fc.rcv_buffers_threshold_ = flow_ctrl
                        .get("rcvBuffersThreshold", fc.rcv_buffers_threshold_)
                        .asInt();

                parse_policy(flow_ctrl,
                             "closedWindowPolicy",
                             fc.closed_window_policy_);

                parse_policy(flow_ctrl,
                             "flowControlOverrunPolicy",
                             fc.flow_control_overrun_policy_);

                parse_policy(flow_ctrl,
                             "reconcileFlowControlPolicy",
                             fc.reconcile_flow_control_policy_);

                parse_policy(flow_ctrl,
                             "receivingFlowControlPolicy",
                             fc.receiving_flow_control_policy_);

                dc.flow_control_config_ = fc;
        }
}

void parse_rtx_flow_ctrl(const Json::Value  root,
                         rina::DTCPConfig & dc)
{
        Json::Value rtx_ctrl = root["rtxControlConfig"];
        if (rtx_ctrl != 0) {
                rina::DTCPRtxControlConfig rfc;

                rfc.max_time_to_retry_ =
                        rtx_ctrl.get("maxTimeToRetry",
                                     rfc.max_time_to_retry_)
                        .asUInt();

                rfc.data_rxms_nmax_ =
                        rtx_ctrl.get("dataRxmsNmax",
                                     rfc.data_rxms_nmax_)
                        .asUInt();

                rfc.initial_rtx_time_ =
                        rtx_ctrl.get("initialRtxTime",
                                     rfc.initial_rtx_time_)
                        .asUInt();

                parse_policy(rtx_ctrl,
                             "rtxTimerExpiryPolicy",
                             rfc.rtx_timer_expiry_policy_);

                parse_policy(rtx_ctrl,
                             "senderAckPolicy",
                             rfc.sender_ack_policy_);

                parse_policy(rtx_ctrl,
                             "recvingAckListPolicy",
                             rfc.recving_ack_list_policy_);

                parse_policy(rtx_ctrl,
                             "rcvrAckPolicy",
                             rfc.rcvr_ack_policy_);

                parse_policy(rtx_ctrl,
                             "sendingAckPolicy",
                             rfc.sending_ack_policy_);

                parse_policy(rtx_ctrl,
                             "rcvrControlAckPolicy",
                             rfc.rcvr_control_ack_policy_);

                dc.rtx_control_config_ = rfc;
        }
}

void parse_efcp_policies(const Json::Value  root,
                         rina::QoSCube    & cube)
{
        Json::Value efcp_conf = root["efcpPolicies"];
        if (efcp_conf != 0) {
                rina::DTPConfig dtpc;

                dtpc.dtcp_present_ = efcp_conf.get("dtcpPresent",
                                               dtpc.dtcp_present_).asBool();

                parse_policy(efcp_conf,
                             "dtpPolicySet",
                             dtpc.dtp_policy_set_);

                // DTCPConfig
                Json::Value dtcp_conf = efcp_conf["dtcpConfiguration"];
                if (dtcp_conf != 0) {
                        rina::DTCPConfig dtpcc;

                        dtpcc.flow_control_ =
                                dtcp_conf.get("flowControl",
                                              dtpcc.flow_control_).asBool();
                        // flow_control_config_
                        parse_flow_ctrl(dtcp_conf, dtpcc);

                        dtpcc.rtx_control_ =
                                dtcp_conf.get("rtxControl",
                                              dtpcc.rtx_control_).asBool();

                        // rtx_control_config_
                        parse_rtx_flow_ctrl(dtcp_conf, dtpcc);

                        parse_policy(dtcp_conf,
                                     "dtcpPolicySet",
                                     dtpcc.dtcp_policy_set_);

                        parse_policy(dtcp_conf,
                                     "lostControlPduPolicy",
                                     dtpcc.lost_control_pdu_policy_);

                        parse_policy(dtcp_conf,
                                     "rttEstimatorPolicy",
                                     dtpcc.rtt_estimator_policy_);

                        cube.dtcp_config_ = dtpcc;
                }

                dtpc.seq_num_rollover_threshold_ =
                        efcp_conf.get("seqNumRolloverThreshold",
                                    dtpc.seq_num_rollover_threshold_).asUInt();
                dtpc.initial_a_timer_ =
                        efcp_conf.get("initialATimer",
                                    dtpc.initial_a_timer_).asUInt();
                dtpc.partial_delivery_ =
                        efcp_conf.get("partialDelivery",
                                    dtpc.partial_delivery_).asBool();
                dtpc.incomplete_delivery_ =
                        efcp_conf.get("incompleteDelivery",
                                    dtpc.incomplete_delivery_).asBool();
                dtpc.in_order_delivery_ =
                        efcp_conf.get("inOrderDelivery",
                                    dtpc.in_order_delivery_).asBool();
                dtpc.max_sdu_gap_ =
                        efcp_conf.get("maxSduGap", dtpc.max_sdu_gap_).asInt();

                cube.dtp_config_ = dtpc;
        }
}

void parse_auth_sduprot_profile(const Json::Value  & root,
                  	        rina::AuthSDUProtectionProfile & profile)
{
	parse_policy(root, "authPolicy", profile.authPolicy);
	parse_policy(root, "encryptPolicy", profile.encryptPolicy);
	parse_policy(root, "TTLPolicy", profile.ttlPolicy);
	parse_policy(root, "ErrorCheckPolicy", profile.crcPolicy);
}

rinad::DIFTemplate * parse_dif_template_config(const Json::Value & root,
					       rinad::DIFTemplate * dif_template)
{
	dif_template->difType = root.get("difType", string())
                        		.asString();

	// Data transfer constants
	Json::Value dt_const = root["dataTransferConstants"];
	if (dt_const != 0) {
		rina::DataTransferConstants dt;

		// There is no asShort()
		dt.address_length_ =
				static_cast<unsigned short>
		(dt_const
				.get("addressLength", 0)
				.asUInt());
		dt.cep_id_length_ = static_cast<unsigned short>
		(dt_const
				.get("cepIdLength", 0)
				.asUInt());
		dt.dif_integrity_ = dt_const
				.get("difIntegrity", false)
				.asBool();
		dt.dif_fragmentation_ = dt_const
				.get("difFragmentation", false)
				.asBool();
		dt.length_length_ = static_cast<unsigned short>
		(dt_const
				.get("lengthLength", 0)
				.asUInt());
		dt.rate_length_ = static_cast<unsigned short>
		(dt_const
				.get("rateLength", 0)
				.asUInt());
		dt.frame_length_ = static_cast<unsigned short>
		(dt_const
				.get("frameLength", 0)
				.asUInt());
		dt.max_pdu_lifetime_ = dt_const
				.get("maxPduLifetime", 0)
				.asUInt();
		dt.max_pdu_size_ = dt_const
				.get("maxPduSize", 0)
				.asUInt();
		dt.max_sdu_size_ = dt_const
				.get("maxSduSize", 0)
				.asUInt();
		dt.port_id_length_ =
				static_cast<unsigned short>
		(dt_const
				.get("portIdLength", 0)
				.asUInt());
		dt.qos_id_length_ = static_cast<unsigned short>
		(dt_const
				.get("qosIdLength", 0)
				.asUInt());
		dt.sequence_number_length_ =
				static_cast<unsigned short>
		(dt_const
				.get("sequenceNumberLength", 0)
				.asUInt());
		dt.ctrl_sequence_number_length_ =
				static_cast<unsigned short>
		(dt_const
				.get("ctrlSequenceNumberLength", 0)
				.asUInt());
		dif_template->dataTransferConstants = dt;
	}

	// QoS cubes
	Json::Value cubes = root["qosCubes"];
	if (cubes != 0) {
		for (unsigned int j = 0; j < cubes.size(); j++) {
			// FIXME: Probably should have good default
			//        values. Check default constructor
			rina::QoSCube cube;

			cube.id_   = cubes[j].get("id", 0).asUInt();
			cube.name_ = cubes[j].get("name", string()).asString();

			parse_efcp_policies(cubes[j], cube);

			cube.average_bandwidth_ =
					cubes[j].get("averageBandwidth",
							cube.average_bandwidth_)
							.asUInt();
			cube.average_sdu_bandwidth_ =
					cubes[j].get("averageSduBandwidth",
							cube.average_sdu_bandwidth_)
							.asUInt();
			cube.peak_bandwidth_duration_ =
					cubes[j].get("peakBandwidthDuration",
							cube.peak_bandwidth_duration_)
							.asUInt();
			cube.peak_sdu_bandwidth_duration_ =
					cubes[j].get("peakSduBandwidthDuration",
							cube.peak_sdu_bandwidth_duration_)
							.asUInt();
			cube.undetected_bit_error_rate_ =
					cubes[j].get("undetectedBitErrorRate",
							cube.undetected_bit_error_rate_)
							.asDouble();
			cube.partial_delivery_ =
					cubes[j].get("partialDelivery",
							cube.partial_delivery_)
							.asBool();
			cube.ordered_delivery_ =
					cubes[j].get("orderedDelivery",
							cube.ordered_delivery_)
							.asBool();
			cube.max_allowable_gap_ =
					cubes[j].get("maxAllowableGap",
							cube.max_allowable_gap_)
							.asInt();
			cube.delay_ = cubes[j].get("delay", cube.delay_).asUInt();
			cube.jitter_ = cubes[j].get("jitter", cube.jitter_).asUInt();
			cube.loss = cubes[j].get("loss", cube.loss).asUInt();

			dif_template->qosCubes.push_back(cube);
		}
	}

	// rmtConfiguration;
	Json::Value rmt_conf = root["rmtConfiguration"];
	if (rmt_conf != 0) {
		rina::RMTConfiguration rc;

		parse_policy(rmt_conf,
			     "policySet",
			     rc.policy_set_);

	        Json::Value pft_conf = rmt_conf["pffConfiguration"];
                if (pft_conf != 0) {
                        rina::PFTConfiguration pftc;

                        parse_policy(pft_conf,
                                     "policySet",
                                     pftc.policy_set_);

                        rc.pft_conf_ = pftc;
                }
		dif_template->rmtConfiguration = rc;
	}

	// flowAllocatorConfiguration;
	Json::Value fa_conf = root["flowAllocatorConfiguration"];
	if (fa_conf != 0) {
		rina::FlowAllocatorConfiguration fac;

		parse_policy(fa_conf,
			     "policySet",
			     fac.policy_set_);

		dif_template->faConfiguration = fac;
	}

	// routingConfiguration;
	Json::Value routing_conf = root["routingConfiguration"];
	if (routing_conf != 0) {
		rina::RoutingConfiguration rc;

		parse_policy(routing_conf,
			     "policySet",
			     rc.policy_set_);

		dif_template->routingConfiguration = rc;
	}

	// resourceAllocatorConfiguration;
	Json::Value ra_conf = root["resourceAllocatorConfiguration"];
	if (ra_conf != 0) {
		rina::ResourceAllocatorConfiguration rac;

	        Json::Value pduftg_conf = ra_conf["pduftgConfiguration"];
                if (pduftg_conf != 0) {
                        rina::PDUFTGConfiguration pftgc;

		        parse_policy(pduftg_conf,
			             "policySet",
			             pftgc.policy_set_);
		        rac.pduftg_conf_ = pftgc;
                }

		dif_template->raConfiguration = rac;
	}

	// namespaceMangerConfiguration;
	Json::Value nsm_conf = root["namespaceManagerConfiguration"];
	if (nsm_conf != 0) {
		rina::NamespaceManagerConfiguration nsmc;

		parse_policy(nsm_conf,
			     "policySet",
			     nsmc.policy_set_);

		dif_template->nsmConfiguration = nsmc;
	}

	// std::list<KnownIPCProcessAddress>
	// knownIPCProcessAddresses;
	Json::Value known = root["knownIPCProcessAddresses"];
	if (known != 0) {
		for (unsigned int j = 0;
				j < known.size();
				j++) {
			rinad::KnownIPCProcessAddress kn;

			parse_name(known[j], kn.name);

			kn.address = known[j]
			                   .get("address", kn.address)
			                   .asUInt();

			dif_template->knownIPCProcessAddresses.push_back(kn);
		}
	}

	// std::list<AddressPrefixConfiguration> addressPrefixes;
	Json::Value addrp = root["addressPrefixes"];
	if (addrp != 0) {
		for (unsigned int j = 0;
				j < addrp.size();
				j++) {
			AddressPrefixConfiguration apc;

			apc.addressPrefix =
					addrp[j].get("addressPrefix",
							apc.addressPrefix)
							.asUInt();

			apc.organization =
					addrp[j].get("organization",
							string())
							.asString();

			dif_template->addressPrefixes.push_back(apc);
		}
	}

	// rina::EnrollmentTaskConfiguration
	// enrollmentTaskConfiguration;
	Json::Value etc = root["enrollmentTaskConfiguration"];
	if (etc != 0) {
		rina::EnrollmentTaskConfiguration et_conf;

		parse_policy(etc,
			     "policySet",
			     et_conf.policy_set_);

		dif_template->etConfiguration = et_conf;
	}

        //sduProtectionConfiguration
        Json::Value secManConf = root["securityManagerConfiguration"];
        if (secManConf != 0){
        	rina::SecurityManagerConfiguration sm_conf;

        	if (secManConf["policySet"] != 0) {
        		parse_policy(secManConf,
        			     "policySet",
        			     sm_conf.policy_set_);
        	}

        	Json::Value profiles = secManConf["authSDUProtProfiles"];
        	if (profiles != 0) {
        		Json::Value defaultProfile = profiles["default"];
        		if (defaultProfile != 0) {
        			parse_auth_sduprot_profile(defaultProfile,
        						   sm_conf.default_auth_profile);
        		}

        		Json::Value specifics = profiles["specific"];
        		if (specifics != 0) {
        			for (unsigned int j = 0;
        					j < specifics.size();
        					j++) {
        				rina::AuthSDUProtectionProfile profile;
        				std::string dif_name;

        				parse_auth_sduprot_profile(specifics[j], profile);

        				dif_name = specifics[j]
        				                   .get("underlyingDIF", dif_name)
        				                   .asString();

        				sm_conf.specific_auth_profiles[dif_name] = profile;
        			}
        		}
        	}

        	dif_template->secManConfiguration = sm_conf;
        }

	// configParameters;
	Json::Value confParams = root["configParameters"];

	if (confParams != 0) {
		Json::Value::Members members =
				confParams.getMemberNames();
		for (unsigned int j = 0; j < members.size(); j++) {
			string value = confParams
					.get(members[j], string()).asString();
			dif_template->configParameters
			.insert(pair<string, string>
			(members[j], value));
		}
	}

	return dif_template;
}

DIFTemplate * parse_dif_template(const std::string& file_name,
				 const std::string& template_name)
{
        // Parse config file with jsoncpp
        Json::Value  root;
        Json::Reader reader;
        ifstream     file;

        file.open(file_name.c_str(), std::ifstream::in);
        if (file.fail()) {
                LOG_ERR("Failed to open config file");
                return 0;
        }

        if (!reader.parse(file, root, false)) {
        	LOG_ERR("Failed to parse configuration");

        	// FIXME: Log messages need to take string for this to work
        	cout << "Failed to parse JSON" << endl
        			<< reader.getFormatedErrorMessages() << endl;

        	return 0;
        }

        file.close();

        DIFTemplate * dif_template = new DIFTemplate();
        dif_template->templateName = template_name;
        parse_dif_template_config(root, dif_template);

        return dif_template;
}

void parse_app_to_dif(const Json::Value & root,
		      std::list< std::pair<std::string, std::string> >& mappings)
{
	string encodedAppName;
	string difName;

        Json::Value appToDIF = root["applicationToDIFMappings"];
        if (appToDIF != 0) {
                for (unsigned int i = 0; i < appToDIF.size(); i++) {
                        encodedAppName = appToDIF[i].get("encodedAppName", string())
                                	            .asString();
                        difName = appToDIF[i].get("difName", string())
                        		     .asString();
                        std::pair<std::string, std::string> mapping(encodedAppName, difName);
                        mappings.push_back(mapping);

                        LOG_DBG("Added mapping of app %s to DIF %s",
                        	encodedAppName.c_str(),
                        	difName.c_str());
                }
        }
}

bool parse_app_to_dif_mappings(const std::string& file_name,
			       std::list< std::pair<std::string, std::string> >& mappings)
{
        // Parse config file with jsoncpp
        Json::Value  root;
        Json::Reader reader;
        ifstream     file;

        file.open(file_name.c_str());
        if (file.fail()) {
                LOG_WARN("Cannot open apps to DIF mappings file");
                return true;
        }

        if (!reader.parse(file, root, false)) {
        	LOG_ERR("Failed to parse apps to DIF mappings file");

        	// FIXME: Log messages need to take string for this to work
        	cout << "Failed to parse JSON" << endl
        			<< reader.getFormatedErrorMessages() << endl;

        	return false;
        }

        file.close();

        parse_app_to_dif(root, mappings);

        return true;
}

}
