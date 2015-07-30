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

#define IPCP_MODULE "namespace-manager"
#include "ipcp-logging.h"

#include "namespace-manager.h"

namespace rinad {

// Class WhatevercastNameSetRIBObject
WhateverCastNameSetRIBObject::WhateverCastNameSetRIBObject(IPCProcess * ipc_process) :
	BaseIPCPRIBObject(ipc_process, EncoderConstants::WHATEVERCAST_NAME_SET_RIB_OBJECT_CLASS,
			rina::objectInstanceGenerator->getObjectInstance(),
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
	rina::ScopedLock g(*lock_);
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
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Error decoding CDAP object value: %s", e.what());
	}

	if (namesToCreate.size() == 0) {
		LOG_IPCP_DBG("No whatevercast name entries to create or update");
		return;
	}

	try {
		rib_daemon_->createObject(EncoderConstants::WHATEVERCAST_NAME_SET_RIB_OBJECT_CLASS,
				EncoderConstants::WHATEVERCAST_NAME_SET_RIB_OBJECT_NAME, &namesToCreate, 0);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems creating RIB object: %s", e.what());
	}
}

void WhateverCastNameSetRIBObject::createObject(const std::string& objectClass,
		const std::string& objectName,
		const void* objectValue) {

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
	BaseRIBObject * ribObject = new SimpleSetMemberIPCPRIBObject(ipc_process_,
			EncoderConstants::WHATEVERCAST_NAME_RIB_OBJECT_CLASS, ss.str(), name);
	add_child(ribObject);
	try {
		rib_daemon_->addRIBObject(ribObject);
	} catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems adding object to the RIB: %s", e.what());
	}
}

// Class DirectoryForwardingTableEntry RIB Object
DirectoryForwardingTableEntryRIBObject::DirectoryForwardingTableEntryRIBObject(IPCProcess * ipc_process,
		const std::string& object_name, rina::DirectoryForwardingTableEntry * entry):
			SimpleSetMemberIPCPRIBObject(ipc_process, EncoderConstants::DFT_ENTRY_RIB_OBJECT_CLASS,
					object_name, entry){
	namespace_manager_ = ipc_process->namespace_manager_;
	namespace_manager_->addDFTEntry(entry);
	ap_name_entry_ = entry->get_ap_naming_info();
}

void DirectoryForwardingTableEntryRIBObject::remoteCreateObject(void * object_value, const std::string& object_name,
		int invoke_id, rina::CDAPSessionDescriptor * session_descriptor) {
	rina::DirectoryForwardingTableEntry * entry;
	rina::DirectoryForwardingTableEntry * currentEntry;

	try {
		entry = (rina::DirectoryForwardingTableEntry *) object_value;
	} catch (rina::Exception & e){
		LOG_IPCP_ERR("Problems decoding message: %s", e.what());
		return;
	}

	if (entry->getKey().compare(ap_name_entry_.getEncodedString()) != 0){
		LOG_IPCP_ERR("Keys of the received and existing entries are different, cannot update");
		delete entry;
		return;
	}

	currentEntry = namespace_manager_->getDFTEntry(ap_name_entry_);
	if (currentEntry->get_address() != entry->get_address()) {
		currentEntry->set_address(entry->get_address());
		std::list<int> cdapSessionIds;
		cdapSessionIds.push_back(session_descriptor->port_id_);
		rina::NotificationPolicy notificationPolicy = rina::NotificationPolicy(cdapSessionIds);
		try {
			rib_daemon_->createObject(EncoderConstants::DFT_ENTRY_RIB_OBJECT_CLASS, object_name,
					currentEntry, &notificationPolicy);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems creating RIB object: %s", e.what());
		}
	}

	delete entry;
}

void DirectoryForwardingTableEntryRIBObject::createObject(const std::string& objectClass,
                                                          const std::string& objectName,
                                                          const void* objectValue)
{
	//Do nothing
}

void DirectoryForwardingTableEntryRIBObject::remoteDeleteObject(int invoke_id,
		rina::CDAPSessionDescriptor * session_descriptor) {
	std::list<int> cdapSessionIds;

	cdapSessionIds.push_back(session_descriptor->port_id_);
	rina::NotificationPolicy notificationPolicy = rina::NotificationPolicy(cdapSessionIds);
	try {
		rib_daemon_->deleteObject(class_, name_, 0, &notificationPolicy);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems deleting RIB object: %s", e.what());
	}
}

void DirectoryForwardingTableEntryRIBObject::deleteObject(const void* objectValue)
{
	namespace_manager_->removeDFTEntry(ap_name_entry_);
	parent_->remove_child(name_);
	rib_daemon_->removeRIBObject(name_);
}

std::string DirectoryForwardingTableEntryRIBObject::get_displayable_value() {
    const rina::DirectoryForwardingTableEntry * dfte =
    		(const rina::DirectoryForwardingTableEntry *) get_value();
    std::stringstream ss;
    ss << "App name: " << dfte->ap_naming_info_.getEncodedString();
    ss << "; Address: " << dfte->address_;
    ss << "; Timestamp: " << dfte->timestamp_;

    return ss.str();
}

// Class DirectoryForwardingTableEntry Set RIB Object
DirectoryForwardingTableEntrySetRIBObject::DirectoryForwardingTableEntrySetRIBObject(IPCProcess * ipc_process):
		BaseIPCPRIBObject(ipc_process, EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_CLASS,
				rina::objectInstanceGenerator->getObjectInstance(),
				EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_NAME)
{
	namespace_manager_ = ipc_process_->namespace_manager_;
	ipc_process->internal_event_manager_->subscribeToEvent(
			rina::InternalEvent::APP_CONNECTIVITY_TO_NEIGHBOR_LOST, this);
}

void DirectoryForwardingTableEntrySetRIBObject::deleteObjects(
		const std::list<std::string>& namesToDelete) {
	std::list<std::string>::const_iterator iterator;

	for(iterator = namesToDelete.begin(); iterator != namesToDelete.end(); ++iterator) {
		rib_daemon_->deleteObject(EncoderConstants::DFT_ENTRY_RIB_OBJECT_CLASS, *iterator, 0, 0);
	}
}

void DirectoryForwardingTableEntrySetRIBObject::eventHappened(rina::InternalEvent * event)
{
	if (event->type != rina::InternalEvent::APP_CONNECTIVITY_TO_NEIGHBOR_LOST)
		return;

	rina::ConnectiviyToNeighborLostEvent * conEvent =
		(rina::ConnectiviyToNeighborLostEvent *) event;
	std::list<std::string> objectsToDelete;

	rina::DirectoryForwardingTableEntry * entry;
		std::list<BaseRIBObject *>::const_iterator iterator;
	for (iterator = get_children().begin(); iterator != get_children().end(); ++iterator) {
		entry = (rina::DirectoryForwardingTableEntry *) (*iterator)->get_value();
		LOG_IPCP_DBG("Entry pointer: %p", entry);
		if (entry->get_address() == conEvent->neighbor_.get_address()) {
			objectsToDelete.push_back((*iterator)->name_);
		}
	}
}

void DirectoryForwardingTableEntrySetRIBObject::remoteCreateObject(void * object_value,
		const std::string& object_name, int invoke_id, rina::CDAPSessionDescriptor * session_descriptor) {
	std::list<rina::DirectoryForwardingTableEntry *> entriesToCreateOrUpdate;

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
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Error decoding CDAP object value: %s", e.what());
	}

	if (entriesToCreateOrUpdate.size() == 0) {
		LOG_IPCP_DBG("No DFT entries to create or update");
		return;
	}

	std::list<int> cdapSessionIds;
	cdapSessionIds.push_back(session_descriptor->port_id_);
	rina::NotificationPolicy notificationPolicy = rina::NotificationPolicy(cdapSessionIds);

	try {
		rib_daemon_->createObject(EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_CLASS,
				EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_NAME, &entriesToCreateOrUpdate,
				&notificationPolicy);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems creating RIB object: %s", e.what());
	}
}

void DirectoryForwardingTableEntrySetRIBObject::populateEntriesToCreateList(rina::DirectoryForwardingTableEntry* entry,
		std::list<rina::DirectoryForwardingTableEntry *> * list)
{
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
	std::list<rina::DirectoryForwardingTableEntry *>::const_iterator iterator;
	rina::DirectoryForwardingTableEntry * currentEntry;
	std::list<rina::DirectoryForwardingTableEntry *> * entries =
						(std::list<rina::DirectoryForwardingTableEntry *> *) objectValue;

	for (iterator = entries->begin(); iterator != entries->end(); ++iterator) {
		currentEntry = *iterator;
		if (namespace_manager_->getDFTEntry(currentEntry->get_ap_naming_info())) {
			continue;
		}

		std::stringstream ss;
		ss<<name_<<EncoderConstants::SEPARATOR<<currentEntry->getKey();
		BaseRIBObject * ribObject = new DirectoryForwardingTableEntryRIBObject(ipc_process_,
							ss.str(), currentEntry);
		add_child(ribObject);
		try {
			rib_daemon_->addRIBObject(ribObject);
		} catch(rina::Exception &e){
			LOG_IPCP_ERR("Problems adding object to the RIB: %s", e.what());
		}
	}
}

void DirectoryForwardingTableEntrySetRIBObject::populateEntriesToDeleteList(rina::DirectoryForwardingTableEntry* entry,
		std::list<rina::DirectoryForwardingTableEntry *> * list)
{
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
			} catch (rina::Exception &e) {
				LOG_IPCP_ERR("Problems removing object from the RIB: %s", e.what());
			}
		} else {
			LOG_IPCP_WARN("Could not find object to delete in the PDU FT");
		}
	}
}

rina::BaseRIBObject * DirectoryForwardingTableEntrySetRIBObject::getObject(const std::string& candidateKey)
{
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

const void* DirectoryForwardingTableEntrySetRIBObject::get_value() const
{
	return 0;
}

//Class Namespace Manager
NamespaceManager::NamespaceManager() : INamespaceManager()
{
	rib_daemon_ = 0;
}

void NamespaceManager::set_application_process(rina::ApplicationProcess * ap)
{
	if (!ap)
		return;

	app = ap;
	ipcp = dynamic_cast<IPCProcess*>(app);
	if (!ipcp) {
		LOG_IPCP_ERR("Bogus instance of IPCP passed, return");
		return;
	}

	rib_daemon_ = ipcp->rib_daemon_;
	populateRIB();
}

void NamespaceManager::set_dif_configuration(const rina::DIFConfiguration& dif_configuration)
{
	std::string ps_name = dif_configuration.nsm_configuration_.policy_set_.name_;
	if (select_policy_set(std::string(), ps_name) != 0) {
		throw rina::Exception("Cannot create namespace manager policy-set");
	}
}

void NamespaceManager::populateRIB()
{
	try {
		BaseIPCPRIBObject * object = new DirectoryForwardingTableEntrySetRIBObject(ipcp);
		rib_daemon_->addRIBObject(object);
		object = new WhateverCastNameSetRIBObject(ipcp);
		rib_daemon_->addRIBObject(object);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems adding object to the RIB : %s", e.what());
	}
}

unsigned int NamespaceManager::getDFTNextHop(const rina::ApplicationProcessNamingInformation& apNamingInfo)
{
	rina::DirectoryForwardingTableEntry * nextHop;

	nextHop = dft_.find(apNamingInfo.getEncodedString());
	if (nextHop) {
		return nextHop->get_address();
	}

	return 0;
}

void NamespaceManager::addDFTEntry(rina::DirectoryForwardingTableEntry * entry)
{
	dft_.put(entry->getKey(), entry);
	LOG_IPCP_DBG("Added entry to DFT: %s", entry->toString().c_str());
}

rina::DirectoryForwardingTableEntry * NamespaceManager::getDFTEntry(
			const rina::ApplicationProcessNamingInformation& apNamingInfo)
{
	return dft_.find(apNamingInfo.getEncodedString());
}

void NamespaceManager::removeDFTEntry(const rina::ApplicationProcessNamingInformation& apNamingInfo)
{
	rina::DirectoryForwardingTableEntry * entry =
			dft_.erase(apNamingInfo.getEncodedString());
	if (entry) {
		LOG_IPCP_DBG("Removed entry form DFT: %s", entry->toString().c_str());
		delete entry;
	}
}

unsigned short NamespaceManager::getRegIPCProcessId(const rina::ApplicationProcessNamingInformation& apNamingInfo)
{
	rina::ApplicationRegistrationInformation * regInfo;

	regInfo = registrations_.find(apNamingInfo.getEncodedString());
	if (!regInfo) {
		LOG_IPCP_DBG("Could not find a registered application with code : %s"
				, apNamingInfo.getEncodedString().c_str());
		return 0;
	}

	return regInfo->ipcProcessId;
}

int NamespaceManager::replyToIPCManagerRegister(const rina::ApplicationRegistrationRequestEvent& event,
		int result)
{
	try {
		rina::extendedIPCManager->registerApplicationResponse(event, result);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
		return -1;
	}

	return 0;
}

void NamespaceManager::processApplicationRegistrationRequestEvent(
		const rina::ApplicationRegistrationRequestEvent& event)
{
	int result = 0;

	rina::ApplicationProcessNamingInformation appToRegister =
			event.applicationRegistrationInformation.appName;
	if (registrations_.find(appToRegister.getEncodedString())) {
		LOG_IPCP_ERR("Application % is already registered in this IPC Process",
				appToRegister.getEncodedString().c_str());

		replyToIPCManagerRegister(event, -1);
		return;
	}

	rina::ApplicationRegistrationInformation * registration = new
			rina::ApplicationRegistrationInformation(
					event.applicationRegistrationInformation.applicationRegistrationType);
	registration->appName = event.applicationRegistrationInformation.appName;
	registration->difName = event.applicationRegistrationInformation.difName;
	registration->ipcProcessId = event.applicationRegistrationInformation.ipcProcessId;
	registrations_.put(appToRegister.getEncodedString(), registration);
	LOG_IPCP_INFO("Successfully registered application %s with IPC Process id %us",
			appToRegister.getEncodedString().c_str(), ipcp->get_id());
	result = replyToIPCManagerRegister(event, 0);
	if (result == -1) {
		registrations_.erase(appToRegister.getEncodedString());
		delete registration;
		return;
	}

	std::list<rina::DirectoryForwardingTableEntry *> entriesToCreate;
	rina::DirectoryForwardingTableEntry * entry = new rina::DirectoryForwardingTableEntry();
	entry->set_address(ipcp->get_address());
	entry->set_ap_naming_info(appToRegister);
	entriesToCreate.push_back(entry);

	std::list<int> cdapSessionIds;
	rina::NotificationPolicy notificationPolicy = rina::NotificationPolicy(cdapSessionIds);

	try {
		rib_daemon_->createObject(EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_CLASS,
				EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_NAME, &entriesToCreate,
				&notificationPolicy);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems creating RIB object: %s", e.what());
	}
}

int NamespaceManager::replyToIPCManagerUnregister(const rina::ApplicationUnregistrationRequestEvent& event,
		int result)
{
	try {
		rina::extendedIPCManager->unregisterApplicationResponse(event, result);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems communicating with the IPC Manager: %s", e.what());
		return -1;
	}

	return 0;
}

rina::ApplicationRegistrationInformation
	NamespaceManager::get_reg_app_info(const rina::ApplicationProcessNamingInformation name)
{
	rina::ApplicationRegistrationInformation * result = registrations_.find(name.getEncodedString());
	if (result != 0) {
		return *result;
	} else {
		throw rina::Exception("Could not locate application registration");
	}
}

void NamespaceManager::processApplicationUnregistrationRequestEvent(
		const rina::ApplicationUnregistrationRequestEvent& event)
{
	int result = 0;

	rina::ApplicationRegistrationInformation * unregisteredApp =
			registrations_.erase(event.applicationName.getEncodedString());

	if (!unregisteredApp) {
		LOG_IPCP_ERR("Application %s is not registered in this IPC Process",
				event.applicationName.getEncodedString().c_str());
		replyToIPCManagerUnregister(event, -1);
		return;
	}

	LOG_IPCP_INFO("Successfully unregistered application %s ",
			 unregisteredApp->appName.getEncodedString().c_str());

	result = replyToIPCManagerUnregister(event, 0);
	if (result == -1) {
		registrations_.put(event.applicationName.getEncodedString(), unregisteredApp);
		return;
	}

	std::list<int> cdapSessionIds;
	rina::NotificationPolicy notificationPolicy = rina::NotificationPolicy(cdapSessionIds);

	try {
		std::stringstream ss;
		ss<<EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_NAME;
		ss<<EncoderConstants::SEPARATOR<< unregisteredApp->appName.getEncodedString();
		rib_daemon_->deleteObject(EncoderConstants::DFT_ENTRY_RIB_OBJECT_CLASS,
				ss.str(), 0, &notificationPolicy);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems creating RIB object: %s", e.what());
	}

	delete unregisteredApp;
}

unsigned int NamespaceManager::getAdressByname(const rina::ApplicationProcessNamingInformation& name)
{
	std::list<rina::Neighbor *> neighbors = ipcp->get_neighbors();
	std::list<rina::Neighbor *>::const_iterator it;
	for (it = neighbors.begin(); it != neighbors.end(); ++it) {
		if ((*it)->name_.processName.compare(name.processName) == 0) {
			return (*it)->address_;
		}
	}

	throw rina::Exception("Unknown neighbor");
}

} //namespace rinad
