/*
 * Implementation of the Shim Wifi IPC Process
 *
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
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

#ifndef SHIM_WIFI_IPCP_IPC_PROCESS_HH
#define SHIM_WIFI_IPCP_IPC_PROCESS_HH

#include <librina/ipc-manager.h>

#include "ipcp/components.h"
#include "ipcp/ipc-process.h"
#include "ipcp/shim-wifi/wpa_controller.h"

namespace rinad {

class ShimWifiIPCProcessImpl;
class ShimWifiScanTask;

/// Proxy to interact with the Shim Wifi IPC Process components
/// in the kernel and those implemented by WPASupplicant or
/// HostAPD
class ShimWifiIPCPProxy: public rina::IPCProcessProxy {
public:
	ShimWifiIPCPProxy(unsigned short id,
			  const std::string& type,
			  const rina::ApplicationProcessNamingInformation& name);
};

/// Implementation of the Shim WiFi IPC Process Daemon
class ShimWifiIPCProcessImpl: public IPCProcess,
					public LazyIPCProcessImpl {
public:
        ShimWifiIPCProcessImpl(const std::string& type,
			const rina::ApplicationProcessNamingInformation& name,
			unsigned short id,
			unsigned int ipc_manager_port,
			std::string log_level,
			std::string log_file,
			std::string& folder);
        ~ShimWifiIPCProcessImpl();
        unsigned short get_id();
        const IPCProcessOperationalState& get_operational_state() const;
        void set_operational_state(const IPCProcessOperationalState& operational_state);
        rina::DIFInformation& get_dif_information();
        void set_dif_information(const rina::DIFInformation& dif_information);
        const std::list<rina::Neighbor> get_neighbors() const;
        unsigned int get_address() const;
        void set_address(unsigned int address);
        unsigned int get_active_address();
        unsigned int get_old_address();
        bool check_address_is_mine(unsigned int address);
        void requestPDUFTEDump();
        int dispatchSelectPolicySet(const std::string& path,
                                    const std::string& name,
                                    bool& got_in_userspace);

        void assign_to_dif_request_handler(const rina::AssignToDIFRequestEvent& event);
        void assign_to_dif_response_handler(const rina::AssignToDIFResponseEvent& event);
        void application_registration_request_handler(const rina::ApplicationRegistrationRequestEvent& event);
        void app_reg_response_handler(const rina::IpcmRegisterApplicationResponseEvent& event);
        void application_unregistration_handler(const rina::ApplicationUnregistrationRequestEvent& event);
        void unreg_app_response_handler(const rina::IpcmUnregisterApplicationResponseEvent& event);
        void flow_allocation_requested_handler(const rina::FlowRequestEvent& event);
        void ipcm_allocate_flow_request_result_handler(const rina::IpcmAllocateFlowRequestResultEvent& event);
        void allocate_flow_response_handler(const rina::AllocateFlowResponseEvent& event);
        void flow_deallocation_requested_handler(const rina::FlowDeallocateRequestEvent& event);
        void ipcm_deallocate_flow_response_event_handler(const rina::IpcmDeallocateFlowResponseEvent& event);
        void enroll_to_dif_handler(const rina::EnrollToDAFRequestEvent& event);
	void dissconnect_neighbour_handler(const rina::DisconnectNeighborRequestEvent& event);

private:
	friend class rinad::ShimWifiScanTask;
	friend class rinad::WpaController;
        ShimWifiIPCPProxy * ipcp_proxy;
        std::map<unsigned int, rina::ApplicationRegistrationRequestEvent>
                pending_app_registration_events;
        std::map<unsigned int, rina::ApplicationUnregistrationRequestEvent>
                pending_app_unregistration_events;
        std::map<unsigned int, rina::FlowRequestEvent>
                pending_flow_allocation_events;
        std::map<unsigned int, rina::FlowDeallocateRequestEvent>
                pending_flow_deallocation_events;
	WpaController* wpa_conn;
	rina::Timer scanner;

	void push_scan_results(std::string& output);

};

} //namespace rinad

#endif //SHIM_WIFI_IPCP_IPC_PROCESS_HH
