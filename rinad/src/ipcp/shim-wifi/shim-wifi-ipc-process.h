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

namespace rinad {

class ShimWifiIPCProcessImpl;

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
        unsigned int get_active_address();
        unsigned int get_old_address();
        bool check_address_is_mine(unsigned int address);
        void requestPDUFTEDump();
        int dispatchSelectPolicySet(const std::string& path,
                                    const std::string& name,
                                    bool& got_in_userspace);
private:

        ShimWifiIPCPProxy * ipcp_proxy;
};

} //namespace rinad

#endif //SHIM_WIFI_IPCP_IPC_PROCESS_HH
