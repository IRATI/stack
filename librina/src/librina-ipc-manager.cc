/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "librina-ipc-manager.h"
#define RINA_PREFIX "ipc-manager"
#include "logs.h"

using namespace rina;

/** CLASS IPC PROCESS */

IPCProcess::IPCProcess(unsigned int id, DIFType type,
		const ApplicationProcessNamingInformation& name) {
	this->id = id;
	this->type = type;
	this->name = name;
}

unsigned int IPCProcess::getId() const {
	return id;
}

DIFType IPCProcess::getType() const {
	return type;
}

const ApplicationProcessNamingInformation& IPCProcess::getName() const {
	return name;
}

void IPCProcess::assignToDIF(unsigned int ipcProcessId,
		const DIFConfiguration& difConfiguration) throw (IPCException) {
	LOG_DBG("IPCProcess::assign to DIF called");
}

void IPCProcess::notifyRegistrationToSupportingDIF(
		const ApplicationProcessNamingInformation& difName) throw (IPCException) {
	LOG_DBG("IPCProcess::notify registration to supporting DIF called");
	throw IPCException(IPCException::operation_not_implemented_error);
}

void IPCProcess::notifyUnregistrationFromSupportingDIF(
		const ApplicationProcessNamingInformation& difName) throw (IPCException) {
	LOG_DBG("IPCProcess::notify unregistration from supporting DIF called");
	throw IPCException(IPCException::operation_not_implemented_error);
}

void IPCProcess::enroll(const ApplicationProcessNamingInformation& difName,
		const ApplicationProcessNamingInformation& supportinDifName)
				throw (IPCException) {
	LOG_DBG("IPCProcess::enroll called");
	throw IPCException(IPCException::operation_not_implemented_error);
}

void IPCProcess::disconnectFromNeighbor(
		const ApplicationProcessNamingInformation& neighbor)
				throw (IPCException) {
	LOG_DBG("IPCProcess::disconnect from neighbour called");
	throw IPCException(IPCException::operation_not_implemented_error);
}

void IPCProcess::registerApplication(
		const ApplicationProcessNamingInformation& applicationName)
				throw (IPCException) {
	LOG_DBG("IPCProcess::register application called");
}

void IPCProcess::unregisterApplication(
		const ApplicationProcessNamingInformation& applicationName)
				throw (IPCException) {
	LOG_DBG("IPCProcess::unregister application called");
}

void IPCProcess::allocateFlow(const FlowRequest& flowRequest)
		throw (IPCException) {
	LOG_DBG("IPCProcess::allocate flow called");
}

void IPCProcess::queryRIB(){
	LOG_DBG("IPCProcess::query RIB called");
}

/** CLASS IPC PROCESS FACTORY */

const std::string IPCProcessFactory::unknown_ipc_process_error =
		"Could not find an IPC Process with the provided id";

IPCProcess * IPCProcessFactory::create(
			const ApplicationProcessNamingInformation& ipcProcessName,
			DIFType difType) throw (IPCException){
	LOG_DBG("IPCProcessFactory::create called");

	int ipcProcessId;
	for (int i = 1; i < 1000; i++) {
		if (ipcProcesses.find(i) == ipcProcesses.end()) {
			ipcProcessId = i;
			break;
		}
	}

	IPCProcess * ipcProcess = new IPCProcess(ipcProcessId, difType, ipcProcessName);
	ipcProcesses[ipcProcessId] = ipcProcess;
	return ipcProcess;
}

void IPCProcessFactory::destroy(unsigned int ipcProcessId) throw (IPCException){
	std::map<int, IPCProcess*>::iterator iterator;
	iterator = ipcProcesses.find(ipcProcessId);
	if (iterator == ipcProcesses.end()) {
		throw new IPCException(IPCProcessFactory::unknown_ipc_process_error);
	}

	ipcProcesses.erase(ipcProcessId);
}

std::vector<IPCProcess *> IPCProcessFactory::listIPCProcesses(){
	LOG_DBG("IPCProcessFactory::list IPC Processes called");
	std::vector<IPCProcess *> response;

	for (std::map<int, IPCProcess*>::iterator it = ipcProcesses.begin();
			it != ipcProcesses.end(); ++it) {
		response.push_back(it->second);
	}

	return response;
}
