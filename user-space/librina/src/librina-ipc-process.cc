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

#define RINA_PREFIX "ipc-process"

#include "logs.h"
#include "librina-ipc-process.h"
#include "core.h"

namespace rina{

/* CLASS FLOW DEALLOCATE REQUEST EVENT */
FlowDeallocateRequestEvent::FlowDeallocateRequestEvent(int portId,
			const ApplicationProcessNamingInformation& DIFName,
			const ApplicationProcessNamingInformation& appName,
			unsigned short ipcProcessId, unsigned int sequenceNumber):
						IPCEvent(FLOW_DEALLOCATION_REQUESTED_EVENT,
								sequenceNumber){
	this->portId = portId;
	this->DIFName = DIFName;
	this->ipcProcessId = ipcProcessId;
	this->applicationName = appName;
}

int FlowDeallocateRequestEvent::getPortId() const{
	return portId;
}

const ApplicationProcessNamingInformation&
	FlowDeallocateRequestEvent::getDIFName() const{
	return DIFName;
}

const ApplicationProcessNamingInformation&
	FlowDeallocateRequestEvent::getApplicationName() const{
	return applicationName;
}

unsigned short FlowDeallocateRequestEvent::getIPCProcessId() const{
	return ipcProcessId;
}

/* CLASS ASSIGN TO DIF REQUEST EVENT */
AssignToDIFRequestEvent::AssignToDIFRequestEvent(
		const DIFConfiguration& difConfiguration,
			unsigned int sequenceNumber):
		IPCEvent(ASSIGN_TO_DIF_REQUEST_EVENT,
										sequenceNumber){
}

const DIFConfiguration&
AssignToDIFRequestEvent::getDIFConfiguration() const{
	return difConfiguration;
}

/* CLASS IPC PROCESS DIF REGISTRATION EVENT */
IPCProcessDIFRegistrationEvent::IPCProcessDIFRegistrationEvent(
		IPCEventType eventType,
		const ApplicationProcessNamingInformation& ipcProcessName,
		const ApplicationProcessNamingInformation& difName,
		unsigned int sequenceNumber): IPCEvent(eventType, sequenceNumber){
}

const ApplicationProcessNamingInformation&
IPCProcessDIFRegistrationEvent::getIPCProcessName() const{
	return ipcProcessName;
}

const ApplicationProcessNamingInformation&
IPCProcessDIFRegistrationEvent::getDIFName() const{
	return difName;
}

/* CLASS IPC PROCESS REGISTERED TO DIF EVENT */
IPCProcessRegisteredToDIFEvent::IPCProcessRegisteredToDIFEvent(
		const ApplicationProcessNamingInformation& ipcProcessName,
		const ApplicationProcessNamingInformation& difName,
		unsigned int sequenceNumber): IPCProcessDIFRegistrationEvent(
				IPC_PROCESS_REGISTERED_TO_DIF, ipcProcessName,
				difName, sequenceNumber){
}

/* CLASS IPC PROCESS UNREGISTERED FROM DIF EVENT */
IPCProcessUnregisteredFromDIFEvent::IPCProcessUnregisteredFromDIFEvent(
		const ApplicationProcessNamingInformation& ipcProcessName,
		const ApplicationProcessNamingInformation& difName,
		unsigned int sequenceNumber): IPCProcessDIFRegistrationEvent(
				IPC_PROCESS_UNREGISTERED_FROM_DIF, ipcProcessName,
				difName, sequenceNumber){
}

/* CLASS QUERY RIB REQUEST EVENT */
QueryRIBRequestEvent::QueryRIBRequestEvent(const std::string& objectClass,
		const std::string& objectName, long objectInstance,
		int scope, const std::string& filter,
		unsigned int sequenceNumber):
				IPCEvent(IPC_PROCESS_QUERY_RIB, sequenceNumber){
	this->objectClass = objectClass;
	this->objectName = objectName;
	this->objectInstance = objectInstance;
	this->scope = scope;
	this->filter = filter;
}

const std::string& QueryRIBRequestEvent::getObjectClass() const{
	return objectClass;
}

const std::string& QueryRIBRequestEvent::getObjectName() const{
	return objectName;
}

long QueryRIBRequestEvent::getObjectInstance() const{
	return objectInstance;
}

int QueryRIBRequestEvent::getScope() const{
	return scope;
}

const std::string& QueryRIBRequestEvent::getFilter() const{
	return filter;
}

/* CLASS EXTENDED IPC MANAGER */
const DIFConfiguration& ExtendedIPCManager::getCurrentConfiguration() const{
	return currentConfiguration;
}

void ExtendedIPCManager::setCurrentConfiguration(
		const DIFConfiguration& currentConfiguration){
	this->currentConfiguration = currentConfiguration;
}

unsigned int ExtendedIPCManager::getIpcProcessId() const{
	return ipcProcessId;
}

void ExtendedIPCManager::setIpcProcessId(unsigned int ipcProcessId){
	this->ipcProcessId = ipcProcessId;
}

void ExtendedIPCManager::assignToDIFResponse(
		const AssignToDIFRequestEvent& event, int result,
		const std::string& errorDescription) throw(IPCException){
	if (result == 0){
		this->currentConfiguration = event.getDIFConfiguration();
	}
#if STUB_API
	//Do nothing
#else
	IpcmAssignToDIFResponseMessage responseMessage;
	responseMessage.setResult(result);
	responseMessage.setErrorDescription(errorDescription);
	responseMessage.setSequenceNumber(event.getSequenceNumber());
	responseMessage.setResponseMessage(true);
	try{
		rinaManager->sendResponseOrNotficationMessage(&responseMessage);
	}catch(NetlinkException &e){
		throw IPCException(e.what());
	}
#endif
}

void ExtendedIPCManager::registerApplicationResponse(
		const ApplicationRegistrationRequestEvent& event, int result,
		const std::string& errorDescription) throw(IPCException){
#if STUB_API
	//Do nothing
#else
	IpcmRegisterApplicationResponseMessage responseMessage;
	responseMessage.setResult(result);
	responseMessage.setErrorDescription(errorDescription);
	responseMessage.setSequenceNumber(event.getSequenceNumber());
	responseMessage.setResponseMessage(true);
	try{
		rinaManager->sendResponseOrNotficationMessage(&responseMessage);
	}catch(NetlinkException &e){
		throw IPCException(e.what());
	}
#endif
}

void ExtendedIPCManager::allocateFlowResponse(
		const FlowRequestEvent& event, int result,
		const std::string& errorDescription) throw(IPCException){
#if STUB_API
	//Do nothing
#else
	IpcmAllocateFlowResponseMessage responseMessage;
	responseMessage.setResult(result);
	responseMessage.setErrorDescription(errorDescription);
	responseMessage.setSequenceNumber(event.getSequenceNumber());
	responseMessage.setResponseMessage(true);
	try{
		rinaManager->sendResponseOrNotficationMessage(&responseMessage);
	}catch(NetlinkException &e){
		throw IPCException(e.what());
	}
#endif
}

Singleton<ExtendedIPCManager> extendedIPCManager;

/* CLASS IPC PROCESS APPLICATION MANAGER */
void IPCProcessApplicationManager::flowDeallocated(
		const FlowDeallocateRequestEvent flowDeallocateEvent,
			int result, std::string errorDescription) throw (IPCException){
#if STUB_API
	//Do nothing
#else
	AppDeallocateFlowResponseMessage responseMessage;
	responseMessage.setApplicationName(flowDeallocateEvent.getApplicationName());
	responseMessage.setResult(result);
	responseMessage.setErrorDescription(errorDescription);
	responseMessage.setSourceIpcProcessId(flowDeallocateEvent.getIPCProcessId());
	responseMessage.setSequenceNumber(flowDeallocateEvent.getSequenceNumber());
	responseMessage.setResponseMessage(true);
	try{
		rinaManager->sendResponseOrNotficationMessage(&responseMessage);
	}catch(NetlinkException &e){
		throw IPCException(e.what());
	}
#endif
}

Singleton<IPCProcessApplicationManager> ipcProcessApplicationManager;

}
