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

#define IPCP_MODULE "shim-wifi-ipcp"
#include "ipcp-logging.h"

#include "ipcp/shim-wifi/shim-wifi-ipc-process.h"
#include "ipcp/ipc-process.h"

namespace rinad {

static void parse_path(const std::string& path, std::string& component,
                       std::string& remainder)
{
        size_t dotpos = path.find_first_of('.');

        component = path.substr(0, dotpos);
        if (dotpos == std::string::npos || dotpos + 1 == path.size()) {
                remainder = std::string();
        } else {
                remainder = path.substr(dotpos + 1);
        }
}

pid_t hostapd_pid = 0;
pid_t wpas_pid = 0;

//Class ShimWifiIPCPProxy
ShimWifiIPCPProxy::ShimWifiIPCPProxy(unsigned short id,
				     const std::string& type,
				     const rina::ApplicationProcessNamingInformation& name) :
						     rina::IPCProcessProxy(id, 0, 0, type, name)
{
}

//Class ShimWifiIPCProcessImpl
ShimWifiIPCProcessImpl::ShimWifiIPCProcessImpl(const std::string& type,
				const rina::ApplicationProcessNamingInformation& nm,
				unsigned short id,
				unsigned int ipc_manager_port,
				std::string log_level,
				std::string log_file) : IPCProcess(nm.processName, nm.processInstance),
					       	       LazyIPCProcessImpl(nm, id, ipc_manager_port, log_level, log_file)
{
	pid_t wpid;
	if (type == rina::SHIM_WIFI_IPC_PROCESS_AP) {
		wpid = hostapd_pid;
	} else if (type == rina::SHIM_WIFI_IPC_PROCESS_STA) {
		wpid = wpas_pid;
	} else {
		LOG_IPCP_ERR("Could not create Shim WiFi IPCP, type %s not recognized",
								type.c_str());
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
	return enrollment_task_->get_neighbors();
}

unsigned int ShimWifiIPCProcessImpl::get_address() const {
	rina::ScopedLock g(*lock_);
	if (state != ASSIGNED_TO_DIF) {
		return 0;
	}

	return dif_information_.dif_configuration_.address_;
}

unsigned int ShimWifiIPCProcessImpl::get_active_address()
{
	return 0;
}

unsigned int ShimWifiIPCProcessImpl::get_old_address() {
	return 0;
}

bool ShimWifiIPCProcessImpl::check_address_is_mine(unsigned int address)
{
	return false;
}

void ShimWifiIPCProcessImpl::set_address(unsigned int address)
{
}

void ShimWifiIPCProcessImpl::requestPDUFTEDump()
{
}

int ShimWifiIPCProcessImpl::dispatchSelectPolicySet(const std::string& path,
                                            	    const std::string& name,
						    bool& got_in_userspace)
{
}


} //namespace rinad
