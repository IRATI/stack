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
#include <thread>
#define RINA_PREFIX     "manager"
#include <librina/logs.h>
#include <librina/cdap_v2.h>
#include <librina/common.h>
#include "encoders_mad.h"

#include "manager.h"

using namespace std;
using namespace rina;
using namespace rinad;

const std::string Manager::mad_name = "mad";
const std::string Manager::mad_instance = "1";

Application::Application(const string& dif_name_, const string& app_name_,
				const string& app_instance_)
		: 	dif_name(dif_name_),
			app_name(app_name_),
			app_instance(app_instance_) {
}

void Application::applicationRegister() {
	ApplicationRegistrationInformation ari;
	RegisterApplicationResponseEvent *resp;
	unsigned int seqnum;
	IPCEvent *event;

	ari.ipcProcessId = 0;  // This is an application, not an IPC process
	ari.appName = ApplicationProcessNamingInformation(app_name,
								app_instance);
	if (dif_name == string()) {
		ari.applicationRegistrationType =
				rina::APPLICATION_REGISTRATION_ANY_DIF;
	} else {
		ari.applicationRegistrationType =
				rina::APPLICATION_REGISTRATION_SINGLE_DIF;
		ari.difName = ApplicationProcessNamingInformation(
				dif_name, string());
	}

	// Request the registration
	seqnum = ipcManager->requestApplicationRegistration(ari);

	// Wait for the response to come
	for (;;) {
		event = ipcEventProducer->eventWait();
		if (event
				&& event->eventType
						== REGISTER_APPLICATION_RESPONSE_EVENT
				&& event->sequenceNumber == seqnum) {
			break;
		}
	}

	resp = dynamic_cast<RegisterApplicationResponseEvent*>(event);

	// Update librina state
	if (resp->result == 0) {
		ipcManager->commitPendingRegistration(seqnum, resp->DIFName);
	} else {
		ipcManager->withdrawPendingRegistration(seqnum);
		throw ApplicationRegistrationException(
				"Failed to register application");
	}
}

const uint Application::max_buffer_size = 1 << 18;

ConnectionCallback::ConnectionCallback(
		rina::cdap::CDAPProviderInterface **prov) {
	prov_ = prov;
}

void ConnectionCallback::open_connection(
		const rina::cdap_rib::con_handle_t &con,
		const rina::cdap_rib::flags_t &flags, int message_id) {
	(void) flags;
	cdap_rib::res_info_t res;
	res.result_ = 1;
	res.result_reason_ = "Ok";
	std::cout << "open conection request CDAP message received"
			<< std::endl;
	(*prov_)->open_connection_response(con, res, message_id);
	std::cout << "open conection response CDAP message sent" << std::endl;
}

void ConnectionCallback::remote_create_result(
		const rina::cdap_rib::con_handle_t &con,
		const rina::cdap_rib::obj_info_t &obj,
		const rina::cdap_rib::res_info_t &res) {
	(void) con;
	(void) obj;
	std::cout << "Result code is: " << res.result_ << std::endl;
}

void ConnectionCallback::remote_read_result(
		const rina::cdap_rib::con_handle_t &con,
		const rina::cdap_rib::obj_info_t &obj,
		const rina::cdap_rib::res_info_t &res) {
	// decode object value
	// print object value
	(void) con;
	std::cout << "Query Rib operation returned result " << res.result_
			<< std::endl;
	std::string query_rib;
	rinad::mad_manager::encoders::StringEncoder().decode(obj.value_,
								query_rib);
	std::cout << "QueryRIB:" << std::endl << query_rib << std::endl;
}

Manager::Manager(const std::string& dif_name, const std::string& apn,
			const std::string& api)
		: Application(dif_name, apn, api) {
	(void) client_app_reg_;
	cdap_prov_ = 0;
}

Manager::~Manager() {
}

void Manager::run()
{
        applicationRegister();

        for (;;) {
                IPCEvent* event = ipcEventProducer->eventWait();
                rina::FlowInformation flow;
                unsigned int port_id;
                DeallocateFlowResponseEvent *resp = 0;

                if (!event)
                        return;

                switch (event->eventType) {

                        case REGISTER_APPLICATION_RESPONSE_EVENT:
                                ipcManager->commitPendingRegistration(
                                                event->sequenceNumber,
                                                dynamic_cast<RegisterApplicationResponseEvent*>(event)
                                                                ->DIFName);
                                break;

                        case UNREGISTER_APPLICATION_RESPONSE_EVENT:
                                ipcManager->appUnregistrationResult(
                                                event->sequenceNumber,
                                                dynamic_cast<UnregisterApplicationResponseEvent*>(event)
                                                                ->result == 0);
                                break;

                        case FLOW_ALLOCATION_REQUESTED_EVENT:
                                flow =
                                                ipcManager->allocateFlowResponse(
                                                                *dynamic_cast<FlowRequestEvent*>(event),
                                                                0, true);
                                LOG_INFO("New flow allocated [port-id = %d]",
                                         flow.portId);
                                startWorker(flow);
                                break;

                        case FLOW_DEALLOCATED_EVENT:
                                port_id =
                                                dynamic_cast<FlowDeallocatedEvent*>(event)
                                                                ->portId;
                                ipcManager->flowDeallocated(port_id);
                                LOG_INFO("Flow torn down remotely [port-id = %d]",
                                         port_id);
                                break;

                        case DEALLOCATE_FLOW_RESPONSE_EVENT:
                                LOG_INFO("Destroying the flow after time-out");
                                resp =
                                                dynamic_cast<DeallocateFlowResponseEvent*>(event);
                                port_id = resp->portId;

                                ipcManager->flowDeallocationResult(
                                                port_id, resp->result == 0);
                                break;

                        default:
                                LOG_INFO("Server got new event of type %d",
                                         event->eventType);
                                break;
                }
        }
}

void Manager::startWorker(rina::FlowInformation flow)
{
        void (Manager::*server_function)(rina::FlowInformation flow);

	server_function = &Manager::operate;

	thread t(server_function, this, flow);
	t.detach();
}

void Manager::operate(rina::FlowInformation flow)
{
        ConnectionCallback callback(&cdap_prov_);
        std::cout << "cdap_prov created" << std::endl;
        cdap::CDAPProviderFactory::init(2000);
        cdap_prov_ = cdap::CDAPProviderFactory::create(false, &callback);
        // CACEP
        cacep(flow);
        // CREATE IPCP
        createIPCP(flow);
        // QUERY RIB
        queryRIB(flow);
        // FINISH
        cdap::CDAPProviderFactory::destroy(flow.portId);
        delete cdap_prov_;
}

void Manager::cacep(rina::FlowInformation flow)
{
        char buffer[max_sdu_size_in_bytes];
        int bytes_read = ipcManager->readSDU(flow.portId, buffer, max_sdu_size_in_bytes);
        cdap_rib::SerializedObject message;
        message.message_ = buffer;
        message.size_ = bytes_read;
        cdap_prov_->process_message(message, flow.portId);
}

void Manager::createIPCP(rina::FlowInformation flow)
{
        char buffer[max_sdu_size_in_bytes];

        mad_manager::structures::ipcp_config_t ipc_config;
        ipc_config.process_instance = "1";
        ipc_config.process_name = "normal-1.IPCP";
        ipc_config.process_type = "normal-ipc";
        ipc_config.dif_to_assign = "normal.DIF";

        cdap_rib::obj_info_t obj;
        obj.name_ =
                        "root, computingSystemID = 1, processingSystemID=1, kernelApplicationProcess, osApplicationProcess, ipcProcesses, ipcProcessID=2";
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

        cdap_prov_->remote_create(flow.portId, obj, flags, filt);
        std::cout << "create IPC request CDAP message sent" << std::endl;

        int bytes_read = ipcManager->readSDU(flow.portId, buffer,
							max_sdu_size_in_bytes);
        cdap_rib::SerializedObject message;
        message.message_ = buffer;
        message.size_ = bytes_read;
        cdap_prov_->process_message(message, flow.portId);
}

void Manager::queryRIB(rina::FlowInformation flow)
{
        char buffer[max_sdu_size_in_bytes];

	cdap_rib::obj_info_t obj;
	obj.name_ =
			"root, computingSystemID = 1, processingSystemID=1, kernelApplicationProcess, osApplicationProcess, ipcProcesses, ipcProcessID=2, RIBDaemon";
	obj.class_ = "RIBDaemon";
	obj.inst_ = 0;

	cdap_rib::flags_t flags;
	flags.flags_ = cdap_rib::flags_t::NONE_FLAGS;

	cdap_rib::filt_info_t filt;
	filt.filter_ = 0;
	filt.scope_ = 0;

        cdap_prov_->remote_read(flow.portId, obj, flags, filt);
        std::cout << "Read RIBDaemon request CDAP message sent" << std::endl;

        int bytes_read = ipcManager->readSDU(flow.portId, buffer,
							max_sdu_size_in_bytes);
        cdap_rib::SerializedObject message;
        message.message_ = buffer;
        message.size_ = bytes_read;
        cdap_prov_->process_message(message, flow.portId);
}
