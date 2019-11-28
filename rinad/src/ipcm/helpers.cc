//
// Helpers for the RINA daemons
//
//    Vincenzo Maffione <v.maffione@nextworks.it>
//    Marc Sune         <marc.sune (at) bisdn.de>
//    Eduard Grasa      <eduard.grasa@i2cat.net>
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
#include <iostream>
#include <vector>

#define RINA_PREFIX  "ipcm"

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/logs.h>

#include "ipcm.h"

using namespace std;


namespace rinad {

// Returns an IPC process assigned to the DIF specified by @dif_name,
// if any.
IPCMIPCProcess *
IPCManager_::select_ipcp_by_dif(
		const rina::ApplicationProcessNamingInformation& target_dif_name,
		bool write_lock)
{

	//Prevent any insertion/deletion to happen
	rina::ReadScopedLock readlock(ipcp_factory_.rwlock);

	vector<IPCMIPCProcess *> ipcps;
	ipcp_factory_.listIPCProcesses(ipcps);

	for (unsigned int i = 0; i < ipcps.size(); i++) {
		rina::ApplicationProcessNamingInformation dif_name = ipcps[i]->dif_name_;

		if (dif_name.processName == target_dif_name.processName) {
			//Acquire lock before leaving rwlock of the factory
			if(write_lock)
				ipcps[i]->rwlock.writelock();
			else
				ipcps[i]->rwlock.readlock();
			return ipcps[i];
		}
	}

	return NULL;
}

// Returns an IPC process where the application is registered,
// if any.
IPCMIPCProcess *
IPCManager_::select_ipcp_by_reg_app(
		const rina::ApplicationProcessNamingInformation& reg_app,
		bool write_lock)
{
	std::list<rina::ApplicationRegistrationInformation>::iterator it;

	//Prevent any insertion/deletion to happen
	rina::ReadScopedLock readlock(ipcp_factory_.rwlock);

	vector<IPCMIPCProcess *> ipcps;
	ipcp_factory_.listIPCProcesses(ipcps);

	for (unsigned int i = 0; i < ipcps.size(); i++) {
		for (it = ipcps[i]->registeredApplications.begin();
				it != ipcps[i]->registeredApplications.end();
				++ it) {
			if (it->appName.getEncodedString() ==
					reg_app.getEncodedString()) {
				//Acquire lock before leaving rwlock of the factory
				if(write_lock)
					ipcps[i]->rwlock.writelock();
				else
					ipcps[i]->rwlock.readlock();
				return ipcps[i];
			}
		}
	}

	return NULL;
}

// Returns any IPC process in the system, giving priority to
// normal IPC processes.
IPCMIPCProcess *
IPCManager_::select_ipcp(bool write_lock)
{

	//Prevent any insertion/deletion to happen
	rina::ReadScopedLock readlock(ipcp_factory_.rwlock);

	vector<IPCMIPCProcess *> ipcps;
	ipcp_factory_.listIPCProcesses(ipcps);

	for (unsigned int i = 0; i < ipcps.size(); i++) {
		if (ipcps[i]->get_type() == rina::NORMAL_IPC_PROCESS) {
			//Acquire lock before leaving rwlock of the factory
			if(write_lock)
				ipcps[i]->rwlock.writelock();
			else
				ipcps[i]->rwlock.readlock();

			return ipcps[i];
		}
	}

	for (unsigned int i = 0; i < ipcps.size(); i++) {
		//Acquire lock before leaving rwlock of the factory
		if(write_lock)
			ipcps[i]->rwlock.writelock();
		else
			ipcps[i]->rwlock.readlock();

		return ipcps[i];
	}

	return NULL;
}

bool
IPCManager_::application_is_registered_to_ipcp(std::list<rina::ApplicationProcessNamingInformation> & reg_names,
		       	       	       	       pid_t pid, IPCMIPCProcess *slave_ipcp)
{
	list<rina::ApplicationRegistrationInformation>::iterator it;
	rina::ApplicationProcessNamingInformation reg_name;

	//Prevent any insertion/deletion to happen
	rina::ReadScopedLock readlock(ipcp_factory_.rwlock);

	for (it = slave_ipcp->registeredApplications.begin();
				it != slave_ipcp->registeredApplications.end(); it++) {
		if (it->pid == pid) {
			reg_name.processName = it->appName.processName;
			reg_name.processInstance = it->appName.processInstance;
			reg_name.entityName = it->appName.entityName;
			reg_name.entityInstance = it->appName.entityInstance;
			reg_names.push_back(reg_name);
		}
	}

	return reg_names.size() > 0;
}

void
IPCManager_::application_has_flow_by_ipcp(pid_t pid, IPCMIPCProcess *slave_ipcp,
				          std::list<int>& port_ids)
{
	std::list<rina::FlowInformation>::iterator it;

	//Prevent any insertion/deletion to happen
	rina::ReadScopedLock readlock(ipcp_factory_.rwlock);

	for (it = slave_ipcp->allocatedFlows.begin();
				it != slave_ipcp->allocatedFlows.end(); it++) {
		if (it->pid == pid) {
			port_ids.push_back(it->portId);
		}
	}
}

IPCMIPCProcess *
IPCManager_::lookup_ipcp_by_port(unsigned int port_id, bool write_lock)
{
	//Prevent any insertion/deletion to happen
	rina::ReadScopedLock readlock(ipcp_factory_.rwlock);

	vector<IPCMIPCProcess *> ipcps;
	ipcp_factory_.listIPCProcesses(ipcps);
	rina::FlowInformation info;

	for (unsigned int i = 0; i < ipcps.size(); i++) {
		if (ipcps[i]->getFlowInformation(port_id, info)) {
			//Acquire lock before leaving rwlock of the factory
			if(write_lock)
				ipcps[i]->rwlock.writelock();
			else
				ipcps[i]->rwlock.readlock();

			return ipcps[i];
		}
	}

	return NULL;
}

void
IPCManager_::collect_flows_by_pid(pid_t pid, list<rina::FlowInformation>& result)
{
	//Prevent any insertion/deletion to happen
	rina::ReadScopedLock readlock(ipcp_factory_.rwlock);

	vector<IPCMIPCProcess *> ipcps;
	ipcp_factory_.listIPCProcesses(ipcps);

	result.clear();

	for (unsigned int i = 0; i < ipcps.size(); i++) {
		const list<rina::FlowInformation>& flows =
			ipcps[i]->allocatedFlows;

		for (list<rina::FlowInformation>::const_iterator it =
			flows.begin(); it != flows.end(); it++) {
			if (it->pid == pid) {
				result.push_back(*it);
			}
		}
	}
}

IPCMIPCProcess *
IPCManager_::lookup_ipcp_by_id(unsigned short id, bool write_lock)
{
	IPCMIPCProcess * ipcp;

	//Prevent any insertion/deletion to happen
	rina::ReadScopedLock readlock(ipcp_factory_.rwlock);
	ipcp = ipcp_factory_.getIPCProcess(id);

	if(!ipcp)
		return NULL;

	//Acquire lock before leaving rwlock of the factory
	write_lock? ipcp->rwlock.writelock(): ipcp->rwlock.readlock();

	return ipcp;
}

bool
IPCManager_::is_any_ipcp_assigned_to_dif(const rina::ApplicationProcessNamingInformation& dif_name)
{
	//Prevent any insertion/deletion to happen
	rina::ReadScopedLock readlock(ipcp_factory_.rwlock);

	vector<IPCMIPCProcess *> ipcps;
	ipcp_factory_.listIPCProcesses(ipcps);

	for (unsigned int i = 0; i < ipcps.size(); i++) {
		if (ipcps[i]->dif_name_.processName == dif_name.processName)
			return true;

	}

	return false;
}

} //rinad namespace
