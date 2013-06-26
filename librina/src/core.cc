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

#define RINA_PREFIX "core"

#include "logs.h"
#include "core.h"

namespace rina {

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
	while (responseMessage == NULL) {
		wait();
	}
	unlock();
	LOG_DBG("Got Netlink reply to request with sequence number %d",
			sequenceNumber);

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
			incomingMessage = netlinkManager->getMessage();
		} catch (NetlinkException &e) {
			LOG_ERR("Error receiving netlink message. %s", e.what());
			continue;
		}

		LOG_DBG("Received Netlink message. %s ",
				incomingMessage->toString().c_str());

		//Process the message
		if (incomingMessage->isResponseMessage()){
			myRINAManager->netlinkResponseMessageArrived(incomingMessage);
		}else{
			NetlinkRequestOrNotificationMessage * message =
					dynamic_cast<NetlinkRequestOrNotificationMessage *>
						(incomingMessage);

			if (incomingMessage->isRequestMessage()){
				myRINAManager->netlinkRequestMessageArrived(incomingMessage);
			}

			eventsQueue->put(message->toIPCEvent());

			if (incomingMessage->isNotificationMessage()){
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
		netlinkManager = new NetlinkManager();
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

BaseNetlinkMessage * RINAManager::sendRequestMessageAndWaitForReply(
		BaseNetlinkMessage * netlinkMessage) throw (NetlinkException) {
	PendingNetlinkMessage * pendingMessage = NULL;

	sendReceiveLock.lock();

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
	pendingMessage = new PendingNetlinkMessage(netlinkMessage->getSequenceNumber());
	netlinkSession->putLocalPendingMessage(pendingMessage);

	sendReceiveLock.unlock();

	//4 wait for reply
	BaseNetlinkMessage * response = pendingMessage->getResponseMessage();
	delete pendingMessage;
	pendingMessage = 0;
	return response;
}

void RINAManager::sendResponseOrNotficationMessage(
		BaseNetlinkMessage * netlinkMessage) throw (NetlinkException) {
	sendReceiveLock.lock();

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

	NetlinkSession *netlinkSession = getNetlinkSession(
			response->getSourcePortId());
	if  (netlinkSession == NULL){
		LOG_ERR("Could not find an existing Netlink session with id %d",
				response->getSourcePortId());
		sendReceiveLock.unlock();
		return;
	}

	PendingNetlinkMessage* pendingMessage = netlinkSession->
			takeLocalPendingMessage(response->getSequenceNumber());
	sendReceiveLock.unlock();
	if(pendingMessage == NULL){
		sendReceiveLock.unlock();
		return;
	}

	//3 Notify thread that sent the Netlink request message
	pendingMessage->setResponseMessage(response);
}

void RINAManager::netlinkRequestMessageArrived(
		BaseNetlinkMessage * request){
	sendReceiveLock.lock();

	NetlinkSession *netlinkSession = getAndCreateNetlinkSession(
			request->getSourcePortId());

	netlinkSession->putRemotePendingMessage(request);
	sendReceiveLock.unlock();
}

BlockingFIFOQueue<IPCEvent>* RINAManager::getEventQueue(){
	return eventQueue;
}

NetlinkManager* RINAManager::getNetlinkManager(){
	return netlinkManager;
}

Singleton<RINAManager> rinaManager;

}

