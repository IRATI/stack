//
// Helpers for the RINA daemons
//
//    Vincenzo Maffione <v.maffione@nextworks.it>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
rina::IPCProcess *
IPCManager_::select_ipcp_by_dif(
		const rina::ApplicationProcessNamingInformation& dif_name,
		bool read_lock)
{

	//Prevent any insertion/deletion to happen
	rina::ReadScopedLock readlock(ipcps_rwlock);

	const vector<rina::IPCProcess *>& ipcps =
		rina::ipcProcessFactory->listIPCProcesses();

	for (unsigned int i = 0; i < ipcps.size(); i++) {
		rina::DIFInformation dif_info = ipcps[i]->getDIFInformation();
		rina::ApplicationProcessNamingInformation ipcp_name = ipcps[i]->name;

		if (dif_info.dif_name_.processName == dif_name.processName
				/* The following OR clause is a temporary hack useful
				 * for testing with shim dummy. TODO It will go away. */
				|| ipcp_name.processName == dif_name.processName) {
			//FIXME: rwlock over ipcp
			return ipcps[i];
		}
	}

	return NULL;
}

// Returns any IPC process in the system, giving priority to
// normal IPC processes.
rina::IPCProcess *
IPCManager_::select_ipcp(bool read_lock)
{

	//Prevent any insertion/deletion to happen
	rina::ReadScopedLock readlock(ipcps_rwlock);

	const vector<rina::IPCProcess *>& ipcps =
		rina::ipcProcessFactory->listIPCProcesses();

	for (unsigned int i = 0; i < ipcps.size(); i++) {
		if (ipcps[i]->type == rina::NORMAL_IPC_PROCESS) {
			//FIXME: rwlock over ipcp
			return ipcps[i];
		}
	}

	for (unsigned int i = 0; i < ipcps.size(); i++) {
		//FIXME: rwlock over ipcp
		return ipcps[i];
	}

	return NULL;
}

bool
IPCManager_::application_is_registered_to_ipcp(
		const rina::ApplicationProcessNamingInformation& app_name,
		rina::IPCProcess *slave_ipcp)
{
	//Prevent any insertion/deletion to happen
	rina::ReadScopedLock readlock(ipcps_rwlock);

	const list<rina::ApplicationProcessNamingInformation>&
		registered_apps = slave_ipcp->getRegisteredApplications();

	for (list<rina::ApplicationProcessNamingInformation>::const_iterator
			it = registered_apps.begin();
				it != registered_apps.end(); it++) {
		if (app_name == *it) {
			return true;
		}
	}

	return false;
}

rina::IPCProcess *
IPCManager_::lookup_ipcp_by_port(unsigned int port_id, bool read_lock)
{
	//Prevent any insertion/deletion to happen
	rina::ReadScopedLock readlock(ipcps_rwlock);

	const vector<rina::IPCProcess *>& ipcps =
		rina::ipcProcessFactory->listIPCProcesses();
	rina::FlowInformation info;

	for (unsigned int i = 0; i < ipcps.size(); i++) {
		if (ipcps[i]->getFlowInformation(port_id, info)) {
			//FIXME: rwlock over ipcp
			return ipcps[i];
		}
	}

	return NULL;
}

void
IPCManager_::collect_flows_by_application(
			const rina::ApplicationProcessNamingInformation&
			app_name, list<rina::FlowInformation>& result)
{
	//Prevent any insertion/deletion to happen
	rina::ReadScopedLock readlock(ipcps_rwlock);

	const vector<rina::IPCProcess *>& ipcps =
		rina::ipcProcessFactory->listIPCProcesses();

	result.clear();

	for (unsigned int i = 0; i < ipcps.size(); i++) {
		const list<rina::FlowInformation>& flows =
			ipcps[i]->getAllocatedFlows();

		for (list<rina::FlowInformation>::const_iterator it =
			flows.begin(); it != flows.end(); it++) {
			if (it->localAppName == app_name) {
				result.push_back(*it);
			}
		}
	}
}

rina::IPCProcess *
IPCManager_::lookup_ipcp_by_id(unsigned int id, bool read_lock)
{
	rina::IPCProcess* ipcp;

	//Prevent any insertion/deletion to happen
	rina::ReadScopedLock readlock(ipcps_rwlock);


	ipcp = rina::ipcProcessFactory->getIPCProcess(id);
	//FIXME:lock over ipcp

	return ipcp;
}

} //rinad namespace
