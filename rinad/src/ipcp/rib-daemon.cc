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
	unsigned char* buffer = new unsigned char[data->get_max_sdu_size()];
	unsigned char* sdu;

	rina::ReadManagementSDUResult result;
	LOG_INFO("Starting Management SDU reader ...");
	while (true) {
		try {
		result = rina::kernelIPCProcess->readManagementSDU(buffer, data->get_max_sdu_size());
		} catch (Exception &e) {
			LOG_ERR("Problems reading management SDU: %s", e.what());
			continue;
		}

		sdu = new unsigned char[result.getBytesRead()];
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

}
