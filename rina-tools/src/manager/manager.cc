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

class ConnectionCallback : public rina::cdap::CDAPCallbackInterface
{
 public:
	ConnectionCallback(rina::cdap::CDAPProviderInterface **prov);
	void open_connection(const rina::cdap_rib::con_handle_t &con,
				const rina::cdap_rib::flags_t &flags,
				int message_id);
	void remote_create_result(const rina::cdap_rib::con_handle_t &con,
					const rina::cdap_rib::obj_info_t &obj,
					const rina::cdap_rib::res_info_t &res);
	void remote_read_result(const rina::cdap_rib::con_handle_t &con,
				const rina::cdap_rib::obj_info_t &obj,
				const rina::cdap_rib::res_info_t &res);
 private:
	rina::cdap::CDAPProviderInterface **prov_;
};

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
	std::cout << "open conection request CDAP message received from app "
			<< con.src_.ap_name_ << std::endl;
	(*prov_)->open_connection_response(con, res, message_id);
	std::cout << "open conection response CDAP message sent to app "
			<< con.src_.ap_name_ << std::endl;
}

void ConnectionCallback::remote_create_result(
		const rina::cdap_rib::con_handle_t &con,
		const rina::cdap_rib::obj_info_t &obj,
		const rina::cdap_rib::res_info_t &res) {
	(void) obj;
	std::cout << "Remote create result from port " << con.port_
			<< ". Result code is: " << res.result_ << std::endl;
}

void ConnectionCallback::remote_read_result(
		const rina::cdap_rib::con_handle_t &con,
		const rina::cdap_rib::obj_info_t &obj,
		const rina::cdap_rib::res_info_t &res) {
	// decode object value
	// print object value
	std::cout << "Query Rib operation from port " << con.port_
			<< " returned result " << res.result_ << std::endl;
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
	if (cdap_prov_)
		delete cdap_prov_;
}

void Manager::run() {
	applicationRegister();
	int order = 1;
	std::map<int, rina::FlowInformation> waiting;
	ConnectionCallback callback(&cdap_prov_);
	cdap::CDAPProviderFactory::init(2000);
	cdap_prov_ = cdap::CDAPProviderFactory::create(false, &callback);

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
				LOG_INFO("New flow allocated to app %s [port-id = %d]",
						flow.remoteAppName.processName.c_str(), flow.portId);
				cacep(flow);
				if (flow.remoteAppName.processName
						== "rina.apps.mad.1") {
					if (waiting.find(1) == waiting.end()) {
						waiting[1] = flow;
					} else {
						std::cout << "Error, flow with the same mad already exist: "
								<< flow
										.remoteAppName
										.processName
								<< std::endl;
						ipcManager
								->requestFlowDeallocation(
								flow.portId);
					}
				}
				if (flow.remoteAppName.processName
						== "rina.apps.mad.2") {
					if (waiting.find(2) == waiting.end()) {
						waiting[2] = flow;
					} else {
						std::cout << "Error, flow with the same mad already exist: "
								<< flow
										.remoteAppName
										.processName
								<< std::endl;
						ipcManager
								->requestFlowDeallocation(
								flow.portId);
					}
				}
				if (flow.remoteAppName.processName
						== "rina.apps.mad.3") {
					if (waiting.find(3) == waiting.end()) {
						waiting[3] = flow;
					} else {
						std::cout << "Error, flow with the same mad already exist: "
								<< flow
										.remoteAppName
										.processName
								<< std::endl;
						ipcManager
								->requestFlowDeallocation(
								flow.portId);
					}
				}
				while (waiting.find(order) != waiting.end()) {
					order++;
					operate(flow);
				}

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
	// FINISH
	//cdap::CDAPProviderFactory::destroy(flow.portId);
}

void Manager::operate(rina::FlowInformation flow) {
	bool create_result = false;
	// CREATE IPCP
	std::cout << "flow.remoteAppName.processName: "
			<< flow.remoteAppName.processName << std::endl;
	if (flow.remoteAppName.processName == "rina.apps.mad.1") {
		create_result = createIPCP_1(flow);
		// QUERY RIB
		if (create_result)
			queryRIB(flow, IPCP_1 + ",RIBDaemon");

	}
	if (flow.remoteAppName.processName == "rina.apps.mad.2") {
		create_result = createIPCP_2(flow);
		if (create_result)
			queryRIB(flow, IPCP_2 + ", RIBDaemon");

	}
	if (flow.remoteAppName.processName == "rina.apps.mad.3") {
		create_result = createIPCP_3(flow);
		if (create_result)
			queryRIB(flow, IPCP_3 + ", RIBDaemon");
	}
}

bool Manager::cacep(rina::FlowInformation &flow) {
	char buffer[max_sdu_size_in_bytes];
	try {
		int bytes_read = ipcManager->readSDU(flow.portId, buffer,
							max_sdu_size_in_bytes);
		cdap_rib::SerializedObject message;
		message.message_ = buffer;
		message.size_ = bytes_read;
		cdap_prov_->process_message(message, flow.portId);
	} catch (Exception &e) {
		std::cout << "ReadSDUException in CACEP: " << e.what()
				<< std::endl;
		return false;
	}
	return true;
}

bool Manager::createIPCP_1(rina::FlowInformation &flow) {
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

	cdap_prov_->remote_create(flow.portId, obj, flags, filt);
	std::cout << "create IPC request CDAP message sent to app "
			<< flow.remoteAppName.processName << std::endl;

	try {
		int bytes_read = ipcManager->readSDU(flow.portId, buffer,
							max_sdu_size_in_bytes);
		cdap_rib::SerializedObject message;
		message.message_ = buffer;
		message.size_ = bytes_read;
		cdap_prov_->process_message(message, flow.portId);
	} catch (Exception &e) {
		std::cout << "ReadSDUException in createIPCP_1: " << e.what()
				<< std::endl;
		return false;
	}
	return true;
}

bool Manager::createIPCP_2(rina::FlowInformation &flow) {
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

	cdap_prov_->remote_create(flow.portId, obj, flags, filt);
	std::cout << "create IPC request CDAP message sent to app "
			<< flow.remoteAppName.processName << std::endl;

	try {
		int bytes_read = ipcManager->readSDU(flow.portId, buffer,
							max_sdu_size_in_bytes);
		cdap_rib::SerializedObject message;
		message.message_ = buffer;
		message.size_ = bytes_read;
		cdap_prov_->process_message(message, flow.portId);
	} catch (Exception &e) {
		std::cout << "ReadSDUException in createIPCP_2: " << e.what()
				<< std::endl;
		return false;
	}
	return true;
}

bool Manager::createIPCP_3(rina::FlowInformation &flow) {
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

	cdap_prov_->remote_create(flow.portId, obj, flags, filt);
	std::cout << "create IPC request CDAP message sent to app "
			<< flow.remoteAppName.processName << std::endl;

	try {
		int bytes_read = ipcManager->readSDU(flow.portId, buffer,
							max_sdu_size_in_bytes);
		cdap_rib::SerializedObject message;
		message.message_ = buffer;
		message.size_ = bytes_read;
		cdap_prov_->process_message(message, flow.portId);
	} catch (Exception &e) {
		std::cout << "ReadSDUException in createIPCP 3: " << e.what()
				<< std::endl;
		return false;
	}
	return true;
}

void Manager::queryRIB(rina::FlowInformation &flow, std::string name) {
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

	cdap_prov_->remote_read(flow.portId, obj, flags, filt);
	std::cout << "Read RIBDaemon request CDAP message sent to port "
			<< flow.portId << std::endl;

	try {
		int bytes_read = ipcManager->readSDU(flow.portId, buffer,
							max_sdu_size_in_bytes);
		cdap_rib::SerializedObject message;
		message.message_ = buffer;
		message.size_ = bytes_read;
		cdap_prov_->process_message(message, flow.portId);
	} catch (Exception &e) {
		std::cout << "ReadSDUException in queryRIB: " << e.what()
				<< std::endl;
	}
}
