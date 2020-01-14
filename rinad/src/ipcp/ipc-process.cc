//
// Implementation of the IPC Process
//
//    Eduard Grasa <eduard.grasa@i2cat.net>
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

#define IPCP_MODULE "ipc-process"
#include "ipcp-logging.h"

#include "ipcp/enrollment-task.h"
#include "ipcp/flow-allocator.h"
#include "ipcp/ipc-process.h"
#include "ipcp/namespace-manager.h"
#include "ipcp/resource-allocator.h"
#include "ipcp/rib-daemon.h"
#include "ipcp/components.h"
#include "ipcp/utils.h"

#include "ipcp/shim-wifi/shim-wifi-ipc-process.h"

namespace rinad {

#define IPCP_LOG_IPCP_FILE_PREFIX "/tmp/ipcp-log-file"

/* Macro useful to perform downcasts in declarations. */
#define DOWNCAST_DECL(_var,_class,_name)        \
        _class *_name = dynamic_cast<_class*>(_var);

//Timeouts for timed wait
#define IPCP_EVENT_TIMEOUT_S 0
#define IPCP_EVENT_TIMEOUT_NS 1000000000 //1 sec

//Class IPCProcessImpl
AbstractIPCProcessImpl::AbstractIPCProcessImpl(const rina::ApplicationProcessNamingInformation& nm,
					       unsigned short id,
					       unsigned int ipc_manager_port,
					       std::string log_level,
					       std::string log_file)
{
        try {
                std::stringstream ss;
                ss << IPCP_LOG_IPCP_FILE_PREFIX << "-" << id;
                rina::initialize(getpid(), log_level, log_file);
                LOG_DBG("IPCProcessImpl");
                rina::extendedIPCManager->ipcManagerPort = ipc_manager_port;
                rina::extendedIPCManager->ipcProcessId = id;
                rina::kernelIPCProcess->ipcProcessId = id;
                LOG_IPCP_INFO("Librina initialized");
        } catch (rina::Exception &e) {
                std::cerr << "Cannot initialize librina" << std::endl;
                exit(EXIT_FAILURE);
        }

        state = NOT_INITIALIZED;
        lock_ = new rina::Lockable();
        keep_running = true;
}

AbstractIPCProcessImpl::~AbstractIPCProcessImpl() {

	LOG_IPCP_INFO("Abstract IPCP Destuctor called");

	if (lock_) {
		delete lock_;
	}
}

const std::string AbstractIPCProcessImpl::get_type() const
{
	return type;
}

//Event loop handlers
void AbstractIPCProcessImpl::event_loop(void)
{
	rina::IPCEvent *e;

	keep_running = true;

	LOG_DBG("Starting main I/O loop...");

	while(keep_running) {
		try {
			e = rina::ipcEventProducer->eventWait();
			if(!e)
				break;

			LOG_IPCP_DBG("Got event of type %s and sequence number %u",
					rina::IPCEvent::eventTypeToString(e->eventType).c_str(),
					e->sequenceNumber);

			switch(e->eventType){
			case rina::IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION:
			{
				DOWNCAST_DECL(e, rina::IPCProcessDIFRegistrationEvent, event);
				dif_registration_notification_handler(*event);
			}
			break;
			case rina::ASSIGN_TO_DIF_REQUEST_EVENT:
			{
				DOWNCAST_DECL(e, rina::AssignToDIFRequestEvent, event);
				assign_to_dif_request_handler(*event);
			}
			break;
			case rina::ASSIGN_TO_DIF_RESPONSE_EVENT:
			{
				DOWNCAST_DECL(e, rina::AssignToDIFResponseEvent, event);
				assign_to_dif_response_handler(*event);
			}
			break;
			case rina::ALLOCATE_FLOW_REQUEST_RESULT_EVENT:
			{
				DOWNCAST_DECL(e, rina::AllocateFlowRequestResultEvent, event);
				allocate_flow_request_result_handler(*event);
			}
			break;
			case rina::FLOW_ALLOCATION_REQUESTED_EVENT:
			{
				DOWNCAST_DECL(e, rina::FlowRequestEvent, event);
				flow_allocation_requested_handler(*event);
			}
			break;
			case rina::FLOW_DEALLOCATED_EVENT:
			{
				DOWNCAST_DECL(e, rina::FlowDeallocatedEvent, event);
				flow_deallocated_handler(*event);
			}
			break;
			case rina::FLOW_DEALLOCATION_REQUESTED_EVENT:
			{
				DOWNCAST_DECL(e, rina::FlowDeallocateRequestEvent, event);
				flow_deallocation_requested_handler(*event);
			}
			break;
			case rina::ALLOCATE_FLOW_RESPONSE_EVENT:
			{
				DOWNCAST_DECL(e, rina::AllocateFlowResponseEvent, event);
				allocate_flow_response_handler(*event);
			}
			break;
			case rina::APPLICATION_REGISTRATION_REQUEST_EVENT:
			{
				DOWNCAST_DECL(e, rina::ApplicationRegistrationRequestEvent, event);
				application_registration_request_handler(*event);
			}
			break;
			case rina::APPLICATION_UNREGISTRATION_REQUEST_EVENT:
			{
				DOWNCAST_DECL(e, rina::ApplicationUnregistrationRequestEvent, event);
				application_unregistration_handler(*event);
			}
			break;
			case rina::ENROLL_TO_DIF_REQUEST_EVENT:
			{
				DOWNCAST_DECL(e, rina::EnrollToDAFRequestEvent, event);
				enroll_to_dif_handler(*event);
			}
			break;
			case rina::DISCONNECT_NEIGHBOR_REQUEST_EVENT:
			{
				DOWNCAST_DECL(e, rina::DisconnectNeighborRequestEvent, event);
				disconnet_neighbor_handler(*event);
			}
			break;
			case rina::IPC_PROCESS_QUERY_RIB:
			{
				DOWNCAST_DECL(e, rina::QueryRIBRequestEvent, event);
				query_rib_handler(*event);
			}
			break;
			case rina::IPC_PROCESS_CREATE_CONNECTION_RESPONSE:
			{
				DOWNCAST_DECL(e, rina::CreateConnectionResponseEvent, event);
				create_efcp_connection_response_handler(*event);
			}
			break;
			case rina::IPC_PROCESS_CREATE_CONNECTION_RESULT:
			{
				DOWNCAST_DECL(e, rina::CreateConnectionResultEvent, event);
				create_efcp_connection_result_handler(*event);
			}
			break;
			case rina::IPC_PROCESS_UPDATE_CONNECTION_RESPONSE:
			{
				DOWNCAST_DECL(e, rina::UpdateConnectionResponseEvent, event);
				update_efcp_connection_response_handler(*event);
			}
			break;
			case rina::IPC_PROCESS_DESTROY_CONNECTION_RESULT:
			{
				DOWNCAST_DECL(e, rina::DestroyConnectionResultEvent, event);
				destroy_efcp_connection_result_handler(*event);
				break;
			}
			case rina::IPC_PROCESS_DUMP_FT_RESPONSE:
			{
				DOWNCAST_DECL(e, rina::DumpFTResponseEvent, event);
				dump_ft_response_handler(*event);
			}
			break;
			case rina::IPC_PROCESS_SET_POLICY_SET_PARAM:
			{
				DOWNCAST_DECL(e, rina::SetPolicySetParamRequestEvent, event);
				set_policy_set_param_handler(*event);
			}
			break;
			case rina::IPC_PROCESS_SET_POLICY_SET_PARAM_RESPONSE:
			{
				DOWNCAST_DECL(e, rina::SetPolicySetParamResponseEvent, event);
				set_policy_set_param_response_handler(*event);
			}
			break;
			case rina::IPC_PROCESS_SELECT_POLICY_SET:
			{
				DOWNCAST_DECL(e, rina::SelectPolicySetRequestEvent, event);
				select_policy_set_handler(*event);
			}
			break;
			case rina::IPC_PROCESS_SELECT_POLICY_SET_RESPONSE:
			{
				DOWNCAST_DECL(e, rina::SelectPolicySetResponseEvent, event);
				select_policy_set_response_handler(*event);
			}
			break;
			case rina::IPC_PROCESS_PLUGIN_LOAD:
			{
				DOWNCAST_DECL(e, rina::PluginLoadRequestEvent, event);
				plugin_load_handler(*event);
			}
			break;
			case rina::IPC_PROCESS_UPDATE_CRYPTO_STATE_RESPONSE:
			{
				DOWNCAST_DECL(e, rina::UpdateCryptoStateResponseEvent, event);
				update_crypto_state_response_handler(*event);
			}
			break;
			case rina::IPC_PROCESS_FWD_CDAP_MSG:
			{
				DOWNCAST_DECL(e, rina::FwdCDAPMsgRequestEvent, event);
				fwd_cdap_msg_handler(*event);
			}
			break;
			case rina::APPLICATION_UNREGISTERED_EVENT:
			{
				DOWNCAST_DECL(e, rina::ApplicationUnregisteredEvent, event);
				application_unregistered_handler(*event);
			}
			break;
			case rina::REGISTER_APPLICATION_RESPONSE_EVENT:
			{
				DOWNCAST_DECL(e, rina::RegisterApplicationResponseEvent, event);
				register_application_response_handler(*event);
			}
			break;
			case rina::UNREGISTER_APPLICATION_RESPONSE_EVENT:
			{
				DOWNCAST_DECL(e, rina::UnregisterApplicationResponseEvent, event);
				unregister_application_response_handler(*event);
			}
			break;
			case rina::UPDATE_DIF_CONFIG_REQUEST_EVENT:
			{
				DOWNCAST_DECL(e, rina::UpdateDIFConfigurationRequestEvent, event);
				update_dif_config_handler(*event);
			}
			break;
			case rina::IPCM_REGISTER_APP_RESPONSE_EVENT:
			{
				DOWNCAST_DECL(e, rina::IpcmRegisterApplicationResponseEvent, event);
				app_reg_response_handler(*event);
			}
			break;
			case rina::IPCM_UNREGISTER_APP_RESPONSE_EVENT:
			{
				DOWNCAST_DECL(e, rina::IpcmUnregisterApplicationResponseEvent, event);
				unreg_app_response_handler(*event);
			}
			break;
			case rina::IPCM_ALLOCATE_FLOW_REQUEST_RESULT:
			{
				DOWNCAST_DECL(e, rina::IpcmAllocateFlowRequestResultEvent, event);
				ipcm_allocate_flow_request_result_handler(*event);
			}
			break;
			case rina::IPC_PROCESS_ALLOCATE_PORT_RESPONSE:
			{
				DOWNCAST_DECL(e, rina::AllocatePortResponseEvent, event);
				ipcp_allocate_port_response_event_handler(*event);
			}
			break;
			case rina::IPC_PROCESS_DEALLOCATE_PORT_RESPONSE:
			{
				DOWNCAST_DECL(e, rina::DeallocatePortResponseEvent, event);
				ipcp_deallocate_port_response_event_handler(*event);
			}
			break;
			case rina::IPC_PROCESS_WRITE_MGMT_SDU_RESPONSE:
			{
				DOWNCAST_DECL(e, rina::WriteMgmtSDUResponseEvent, event);
				ipcp_write_mgmt_sdu_response_event_handler(*event);
			}
			break;
			case rina::IPC_PROCESS_READ_MGMT_SDU_NOTIF:
			{
				DOWNCAST_DECL(e, rina::ReadMgmtSDUResponseEvent, event);
				ipcp_read_mgmt_sdu_notif_event_handler(*event);
			}
			break;
			case rina::IPCP_SCAN_MEDIA_REQUEST_EVENT:
			{
				DOWNCAST_DECL(e, rina::ScanMediaRequestEvent, event);
				ipcp_scan_media_request_event_handler(*event);
			}
			break;

			//Unsupported events (they belong to the IPC Manager)
			case rina::APPLICATION_REGISTRATION_CANCELED_EVENT:
			case rina::UPDATE_DIF_CONFIG_RESPONSE_EVENT:
			case rina::ENROLL_TO_DIF_RESPONSE_EVENT:
			case rina::GET_DIF_PROPERTIES:
			case rina::GET_DIF_PROPERTIES_RESPONSE_EVENT:
			case rina::QUERY_RIB_RESPONSE_EVENT:
			case rina::IPC_PROCESS_DAEMON_INITIALIZED_EVENT:
			case rina::TIMER_EXPIRED_EVENT:
			case rina::IPC_PROCESS_PLUGIN_LOAD_RESPONSE:
			case rina::IPCM_CREATE_IPCP_RESPONSE:
			case rina::IPCM_DESTROY_IPCP_RESPONSE:
			default:
				break;
			}

			delete e;
		} catch (rina::Exception &ex) {
			LOG_IPCP_ERR("Problems running event loop: %s", ex.what());
		} catch (std::exception &ex1) {
			LOG_IPCP_ERR("Problems running event loop: %s", ex1.what());
		} catch (...) {
			LOG_IPCP_ERR("Unhandled exception!!!");
		}
	}
}

//Class LazyIPCPProcessImpl
LazyIPCProcessImpl::LazyIPCProcessImpl(const rina::ApplicationProcessNamingInformation& nm,
				       unsigned short id,
				       unsigned int ipc_manager_port,
				       std::string log_level,
				       std::string log_file) : AbstractIPCProcessImpl(nm, id, ipc_manager_port, log_level, log_file)
{
}

void LazyIPCProcessImpl::dif_registration_notification_handler(const rina::IPCProcessDIFRegistrationEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::assign_to_dif_request_handler(const rina::AssignToDIFRequestEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::assign_to_dif_response_handler(const rina::AssignToDIFResponseEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::allocate_flow_request_result_handler(const rina::AllocateFlowRequestResultEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::flow_allocation_requested_handler(const rina::FlowRequestEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::flow_deallocated_handler(const rina::FlowDeallocatedEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::flow_deallocation_requested_handler(const rina::FlowDeallocateRequestEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::allocate_flow_response_handler(const rina::AllocateFlowResponseEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::application_registration_request_handler(const rina::ApplicationRegistrationRequestEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::application_unregistration_handler(const rina::ApplicationUnregistrationRequestEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::enroll_to_dif_handler(const rina::EnrollToDAFRequestEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::disconnet_neighbor_handler(const rina::DisconnectNeighborRequestEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::query_rib_handler(const rina::QueryRIBRequestEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::create_efcp_connection_response_handler(const rina::CreateConnectionResponseEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::create_efcp_connection_result_handler(const rina::CreateConnectionResultEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::update_efcp_connection_response_handler(const rina::UpdateConnectionResponseEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::destroy_efcp_connection_result_handler(const rina::DestroyConnectionResultEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::dump_ft_response_handler(const rina::DumpFTResponseEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::set_policy_set_param_handler(const rina::SetPolicySetParamRequestEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::set_policy_set_param_response_handler(const rina::SetPolicySetParamResponseEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::select_policy_set_handler(const rina::SelectPolicySetRequestEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::select_policy_set_response_handler(const rina::SelectPolicySetResponseEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::plugin_load_handler(const rina::PluginLoadRequestEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::update_crypto_state_response_handler(const rina::UpdateCryptoStateResponseEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::fwd_cdap_msg_handler(rina::FwdCDAPMsgRequestEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::application_unregistered_handler(const rina::ApplicationUnregisteredEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::register_application_response_handler(const rina::RegisterApplicationResponseEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::unregister_application_response_handler(const rina::UnregisterApplicationResponseEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::update_dif_config_handler(const rina::UpdateDIFConfigurationRequestEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::app_reg_response_handler(const rina::IpcmRegisterApplicationResponseEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::unreg_app_response_handler(const rina::IpcmUnregisterApplicationResponseEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::ipcm_allocate_flow_request_result_handler(const rina::IpcmAllocateFlowRequestResultEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::ipcp_allocate_port_response_event_handler(const rina::AllocatePortResponseEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::ipcp_deallocate_port_response_event_handler(const rina::DeallocatePortResponseEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::ipcp_write_mgmt_sdu_response_event_handler(const rina::WriteMgmtSDUResponseEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::ipcp_read_mgmt_sdu_notif_event_handler(rina::ReadMgmtSDUResponseEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::ipcp_scan_media_request_event_handler(rina::ScanMediaRequestEvent& event)
{
	LOG_IPCP_WARN("Ignoring event of type %d", event.eventType);
}

void LazyIPCProcessImpl::sync_with_kernel(void)
{
	LOG_IPCP_WARN("Ignoring call to sync_with_kernel");
}

//Class IPCPFactory
static AbstractIPCProcessImpl * ipcp = NULL;

AbstractIPCProcessImpl* IPCPFactory::createIPCP(const std::string& type,
						const rina::ApplicationProcessNamingInformation& name,
				  		unsigned short id,
						unsigned int ipc_manager_port,
						std::string log_level,
						std::string log_file,
						std::string install_dir)
{
	if (ipcp)
		return ipcp;

	if(type == rina::NORMAL_IPC_PROCESS) {
		ipcp = new IPCProcessImpl(name,
					  ipcp_id,
					  ipc_manager_port,
					  log_level,
					  log_file);
		return ipcp;
	} else if(type == rina::SHIM_WIFI_IPC_PROCESS_STA) {
		ipcp = new ShimWifiStaIPCProcessImpl(name,
						     ipcp_id,
						     ipc_manager_port,
						     log_level,
						     log_file,
						     install_dir);
		return ipcp;
	} else if (type == rina::SHIM_WIFI_IPC_PROCESS_AP) {
		ipcp = new ShimWifiAPIPCProcessImpl(name,
						    ipcp_id,
						    ipc_manager_port,
						    log_level,
						    log_file,
						    install_dir);
		return ipcp;
	} else {
		LOG_IPCP_WARN("Unsupported IPCP type: %s", type.c_str());
		return 0;
	}
}

void IPCPFactory::destroyIPCP(const std::string& type)
{
	if (!ipcp)
		return;

	if (type == rina::NORMAL_IPC_PROCESS) {
		IPCProcessImpl * normal_ipcp;
		normal_ipcp = dynamic_cast<IPCProcessImpl*>(ipcp);
		delete normal_ipcp;
	} else if(type == rina::SHIM_WIFI_IPC_PROCESS_STA) {
		ShimWifiStaIPCProcessImpl * shim_wifi_sta_ipcp;
		shim_wifi_sta_ipcp = dynamic_cast<ShimWifiStaIPCProcessImpl*>(ipcp);
		delete shim_wifi_sta_ipcp;
	} else if (type == rina::SHIM_WIFI_IPC_PROCESS_AP) {
		ShimWifiAPIPCProcessImpl * shim_wifi_ap_ipcp;
		shim_wifi_ap_ipcp = dynamic_cast<ShimWifiAPIPCProcessImpl*>(ipcp);
		delete shim_wifi_ap_ipcp;
	} else {
		LOG_IPCP_WARN("Unsupported IPCP type: %s", type.c_str());
	}

	ipcp = 0;
}

IPCProcessImpl* IPCPFactory::getIPCP()
{
	IPCProcessImpl * normal_ipcp = NULL;

	if (ipcp) {
		normal_ipcp = dynamic_cast<IPCProcessImpl*>(ipcp);
		return normal_ipcp;
	}

	return NULL;
}

//Class KernelSyncTrigger
KernelSyncTrigger::KernelSyncTrigger(AbstractIPCProcessImpl * ipc_process,
		  	  	     unsigned int sync_period)
	: rina::SimpleThread(std::string("Kernel-sync-trigger"), false)
{
	end = false;
	ipcp = ipc_process;
	period_in_ms = sync_period;
}

int KernelSyncTrigger::run()
{
	while(!end) {
		sleep.sleepForMili(period_in_ms);
		ipcp->sync_with_kernel();
	}

	return 0;
}

void KernelSyncTrigger::finish()
{
	end = true;
}

} //namespace rinad
