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
class CancelEnrollmentTimerTask;

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
        virtual ~ShimWifiIPCProcessImpl();
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

        virtual void assign_to_dif_request_handler(const rina::AssignToDIFRequestEvent& event);
        virtual void assign_to_dif_response_handler(const rina::AssignToDIFResponseEvent& event);
        virtual void application_registration_request_handler(const rina::ApplicationRegistrationRequestEvent& event);
        virtual void app_reg_response_handler(const rina::IpcmRegisterApplicationResponseEvent& event);
        virtual void application_unregistration_handler(const rina::ApplicationUnregistrationRequestEvent& event);
        virtual void unreg_app_response_handler(const rina::IpcmUnregisterApplicationResponseEvent& event);
        virtual void flow_allocation_requested_handler(const rina::FlowRequestEvent& event);
        virtual void ipcm_allocate_flow_request_result_handler(const rina::IpcmAllocateFlowRequestResultEvent& event);
        virtual void allocate_flow_response_handler(const rina::AllocateFlowResponseEvent& event);
        virtual void flow_deallocation_requested_handler(const rina::FlowDeallocateRequestEvent& event);
        virtual void enroll_to_dif_handler(const rina::EnrollToDAFRequestEvent& event);
        virtual void ipcp_scan_media_request_event_handler(rina::ScanMediaRequestEvent& event);

protected:
	friend class rinad::ShimWifiScanTask;
	friend class rinad::WpaController;
        ShimWifiIPCPProxy * ipcp_proxy;
        std::map<unsigned int, rina::ApplicationRegistrationRequestEvent>
                pending_app_registration_events;
        std::map<unsigned int, rina::ApplicationUnregistrationRequestEvent>
                pending_app_unregistration_events;
        std::map<unsigned int, rina::FlowRequestEvent>
                pending_flow_allocation_events;
};

/// Class that captures the state of an enrollment operation
class StaEnrollmentSM {
public:
	enum StaEnrollmentState {
		DISCONNECTED,
		ENROLLMENT_STARTED,
		TRYING_TO_ASSOCIATE,
		ASSOCIATED,
		KEY_NEGOTIATION_COMPLETED,
		ENROLLED
	};

	StaEnrollmentSM();
	StaEnrollmentSM(const std::string& dif_name,
			const std::string neighbor);
	~StaEnrollmentSM(){};

	void restart(const std::string& dif_name,
		     const std::string neighbor);
	std::string state_to_string();
	std::string to_string();

	rina::EnrollToDAFRequestEvent enroll_event;
	StaEnrollmentState state;
	std::string dif_name;
	std::string neighbor;
	int enrollment_start_time_ms;
	int enrollment_end_time_ms;
};

class ShimWifiStaIPCProcessImpl: public ShimWifiIPCProcessImpl {
public:
	static const long DEFAULT_ENROLLMENT_TIMEOUT_MS;

	ShimWifiStaIPCProcessImpl(const rina::ApplicationProcessNamingInformation& name,
			          unsigned short id,
			          unsigned int ipc_manager_port,
			          std::string log_level,
			          std::string log_file,
			          std::string& folder);
	~ShimWifiStaIPCProcessImpl();

	void assign_to_dif_response_handler(const rina::AssignToDIFResponseEvent& event);
	void enroll_to_dif_handler(const rina::EnrollToDAFRequestEvent& event);
	void disconnet_neighbor_handler(const rina::DisconnectNeighborRequestEvent& event);
	void ipcp_scan_media_request_event_handler(rina::ScanMediaRequestEvent& event);

private:
	friend class rinad::CancelEnrollmentTimerTask;
	friend class rinad::WpaController;

	WpaController* wpa_conn;
	rina::Timer timer;
	StaEnrollmentSM sta_enr_sm;
	CancelEnrollmentTimerTask * timer_task;
	long enrollment_timeout;
	int scan_period_ms;

	void trigger_scan();
	void abort_enrollment();
	void notify_cancel_enrollment();
	void notify_trying_to_associate(const std::string& dif_name,
					const std::string& neigh_name);
	void notify_associated(const std::string& neigh_name);
	void notify_key_negotiated(const std::string& neigh_name);
	void notify_connected(const std::string& neigh_name);
	void notify_disconnected(const std::string& neigh_name);
	void notify_scan_results(void);
};

class ShimWifiAPIPCProcessImpl: public ShimWifiIPCProcessImpl {
public:
	ShimWifiAPIPCProcessImpl(const rina::ApplicationProcessNamingInformation& name,
			          unsigned short id,
			          unsigned int ipc_manager_port,
			          std::string log_level,
			          std::string log_file,
			          std::string& folder);
	~ShimWifiAPIPCProcessImpl();
};

} //namespace rinad

#endif //SHIM_WIFI_IPCP_IPC_PROCESS_HH
