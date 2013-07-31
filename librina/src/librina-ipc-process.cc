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
const std::string ExtendedIPCManager::error_allocate_flow =
		"Error allocating flow";

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
		const AssignToDIFRequestEvent& event, int result)
	throw(AssignToDIFResponseException){
	if (result == 0){
		this->currentConfiguration = event.getDIFConfiguration();
	}
#if STUB_API
	//Do nothing
#else
	IpcmAssignToDIFResponseMessage responseMessage;
	responseMessage.setResult(result);
	responseMessage.setSequenceNumber(event.getSequenceNumber());
	responseMessage.setResponseMessage(true);
	try{
		rinaManager->sendResponseOrNotficationMessage(&responseMessage);
	}catch(NetlinkException &e){
		throw AssignToDIFResponseException(e.what());
	}
#endif
}

void ExtendedIPCManager::registerApplicationResponse(
		const ApplicationRegistrationRequestEvent& event, int result)
	throw(RegisterApplicationResponseException){
#if STUB_API
	//Do nothing
#else
	IpcmRegisterApplicationResponseMessage responseMessage;
	responseMessage.setResult(result);
	responseMessage.setSequenceNumber(event.getSequenceNumber());
	responseMessage.setResponseMessage(true);
	try{
		rinaManager->sendResponseOrNotficationMessage(&responseMessage);
	}catch(NetlinkException &e){
		throw RegisterApplicationResponseException(e.what());
	}
#endif
}

void ExtendedIPCManager::unregisterApplicationResponse(
		const ApplicationUnregistrationRequestEvent& event, int result)
	throw(UnregisterApplicationResponseException){
#if STUB_API
	//Do nothing
#else
	IpcmUnregisterApplicationResponseMessage responseMessage;
	responseMessage.setResult(result);
	responseMessage.setSequenceNumber(event.getSequenceNumber());
	responseMessage.setResponseMessage(true);
	try{
		rinaManager->sendResponseOrNotficationMessage(&responseMessage);
	}catch(NetlinkException &e){
		throw UnregisterApplicationResponseException(e.what());
	}
#endif
}

void ExtendedIPCManager::allocateFlowRequestResult(
		const FlowRequestEvent& event, int result)
	throw(AllocateFlowResponseException){
#if STUB_API
	//Do nothing
#else
	IpcmAllocateFlowRequestResultMessage responseMessage;
	responseMessage.setResult(result);
	responseMessage.setSequenceNumber(event.getSequenceNumber());
	responseMessage.setResponseMessage(true);
	try{
		rinaManager->sendResponseOrNotficationMessage(&responseMessage);
	}catch(NetlinkException &e){
		throw AllocateFlowResponseException(e.what());
	}
#endif
}

int ExtendedIPCManager::allocateFlowRequestArrived(
			const ApplicationProcessNamingInformation& localAppName,
			const ApplicationProcessNamingInformation& remoteAppName,
			const FlowSpecification& flowSpecification)
		throw (AllocateFlowRequestArrivedException){
#if STUP_API
	return 25;
#else
	IpcmAllocateFlowRequestArrivedMessage message;
	message.setSourceAppName(remoteAppName);
	message.setDestAppName(localAppName);
	message.setFlowSpecification(flowSpecification);
	message.setDifName(currentConfiguration.getDifName());
	message.setSourceIpcProcessId(ipcProcessId);
	message.setRequestMessage(true);

	int portId = 0;
	IpcmAllocateFlowResponseMessage * allocateFlowResponse;
	try{
		allocateFlowResponse =
				dynamic_cast<IpcmAllocateFlowResponseMessage *>(
						rinaManager->sendRequestAndWaitForResponse(&message,
								ExtendedIPCManager::error_allocate_flow));
	}catch(NetlinkException &e){
		throw AllocateFlowRequestArrivedException(e.what());
	}

	if (allocateFlowResponse->getResult()<0){
		delete allocateFlowResponse;
		throw AllocateFlowRequestArrivedException(
				ExtendedIPCManager::error_allocate_flow );
	}

	portId = allocateFlowResponse->getPortId();
	LOG_DBG("Allocated flow with portId %d", portId);
	delete allocateFlowResponse;

	return portId;
#endif
}

void ExtendedIPCManager::flowDeallocated(
		const FlowDeallocateRequestEvent flowDeallocateEvent,
		int result)
	throw (DeallocateFlowResponseException){
#if STUB_API
	//Do nothing
#else
	IpcmDeallocateFlowResponseMessage responseMessage;
	responseMessage.setResult(result);
	responseMessage.setSourceIpcProcessId(ipcProcessId);
	responseMessage.setSequenceNumber(flowDeallocateEvent.getSequenceNumber());
	responseMessage.setResponseMessage(true);
	try{
		rinaManager->sendResponseOrNotficationMessage(&responseMessage);
	}catch(NetlinkException &e){
		throw DeallocateFlowResponseException(e.what());
	}
#endif
}

void ExtendedIPCManager::flowDeallocatedRemotely(
		int portId, int code)
		throw (DeallocateFlowResponseException){
#if STUB_API
	//Do nothing
#else
	IpcmFlowDeallocatedNotificationMessage message;
	message.setPortId(portId);
	message.setCode(code);
	message.setSourceIpcProcessId(ipcProcessId);
	message.setNotificationMessage(true);
	try{
		rinaManager->sendResponseOrNotficationMessage(&message);
	}catch(NetlinkException &e){
		throw DeallocateFlowResponseException(e.what());
	}
#endif
}

void ExtendedIPCManager::queryRIBResponse(
		const QueryRIBRequestEvent& event, int result,
		const std::list<RIBObject>& ribObjects)
	throw(QueryRIBResponseException){
#if STUB_API
	//Do nothing
#else
	IpcmDIFQueryRIBResponseMessage responseMessage;
	responseMessage.setResult(result);
	responseMessage.setRIBObjects(ribObjects);
	responseMessage.setSequenceNumber(event.getSequenceNumber());
	responseMessage.setResponseMessage(true);
	try{
		rinaManager->sendResponseOrNotficationMessage(&responseMessage);
	}catch(NetlinkException &e){
		throw QueryRIBResponseException(e.what());
	}
#endif
}

Singleton<ExtendedIPCManager> extendedIPCManager;

}
