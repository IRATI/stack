/*
 * Implementation of the IPC Process
 *
 *    Eduard Grasa <eduard.grasa@i2cat.net>
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

#ifndef IPCP_IPC_PROCESS_HH
#define IPCP_IPC_PROCESS_HH

#include <map>

#include <librina/ipc-manager.h>
#include "ipcp/components.h"

namespace rinad {

class IPCPFactory;
class IPCProcessImpl;

/// Base class for the implementation of the user-space components
/// of an IPC Process. Contains the getters/setters for generic fields as
/// well as code to deal with the events coming from the IPC Manager
/// Daemon or the kernel. Specific implementations have to inherit from this class
class AbstractIPCProcessImpl {
public:
        AbstractIPCProcessImpl(const rina::ApplicationProcessNamingInformation& name,
        		       unsigned short id,
			       unsigned int ipc_manager_port,
			       std::string log_level,
			       std::string log_file);
        virtual ~AbstractIPCProcessImpl();
        const std::string get_type() const;

	//Event loop (run)
	void event_loop(void);
	bool keep_running;

	//Event handlers to be implemented by each particular IPCP
        virtual void dif_registration_notification_handler(const rina::IPCProcessDIFRegistrationEvent& event) = 0;
        virtual void assign_to_dif_request_handler(const rina::AssignToDIFRequestEvent& event) = 0;
        virtual void assign_to_dif_response_handler(const rina::AssignToDIFResponseEvent& event) = 0;
        virtual void allocate_flow_request_result_handler(const rina::AllocateFlowRequestResultEvent& event) = 0;
        virtual void flow_allocation_requested_handler(const rina::FlowRequestEvent& event) = 0;
        virtual void flow_deallocated_handler(const rina::FlowDeallocatedEvent& event) = 0;
        virtual void flow_deallocation_requested_handler(const rina::FlowDeallocateRequestEvent& event) = 0;
        virtual void allocate_flow_response_handler(const rina::AllocateFlowResponseEvent& event) = 0;
        virtual void application_registration_request_handler(const rina::ApplicationRegistrationRequestEvent& event) = 0;
        virtual void application_unregistration_handler(const rina::ApplicationUnregistrationRequestEvent& event) = 0;
        virtual void enroll_to_dif_handler(const rina::EnrollToDAFRequestEvent& event) = 0;
        virtual void disconnet_neighbor_handler(const rina::DisconnectNeighborRequestEvent& event) = 0;
        virtual void query_rib_handler(const rina::QueryRIBRequestEvent& event) = 0;
        virtual void create_efcp_connection_response_handler(const rina::CreateConnectionResponseEvent& event) = 0;
        virtual void create_efcp_connection_result_handler(const rina::CreateConnectionResultEvent& event) = 0;
        virtual void update_efcp_connection_response_handler(const rina::UpdateConnectionResponseEvent& event) = 0;
        virtual void destroy_efcp_connection_result_handler(const rina::DestroyConnectionResultEvent& event) = 0;
        virtual void dump_ft_response_handler(const rina::DumpFTResponseEvent& event) = 0;
        virtual void set_policy_set_param_handler(const rina::SetPolicySetParamRequestEvent& event) = 0;
        virtual void set_policy_set_param_response_handler(const rina::SetPolicySetParamResponseEvent& event) = 0;
        virtual void select_policy_set_handler(const rina::SelectPolicySetRequestEvent& event) = 0;
        virtual void select_policy_set_response_handler(const rina::SelectPolicySetResponseEvent& event) = 0;
        virtual void plugin_load_handler(const rina::PluginLoadRequestEvent& event) = 0;
        virtual void update_crypto_state_response_handler(const rina::UpdateCryptoStateResponseEvent& event) = 0;
        virtual void fwd_cdap_msg_handler(rina::FwdCDAPMsgRequestEvent& event) = 0;
        virtual void application_unregistered_handler(const rina::ApplicationUnregisteredEvent& event) = 0;
        virtual void register_application_response_handler(const rina::RegisterApplicationResponseEvent& event) = 0;
        virtual void unregister_application_response_handler(const rina::UnregisterApplicationResponseEvent& event) = 0;
        virtual void update_dif_config_handler(const rina::UpdateDIFConfigurationRequestEvent& event) = 0;
        virtual void app_reg_response_handler(const rina::IpcmRegisterApplicationResponseEvent& event) = 0;
        virtual void unreg_app_response_handler(const rina::IpcmUnregisterApplicationResponseEvent& event) = 0;
        virtual void ipcm_allocate_flow_request_result_handler(const rina::IpcmAllocateFlowRequestResultEvent& event) = 0;
        virtual void ipcp_allocate_port_response_event_handler(const rina::AllocatePortResponseEvent& event) = 0;
        virtual void ipcp_deallocate_port_response_event_handler(const rina::DeallocatePortResponseEvent& event) = 0;
        virtual void ipcp_write_mgmt_sdu_response_event_handler(const rina::WriteMgmtSDUResponseEvent& event) = 0;
        virtual void ipcp_read_mgmt_sdu_notif_event_handler(rina::ReadMgmtSDUResponseEvent& event) = 0;
        virtual void ipcp_scan_media_request_event_handler(rina::ScanMediaRequestEvent& event) = 0;
        // Cause relevant IPCP components to sync with information
        // exported by the kernel via sysfs
        virtual void sync_with_kernel() = 0;


protected:
        IPCProcessOperationalState state;
	std::map<unsigned int, rina::AssignToDIFRequestEvent> pending_events_;
        std::map<unsigned int, rina::SetPolicySetParamRequestEvent>
                pending_set_policy_set_param_events;
        std::map<unsigned int, rina::SelectPolicySetRequestEvent>
                pending_select_policy_set_events;
        rina::Lockable * lock_;
	rina::DIFInformation dif_information_;
	std::string type;
};

/// An IPC Process that does nothing regardless of the event that
/// has happened. Convenience class to avoid repeating code for
/// IPCP implementations that do nothing on certain events, they
/// can just subclass this one
class LazyIPCProcessImpl: public AbstractIPCProcessImpl {
public:
	LazyIPCProcessImpl(const rina::ApplicationProcessNamingInformation& name,
                       unsigned short id,
		       unsigned int ipc_manager_port,
		       std::string log_level,
		       std::string log_file);
        virtual ~LazyIPCProcessImpl() {};

        //Event loop handlers
        virtual void dif_registration_notification_handler(const rina::IPCProcessDIFRegistrationEvent& event);
        virtual void assign_to_dif_request_handler(const rina::AssignToDIFRequestEvent& event);
        virtual void assign_to_dif_response_handler(const rina::AssignToDIFResponseEvent& event);
        virtual void allocate_flow_request_result_handler(const rina::AllocateFlowRequestResultEvent& event);
        virtual void flow_allocation_requested_handler(const rina::FlowRequestEvent& event);
        virtual void flow_deallocated_handler(const rina::FlowDeallocatedEvent& event);
        virtual void flow_deallocation_requested_handler(const rina::FlowDeallocateRequestEvent& event);
        virtual void allocate_flow_response_handler(const rina::AllocateFlowResponseEvent& event);
        virtual void application_registration_request_handler(const rina::ApplicationRegistrationRequestEvent& event);
        virtual void application_unregistration_handler(const rina::ApplicationUnregistrationRequestEvent& event);
        virtual void enroll_to_dif_handler(const rina::EnrollToDAFRequestEvent& event);
        virtual void disconnet_neighbor_handler(const rina::DisconnectNeighborRequestEvent& event);
        virtual void query_rib_handler(const rina::QueryRIBRequestEvent& event);
        virtual void create_efcp_connection_response_handler(const rina::CreateConnectionResponseEvent& event);
        virtual void create_efcp_connection_result_handler(const rina::CreateConnectionResultEvent& event);
        virtual void update_efcp_connection_response_handler(const rina::UpdateConnectionResponseEvent& event);
        virtual void destroy_efcp_connection_result_handler(const rina::DestroyConnectionResultEvent& event);
        virtual void dump_ft_response_handler(const rina::DumpFTResponseEvent& event);
        virtual void set_policy_set_param_handler(const rina::SetPolicySetParamRequestEvent& event);
        virtual void set_policy_set_param_response_handler(const rina::SetPolicySetParamResponseEvent& event);
        virtual void select_policy_set_handler(const rina::SelectPolicySetRequestEvent& event);
        virtual void select_policy_set_response_handler(const rina::SelectPolicySetResponseEvent& event);
        virtual void plugin_load_handler(const rina::PluginLoadRequestEvent& event);
        virtual void update_crypto_state_response_handler(const rina::UpdateCryptoStateResponseEvent& event);
        virtual void fwd_cdap_msg_handler(rina::FwdCDAPMsgRequestEvent& event);
        virtual void application_unregistered_handler(const rina::ApplicationUnregisteredEvent& event);
        virtual void register_application_response_handler(const rina::RegisterApplicationResponseEvent& event);
        virtual void unregister_application_response_handler(const rina::UnregisterApplicationResponseEvent& event);
        virtual void update_dif_config_handler(const rina::UpdateDIFConfigurationRequestEvent& event);
        virtual void app_reg_response_handler(const rina::IpcmRegisterApplicationResponseEvent& event);
        virtual void unreg_app_response_handler(const rina::IpcmUnregisterApplicationResponseEvent& event);
        virtual void ipcm_allocate_flow_request_result_handler(const rina::IpcmAllocateFlowRequestResultEvent& event);
        virtual void ipcp_allocate_port_response_event_handler(const rina::AllocatePortResponseEvent& event);
        virtual void ipcp_deallocate_port_response_event_handler(const rina::DeallocatePortResponseEvent& event);
        virtual void ipcp_write_mgmt_sdu_response_event_handler(const rina::WriteMgmtSDUResponseEvent& event);
        virtual void ipcp_read_mgmt_sdu_notif_event_handler(rina::ReadMgmtSDUResponseEvent& event);
        virtual void ipcp_scan_media_request_event_handler(rina::ScanMediaRequestEvent& event);
	virtual void sync_with_kernel(void);
};

/// Periodically causes the IPCP Daemon to synchronize
/// with the kernel
class KernelSyncTrigger : public rina::SimpleThread {
public:
	KernelSyncTrigger(AbstractIPCProcessImpl * ipcp,
			  unsigned int sync_period);
	~KernelSyncTrigger() throw() {};

	int run();
	void finish();

private:
	bool end;
	AbstractIPCProcessImpl * ipcp;
	unsigned int period_in_ms;
	rina::Sleep sleep;
};

/// Implementation of the normal IPC Process Daemon
class IPCProcessImpl: public IPCProcess, public LazyIPCProcessImpl, public rina::InternalEventListener  {
public:
        IPCProcessImpl(const rina::ApplicationProcessNamingInformation& name,
                       unsigned short id,
		       unsigned int ipc_manager_port,
		       std::string log_level,
		       std::string log_file);
        ~IPCProcessImpl();
        unsigned short get_id();
        const IPCProcessOperationalState& get_operational_state() const;
        void set_operational_state(const IPCProcessOperationalState& operational_state);
        rina::DIFInformation& get_dif_information();
        void set_dif_information(const rina::DIFInformation& dif_information);
        const std::list<rina::Neighbor> get_neighbors() const;
        unsigned int get_address() const;
        void set_address(unsigned int address);
        void requestPDUFTEDump();
        void expire_old_address(void);
        unsigned int get_old_address();
        unsigned int get_active_address();
        void activate_new_address(void);
        bool check_address_is_mine(unsigned int address);
        void eventHappened(rina::InternalEvent * event);
        unsigned int getAdressByname(const rina::ApplicationProcessNamingInformation& name);
        int dispatchSelectPolicySet(const std::string& path,
                                    const std::string& name,
                                    bool& got_in_userspace);

        //Event loop handlers
        void dif_registration_notification_handler(const rina::IPCProcessDIFRegistrationEvent& event);
        void assign_to_dif_request_handler(const rina::AssignToDIFRequestEvent& event);
        void assign_to_dif_response_handler(const rina::AssignToDIFResponseEvent& event);
        void allocate_flow_request_result_handler(const rina::AllocateFlowRequestResultEvent& event);
        void flow_allocation_requested_handler(const rina::FlowRequestEvent& event);
        void flow_deallocated_handler(const rina::FlowDeallocatedEvent& event);
        void flow_deallocation_requested_handler(const rina::FlowDeallocateRequestEvent& event);
        void allocate_flow_response_handler(const rina::AllocateFlowResponseEvent& event);
        void application_registration_request_handler(const rina::ApplicationRegistrationRequestEvent& event);
        void application_unregistration_handler(const rina::ApplicationUnregistrationRequestEvent& event);
        void enroll_to_dif_handler(const rina::EnrollToDAFRequestEvent& event);
        void disconnet_neighbor_handler(const rina::DisconnectNeighborRequestEvent& event);
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
        void fwd_cdap_msg_handler(rina::FwdCDAPMsgRequestEvent& event);
        void ipcp_allocate_port_response_event_handler(const rina::AllocatePortResponseEvent& event);
        void ipcp_deallocate_port_response_event_handler(const rina::DeallocatePortResponseEvent& event);
        void ipcp_write_mgmt_sdu_response_event_handler(const rina::WriteMgmtSDUResponseEvent& event);
        void ipcp_read_mgmt_sdu_notif_event_handler(rina::ReadMgmtSDUResponseEvent& event);
        void sync_with_kernel(void);

private:
        void subscribeToEvents();
        void addressChange(rina::AddressChangeEvent * event);

        KernelSyncTrigger * kernel_sync;
        unsigned int old_address;
        bool address_change_period;
        bool use_new_address;
        rina::Timer timer;
};

class IPCPFactory{

public:
	/**
	* Create IPCP
	*/
	static AbstractIPCProcessImpl* createIPCP(const std::string& type,
						  const rina::ApplicationProcessNamingInformation& name,
					  	  unsigned short id,
						  unsigned int ipc_manager_port,
						  std::string log_level,
						  std::string log_file,
						  std::string install_dir);

	static IPCProcessImpl* getIPCP();

	static void destroyIPCP(const std::string& type);
};


} //namespace rinad

#endif //IPCP_IPC_PROCESS_HH
