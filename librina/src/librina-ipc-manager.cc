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

#define RINA_PREFIX "ipc-manager"

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <iostream>

#include "logs.h"
#include "librina-ipc-manager.h"
#include "core.h"
#include "concurrency.h"
#include "rina-syscalls.h"

namespace rina{

/* CLASS IPC PROCESS*/
const std::string IPCProcess::error_assigning_to_dif =
		"Error assigning IPC Process to DIF";
const std::string IPCProcess::error_registering_app =
		"Error registering application";
const std::string IPCProcess::error_unregistering_app =
		"Error unregistering application";
const std::string IPCProcess::error_not_a_dif_member =
		"Error: the IPC Process is not member of a DIF";
const std::string IPCProcess::error_allocating_flow =
		"Error allocating flow";
const std::string IPCProcess::error_querying_rib =
		"Error querying rib";

IPCProcess::IPCProcess() {
	id = 0;
	portId = 0;
	difMember = false;
}

IPCProcess::IPCProcess(unsigned short id,
		unsigned int portId, const std::string& type,
		const ApplicationProcessNamingInformation& name) {
	this->id = id;
	this->portId = portId;
	this->type = type;
	this->name = name;
	difMember = false;
}

bool IPCProcess::isDIFMember() const{
	return difMember;
}

void IPCProcess::setDIFMember(bool difMember){
	this->difMember = difMember;
}

unsigned int IPCProcess::getId() const {
	return id;
}

const std::string& IPCProcess::getType() const {
	return type;
}

const ApplicationProcessNamingInformation& IPCProcess::getName() const {
	return name;
}

unsigned int IPCProcess::getPortId() const{
	return portId;
}

void IPCProcess::setPortId(unsigned int portId){
	this->portId = portId;
}

const DIFConfiguration& IPCProcess::getConfiguration() const{
	return difConfiguration;
}

void IPCProcess::setConfiguration(const DIFConfiguration& difConfiguration){
	this->difConfiguration = difConfiguration;
}

void IPCProcess::assignToDIF(
		const DIFConfiguration& difConfiguration) throw (AssignToDIFException) {
	LOG_DBG("IPCProcess::assign to DIF called");
#if STUB_API
	//Do nothing
#else
	IpcmAssignToDIFRequestMessage message;
	message.setDIFConfiguration(difConfiguration);
	message.setDestIpcProcessId(id);
	message.setDestPortId(portId);
	message.setRequestMessage(true);

	IpcmAssignToDIFResponseMessage * assignToDIFResponse;
	try{
		assignToDIFResponse =
				dynamic_cast<IpcmAssignToDIFResponseMessage *>(
						rinaManager->sendRequestAndWaitForResponse(&message,
								IPCProcess::error_assigning_to_dif));
	}catch(NetlinkException &e){
		throw AssignToDIFException(e.what());
	}

	if (assignToDIFResponse->getResult() < 0){
		std::string reason = IPCProcess::error_assigning_to_dif + " " +
				assignToDIFResponse->getErrorDescription();
		delete assignToDIFResponse;
		throw AssignToDIFException(reason);
	}

	LOG_DBG("Assigned IPC Process %d to DIF %s", id,
			difConfiguration.getDifName().getProcessName().c_str());
	delete assignToDIFResponse;
#endif

	this->difConfiguration = difConfiguration;
	this->difMember = true;
}

void IPCProcess::notifyRegistrationToSupportingDIF(
		const ApplicationProcessNamingInformation& ipcProcessName,
		const ApplicationProcessNamingInformation& difName)
throw (NotifyRegistrationToDIFException) {
	LOG_DBG("IPCProcess::notify registration to supporting DIF called");
#if STUB_API
	//Do nothing
#else
	IpcmDIFRegistrationNotification message;
	message.setIpcProcessName(ipcProcessName);
	message.setDestIpcProcessId(id);
	message.setDestPortId(portId);
	message.setDifName(difName);
	message.setRegistered(true);
	message.setNotificationMessage(true);

	try{
		rinaManager->sendResponseOrNotficationMessage(&message);
	}catch(NetlinkException &e){
		throw NotifyRegistrationToDIFException(e.what());
	}
#endif
}

void IPCProcess::notifyUnregistrationFromSupportingDIF(
		const ApplicationProcessNamingInformation& ipcProcessName,
		const ApplicationProcessNamingInformation& difName)
throw (NotifyUnregistrationFromDIFException) {
	LOG_DBG("IPCProcess::notify unregistration from supporting DIF called");
#if STUB_API
	//Do nothing
#else
	IpcmDIFRegistrationNotification message;
	message.setIpcProcessName(ipcProcessName);
	message.setDestIpcProcessId(id);
	message.setDestPortId(portId);
	message.setDifName(difName);
	message.setRegistered(false);
	message.setNotificationMessage(true);

	try{
		rinaManager->sendResponseOrNotficationMessage(&message);
	}catch(NetlinkException &e){
		throw NotifyUnregistrationFromDIFException(e.what());
	}
#endif
}

void IPCProcess::enroll(const ApplicationProcessNamingInformation& difName,
		const ApplicationProcessNamingInformation& supportinDifName)
throw (EnrollException) {
	LOG_DBG("IPCProcess::enroll called");
	throw IPCException(IPCException::operation_not_implemented_error);
}

void IPCProcess::disconnectFromNeighbor(
		const ApplicationProcessNamingInformation& neighbor)
throw (DisconnectFromNeighborException) {
	LOG_DBG("IPCProcess::disconnect from neighbour called");
	throw IPCException(IPCException::operation_not_implemented_error);
}

void IPCProcess::registerApplication(
		const ApplicationProcessNamingInformation& applicationName)
throw (IpcmRegisterApplicationException) {
	LOG_DBG("IPCProcess::register application called");
	if (!difMember){
		throw IPCException(IPCProcess::error_not_a_dif_member);
	}
#if STUB_API
	//Do nothing
#else
	IpcmRegisterApplicationRequestMessage message;
	message.setApplicationName(applicationName);
	message.setDifName(difConfiguration.getDifName());
	message.setDestIpcProcessId(id);
	message.setDestPortId(portId);
	message.setRequestMessage(true);

	IpcmRegisterApplicationResponseMessage * registerAppResponse;
	try{
		registerAppResponse =
				dynamic_cast<IpcmRegisterApplicationResponseMessage *>(
						rinaManager->sendRequestAndWaitForResponse(&message,
								IPCProcess::error_registering_app));
	}catch(NetlinkException &e){
		throw IpcmRegisterApplicationException(e.what());
	}

	if (registerAppResponse->getResult() < 0){
		std::string reason = IPCProcess::error_registering_app + " " +
				registerAppResponse->getErrorDescription();
		delete registerAppResponse;
		throw IpcmRegisterApplicationException(reason);
	}

	LOG_DBG("Registered app %s to DIF %s",
			applicationName.getProcessName().c_str(),
			difConfiguration.getDifName().getProcessName().c_str());
	delete registerAppResponse;
#endif
}

void IPCProcess::unregisterApplication(
		const ApplicationProcessNamingInformation& applicationName)
throw (IpcmUnregisterApplicationException) {
	LOG_DBG("IPCProcess::unregister application called");
#if STUB_API
	//Do nothing
#else
	IpcmUnregisterApplicationRequestMessage message;
	message.setApplicationName(applicationName);
	message.setDifName(difConfiguration.getDifName());
	message.setDestIpcProcessId(id);
	message.setDestPortId(portId);
	message.setRequestMessage(true);

	IpcmUnregisterApplicationResponseMessage * unregisterAppResponse;
	try{
		unregisterAppResponse =
				dynamic_cast<IpcmUnregisterApplicationResponseMessage *>(
						rinaManager->sendRequestAndWaitForResponse(&message,
								IPCProcess::error_unregistering_app));
	}catch(NetlinkException &e){
		throw IpcmUnregisterApplicationException(e.what());
	}

	if (unregisterAppResponse->getResult() < 0){
		std::string reason = IPCProcess::error_unregistering_app + " " +
				unregisterAppResponse->getErrorDescription();
		delete unregisterAppResponse;
		throw IpcmUnregisterApplicationException(reason);
	}
	LOG_DBG("Unregistered app %s from DIF %s",
			applicationName.getProcessName().c_str(),
			difConfiguration.getDifName().getProcessName().c_str());
	delete unregisterAppResponse;
#endif
}

void IPCProcess::allocateFlow(const FlowRequestEvent& flowRequest)
throw (AllocateFlowException) {
	LOG_DBG("IPCProcess::allocate flow called");
	if (!difMember){
		throw IPCException(IPCProcess::error_not_a_dif_member);
	}
#if STUB_API
	//Do nothing
#else
	IpcmAllocateFlowRequestMessage message;
	message.setSourceAppName(flowRequest.getLocalApplicationName());
	message.setDestAppName(flowRequest.getRemoteApplicationName());
	message.setFlowSpec(flowRequest.getFlowSpecification());
	message.setDifName(flowRequest.getDIFName());
	message.setPortId(flowRequest.getPortId());
	message.setDestIpcProcessId(id);
	message.setDestPortId(portId);
	message.setRequestMessage(true);

	IpcmAllocateFlowResponseMessage * allocateFlowResponse;
	try{
		allocateFlowResponse =
				dynamic_cast<IpcmAllocateFlowResponseMessage *>(
						rinaManager->sendRequestAndWaitForResponse(&message,
								IPCProcess::error_allocating_flow));
	}catch(NetlinkException &e){
		throw AllocateFlowException(e.what());
	}

	if (allocateFlowResponse->getResult() < 0){
		std::string reason = IPCProcess::error_allocating_flow + " " +
				allocateFlowResponse->getErrorDescription();
		delete allocateFlowResponse;
		throw AllocateFlowException(reason);
	}

	LOG_DBG("Allocated flow from %s to %s with portId %d",
			message.getSourceAppName().getProcessName().c_str(),
			message.getDestAppName().getProcessName().c_str(),
			message.getPortId());
	delete allocateFlowResponse;

#endif
}

const std::list<RIBObject> IPCProcess::queryRIB(const std::string& objectClass,
		const std::string& objectName, unsigned long objectInstance,
		unsigned int scope, const std::string& filter)
			throw (QueryRIBException){
	LOG_DBG("IPCProcess::query RIB called");
#if STUB_API
	std::list<RIBObject> ribObjects;
	return ribObjects;
#else
	IpcmDIFQueryRIBRequestMessage message;
	message.setObjectClass(objectClass);
	message.setObjectName(objectName);
	message.setObjectInstance(objectInstance);
	message.setScope(scope);
	message.setFilter(filter);
	message.setDestIpcProcessId(id);
	message.setDestPortId(portId);
	message.setRequestMessage(true);

	IpcmDIFQueryRIBResponseMessage * queryRIBResponse;
	try{
		queryRIBResponse =
				dynamic_cast<IpcmDIFQueryRIBResponseMessage *>(
						rinaManager->sendRequestAndWaitForResponse(&message,
								IPCProcess::error_querying_rib));
	}catch(NetlinkException &e){
		throw QueryRIBException(e.what());
	}

	if (queryRIBResponse->getResult() < 0){
		std::string reason = IPCProcess::error_querying_rib + " " +
				queryRIBResponse->getErrorDescription();
		delete queryRIBResponse;
		throw QueryRIBException(reason);
	}

	LOG_DBG("Queried RIB of IPC Process %d; got %d objects",
			id, queryRIBResponse->getRIBObjects().size());
	std::list<RIBObject> ribObjects = queryRIBResponse->getRIBObjects();
	delete queryRIBResponse;
	return ribObjects;
#endif
}

/** CLASS IPC PROCESS FACTORY */
const std::string IPCProcessFactory::unknown_ipc_process_error =
		"Could not find an IPC Process with the provided id";
const std::string IPCProcessFactory::path_to_ipc_process_types =
		"/sys/rina/personalities/default/shims/";
const std::string IPCProcessFactory::normal_ipc_process_type =
		"normal";

IPCProcessFactory::IPCProcessFactory(): Lockable(){
}

IPCProcessFactory::~IPCProcessFactory() throw(){
}

std::list<std::string> IPCProcessFactory::getSupportedIPCProcessTypes(){
	std::list<std::string> result;
	result.push_back(normal_ipc_process_type);

	DIR *dp;
	struct dirent *dirp;
	if((dp = opendir(path_to_ipc_process_types.c_str())) == 0) {
		LOG_ERR("Error %d opening %s", errno,
				path_to_ipc_process_types.c_str());
		return result;
	}

	std::string name;
	while ((dirp = readdir(dp)) != 0) {
		name = std::string(dirp->d_name);
		if (name.compare(".") != 0 && name.compare("..") != 0){
			result.push_back(name);
		}
	}

	closedir(dp);

	return result;
}

IPCProcess * IPCProcessFactory::create(
		const ApplicationProcessNamingInformation& ipcProcessName,
		const std::string& difType) throw (CreateIPCProcessException) {
	LOG_DBG("IPCProcessFactory::create called");

	lock();
	int ipcProcessId = 1;
	for (int i = 1; i < 1000; i++) {
		if (ipcProcesses.find(i) == ipcProcesses.end()) {
			ipcProcessId = i;
			break;
		}
	}

#if STUB_API
	//Do nothing
#else
	int result = syscallCreateIPCProcess(
			ipcProcessName, ipcProcessId, difType);
	if (result != 0){
		throw CreateIPCProcessException();
	}
#endif

	IPCProcess * ipcProcess = new IPCProcess(ipcProcessId, 0, difType,
			ipcProcessName);
	ipcProcesses[ipcProcessId] = ipcProcess;
	unlock();

	return ipcProcess;
}

void IPCProcessFactory::destroy(unsigned int ipcProcessId)
throw (DestroyIPCProcessException) {
	LOG_DBG("IPCProcessFactory::destroy called");
	lock();

	std::map<int, IPCProcess*>::iterator iterator;
	iterator = ipcProcesses.find(ipcProcessId);
	if (iterator == ipcProcesses.end()) {
		throw IPCException(IPCProcessFactory::unknown_ipc_process_error);
	}

#if STUB_API
	//Do nothing
#else
	int result = syscallDestroyIPCProcess(ipcProcessId);
	if (result != 0){
		throw DestroyIPCProcessException();
	}
#endif

	delete iterator->second;
	ipcProcesses.erase(ipcProcessId);

	unlock();
}

std::vector<IPCProcess *> IPCProcessFactory::listIPCProcesses() {
	LOG_DBG("IPCProcessFactory::list IPC Processes called");
	std::vector<IPCProcess *> response;

	lock();
	for (std::map<int, IPCProcess*>::iterator it = ipcProcesses.begin();
			it != ipcProcesses.end(); ++it) {
		response.push_back(it->second);
	}
	unlock();

	return response;
}

Singleton<IPCProcessFactory> ipcProcessFactory;

/** CLASS APPLICATION MANAGER */

void ApplicationManager::applicationRegistered(
		const ApplicationRegistrationRequestEvent& event,
		const ApplicationProcessNamingInformation& difName, int result,
		const std::string& errorDescription)
			throw (NotifyApplicationRegisteredException) {
	LOG_DBG("ApplicationManager::applicationRegistered called");

#if STUB_API
	//Do nothing
#else
	AppRegisterApplicationResponseMessage responseMessage;
	responseMessage.setApplicationName(event.getApplicationName());
	responseMessage.setDifName(difName);
	responseMessage.setResult(result);
	responseMessage.setErrorDescription(errorDescription);
	responseMessage.setSequenceNumber(event.getSequenceNumber());
	responseMessage.setResponseMessage(true);
	try{
		rinaManager->sendResponseOrNotficationMessage(&responseMessage);
	}catch(NetlinkException &e){
		throw NotifyApplicationRegisteredException(e.what());
	}
#endif
}

void ApplicationManager::applicationUnregistered(
		const ApplicationUnregistrationRequestEvent& event,
		int result, const std::string& errorDescription)
			throw (NotifyApplicationUnregisteredException) {
	LOG_DBG("ApplicationManager::applicationUnregistered called");

#if STUB_API
	//Do nothing
#else
	AppUnregisterApplicationResponseMessage responseMessage;
	responseMessage.setApplicationName(event.getApplicationName());
	responseMessage.setResult(result);
	responseMessage.setErrorDescription(errorDescription);
	responseMessage.setSequenceNumber(event.getSequenceNumber());
	responseMessage.setResponseMessage(true);
	try{
		rinaManager->sendResponseOrNotficationMessage(&responseMessage);
	}catch(NetlinkException &e){
		throw NotifyApplicationUnregisteredException(e.what());
	}
#endif
}

void ApplicationManager::flowAllocated(const FlowRequestEvent& flowRequestEvent,
		std::string errorDescription) throw (NotifyFlowAllocatedException) {
	LOG_DBG("ApplicationManager::flowAllocated called");

#if STUB_API
	//Do nothing
#else
	AppAllocateFlowRequestResultMessage responseMessage;
	responseMessage.setPortId(flowRequestEvent.getPortId());
	responseMessage.setErrorDescription(errorDescription);
	responseMessage.setSourceAppName(flowRequestEvent.getLocalApplicationName());
	responseMessage.setDifName(flowRequestEvent.getDIFName());
	responseMessage.setSequenceNumber(flowRequestEvent.getSequenceNumber());
	responseMessage.setResponseMessage(true);
	try{
		rinaManager->sendResponseOrNotficationMessage(&responseMessage);
	}catch(NetlinkException &e){
		throw NotifyFlowAllocatedException(e.what());
	}
#endif
}

void ApplicationManager::getDIFPropertiesResponse(
		const GetDIFPropertiesRequestEvent &event,
			int result, const std::string& errorDescription,
			const std::list<DIFProperties>& difProperties)
			throw (GetDIFPropertiesResponseException){
#if STUB_API
	//Do nothing
#else
	AppGetDIFPropertiesResponseMessage responseMessage;
	responseMessage.setResult(result);
	responseMessage.setErrorDescription(errorDescription);
	responseMessage.setApplicationName(event.getApplicationName());
	responseMessage.setDIFProperties(difProperties);
	responseMessage.setSequenceNumber(event.getSequenceNumber());
	responseMessage.setResponseMessage(true);
	try{
		rinaManager->sendResponseOrNotficationMessage(&responseMessage);
	}catch(NetlinkException &e){
		throw GetDIFPropertiesResponseException(e.what());
	}
#endif
}

Singleton<ApplicationManager> applicationManager;

/* CLASS GET DIF PROPERTIES REQUEST EVENT */
GetDIFPropertiesRequestEvent::GetDIFPropertiesRequestEvent(
		const ApplicationProcessNamingInformation& appName,
		const ApplicationProcessNamingInformation& DIFName,
		unsigned int sequenceNumber):
				IPCEvent(GET_DIF_PROPERTIES, sequenceNumber){
	this->applicationName = appName;
	this->DIFName = DIFName;
}

const ApplicationProcessNamingInformation&
	GetDIFPropertiesRequestEvent::getApplicationName() const{
	return applicationName;
}
const ApplicationProcessNamingInformation&
	GetDIFPropertiesRequestEvent::getDIFName() const{
	return DIFName;
}

}
