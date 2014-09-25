/*
 * Configuration reader for IPC Manager
 *
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
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

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <string>

#define RINA_PREFIX "ipcm"

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/logs.h>

#include "rina-configuration.h"
#include "json/json.h"
#include "ipcm.h"

using namespace std;

namespace rinad {


void parse_name(const Json::Value &root,
                rina::ApplicationProcessNamingInformation &name)
{
        name.processName =
                root.get("apName",
                         string()).asString();
        name.processInstance =
                root.get("apInstance",
                         string()).asString();
        name.entityName =
                root.get("aeName",
                         string()).asString();
        name.entityInstance =
                root.get("aeInstance",
                         string()).asString();
}

void parse_policy(const Json::Value  &root,
                  const string       &name,
                  rina::PolicyConfig &pol)
{
        Json::Value p = root[name];

        pol.name_ = p.get("name", string()).asString();
        pol.version_ = p.get("version", string()).asString();

        Json::Value par = p["parameters"];
        if (par != 0) {
                for (unsigned int i = 0; i < par.size(); i++) {
                        rina::PolicyParameter pp;

                        pp.name_ = par[i].get("name", string()).asString();
                        pp.value_ = par[i].get("value", string()).asString();
                        pol.parameters_.push_back(pp);
                }
        }
}

void parse_flow_ctrl(const Json::Value root,
                     rina::DTCPConfig &dc)
{
        Json::Value flow_ctrl = root["flowControlConfig"];

        if (flow_ctrl != 0) {
                rina::DTCPFlowControlConfig fc;

                fc.window_based_ =
                        flow_ctrl.get("windowBased",
                                      fc.window_based_).asBool();

                // window_based_config_
                Json::Value w_flow_ctrl = flow_ctrl["windowBasedConfig"];
                if (w_flow_ctrl != 0) {
                        rina::DTCPWindowBasedFlowControlConfig wfc;

                        wfc.max_closed_window_queue_length_ =
                                w_flow_ctrl.get
                                ("maxClosedWindowQueueLength",
                                 wfc.max_closed_window_queue_length_)
                                .asUInt();

                        wfc.initial_credit_ =
                                w_flow_ctrl.get("initialCredit",
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

                        rfc.sending_rate_ =
                                r_flow_ctrl.get("sendingRate",
                                                rfc.sending_rate_)
                                .asUInt();

                        rfc.time_period_ =
                                r_flow_ctrl.get("timePeriod",
                                                rfc.time_period_)
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

                }

                fc.sent_bytes_threshold_ =
                        flow_ctrl.get("sentBytesThreshold",
                                      fc.sent_bytes_threshold_)
                        .asInt();

                fc.sent_bytes_percent_threshold_ =
                        flow_ctrl.get("sentBytesPercentThreshold",
                                      fc.sent_bytes_percent_threshold_)
                        .asInt();

                fc.sent_buffers_threshold_ =
                        flow_ctrl.get("sentBuffersThreshold",
                                      fc.sent_buffers_threshold_)
                        .asInt();

                fc.rcv_bytes_threshold_ =
                        flow_ctrl.get("rcvBytesThreshold",
                                      fc.rcv_bytes_threshold_)
                        .asInt();

                fc.rcv_bytes_percent_threshold_ =
                        flow_ctrl.get("rcvBytesThreshold",
                                      fc.rcv_bytes_percent_threshold_)
                        .asInt();

                fc.rcv_buffers_threshold_ =
                        flow_ctrl.get("rcvBuffersThreshold",
                                      fc.rcv_buffers_threshold_)
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

void parse_rtx_flow_ctrl(const Json::Value root,
                         rina::DTCPConfig &dc)
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


void parse_efcp_policies(const Json::Value root,
                         rina::QoSCube    &cube)
{
        Json::Value con_pol = root["efcpPolicies"];
        if (con_pol != 0) {
                rina::ConnectionPolicies cp;

                cp.dtcp_present_ =
                        con_pol.get("dtcpPresent", cp.dtcp_present_).asBool();

                // DTCPConfig
                Json::Value dtcp_conf = con_pol["dtcpConfiguration"];
                if (dtcp_conf != 0) {
                        rina::DTCPConfig dc;

                        dc.flow_control_ =
                                dtcp_conf.get("flowControl",
                                              dc.flow_control_).asBool();
                        // flow_control_config_
                        parse_flow_ctrl(dtcp_conf, dc);

                        dc.rtx_control_ =
                                dtcp_conf.get("rtxControl",
                                              dc.rtx_control_).asBool();

                        // rtx_control_config_
                        parse_rtx_flow_ctrl(dtcp_conf, dc);

                        parse_policy(dtcp_conf,
                                     "lostControlPduPolicy",
                                     dc.lost_control_pdu_policy_);

                        parse_policy(dtcp_conf,
                                     "rttEstimatorPolicy",
                                     dc.rtt_estimator_policy_);
                }

                parse_policy(dtcp_conf,
                             "rcvrTimerInactivityPolicy",
                             cp.rcvr_timer_inactivity_policy_);

                parse_policy(dtcp_conf,
                             "senderTimerInactivityPolicy",
                             cp.sender_timer_inactivity_policy_);

                parse_policy(con_pol,
                             "initialSeqNumPolicy",
                             cp.initial_seq_num_policy_);

                cp.seq_num_rollover_threshold_ =
                        con_pol.get("seqNumRolloverThreshold",
                                    cp.seq_num_rollover_threshold_).asUInt();
                cp.initial_a_timer_ =
                        con_pol.get("initialATimer",
                                    cp.initial_a_timer_).asUInt();
                cp.partial_delivery_ =
                        con_pol.get("partialDelivery",
                                    cp.partial_delivery_).asBool();
                cp.incomplete_delivery_ =
                        con_pol.get("incompleteDelivery",
                                    cp.incomplete_delivery_).asBool();
                cp.in_order_delivery_ =
                        con_pol.get("inOrderDelivery",
                                    cp.in_order_delivery_).asBool();
                cp.max_sdu_gap_ =
                        con_pol.get("maxSduGap", cp.max_sdu_gap_).asInt();

                cube.efcp_policies_ = cp;
        }
}

void parse_dif_configs(const Json::Value   &root,
                       list<DIFProperties> &difConfigurations)
{
		const std::string NORMAL_TYPE = "normal-ipc";
		rina::IPCProcessFactory fact;
        Json::Value dif_configs = root["difConfigurations"];
        for (unsigned int i = 0; i < dif_configs.size(); i++) {
                rinad::DIFProperties props;

                props.difName =
                        rina::ApplicationProcessNamingInformation
                        (dif_configs[i].get("difName", string()).asString(),
                         string());

                props.difType = dif_configs[i].get("difType",
                                                   string()).asString();
                std::list<std::string> suportedDIFS = fact.getSupportedIPCProcessTypes();
                bool difCorrect = false;
            	std::string s;
                for (std::list<std::string>::iterator it = suportedDIFS.begin(); it != suportedDIFS.end(); ++it) {
                	if (props.difType.compare(*it) == 0)
                		difCorrect = true;
                	s.append(*it);
                	s.append(", ");
                }
                if (!difCorrect)
                {

                	std::stringstream ss;
                	ss << "difType parameter of DIF " << props.difName.processName <<" is wrong, options are: "
                			<<s;
                	throw Exception(ss.str().c_str());
                }
                LOG_INFO("Parsing configuration of DIF %s of type %s",
                		props.difName.processName.c_str(),
                		props.difType.c_str());

                // normal.DIF specific configurations
                if(props.difType.compare(NORMAL_TYPE) == 0){
                    // Data transfer constants
                	Json::Value dt_const = dif_configs[i]["dataTransferConstants"];
					if (dt_const != 0) {
							rina::DataTransferConstants dt;

							// There is no asShort()
							dt.address_length_ = static_cast<unsigned short>
									(dt_const.get("addressLength", 0).asUInt());
							dt.cep_id_length_ = static_cast<unsigned short>
									(dt_const.get("cepIdLength", 0).asUInt());
							dt.dif_integrity_ = dt_const.get
									("difIntegrity", false).asBool();
							dt.length_length_ = static_cast<unsigned short>
									(dt_const.get("lengthLength", 0).asUInt());
							dt.max_pdu_lifetime_ = dt_const.get
									("maxPduLifetime", 0).asUInt();
							dt.max_pdu_size_ = dt_const.get
									("maxPduSize", 0).asUInt();
							dt.port_id_length_ = static_cast<unsigned short>
									(dt_const.get("portIdLength", 0).asUInt());
							dt.qos_id_length_ = static_cast<unsigned short>
									(dt_const.get("qosIdLength", 0).asUInt());
							dt.sequence_number_length_ = static_cast<unsigned short>
									(dt_const.get
									 ("sequenceNumberLength", 0).asUInt());
							props.dataTransferConstants = dt;
					}
					else
	                	throw Exception("dataTransferConstants parameter can not be empty");

					// QoS cubes
                	Json::Value cubes = dif_configs[i]["qosCubes"];
                	if (cubes.empty())
	                	throw Exception("qosCubes parameter can not be empty");
					for (unsigned int j = 0; j < cubes.size(); j++) {
							// FIXME: Probably should have good default values
							// Check default constructor
							rina::QoSCube cube;

							cube.id_ = cubes[j].get("id", 0).asUInt();
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
							cube.delay_ = cubes[j].get("delay", cube.delay_)
									.asUInt();
							cube.jitter_ = cubes[j].get("jitter", cube.jitter_)
									.asUInt();

							props.qosCubes.push_back(cube);
					}

					// rmtConfiguration;
					Json::Value rmt_conf = dif_configs[i]["rmtConfiguration"];
					if (rmt_conf != 0) {
							rina::RMTConfiguration rc;

							parse_policy(rmt_conf,
										 "rmtQueueMonitorPolicy",
										 rc.rmt_queue_monitor_policy_);

							parse_policy(rmt_conf,
										 "rmtSchedulingPolicy",
										 rc.rmt_scheduling_policy_);

							parse_policy(rmt_conf,
										 "maxQueuePolicy",
										 rc.max_queue_policy_);

							props.rmtConfiguration = rc;
					}

					// std::map<std::string, std::string> policies;
					Json::Value policies = dif_configs[i]["policies"];
					if (policies != 0) {
							Json::Value::Members members =
									policies.getMemberNames();
							for (unsigned int j = 0; j < members.size(); j++) {
									string value = policies.get
											(members[i], string()).asString();
									props.policies.insert
											(pair<string, string>
											 (members[i], value));
							}
					}

					// std::map<std::string, std::string> policyParameters;
					Json::Value policyParams = dif_configs[i]["policyParameters"];
					if (policyParams != 0) {
							Json::Value::Members members =
									policyParams.getMemberNames();
							for (unsigned int j = 0; j < members.size(); j++) {
									string value = policyParams.get
											(members[i], string()).asString();
									props.policyParameters.insert
											(pair<string, string>
											 (members[i], value));
							}
					}

					// NMinusOneFlowsConfiguration
					//       nMinusOneFlowsConfiguration;

					Json::Value flow_conf =
							dif_configs[i]["nMinusOneFlowsConfiguration"];
					if (flow_conf != 0) {
							rinad::NMinusOneFlowsConfiguration fc;

							fc.managementFlowQoSId =
									flow_conf.get("managementFlowQosId",
												  fc.managementFlowQoSId)
									.asInt();

							Json::Value data_flow = flow_conf["dataFlowsQosIds"];
							for (unsigned int j = 0; j < data_flow.size(); j++) {
									fc.dataFlowsQoSIds.push_back
											(data_flow[j].asInt());
							}

							props.nMinusOneFlowsConfiguration = fc;
					}

					// std::list<ExpectedApplicationRegistration>
					// expectedApplicationRegistrations;
					Json::Value exp_app =
							dif_configs[i]["expectedApplicationRegistrations"];
					if (exp_app != 0) {
							for (unsigned int j = 0; j < exp_app.size(); j++) {
									rinad::ExpectedApplicationRegistration exp;

									exp.applicationProcessName =
											exp_app[j]
											.get("apName",
												 string())
											.asString();

									exp.applicationProcessInstance =
											exp_app[j]
											.get("apInstance",
												 string())
											.asString();

									exp.applicationEntityName =
											exp_app[j]
											.get("aeName",
												 string())
											.asString();

									exp.socketPortNumber =
											exp_app[j]
											.get("socketPortNumber",
												 exp.socketPortNumber)
											.asInt();

									props.expectedApplicationRegistrations
											.push_back(exp);
							}
					}

					// std::list<DirectoryEntry> directory;
					Json::Value dir = dif_configs[i]["directory"];
					if (dir != 0) {
							for (unsigned int j = 0; j < dir.size(); j++) {
									rinad::DirectoryEntry de;

									de.applicationProcessName =
											dir[j].get("apName",
													string())
											.asString();

									de.applicationProcessInstance =
											dir[j].get("apInstance",
													   string())
											.asString();

									de.applicationEntityName =
											dir[j].get("aeName",
													   string())
											.asString();

									de.hostname =
											dir[j].get("hostname",
													   string())
											.asString();

									de.socketPortNumber =
											dir[j].get("socketPortNumber",
													   de.socketPortNumber)
											.asInt();

									props.directory.push_back(de);
							}
					}

					// std::list<KnownIPCProcessAddress>
					// knownIPCProcessAddresses;

					Json::Value known = dif_configs[i]["knownIPCProcessAddresses"];
					if (known != 0) {
							for (unsigned int j = 0; j < known.size(); j++) {
									rinad::KnownIPCProcessAddress kn;

									parse_name(known[j], kn.name);

									kn.address =
											known[j].get("address",
														 kn.address).asUInt();

									props.knownIPCProcessAddresses.push_back(kn);
							}
					}
					else
	                	throw Exception("knownIPCProcessAddresses parameter can not be empty");

					// rina::PDUFTableGeneratorConfiguration
					// pdufTableGeneratorConfiguration;
					Json::Value pft =
							dif_configs[i]["pdufTableGeneratorConfiguration"];
					if (pft != 0) {
							rina::PDUFTableGeneratorConfiguration pf;

							parse_policy(pft, "pduFtGeneratorPolicy",
										 pf.pduft_generator_policy_);

							Json::Value lsr_config = pft["linkStateRoutingConfiguration"];

							rina::LinkStateRoutingConfiguration lsr;

							lsr.object_maximum_age_ =
									lsr_config.get("objectMaximumAge",
											lsr.object_maximum_age_)
									.asInt();

							lsr.wait_until_read_cdap_ =
									lsr_config.get("waitUntilReadCdap",
											lsr.wait_until_read_cdap_)
									.asInt();

							lsr.wait_until_error_ =
									lsr_config.get("waitUntilError",
											lsr.wait_until_error_)
									.asInt();

							lsr.wait_until_pduft_computation_ =
									lsr_config.get("waitUntilPduftComputation",
											lsr.wait_until_pduft_computation_)
									.asInt();

							lsr.wait_until_fsodb_propagation_ =
									lsr_config.get("waitUntilFsodbPropagation",
											lsr.wait_until_fsodb_propagation_)
									.asInt();

							lsr.wait_until_age_increment_ =
									lsr_config.get("waitUntilAgeIncrement",
											lsr.wait_until_age_increment_)
									.asInt();

							lsr.routing_algorithm_ =
									lsr_config.get("routingAlgorithm",
											string())
									.asString();

							pf.link_state_routing_configuration_ = lsr;

							props.pdufTableGeneratorConfiguration = pf;
					}
					else
	                	throw Exception("pdufTableGeneratorConfiguration parameter can not be empty");

					// std::list<AddressPrefixConfiguration> addressPrefixes;
					Json::Value addrp = dif_configs[i]["addressPrefixes"];
					if (addrp != 0) {
							for (unsigned int j = 0; j < addrp.size(); j++) {
									AddressPrefixConfiguration apc;

									apc.addressPrefix =
											addrp[j].get("addressPrefix",
														 apc.addressPrefix)
											.asUInt();

									apc.organization =
											addrp[j].get("organization",
														 string())
											.asString();

									props.addressPrefixes.push_back(apc);
							}
					}
					else
	                	throw Exception("pdufTableGeneratorConfiguration parameter can not be empty");
                }
                // configParameters;
                Json::Value confParams = dif_configs[i]["configParameters"];
				if (confParams != 0) {
						Json::Value::Members members =
								confParams.getMemberNames();
					for (unsigned int j = 0; j < members.size(); j++) {
								string value = confParams.get
										(members[j], string()).asString();
								props.configParameters.insert
										(pair<string, string>
										 (members[j], value));
					}
				}
                difConfigurations.push_back(props);
        }
}

void parse_ipc_to_create(const Json::Value         root,
                         list<IPCProcessToCreate> &ipcProcessesToCreate)
{
        Json::Value ipc_processes = root["ipcProcessesToCreate"];
        for (unsigned int i = 0; i < ipc_processes.size(); i++) {

                rinad::IPCProcessToCreate ipc;

                ipc.type = ipc_processes[i].get("type", string()).asString();

                // IPC process Names
                // Might want to move this to another function
                parse_name(ipc_processes[i], ipc.name);

                ipc.difName = rina::ApplicationProcessNamingInformation
                        (ipc_processes[i].get
                         ("difName", string()).asString(),
                         string());

                //Neighbors
                Json::Value neigh = ipc_processes[i]["neighbors"];
                if (neigh != 0) {
                        for (unsigned int j = 0; j < neigh.size(); j++) {
                                rinad::NeighborData neigh_data;

                                parse_name(neigh[j], neigh_data.apName);

                                neigh_data.difName =
                                        rina::ApplicationProcessNamingInformation
                                        (neigh[j].get
                                         ("difName", string()).asString(),
                                         string());

                                neigh_data.supportingDifName =
                                        rina::ApplicationProcessNamingInformation
                                        (neigh[j].get
                                         ("supportingDifName",
                                          string()).asString(),
                                         string());

                                ipc.neighbors.push_back(neigh_data);

                        }
                }

                // DIFs to register at
                Json::Value difs_reg = ipc_processes[i]["difsToRegisterAt"];
                if (difs_reg != 0) {
                        for (unsigned int j = 0; j < difs_reg.size(); j++) {
                                ipc.difsToRegisterAt.push_back
                                        (rina::ApplicationProcessNamingInformation
                                         (difs_reg[j].asString(),
                                          string()));
                        }
                }

                ipc.hostname = ipc_processes[i].get
                        ("hostName", string()).asString();

                // SDU protection options
                Json::Value sdu_prot = ipc_processes[i]["sduProtectionOptions"];
                if (sdu_prot != 0) {
                        for (unsigned int j = 0; j < sdu_prot.size(); j++) {
                                string key =
                                        sdu_prot[j].get("nMinus1DIFName",
                                                        string()).asString();
                                string value =
                                        sdu_prot[j].get("sduProtectionType",
                                                        string()).asString();
                                ipc.sduProtectionOptions.insert
                                        (pair<string, string>(key, value));
                        }
                }

                // parameters
                Json::Value params = ipc_processes[i]["parameters"];
                if (params != 0) {
                        Json::Value::Members members = params.getMemberNames();
                        for (unsigned int j = 0; j < members.size(); j++) {
                                string value = params.get
                                        (members[i], string()).asString();
                                ipc.parameters.insert
                                        (pair<string, string>
                                         (members[i], value));
                        }
                }

                ipcProcessesToCreate.push_back(ipc);
        }

}

void parse_app_to_dif(const Json::Value &root,
                      std::map<std::string,
                      rina::ApplicationProcessNamingInformation>
                      &applicationToDIFMappings)
{
        Json::Value appToDIF = root["applicationToDIFMappings"];
        if (appToDIF != 0) {
                for (unsigned int i = 0; i < appToDIF.size(); i++) {

                        string encodedAppName =
                                appToDIF[i].get
                                ("encodedAppName", string()).asString();
                        rina::ApplicationProcessNamingInformation difName =
                                rina::ApplicationProcessNamingInformation
                                (appToDIF[i].get
                                 ("difName", string()).asString(),
                                 string());

                        applicationToDIFMappings.insert
                                (pair<string,
                                 rina::ApplicationProcessNamingInformation>
                                 (encodedAppName, difName));
                }
        }
}


void parse_local_conf(const Json::Value &root,
                      rinad::LocalConfiguration &local)
{
        Json::Value local_conf = root["localConfiguration"];
        if (local_conf != 0) {
                local.consolePort
                        = local_conf.get("consolePort",
                                         local.consolePort).asInt();
                local.cdapTimeoutInMs
                        = local_conf.get("cdapTimeoutInMs",
                                         local.cdapTimeoutInMs).asInt();
                local.enrollmentTimeoutInMs
                        = local_conf.get("enrollmentTimeoutInMs",
                                         local.enrollmentTimeoutInMs).asInt();
                local.maxEnrollmentRetries
                        = local_conf.get("maxEnrollmentRetries",
                                         local.maxEnrollmentRetries).asInt();
                local.flowAllocatorTimeoutInMs
                        = local_conf.get("flowAllocatorTimeoutInMs",
                                         local.flowAllocatorTimeoutInMs)
                        .asInt();
                local.watchdogPeriodInMs
                        = local_conf.get("watchdogPeriodInMs",
                                         local.watchdogPeriodInMs).asInt();
                local.declaredDeadIntervalInMs
                        = local_conf.get("declaredDeadIntervalInMs",
                                         local.declaredDeadIntervalInMs)
                        .asInt();
                local.neighborsEnrollerPeriodInMs
                        = local_conf.get("neighborsEnrollerPeriodInMs",
                                         local.neighborsEnrollerPeriodInMs)
                        .asInt();
                local.lengthOfFlowQueues
                        = local_conf.get("lengthOfFlowQueues",
                                         local.lengthOfFlowQueues).asInt();
                local.installationPath
                        = local_conf.get("installationPath",
                                         local.installationPath).asString();
                local.libraryPath
                        = local_conf.get("libraryPath",
                                         local.libraryPath).asString();
        }
}

bool parse_configuration(string file_loc,
                         IPCManager *ipcm)
{
        // General note: Params should be checked before they are used
        // Some can be NULL

        // Parse config file with jsoncpp
        Json::Value root;
        Json::Reader reader;
        ifstream file;

        file.open(file_loc.c_str());
        if(file.fail()) {
                LOG_ERR("Failed to open config file");
                return false;
        }

        if (!reader.parse(file, root, false)) {
                LOG_ERR("Failed to parse configuration");

                // FIXME: Log messages need to take string for this to work
                cout << "Failed to parse JSON" << endl
                          << reader.getFormatedErrorMessages() << endl;

                return false;
        }
        file.close();

        // Get everything in our data structures
        rinad::RINAConfiguration config;
        try {
        parse_local_conf(root, config.local);
        parse_app_to_dif(root, config.applicationToDIFMappings);
        parse_ipc_to_create(root, config.ipcProcessesToCreate);
        parse_dif_configs(root, config.difConfigurations);
        ipcm->config = config;
        }
        catch(Exception &e)
        {
        	LOG_ERR("Wrong configuration: %s", e.what());
        }

        return true;
}

}
