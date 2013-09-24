//
// Core librina implementation
//
//    Eduard Grasa          <eduard.grasa@i2cat.net>
//    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#include <unistd.h>

#define RINA_PREFIX "core"

#include "logs.h"
#include "core.h"

namespace rina {

/* CLASS RINA NETLINK ENDPOINT */
RINANetlinkEndpoint::RINANetlinkEndpoint(){
	netlinkPortId = 0;
	ipcProcessId = 0;
}

RINANetlinkEndpoint::RINANetlinkEndpoint(
		unsigned int netlinkPortId, unsigned short ipcProcessId){
	this->netlinkPortId = netlinkPortId;
	this->ipcProcessId = ipcProcessId;
}

RINANetlinkEndpoint::RINANetlinkEndpoint(
		unsigned int netlinkPortId,
				unsigned short ipcProcessId,
				const ApplicationProcessNamingInformation& appNamingInfo){
	this->netlinkPortId = netlinkPortId;
	this->ipcProcessId = ipcProcessId;
	this->applicationProcessName = appNamingInfo;
}

unsigned short RINANetlinkEndpoint::getIpcProcessId() const {
	return ipcProcessId;
}

void RINANetlinkEndpoint::setIpcProcessId(unsigned short ipcProcessId) {
	this->ipcProcessId = ipcProcessId;
}

unsigned int RINANetlinkEndpoint::getNetlinkPortId() const {
	return netlinkPortId;
}

void RINANetlinkEndpoint::setNetlinkPortId(unsigned int netlinkPortId) {
	this->netlinkPortId = netlinkPortId;
}

const ApplicationProcessNamingInformation&
RINANetlinkEndpoint::getApplicationProcessName() const {
	return applicationProcessName;
}

void RINANetlinkEndpoint::setApplicationProcessName(
		const ApplicationProcessNamingInformation& applicationProcessName) {
	this->applicationProcessName = applicationProcessName;
}

/* CLASS NETLINK PORT ID MAP */
void NetlinkPortIdMap::putIPCProcessIdToNelinkPortIdMapping(
		unsigned int netlinkPortId, unsigned short ipcProcessId)
{
	RINANetlinkEndpoint * current = ipcProcessIdMappings[ipcProcessId];
	if (current != 0) {
		current->setIpcProcessId(ipcProcessId);
		current->setNetlinkPortId(netlinkPortId);
	} else {
		ipcProcessIdMappings[ipcProcessId] =
				new RINANetlinkEndpoint(netlinkPortId, ipcProcessId);
	}
}

RINANetlinkEndpoint * NetlinkPortIdMap::getNetlinkPortIdFromIPCProcessId(
		unsigned short ipcProcessId) throw(NetlinkException) {
	std::map<unsigned short, RINANetlinkEndpoint *>::iterator it =
			ipcProcessIdMappings.find(ipcProcessId);
	if (it == ipcProcessIdMappings.end()){
		LOG_ERR("Could not find the netlink endpoint of IPC Process %d",
				ipcProcessId);
		throw NetlinkException(
				NetlinkException::error_fetching_netlink_port_id);
	}

	return it->second;
}

void NetlinkPortIdMap::putAPNametoNetlinkPortIdMapping(
		ApplicationProcessNamingInformation apName,
		unsigned int netlinkPortId, unsigned short ipcProcessId){
	RINANetlinkEndpoint * current =
			applicationNameMappings[apName.getProcessNamePlusInstance()];
	if(current != 0){
		current->setIpcProcessId(ipcProcessId);
		current->setNetlinkPortId(netlinkPortId);
	}else{
		applicationNameMappings[apName.getProcessNamePlusInstance()]  =
			new RINANetlinkEndpoint(netlinkPortId, ipcProcessId, apName);
	}
}

RINANetlinkEndpoint * NetlinkPortIdMap::getNetlinkPortIdFromAPName(
		ApplicationProcessNamingInformation apName) throw(NetlinkException) {
	std::map<std::string, RINANetlinkEndpoint *>::iterator it =
			applicationNameMappings.find(apName.getProcessNamePlusInstance());
	if (it == applicationNameMappings.end()){
		LOG_ERR("Could not find the netlink endpoint of Application %s",
				apName.toString().c_str());
		throw NetlinkException(
				NetlinkException::error_fetching_netlink_port_id);
	};

	return it->second;
}

unsigned int NetlinkPortIdMap::getIPCManagerPortId(){
	return 1;
}

void NetlinkPortIdMap::updateMessageOrPortIdMap(
		BaseNetlinkMessage* message, bool send) throw(NetlinkException){
	switch (message->getOperationCode()) {
	case RINA_C_APP_ALLOCATE_FLOW_REQUEST: {
		if(send){
			message->setDestPortId(getIPCManagerPortId());
		}else{
			AppAllocateFlowRequestMessage * specificMessage =
					dynamic_cast<AppAllocateFlowRequestMessage *>(message);
			putAPNametoNetlinkPortIdMapping(
					specificMessage->getSourceAppName(),
					specificMessage->getSourcePortId(),
					specificMessage->getSourceIpcProcessId());
		}
		break;
	}
	case RINA_C_APP_ALLOCATE_FLOW_REQUEST_RESULT: {
		AppAllocateFlowRequestResultMessage * specificMessage =
				dynamic_cast<AppAllocateFlowRequestResultMessage *>(message);
		if(send){
			RINANetlinkEndpoint * endpoint = getNetlinkPortIdFromAPName(
					specificMessage->getSourceAppName());
			specificMessage->setDestPortId(endpoint->getNetlinkPortId());
			specificMessage->setDestIpcProcessId(endpoint->getIpcProcessId());
		}
		break;
	}
	case RINA_C_APP_ALLOCATE_FLOW_RESPONSE: {
		if(send){
			message->setDestPortId(getIPCManagerPortId());
		}
		break;
	}
	case RINA_C_APP_ALLOCATE_FLOW_REQUEST_ARRIVED: {
		AppAllocateFlowRequestArrivedMessage * specificMessage =
				dynamic_cast<AppAllocateFlowRequestArrivedMessage *>(message);
		if(send){
			RINANetlinkEndpoint * endpoint = getNetlinkPortIdFromAPName(
					specificMessage->getDestAppName());
			specificMessage->setDestPortId(endpoint->getNetlinkPortId());
			specificMessage->setDestIpcProcessId(endpoint->getIpcProcessId());
		}
		break;
	}
	case RINA_C_APP_DEALLOCATE_FLOW_REQUEST: {
		if(send){
			message->setDestPortId(getIPCManagerPortId());
		}
		break;
	}
	case RINA_C_APP_DEALLOCATE_FLOW_RESPONSE: {
		AppDeallocateFlowResponseMessage * specificMessage =
				dynamic_cast<AppDeallocateFlowResponseMessage *>(message);
		if(send){
			RINANetlinkEndpoint * endpoint = getNetlinkPortIdFromAPName(
					specificMessage->getApplicationName());
			specificMessage->setDestPortId(endpoint->getNetlinkPortId());
			specificMessage->setDestIpcProcessId(endpoint->getIpcProcessId());
		}
		break;
	}
	case RINA_C_APP_FLOW_DEALLOCATED_NOTIFICATION: {
		AppFlowDeallocatedNotificationMessage * specificMessage =
				dynamic_cast<AppFlowDeallocatedNotificationMessage *>(message);
		if(send){
			RINANetlinkEndpoint * endpoint = getNetlinkPortIdFromAPName(
					specificMessage->getApplicationName());
			specificMessage->setDestPortId(endpoint->getNetlinkPortId());
			specificMessage->setDestIpcProcessId(endpoint->getIpcProcessId());
		}
		break;
	}
	case RINA_C_APP_REGISTER_APPLICATION_REQUEST: {
		AppRegisterApplicationRequestMessage * specificMessage =
				dynamic_cast<AppRegisterApplicationRequestMessage *>(message);
		if(send){
			specificMessage->setDestPortId(getIPCManagerPortId());
		}else{
			putAPNametoNetlinkPortIdMapping(
					specificMessage->getApplicationName(),
					specificMessage->getSourcePortId(),
					specificMessage->getSourceIpcProcessId());
		}
		break;
	}
	case RINA_C_APP_REGISTER_APPLICATION_RESPONSE:{
		AppRegisterApplicationResponseMessage * specificMessage =
				dynamic_cast<AppRegisterApplicationResponseMessage *>(message);
		if(send){
			RINANetlinkEndpoint * endpoint = getNetlinkPortIdFromAPName(
					specificMessage->getApplicationName());
			specificMessage->setDestPortId(endpoint->getNetlinkPortId());
			specificMessage->setDestIpcProcessId(endpoint->getIpcProcessId());
		}
		break;
	}
	case RINA_C_APP_UNREGISTER_APPLICATION_REQUEST:{
		if(send){
			message->setDestPortId(getIPCManagerPortId());
		}else{
			AppUnregisterApplicationRequestMessage * specificMessage =
					dynamic_cast<AppUnregisterApplicationRequestMessage *>(message);
			putAPNametoNetlinkPortIdMapping(
					specificMessage->getApplicationName(),
					specificMessage->getSourcePortId(),
					specificMessage->getSourceIpcProcessId());
		}
		break;
	}
	case RINA_C_APP_UNREGISTER_APPLICATION_RESPONSE:{
		if(send){
			AppUnregisterApplicationResponseMessage * specificMessage =
					dynamic_cast<AppUnregisterApplicationResponseMessage *>(message);
			RINANetlinkEndpoint * endpoint = getNetlinkPortIdFromAPName(
					specificMessage->getApplicationName());
			specificMessage->setDestPortId(endpoint->getNetlinkPortId());
			specificMessage->setDestIpcProcessId(endpoint->getIpcProcessId());
		}
		break;
	}
	case RINA_C_APP_GET_DIF_PROPERTIES_REQUEST:{
		if(send){
			message->setDestPortId(getIPCManagerPortId());
		}else{
			AppGetDIFPropertiesRequestMessage * specificMessage =
					dynamic_cast<AppGetDIFPropertiesRequestMessage *>(message);
			putAPNametoNetlinkPortIdMapping(
					specificMessage->getApplicationName(),
					specificMessage->getSourcePortId(),
					specificMessage->getSourceIpcProcessId());
		}
		break;
	}
	case RINA_C_APP_GET_DIF_PROPERTIES_RESPONSE:{
		if(send){
			AppGetDIFPropertiesResponseMessage * specificMessage =
					dynamic_cast<AppGetDIFPropertiesResponseMessage *>(message);
			RINANetlinkEndpoint * endpoint = getNetlinkPortIdFromAPName(
					specificMessage->getApplicationName());
			specificMessage->setDestPortId(endpoint->getNetlinkPortId());
			specificMessage->setDestIpcProcessId(endpoint->getIpcProcessId());
		}
		break;
	}
	case RINA_C_IPCM_REGISTER_APPLICATION_RESPONSE:{
		if(send){
			message->setDestPortId(getIPCManagerPortId());
		}
		break;
	}
	case RINA_C_IPCM_UNREGISTER_APPLICATION_RESPONSE:{
		if(send){
			message->setDestPortId(getIPCManagerPortId());
		}
		break;
	}
	case RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE:{
		if(send){
			message->setDestPortId(getIPCManagerPortId());
		}
		break;
	}
	case RINA_C_IPCM_QUERY_RIB_RESPONSE:{
		if(send){
			message->setDestPortId(getIPCManagerPortId());
		}
		break;
	}
	case RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_RESULT:{
		if(send){
			message->setDestPortId(getIPCManagerPortId());
		}
		break;
	}
	case RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_ARRIVED:{
		if(send){
			message->setDestPortId(getIPCManagerPortId());
		}
		break;
	}
	case RINA_C_IPCM_DEALLOCATE_FLOW_RESPONSE:{
		if(send){
			message->setDestPortId(getIPCManagerPortId());
		}
		break;
	}
	case RINA_C_IPCM_FLOW_DEALLOCATED_NOTIFICATION:{
		if(send){
			message->setDestPortId(getIPCManagerPortId());
		}
		break;
	}
	default:{
		//Do nothing
	}
	}
}

/**
 * An OS Process has finalized. Retrieve the information associated to
 * the NL port-id (application name, IPC Process id if it is IPC process),
 * and return it in the form of an OSProcessFinalized event
 * @param nl_portid
 * @return
 */
IPCEvent * NetlinkPortIdMap::osProcessFinalized(unsigned int nl_portid){
	//1 Try to get application process name, if not there return 0
	ApplicationProcessNamingInformation apNamingInfo;
	std::map<std::string, RINANetlinkEndpoint*>::iterator iterator;
	std::map<unsigned short, RINANetlinkEndpoint*>::iterator iterator2;
	bool foundAppName = false;
	unsigned short ipcProcessId = 0;

	for(iterator = applicationNameMappings.begin();
			iterator != applicationNameMappings.end();
			++iterator){
		if(iterator->second->getNetlinkPortId() == nl_portid){
			apNamingInfo = iterator->second->getApplicationProcessName();
			foundAppName = true;
			applicationNameMappings.erase(iterator);
			break;
		}
	}

	if (!foundAppName){
		return 0;
	}

	//2 Try to get IPC Process id
	for(iterator2 = ipcProcessIdMappings.begin();
			iterator2 != ipcProcessIdMappings.end();
			++iterator2){
		if (iterator2->second->getNetlinkPortId() == nl_portid){
			ipcProcessId = iterator2->first;
			ipcProcessIdMappings.erase(iterator2);
			break;
		}
	}

	OSProcessFinalizedEvent * event = new OSProcessFinalizedEvent(
			apNamingInfo, ipcProcessId, 0);
	return event;
}

/* CLASS PENDING NETLINK MESSAGE */
PendingNetlinkMessage::PendingNetlinkMessage(unsigned int sequenceNumber) :
				ConditionVariable() {
	this->responseMessage = NULL;
	this->sequenceNumber = sequenceNumber;
}

PendingNetlinkMessage::~PendingNetlinkMessage() throw(){
}

BaseNetlinkMessage * PendingNetlinkMessage::getResponseMessage() {
	lock();

	if (responseMessage == 0){
		try{
			timedwait(WAIT_RESPONSE_TIMEOUT, 0);
		}catch(ConcurrentException &e){
		}
	}

	unlock();
	if (responseMessage != 0){
		LOG_DBG("Got Netlink reply to request with sequence number %d",
				sequenceNumber);
	}else{
		LOG_ERR("Timed out while waiting for response message");
	}

	return responseMessage;
}

void PendingNetlinkMessage::setResponseMessage(
		BaseNetlinkMessage * responseMessage) {
	lock();
	this->responseMessage = responseMessage;
	signal();
	unlock();
}

unsigned int PendingNetlinkMessage::getSequenceNumber() const{
	return sequenceNumber;
}

/* Class Netlink Session */
NetlinkSession::NetlinkSession(int sessionId){
	this->sessionId = sessionId;
}

NetlinkSession::~NetlinkSession(){
}

void NetlinkSession::putLocalPendingMessage(
		PendingNetlinkMessage* pendingMessage){
	localPendingMessages[pendingMessage->getSequenceNumber()]
	                     = pendingMessage;
}

PendingNetlinkMessage*
NetlinkSession::takeLocalPendingMessage(unsigned int sequenceNumber){
	std::map<unsigned int, PendingNetlinkMessage *>::iterator it =
			localPendingMessages.find(sequenceNumber);

	if(it == localPendingMessages.end()){
		LOG_ERR("Could not find the matching request for a local response message with sequence number %d",
				sequenceNumber);
		return NULL;
	}

	//2 Remove pending Netlink message from table
	localPendingMessages.erase(sequenceNumber);
	return it->second;
}

void NetlinkSession::putRemotePendingMessage(BaseNetlinkMessage* pendingMessage){
	remotePendingMessages[pendingMessage->getSequenceNumber()]
	                      = pendingMessage;
}

BaseNetlinkMessage*
NetlinkSession::takeRemotePendingMessage(unsigned int sequenceNumber){
	std::map<unsigned int, BaseNetlinkMessage *>::iterator it =
			remotePendingMessages.find(sequenceNumber);
	if(it == remotePendingMessages.end()){
		LOG_ERR("Could not find the matching request for a remote response message with sequence number %d",
				sequenceNumber);
		return NULL;
	}

	//2 Remove pending Netlink message from table
	remotePendingMessages.erase(sequenceNumber);

	return it->second;
}

/* CLASS RINA Manager */
/** main function of the Netlink message reader thread */
void * doNetlinkMessageReaderWork(void * arg) {
	RINAManager * myRINAManager = (RINAManager *) arg;
	NetlinkManager * netlinkManager = myRINAManager->getNetlinkManager();
	BlockingFIFOQueue<IPCEvent> * eventsQueue =
			myRINAManager->getEventQueue();
	BaseNetlinkMessage * incomingMessage;

	//Continuously try to read incoming Netlink messages
	while (true) {
		//Receive message
		try {
			LOG_DBG("Waiting for message %d", netlinkManager->getLocalPort());
			incomingMessage = netlinkManager->getMessage();
		} catch (NetlinkException &e) {
			LOG_ERR("Error receiving netlink message. %s", e.what());
			continue;
		}

		LOG_DBG("Received Netlink message. %s ",
				incomingMessage->toString().c_str());

		//Process the message
		if (incomingMessage->getOperationCode() ==
				RINA_C_IPCM_SOCKET_CLOSED_NOTIFICATION){
			IpcmNLSocketClosedNotificationMessage * message =
					dynamic_cast<IpcmNLSocketClosedNotificationMessage *>
			(incomingMessage);
			LOG_DBG("NL socket at port %d is closed", message->getPortId());

			IPCEvent * event = myRINAManager->osProcessFinalized(message->getPortId());
			if (event)
				eventsQueue->put(event);

			delete message;
		}else if (incomingMessage->isResponseMessage()){
			myRINAManager->netlinkResponseMessageArrived(incomingMessage);
		}else{
			NetlinkRequestOrNotificationMessage * message =
					dynamic_cast<NetlinkRequestOrNotificationMessage *>
			(incomingMessage);

			bool notification = incomingMessage->isNotificationMessage();
			bool request = incomingMessage->isRequestMessage();
			IPCEvent * ipcEvent = message->toIPCEvent();

			if (request){
				myRINAManager->netlinkRequestMessageArrived(incomingMessage);
			}else{
				myRINAManager->netlinkNotificationMessageArrived(
						incomingMessage);
			}

			eventsQueue->put(ipcEvent);

			if (notification){
				delete message;
				message = 0;
			}
		}
	}

	return (void *) 0;
}

RINAManager::RINAManager() {
	//1 Initialize NetlinkManager
	try {
		netlinkManager = new NetlinkManager(getNelinkPortId());
	} catch (NetlinkException &e) {
		LOG_ERR("Error initializing Netlink Manager. %s", e.what());
		LOG_ERR("Program will exit now");
		exit(-1); 	//FIXME Is this too drastic?
	}
	LOG_DBG("Initialized Netlink Manager");

	initialize();
}

RINAManager::RINAManager(unsigned int netlinkPort){
	//1 Initialize NetlinkManager
	try {
		netlinkManager = new NetlinkManager(netlinkPort);
	} catch (NetlinkException &e) {
		LOG_ERR("Error initializing Netlink Manager. %s", e.what());
		LOG_ERR("Program will exit now");
		exit(-1); 	//FIXME Is this too drastic?
	}
	LOG_DBG("Initialized Netlink Manager");

	initialize();
}

void RINAManager::initialize(){
	//2 Initialie events Queue
	eventQueue = new BlockingFIFOQueue<IPCEvent>();
	LOG_DBG("Initialized event queue");

	//3 Start Netlink message reader thread
	ThreadAttributes * threadAttributes = new ThreadAttributes();
	threadAttributes->setJoinable();
	netlinkMessageReader = new Thread(threadAttributes,
			&doNetlinkMessageReaderWork, (void *) this);
	LOG_DBG("Started Netlink Message reader thread");
}

RINAManager::~RINAManager() {
	delete netlinkManager;
	delete netlinkMessageReader;
	delete eventQueue;
}

NetlinkSession* RINAManager::getAndCreateNetlinkSession(
		unsigned int sessionId){
	NetlinkSession* response = NULL;

	std::map<unsigned int, NetlinkSession*>::iterator it =
			netlinkSessions.find(sessionId);
	if(it == netlinkSessions.end()){
		LOG_DBG("Creating a new Netlink session with id %d",
				sessionId);
		response = new NetlinkSession(sessionId);
		netlinkSessions[sessionId] = response;
	}else{
		response = it->second;
	}

	return response;
}

NetlinkSession* RINAManager::getNetlinkSession(unsigned int sessionId){
	NetlinkSession* response = NULL;

	std::map<unsigned int, NetlinkSession*>::iterator it =
			netlinkSessions.find(sessionId);
	if(it != netlinkSessions.end()){
		response = it->second;
	}

	return response;
}

BaseNetlinkMessage * RINAManager::sendRequestAndWaitForResponse(
		BaseNetlinkMessage * request, const std::string& errorDescription)
throw(NetlinkException){
	BaseNetlinkMessage* response = 0;
	response = sendRequestMessageAndWaitForResponse(request);
	return response;
}

BaseNetlinkMessage * RINAManager::sendRequestMessageAndWaitForResponse(
		BaseNetlinkMessage * netlinkMessage) throw (NetlinkException) {
	sendReceiveLock.lock();

	//1 Populate destination port id
	try {
		netlinkPortIdMap.updateMessageOrPortIdMap(netlinkMessage, true);
	} catch (NetlinkException &e) {
		sendReceiveLock.unlock();
		throw e;
	}

	NetlinkSession * netlinkSession =
			getAndCreateNetlinkSession(netlinkMessage->getDestPortId());
	netlinkMessage->setSequenceNumber(netlinkManager->getSequenceNumber());

	//2 Send the message
	try {
		netlinkManager->sendMessage(netlinkMessage);
	} catch (NetlinkException &e) {
		sendReceiveLock.unlock();
		throw e;
	}

	//3 Put message in the queue
	PendingNetlinkMessage pendingMessage(netlinkMessage->getSequenceNumber());
	netlinkSession->putLocalPendingMessage(&pendingMessage);

	sendReceiveLock.unlock();

	//4 wait for reply
	BaseNetlinkMessage * response = pendingMessage.getResponseMessage();

	if (response == 0){
		throw NetlinkException(NetlinkException::error_waiting_for_response);
	}

	return response;
}

void RINAManager::sendResponseOrNotficationMessage(
		BaseNetlinkMessage * netlinkMessage) throw (NetlinkException) {
	sendReceiveLock.lock();

	//1 Populate destination port id
	try {
		netlinkPortIdMap.updateMessageOrPortIdMap(netlinkMessage, true);
	} catch (NetlinkException &e) {
		sendReceiveLock.unlock();
		throw e;
	}

	if (netlinkMessage->isResponseMessage()) {
		NetlinkSession * netlinkSession = getNetlinkSession(
				netlinkMessage->getDestPortId());
		if (netlinkSession == NULL) {
			LOG_ERR("Could not find an existing Netlink session with id %d",
					netlinkMessage->getDestPortId());
			sendReceiveLock.unlock();
			throw NetlinkException(
					NetlinkException::error_fetching_netlink_session);
		}

		BaseNetlinkMessage * requestMessage = netlinkSession
				->takeRemotePendingMessage(
						netlinkMessage->getSequenceNumber());
		if (requestMessage == NULL) {
			sendReceiveLock.unlock();
			throw NetlinkException(
					NetlinkException::
					error_fetching_pending_netlink_request_message);
		}

		delete requestMessage;
		requestMessage = 0;
	}

	//2 Send the message
	try {
		netlinkManager->sendMessage(netlinkMessage);
	} catch (NetlinkException &e) {
		sendReceiveLock.unlock();
		throw e;
	}

	sendReceiveLock.unlock();
}

void RINAManager::netlinkResponseMessageArrived(
		BaseNetlinkMessage * response){
	sendReceiveLock.lock();

	//Try to update netlink port id map
	try {
		netlinkPortIdMap.updateMessageOrPortIdMap(response, false);
	} catch (NetlinkException &e) {
		LOG_WARN("Exception while trying to update netlink portId map. %s",
				e.what());
	}

	NetlinkSession *netlinkSession = getNetlinkSession(
			response->getSourcePortId());
	if  (netlinkSession == 0){
		LOG_ERR("Could not find an existing Netlink session with id %d",
				response->getSourcePortId());
		sendReceiveLock.unlock();
		return;
	}

	PendingNetlinkMessage* pendingMessage = netlinkSession->
			takeLocalPendingMessage(response->getSequenceNumber());
	sendReceiveLock.unlock();
	if(pendingMessage == 0){
		sendReceiveLock.unlock();
		return;
	}

	//3 Notify thread that sent the Netlink request message
	pendingMessage->setResponseMessage(response);
}

void RINAManager::netlinkRequestMessageArrived(
		BaseNetlinkMessage * request){
	sendReceiveLock.lock();

	//Try to update netlink port id map
	try {
		netlinkPortIdMap.updateMessageOrPortIdMap(request, false);
	} catch (NetlinkException &e) {
		LOG_WARN("Exception while trying to update netlink portId map. %s",
				e.what());
	}

	NetlinkSession *netlinkSession = getAndCreateNetlinkSession(
			request->getSourcePortId());

	netlinkSession->putRemotePendingMessage(request);
	sendReceiveLock.unlock();
}

void RINAManager::netlinkNotificationMessageArrived(
		BaseNetlinkMessage * notification){
	sendReceiveLock.lock();

	//Try to update netlink port id map
	try {
		netlinkPortIdMap.updateMessageOrPortIdMap(notification, false);
	} catch (NetlinkException &e) {
		LOG_WARN("Exception while trying to update netlink portId map. %s",
				e.what());
	}

	sendReceiveLock.unlock();
}

IPCEvent * RINAManager::osProcessFinalized(unsigned int nl_portid){
	IPCEvent * event;
	sendReceiveLock.lock();
	event = netlinkPortIdMap.osProcessFinalized(nl_portid);
	sendReceiveLock.unlock();
	return event;
}

BlockingFIFOQueue<IPCEvent>* RINAManager::getEventQueue(){
	return eventQueue;
}

NetlinkManager* RINAManager::getNetlinkManager(){
	return netlinkManager;
}

Singleton<RINAManager> rinaManager;


/* Get and set default Netlink port id */
unsigned int netlinkPortId = getpid();
Lockable * netlinkLock = new Lockable();

void setNetlinkPortId(unsigned int newNetlinkPortId){
	netlinkLock->lock();
	netlinkPortId = newNetlinkPortId;
	netlinkLock->unlock();
}

unsigned int getNelinkPortId(){
	unsigned int result = 0;
	netlinkLock->lock();
	result = netlinkPortId;
	netlinkLock->unlock();
	return result;
}

}

