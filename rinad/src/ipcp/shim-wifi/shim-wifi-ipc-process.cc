//
// Implementation of the shim WiFi IPC Process in user space
//
//    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301  USA
//

#include <cstdlib>
#include <sstream>
#include <iostream>
#include <signal.h>
#include <assert.h>

#include "ipcp/shim-wifi/shim-wifi-ipc-process.h"
#include "librina/configuration.h"
#include "ipcp/ipc-process.h"

#define IPCP_MODULE "shim-wifi-ipcp"
#include "ipcp-logging.h"
#define SCAN_INTERVAL 10

namespace rinad {

class ShimWifiScanTask: public rina::TimerTask {
public:
	ShimWifiScanTask(ShimWifiIPCProcessImpl * ipcp_): ipcp(ipcp_) {};
	~ShimWifiScanTask() throw() {};
	void run();

private:
	ShimWifiIPCProcessImpl * ipcp;
};

void ShimWifiScanTask::run() {
	LOG_IPCP_DBG("Scanner task trigerred...");
	ipcp->wpa_conn->scan();
}

//Class ShimWifiIPCPProxy
ShimWifiIPCPProxy::ShimWifiIPCPProxy(unsigned short id,
				     const std::string& type,
				     const rina::ApplicationProcessNamingInformation& name) :
						     rina::IPCProcessProxy(id, 0, 0, type, name)
{
}

//Class ShimWifiIPCProcessImpl
ShimWifiIPCProcessImpl::ShimWifiIPCProcessImpl(const std::string& type_,
				const rina::ApplicationProcessNamingInformation& nm,
				unsigned short id,
				unsigned int ipc_manager_port,
				std::string log_level,
				std::string log_file,
				std::string& folder) : IPCProcess(nm.processName, nm.processInstance),
					       	       LazyIPCProcessImpl(nm, id, ipc_manager_port, log_level, log_file)
{
	type = type_;

	if (type != rina::SHIM_WIFI_IPC_PROCESS_AP &&
			type != rina::SHIM_WIFI_IPC_PROCESS_STA) {
		LOG_IPCP_ERR("Could not create Shim WiFi IPCP, type %s not recognized",
			     type.c_str());
		exit(EXIT_FAILURE);
	}

        try {
                rina::ApplicationProcessNamingInformation naming_info(name_, instance_);
                rina::extendedIPCManager->notifyIPCProcessInitialized(naming_info);
        } catch (rina::Exception &e) {
        		LOG_IPCP_ERR("Problems communicating with IPC Manager: %s. Exiting... ", e.what());
        		exit(EXIT_FAILURE);
        }

	std::string folder_name;
	std::string::size_type pos = folder.rfind("/bin");
	if (pos == std::string::npos) {
		folder_name = ".";
	} else {
		folder_name = folder.substr(0, pos);
	}

        ipcp_proxy = new ShimWifiIPCPProxy(id, type, nm);
        if (type == rina::SHIM_WIFI_IPC_PROCESS_STA) {
        	wpa_conn = new WpaController(this, type, folder_name);
        } else {
        	wpa_conn = 0;
        }

        state = INITIALIZED;

        LOG_IPCP_INFO("Initialized Shim Wifi IPC Process of type %s with name: %s, instance %s, id %hu ",
        	       type.c_str(),
		       name_.c_str(),
		       instance_.c_str(),
		       id);
}

ShimWifiIPCProcessImpl::~ShimWifiIPCProcessImpl()
{
	if(wpa_conn){
		delete wpa_conn;
		wpa_conn = 0;
	}

	if (ipcp_proxy) {
		delete ipcp_proxy;
		ipcp_proxy = 0;
	}
}

unsigned short ShimWifiIPCProcessImpl::get_id() {
	return rina::extendedIPCManager->ipcProcessId;
}

const IPCProcessOperationalState& ShimWifiIPCProcessImpl::get_operational_state() const {
	rina::ScopedLock g(*lock_);
	return state;
}

void ShimWifiIPCProcessImpl::set_operational_state(const IPCProcessOperationalState& operational_state) {
	rina::ScopedLock g(*lock_);
	state = operational_state;
}

rina::DIFInformation& ShimWifiIPCProcessImpl::get_dif_information() {
	rina::ScopedLock g(*lock_);
	return dif_information_;
}

void ShimWifiIPCProcessImpl::set_dif_information(const rina::DIFInformation& dif_information) {
	rina::ScopedLock g(*lock_);
	dif_information_ = dif_information;
}

const std::list<rina::Neighbor> ShimWifiIPCProcessImpl::get_neighbors() const {
	std::list<rina::Neighbor> neighbors;
	return neighbors;
}

unsigned int ShimWifiIPCProcessImpl::get_address() const {
	rina::ScopedLock g(*lock_);
	if (state != ASSIGNED_TO_DIF) {
		return 0;
	}

	return dif_information_.dif_configuration_.address_;
}

unsigned int ShimWifiIPCProcessImpl::get_active_address() {
	return 0;
}

unsigned int ShimWifiIPCProcessImpl::get_old_address() {
	return 0;
}

bool ShimWifiIPCProcessImpl::check_address_is_mine(unsigned int address) {
	return false;
}

void ShimWifiIPCProcessImpl::set_address(unsigned int address) {
}

void ShimWifiIPCProcessImpl::requestPDUFTEDump() {
}

int ShimWifiIPCProcessImpl::dispatchSelectPolicySet(const std::string& path,
                                            	    const std::string& name,
						    bool& got_in_userspace) {
	return 0;
}

void ShimWifiIPCProcessImpl::assign_to_dif_request_handler(const rina::AssignToDIFRequestEvent& event)
{
	rina::ScopedLock g(*lock_);


	if (state != INITIALIZED) {
		LOG_IPCP_ERR("Got a DIF assignment request while not in INITIALIZED state. Current state is: %d",
				state);
		rina::extendedIPCManager->assignToDIFResponse(event, -1);
		return;
	}

	try {
		//Do not register to VLAN in the interfaces, this is a shim Wifi
		dif_information_ = event.difInformation;
		dif_information_.dif_name_.processName = "0";
		dif_information_.dif_type_ = rina::SHIM_ETH_VLAN_IPC_PROCESS;
		ipcp_proxy->assignToDIF(dif_information_, event.sequenceNumber);
		pending_events_.insert(std::pair<unsigned int,
						 rina::AssignToDIFRequestEvent>(event.sequenceNumber, event));
		state = ASSIGN_TO_DIF_IN_PROCESS;
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems sending DIF Assignment request to the kernel: %s", e.what());
		rina::extendedIPCManager->assignToDIFResponse(event, -1);
	}
}

void ShimWifiIPCProcessImpl::push_scan_results(std::string& output){
	int rv;
	rina::MediaReport report;

	report.ipcp_id = get_id();
	report.current_dif_name = dif_information_.dif_name_.toString();
	//FIXME: add proper values here
	report.bs_ipcp_address ="0";

	std::istringstream ss(output);
	std::string line;
	int i = 0;
	while (std::getline(ss, line)) {
		if (i==0) continue;
		++i;
		std::vector<std::string> v;
		//line: bssid/frequency/signal/flags/ssid
		std::string value;
		std::istringstream ss(line);
		while(getline(ss, value, '\t')){
			v.push_back(value);
		}
		rina::BaseStationInfo bs_info;
		bs_info.ipcp_address = v[0];
		bs_info.signal_strength = atoi(v[2].c_str());
		std::map<std::string, rina::MediaDIFInfo>::iterator it =
					report.available_difs.find(v[4]);
		if(it == report.available_difs.end()){
			rina::MediaDIFInfo m_info;
			m_info.dif_name = v[4];
			m_info.security_policies = v[3];
			m_info.available_bs_ipcps.push_back(bs_info);
			report.available_difs[v[4]] = m_info;
		} else {
			it->second.available_bs_ipcps.push_back(bs_info);
		}
	}

	rina::extendedIPCManager->sendMediaReport(report);
}

void ShimWifiIPCProcessImpl::assign_to_dif_response_handler(const rina::AssignToDIFResponseEvent& event)
{
	int rv;
	std::string output;
	rina::ScopedLock g(*lock_);

	if (state == ASSIGNED_TO_DIF ) {
		LOG_IPCP_INFO("Got reply from the Kernel components regarding DIF assignment: %d",
				event.result);
		return;
	}

	if (state != ASSIGN_TO_DIF_IN_PROCESS) {
		LOG_IPCP_ERR("Got a DIF assignment response while not in ASSIGN_TO_DIF_IN_PROCESS state. State is %d ",
				state);
		return;
	}

	std::map<unsigned int, rina::AssignToDIFRequestEvent>::iterator it;
	it = pending_events_.find(event.sequenceNumber);
	if (it == pending_events_.end()) {
		LOG_IPCP_ERR("Couldn't find an Assign to DIF request event associated to the handle %u",
				event.sequenceNumber);
		return;
	}

	rina::AssignToDIFRequestEvent requestEvent = it->second;
	pending_events_.erase(it);
	if (event.result != 0) {
		LOG_IPCP_ERR("The kernel couldn't successfully process the Assign to DIF Request: %d",
				event.result);
		LOG_IPCP_ERR("Could not assign IPC Process to DIF %s",
				it->second.difInformation.dif_name_.processName.c_str());
		state = INITIALIZED;

		try {
			rina::extendedIPCManager->assignToDIFResponse(requestEvent, -1);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
		}

		return;
	}

	LOG_IPCP_DBG("The kernel processed successfully the Assign to DIF request");

	dif_information_ = requestEvent.difInformation;

	//If type is station, initialize WPA supplicant
	if (type == rina::SHIM_WIFI_IPC_PROCESS_STA) {
		std::string if_name;
		std::string prog;
		std::string ctrl_if_path;
		std::list<rina::PolicyParameter>::iterator itt;
		for (itt = dif_information_.dif_configuration_.parameters_.begin();
				itt != dif_information_.dif_configuration_.parameters_.end();
				++itt) {
			if (itt->name_ == "interface-name") {
				if_name = itt->value_;
				break;
			}
		}

		//Launch wpa_supplicant process
		rv = wpa_conn->launch_wpa(if_name);
		assert(rv == 0);

		sleep(5); //This is ugly but we need to wait for hostapd/wpa-supplicant to be initialized

		//Connect to control interface and monitoring interface
		rv == wpa_conn->create_ctrl_connection(if_name);
		assert(rv == 0);

		//Disable networks specified in configuration file to avoid connecting
		rv == wpa_conn->disable_network("all", "all");
		assert(rv == 0);

		//Create scan timer
		ShimWifiScanTask * task = new ShimWifiScanTask(this);
		scanner.scheduleTask(task, SCAN_INTERVAL);
	}

	state = ASSIGNED_TO_DIF;

	try {
		rina::extendedIPCManager->assignToDIFResponse(requestEvent, 0);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
	}
}

void ShimWifiIPCProcessImpl::application_registration_request_handler(const rina::ApplicationRegistrationRequestEvent& event)
{
	rina::ScopedLock g(*lock_);

	if (state != INITIALIZED && state != ASSIGNED_TO_DIF) {
		LOG_IPCP_ERR("Got an application registration request while not in "
				"INITIALIZED or ASSIGNED_TO_DIF state. Current state is: %d",
				state);
		rina::extendedIPCManager->registerApplicationResponse(event, -1);
		return;
	}

	try {
		ipcp_proxy->registerApplication(event.applicationRegistrationInformation.appName,
						event.applicationRegistrationInformation.dafName,
						event.applicationRegistrationInformation.ipcProcessId,
						event.applicationRegistrationInformation.difName,
						event.sequenceNumber);
		pending_app_registration_events.insert(std::pair<unsigned int,
				rina::ApplicationRegistrationRequestEvent>(event.sequenceNumber, event));
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems sending application registration request to the kernel: %s", e.what());
		rina::extendedIPCManager->registerApplicationResponse(event, -1);
	}
}

void ShimWifiIPCProcessImpl::app_reg_response_handler(const rina::IpcmRegisterApplicationResponseEvent& event)
{
	rina::ScopedLock g(*lock_);

	if (state != INITIALIZED && state != ASSIGNED_TO_DIF) {
		LOG_IPCP_ERR("Got a DIF assignment response while not in INITIALIZED "
				"or ASSIGNED_TO_DIF state. State is %d ",
				state);
		return;
	}

	std::map<unsigned int, rina::ApplicationRegistrationRequestEvent>::iterator it;
	it = pending_app_registration_events.find(event.sequenceNumber);
	if (it == pending_app_registration_events.end()) {
		LOG_IPCP_ERR("Couldn't find an Application Registration request event associated to the handle %u",
				event.sequenceNumber);
		return;
	}

	rina::ApplicationRegistrationRequestEvent requestEvent = it->second;
	pending_app_registration_events.erase(it);
	if (event.result != 0) {
		LOG_IPCP_ERR("The kernel couldn't successfully process the Application Registration request: %d",
				event.result);
		LOG_IPCP_ERR("Could not register application to DIF %s",
				it->second.applicationRegistrationInformation.toString().c_str());

		try {
			rina::extendedIPCManager->registerApplicationResponse(requestEvent, -1);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
		}

		return;
	}

	LOG_IPCP_DBG("The kernel processed successfully the register application request");
	try {
		rina::extendedIPCManager->registerApplicationResponse(requestEvent, 0);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
	}
}

void ShimWifiIPCProcessImpl::application_unregistration_handler(const rina::ApplicationUnregistrationRequestEvent& event)
{
	rina::ScopedLock g(*lock_);

	if (state != INITIALIZED && state != ASSIGNED_TO_DIF) {
		LOG_IPCP_ERR("Got an application unregistration request while not in "
				"INITIALIZED or ASSIGNED_TO_DIF state. Current state is: %d",
				state);
		rina::extendedIPCManager->unregisterApplicationResponse(event, -1);
		return;
	}

	try {
		ipcp_proxy->unregisterApplication(event.applicationName,
						  event.DIFName,
						  event.sequenceNumber);
		pending_app_unregistration_events.insert(std::pair<unsigned int,
				rina::ApplicationUnregistrationRequestEvent>(event.sequenceNumber, event));
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems sending application unregistration request to the kernel: %s", e.what());
		rina::extendedIPCManager->unregisterApplicationResponse(event, -1);
	}
}

void ShimWifiIPCProcessImpl::unreg_app_response_handler(const rina::IpcmUnregisterApplicationResponseEvent& event)
{
	rina::ScopedLock g(*lock_);

	if (state != INITIALIZED && state != ASSIGNED_TO_DIF) {
		LOG_IPCP_ERR("Got a DIF assignment response while not in INITIALIZED "
				"or ASSIGNED_TO_DIF state. State is %d ",
				state);
		return;
	}

	std::map<unsigned int, rina::ApplicationUnregistrationRequestEvent>::iterator it;
	it = pending_app_unregistration_events.find(event.sequenceNumber);
	if (it == pending_app_unregistration_events.end()) {
		LOG_IPCP_ERR("Couldn't find an Application Unregistration request event associated to the handle %u",
				event.sequenceNumber);
		return;
	}

	rina::ApplicationUnregistrationRequestEvent requestEvent = it->second;
	pending_app_unregistration_events.erase(it);
	if (event.result != 0) {
		LOG_IPCP_ERR("The kernel couldn't successfully process the Application Unregistration request: %d",
				event.result);
		LOG_IPCP_ERR("Could not unregister application %s from DIF %s",
				it->second.applicationName.getEncodedString().c_str(),
				it->second.DIFName.processName.c_str());

		try {
			rina::extendedIPCManager->unregisterApplicationResponse(requestEvent, -1);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
		}

		return;
	}

	LOG_IPCP_DBG("The kernel processed successfully the unregister application request");
	try {
		rina::extendedIPCManager->unregisterApplicationResponse(requestEvent, 0);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
	}
}

void ShimWifiIPCProcessImpl::flow_allocation_requested_handler(const rina::FlowRequestEvent& event)
{
	rina::ScopedLock g(*lock_);

	if (state != ASSIGNED_TO_DIF) {
		LOG_IPCP_ERR("Got a flow allocation request while not in "
				"ASSIGNED_TO_DIF state. Current state is: %d",
				state);
		rina::extendedIPCManager->allocateFlowRequestResult(event, -1);
		return;
	}

	try {
		ipcp_proxy->allocateFlow(event,
					 event.sequenceNumber);
		pending_flow_allocation_events.insert(std::pair<unsigned int,
				rina::FlowRequestEvent>(event.sequenceNumber, event));
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems sending flow allocation request to the kernel: %s", e.what());
		rina::extendedIPCManager->allocateFlowRequestResult(event, -1);
	}
}

void ShimWifiIPCProcessImpl::ipcm_allocate_flow_request_result_handler(const rina::IpcmAllocateFlowRequestResultEvent& event)
{
	rina::ScopedLock g(*lock_);

	if (state != ASSIGNED_TO_DIF) {
		LOG_IPCP_ERR("Got a flow allocation response while not in  "
				"ASSIGNED_TO_DIF state. State is %d ",
				state);
		return;
	}

	std::map<unsigned int, rina::FlowRequestEvent>::iterator it;
	it = pending_flow_allocation_events.find(event.sequenceNumber);
	if (it == pending_flow_allocation_events.end()) {
		LOG_IPCP_ERR("Couldn't find a flow allocation request event associated to the handle %u",
				event.sequenceNumber);
		return;
	}

	rina::FlowRequestEvent requestEvent = it->second;
	pending_flow_allocation_events.erase(it);
	if (event.result != 0) {
		LOG_IPCP_ERR("The kernel couldn't successfully process the Flow allocation request: %d",
				event.result);
		LOG_IPCP_ERR("Could not allocate flow to %s",
				it->second.remoteApplicationName.getEncodedString().c_str());

		try {
			rina::extendedIPCManager->allocateFlowRequestResult(requestEvent, -1);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
		}

		return;
	}

	LOG_IPCP_DBG("The kernel processed successfully the flow allocation request");
	try {
		rina::extendedIPCManager->allocateFlowRequestResult(requestEvent, 0);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
	}
}

void ShimWifiIPCProcessImpl::allocate_flow_response_handler(const rina::AllocateFlowResponseEvent& event)
{
	rina::ScopedLock g(*lock_);

	if (state != ASSIGNED_TO_DIF) {
		LOG_IPCP_ERR("Got a flow allocation request while not in "
				"ASSIGNED_TO_DIF state. Current state is: %d",
				state);
		return;
	}

	try {
		rina::FlowRequestEvent fre;
		fre.sequenceNumber = event.sequenceNumber;
		ipcp_proxy->allocateFlowResponse(fre,
						 event.result,
						 event.notifySource,
						 event.flowAcceptorIpcProcessId);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems sending flow allocation response to the kernel: %s", e.what());
	}
}

void ShimWifiIPCProcessImpl::flow_deallocation_requested_handler(const rina::FlowDeallocateRequestEvent& event)
{
	rina::ScopedLock g(*lock_);

	if (state != ASSIGNED_TO_DIF) {
		LOG_IPCP_ERR("Got a flow deallocation request while not in "
				"ASSIGNED_TO_DIF state. Current state is: %d",
				state);
		rina::extendedIPCManager->flowDeallocationResult(event.portId, -1);
		return;
	}

	try {
		ipcp_proxy->deallocateFlow(event.portId, event.sequenceNumber);
		pending_flow_deallocation_events.insert(std::pair<unsigned int,
				rina::FlowDeallocateRequestEvent>(event.sequenceNumber, event));
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems sending flow deallocation request to the kernel: %s", e.what());
		rina::extendedIPCManager->flowDeallocationResult(event.portId, -1);
	}
}

void ShimWifiIPCProcessImpl::ipcm_deallocate_flow_response_event_handler(const rina::IpcmDeallocateFlowResponseEvent& event)
{
	rina::ScopedLock g(*lock_);

	if (state != ASSIGNED_TO_DIF) {
		LOG_IPCP_ERR("Got a flow deallocation response while not in  "
				"ASSIGNED_TO_DIF state. State is %d ",
				state);
		return;
	}

	std::map<unsigned int, rina::FlowDeallocateRequestEvent>::iterator it;
	it = pending_flow_deallocation_events.find(event.sequenceNumber);
	if (it == pending_flow_deallocation_events.end()) {
		LOG_IPCP_ERR("Couldn't find a flow deallocation request event associated to the handle %u",
				event.sequenceNumber);
		return;
	}

	rina::FlowDeallocateRequestEvent requestEvent = it->second;
	pending_flow_deallocation_events.erase(it);
	if (event.result != 0) {
		LOG_IPCP_ERR("The kernel couldn't successfully process the Flow deallocation request: %d",
				event.result);
		LOG_IPCP_ERR("Could not deallocate flow at port-id %d",
				it->second.portId);

		try {
			rina::extendedIPCManager->notifyflowDeallocated(requestEvent, -1);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
		}

		return;
	}

	LOG_IPCP_DBG("The kernel processed successfully the flow deallocation request");
	try {
		rina::extendedIPCManager->notifyflowDeallocated(requestEvent, 0);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
	}
}

// This function will attach to the SSID and BSSID specified by the enrollment request event,
// triggering a handover if needed
void ShimWifiIPCProcessImpl::enroll_to_dif_handler(const rina::EnrollToDAFRequestEvent& event)
{
	std::list<rina::Neighbor> neighbors;
	rina::Neighbor ap;
	rina::ScopedLock g(*lock_);
	int rv;

	if (state != ASSIGNED_TO_DIF) {
		LOG_IPCP_ERR("Got a enroll to DIF request while not in  "
				"ASSIGNED_TO_DIF state. State is %d ",
				state);
		try {
			rina::extendedIPCManager->enrollToDIFResponse(event,
								      -1,
								      neighbors,
								      dif_information_);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
		}

		return;
	}

	LOG_IPCP_DBG("Attaching to SSID %s and BSSID %s",
		     event.dafName.processName.c_str(),
		     event.neighborName.processName.c_str());

	//Carry out attachment/re-attachment
	if (type == rina::SHIM_WIFI_IPC_PROCESS_STA) {
		rv = wpa_conn->select_network(event.dafName.processName,
					      event.neighborName.processName);
		if(rv != 0){
			LOG_IPCP_ERR("Could not enroll to DIF %s (BSSID %s)",
					event.dafName.processName.c_str(),
					event.neighborName.processName.c_str());
			try {
				rina::extendedIPCManager->enrollToDIFResponse(event,
									      -1,
									      neighbors,
									      dif_information_);
			} catch (rina::Exception &e) {
				LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s",
						e.what());
			}

			return;
		}
	}

	LOG_IPCP_DBG("Attachment successful!");
	ap.name_ = event.neighborName;
	ap.enrolled_ = true;
	neighbors.push_back(ap);

	try {
		rina::extendedIPCManager->enrollToDIFResponse(event,
							      0,
							      neighbors,
							      dif_information_);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
	}

	return;
}

} //namespace rinad
