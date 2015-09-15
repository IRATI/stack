//
// Manager
//
// Addy Bombeke <addy.bombeke@ugent.be>
// Vincenzo Maffione <v.maffione@nextworks.it>
// Bernat Gastón <bernat.gaston@i2cat.net>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
#include <iostream>

#include <cassert>
#define RINA_PREFIX     "manager"
#include <librina/logs.h>
#include <librina/cdap_v2.h>
#include <librina/common.h>
#include <rinad/ipcm/encoders_mad.h>

#include "manager.h"

using namespace std;
using namespace rina;
using namespace rinad;

const std::string Manager::mad_name = "mad";
const std::string Manager::mad_instance = "1";
const std::string ManagerWorker::IPCP_1 = "/computingSystemID=1/processingSystemID=1/kernelApplicationProcess/osApplicationProcess/ipcProcesses/ipcProcessID=4";
const std::string ManagerWorker::IPCP_2 = "/computingSystemID=1/processingSystemID=1/kernelApplicationProcess/osApplicationProcess/ipcProcesses/ipcProcessID=6";
const std::string ManagerWorker::IPCP_3 = "/computingSystemID=1/processingSystemID=1/kernelApplicationProcess/osApplicationProcess/ipcProcesses/ipcProcessID=4";

void ConnectionCallback::open_connection(
		const rina::cdap_rib::con_handle_t &con,
		const rina::cdap_rib::flags_t &flags, int message_id) {
	cdap_rib::res_info_t res;
	res.code_ = rina::cdap_rib::CDAP_SUCCESS;
	std::cout << "open conection request CDAP message received"
			<< std::endl;
	cdap::getProvider()->send_open_connection_result(con, res, message_id);
	std::cout << "open conection response CDAP message sent to ae" << con.dest_.ae_name_<< std::endl;
}

void ConnectionCallback::remote_create_result(
		const rina::cdap_rib::con_handle_t &con,
		const rina::cdap_rib::obj_info_t &obj,
		const rina::cdap_rib::res_info_t &res) {
	std::cout << "Result code is: " << res.code_ << std::endl;
}

void ConnectionCallback::remote_read_result(
		const rina::cdap_rib::con_handle_t &con,
		const rina::cdap_rib::obj_info_t &obj,
		const rina::cdap_rib::res_info_t &res) {
	// decode object value
	// print object value
	std::cout << "Query Rib operation returned result " << res.code_
			<< std::endl;
	std::string query_rib;
	rinad::mad_manager::encoders::StringEncoder().decode(obj.value_,
								query_rib);
	std::cout << "QueryRIB:" << std::endl << query_rib << std::endl;
}

ManagerWorker::ManagerWorker(rina::ThreadAttributes * threadAttributes,
                             rina::FlowInformation flow,
		     	     unsigned int max_size,
		     	     Server * serv) : ServerWorker(threadAttributes, serv)
{
	flow_ = flow;
	max_sdu_size = max_size;
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
        cdap::init(&callback, false);
        cdap_prov_ = cdap::getProvider();
        // CACEP
        cacep(flow.portId);

        if (flow.remoteAppName.processName == "rina.apps.mad.1") {
        	create_result = createIPCP_1(flow.portId);
        	// QUERY RIB
        	if (create_result)
        	        queryRIB(flow.portId, IPCP_1 + "/ribDaemon");

        }
        if (flow.remoteAppName.processName == "rina.apps.mad.2") {
        	create_result = createIPCP_2(flow.portId);
        	if (create_result)
        		queryRIB(flow.portId, IPCP_2 + "/ribDaemon");

        }
        if (flow.remoteAppName.processName == "rina.apps.mad.3") {
        	create_result = createIPCP_3(flow.portId);
        	if (create_result)
        		queryRIB(flow.portId, IPCP_3 + "/ribDaemon");
        }
}

void ManagerWorker::cacep(int port_id)
{
	char buffer[max_sdu_size_in_bytes];
	int bytes_read = ipcManager->readSDU(port_id, buffer, max_sdu_size_in_bytes);
	cdap_rib::SerializedObject message;
	message.message_ = buffer;
	message.size_ = bytes_read;
	cdap::getProvider()->process_message(message, port_id);
}

bool ManagerWorker::createIPCP_1(int port_id)
{
	char buffer[max_sdu_size_in_bytes];

	mad_manager::structures::ipcp_config_t ipc_config;
	ipc_config.process_instance = "1";
	ipc_config.process_name = "test1.IRATI";
	ipc_config.process_type = "normal-ipc";
	ipc_config.dif_to_assign = "normal.DIF";
	ipc_config.difs_to_register.push_back("400");

	cdap_rib::obj_info_t obj;
	obj.name_ = IPCP_1;
	obj.class_ = "IPCProcess";
	obj.inst_ = 0;
	mad_manager::encoders::IPCPConfigEncoder().encode(ipc_config,
								obj.value_);
	mad_manager::structures::ipcp_config_t object;
	mad_manager::encoders::IPCPConfigEncoder().decode(obj.value_, object);

	cdap_rib::flags_t flags;
	flags.flags_ = cdap_rib::flags_t::NONE_FLAGS;

	cdap_rib::filt_info_t filt;
	filt.filter_ = 0;
	filt.scope_ = 0;

	cdap_prov_->remote_create(port_id, obj, flags, filt);
	std::cout << "create IPC request CDAP message sent to port "
			<< port_id << std::endl;

	try {
		int bytes_read = ipcManager->readSDU(port_id, buffer,
							max_sdu_size_in_bytes);
		cdap_rib::SerializedObject message;
		message.message_ = buffer;
		message.size_ = bytes_read;
		cdap_prov_->process_message(message, port_id);
	} catch (Exception &e) {
		std::cout << "ReadSDUException in createIPCP_1: " << e.what()
				<< std::endl;
		return false;
	}
	return true;
}

bool ManagerWorker::createIPCP_2(int port_id) {
	char buffer[max_sdu_size_in_bytes];

	mad_manager::structures::ipcp_config_t ipc_config;
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
	mad_manager::encoders::IPCPConfigEncoder().encode(ipc_config,
								obj.value_);
	mad_manager::structures::ipcp_config_t object;
	mad_manager::encoders::IPCPConfigEncoder().decode(obj.value_, object);

	cdap_rib::flags_t flags;
	flags.flags_ = cdap_rib::flags_t::NONE_FLAGS;

	cdap_rib::filt_info_t filt;
	filt.filter_ = 0;
	filt.scope_ = 0;

	cdap_prov_->remote_create(port_id, obj, flags, filt);
	std::cout << "create IPC request CDAP message sent to port "
			<< port_id << std::endl;

	try {
		int bytes_read = ipcManager->readSDU(port_id, buffer,
							max_sdu_size_in_bytes);
		cdap_rib::SerializedObject message;
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

	mad_manager::structures::ipcp_config_t ipc_config;
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
	mad_manager::encoders::IPCPConfigEncoder().encode(ipc_config,
								obj.value_);
	mad_manager::structures::ipcp_config_t object;
	mad_manager::encoders::IPCPConfigEncoder().decode(obj.value_, object);

	cdap_rib::flags_t flags;
	flags.flags_ = cdap_rib::flags_t::NONE_FLAGS;

	cdap_rib::filt_info_t filt;
	filt.filter_ = 0;
	filt.scope_ = 0;

	cdap_prov_->remote_create(port_id, obj, flags, filt);
	std::cout << "create IPC request CDAP message sent to port "
			<< port_id << std::endl;

	try {
		int bytes_read = ipcManager->readSDU(port_id, buffer,
							max_sdu_size_in_bytes);
		cdap_rib::SerializedObject message;
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

        cdap_prov_->remote_read(port_id, obj, flags, filt);
        std::cout << "Read RIBDaemon request CDAP message sent" << std::endl;

        int bytes_read = ipcManager->readSDU(port_id,
        				     buffer,
        				     max_sdu_size_in_bytes);
        cdap_rib::SerializedObject message;
        message.message_ = buffer;
        message.size_ = bytes_read;
        cdap_prov_->process_message(message, port_id);
}

Manager::Manager(const std::string& dif_name, const std::string& apn,
			const std::string& api)
		: Server(dif_name, apn, api)
{
}

void Manager::run()
{
	int order = 1;
	std::map<int, rina::FlowInformation> waiting;
        rina::FlowInformation flow;
        applicationRegister();

        for(;;) {
                IPCEvent* event = ipcEventProducer->eventWait();
                int port_id = 0;
                DeallocateFlowResponseEvent *resp = NULL;

                if (!event)
                        return;

                switch (event->eventType) {

                case REGISTER_APPLICATION_RESPONSE_EVENT:
                        ipcManager->commitPendingRegistration(event->sequenceNumber,
                                                             dynamic_cast<RegisterApplicationResponseEvent*>(event)->DIFName);
                        break;

                case UNREGISTER_APPLICATION_RESPONSE_EVENT:
                        ipcManager->appUnregistrationResult(event->sequenceNumber,
                                                            dynamic_cast<UnregisterApplicationResponseEvent*>(event)->result == 0);
                        break;

                case FLOW_ALLOCATION_REQUESTED_EVENT:
                        flow = ipcManager->allocateFlowResponse(*dynamic_cast<FlowRequestEvent*>(event), 0, true);
                        port_id = flow.portId;
                        LOG_INFO("New flow allocated [port-id = %d]", port_id);

                        if (flow.remoteAppName.processName == "rina.apps.mad.1")
                        {
                        	if (waiting.find(1) == waiting.end()){
                        		waiting[1] = flow;
                        	} else {
                        		std::cout << "Error, flow with the same mad already exist: "<< flow.remoteAppName.processName<< std::endl;
                        		ipcManager->requestFlowDeallocation(flow.portId);
                        	}
                        }
                        if (flow.remoteAppName.processName== "rina.apps.mad.2") {
                        	if (waiting.find(2) == waiting.end()) {
                        		waiting[2] = flow;
                        	} else {
                        		std::cout << "Error, flow with the same mad already exist: "<< flow.remoteAppName.processName<< std::endl;
                        		ipcManager->requestFlowDeallocation(flow.portId);
                        	}
                        }
                        if (flow.remoteAppName.processName== "rina.apps.mad.3") {
                        	if (waiting.find(3) == waiting.end()) {
                        		waiting[3] = flow;
                        	} else {
                        		std::cout << "Error, flow with the same mad already exist: "<< flow.remoteAppName.processName<< std::endl;
                        		ipcManager->requestFlowDeallocation(flow.portId);
                        	}
                        }
                       while (waiting.find(order) != waiting.end()) {
                        	startWorker(waiting[order]);
                        	order++;
                        }
                        break;

                case FLOW_DEALLOCATED_EVENT:
                        port_id = dynamic_cast<FlowDeallocatedEvent*>(event)->portId;
                        ipcManager->flowDeallocated(port_id);
                        LOG_INFO("Flow torn down remotely [port-id = %d]", port_id);
                        break;

                case DEALLOCATE_FLOW_RESPONSE_EVENT:
                        LOG_INFO("Destroying the flow after time-out");
                        resp = dynamic_cast<DeallocateFlowResponseEvent*>(event);
                        port_id = resp->portId;

                        ipcManager->flowDeallocationResult(port_id, resp->result == 0);
                        break;

                default:
                        LOG_INFO("Server got new event of type %d",
                                        event->eventType);
                        break;
                }
                sleep(1);
        }
}

ServerWorker * Manager::internal_start_worker(rina::FlowInformation flow)
{
	ThreadAttributes threadAttributes;
	ManagerWorker * worker = new ManagerWorker(&threadAttributes,
						   flow,
						   max_sdu_size_in_bytes,
						   this);
	worker->start();
        worker->detach();
        return worker;
}
