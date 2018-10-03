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

namespace rinad {

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

        ipcp_proxy = new ShimWifiIPCPProxy(id, type, nm);

        state = INITIALIZED;

        LOG_IPCP_INFO("Initialized Shim Wifi IPC Process of type %s with name: %s, instance %s, id %hu ",
        	       type.c_str(),
		       name_.c_str(),
		       instance_.c_str(),
		       id);
}

ShimWifiIPCProcessImpl::~ShimWifiIPCProcessImpl()
{
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
		return;
	}

	try {
		ipcp_proxy->deallocateFlow(event.portId, event.sequenceNumber);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems sending flow deallocation request to the kernel: %s", e.what());
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

	LOG_IPCP_DBG("Trying to enroll to SSID %s and BSSID %s",
		     event.dafName.processName.c_str(),
		     event.neighborName.processName.c_str());

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

void ShimWifiIPCProcessImpl::ipcp_scan_media_request_event_handler(rina::ScanMediaRequestEvent& event)
{
	//Ignore
}

//Class StaEnrollmentSM
StaEnrollmentSM::StaEnrollmentSM(const std::string& dif_name,
				 const std::string neighbor)
{
	restart(dif_name, neighbor);
}

StaEnrollmentSM::StaEnrollmentSM()
{
	restart("", "");
}

void StaEnrollmentSM::restart(const std::string& dif,
			      const std::string neigh)
{
	dif_name = dif;
	neighbor = neigh;
	state = StaEnrollmentSM::DISCONNECTED;
	enrollment_start_time_ms = rina::Time::get_time_in_ms();
	enrollment_end_time_ms = 0;
}

std::string StaEnrollmentSM::state_to_string()
{
	switch(state) {
	case DISCONNECTED:
		return "DISCONNECTED";
	case ENROLLMENT_STARTED:
		return "ENROLLMENT STARTED";
	case TRYING_TO_ASSOCIATE:
		return "TRYING TO ASSOCIATE";
	case ASSOCIATED:
		return "ASSOCIATED";
	case KEY_NEGOTIATION_COMPLETED:
		return "KEY NEGOTIATION COMPLETED";
	case ENROLLED:
		return "ENROLLED";
	default:
		return "Unknown state";
	}
}

std::string StaEnrollmentSM::to_string()
{
	std::stringstream ss;

	ss << "DIF name: " << dif_name
	   << "; Neighbor name: " << neighbor
	   << "; State: " << state_to_string()
	   << "; Enrollment time (ms): "
	   << (enrollment_end_time_ms - enrollment_start_time_ms)
	   << std::endl;

	return ss.str();
}

//Class CancelEnrollmentTimerTask
class CancelEnrollmentTimerTask: public rina::TimerTask {
public:
	CancelEnrollmentTimerTask(ShimWifiStaIPCProcessImpl * ipcp_): ipcp(ipcp_) {};
	~CancelEnrollmentTimerTask() throw() {};
	void run();
	std::string name() const {
		return "cancel-enrollment";
	}

private:
	ShimWifiStaIPCProcessImpl * ipcp;
};

void CancelEnrollmentTimerTask::run()
{
	ipcp->notify_cancel_enrollment();
}

//Class ShimWifiStaIPCProcessImpl
const long ShimWifiStaIPCProcessImpl::DEFAULT_ENROLLMENT_TIMEOUT_MS = 10000;

ShimWifiStaIPCProcessImpl::ShimWifiStaIPCProcessImpl(const rina::ApplicationProcessNamingInformation& name,
		          	  	  	     unsigned short id,
						     unsigned int ipc_manager_port,
						     std::string log_level,
						     std::string log_file,
						     std::string& folder) :
		ShimWifiIPCProcessImpl(rina::SHIM_WIFI_IPC_PROCESS_STA,
				       name, id, ipc_manager_port,
				       log_level, log_file, folder)
{
	std::string folder_name;

	std::string::size_type pos = folder.rfind("/bin");
	if (pos == std::string::npos) {
		folder_name = ".";
	} else {
		folder_name = folder.substr(0, pos);
	}

	scan_period_ms = 0;
	wpa_conn = new WpaController(this, type, folder_name);
	timer_task = 0;
	enrollment_timeout = DEFAULT_ENROLLMENT_TIMEOUT_MS;
}

ShimWifiStaIPCProcessImpl::~ShimWifiStaIPCProcessImpl()
{
	if(wpa_conn){
		delete wpa_conn;
		wpa_conn = 0;
	}
}

void ShimWifiStaIPCProcessImpl::assign_to_dif_response_handler(const rina::AssignToDIFResponseEvent& event)
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

	std::string if_name;
	std::string driver = "nl80211";
	std::string prog;
	std::string ctrl_if_path;
	std::list<rina::PolicyParameter>::iterator itt;
	for (itt = dif_information_.dif_configuration_.parameters_.begin();
			itt != dif_information_.dif_configuration_.parameters_.end();
			++itt) {
		if (itt->name_ == "interface-name") {
			if_name = itt->value_;
		} else if (itt->name_ == "wpas-driver") {
			driver = itt->value_;
		}
	}

	//Launch wpa_supplicant process
	rv = wpa_conn->launch_wpa(if_name, driver);
	if (rv != 0) {
		LOG_IPCP_ERR("Problems launching WPA Supplicant");
		state = INITIALIZED;

		try {
			rina::extendedIPCManager->assignToDIFResponse(requestEvent, -1);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
		}

		return;
	}

	sleep(5); //This is ugly but we need to wait for hostapd/wpa-supplicant to be initialized

	//Connect to control interface and monitoring interface
	rv = wpa_conn->create_ctrl_connection(if_name);
	if (rv != 0) {
		LOG_IPCP_ERR("Problems connecting to WPA Supplicant control interface");
		state = INITIALIZED;

		try {
			rina::extendedIPCManager->assignToDIFResponse(requestEvent, -1);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
		}

		return;
	}

	//Disable networks specified in configuration file to avoid connecting
	rv = wpa_conn->disable_network("all", "all");
	if (rv != 0) {
		LOG_IPCP_ERR("Problems disabling all networks");
		state = INITIALIZED;

		try {
			rina::extendedIPCManager->assignToDIFResponse(requestEvent, -1);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
		}

		return;
	}

	state = ASSIGNED_TO_DIF;

	try {
		rina::extendedIPCManager->assignToDIFResponse(requestEvent, 0);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
	}
}

// This function will attach to the SSID and BSSID specified by the enrollment request event,
// triggering a handover if needed
void ShimWifiStaIPCProcessImpl::enroll_to_dif_handler(const rina::EnrollToDAFRequestEvent& event)
{
	std::list<rina::Neighbor> neighbors;
	int rv;

	rina::ScopedLock g(*lock_);

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

	if (sta_enr_sm.state != StaEnrollmentSM::DISCONNECTED &&
			sta_enr_sm.state != StaEnrollmentSM::ENROLLED) {
		LOG_IPCP_ERR("Another enrollment operation is already in progress \n %s",
			     sta_enr_sm.to_string().c_str());
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

	if (sta_enr_sm.state == StaEnrollmentSM::ENROLLED &&
			sta_enr_sm.neighbor == event.neighborName.processName) {
		LOG_IPCP_WARN("Already enrolled to BSSID %s", sta_enr_sm.neighbor.c_str());
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

	LOG_IPCP_DBG("Trying to enroll to SSID %s and BSSID %s",
		     event.dafName.processName.c_str(),
		     event.neighborName.processName.c_str());

	//Carry out attachment/re-attachment
	if (event.dafName.processName == sta_enr_sm.dif_name &&
			sta_enr_sm.state == StaEnrollmentSM::ENROLLED) {
		//Attaching to another BSSID within the same SSID
		rv = wpa_conn->bssid_reassociate(event.dafName.processName,
						 event.neighborName.processName);
	} else {
		//Joining BSSID for the first time
		rv = wpa_conn->select_network(event.dafName.processName,
					      event.neighborName.processName);
	}

	if (rv != 0){
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

	LOG_IPCP_DBG("Enrollment in process!");
	sta_enr_sm.restart(event.dafName.processName,
			   event.neighborName.processName);
	sta_enr_sm.state = StaEnrollmentSM::ENROLLMENT_STARTED;
	sta_enr_sm.enroll_event = event;
	timer_task = new CancelEnrollmentTimerTask(this);
	timer.scheduleTask(timer_task, enrollment_timeout);
}

void ShimWifiStaIPCProcessImpl::disconnet_neighbor_handler(const rina::DisconnectNeighborRequestEvent& event)
{
	int rv;

	rina::ScopedLock g(*lock_);

	if (sta_enr_sm.state != StaEnrollmentSM::ENROLLED) {
		LOG_IPCP_ERR("Cannot disconnect from neighbor because I'm not connected to it");

		try {
			rina::extendedIPCManager->disconnectNeighborResponse(event, -1);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s",
					e.what());
		}

		return;
	}

	rv = wpa_conn->disconnect();
	if (rv != 0) {
		LOG_IPCP_ERR("Could not disconnect from SDID %s (BSSID %s)",
				sta_enr_sm.dif_name.c_str(),
				sta_enr_sm.neighbor.c_str());

		try {
			rina::extendedIPCManager->disconnectNeighborResponse(event, -1);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s",
					e.what());
		}

		return;
	}

	sta_enr_sm.restart("", "");

	try {
		rina::extendedIPCManager->disconnectNeighborResponse(event, 0);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s",
				e.what());
	}

	return;
}

void ShimWifiStaIPCProcessImpl::ipcp_scan_media_request_event_handler(rina::ScanMediaRequestEvent& event)
{
	trigger_scan();
}

void ShimWifiStaIPCProcessImpl::trigger_scan()
{
	int rv;

	rina::ScopedLock g(*lock_);

	if (sta_enr_sm.state != StaEnrollmentSM::DISCONNECTED &&
			sta_enr_sm.state != StaEnrollmentSM::ENROLLED) {
		//Enrollment is taking place, do not trigger scan to avoid
		//making the procedure longer
		return;
	}

	rv = wpa_conn->scan();
	if (rv != 0) {
		LOG_IPCP_WARN("Problems triggering scan");
	}
}

void ShimWifiStaIPCProcessImpl::abort_enrollment()
{
	std::list<rina::Neighbor> neighbors;

	LOG_IPCP_ERR("Aborting enrollment. %s", sta_enr_sm.to_string().c_str());
	sta_enr_sm.restart("", "");

	try {
		rina::extendedIPCManager->enrollToDIFResponse(sta_enr_sm.enroll_event,
				-1,
				neighbors,
				dif_information_);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
	}
}

void ShimWifiStaIPCProcessImpl::notify_cancel_enrollment()
{
	rina::ScopedLock g(*lock_);

	if (sta_enr_sm.state == StaEnrollmentSM::DISCONNECTED ||
			sta_enr_sm.state == StaEnrollmentSM::ENROLLED)
		return;

	abort_enrollment();
}

void ShimWifiStaIPCProcessImpl::notify_trying_to_associate(const std::string& dif_name,
							   const std::string& neigh_name)
{
	rina::ScopedLock g(*lock_);

	timer.cancelTask(timer_task);
	if (sta_enr_sm.state != StaEnrollmentSM::ENROLLMENT_STARTED &&
			sta_enr_sm.state != StaEnrollmentSM::TRYING_TO_ASSOCIATE) {
		LOG_IPCP_WARN("Received trying to associate message in wrong state: %s",
			      sta_enr_sm.state_to_string().c_str());

		//TODO abort enrollment?
	}

	if (sta_enr_sm.dif_name != dif_name || sta_enr_sm.neighbor != neigh_name) {
		LOG_IPCP_ERR("WPA Supplicant is trying to associate to the wrong SSID (%s) or BSSID (%s)",
			      dif_name.c_str(),
			      neigh_name.c_str());
		abort_enrollment();
	}

	sta_enr_sm.state = StaEnrollmentSM::TRYING_TO_ASSOCIATE;
	LOG_IPCP_DBG("Enrollment state machine transitioned to the state %s after %d ms",
		     sta_enr_sm.state_to_string().c_str(),
		     rina::Time::get_time_in_ms() - sta_enr_sm.enrollment_start_time_ms);
	timer_task = new CancelEnrollmentTimerTask(this);
	timer.scheduleTask(timer_task, enrollment_timeout);
}

void ShimWifiStaIPCProcessImpl::notify_associated(const std::string& neigh_name)
{
	rina::ScopedLock g(*lock_);

	timer.cancelTask(timer_task);
	if (sta_enr_sm.state != StaEnrollmentSM::TRYING_TO_ASSOCIATE) {
		LOG_IPCP_WARN("Received associated message in wrong state: %s",
			      sta_enr_sm.state_to_string().c_str());
		//TODO abort enrollment?
	}

	if (sta_enr_sm.neighbor != neigh_name) {
		LOG_IPCP_ERR("WPA Supplicant associated to the wrong BSSID (%s)",
			      neigh_name.c_str());
		abort_enrollment();
	}

	sta_enr_sm.state = StaEnrollmentSM::ASSOCIATED;
	LOG_IPCP_DBG("Enrollment state machine transitioned to the state %s after %d ms",
		     sta_enr_sm.state_to_string().c_str(),
		     rina::Time::get_time_in_ms() - sta_enr_sm.enrollment_start_time_ms);
	timer_task = new CancelEnrollmentTimerTask(this);
	timer.scheduleTask(timer_task, enrollment_timeout);
}

void ShimWifiStaIPCProcessImpl::notify_key_negotiated(const std::string& neigh_name)
{
	rina::ScopedLock g(*lock_);

	timer.cancelTask(timer_task);
	if (sta_enr_sm.state != StaEnrollmentSM::ASSOCIATED) {
		LOG_IPCP_WARN("Received key negotiated message in wrong state: %s",
			      sta_enr_sm.state_to_string().c_str());
		//TODO abort enrollment?
	}

	if (sta_enr_sm.neighbor != neigh_name) {
		LOG_IPCP_ERR("WPA Supplicant negotiated key with the wrong BSSID (%s)",
			      neigh_name.c_str());
		abort_enrollment();
	}

	sta_enr_sm.state = StaEnrollmentSM::KEY_NEGOTIATION_COMPLETED;
	LOG_IPCP_DBG("Enrollment state machine transitioned to the state %s after %d ms",
		     sta_enr_sm.state_to_string().c_str(),
		     rina::Time::get_time_in_ms() - sta_enr_sm.enrollment_start_time_ms);
	timer_task = new CancelEnrollmentTimerTask(this);
	timer.scheduleTask(timer_task, enrollment_timeout);
}

void ShimWifiStaIPCProcessImpl::notify_connected(const std::string& neigh_name)
{
	rina::Neighbor ap;
	std::list<rina::Neighbor> neighbors;

	rina::ScopedLock g(*lock_);

	timer.cancelTask(timer_task);
	if (sta_enr_sm.state != StaEnrollmentSM::KEY_NEGOTIATION_COMPLETED) {
		LOG_IPCP_WARN("Received key negotiated message in wrong state: %s",
			      sta_enr_sm.state_to_string().c_str());

		return;
	}

	if (sta_enr_sm.neighbor != neigh_name) {
		LOG_IPCP_ERR("WPA Supplicant connected to the wrong BSSID (%s)",
			      neigh_name.c_str());
		abort_enrollment();
	}

	sta_enr_sm.state = StaEnrollmentSM::ENROLLED;
	sta_enr_sm.enrollment_end_time_ms = rina::Time::get_time_in_ms();
	LOG_IPCP_DBG("Enrollment completed after %d ms",
		     sta_enr_sm.enrollment_end_time_ms - sta_enr_sm.enrollment_start_time_ms);

	ap.name_ = sta_enr_sm.enroll_event.neighborName;
	ap.enrolled_ = true;
	neighbors.push_back(ap);
	dif_information_.dif_name_.processName = sta_enr_sm.enroll_event.dafName.processName;

	try {
		rina::extendedIPCManager->enrollToDIFResponse(sta_enr_sm.enroll_event,
							      0,
							      neighbors,
							      dif_information_);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
	}
}

void ShimWifiStaIPCProcessImpl::notify_disconnected(const std::string& neigh_name)
{
	std::list<rina::Neighbor> neighbors;
	StaEnrollmentSM::StaEnrollmentState current_state;

	rina::ScopedLock g(*lock_);

	if (sta_enr_sm.neighbor != neigh_name) {
		LOG_IPCP_DBG("Disconnected from old AP: %s", neigh_name.c_str());
		return;
	}

	current_state = sta_enr_sm.state;
	sta_enr_sm.restart("", "");
	if (current_state != StaEnrollmentSM::DISCONNECTED &&
			current_state != StaEnrollmentSM::ENROLLED) {
		timer.cancelTask(timer_task);

		try {
			rina::extendedIPCManager->enrollToDIFResponse(sta_enr_sm.enroll_event,
								      -1,
								      neighbors,
								      dif_information_);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
		}
	}
}

void ShimWifiStaIPCProcessImpl::notify_scan_results()
{
	rina::MediaReport report;
	std::map<std::string, rina::MediaDIFInfo>::iterator difs_it;
	std::string output;
	std::stringstream line_ss;
	std::string line;
	std::string value;
	std::vector<std::string> v;
	int i = 0;

	rina::ScopedLock g(*lock_);

	if (sta_enr_sm.state != StaEnrollmentSM::DISCONNECTED &&
			sta_enr_sm.state != StaEnrollmentSM::ENROLLED) {
		//Enrollment is taking place, do not read scan results to avoid
		//making the procedure longer
		return;
	}

	output = wpa_conn->scan_results();

	report.ipcp_id = get_id();
	report.current_dif_name = dif_information_.dif_name_.toString();
	//FIXME: add proper values here
	report.bs_ipcp_address ="0";

	line_ss.str(output);
	while (std::getline(line_ss, line)) {
		if (i==0) {
			++i;
			continue;
		}
		std::stringstream value_ss(line);
		std::vector<std::string> v;
		//line: bssid/frequency/signal/flags/ssid
		while(getline(value_ss, value, '\t')){
			v.push_back(value);
		}
		rina::BaseStationInfo bs_info;
		bs_info.ipcp_address = v[0];
		bs_info.signal_strength = atoi(v[2].c_str());
		difs_it = report.available_difs.find(v[4]);
		if(difs_it == report.available_difs.end()){
			rina::MediaDIFInfo m_info;
			m_info.dif_name = v[4];
			m_info.security_policies = v[3];
			m_info.available_bs_ipcps.push_back(bs_info);
			report.available_difs[v[4]] = m_info;
		} else {
			difs_it->second.available_bs_ipcps.push_back(bs_info);
		}
	}

	rina::extendedIPCManager->sendMediaReport(report);
}

//Class ShimWifiAPIPCProcessImpl
ShimWifiAPIPCProcessImpl::ShimWifiAPIPCProcessImpl(const rina::ApplicationProcessNamingInformation& name,
		          	  	  	   unsigned short id,
						   unsigned int ipc_manager_port,
						   std::string log_level,
						   std::string log_file,
						   std::string& folder) :
		ShimWifiIPCProcessImpl(rina::SHIM_WIFI_IPC_PROCESS_AP,
				       name, id, ipc_manager_port,
				       log_level, log_file, folder)
{
}

ShimWifiAPIPCProcessImpl::~ShimWifiAPIPCProcessImpl()
{
}

} //namespace rinad
