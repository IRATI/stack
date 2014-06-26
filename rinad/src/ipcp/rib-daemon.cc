//
// RIB Daemon
//
//    Bernat Gaston <bernat.gaston@i2cat.net>
//    Eduard Grasa <eduard.grasa@i2cat.net>
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

#define RINA_PREFIX "rib-daemon"

#include <librina/logs.h>
#include "rib-daemon.h"

namespace rinad {

//Class RIB
RIB::RIB() {
}

RIB::~RIB() throw() {
}

BaseRIBObject* RIB::getRIBObject(const std::string& objectClass, const std::string& objectName) throw (Exception) {
	BaseRIBObject* ribObject;
	std::map<std::string, BaseRIBObject*>::iterator it;

	lock();
	it = rib_.find(objectName);
	unlock();

	if (it == rib_.end()) {
		throw Exception("Could not find object in the RIB");
	}

	ribObject = it->second;
	if (ribObject->get_class().compare(objectClass) != 0) {
		throw Exception("Object class does not match the user specified one");
	}

	return ribObject;
}

void RIB::addRIBObject(BaseRIBObject* ribObject) throw (Exception) {
	lock();
	if (rib_.find(ribObject->get_name()) != rib_.end()) {
		throw Exception("Object already exists in the RIB");
	}
	rib_[ribObject->get_name()] = ribObject;
	unlock();
}

BaseRIBObject * RIB::removeRIBObject(const std::string& objectName) throw (Exception) {
	std::map<std::string, BaseRIBObject*>::iterator it;
	BaseRIBObject* ribObject;

	lock();
	it = rib_.find(objectName);
	if (it == rib_.end()) {
		throw Exception("Could not find object in the RIB");
	}

	ribObject = it->second;
	rib_.erase(it);
	unlock();

	return ribObject;
}

std::list<BaseRIBObject*> RIB::getRIBObjects() {
	std::list<BaseRIBObject*> result;

	for(std::map<std::string, BaseRIBObject*>::iterator it = rib_.begin();
			it != rib_.end(); ++it) {
		result.push_back(it->second);
	}

	return result;
}

//Class ManagementSDUReader data
ManagementSDUReaderData::ManagementSDUReaderData(IRIBDaemon * rib_daemon,
		unsigned int max_sdu_size) {
	rib_daemon_ = rib_daemon;
	max_sdu_size_ = max_sdu_size;
}

IRIBDaemon * ManagementSDUReaderData::get_rib_daemon() {
	return rib_daemon_;
}

unsigned int ManagementSDUReaderData::get_max_sdu_size() {
	return max_sdu_size_;
}

void * doManagementSDUReaderWork(void* arg) {
	ManagementSDUReaderData * data = (ManagementSDUReaderData *) arg;
	char* buffer = new char[data->get_max_sdu_size()];
	char* sdu;

	rina::ReadManagementSDUResult result;
	LOG_INFO("Starting Management SDU reader ...");
	while (true) {
		try {
		result = rina::kernelIPCProcess->readManagementSDU(buffer, data->get_max_sdu_size());
		} catch (Exception &e) {
			LOG_ERR("Problems reading management SDU: %s", e.what());
			continue;
		}

		sdu = new char[result.getBytesRead()];
		for(int i=0; i<result.getBytesRead(); i++) {
			sdu[i] = buffer[i];
		}

		data->get_rib_daemon()->cdapMessageDelivered(sdu, result.getBytesRead(), result.getPortId());

		delete sdu;
	}

	delete buffer;

	return 0;
}

/// Class BaseRIBDaemon
BaseRIBDaemon::BaseRIBDaemon(){
}

void BaseRIBDaemon::subscribeToEvent(const IPCProcessEventType& eventId, EventListener * eventListener) {
	if (!eventListener) {
		return;
	}

	events_lock_.lock();

	std::map<IPCProcessEventType, std::list<EventListener*> >::iterator it = event_listeners_.find(eventId);
	if (it == event_listeners_.end()) {
		std::list<EventListener *> listenersList;
		listenersList.push_back(eventListener);
		event_listeners_[eventId] = listenersList;
	} else {
		std::list<EventListener *>::iterator listIterator;
		for(listIterator=it->second.begin(); listIterator != it->second.end(); ++listIterator) {
			if (*listIterator == eventListener) {
				events_lock_.unlock();
				return;
			}
		}

		it->second.push_back(eventListener);
	}

	LOG_INFO("EventListener subscribed to event %d", eventId);
	events_lock_.unlock();
}

void BaseRIBDaemon::unsubscribeFromEvent(const IPCProcessEventType& eventId, EventListener * eventListener) {
	if (!eventListener) {
		return;
	}

	events_lock_.lock();
	std::map<IPCProcessEventType, std::list<EventListener*> >::iterator it = event_listeners_.find(eventId);
	if (it == event_listeners_.end()) {
		events_lock_.unlock();
		return;
	}

	it->second.remove(eventListener);
	if (it->second.size() == 0) {
		event_listeners_.erase(it);
	}

	LOG_INFO("EventListener unsubscribed from event %d", eventId);
	events_lock_.unlock();
}

void BaseRIBDaemon::deliverEvent(Event * event) {
	if (!event) {
		return;
	}

	LOG_INFO("Event %d has just happened. Notifying event listeners.", event->get_id());

	events_lock_.lock();
	std::map<IPCProcessEventType, std::list<EventListener*> >::iterator it = event_listeners_.find(event->get_id());
	if (it == event_listeners_.end()) {
		events_lock_.unlock();
		delete event;
		return;
	}

	std::list<EventListener *>::iterator listIterator;
	for(listIterator=it->second.begin(); listIterator != it->second.end(); ++listIterator) {
		(*listIterator)->eventHappened(event);
	}

	events_lock_.unlock();

	if (event) {
		delete event;
	}
}

///Class RIBDaemon
RIBDaemon::RIBDaemon() {
	ipc_process_ = 0;
	management_sdu_reader_ = 0;
	cdap_session_manager_ = 0;
	encoder_ = 0;
	n_minus_one_flow_manager_ = 0;
}

void RIBDaemon::set_ipc_process(IPCProcess * ipc_process){
	ipc_process_ = ipc_process;
	cdap_session_manager_ = ipc_process->get_cdap_session_manager();
	encoder_ = ipc_process->get_encoder();
	n_minus_one_flow_manager_ = ipc_process->get_resource_allocator()->get_n_minus_one_flow_manager();

	subscribeToEvents();

	rina::ThreadAttributes * threadAttributes = new rina::ThreadAttributes();
	threadAttributes->setJoinable();
	ManagementSDUReaderData * data = new ManagementSDUReaderData(this, max_sdu_size_in_bytes);
	management_sdu_reader_ = new rina::Thread(threadAttributes,
			&doManagementSDUReaderWork, (void *) data);
}

void RIBDaemon::subscribeToEvents() {
	subscribeToEvent(IPCP_EVENT_N_MINUS_1_FLOW_ALLOCATED, this);
	subscribeToEvent(IPCP_EVENT_N_MINUS_1_FLOW_DEALLOCATED, this);
}

void RIBDaemon::eventHappened(Event * event) {
	if (!event) {
		return;
	}

	if (event->get_id() == IPCP_EVENT_N_MINUS_1_FLOW_DEALLOCATED) {
		NMinusOneFlowDeallocatedEvent * flowEvent = (NMinusOneFlowDeallocatedEvent *) event;
		nMinusOneFlowDeallocated(flowEvent->get_port_id());
	} else if (event->get_id() == IPCP_EVENT_N_MINUS_1_FLOW_ALLOCATED) {
		NMinusOneFlowAllocatedEvent * flowEvent = (NMinusOneFlowAllocatedEvent *) event;
		nMinusOneFlowAllocated(flowEvent);
	}
}

void RIBDaemon::nMinusOneFlowDeallocated(int portId) {
	cdap_session_manager_->removeCDAPSession(portId);
}

void RIBDaemon::nMinusOneFlowAllocated(NMinusOneFlowAllocatedEvent * event) {
	if (!event) {
		return;
	}
}

void RIBDaemon::addRIBObject(BaseRIBObject * ribObject) throw (Exception) {
	if (!ribObject) {
		throw Exception("Object is null");
	}
	rib_.addRIBObject(ribObject);
	LOG_INFO("Object with name %s, class %s, instance %ld added to the RIB",
			ribObject->get_name().c_str(), ribObject->get_class().c_str(),
			ribObject->get_instance());
}

void RIBDaemon::removeRIBObject(BaseRIBObject * ribObject) throw (Exception) {
	if (!ribObject) {
		throw Exception("Object is null");
	}

	removeRIBObject(ribObject->get_name());
}

void RIBDaemon::removeRIBObject(const std::string& objectName) throw (Exception){
	BaseRIBObject * object = rib_.removeRIBObject(objectName);
	LOG_INFO("Object with name %s, class %s, instance %ld removed from the RIB",
			object->get_name().c_str(), object->get_class().c_str(),
			object->get_instance());

	delete object;
}

std::list<BaseRIBObject *> RIBDaemon::getRIBObjects() {
	return rib_.getRIBObjects();
}

BaseRIBObject * RIBDaemon::readObject(const std::string& objectClass,
			const std::string& objectName) throw (Exception) {
	return rib_.getRIBObject(objectClass, objectName);
}

void RIBDaemon::writeObject(const std::string& objectClass, const std::string& objectName,
				const void* objectValue) throw (Exception) {
	BaseRIBObject * object = rib_.getRIBObject(objectClass, objectName);
	object->writeObject(objectValue);
}

void RIBDaemon::startObject(const std::string& objectClass, const std::string& objectName,
			const void* objectValue) throw (Exception) {
	BaseRIBObject * object = rib_.getRIBObject(objectClass, objectName);
	object->startObject(objectValue);
}

void RIBDaemon::stopObject(const std::string& objectClass, const std::string& objectName,
		const void* objectValue) throw (Exception) {
	BaseRIBObject * object = rib_.getRIBObject(objectClass, objectName);
	object->stopObject(objectValue);
}

void RIBDaemon::processQueryRIBRequestEvent(const rina::QueryRIBRequestEvent& event) {
	std::list<BaseRIBObject *> ribObjects = getRIBObjects();
	std::list<rina::RIBObjectData> result;

	std::list<BaseRIBObject*>::iterator it;
	for (it = ribObjects.begin(); it != ribObjects.end(); ++it) {
		result.push_back((*it)->get_data());
	}

	try {
		rina::extendedIPCManager->queryRIBResponse(event, 0, result);
	} catch (Exception &e) {
		LOG_ERR("Problems sending query RIB response to IPC Manager: %s",
				e.what());
	}
}

ICDAPResponseMessageHandler * RIBDaemon::getCDAPMessageHandler(const rina::CDAPMessage * cdapMessage) {
	ICDAPResponseMessageHandler * handler;
	std::map<int, ICDAPResponseMessageHandler *>::iterator it;

	if (!cdapMessage) {
		return 0;
	}

	handlers_lock_.lock();
	it = handlers_waiting_for_reply_.find(cdapMessage->get_invoke_id());
	if (it == handlers_waiting_for_reply_.end()) {
		handlers_lock_.unlock();
		return 0;
	}

	handler = it->second;

	if (cdapMessage->get_flags() != rina::CDAPMessage::F_RD_INCOMPLETE) {
		handlers_waiting_for_reply_.erase(it);
	}
	handlers_lock_.unlock();

	return handler;
}

void RIBDaemon::processIncomingRequestMessage(const rina::CDAPMessage * cdapMessage,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	BaseRIBObject * ribObject;

	LOG_DBG("Remote operation %d called on object %s", cdapMessage->get_op_code(),
			cdapMessage->get_obj_name().c_str());
	try {
		switch (cdapMessage->get_op_code()){
		case rina::CDAPMessage::M_CREATE:
			// Creation is delegated to the parent objects if the object doesn't exist.
			// Create semantics are CREATE or UPDATE. If the object exists it is an
			// update, therefore the message is handled to the object. If the object
			// doesn't exist it is a CREATE, therefore it is handled to the parent object
			try{
				ribObject = rib_.getRIBObject(cdapMessage->get_obj_class(),
											cdapMessage->get_obj_name());
				ribObject->remoteCreateObject(cdapMessage, cdapSessionDescriptor);
			}catch (Exception &e) {
				//Look for parent object, delegate creation there
				int position = cdapMessage->get_obj_name().rfind(RIBObjectNames::SEPARATOR);
				if (position == std::string::npos) {
					throw e;
				}
				std::string parentObjectName = cdapMessage->get_obj_name().substr(0, position);
				ribObject = rib_.getRIBObject(cdapMessage->get_obj_class(), parentObjectName);
				ribObject->remoteCreateObject(cdapMessage, cdapSessionDescriptor);
			}

			break;
		case rina::CDAPMessage::M_DELETE:
			ribObject = rib_.getRIBObject(cdapMessage->get_obj_class(),
							cdapMessage->get_obj_name());
			ribObject->remoteDeleteObject(cdapMessage, cdapSessionDescriptor);
			break;
		case rina::CDAPMessage::M_START:
			ribObject = rib_.getRIBObject(cdapMessage->get_obj_class(),
							cdapMessage->get_obj_name());
			ribObject->remoteStartObject(cdapMessage, cdapSessionDescriptor);
			break;
		case rina::CDAPMessage::M_STOP:
			ribObject = rib_.getRIBObject(cdapMessage->get_obj_class(),
							cdapMessage->get_obj_name());
			ribObject->remoteStopObject(cdapMessage, cdapSessionDescriptor);
			break;
		case rina::CDAPMessage::M_READ:
			ribObject = rib_.getRIBObject(cdapMessage->get_obj_class(),
							cdapMessage->get_obj_name());
			ribObject->remoteReadObject(cdapMessage, cdapSessionDescriptor);
			break;
		case rina::CDAPMessage::M_CANCELREAD:
			ribObject = rib_.getRIBObject(cdapMessage->get_obj_class(),
							cdapMessage->get_obj_name());
			ribObject->remoteCancelReadObject(cdapMessage, cdapSessionDescriptor);
			break;
		case rina::CDAPMessage::M_WRITE:
			ribObject = rib_.getRIBObject(cdapMessage->get_obj_class(),
							cdapMessage->get_obj_name());
			ribObject->remoteWriteObject(cdapMessage, cdapSessionDescriptor);
			break;
		default:
			LOG_ERR("Invalid operation code for a request message: %d", cdapMessage->get_op_code());
			delete cdapMessage;
		}
	} catch(Exception &e) {
		LOG_ERR("Problems processing incoming CDAP request message %s", e.what());
		delete cdapMessage;
	}

	return;
}

void RIBDaemon::processIncomingResponseMessage(const rina::CDAPMessage * cdapMessage,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	ICDAPResponseMessageHandler * handler;

	handler = getCDAPMessageHandler(cdapMessage);
	if (!handler) {
		LOG_ERR("Could not find a message handler for invoke-id %d",
				cdapMessage->get_invoke_id());
		delete cdapMessage;
		return;
	}

	switch (cdapMessage->get_op_code()) {
	case rina::CDAPMessage::M_CREATE_R:
		handler->createResponse(cdapMessage, cdapSessionDescriptor);
		break;
	case rina::CDAPMessage::M_DELETE_R:
		handler->deleteResponse(cdapMessage, cdapSessionDescriptor);
		break;
	case rina::CDAPMessage::M_START_R:
		handler->startResponse(cdapMessage, cdapSessionDescriptor);
		break;
	case rina::CDAPMessage::M_STOP_R:
		handler->stopResponse(cdapMessage, cdapSessionDescriptor);
		break;
	case rina::CDAPMessage::M_READ_R:
		handler->readResponse(cdapMessage, cdapSessionDescriptor);
		break;
	case rina::CDAPMessage::M_CANCELREAD_R:
		handler->cancelReadResponse(cdapMessage, cdapSessionDescriptor);
		break;
	case rina::CDAPMessage::M_WRITE_R:
		handler->writeResponse(cdapMessage, cdapSessionDescriptor);
		break;
	default:
		LOG_ERR("Invalid operation code for a response message %d", cdapMessage->get_op_code());
		delete cdapMessage;
	}
}

void RIBDaemon::cdapMessageDelivered(char* message, int length, int portId) {
	const rina::CDAPMessage * cdapMessage;
	const rina::CDAPSessionInterface * cdapSession;
	rina::CDAPSessionDescriptor  * cdapSessionDescriptor;
	IEnrollmentTask * enrollmentTask;

	//1 Decode the message and obtain the CDAP session descriptor
	atomic_send_lock_.lock();
	try{
		rina::SerializedMessage serializedMessage = rina::SerializedMessage(message, length);
		cdapMessage = cdap_session_manager_->messageReceived(serializedMessage, portId);
	} catch (Exception &e) {
		atomic_send_lock_.unlock();
		LOG_ERR("Error decoding CDAP message: %s", e.what());
		return;
	}

	cdapSession = cdap_session_manager_->get_cdap_session(portId);
	if (!cdapSession) {
		atomic_send_lock_.unlock();
		LOG_ERR("Could not find open CDAP session related to portId %d", portId);
		delete cdapMessage;
		return;
	}

	cdapSessionDescriptor = cdapSession->get_session_descriptor();
	LOG_DBG("Received CDAP message through portId %d: %s", portId,
			cdapMessage->to_string().c_str());
	atomic_send_lock_.unlock();

	//2 Find the message recipient and call it
	rina::CDAPMessage::Opcode opcode = cdapMessage->get_op_code();
	enrollmentTask = ipc_process_->get_enrollment_task();
	switch (opcode) {
	case rina::CDAPMessage::M_CONNECT:
		enrollmentTask->connect(cdapMessage, cdapSessionDescriptor);
		break;
	case rina::CDAPMessage::M_CONNECT_R:
		enrollmentTask->connectResponse(cdapMessage, cdapSessionDescriptor);
		break;
	case rina::CDAPMessage::M_RELEASE:
		enrollmentTask->release(cdapMessage, cdapSessionDescriptor);
		break;
	case rina::CDAPMessage::M_RELEASE_R:
		enrollmentTask->releaseResponse(cdapMessage, cdapSessionDescriptor);
		break;
	case rina::CDAPMessage::M_CREATE:
		processIncomingRequestMessage(cdapMessage, cdapSessionDescriptor);
		break;
	case rina::CDAPMessage::M_CREATE_R:
		processIncomingResponseMessage(cdapMessage, cdapSessionDescriptor);
		break;
	case rina::CDAPMessage::M_DELETE:
		processIncomingRequestMessage(cdapMessage, cdapSessionDescriptor);
		break;
	case rina::CDAPMessage::M_DELETE_R:
		processIncomingResponseMessage(cdapMessage, cdapSessionDescriptor);
		break;
	case rina::CDAPMessage::M_START:
		processIncomingRequestMessage(cdapMessage, cdapSessionDescriptor);
		break;
	case rina::CDAPMessage::M_START_R:
		processIncomingResponseMessage(cdapMessage, cdapSessionDescriptor);
		break;
	case rina::CDAPMessage::M_STOP:
		processIncomingRequestMessage(cdapMessage, cdapSessionDescriptor);
		break;
	case rina::CDAPMessage::M_STOP_R:
		processIncomingResponseMessage(cdapMessage, cdapSessionDescriptor);
		break;
	case rina::CDAPMessage::M_READ:
		processIncomingRequestMessage(cdapMessage, cdapSessionDescriptor);
		break;
	case rina::CDAPMessage::M_READ_R:
		processIncomingResponseMessage(cdapMessage, cdapSessionDescriptor);
		break;
	case rina::CDAPMessage::M_CANCELREAD:
		processIncomingRequestMessage(cdapMessage, cdapSessionDescriptor);
		break;
	case rina::CDAPMessage::M_CANCELREAD_R:
		processIncomingResponseMessage(cdapMessage, cdapSessionDescriptor);
		break;
	case rina::CDAPMessage::M_WRITE:
		processIncomingRequestMessage(cdapMessage, cdapSessionDescriptor);
		break;
	case rina::CDAPMessage::M_WRITE_R:
		processIncomingResponseMessage(cdapMessage, cdapSessionDescriptor);
		break;
	default:
		LOG_ERR("Unrecognized CDAP operation code: %d", cdapMessage->get_op_code());
		delete cdapMessage;
	}
}

void RIBDaemon::sendMessage(const rina::CDAPMessage& cdapMessage, int sessionId,
				ICDAPResponseMessageHandler * cdapMessageHandler) throw (Exception) {
	sendMessage(false, cdapMessage, sessionId, 0, cdapMessageHandler);
}

void RIBDaemon::sendMessageToAddress(const rina::CDAPMessage& cdapMessage, int sessionId,
		unsigned int address, ICDAPResponseMessageHandler * cdapMessageHandler) throw (Exception) {
	sendMessage(true, cdapMessage, sessionId, address, cdapMessageHandler);
}

void RIBDaemon::sendMessage(bool useAddress, const rina::CDAPMessage& cdapMessage, int sessionId,
			unsigned int address, ICDAPResponseMessageHandler * cdapMessageHandler) throw (Exception) {
	const rina::SerializedMessage * sdu;

	if (!cdapMessageHandler && cdapMessage.get_invoke_id() != 0
			&& cdapMessage.get_op_code() != rina::CDAPMessage::M_CONNECT
			&& cdapMessage.get_op_code() != rina::CDAPMessage::M_CONNECT_R
			&& cdapMessage.get_op_code() != rina::CDAPMessage::M_RELEASE
			&& cdapMessage.get_op_code() != rina::CDAPMessage::M_RELEASE_R
			&& cdapMessage.get_op_code() != rina::CDAPMessage::M_CANCELREAD_R
			&& cdapMessage.get_op_code() != rina::CDAPMessage::M_WRITE_R
			&& cdapMessage.get_op_code() != rina::CDAPMessage::M_READ_R
			&& cdapMessage.get_op_code() != rina::CDAPMessage::M_CREATE_R
			&& cdapMessage.get_op_code() != rina::CDAPMessage::M_DELETE_R
			&& cdapMessage.get_op_code() != rina::CDAPMessage::M_START_R
			&& cdapMessage.get_op_code() != rina::CDAPMessage::M_STOP_R) {
		throw Exception("Requested a response message but message handler is null");
	}

	atomic_send_lock_.lock();
	try {
		sdu = cdap_session_manager_->encodeNextMessageToBeSent(cdapMessage, sessionId);
		if (useAddress) {
			rina::kernelIPCProcess->sendMgmgtSDUToAddress(sdu->get_message(), sdu->get_size(), address);
			LOG_DBG("Sent CDAP message to address %ud: %s", address,
					cdapMessage.to_string().c_str());
		} else {
			rina::kernelIPCProcess->writeMgmgtSDUToPortId(sdu->get_message(), sdu->get_size(), sessionId);
			LOG_DBG("Sent CDAP message through port-id %d: %s" , sessionId,
					cdapMessage.to_string().c_str());
		}

		cdap_session_manager_->messageSent(cdapMessage, sessionId);
	} catch (Exception &e) {
		if (sdu) {
			delete sdu->get_message();
			delete sdu;
		}

		std::string reason = std::string(e.what());
		if (reason.compare("Flow closed") == 0) {
			cdap_session_manager_->removeCDAPSession(sessionId);
		}

		atomic_send_lock_.unlock();

		throw e;
	}

	delete sdu->get_message();
    delete sdu;

    if (cdapMessage.get_invoke_id() != 0
    		&& cdapMessage.get_op_code() != rina::CDAPMessage::M_CONNECT
    		&& cdapMessage.get_op_code() != rina::CDAPMessage::M_CONNECT_R
    		&& cdapMessage.get_op_code() != rina::CDAPMessage::M_RELEASE
    		&& cdapMessage.get_op_code() != rina::CDAPMessage::M_RELEASE_R
    		&& cdapMessage.get_op_code() != rina::CDAPMessage::M_CANCELREAD_R
    		&& cdapMessage.get_op_code() != rina::CDAPMessage::M_WRITE_R
    		&& cdapMessage.get_op_code() != rina::CDAPMessage::M_READ_R
    		&& cdapMessage.get_op_code() != rina::CDAPMessage::M_CREATE_R
    		&& cdapMessage.get_op_code() != rina::CDAPMessage::M_DELETE_R
    		&& cdapMessage.get_op_code() != rina::CDAPMessage::M_START_R
    		&& cdapMessage.get_op_code() != rina::CDAPMessage::M_STOP_R) {
    	handlers_lock_.lock();
    	handlers_waiting_for_reply_[cdapMessage.get_invoke_id()] = cdapMessageHandler;
    	handlers_lock_.unlock();
    }

	atomic_send_lock_.unlock();
}

void RIBDaemon::sendMessages(const std::list<const rina::CDAPMessage*>& cdapMessages,
			const IUpdateStrategy& updateStrategy) {
	//TODO
}

}
