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

// Class WhatevercastNameSetRIBObject
WhateverCastNameSetRIBObject::WhateverCastNameSetRIBObject(IPCProcess * ipc_process) :
	BaseRIBObject(ipc_process, EncoderConstants::WHATEVERCAST_NAME_SET_RIB_OBJECT_CLASS,
			objectInstanceGenerator->getObjectInstance(),
			EncoderConstants::WHATEVERCAST_NAME_SET_RIB_OBJECT_NAME) {
	lock_ = new rina::Lockable();
}

WhateverCastNameSetRIBObject::~WhateverCastNameSetRIBObject() {
	if (lock_) {
		delete lock_;
	}
}

const void* WhateverCastNameSetRIBObject::get_value() const {
	return 0;
}

void WhateverCastNameSetRIBObject::remoteCreateObject(void * object_value,
		const std::string& object_name, int invoke_id,
		rina::CDAPSessionDescriptor * session_descriptor) {
	rina::AccessGuard g(*lock_);
	(void) session_descriptor;
	(void) invoke_id;
	std::list<rina::WhatevercastName *> namesToCreate;

	try {
		if (object_name.compare(EncoderConstants::WHATEVERCAST_NAME_SET_RIB_OBJECT_CLASS) == 0) {
			std::list<rina::WhatevercastName *> * names =
					(std::list<rina::WhatevercastName *> *) object_value;
			std::list<rina::WhatevercastName *>::const_iterator iterator;
			for(iterator = names->begin(); iterator != names->end(); ++iterator) {
				namesToCreate.push_back((*iterator));
			}

			delete names;
		} else {
			rina::WhatevercastName * name = (rina::WhatevercastName *) object_value;
			namesToCreate.push_back(name);
		}
	} catch (Exception &e) {
		LOG_ERR("Error decoding CDAP object value: %s", e.what());
	}

	if (namesToCreate.size() == 0) {
		LOG_DBG("No whatevercast name entries to create or update");
		return;
	}

	try {
		rib_daemon_->createObject(EncoderConstants::WHATEVERCAST_NAME_SET_RIB_OBJECT_CLASS,
				EncoderConstants::WHATEVERCAST_NAME_SET_RIB_OBJECT_NAME, &namesToCreate, 0);
	} catch (Exception &e) {
		LOG_ERR("Problems creating RIB object: %s", e.what());
	}
}

void WhateverCastNameSetRIBObject::createObject(const std::string& objectClass,
		const std::string& objectName,
		const void* objectValue) {
	(void) objectName;

	if (objectClass.compare(EncoderConstants::WHATEVERCAST_NAME_SET_RIB_OBJECT_CLASS) == 0) {
		std::list<rina::WhatevercastName *>::const_iterator iterator;
		std::list<rina::WhatevercastName *> * names =
				(std::list<rina::WhatevercastName *> *) objectValue;

		for (iterator = names->begin(); iterator != names->end(); ++iterator) {
			createName((*iterator));
		}
	} else {
		rina::WhatevercastName * currentName = (rina::WhatevercastName*) objectValue;
		createName(currentName);
	}
}

void WhateverCastNameSetRIBObject::createName(rina::WhatevercastName * name) {
	std::stringstream ss;
	ss<<EncoderConstants::WHATEVERCAST_NAME_SET_RIB_OBJECT_NAME<<EncoderConstants::SEPARATOR;
	ss<<name->rule_;
	BaseRIBObject * ribObject = new SimpleSetMemberRIBObject(ipc_process_,
			EncoderConstants::WHATEVERCAST_NAME_RIB_OBJECT_CLASS, ss.str(), name);
	add_child(ribObject);
	try {
		rib_daemon_->addRIBObject(ribObject);
	} catch(Exception &e){
		LOG_ERR("Problems adding object to the RIB: %s", e.what());
	}
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

void DirectoryForwardingTableEntryRIBObject::remoteCreateObject(void * object_value, const std::string& object_name,
		int invoke_id, rina::CDAPSessionDescriptor * session_descriptor) {
	rina::DirectoryForwardingTableEntry * entry;
	rina::DirectoryForwardingTableEntry * currentEntry;

	(void) invoke_id;

	try {
		entry = (rina::DirectoryForwardingTableEntry *) object_value;
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
		cdapSessionIds.push_back(session_descriptor->port_id_);
		NotificationPolicy notificationPolicy = NotificationPolicy(cdapSessionIds);
		try {
			rib_daemon_->createObject(EncoderConstants::DFT_ENTRY_RIB_OBJECT_CLASS, object_name,
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

void DirectoryForwardingTableEntryRIBObject::remoteDeleteObject(int invoke_id,
		rina::CDAPSessionDescriptor * session_descriptor) {
	std::list<int> cdapSessionIds;

	(void) invoke_id;

	cdapSessionIds.push_back(session_descriptor->port_id_);
	NotificationPolicy notificationPolicy = NotificationPolicy(cdapSessionIds);
	try {
		rib_daemon_->deleteObject(class_, name_, 0, &notificationPolicy);
	} catch (Exception &e) {
		LOG_ERR("Problems deleting RIB object: %s", e.what());
	}
}

void DirectoryForwardingTableEntryRIBObject::deleteObject(const void* objectValue)
{
        (void) objectValue;

	namespace_manager_->removeDFTEntry(ap_name_entry_);
	parent_->remove_child(name_);
	rib_daemon_->removeRIBObject(name_);
}

// Class DirectoryForwardingTableEntry Set RIB Object
DirectoryForwardingTableEntrySetRIBObject::DirectoryForwardingTableEntrySetRIBObject(IPCProcess * ipc_process):
		BaseRIBObject(ipc_process, EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_CLASS,
				objectInstanceGenerator->getObjectInstance(),
				EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_NAME) {
	namespace_manager_ = ipc_process_->get_namespace_manager();
	rib_daemon_->subscribeToEvent(IPCP_EVENT_CONNECTIVITY_TO_NEIGHBOR_LOST, this);
}

void DirectoryForwardingTableEntrySetRIBObject::deleteObjects(
		const std::list<std::string>& namesToDelete) {
	std::list<std::string>::const_iterator iterator;

	for(iterator = namesToDelete.begin(); iterator != namesToDelete.end(); ++iterator) {
		rib_daemon_->deleteObject(EncoderConstants::DFT_ENTRY_RIB_OBJECT_CLASS, *iterator, 0, 0);
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
			objectsToDelete.push_back((*iterator)->name_);
		}
	}
}

void DirectoryForwardingTableEntrySetRIBObject::remoteCreateObject(void * object_value,
		const std::string& object_name, int invoke_id, rina::CDAPSessionDescriptor * session_descriptor) {
	std::list<rina::DirectoryForwardingTableEntry *> entriesToCreateOrUpdate;

	(void) invoke_id;

	try {
		if (object_name.compare(EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_NAME) == 0) {
			std::list<rina::DirectoryForwardingTableEntry *> * entries =
					(std::list<rina::DirectoryForwardingTableEntry *> *) object_value;
			std::list<rina::DirectoryForwardingTableEntry *>::const_iterator iterator;
			for(iterator = entries->begin(); iterator != entries->end(); ++iterator) {
				populateEntriesToCreateList(*iterator, &entriesToCreateOrUpdate);
			}

			delete entries;
		} else {
			rina::DirectoryForwardingTableEntry * receivedEntry =
					(rina::DirectoryForwardingTableEntry *) object_value;
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
	cdapSessionIds.push_back(session_descriptor->port_id_);
	NotificationPolicy notificationPolicy = NotificationPolicy(cdapSessionIds);

	try {
		rib_daemon_->createObject(EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_CLASS,
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
		ss<<name_<<EncoderConstants::SEPARATOR<<currentEntry->getKey();
		BaseRIBObject * ribObject = new DirectoryForwardingTableEntryRIBObject(ipc_process_,
							ss.str(), currentEntry);
		add_child(ribObject);
		try {
			rib_daemon_->addRIBObject(ribObject);
		} catch(Exception &e){
			LOG_ERR("Problems adding object to the RIB: %s", e.what());
		}
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
			remove_child(ribObject->name_);
			try {
				rib_daemon_->removeRIBObject(ribObject->name_);
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

void NamespaceManager::set_dif_configuration(const rina::DIFConfiguration& dif_configuration) {
	LOG_DBG("DIF configuration set: %u", dif_configuration.address_);
}

void NamespaceManager::populateRIB() {
	try {
		BaseRIBObject * object = new DirectoryForwardingTableEntrySetRIBObject(ipc_process_);
		rib_daemon_->addRIBObject(object);
		object = new WhateverCastNameSetRIBObject(ipc_process_);
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

unsigned short NamespaceManager::getRegIPCProcessId(const rina::ApplicationProcessNamingInformation& apNamingInfo) {
	rina::ApplicationRegistrationInformation * regInfo;

	regInfo = registrations_.find(apNamingInfo.getEncodedString());
	if (!regInfo) {
		LOG_DBG("Could not find a registered application with code : %s"
				, apNamingInfo.getEncodedString().c_str());
		return 0;
	}

	return regInfo->ipcProcessId;
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
			event.applicationRegistrationInformation.appName;
	if (registrations_.find(appToRegister.getEncodedString())) {
		LOG_ERR("Application % is already registered in this IPC Process",
				appToRegister.getEncodedString().c_str());

		replyToIPCManagerRegister(event, -1);
		return;
	}

	rina::ApplicationRegistrationInformation * registration = new
			rina::ApplicationRegistrationInformation(
					event.applicationRegistrationInformation.applicationRegistrationType);
	registration->appName = event.applicationRegistrationInformation.appName;
	registration->difName = event.applicationRegistrationInformation.difName;
	registration->ipcProcessId = ipc_process_->get_id();
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
			registrations_.erase(event.applicationName.getEncodedString());

	if (!unregisteredApp) {
		LOG_ERR("Application %s is not registered in this IPC Process",
				event.applicationName.getEncodedString().c_str());
		replyToIPCManagerUnregister(event, -1);
		return;
	}

	LOG_INFO("Successfully unregistered application %s ",
			 unregisteredApp->appName.getEncodedString().c_str());

	result = replyToIPCManagerUnregister(event, 0);
	if (result == -1) {
		registrations_.put(event.applicationName.getEncodedString(), unregisteredApp);
		return;
	}

	std::list<int> cdapSessionIds;
	NotificationPolicy notificationPolicy = NotificationPolicy(cdapSessionIds);

	try {
		std::stringstream ss;
		ss<<EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_NAME;
		ss<<EncoderConstants::SEPARATOR<< unregisteredApp->appName.getEncodedString();
		rib_daemon_->deleteObject(EncoderConstants::DFT_ENTRY_RIB_OBJECT_CLASS,
				ss.str(), 0, &notificationPolicy);
	} catch (Exception &e) {
		LOG_ERR("Problems creating RIB object: %s", e.what());
	}

	delete unregisteredApp;
}

unsigned int NamespaceManager::getIPCProcessAddress(const std::string& process_name,
			const std::string& process_instance, const rina::AddressingConfiguration& address_conf) {
	std::list<rina::StaticIPCProcessAddress>::const_iterator it;
	LOG_DBG("I know %d addresses", address_conf.static_address_.size());
	for (it = address_conf.static_address_.begin();
			it != address_conf.static_address_.end(); ++it) {
		LOG_DBG("Candidate: %s, %s", it->ap_name_.c_str(), it->ap_instance_.c_str());
		if (it->ap_name_.compare(process_name) == 0 &&
				it->ap_instance_.compare(process_instance) == 0) {
			return it->address_;
		}
	}

	return 0;

}

unsigned int NamespaceManager::getAddressPrefix(const std::string& process_name,
		const rina::AddressingConfiguration& address_conf) {
	std::list<rina::AddressPrefixConfiguration>::const_iterator it;
	for (it = address_conf.address_prefixes_.begin();
			it != address_conf.address_prefixes_.end(); ++it) {
		if (process_name.find(it->organization_) != std::string::npos) {
			return it->address_prefix_;
		}
	}

	throw Exception("Unknown organization");
}

bool NamespaceManager::isAddressInUse(unsigned int address,
		const std::string& ipcp_name) {
	std::list<rina::Neighbor * >::const_iterator it;
	std::list<rina::Neighbor *> neighbors = ipc_process_->get_neighbors();

	for (it = neighbors.begin(); it != neighbors.end(); ++it) {
		if ((*it)->address_ == address) {
			if ((*it)->name_.processName.compare(ipcp_name) == 0) {
				return false;
			} else {
				return true;
			}
		}
	}

	return false;
}

bool NamespaceManager::isValidAddress(unsigned int address, const std::string& ipcp_name,
		const std::string& ipcp_instance) {
	if (address == 0) {
		return false;
	}

	//Check if we know the remote IPC Process address
	rina::AddressingConfiguration configuration = ipc_process_->get_dif_information().
			dif_configuration_.nsm_configuration_.addressing_configuration_;
	unsigned int knownAddress = getIPCProcessAddress(ipcp_name, ipcp_instance, configuration);
	if (knownAddress != 0) {
		if (address == knownAddress) {
			return true;
		} else {
			return false;
		}
	}

	//Check the prefix information
	try {
		unsigned int prefix = getAddressPrefix(ipcp_name, configuration);

		//Check if the address is within the range of the prefix
		if (address < prefix || address >= prefix + rina::AddressPrefixConfiguration::MAX_ADDRESSES_PER_PREFIX){
			return false;
		}
	} catch (Exception &e) {
		//We don't know the organization of the IPC Process
		return false;
	}

	return !isAddressInUse(address, ipcp_name);
}

unsigned int NamespaceManager::getValidAddress(const std::string& ipcp_name,
				const std::string& ipcp_instance) {
	rina::AddressingConfiguration configuration = ipc_process_->get_dif_information().
				dif_configuration_.nsm_configuration_.addressing_configuration_;
	unsigned int candidateAddress = getIPCProcessAddress(ipcp_name, ipcp_instance, configuration);
	if (candidateAddress != 0) {
		return candidateAddress;
	}

	unsigned int prefix = 0;

	try {
		prefix = getAddressPrefix(ipcp_name, configuration);
	} catch (Exception &e) {
		//We don't know the organization of the IPC Process
		return 0;
	}

	candidateAddress = prefix;
	while (candidateAddress < prefix + rina::AddressPrefixConfiguration::MAX_ADDRESSES_PER_PREFIX) {
		if (isAddressInUse(candidateAddress, ipcp_name)) {
			candidateAddress++;
		} else {
			return candidateAddress;
		}
	}

	return 0;
}

unsigned int NamespaceManager::getAdressByname(const rina::ApplicationProcessNamingInformation& name) {
	std::list<rina::Neighbor *> neighbors = ipc_process_->get_neighbors();
	std::list<rina::Neighbor *>::const_iterator it;
	for (it = neighbors.begin(); it != neighbors.end(); ++it) {
		if ((*it)->name_.processName.compare(name.processName) == 0) {
			return (*it)->address_;
		}
	}

	throw Exception("Unknown neighbor");
}

}
