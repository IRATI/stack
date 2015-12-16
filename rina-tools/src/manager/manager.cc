//
// Manager
//
// Addy Bombeke <addy.bombeke@ugent.be>
// Vincenzo Maffione <v.maffione@nextworks.it>
// Bernat Gastón <bernat.gaston@i2cat.net>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//   1. Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//   2. Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
//
#include <iostream>

#include <cassert>
#define RINA_PREFIX     "manager"
#include <librina/logs.h>
#include <librina/cdap_v2.h>
#include <librina/common.h>
#include <rinad/common/configuration.h>
#include <rinad/common/encoder.h>

#include "manager.h"

const std::string Manager::mad_name = "mad";
const std::string Manager::mad_instance = "1";
const std::string ManagerWorker::IPCP_1 =
        "/computingSystemID=1/processingSystemID=1/kernelApplicationProcess/osApplicationProcess/ipcProcesses/ipcProcessID=4";
const std::string ManagerWorker::IPCP_2 =
        "/computingSystemID=1/processingSystemID=1/kernelApplicationProcess/osApplicationProcess/ipcProcesses/ipcProcessID=6";
const std::string ManagerWorker::IPCP_3 =
        "/computingSystemID=1/processingSystemID=1/kernelApplicationProcess/osApplicationProcess/ipcProcesses/ipcProcessID=4";

void ConnectionCallback::open_connection(
        const rina::cdap_rib::con_handle_t &con,
        const rina::cdap::CDAPMessage& message)
{
    rina::cdap_rib::res_info_t res;
    res.code_ = rina::cdap_rib::CDAP_SUCCESS;
    std::cout << "open conection request CDAP message received" << std::endl;
    rina::cdap::getProvider()->send_open_connection_result(con, res,
                                                           message.invoke_id_);
    std::cout << "open conection response CDAP message sent to AE "
              << con.dest_.ae_name_ << std::endl;
}

void ConnectionCallback::remote_create_result(
        const rina::cdap_rib::con_handle_t &con,
        const rina::cdap_rib::obj_info_t &obj,
        const rina::cdap_rib::res_info_t &res, const int invoke_id)
{
    std::cout << "Result code is: " << res.code_ << std::endl;
}

void ConnectionCallback::remote_read_result(
        const rina::cdap_rib::con_handle_t &con,
        const rina::cdap_rib::obj_info_t &obj,
        const rina::cdap_rib::res_info_t &res,
        const rina::cdap_rib::flags_t &flags, const int invoke_id)
{
    // decode object value
    // print object value
    std::cout << "Query Rib operation returned result " << res.code_
              << std::endl;
    std::string query_rib;
    rina::cdap::StringEncoder str_encoder;
    str_encoder.decode(obj.value_, query_rib);
    std::cout << "QueryRIB:" << std::endl << query_rib << std::endl;
}

ManagerWorker::ManagerWorker(rina::ThreadAttributes * threadAttributes,
                             rina::FlowInformation flow, unsigned int max_size,
                             Server * serv)
        : ServerWorker(threadAttributes, serv)
{
    flow_ = flow;
    max_sdu_size = max_size;
    cdap_prov_ = 0;
}

int ManagerWorker::internal_run()
{
    operate(flow_);
    return 0;
}

void ManagerWorker::operate(rina::FlowInformation flow)
{
    bool create_result = false;
    ConnectionCallback callback;
    std::cout << "cdap_prov created" << std::endl;
    rina::cdap_rib::concrete_syntax_t syntax;
    rina::cdap::init(&callback, syntax, false);
    cdap_prov_ = rina::cdap::getProvider();
    // CACEP
    cacep(flow.portId);

    if (flow.remoteAppName.processName == "rina.apps.mad.1")
    {
        create_result = createIPCP_1(flow.portId);
        // QUERY RIB
        /*        	if (create_result)
         queryRIB(flow.portId, IPCP_1 + "/ribDaemon");
         */
    }
    /*        if (flow.remoteAppName.processName == "rina.apps.mad.2") {
     create_result = createIPCP_2(flow.portId);
     if (create_result)
     queryRIB(flow.portId, IPCP_2 + "/ribDaemon");

     }
     if (flow.remoteAppName.processName == "rina.apps.mad.3") {
     create_result = createIPCP_3(flow.portId);
     if (create_result)
     queryRIB(flow.portId, IPCP_3 + "/ribDaemon");
     }
     */
}

void ManagerWorker::cacep(int port_id)
{
    unsigned char *buffer = new unsigned char[max_sdu_size_in_bytes];
    int bytes_read = rina::ipcManager->readSDU(port_id, buffer,
                                               max_sdu_size_in_bytes);
    rina::ser_obj_t message;
    message.message_ = buffer;
    message.size_ = bytes_read;
    rina::cdap::getProvider()->process_message(message, port_id);
}

bool ManagerWorker::createIPCP_1(int port_id)
{
    unsigned char *buffer = new unsigned char[max_sdu_size_in_bytes];

    rinad::configs::ipcp_config_t ipc_config;
    ipc_config.name.processInstance = "1";
    ipc_config.name.processName = "test1.IRATI";

    ipc_config.dif_to_assign.dif_name_.processName = "normal.DIF";
    ipc_config.dif_to_assign.dif_type_ = "normal-ipc";
    ipc_config.dif_to_assign.dif_configuration_.address_ = 16;
    // Data Transfer Constants
    ipc_config.dif_to_assign.dif_configuration_.efcp_configuration_
            .data_transfer_constants_.address_length_ = 2;
    ipc_config.dif_to_assign.dif_configuration_.efcp_configuration_
            .data_transfer_constants_.cep_id_length_ = 2;
    ipc_config.dif_to_assign.dif_configuration_.efcp_configuration_
            .data_transfer_constants_.length_length_ = 2;
    ipc_config.dif_to_assign.dif_configuration_.efcp_configuration_
            .data_transfer_constants_.port_id_length_ = 2;
    ipc_config.dif_to_assign.dif_configuration_.efcp_configuration_
            .data_transfer_constants_.qos_id_length_ = 2;
    ipc_config.dif_to_assign.dif_configuration_.efcp_configuration_
            .data_transfer_constants_.sequence_number_length_ = 4;
    ipc_config.dif_to_assign.dif_configuration_.efcp_configuration_
            .data_transfer_constants_.max_pdu_size_ = 10000;
    ipc_config.dif_to_assign.dif_configuration_.efcp_configuration_
            .data_transfer_constants_.max_pdu_lifetime_ = 60000;
    ipc_config.dif_to_assign.dif_configuration_.efcp_configuration_
            .data_transfer_constants_.ctrl_sequence_number_length_ = 4;
    ipc_config.dif_to_assign.dif_configuration_.efcp_configuration_
            .data_transfer_constants_.rate_length_ = 4;
    ipc_config.dif_to_assign.dif_configuration_.efcp_configuration_
            .data_transfer_constants_.frame_length_ = 4;

    // Qos Cube unreliable with flow control
    rina::QoSCube *unreliable = new rina::QoSCube;
    unreliable->name_ = "unreliablewithflowcontrol";
    unreliable->id_ = 1;
    unreliable->partial_delivery_ = false;
    unreliable->ordered_delivery_ = true;
    unreliable->dtp_config_.dtp_policy_set_.name_ = "default";
    unreliable->dtp_config_.dtp_policy_set_.version_ = "0";
    unreliable->dtp_config_.initial_a_timer_ = 300;
    unreliable->dtp_config_.dtcp_present_ = true;
    unreliable->dtcp_config_.dtcp_policy_set_.name_ = "default";
    unreliable->dtcp_config_.dtcp_policy_set_.version_ = "0";
    unreliable->dtcp_config_.rtx_control_ = false;
    unreliable->dtcp_config_.flow_control_ = true;
    unreliable->dtcp_config_.flow_control_config_.rate_based_ = false;
    unreliable->dtcp_config_.flow_control_config_.window_based_ = true;
    unreliable->dtcp_config_.flow_control_config_.window_based_config_
            .max_closed_window_queue_length_ = 50;
    unreliable->dtcp_config_.flow_control_config_.window_based_config_
            .initial_credit_ = 50;
    ipc_config.dif_to_assign.dif_configuration_.efcp_configuration_.qos_cubes_
            .push_back(unreliable);
    // Qos Cube reliable with flow control
    rina::QoSCube *reliable = new rina::QoSCube;
    reliable->name_ = "reliablewithflowcontrol";
    reliable->id_ = 2;
    reliable->partial_delivery_ = false;
    reliable->ordered_delivery_ = true;
    reliable->max_allowable_gap_ = 0;
    reliable->dtp_config_.dtp_policy_set_.name_ = "default";
    reliable->dtp_config_.dtp_policy_set_.version_ = "0";
    reliable->dtp_config_.initial_a_timer_ = 300;
    reliable->dtp_config_.dtcp_present_ = true;
    reliable->dtcp_config_.dtcp_policy_set_.name_ = "default";
    reliable->dtcp_config_.dtcp_policy_set_.version_ = "0";
    reliable->dtcp_config_.rtx_control_ = true;
    reliable->dtcp_config_.rtx_control_config_.data_rxms_nmax_ = 5;
    reliable->dtcp_config_.rtx_control_config_.initial_rtx_time_ = 1000;
    reliable->dtcp_config_.flow_control_ = true;
    reliable->dtcp_config_.flow_control_config_.rate_based_ = false;
    reliable->dtcp_config_.flow_control_config_.window_based_ = true;
    reliable->dtcp_config_.flow_control_config_.window_based_config_
            .max_closed_window_queue_length_ = 50;
    reliable->dtcp_config_.flow_control_config_.window_based_config_
            .initial_credit_ = 50;
    ipc_config.dif_to_assign.dif_configuration_.efcp_configuration_.qos_cubes_
            .push_back(reliable);
    // Namespace configuration
    rina::StaticIPCProcessAddress addr1;
    addr1.ap_name_ = "test1.IRATI";
    addr1.ap_instance_ = "1";
    addr1.address_ = 16;
    ipc_config.dif_to_assign.dif_configuration_.nsm_configuration_
            .addressing_configuration_.static_address_.push_back(addr1);
    rina::StaticIPCProcessAddress addr2;
    addr2.ap_name_ = "test2.IRATI";
    addr2.ap_instance_ = "1";
    addr2.address_ = 17;
    ipc_config.dif_to_assign.dif_configuration_.nsm_configuration_
            .addressing_configuration_.static_address_.push_back(addr2);
    rina::AddressPrefixConfiguration pref1;
    pref1.address_prefix_ = 0;
    pref1.organization_ = "N.Bourbaki";
    ipc_config.dif_to_assign.dif_configuration_.nsm_configuration_
            .addressing_configuration_.address_prefixes_.push_back(pref1);
    rina::AddressPrefixConfiguration pref2;
    pref2.address_prefix_ = 16;
    pref2.organization_ = "IRATI";
    ipc_config.dif_to_assign.dif_configuration_.nsm_configuration_
            .addressing_configuration_.address_prefixes_.push_back(pref2);
    ipc_config.dif_to_assign.dif_configuration_.nsm_configuration_.policy_set_
            .name_ = "default";
    ipc_config.dif_to_assign.dif_configuration_.nsm_configuration_.policy_set_
            .version_ = "1";
    // RMT configuration
    ipc_config.dif_to_assign.dif_configuration_.rmt_configuration_.pft_conf_
            .policy_set_.name_ = "default";
    ipc_config.dif_to_assign.dif_configuration_.rmt_configuration_.pft_conf_
            .policy_set_.version_ = "0";
    ipc_config.dif_to_assign.dif_configuration_.rmt_configuration_.policy_set_
            .name_ = "default";
    ipc_config.dif_to_assign.dif_configuration_.rmt_configuration_.policy_set_
            .version_ = "1";
    // Enrollment Task configuration
    ipc_config.dif_to_assign.dif_configuration_.et_configuration_.policy_set_
            .name_ = "default";
    ipc_config.dif_to_assign.dif_configuration_.et_configuration_.policy_set_
            .version_ = "1";
    rina::PolicyParameter par1;
    par1.name_ = "enrollTimeoutInMs";
    par1.value_ = "10000";
    ipc_config.dif_to_assign.dif_configuration_.et_configuration_.policy_set_
            .parameters_.push_back(par1);
    rina::PolicyParameter par2;
    par2.name_ = "watchdogPeriodInMs";
    par2.value_ = "30000";
    ipc_config.dif_to_assign.dif_configuration_.et_configuration_.policy_set_
            .parameters_.push_back(par2);
    rina::PolicyParameter par3;
    par3.name_ = "declaredDeadIntervalInMs";
    par3.value_ = "120000";
    ipc_config.dif_to_assign.dif_configuration_.et_configuration_.policy_set_
            .parameters_.push_back(par3);
    rina::PolicyParameter par4;
    par4.name_ = "neighborsEnrollerPeriodInMs";
    par4.value_ = "30000";
    ipc_config.dif_to_assign.dif_configuration_.et_configuration_.policy_set_
            .parameters_.push_back(par4);
    rina::PolicyParameter par5;
    par5.name_ = "maxEnrollmentRetries";
    par5.value_ = "3";
    ipc_config.dif_to_assign.dif_configuration_.et_configuration_.policy_set_
            .parameters_.push_back(par5);
    // Flow allocation configuration
    ipc_config.dif_to_assign.dif_configuration_.fa_configuration_.policy_set_
            .name_ = "default";
    ipc_config.dif_to_assign.dif_configuration_.fa_configuration_.policy_set_
            .version_ = "1";
    // Security manager configuration
    ipc_config.dif_to_assign.dif_configuration_.sm_configuration_.policy_set_
            .name_ = "default";
    ipc_config.dif_to_assign.dif_configuration_.sm_configuration_.policy_set_
            .version_ = "1";
    // Resource allocation configuration
    ipc_config.dif_to_assign.dif_configuration_.ra_configuration_.pduftg_conf_
            .policy_set_.name_ = "default";
    ipc_config.dif_to_assign.dif_configuration_.ra_configuration_.pduftg_conf_
            .policy_set_.version_ = "0";
    // Routing configuration
    ipc_config.dif_to_assign.dif_configuration_.routing_configuration_
            .policy_set_.name_ = "link-state";
    ipc_config.dif_to_assign.dif_configuration_.routing_configuration_
            .policy_set_.version_ = "1";
    par1.name_ = "objectMaximumAge";
    par1.value_ = "10000";
    ipc_config.dif_to_assign.dif_configuration_.routing_configuration_
            .policy_set_.parameters_.push_back(par1);
    par2.name_ = "waitUntilReadCDAP";
    par2.value_ = "5001";
    ipc_config.dif_to_assign.dif_configuration_.routing_configuration_
            .policy_set_.parameters_.push_back(par2);
    par3.name_ = "waitUntilError";
    par3.value_ = "5001";
    ipc_config.dif_to_assign.dif_configuration_.routing_configuration_
            .policy_set_.parameters_.push_back(par3);
    par4.name_ = "waitUntilPDUFTComputation";
    par4.value_ = "103";
    ipc_config.dif_to_assign.dif_configuration_.routing_configuration_
            .policy_set_.parameters_.push_back(par4);
    par5.name_ = "waitUntilFSODBPropagation";
    par5.value_ = "101";
    ipc_config.dif_to_assign.dif_configuration_.routing_configuration_
            .policy_set_.parameters_.push_back(par5);
    rina::PolicyParameter par6;
    par6.name_ = "waitUntilAgeIncrement";
    par6.value_ = "997";
    ipc_config.dif_to_assign.dif_configuration_.routing_configuration_
            .policy_set_.parameters_.push_back(par6);
    rina::PolicyParameter par7;
    par7.name_ = "routingAlgorithm";
    par7.value_ = "Dijkstra";
    ipc_config.dif_to_assign.dif_configuration_.routing_configuration_
            .policy_set_.parameters_.push_back(par7);
    // DIFs to register
    rina::ApplicationProcessNamingInformation dif1;
    dif1.processName = "300";
    ipc_config.difs_to_register.push_back(dif1);

    rina::cdap_rib::obj_info_t obj;
    obj.name_ = IPCP_1;
    obj.class_ = "IPCProcess";
    obj.inst_ = 0;
    rinad::encoders::IPCPConfigEncoder encoder;
    encoder.encode(ipc_config, obj.value_);
    rina::cdap_rib::flags_t flags;
    flags.flags_ = rina::cdap_rib::flags_t::NONE_FLAGS;

    rina::cdap_rib::filt_info_t filt;
    filt.filter_ = 0;
    filt.scope_ = 0;

    rina::cdap_rib::con_handle_t con;
    con.port_id = port_id;

    cdap_prov_->remote_create(con, obj, flags, filt, 28);
    std::cout << "create IPC request CDAP message sent to port " << port_id
              << std::endl;

    try
    {
        int bytes_read = rina::ipcManager->readSDU(port_id, buffer,
                                                   max_sdu_size_in_bytes);
        rina::ser_obj_t message;
        message.message_ = buffer;
        message.size_ = bytes_read;
        cdap_prov_->process_message(message, port_id);
    } catch (rina::Exception &e)
    {
        std::cout << "ReadSDUException in createIPCP_1: " << e.what()
                  << std::endl;
        return false;
    }
    return true;
}
/*
 bool ManagerWorker::createIPCP_2(int port_id) {
 char buffer[max_sdu_size_in_bytes];

 mad_manager::ipcp_config_t ipc_config;
 ipc_config.process_instance = "1";
 ipc_config.process_name = "test2.IRATI";
 ipc_config.process_type = "normal-ipc";
 ipc_config.dif_to_assign = "normal.DIF";
 ipc_config.difs_to_register.push_back("410");
 ipc_config.enr_conf.enr_dif = "normal.DIF";
 ipc_config.enr_conf.enr_un_dif = "400";
 ipc_config.enr_conf.neighbor_name = "test1.IRATI";
 ipc_config.enr_conf.neighbor_instance = "1";

 cdap_rib::obj_info_t obj;
 obj.name_ = IPCP_2;
 obj.class_ = "IPCProcess";
 obj.inst_ = 0;
 mad_manager::IPCPConfigEncoder().encode(ipc_config,
 obj.value_);
 mad_manager::ipcp_config_t object;
 mad_manager::IPCPConfigEncoder().decode(obj.value_, object);

 cdap_rib::flags_t flags;
 flags.flags_ = cdap_rib::flags_t::NONE_FLAGS;

 cdap_rib::filt_info_t filt;
 filt.filter_ = 0;
 filt.scope_ = 0;

 cdap_rib::con_handle_t con;
 con.port_id = port_id;

 cdap_prov_->remote_create(con, obj, flags, filt, 34);
 std::cout << "create IPC request CDAP message sent to port "
 << port_id << std::endl;

 try {
 int bytes_read = ipcManager->readSDU(port_id, buffer,
 max_sdu_size_in_bytes);
 ser_obj_t message;
 message.message_ = buffer;
 message.size_ = bytes_read;
 cdap_prov_->process_message(message, port_id);
 } catch (Exception &e) {
 std::cout << "ReadSDUException in createIPCP_2: " << e.what()
 << std::endl;
 return false;
 }
 return true;
 }

 bool ManagerWorker::createIPCP_3(int port_id) {
 char buffer[max_sdu_size_in_bytes];

 mad_manager::ipcp_config_t ipc_config;
 ipc_config.process_instance = "1";
 ipc_config.process_name = "test3.IRATI";
 ipc_config.process_type = "normal-ipc";
 ipc_config.dif_to_assign = "normal.DIF";
 ipc_config.enr_conf.enr_dif = "normal.DIF";
 ipc_config.enr_conf.enr_un_dif = "410";
 ipc_config.enr_conf.neighbor_name = "test2.IRATI";
 ipc_config.enr_conf.neighbor_instance = "1";

 cdap_rib::obj_info_t obj;
 obj.name_ = IPCP_3;
 obj.class_ = "IPCProcess";
 obj.inst_ = 0;
 mad_manager::IPCPConfigEncoder().encode(ipc_config,
 obj.value_);
 mad_manager::ipcp_config_t object;
 mad_manager::IPCPConfigEncoder().decode(obj.value_, object);

 cdap_rib::flags_t flags;
 flags.flags_ = cdap_rib::flags_t::NONE_FLAGS;

 cdap_rib::filt_info_t filt;
 filt.filter_ = 0;
 filt.scope_ = 0;

 cdap_rib::con_handle_t con;
 con.port_id = port_id;

 cdap_prov_->remote_create(con, obj, flags, filt, 67);
 std::cout << "create IPC request CDAP message sent to port "
 << port_id << std::endl;

 try {
 int bytes_read = ipcManager->readSDU(port_id, buffer,
 max_sdu_size_in_bytes);
 ser_obj_t message;
 message.message_ = buffer;
 message.size_ = bytes_read;
 cdap_prov_->process_message(message, port_id);
 } catch (Exception &e) {
 std::cout << "ReadSDUException in createIPCP 3: " << e.what()
 << std::endl;
 return false;
 }
 return true;
 }

 void ManagerWorker::queryRIB(int port_id, std::string name)
 {
 char buffer[max_sdu_size_in_bytes];

 cdap_rib::obj_info_t obj;
 obj.name_ = name;
 obj.class_ = "RIBDaemon";
 obj.inst_ = 0;

 cdap_rib::flags_t flags;
 flags.flags_ = cdap_rib::flags_t::NONE_FLAGS;

 cdap_rib::filt_info_t filt;
 filt.filter_ = 0;
 filt.scope_ = 0;

 cdap_rib::con_handle_t con;
 con.port_id = port_id;

 cdap_prov_->remote_read(con, obj, flags, filt, 89);
 std::cout << "Read RIBDaemon request CDAP message sent" << std::endl;

 int bytes_read = ipcManager->readSDU(port_id,
 buffer,
 max_sdu_size_in_bytes);
 ser_obj_t message;
 message.message_ = buffer;
 message.size_ = bytes_read;
 cdap_prov_->process_message(message, port_id);
 }
 */
Manager::Manager(const std::string& dif_name, const std::string& apn,
                 const std::string& api)
        : Server(dif_name, apn, api)
{
    client_app_reg_ = false;
}

void Manager::run()
{
    int order = 1;
    std::map<int, rina::FlowInformation> waiting;
    rina::FlowInformation flow;
    applicationRegister();

    for (;;)
    {
        rina::IPCEvent* event = rina::ipcEventProducer->eventWait();
        int port_id = 0;
        rina::DeallocateFlowResponseEvent *resp = NULL;

        if (!event)
            return;

        switch (event->eventType) {

            case rina::REGISTER_APPLICATION_RESPONSE_EVENT:
                rina::ipcManager->commitPendingRegistration(
                        event->sequenceNumber,
                        dynamic_cast<rina::RegisterApplicationResponseEvent*>(event)
                                ->DIFName);
                break;

            case rina::UNREGISTER_APPLICATION_RESPONSE_EVENT:
                rina::ipcManager->appUnregistrationResult(
                        event->sequenceNumber,
                        dynamic_cast<rina::UnregisterApplicationResponseEvent*>(event)
                                ->result == 0);
                break;

            case rina::FLOW_ALLOCATION_REQUESTED_EVENT:
                flow = rina::ipcManager->allocateFlowResponse(
                        *dynamic_cast<rina::FlowRequestEvent*>(event), 0, true);
                port_id = flow.portId;
                LOG_INFO("New flow allocated [port-id = %d]", port_id);

                if (flow.remoteAppName.processName == "rina.apps.mad.1")
                {
                    if (waiting.find(1) == waiting.end())
                    {
                        waiting[1] = flow;
                    } else
                    {
                        std::cout
                                << "Error, flow with the same mad already exist: "
                                << flow.remoteAppName.processName << std::endl;
                        rina::ipcManager->requestFlowDeallocation(flow.portId);
                    }
                }
                if (flow.remoteAppName.processName == "rina.apps.mad.2")
                {
                    if (waiting.find(2) == waiting.end())
                    {
                        waiting[2] = flow;
                    } else
                    {
                        std::cout
                                << "Error, flow with the same mad already exist: "
                                << flow.remoteAppName.processName << std::endl;
                        rina::ipcManager->requestFlowDeallocation(flow.portId);
                    }
                }
                if (flow.remoteAppName.processName == "rina.apps.mad.3")
                {
                    if (waiting.find(3) == waiting.end())
                    {
                        waiting[3] = flow;
                    } else
                    {
                        std::cout
                                << "Error, flow with the same mad already exist: "
                                << flow.remoteAppName.processName << std::endl;
                        rina::ipcManager->requestFlowDeallocation(flow.portId);
                    }
                }
                while (waiting.find(order) != waiting.end())
                {
                    startWorker(waiting[order]);
                    order++;
                }
                break;

            case rina::FLOW_DEALLOCATED_EVENT:
                port_id = dynamic_cast<rina::FlowDeallocatedEvent*>(event)
                        ->portId;
                rina::ipcManager->flowDeallocated(port_id);
                LOG_INFO("Flow torn down remotely [port-id = %d]", port_id);
                break;

            case rina::DEALLOCATE_FLOW_RESPONSE_EVENT:
                LOG_INFO("Destroying the flow after time-out");
                resp = dynamic_cast<rina::DeallocateFlowResponseEvent*>(event);
                port_id = resp->portId;

                rina::ipcManager->flowDeallocationResult(port_id,
                                                         resp->result == 0);
                break;

            default:
                LOG_INFO("Server got new event of type %d", event->eventType);
                break;
        }
        sleep(1);
    }
}

ServerWorker * Manager::internal_start_worker(rina::FlowInformation flow)
{
    rina::ThreadAttributes threadAttributes;
    ManagerWorker * worker = new ManagerWorker(&threadAttributes, flow,
                                               max_sdu_size_in_bytes, this);
    worker->start();
    worker->detach();
    return worker;
}
