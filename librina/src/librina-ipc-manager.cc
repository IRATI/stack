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

/** CLASS IPC PROCESS */

namespace rina{

IPCProcess::IPCProcess() {
	id = 0;
	type = DIF_TYPE_NORMAL;
}

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

void IPCProcess::assignToDIF(const DIFConfiguration& difConfiguration)
		throw (IPCException) {
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

void IPCProcess::queryRIB() {
	LOG_DBG("IPCProcess::query RIB called");
}

/** CLASS IPC PROCESS FACTORY */

const std::string IPCProcessFactory::unknown_ipc_process_error =
		"Could not find an IPC Process with the provided id";

IPCProcess * IPCProcessFactory::create(
		const ApplicationProcessNamingInformation& ipcProcessName,
		DIFType difType) throw (IPCException) {
	LOG_DBG("IPCProcessFactory::create called");

	int ipcProcessId;
	for (int i = 1; i < 1000; i++) {
		if (ipcProcesses.find(i) == ipcProcesses.end()) {
			ipcProcessId = i;
			break;
		}
	}

	IPCProcess * ipcProcess = new IPCProcess(ipcProcessId, difType,
			ipcProcessName);
	ipcProcesses[ipcProcessId] = ipcProcess;
	return ipcProcess;
}

void IPCProcessFactory::destroy(unsigned int ipcProcessId) throw (IPCException) {
	std::map<int, IPCProcess*>::iterator iterator;
	iterator = ipcProcesses.find(ipcProcessId);
	if (iterator == ipcProcesses.end()) {
		throw IPCException(IPCProcessFactory::unknown_ipc_process_error);
	}

	delete iterator->second;
	ipcProcesses.erase(ipcProcessId);
}

std::vector<IPCProcess *> IPCProcessFactory::listIPCProcesses() {
	LOG_DBG("IPCProcessFactory::list IPC Processes called");
	std::vector<IPCProcess *> response;

	for (std::map<int, IPCProcess*>::iterator it = ipcProcesses.begin();
			it != ipcProcesses.end(); ++it) {
		response.push_back(it->second);
	}

	return response;
}

Singleton<IPCProcessFactory> ipcProcessFactory;

/** CLASS APPLICATION MANAGER */

void ApplicationManager::applicationRegistered(unsigned int transactionId,
		const std::string& response) throw (IPCException) {
	LOG_DBG("ApplicationManager::applicationRegistered called");
}

void ApplicationManager::applicationUnregistered(unsigned int transactionId,
		const std::string& response) throw (IPCException) {
	LOG_DBG("ApplicationManager::applicationUnregistered called");
}

void ApplicationManager::flowAllocated(unsigned int transactionId, int portId,
		unsigned int ipcProcessPid, std::string response,
		const ApplicationProcessNamingInformation& difName) throw (IPCException) {
	LOG_DBG("ApplicationManager::flowAllocated called");
}

Singleton<ApplicationManager> applicationManager;

/** CLASS APPLICATION REGISTRATION REQUEST */

ApplicationRegistrationRequestEvent::ApplicationRegistrationRequestEvent(
		const ApplicationProcessNamingInformation& appName,
		const ApplicationProcessNamingInformation& DIFName,
		unsigned int transactionId) :
		IPCEvent(APPLICATION_REGISTRATION_REQUEST_EVENT) {
	this->applicationName = appName;
	this->DIFName = DIFName;
	this->transactionId = transactionId;
}

const ApplicationProcessNamingInformation&
ApplicationRegistrationRequestEvent::getApplicationName() const {
	return applicationName;
}

const ApplicationProcessNamingInformation&
ApplicationRegistrationRequestEvent::getDIFName() const {
	return DIFName;
}

unsigned int ApplicationRegistrationRequestEvent::getTransactionId() const {
	return transactionId;
}

/** CLASS APPLICATION UNREGISTRATION REQUEST */

ApplicationUnregistrationRequestEvent::ApplicationUnregistrationRequestEvent(
		const ApplicationProcessNamingInformation& appName,
		const ApplicationProcessNamingInformation& DIFName,
		unsigned int transactionId) :
		IPCEvent(APPLICATION_UNREGISTRATION_REQUEST_EVENT) {
	this->applicationName = appName;
	this->DIFName = DIFName;
	this->transactionId = transactionId;
}

const ApplicationProcessNamingInformation&
ApplicationUnregistrationRequestEvent::getApplicationName() const {
	return applicationName;
}

const ApplicationProcessNamingInformation&
ApplicationUnregistrationRequestEvent::getDIFName() const {
	return DIFName;
}

unsigned int ApplicationUnregistrationRequestEvent::getTransactionId() const {
	return transactionId;
}

/** CLASS FLOW ALLOCATION REQUEST */

FlowAllocationRequestEvent::FlowAllocationRequestEvent(
		const ApplicationProcessNamingInformation& sourceName,
		const ApplicationProcessNamingInformation& destName,
		const FlowSpecification& flowSpec, unsigned int transactionId) :
		IPCEvent(FLOW_ALLOCATION_REQUEST_EVENT) {
	this->sourceApplicationName = sourceName;
	this->destinationApplicationName = destName;
	this->flowSpecification = flowSpec;
	this->transactionId = transactionId;
}

const ApplicationProcessNamingInformation&
FlowAllocationRequestEvent::getSourceApplicationName() const {
	return sourceApplicationName;
}

const ApplicationProcessNamingInformation&
FlowAllocationRequestEvent::getDestinationApplicationName() const {
	return destinationApplicationName;
}

const FlowSpecification&
FlowAllocationRequestEvent::getFlowSpecification() const {
	return flowSpecification;
}

unsigned int FlowAllocationRequestEvent::getTransactionId() const {
	return transactionId;
}

}
