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
                // TODO: rate_based_config_

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

                        //TODO: rtx_control_config_


                        dc.initial_sender_inactivity_time_ =
                                dtcp_conf.get
                                ("initialSenderInactivityTime",
                                 dc.initial_sender_inactivity_time_)
                                .asUInt();

                        dc.initial_recvr_inactivity_time_ =
                                dtcp_conf.get("initialRecvrInactivityTime",
                                              dc.initial_recvr_inactivity_time_)
                                .asUInt();

                        parse_policy(dtcp_conf,
                                     "rcvrTimerInactivityPolicy",
                                     dc.rcvr_timer_inactivity_policy_);

                        parse_policy(dtcp_conf,
                                     "senderTimerInactivityPolicy",
                                     dc.sender_timer_inactivity_policy_);

                        parse_policy(dtcp_conf,
                                     "lostControlPduPolicy",
                                     dc.lost_control_pdu_policy_);

                        parse_policy(dtcp_conf,
                                     "rttEstimatorPolicy",
                                     dc.rtt_estimator_policy_);
                }

                // TODO: PolicyConfig

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
        Json::Value dif_configs = root["difConfigurations"];
        for (unsigned int i = 0; i < dif_configs.size(); i++) {
                rinad::DIFProperties props;

                props.difName =
                        rina::ApplicationProcessNamingInformation
                        (dif_configs[i].get("difName", string()).asString(),
                         string());

                props.difType = dif_configs[i].get("difType",
                                                   string()).asString();

                // Data transfer constants
                Json::Value dt_const = dif_configs[i]["difProperties"];
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
                        dt.qos_id_lenght_ = static_cast<unsigned short>
                                (dt_const.get("qosIdLength", 0).asUInt());
                        dt.sequence_number_length_ = static_cast<unsigned short>
                                (dt_const.get
                                 ("sequenceNumberLength", 0).asUInt());
                        props.dataTransferConstants = dt;
                }

                // QoS cubes
                Json::Value cubes = dif_configs[i]["qosCubes"];
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

                // TODO: rina::RMTConfiguration rmtConfiguration;
                // TODO: std::map<std::string, std::string> policies;
                // TODO: std::map<std::string, std::string> policyParameters;
                // TODO: NMinusOneFlowsConfiguration 
                //       nMinusOneFlowsConfiguration;
                // TODO: std::list<ExpectedApplicationRegistration> 
                //       expectedApplicationRegistrations;
                // TODO: std::list<DirectoryEntry> directory;
                // TODO: std::list<KnownIPCProcessAddress> 
                //       knownIPCProcessAddresses;
                // TODO: rina::PDUFTableGeneratorConfiguration 
                //       pdufTableGeneratorConfiguration;
                // TODO: std::list<AddressPrefixConfiguration> addressPrefixes;
                // TODO: std::list<rina::Parameter> configParameters;

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
                rina::ApplicationProcessNamingInformation name;
                name.processName =
                        ipc_processes[i].get("applicationProcessName",
                                             string()).asString();
                name.processInstance =
                        ipc_processes[i].get("applicationProcessInstance",
                                             string()).asString();
                name.entityName =
                        ipc_processes[i].get("applicationEntityName",
                                             string()).asString();
                name.entityInstance =
                        ipc_processes[i].get("applicationEntityInstance",
                                             string()).asString();
                ipc.name = name;

                ipc.difName = rina::ApplicationProcessNamingInformation
                        (ipc_processes[i].get
                         ("difName", string()).asString(),
                         string());

                //Neighbors
                Json::Value neigh = ipc_processes[i]["neighbors"];
                if (neigh != 0) {
                        for (unsigned int j = 0; j < neigh.size(); j++) {
                                rinad::NeighborData neigh_data;

                                rina::ApplicationProcessNamingInformation name2;
                                name2.processName =
                                        neigh[j].get
                                        ("applicationProcessName",
                                         string()).asString();
                                name2.processInstance =
                                        neigh[j].get
                                        ("applicationProcessInstance",
                                         string()).asString();
                                name2.entityName =
                                        neigh[j].get
                                        ("applicationEntityName",
                                         string()).asString();
                                name2.entityInstance =
                                        neigh[j].get
                                        ("applicationEntityInstance",
                                         string()).asString();
                                neigh_data.apName = name2;

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
        parse_local_conf(root, config.local);
        parse_app_to_dif(root, config.applicationToDIFMappings);
        parse_ipc_to_create(root, config.ipcProcessesToCreate);
        parse_dif_configs(root, config.difConfigurations);
        ipcm->config = config;

        return true;
}

}
