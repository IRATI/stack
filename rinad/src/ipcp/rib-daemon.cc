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
#include <librina/common.h>
#include "rib-daemon.h"

namespace rinad {

//Class RIB
RIB::RIB()
{ }

RIB::~RIB() throw()
{ }

BaseRIBObject* RIB::getRIBObject(const std::string& objectClass,
                                 const std::string& objectName, bool check)
{
	BaseRIBObject* ribObject;
	std::map<std::string, BaseRIBObject*>::iterator it;

	lock();
	it = rib_.find(objectName);
	unlock();

	if (it == rib_.end()) {
		throw Exception("Could not find object in the RIB");
	}

	ribObject = it->second;
	if (check && ribObject->class_.compare(objectClass) != 0) {
		throw Exception("Object class does not match the user specified one");
	}

	return ribObject;
}

void RIB::addRIBObject(BaseRIBObject* ribObject)
{
	lock();
	if (rib_.find(ribObject->name_) != rib_.end()) {
		throw Exception("Object already exists in the RIB");
	}
	rib_[ribObject->name_] = ribObject;
	unlock();
}

BaseRIBObject * RIB::removeRIBObject(const std::string& objectName)
{
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

std::list<BaseRIBObject*> RIB::getRIBObjects()
{
	std::list<BaseRIBObject*> result;

	for (std::map<std::string, BaseRIBObject*>::iterator it = rib_.begin();
			it != rib_.end(); ++it) {
		result.push_back(it->second);
	}

	return result;
}

//Class ManagementSDUReader data
ManagementSDUReaderData::ManagementSDUReaderData(IRIBDaemon * rib_daemon,
                                                 unsigned int max_sdu_size)
{
	rib_daemon_ = rib_daemon;
	max_sdu_size_ = max_sdu_size;
}

void * doManagementSDUReaderWork(void* arg)
{
	ManagementSDUReaderData * data = (ManagementSDUReaderData *) arg;
	char* buffer = new char[data->max_sdu_size_];
	char* sdu;

	rina::ReadManagementSDUResult result;
	LOG_INFO("Starting Management SDU reader ...");
	while (true) {
		try {
		result = rina::kernelIPCProcess->readManagementSDU(buffer, data->max_sdu_size_);
		} catch (Exception &e) {
			LOG_ERR("Problems reading management SDU: %s", e.what());
			continue;
		}

		sdu = new char[result.getBytesRead()];
		for (int i=0; i<result.getBytesRead(); i++) {
			sdu[i] = buffer[i];
		}

		data->rib_daemon_->cdapMessageDelivered(sdu, result.getBytesRead(), result.getPortId());

		delete sdu;
	}

	delete buffer;

	return 0;
}

/// Class BaseRIBDaemon
BaseRIBDaemon::BaseRIBDaemon()
{ }

void BaseRIBDaemon::subscribeToEvent(const IPCProcessEventType& eventId,
                                     EventListener * eventListener)
{
	if (!eventListener)
		return;

	events_lock_.lock();

	std::map<IPCProcessEventType, std::list<EventListener*> >::iterator it = event_listeners_.find(eventId);
	if (it == event_listeners_.end()) {
		std::list<EventListener *> listenersList;
		listenersList.push_back(eventListener);
		event_listeners_[eventId] = listenersList;
	} else {
		std::list<EventListener *>::iterator listIterator;
		for (listIterator=it->second.begin(); listIterator != it->second.end(); ++listIterator) {
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

void BaseRIBDaemon::unsubscribeFromEvent(const IPCProcessEventType& eventId,
                                         EventListener *            eventListener)
{
	if (!eventListener)
		return;

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

void BaseRIBDaemon::deliverEvent(Event * event)
{
	if (!event)
		return;

	LOG_INFO("Event %d has just happened. Notifying event listeners.", event->get_id());

	events_lock_.lock();
	std::map<IPCProcessEventType, std::list<EventListener*> >::iterator it = event_listeners_.find(event->get_id());
	if (it == event_listeners_.end()) {
		events_lock_.unlock();
		delete event;
		return;
	}

	std::list<EventListener *>::iterator listIterator;
	for (listIterator=it->second.begin(); listIterator != it->second.end(); ++listIterator) {
		(*listIterator)->eventHappened(event);
	}

	events_lock_.unlock();

	if (event) {
		delete event;
	}
}

///Class RIBDaemon
RIBDaemon::RIBDaemon()
{
	ipc_process_ = 0;
	management_sdu_reader_ = 0;
	cdap_session_manager_ = 0;
	encoder_ = 0;
	n_minus_one_flow_manager_ = 0;
}

void RIBDaemon::set_ipc_process(IPCProcess * ipc_process)
{
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

void RIBDaemon::set_dif_configuration(const rina::DIFConfiguration& dif_configuration) {
	LOG_DBG("Configuration set: %u", dif_configuration.address_);
}

void RIBDaemon::subscribeToEvents()
{
	subscribeToEvent(IPCP_EVENT_N_MINUS_1_FLOW_ALLOCATED, this);
	subscribeToEvent(IPCP_EVENT_N_MINUS_1_FLOW_DEALLOCATED, this);
}

void RIBDaemon::eventHappened(Event * event)
{
	if (!event)
		return;

	if (event->get_id() == IPCP_EVENT_N_MINUS_1_FLOW_DEALLOCATED) {
		NMinusOneFlowDeallocatedEvent * flowEvent = (NMinusOneFlowDeallocatedEvent *) event;
		nMinusOneFlowDeallocated(flowEvent->port_id_);
	} else if (event->get_id() == IPCP_EVENT_N_MINUS_1_FLOW_ALLOCATED) {
		NMinusOneFlowAllocatedEvent * flowEvent = (NMinusOneFlowAllocatedEvent *) event;
		nMinusOneFlowAllocated(flowEvent);
	}
}

void RIBDaemon::nMinusOneFlowDeallocated(int portId) {
	cdap_session_manager_->removeCDAPSession(portId);
}

void RIBDaemon::nMinusOneFlowAllocated(NMinusOneFlowAllocatedEvent * event)
{
	if (!event)
		return;
}

void RIBDaemon::addRIBObject(BaseRIBObject * ribObject)
{
	if (!ribObject)
		throw Exception("Object is null");

	rib_.addRIBObject(ribObject);
	LOG_INFO("Object with name %s, class %s, instance %ld added to the RIB",
			ribObject->name_.c_str(), ribObject->class_.c_str(),
			ribObject->instance_);
}

void RIBDaemon::removeRIBObject(BaseRIBObject * ribObject)
{
	if (!ribObject)
		throw Exception("Object is null");

	removeRIBObject(ribObject->name_);
}

void RIBDaemon::removeRIBObject(const std::string& objectName)
{
	BaseRIBObject * object = rib_.removeRIBObject(objectName);
	LOG_INFO("Object with name %s, class %s, instance %ld removed from the RIB",
                 object->name_.c_str(), object->class_.c_str(),
                 object->instance_);
        
	delete object;
}

std::list<BaseRIBObject *> RIBDaemon::getRIBObjects()
{
	return rib_.getRIBObjects();
}

bool RIBDaemon::isOnList(int candidate, std::list<int> list)
{
	for (std::list<int>::iterator it = list.begin(); it != list.end(); ++it) {
		if ((*it) == candidate)
			return true;
	}

	return false;
}

void RIBDaemon::createObject(const std::string& objectClass,
                             const std::string& objectName,
                             const void* objectValue,
                             const NotificationPolicy * notificationPolicy) {
	BaseRIBObject * ribObject;

	try {
		ribObject = rib_.getRIBObject(objectClass, objectName, true);
	} catch (Exception &e) {
		//Delegate creation to the parent if the object is not there
                std::string::size_type position =
                        objectName.rfind(EncoderConstants::SEPARATOR);
		if (position == std::string::npos)
			throw e;
		std::string parentObjectName = objectName.substr(0, position);
		ribObject = rib_.getRIBObject(objectClass, parentObjectName, false);
	}

	//Create the object
	ribObject->createObject(objectClass, objectName, objectValue);

	//Notify neighbors if needed
	if (!notificationPolicy) {
		return;
	}

	//We need to notify, find out to whom the notifications must be sent to, and do it
	std::list<int> peersToIgnore = notificationPolicy->cdap_session_ids_;
	std::vector<int> peers;
	cdap_session_manager_->getAllCDAPSessionIds(peers);

	rina::CDAPMessage * cdapMessage = 0;

	for (std::vector<int>::size_type i = 0; i < peers.size(); i++) {
		if (!isOnList(peers[i], peersToIgnore)) {
			try {
				cdapMessage = cdap_session_manager_->
                                        getCreateObjectRequestMessage(peers[i], 0,
                                                                      rina::CDAPMessage::NONE_FLAGS,
                                                                      objectClass,
                                                                      0,
                                                                      objectName,
                                                                      0,
                                                                      false);
				encoder_->encode(objectValue, cdapMessage);
				sendMessage(*cdapMessage, peers[i], 0);
			} catch(Exception & e) {
				LOG_ERR("Problems notifying neighbors: %s", e.what());
			}

			delete cdapMessage;
		}
	}
}

void RIBDaemon::deleteObject(const std::string& objectClass,
                             const std::string& objectName,
                             const void* objectValue,
                             const NotificationPolicy * notificationPolicy)
{
	BaseRIBObject * ribObject;

	ribObject = rib_.getRIBObject(objectClass, objectName, true);
	ribObject->deleteObject(objectValue);

	//Notify neighbors if needed
	if (!notificationPolicy)
		return;

	//We need to notify, find out to whom the notifications must be sent to, and do it
	std::list<int> peersToIgnore = notificationPolicy->cdap_session_ids_;
	std::vector<int> peers;
	cdap_session_manager_->getAllCDAPSessionIds(peers);

	const rina::CDAPMessage * cdapMessage = 0;
	for (std::vector<int>::size_type i = 0; i<peers.size(); i++) {
		if (!isOnList(peers[i], peersToIgnore)) {
			try {
				cdapMessage =
                                        cdap_session_manager_->
                                        getDeleteObjectRequestMessage(peers[i], 0,
                                                                      rina::CDAPMessage::NONE_FLAGS,
                                                                      objectClass,
                                                                      0,
                                                                      objectName,
                                                                      0, false);
				sendMessage(*cdapMessage, peers[i], 0);
				delete cdapMessage;
			} catch(Exception & e) {
				LOG_ERR("Problems notifying neighbors: %s", e.what());
				if (cdapMessage) {
					delete cdapMessage;
				}
			}
		}
	}
}

BaseRIBObject * RIBDaemon::readObject(const std::string& objectClass,
                                      const std::string& objectName)
{ return rib_.getRIBObject(objectClass, objectName, true); }

void RIBDaemon::writeObject(const std::string& objectClass,
                            const std::string& objectName,
                            const void* objectValue)
{
	BaseRIBObject * object = rib_.getRIBObject(objectClass, objectName, true);
	object->writeObject(objectValue);
}

void RIBDaemon::startObject(const std::string& objectClass,
                            const std::string& objectName,
                            const void* objectValue)
{
	BaseRIBObject * object = rib_.getRIBObject(objectClass, objectName, true);
	object->startObject(objectValue);
}

void RIBDaemon::stopObject(const std::string& objectClass,
                           const std::string& objectName,
                           const void* objectValue)
{
	BaseRIBObject * object = rib_.getRIBObject(objectClass, objectName, true);
	object->stopObject(objectValue);
}

void RIBDaemon::processQueryRIBRequestEvent(const rina::QueryRIBRequestEvent& event)
{
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

ICDAPResponseMessageHandler * RIBDaemon::getCDAPMessageHandler(const rina::CDAPMessage * cdapMessage)
{
	ICDAPResponseMessageHandler * handler;

	if (!cdapMessage) {
		return 0;
	}

	if (cdapMessage->get_flags() != rina::CDAPMessage::F_RD_INCOMPLETE) {
		handler = handlers_waiting_for_reply_.erase(cdapMessage->get_invoke_id());
	} else {
		handler = handlers_waiting_for_reply_.find(cdapMessage->get_invoke_id());
	}

	return handler;
}

void RIBDaemon::processIncomingRequestMessage(const rina::CDAPMessage * cdapMessage,
                                              rina::CDAPSessionDescriptor * cdapSessionDescriptor)
{
	BaseRIBObject * ribObject = 0;
	void * decodedObject = 0;

	LOG_DBG("Remote operation %d called on object %s", cdapMessage->get_op_code(),
			cdapMessage->get_obj_name().c_str());
	try {
		switch (cdapMessage->get_op_code()) {
		case rina::CDAPMessage::M_CREATE:
			decodedObject = encoder_->decode(cdapMessage);

			// Creation is delegated to the parent objects if the object doesn't exist.
			// Create semantics are CREATE or UPDATE. If the object exists it is an
			// update, therefore the message is handled to the object. If the object
			// doesn't exist it is a CREATE, therefore it is handled to the parent object
			try {
				ribObject = rib_.getRIBObject(cdapMessage->get_obj_class(),
											cdapMessage->get_obj_name(), true);
				ribObject->remoteCreateObject(decodedObject, cdapMessage->obj_name_,
						cdapMessage->invoke_id_, cdapSessionDescriptor);
			} catch (Exception &e) {
				//Look for parent object, delegate creation there
                                std::string::size_type position =
                                        cdapMessage->get_obj_name().rfind(EncoderConstants::SEPARATOR);
				if (position == std::string::npos) {
					throw e;
				}
				std::string parentObjectName = cdapMessage->get_obj_name().substr(0, position);
				ribObject = rib_.getRIBObject(cdapMessage->get_obj_class(), parentObjectName, true);
				ribObject->remoteCreateObject(decodedObject, cdapMessage->obj_name_,
						cdapMessage->invoke_id_, cdapSessionDescriptor);
			}

			break;
		case rina::CDAPMessage::M_DELETE:
			ribObject = rib_.getRIBObject(cdapMessage->get_obj_class(),
							cdapMessage->get_obj_name(), true);
			ribObject->remoteDeleteObject(cdapMessage->invoke_id_,
					cdapSessionDescriptor);
			break;
		case rina::CDAPMessage::M_START:
			if (cdapMessage->obj_value_) {
				decodedObject = encoder_->decode(cdapMessage);
			}
			ribObject = rib_.getRIBObject(cdapMessage->get_obj_class(),
							cdapMessage->get_obj_name(), true);
			ribObject->remoteStartObject(decodedObject, cdapMessage->invoke_id_,
					cdapSessionDescriptor);
			break;
		case rina::CDAPMessage::M_STOP:
			if (cdapMessage->obj_value_) {
				decodedObject = encoder_->decode(cdapMessage);
			}
			ribObject = rib_.getRIBObject(cdapMessage->get_obj_class(),
							cdapMessage->get_obj_name(), true);
			ribObject->remoteStopObject(decodedObject, cdapMessage->invoke_id_,
					cdapSessionDescriptor);
			break;
		case rina::CDAPMessage::M_READ:
			ribObject = rib_.getRIBObject(cdapMessage->get_obj_class(),
							cdapMessage->get_obj_name(), true);
			ribObject->remoteReadObject(cdapMessage->invoke_id_,
					cdapSessionDescriptor);
			break;
		case rina::CDAPMessage::M_CANCELREAD:
			ribObject = rib_.getRIBObject(cdapMessage->get_obj_class(),
							cdapMessage->get_obj_name(), true);
			ribObject->remoteCancelReadObject(cdapMessage->invoke_id_,
					cdapSessionDescriptor);
			break;
		case rina::CDAPMessage::M_WRITE:
			decodedObject = encoder_->decode(cdapMessage);
			ribObject = rib_.getRIBObject(cdapMessage->get_obj_class(),
							cdapMessage->get_obj_name(), true);
			ribObject->remoteWriteObject(decodedObject, cdapMessage->invoke_id_,
					cdapSessionDescriptor);
			break;
		default:
			LOG_ERR("Invalid operation code for a request message: %d", cdapMessage->get_op_code());
		}
	} catch(Exception &e) {
		LOG_ERR("Problems processing incoming CDAP request message %s", e.what());
	}

	return;
}

void RIBDaemon::processIncomingResponseMessage(const rina::CDAPMessage * cdapMessage,
                                               rina::CDAPSessionDescriptor * cdapSessionDescriptor)
{
	ICDAPResponseMessageHandler * handler;

	handler = getCDAPMessageHandler(cdapMessage);
	if (!handler) {
		LOG_ERR("Could not find a message handler for invoke-id %d",
				cdapMessage->get_invoke_id());
		return;
	}

	try {
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
		}
	} catch (Exception &e) {
		LOG_ERR("Problems processing CDAP response message: %s", e.what());
	}
}

void RIBDaemon::cdapMessageDelivered(char* message, int length, int portId)
{
	const rina::CDAPMessage * cdapMessage;
	const rina::CDAPSessionInterface * cdapSession;
	rina::CDAPSessionDescriptor  * cdapSessionDescriptor;
	IEnrollmentTask * enrollmentTask;

	//1 Decode the message and obtain the CDAP session descriptor
	atomic_send_lock_.lock();
	try {
		rina::SerializedObject serializedMessage = rina::SerializedObject(message, length);
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
	try {
		switch (opcode) {
		case rina::CDAPMessage::M_CONNECT:
			enrollmentTask->connect(cdapMessage, cdapSessionDescriptor);
			delete cdapMessage;
			break;
		case rina::CDAPMessage::M_CONNECT_R:
			enrollmentTask->connectResponse(cdapMessage, cdapSessionDescriptor);
			delete cdapMessage;
			break;
		case rina::CDAPMessage::M_RELEASE:
			enrollmentTask->release(cdapMessage, cdapSessionDescriptor);
			delete cdapMessage;
			break;
		case rina::CDAPMessage::M_RELEASE_R:
			enrollmentTask->releaseResponse(cdapMessage, cdapSessionDescriptor);
			delete cdapMessage;
			break;
		case rina::CDAPMessage::M_CREATE:
			processIncomingRequestMessage(cdapMessage, cdapSessionDescriptor);
			delete cdapMessage;
			break;
		case rina::CDAPMessage::M_CREATE_R:
			processIncomingResponseMessage(cdapMessage, cdapSessionDescriptor);
			delete cdapMessage;
			break;
		case rina::CDAPMessage::M_DELETE:
			processIncomingRequestMessage(cdapMessage, cdapSessionDescriptor);
			delete cdapMessage;
			break;
		case rina::CDAPMessage::M_DELETE_R:
			processIncomingResponseMessage(cdapMessage, cdapSessionDescriptor);
			delete cdapMessage;
			break;
		case rina::CDAPMessage::M_START:
			processIncomingRequestMessage(cdapMessage, cdapSessionDescriptor);
			delete cdapMessage;
			break;
		case rina::CDAPMessage::M_START_R:
			processIncomingResponseMessage(cdapMessage, cdapSessionDescriptor);
			delete cdapMessage;
			break;
		case rina::CDAPMessage::M_STOP:
			processIncomingRequestMessage(cdapMessage, cdapSessionDescriptor);
			delete cdapMessage;
			break;
		case rina::CDAPMessage::M_STOP_R:
			processIncomingResponseMessage(cdapMessage, cdapSessionDescriptor);
			delete cdapMessage;
			break;
		case rina::CDAPMessage::M_READ:
			processIncomingRequestMessage(cdapMessage, cdapSessionDescriptor);
			delete cdapMessage;
			break;
		case rina::CDAPMessage::M_READ_R:
			processIncomingResponseMessage(cdapMessage, cdapSessionDescriptor);
			delete cdapMessage;
			break;
		case rina::CDAPMessage::M_CANCELREAD:
			processIncomingRequestMessage(cdapMessage, cdapSessionDescriptor);
			delete cdapMessage;
			break;
		case rina::CDAPMessage::M_CANCELREAD_R:
			processIncomingResponseMessage(cdapMessage, cdapSessionDescriptor);
			delete cdapMessage;
			break;
		case rina::CDAPMessage::M_WRITE:
			processIncomingRequestMessage(cdapMessage, cdapSessionDescriptor);
			delete cdapMessage;
			break;
		case rina::CDAPMessage::M_WRITE_R:
			processIncomingResponseMessage(cdapMessage, cdapSessionDescriptor);
			delete cdapMessage;
			break;
		default:
			LOG_ERR("Unrecognized CDAP operation code: %d", cdapMessage->get_op_code());
			delete cdapMessage;
		}
	} catch(Exception &e) {
		LOG_ERR("Problems processing incoming CDAP message: %s", e.what());
		delete cdapMessage;
	}
}

void RIBDaemon::sendMessage(const rina::CDAPMessage& cdapMessage, int sessionId,
                            ICDAPResponseMessageHandler * cdapMessageHandler)
{
	sendMessage(false, cdapMessage, sessionId, 0, cdapMessageHandler);
}

void RIBDaemon::sendMessageToAddress(const rina::CDAPMessage& cdapMessage,
                                     int sessionId,
                                     unsigned int address,
                                     ICDAPResponseMessageHandler * cdapMessageHandler)
{
	sendMessage(true, cdapMessage, sessionId, address, cdapMessageHandler);
}

void RIBDaemon::sendMessage(bool useAddress, const rina::CDAPMessage& cdapMessage, int sessionId,
			unsigned int address, ICDAPResponseMessageHandler * cdapMessageHandler) {
	const rina::SerializedObject * sdu;

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
        sdu = 0;
	try {
		sdu = cdap_session_manager_->encodeNextMessageToBeSent(cdapMessage, sessionId);
		if (useAddress) {
			rina::kernelIPCProcess->sendMgmgtSDUToAddress(sdu->get_message(), sdu->get_size(), address);
			LOG_DBG("Sent CDAP message to address %ud: %s", address,
					cdapMessage.to_string().c_str());
		} else {
			rina::kernelIPCProcess->writeMgmgtSDUToPortId(sdu->get_message(), sdu->get_size(), sessionId);
			LOG_DBG("Sent CDAP message of size %d through port-id %d: %s" , sdu->get_size(), sessionId,
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
    	handlers_waiting_for_reply_.put(cdapMessage.get_invoke_id(), cdapMessageHandler);
    }

	atomic_send_lock_.unlock();
}

void
RIBDaemon::sendMessages(const std::list<const rina::CDAPMessage*>& cdapMessages,
			const IUpdateStrategy& updateStrategy)
{
        (void) cdapMessages; // Stop compiler barfs
        (void) updateStrategy; // Stop compiler barfs

	//TODO
}

}
