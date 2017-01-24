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

#include "ipcp/components.h"
#include "ipcp/ipc-process.h"

namespace rinad {

class ShimWifiIPCProcessImpl;

/// Implementation of the Shim WiFi IPC Process Daemon
class ShimWifiIPCProcessImpl: public IPCProcess,
					public LazyIPCProcessImpl {
public:
        ShimWifiIPCProcessImpl(const std::string& type,
			const rina::ApplicationProcessNamingInformation& name,
			unsigned short id,
			unsigned int ipc_manager_port,
			std::string log_level,
			std::string log_file);
        ~ShimWifiIPCProcessImpl();
        unsigned short get_id();
        const IPCProcessOperationalState& get_operational_state() const;
        void set_operational_state(const IPCProcessOperationalState& operational_state);
        rina::DIFInformation& get_dif_information();
        void set_dif_information(const rina::DIFInformation& dif_information);
        const std::list<rina::Neighbor> get_neighbors() const;
        unsigned int get_address() const;
        void set_address(unsigned int address);
        void requestPDUFTEDump();
        int dispatchSelectPolicySet(const std::string& path,
                                    const std::string& name,
                                    bool& got_in_userspace);
        // Cause relevant IPCP components to sync with information
        // exported by the kernel via sysfs
        void sync_with_kernel();

        void assign_to_dif_request_handler(const rina::AssignToDIFRequestEvent& event);
        void assign_to_dif_response_handler(const rina::AssignToDIFResponseEvent& event);
#if 0
        //Event loop handlers
        void dif_registration_notification_handler(const rina::IPCProcessDIFRegistrationEvent& event);
        void allocate_flow_request_result_handler(const rina::AllocateFlowRequestResultEvent& event);
        void flow_allocation_requested_handler(const rina::FlowRequestEvent& event);
        void deallocate_flow_response_handler(const rina::DeallocateFlowResponseEvent& event);
        void flow_deallocated_handler(const rina::FlowDeallocatedEvent& event);
        void flow_deallocation_requested_handler(const rina::FlowDeallocateRequestEvent& event);
        void allocate_flow_response_handler(const rina::AllocateFlowResponseEvent& event);
        void application_registration_request_handler(const rina::ApplicationRegistrationRequestEvent& event);
        void application_unregistration_handler(const rina::ApplicationUnregistrationRequestEvent& event);
        void enroll_to_dif_handler(const rina::EnrollToDAFRequestEvent& event);
        void query_rib_handler(const rina::QueryRIBRequestEvent& event);
        void create_efcp_connection_response_handler(const rina::CreateConnectionResponseEvent& event);
        void create_efcp_connection_result_handler(const rina::CreateConnectionResultEvent& event);
        void update_efcp_connection_response_handler(const rina::UpdateConnectionResponseEvent& event);
        void destroy_efcp_connection_result_handler(const rina::DestroyConnectionResultEvent& event);
        void dump_ft_response_handler(const rina::DumpFTResponseEvent& event);
        void set_policy_set_param_handler(const rina::SetPolicySetParamRequestEvent& event);
        void set_policy_set_param_response_handler(const rina::SetPolicySetParamResponseEvent& event);
        void select_policy_set_handler(const rina::SelectPolicySetRequestEvent& event);
        void select_policy_set_response_handler(const rina::SelectPolicySetResponseEvent& event);
        void plugin_load_handler(const rina::PluginLoadRequestEvent& event);
        void update_crypto_state_response_handler(const rina::UpdateCryptoStateResponseEvent& event);
        void fwd_cdap_msg_handler(const rina::FwdCDAPMsgRequestEvent& event);
#endif

private:
	KernelSyncTrigger * kernel_sync;
};

} //namespace rinad

#endif //SHIM_WIFI_IPCP_IPC_PROCESS_HH
