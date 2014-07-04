//
// Namespace Manager
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

#include <sstream>

#define RINA_PREFIX "namespace-manager"

#include <librina/logs.h>

#include "namespace-manager.h"

namespace rinad {

//	CLASS WhatevercastName
bool WhatevercastName::operator==(const WhatevercastName &other) {
	if (name_ == other.name_) {
		return true;
	}
	return false;
}

std::string WhatevercastName::toString() {
	std::string result = "Name: " + name_ + "\n";
	result = result + "Rule: " + rule_;
	return result;
}

// Class DirectoryForwardingTableEntry RIB Object
DirectoryForwardingTableEntryRIBObject::DirectoryForwardingTableEntryRIBObject(IPCProcess * ipc_process,
		const std::string& object_name, rina::DirectoryForwardingTableEntry * entry):
			SimpleSetMemberRIBObject(ipc_process, EncoderConstants::DFT_ENTRY_RIB_OBJECT_CLASS,
					object_name, entry){
	namespace_manager_ = ipc_process->get_namespace_manager();
	namespace_manager_->addDFTEntry(entry);
	ap_name_entry_ = entry->get_ap_naming_info();
}

void DirectoryForwardingTableEntryRIBObject::remoteCreateObject(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	rina::DirectoryForwardingTableEntry * entry;
	rina::DirectoryForwardingTableEntry * currentEntry;

	try {
		rina::ByteArrayObjectValue * value = (rina::ByteArrayObjectValue*)  cdapMessage->get_obj_value();
		rina::SerializedObject * serializedObject = (rina::SerializedObject *) value->get_value();
		entry = (rina::DirectoryForwardingTableEntry *)
					get_encoder()->decode(*serializedObject, EncoderConstants::DFT_ENTRY_RIB_OBJECT_CLASS);
	} catch (Exception & e){
		LOG_ERR("Problems decoding message: %s", e.what());
		return;
	}

	if (entry->getKey().compare(ap_name_entry_.getEncodedString()) != 0){
		LOG_ERR("Keys of the received and existing entries are different, cannot update");
		delete entry;
		return;
	}

	currentEntry = namespace_manager_->getDFTEntry(ap_name_entry_);
	if (currentEntry->get_address() != entry->get_address()) {
		currentEntry->set_address(entry->get_address());
		std::list<int> cdapSessionIds;
		cdapSessionIds.push_back(cdapSessionDescriptor->get_port_id());
		NotificationPolicy notificationPolicy = NotificationPolicy(cdapSessionIds);
		try {
			get_rib_daemon()->createObject(cdapMessage->get_obj_class(), cdapMessage->get_obj_name(),
					currentEntry, &notificationPolicy);
		} catch (Exception &e) {
			LOG_ERR("Problems creating RIB object: %s", e.what());
		}
	}

	delete entry;
}

void DirectoryForwardingTableEntryRIBObject::createObject(const std::string& objectClass,
                                                          const std::string& objectName,
                                                          const void* objectValue)
{
        (void) objectClass; // Stop compiler barfs;
        (void) objectName; // Stop compiler barfs;
        (void) objectValue; // Stop compiler barfs

	//Do nothing
}

void DirectoryForwardingTableEntryRIBObject::remoteDeleteObject(const rina::CDAPMessage * cdapMessage,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	std::list<int> cdapSessionIds;
	cdapSessionIds.push_back(cdapSessionDescriptor->get_port_id());
	NotificationPolicy notificationPolicy = NotificationPolicy(cdapSessionIds);
	try {
		get_rib_daemon()->deleteObject(cdapMessage->get_obj_class(), cdapMessage->get_obj_name(),
				0, &notificationPolicy);
	} catch (Exception &e) {
		LOG_ERR("Problems deleting RIB object: %s", e.what());
	}
}

void DirectoryForwardingTableEntryRIBObject::deleteObject(const void* objectValue)
{
        (void) objectValue;

	namespace_manager_->removeDFTEntry(ap_name_entry_);
	get_parent()->remove_child(get_name());
	get_rib_daemon()->removeRIBObject(get_name());
}

// Class DirectoryForwardingTableEntry Set RIB Object
DirectoryForwardingTableEntrySetRIBObject::DirectoryForwardingTableEntrySetRIBObject(IPCProcess * ipc_process):
		BaseRIBObject(ipc_process, EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_CLASS,
				objectInstanceGenerator->getObjectInstance(),
				EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_NAME) {
	namespace_manager_ = get_ipc_process()->get_namespace_manager();
	get_rib_daemon()->subscribeToEvent(IPCP_EVENT_CONNECTIVITY_TO_NEIGHBOR_LOST, this);
}

void DirectoryForwardingTableEntrySetRIBObject::deleteObjects(
		const std::list<std::string>& namesToDelete) {
	std::list<std::string>::const_iterator iterator;

	for(iterator = namesToDelete.begin(); iterator != namesToDelete.end(); ++iterator) {
		get_rib_daemon()->deleteObject(EncoderConstants::DFT_ENTRY_RIB_OBJECT_CLASS, *iterator, 0, 0);
	}
}

void DirectoryForwardingTableEntrySetRIBObject::eventHappened(Event * event) {
	if (event->get_id() != IPCP_EVENT_CONNECTIVITY_TO_NEIGHBOR_LOST) {
		return;
	}

	ConnectiviyToNeighborLostEvent * conEvent = (ConnectiviyToNeighborLostEvent *) event;
	std::list<std::string> objectsToDelete;

	rina::DirectoryForwardingTableEntry * entry;
	std::list<BaseRIBObject *>::const_iterator iterator;
	for (iterator = get_children().begin(); iterator != get_children().end(); ++iterator) {
		entry = (rina::DirectoryForwardingTableEntry *) (*iterator)->get_value();
		if (entry->get_address() == conEvent->neighbor_->get_address()) {
			objectsToDelete.push_back((*iterator)->get_name());
		}
	}
}

void DirectoryForwardingTableEntrySetRIBObject::remoteCreateObject(
		const rina::CDAPMessage * cdapMessage, rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	std::list<rina::DirectoryForwardingTableEntry *> entriesToCreateOrUpdate;

	try {
		rina::ByteArrayObjectValue * value = (rina::ByteArrayObjectValue*)  cdapMessage->get_obj_value();
		rina::SerializedObject * serializedObject = (rina::SerializedObject *) value->get_value();

		if (cdapMessage->get_obj_name().compare(EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_NAME) == 0) {
			std::list<rina::DirectoryForwardingTableEntry *> * entries =
					(std::list<rina::DirectoryForwardingTableEntry *> *)
						get_encoder()->decode(*serializedObject,
								EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_CLASS);
			std::list<rina::DirectoryForwardingTableEntry *>::const_iterator iterator;
			for(iterator = entries->begin(); iterator != entries->end(); ++iterator) {
				populateEntriesToCreateList(*iterator, &entriesToCreateOrUpdate);
			}

			delete entries;
		} else {
			rina::DirectoryForwardingTableEntry * receivedEntry = (rina::DirectoryForwardingTableEntry *)
					get_encoder()->decode(*serializedObject,
							EncoderConstants::DFT_ENTRY_RIB_OBJECT_CLASS);
			populateEntriesToCreateList(receivedEntry, &entriesToCreateOrUpdate);
		}
	} catch (Exception &e) {
		LOG_ERR("Error decoding CDAP object value: %s", e.what());
	}

	if (entriesToCreateOrUpdate.size() == 0) {
		LOG_DBG("No DFT entries to create or update");
		return;
	}

	std::list<int> cdapSessionIds;
	cdapSessionIds.push_back(cdapSessionDescriptor->get_port_id());
	NotificationPolicy notificationPolicy = NotificationPolicy(cdapSessionIds);

	try {
		get_rib_daemon()->createObject(EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_CLASS,
				EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_NAME, &entriesToCreateOrUpdate,
				&notificationPolicy);
	} catch (Exception &e) {
		LOG_ERR("Problems creating RIB object: %s", e.what());
	}
}

void DirectoryForwardingTableEntrySetRIBObject::populateEntriesToCreateList(rina::DirectoryForwardingTableEntry* entry,
		std::list<rina::DirectoryForwardingTableEntry *> * list) {
	rina::DirectoryForwardingTableEntry * currentEntry;

	currentEntry = namespace_manager_->getDFTEntry(entry->get_ap_naming_info());
	if (!currentEntry) {
		list->push_back(entry);
	} else if (currentEntry->get_address() != entry->get_address()) {
		currentEntry->set_address(entry->get_address());
		delete entry;
	} else {
		delete entry;
	}
}

void DirectoryForwardingTableEntrySetRIBObject::createObject(const std::string& objectClass,
                                                             const std::string& objectName,
                                                             const void*        objectValue)
{
        (void) objectClass; // Stop compiler barfs
        (void) objectName;  // Stop compiler barfs

	std::list<rina::DirectoryForwardingTableEntry *>::const_iterator iterator;
	rina::DirectoryForwardingTableEntry * currentEntry;
	std::list<rina::DirectoryForwardingTableEntry *> * entries =
						(std::list<rina::DirectoryForwardingTableEntry *> *) objectValue;

	for (iterator = entries->begin(); iterator != entries->end(); ++iterator) {
		currentEntry = *iterator;
		if (namespace_manager_->getDFTEntry(currentEntry->get_ap_naming_info())) {
			continue;
		}

		namespace_manager_->addDFTEntry(currentEntry);

		std::stringstream ss;
		ss<<get_name()<<EncoderConstants::SEPARATOR<<currentEntry->getKey();
		BaseRIBObject * ribObject = new DirectoryForwardingTableEntryRIBObject(get_ipc_process(),
							ss.str(), currentEntry);
		add_child(ribObject);
		try {
			get_rib_daemon()->addRIBObject(ribObject);
		} catch(Exception &e){
			LOG_ERR("Problems adding object to the RIB: %s", e.what());
		}
	}
}

void DirectoryForwardingTableEntrySetRIBObject::remoteDeleteObject(const rina::CDAPMessage * cdapMessage,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	std::list<rina::DirectoryForwardingTableEntry *> entriesToDelete;

	try {
		rina::ByteArrayObjectValue * value = (rina::ByteArrayObjectValue*)  cdapMessage->get_obj_value();
		rina::SerializedObject * serializedObject = (rina::SerializedObject *) value->get_value();

		if (cdapMessage->get_obj_name().compare(EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_NAME) == 0) {
			std::list<rina::DirectoryForwardingTableEntry *> * entries =
					(std::list<rina::DirectoryForwardingTableEntry *> *)
						get_encoder()->decode(*serializedObject,
								EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_CLASS);
			std::list<rina::DirectoryForwardingTableEntry *>::const_iterator iterator;
			for(iterator = entries->begin(); iterator != entries->end(); ++iterator) {
				populateEntriesToDeleteList(*iterator, &entriesToDelete);
			}

			delete entries;
		} else {
			rina::DirectoryForwardingTableEntry * receivedEntry = (rina::DirectoryForwardingTableEntry *)
								get_encoder()->decode(*serializedObject,
										EncoderConstants::DFT_ENTRY_RIB_OBJECT_CLASS);
			populateEntriesToDeleteList(receivedEntry, &entriesToDelete);
		}
	} catch (Exception &e) {
		LOG_ERR("Error decoding CDAP object value: %s", e.what());
	}

	if (entriesToDelete.size() == 0) {
		LOG_DBG("No DFT entries to delete");
		return;
	}

	std::list<int> cdapSessionIds;
	cdapSessionIds.push_back(cdapSessionDescriptor->get_port_id());
	NotificationPolicy notificationPolicy = NotificationPolicy(cdapSessionIds);

	try {
		get_rib_daemon()->deleteObject(EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_CLASS,
				EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_NAME, &entriesToDelete,
				&notificationPolicy);
	} catch (Exception &e) {
		LOG_ERR("Problems creating RIB object: %s", e.what());
	}
}

void DirectoryForwardingTableEntrySetRIBObject::populateEntriesToDeleteList(rina::DirectoryForwardingTableEntry* entry,
		std::list<rina::DirectoryForwardingTableEntry *> * list) {
	rina::DirectoryForwardingTableEntry * currentEntry;

	currentEntry = namespace_manager_->getDFTEntry(entry->get_ap_naming_info());
	if (currentEntry) {
		list->push_back(currentEntry);
	}

	delete entry;
}

void DirectoryForwardingTableEntrySetRIBObject::deleteObject(const void* objectValue)
{
	std::list<rina::DirectoryForwardingTableEntry *>::const_iterator iterator;
	//rina::DirectoryForwardingTableEntry * currentEntry;
	std::list<rina::DirectoryForwardingTableEntry *> * entries =
			(std::list<rina::DirectoryForwardingTableEntry *> *) objectValue;

	BaseRIBObject * ribObject;
	for (iterator = entries->begin(); iterator != entries->end(); ++iterator) {
		ribObject = getObject ((*iterator)->getKey());
		if (ribObject) {
			remove_child(ribObject->get_name());
			try {
				get_rib_daemon()->removeRIBObject(ribObject->get_name());
			} catch (Exception &e) {
				LOG_ERR("Problems removing object from the RIB: %s", e.what());
			}
		} else {
			LOG_WARN("Could not find object to delete in the PDU FT");
		}
	}
}

BaseRIBObject * DirectoryForwardingTableEntrySetRIBObject::getObject(const std::string& candidateKey) {
	rina::DirectoryForwardingTableEntry * entry;
	std::list<BaseRIBObject *>::const_iterator iterator;

	for (iterator = get_children().begin(); iterator != get_children().end(); ++iterator) {
		entry = (rina::DirectoryForwardingTableEntry *) (*iterator)->get_value();
		if (entry->getKey().compare(candidateKey) == 0) {
			return *iterator;
		}
	}

	return 0;
}

const void* DirectoryForwardingTableEntrySetRIBObject::get_value() const {
	return 0;
}

//Class Namespace Manager
NamespaceManager::NamespaceManager() {
	ipc_process_ = 0;
	rib_daemon_ = 0;
}

void NamespaceManager::set_ipc_process(IPCProcess * ipc_process) {
	ipc_process_ = ipc_process;
	rib_daemon_ = ipc_process->get_rib_daemon();
	populateRIB();
}

void NamespaceManager::populateRIB() {
	try {
		BaseRIBObject * object = new DirectoryForwardingTableEntrySetRIBObject(ipc_process_);
		rib_daemon_->addRIBObject(object);
	} catch (Exception &e) {
		LOG_ERR("Problems adding object to the RIB : %s", e.what());
	}
}

unsigned int NamespaceManager::getDFTNextHop(const rina::ApplicationProcessNamingInformation& apNamingInfo) {
	rina::DirectoryForwardingTableEntry * nextHop;

	nextHop = dft_.find(apNamingInfo.getEncodedString());
	if (nextHop) {
		return nextHop->get_address();
	}

	return 0;
}

void NamespaceManager::addDFTEntry(rina::DirectoryForwardingTableEntry * entry) {
	dft_.put(entry->getKey(), entry);
	LOG_DBG("Added entry to DFT: %s", entry->toString().c_str());
}

rina::DirectoryForwardingTableEntry * NamespaceManager::getDFTEntry(
			const rina::ApplicationProcessNamingInformation& apNamingInfo) {
	return dft_.find(apNamingInfo.getEncodedString());
}

void NamespaceManager::removeDFTEntry(const rina::ApplicationProcessNamingInformation& apNamingInfo){
	rina::DirectoryForwardingTableEntry * entry =
			dft_.erase(apNamingInfo.getEncodedString());
	if (entry) {
		LOG_DBG("Removed entry form DFT: %s", entry->toString().c_str());
		delete entry;
	}
}

int NamespaceManager::replyToIPCManagerRegister(const rina::ApplicationRegistrationRequestEvent& event,
		int result) {
	try {
		rina::extendedIPCManager->registerApplicationResponse(event, result);
	} catch (Exception &e) {
		LOG_ERR("Problems communicating with the IPC Manager: %s", e.what());
		return -1;
	}

	return 0;
}

void NamespaceManager::processApplicationRegistrationRequestEvent(
		const rina::ApplicationRegistrationRequestEvent& event) {
	int result = 0;

	rina::ApplicationProcessNamingInformation appToRegister =
			event.getApplicationRegistrationInformation().getApplicationName();
	if (registrations_.find(appToRegister.getEncodedString())) {
		LOG_ERR("Application % is already registered in this IPC Process",
				appToRegister.getEncodedString().c_str());

		replyToIPCManagerRegister(event, -1);
		return;
	}

	rina::ApplicationRegistrationInformation * registration = new
			rina::ApplicationRegistrationInformation(
					event.getApplicationRegistrationInformation().getRegistrationType());
	registration->setApplicationName(event.getApplicationRegistrationInformation().getApplicationName());
	registration->setDIFName(event.getApplicationRegistrationInformation().getDIFName());
	registration->setIpcProcessId(ipc_process_->get_id());
	registrations_.put(appToRegister.getEncodedString(), registration);
	LOG_INFO("Successfully registered application %s with IPC Process id %us",
			appToRegister.getEncodedString().c_str(), ipc_process_->get_id());
	result = replyToIPCManagerRegister(event, 0);
	if (result == -1) {
		registrations_.erase(appToRegister.getEncodedString());
		delete registration;
		return;
	}

	std::list<rina::DirectoryForwardingTableEntry *> entriesToCreate;
	rina::DirectoryForwardingTableEntry * entry = new rina::DirectoryForwardingTableEntry();
	entry->set_address(ipc_process_->get_address());
	entry->set_ap_naming_info(appToRegister);
	entriesToCreate.push_back(entry);

	std::list<int> cdapSessionIds;
	NotificationPolicy notificationPolicy = NotificationPolicy(cdapSessionIds);

	try {
		rib_daemon_->createObject(EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_CLASS,
				EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_NAME, &entriesToCreate,
				&notificationPolicy);
	} catch (Exception &e) {
		LOG_ERR("Problems creating RIB object: %s", e.what());
	}
}

int NamespaceManager::replyToIPCManagerUnregister(const rina::ApplicationUnregistrationRequestEvent& event,
		int result) {
	try {
		rina::extendedIPCManager->unregisterApplicationResponse(event, result);
	} catch (Exception &e) {
		LOG_ERR("Problems communicating with the IPC Manager: %s", e.what());
		return -1;
	}

	return 0;
}

void NamespaceManager::processApplicationUnregistrationRequestEvent(
		const rina::ApplicationUnregistrationRequestEvent& event) {
	int result = 0;

	rina::ApplicationRegistrationInformation * unregisteredApp =
			registrations_.erase(event.getApplicationName().getEncodedString());

	if (!unregisteredApp) {
		LOG_ERR("Application %s is not registered in this IPC Process",
				event.getApplicationName().getEncodedString().c_str());
		replyToIPCManagerUnregister(event, -1);
		return;
	}

	LOG_INFO("Successfully unregistered application %s ",
			 unregisteredApp->getApplicationName().getEncodedString().c_str());

	result = replyToIPCManagerUnregister(event, 0);
	if (result == -1) {
		registrations_.put(event.getApplicationName().getEncodedString(), unregisteredApp);
		return;
	}

	std::list<int> cdapSessionIds;
	NotificationPolicy notificationPolicy = NotificationPolicy(cdapSessionIds);

	try {
		std::stringstream ss;
		ss<<EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_NAME;
		ss<<EncoderConstants::SEPARATOR<< unregisteredApp->getApplicationName().getEncodedString();
		rib_daemon_->deleteObject(EncoderConstants::DFT_ENTRY_RIB_OBJECT_CLASS,
				ss.str(), 0, &notificationPolicy);
	} catch (Exception &e) {
		LOG_ERR("Problems creating RIB object: %s", e.what());
	}

	delete unregisteredApp;
}

}
