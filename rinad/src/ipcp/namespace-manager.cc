//
// Namespace Manager
//
//    Bernat Gaston <bernat.gaston@i2cat.net>
//    Eduard Grasa <eduard.grasa@i2cat.net>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301  USA
//

#include <assert.h>
#include <sstream>

#define IPCP_MODULE "namespace-manager"
#include "ipcp-logging.h"

#include "namespace-manager.h"
#include "common/encoder.h"

namespace rinad {

// Class WhateverCastNameRIBObj
const std::string WhateverCastNameRIBObj::class_name = "Neighbor";
const std::string WhateverCastNameRIBObj::object_name_prefix = "/difm/nsm/whatnms/name=";

WhateverCastNameRIBObj::WhateverCastNameRIBObj(rina::WhatevercastName* name_) :
		rina::rib::RIBObj(class_name), name(name_)
{
}

const std::string WhateverCastNameRIBObj::get_displayable_value() const
{
    std::stringstream ss;
    ss << "Name: " << name->name_;
    ss << "; Rule: " << name->rule_;

    //TODO add set members

    return ss.str();
}

void WhateverCastNameRIBObj::read(const rina::cdap_rib::con_handle_t &con,
				  const std::string& fqn,
				  const std::string& class_,
				  const rina::cdap_rib::filt_info_t &filt,
				  const int invoke_id,
				  rina::cdap_rib::obj_info_t &obj_reply,
				  rina::cdap_rib::res_info_t& res)
{
	if (name) {
		encoders::WhatevercastNameEncoder encoder;
		encoder.encode(*name, obj_reply.value_);
	}

	res.code_ = rina::cdap_rib::CDAP_SUCCESS;
}

// Class WhateverCastNamesRIBObj
const std::string WhateverCastNamesRIBObj::class_name = "Neighbor";
const std::string WhateverCastNamesRIBObj::object_name = "/difm/nsm/whatnms";

WhateverCastNamesRIBObj::WhateverCastNamesRIBObj(IPCProcess * ipc_process) :
	IPCPRIBObj(ipc_process, class_name)
{
}

WhateverCastNamesRIBObj::~WhateverCastNamesRIBObj()
{
}

void WhateverCastNamesRIBObj::create(const rina::cdap_rib::con_handle_t &con,
				     const std::string& fqn,
				     const std::string& class_,
				     const rina::cdap_rib::filt_info_t &filt,
				     const int invoke_id,
				     const rina::ser_obj_t &obj_req,
				     rina::ser_obj_t &obj_reply,
				     rina::cdap_rib::res_info_t& res)
{
	std::list<rina::WhatevercastName> namesToCreate;
	encoders::WhatevercastNameListEncoder encoder;

	//1 Decode list of names
	encoder.decode(obj_req, namesToCreate);

	//2 Iterate list and create names
	std::list<rina::WhatevercastName>::iterator it;
	for (it = namesToCreate.begin(); it != namesToCreate.end(); ++it) {
		rina::WhatevercastName * name = new rina::WhatevercastName(*it);
		ipc_process_->namespace_manager_->add_whatevercast_name(name);
	}
}

// Class DFTEntryRIBObj
const std::string DFTEntryRIBObj::class_name = "DirectoryForwardingTableEntry";
const std::string DFTEntryRIBObj::object_name_prefix = "/difm/nsm/dft/key=";

DFTEntryRIBObj::DFTEntryRIBObj(IPCProcess * ipcp,
			       rina::DirectoryForwardingTableEntry* entry_) :
		IPCPRIBObj(ipcp, class_name), entry(entry_)
{
	nsm = ipc_process_->namespace_manager_;
}

const std::string DFTEntryRIBObj::get_displayable_value() const
{
	std::stringstream ss;
	ss << "App name: " << entry->ap_naming_info_.getEncodedString();
	ss << "; Address: " << entry->address_;
	ss << "; Sequence number: " << entry->seqnum_;

	return ss.str();
}

bool DFTEntryRIBObj::delete_(const rina::cdap_rib::con_handle_t &con_handle,
			     const std::string& fqn,
			     const std::string& class_,
			     const rina::cdap_rib::filt_info_t &filt,
			     const int invoke_id,
			     rina::cdap_rib::res_info_t& res)
{
	std::list<int> exc_neighs;
	exc_neighs.push_back(con_handle.port_id);
	nsm->removeDFTEntry(entry->getKey(),
			    true,
			    false,
			    exc_neighs);
	return true;
}

void DFTEntryRIBObj::read(const rina::cdap_rib::con_handle_t &con,
			  const std::string& fqn,
			  const std::string& class_,
			  const rina::cdap_rib::filt_info_t &filt,
			  const int invoke_id,
			  rina::cdap_rib::obj_info_t &obj_reply,
			  rina::cdap_rib::res_info_t& res)
{
	if (entry) {
		encoders::DFTEEncoder encoder;
		encoder.encode(*entry, obj_reply.value_);
	}

	res.code_ = rina::cdap_rib::CDAP_SUCCESS;
}

// Class DirectoryForwardingTableEntry Set RIB Object
const std::string DFTRIBObj::class_name = "DirectoryForwardingTable";
const std::string DFTRIBObj::object_name = "/difm/nsm/dft";

DFTRIBObj::DFTRIBObj(IPCProcess * ipc_process):
		IPCPRIBObj(ipc_process, class_name)
{
	namespace_manager_ = ipc_process_->namespace_manager_;
	ipc_process->internal_event_manager_->subscribeToEvent(rina::InternalEvent::APP_CONNECTIVITY_TO_NEIGHBOR_LOST,
							       this);
}

void DFTRIBObj::eventHappened(rina::InternalEvent * event)
{
	if (event->type != rina::InternalEvent::APP_CONNECTIVITY_TO_NEIGHBOR_LOST)
		return;

	rina::ConnectiviyToNeighborLostEvent * conEvent =
		(rina::ConnectiviyToNeighborLostEvent *) event;

	CheckDFTEntriesToRemoveTimerTask * task = new CheckDFTEntriesToRemoveTimerTask(this,
										       conEvent->neighbor_.address_,
										       conEvent->neighbor_.name_.processName);
	timer.scheduleTask(task, 5000);
}

void DFTRIBObj::checkDFTEntriesToRemove(unsigned int address,
					const std::string& name)
{
	rina::ScopedLock g(lock);
	std::list<rina::RoutingTableEntry> nextHops;
	std::list<std::string> entriesToDelete;

	//1 Check if we still have connectivity to neighbor
	nextHops = namespace_manager_->ipcp->resource_allocator_->get_rt_entries();
	for (std::list<rina::RoutingTableEntry>::iterator it = nextHops.begin();
			it != nextHops.end(); ++it) {
		if (it->destination.name == name)
			return;
	}

	//2 If not, remove entries
	std::list<rina::DirectoryForwardingTableEntry> entries = namespace_manager_->getDFTEntries();
	std::list<rina::DirectoryForwardingTableEntry>::const_iterator iterator;
	for (iterator = entries.begin(); iterator != entries.end(); ++iterator) {
		if (iterator->get_address() == address)
			entriesToDelete.push_back(iterator->getKey());
	}

	if (entriesToDelete.size() == 0)
		return;

	std::list<int> exc_neighs;
	std::list<std::string>::const_iterator it;
	for (it = entriesToDelete.begin(); it != entriesToDelete.end(); ++it) {
		namespace_manager_->removeDFTEntry(*it,
				true,
				true,
				exc_neighs);
	}

}

void DFTRIBObj::create(const rina::cdap_rib::con_handle_t &con_handle,
		       const std::string& fqn,
		       const std::string& class_,
		       const rina::cdap_rib::filt_info_t &filt,
		       const int invoke_id,
		       const rina::ser_obj_t &obj_req,
		       rina::ser_obj_t &obj_reply,
		       rina::cdap_rib::res_info_t& res)
{
	rina::ScopedLock g(lock);

	std::list<rina::DirectoryForwardingTableEntry> entriesToCreateOrUpdate;
	std::list<rina::DirectoryForwardingTableEntry> entriesToCreate;
	std::list<rina::DirectoryForwardingTableEntry> entriesToUpdate;
	rina::DirectoryForwardingTableEntry * entry;
	encoders::DFTEListEncoder encoder;

	//1 Decode list of names
	encoder.decode(obj_req, entriesToCreateOrUpdate);

	//2 Iterate list and create or update entries
	std::list<rina::DirectoryForwardingTableEntry>::iterator it;
	for (it = entriesToCreateOrUpdate.begin(); it != entriesToCreateOrUpdate.end(); ++it) {
		entry = namespace_manager_->getDFTEntry(it->getKey());
		if (!entry) {
			entriesToCreate.push_back(*it);
		} else if (it->seqnum_ > entry->seqnum_){
			entry->address_ = it->address_;
			entry->seqnum_ = it->seqnum_;
			LOG_IPCP_INFO("Updated application %s IPCP address to %d",
				       it->getKey().c_str(),
				       it->address_);
			entriesToUpdate.push_back(*it);
		}
	}

	std::list<int> exc_neighs;
	exc_neighs.push_back(con_handle.port_id);

	if (entriesToCreate.size() > 0) {
		namespace_manager_->addDFTEntries(entriesToCreate,
						  true,
						  exc_neighs);
	}

	if (entriesToUpdate.size() > 0) {
		namespace_manager_->notify_neighbors_add(entriesToUpdate,
							 exc_neighs);
	}
}

//Class AddressChangeTimerTask
AddressChangeTimerTask::AddressChangeTimerTask(INamespaceManager * nsm,
		       	       	       	       unsigned int naddr,
					       unsigned int oaddr)
{
	namespace_manager = nsm;
	new_address = naddr;
	old_address = oaddr;
}

void AddressChangeTimerTask::run()
{
	namespace_manager->addressChangeUpdateDFT(new_address,
						  old_address);
}

//Class Namespace Manager
NamespaceManager::NamespaceManager() : INamespaceManager()
{
	rib_daemon_ = 0;
	event_manager_ = 0;
}

NamespaceManager::~NamespaceManager()
{
	delete ps;
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
	event_manager_ = ipcp->internal_event_manager_;
	subscribeToEvents();
	populateRIB();
}

void NamespaceManager::set_dif_configuration(const rina::DIFInformation& dif_information)
{
	std::string ps_name = dif_information.dif_configuration_.nsm_configuration_.policy_set_.name_;
	if (select_policy_set(std::string(), ps_name) != 0) {
		throw rina::Exception("Cannot create namespace manager policy-set");
	}

	INamespaceManagerPs *nsmps = dynamic_cast<INamespaceManagerPs *> (ps);
	assert(nsmps);
	nsmps->set_dif_configuration(dif_information.dif_configuration_);
}

void NamespaceManager::populateRIB()
{
	rina::rib::RIBObj* tmp;

	try {
		tmp = new rina::rib::RIBObj("NamespaceManager");
		rib_daemon_->addObjRIB("/difm/nsm", &tmp);

		tmp = new WhateverCastNamesRIBObj(ipcp);
		rib_daemon_->addObjRIB(WhateverCastNamesRIBObj::object_name, &tmp);

		tmp = new DFTRIBObj(ipcp);
		rib_daemon_->addObjRIB(DFTRIBObj::object_name, &tmp);
	} catch (rina::Exception &e) {
		LOG_ERR("Problems adding object to the RIB : %s", e.what());
	}
}

void NamespaceManager::subscribeToEvents()
{
	event_manager_->subscribeToEvent(rina::InternalEvent::ADDRESS_CHANGE,
					 this);
}

void NamespaceManager::eventHappened(rina::InternalEvent * event)
{
	if (event->type == rina::InternalEvent::ADDRESS_CHANGE){
		rina::AddressChangeEvent * addrEvent =
				(rina::AddressChangeEvent *) event;
		addressChange(addrEvent);
	}
}

void NamespaceManager::addressChange(rina::AddressChangeEvent * event)
{
	//Set timer to modify entries in DFT (first give enough time
	//to routing to advertise new address)
	AddressChangeTimerTask * task = new AddressChangeTimerTask(this,
								   event->new_address,
								   event->old_address);
	timer.scheduleTask(task, event->use_new_timeout);
}

void NamespaceManager::addressChangeUpdateDFT(unsigned int new_address,
			    	    	      unsigned int old_address)
{
	std::list<rina::DirectoryForwardingTableEntry> mod_entries;
	rina::DirectoryForwardingTableEntry * entry;
	std::vector<int> session_ids;

	rina::ScopedLock g(lock);

	std::list<std::string> keys = dft_.getKeys();
	for(std::list<std::string>::iterator it = keys.begin();
			it != keys.end(); ++it) {
		entry = dft_.find(*it);
		if (entry->address_ == old_address) {
			entry->address_ = new_address;
			entry->seqnum_ = entry->seqnum_ + 1;
			mod_entries.push_back(*entry);
		}
	}

	if (mod_entries.size() == 0)
		return;

	rina::cdap::getProvider()->get_session_manager()->getAllCDAPSessionIds(session_ids);
	encoders::DFTEListEncoder encoder;
	rina::cdap_rib::obj_info_t obj;
	obj.class_ = DFTRIBObj::class_name;
	obj.name_ = DFTRIBObj::object_name;
	encoder.encode(mod_entries, obj.value_);
	rina::cdap_rib::flags_t flags;
	rina::cdap_rib::filt_info_t filt;
	rina::cdap_rib::con_handle_t con;
	for (int i = 0; i < session_ids.size(); i++) {
		try {
			con.port_id = session_ids[i];
			ipcp->rib_daemon_->getProxy()->remote_create(con,
								     obj,
								     flags,
								     filt,
								     NULL);
		} catch (rina::Exception &e) {
			LOG_WARN("Problems sending create CDAP message: %s",
					e.what());
		}
	}
}

unsigned int NamespaceManager::getDFTNextHop(rina::ApplicationProcessNamingInformation& apNamingInfo)
{
	rina::DirectoryForwardingTableEntry * nextHop = 0;
	std::list<rina::DirectoryForwardingTableEntry *> entries;
	std::list<rina::DirectoryForwardingTableEntry *>::iterator it;
	unsigned int my_address = 0;

	rina::ScopedLock g(lock);

	// Searching for a DAP name (specific DAF member)
	nextHop = dft_.find(apNamingInfo.getEncodedString());
	if (nextHop) {
		return nextHop->get_address();
	}

	if (apNamingInfo.processInstance == "" &&
			apNamingInfo.entityName == "" &&
			apNamingInfo.entityInstance == "") {
		//Searching for a DAF name
		my_address = ipcp->get_active_address();
		entries = dft_.getEntries();
		for (it = entries.begin(); it != entries.end(); ++it) {
			if ((*it)->ap_naming_info_.processName == apNamingInfo.processName &&
					(*it)->address_ != my_address) {
				apNamingInfo.processInstance = (*it)->ap_naming_info_.processInstance;
				return (*it)->address_;
			}
		}

		return 0;
	}

	return 0;
}

void NamespaceManager::addDFTEntries(const std::list<rina::DirectoryForwardingTableEntry>& entries,
			    	     bool notify_neighs,
			    	     std::list<int>& neighs_to_exclude)
{
	rina::ScopedLock g(lock);
	rina::DirectoryForwardingTableEntry * entry;

	std::list<rina::DirectoryForwardingTableEntry>::const_iterator it;
	for (it = entries.begin(); it != entries.end(); ++it) {
		if (dft_.find(it->getKey()) != 0)
			continue;

		entry = new rina::DirectoryForwardingTableEntry();
		entry->address_ = it->address_;
		entry->ap_naming_info_ = it->ap_naming_info_;
		entry->seqnum_ = it->seqnum_;

		try {
			std::stringstream ss;
			ss << DFTEntryRIBObj::object_name_prefix
			   << entry->getKey();

			rina::rib::RIBObj * nrobj = new DFTEntryRIBObj(ipcp, entry);
			rib_daemon_->addObjRIB(ss.str(), &nrobj);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems creating RIB object: %s",
					e.what());
		}

		dft_.put(entry->getKey(), entry);
		LOG_IPCP_DBG("Added entry to DFT: %s",
			     entry->toString().c_str());
	}

	notify_neighbors_add(entries, neighs_to_exclude);
}

void NamespaceManager::notify_neighbors_add(const std::list<rina::DirectoryForwardingTableEntry>& entries,
		          	  	    std::list<int>& neighs_to_exclude)
{
	std::vector<int> session_ids;
	rina::cdap::getProvider()->get_session_manager()->getAllCDAPSessionIds(session_ids);
	encoders::DFTEListEncoder encoder;
	rina::cdap_rib::obj_info_t obj;
	obj.class_ = DFTRIBObj::class_name;
	obj.name_ = DFTRIBObj::object_name;
	encoder.encode(entries, obj.value_);
	rina::cdap_rib::flags_t flags;
	rina::cdap_rib::filt_info_t filt;
	rina::cdap_rib::con_handle_t con;
	for (int i = 0; i < session_ids.size(); i++) {
		if (contains_entry(session_ids[i],
				neighs_to_exclude))
			continue;

		try {
			con.port_id = session_ids[i];
			ipcp->rib_daemon_->getProxy()->remote_create(con,
								     obj,
								     flags,
								     filt,
								     NULL);
		} catch (rina::Exception &e) {
			LOG_WARN("Problems sending create CDAP message: %s",
					e.what());
		}
	}
}

rina::DirectoryForwardingTableEntry * NamespaceManager::getDFTEntry(const std::string& key)
{
	return dft_.find(key);
}

std::list<rina::DirectoryForwardingTableEntry> NamespaceManager::getDFTEntries()
{
	return dft_.getCopyofentries();
}

void NamespaceManager::removeDFTEntry(const std::string& key,
			 	      bool notify_neighs,
			 	      bool remove_from_rib,
			 	      std::list<int>& neighs_to_exclude)
{
	rina::ScopedLock g(lock);
	std::string obj_name;

	rina::DirectoryForwardingTableEntry * entry = dft_.erase(key);
	if (!entry) {
		LOG_IPCP_WARN("Could not find DFT for key: %s",
			      key.c_str());
		return;
	}

	std::stringstream ss;
	ss << DFTEntryRIBObj::object_name_prefix
	   << key;
	obj_name = ss.str();

	if (remove_from_rib) {
		try {
			rib_daemon_->removeObjRIB(obj_name);
		} catch (rina::Exception &e){
			LOG_IPCP_ERR("Error removing object from RIB %s",
					e.what());
		}
	}

	LOG_IPCP_DBG("Removed entry from DFT: %s",
		     entry->toString().c_str());

	std::vector<int> session_ids;
	rina::cdap::getProvider()->get_session_manager()->getAllCDAPSessionIds(session_ids);
	rina::cdap_rib::obj_info_t obj;
	obj.class_ = DFTEntryRIBObj::class_name;
	obj.name_ = obj_name;
	rina::cdap_rib::flags_t flags;
	rina::cdap_rib::filt_info_t filt;
	rina::cdap_rib::con_handle_t con;
	for (int i = 0; i < session_ids.size(); i++) {
		if (contains_entry(session_ids[i],
				   neighs_to_exclude))
			continue;

		try {
			con.port_id = session_ids[i];
			ipcp->rib_daemon_->getProxy()->remote_delete(con,
								     obj,
								     flags,
								     filt,
								     NULL);
		} catch (rina::Exception &e) {
			LOG_WARN("Problems sending delete CDAP message: %s",
					e.what());
		}
	}

	delete entry;
}

unsigned short NamespaceManager::getRegIPCProcessId(rina::ApplicationProcessNamingInformation& apNamingInfo)
{
	rina::ApplicationRegistrationInformation * regInfo;
	std::list<rina::ApplicationRegistrationInformation *> entries;
	std::list<rina::ApplicationRegistrationInformation *>:: iterator it;

	rina::ScopedLock g(lock);

	regInfo = registrations_.find(apNamingInfo.getEncodedString());
	if (regInfo) {
		return regInfo->ipcProcessId;
	}

	//Could not find a match for a specific DAP, try to look for DAF name
	entries = registrations_.getEntries();
	for (it = entries.begin(); it != entries.end(); ++it) {
		if ((*it)->dafName.processName == apNamingInfo.processName) {
			//Use the specific DAP name instead of the DAF name in the flow destination naming info
			apNamingInfo.processName = (*it)->appName.processName;
			apNamingInfo.processInstance = (*it)->appName.processInstance;

			return (*it)->ipcProcessId;
		}
	}

	LOG_IPCP_DBG("Could not find a registered application with code : %s",
		     apNamingInfo.getEncodedString().c_str());
	return 0;
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
	rina::ApplicationRegistrationInformation * registration = 0;
	std::list<rina::DirectoryForwardingTableEntry> entriesToCreate;
	rina::ApplicationProcessNamingInformation appToRegister;
	rina::DirectoryForwardingTableEntry entry;
	std::list<int> exc_neighs;
	int result = 0;

	appToRegister = event.applicationRegistrationInformation.appName;
	if (registrations_.find(appToRegister.getEncodedString())) {
		LOG_IPCP_ERR("Application % is already registered in this IPC Process",
				appToRegister.getEncodedString().c_str());

		replyToIPCManagerRegister(event, -1);
		return;
	}

	registration = new rina::ApplicationRegistrationInformation(
			event.applicationRegistrationInformation.applicationRegistrationType);
	registration->appName = event.applicationRegistrationInformation.appName;
	registration->difName = event.applicationRegistrationInformation.difName;
	registration->dafName = event.applicationRegistrationInformation.dafName;
	registration->ipcProcessId = event.applicationRegistrationInformation.ipcProcessId;
	registrations_.put(appToRegister.getEncodedString(), registration);
	LOG_IPCP_INFO("Successfully registered application %s with IPC Process id %us",
		       appToRegister.getEncodedString().c_str(),
		       ipcp->get_id());
	result = replyToIPCManagerRegister(event, 0);
	if (result == -1) {
		registrations_.erase(appToRegister.getEncodedString());
		delete registration;
		return;
	}

	entry.address_ = ipcp->get_address();
	entry.ap_naming_info_ = appToRegister;
	entriesToCreate.push_back(entry);

	appToRegister = event.applicationRegistrationInformation.dafName;
	if (appToRegister.processName != "") {
		//Register the DAF name to allow enrollment to a DAF just using its name
		//Use the particular DAP process name as the DAF process instance, to
		//be able to differentiate multiple DAF registrations
		appToRegister.processInstance = event.applicationRegistrationInformation.appName.processName;
		entry.ap_naming_info_ = appToRegister;
		entriesToCreate.push_back(entry);
	}

	addDFTEntries(entriesToCreate, true, exc_neighs);
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

bool NamespaceManager::contains_entry(int candidate,
				      const std::list<int>& elements)
{
	std::list<int>::const_iterator it;
	for (it = elements.begin(); it != elements.end(); ++it) {
		if (candidate == *it)
			return true;
	}

	return false;
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

std::list<rina::WhatevercastName> NamespaceManager::get_whatevercast_names()
{
	return what_names.getCopyofentries();
}

void NamespaceManager::add_whatevercast_name(rina::WhatevercastName * name)
{
	rina::ScopedLock g(lock);

	if (what_names.find(name->name_) != 0) {
		LOG_IPCP_WARN("Tried to add an already existing Whatevercast name: %s",
				name->name_.c_str());
		return;
	}

	try {
		std::stringstream ss;
		ss << WhateverCastNameRIBObj::object_name_prefix
		   << name->name_;

		rina::rib::RIBObj * nrobj = new WhateverCastNameRIBObj(name);
		rib_daemon_->addObjRIB(ss.str(), &nrobj);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems creating RIB object: %s",
				e.what());
	}

	what_names.put(name->name_, name);
}

void NamespaceManager::remove_whatevercast_name(const std::string& name_key)
{
	rina::WhatevercastName * name;

	rina::ScopedLock g(lock);

	name = what_names.erase(name_key);
	if (name == 0) {
		LOG_IPCP_WARN("Could not find Whatevercast Name for key: %s",
			      name_key.c_str());
		return;
	}

	try {
		std::stringstream ss;
		ss << WhateverCastNameRIBObj::object_name_prefix
	           << name->name_;
		rib_daemon_->removeObjRIB(ss.str());
	} catch (rina::Exception &e){
		LOG_IPCP_ERR("Error removing object from RIB %s",
			     e.what());
	}

	delete name;
}

void NamespaceManager::processApplicationUnregistrationRequestEvent(
		const rina::ApplicationUnregistrationRequestEvent& event)
{
	rina::ApplicationRegistrationInformation * unregisteredApp = 0;
	rina::ApplicationProcessNamingInformation dafToUnregister;
	std::list<int> exc_neighs;
	int result = 0;

	unregisteredApp = registrations_.erase(event.applicationName.getEncodedString());

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

	removeDFTEntry(unregisteredApp->appName.getEncodedString(),
		       true,
		       true,
		       exc_neighs);

	if (unregisteredApp->dafName.processName != "") {
		//Remove DFT entry corresponding to DAF name
		dafToUnregister.processName = unregisteredApp->dafName.processName;
		dafToUnregister.processInstance = unregisteredApp->appName.processName;
		removeDFTEntry(dafToUnregister.getEncodedString(),
			       true,
			       true,
			       exc_neighs);
	}

	delete unregisteredApp;
}

unsigned int NamespaceManager::getAdressByname(const rina::ApplicationProcessNamingInformation& name)
{
	std::list<rina::Neighbor> neighbors =
			ipcp->enrollment_task_->get_neighbors();
	std::list<rina::Neighbor>::const_iterator it;
	for (it = neighbors.begin(); it != neighbors.end(); ++it) {
		if (it->name_.processName.compare(name.processName) == 0) {
			return it->address_;
		}
	}

	throw rina::Exception("Unknown neighbor");
}


//Class checkDFTEntriesToRemoveTimerTask
CheckDFTEntriesToRemoveTimerTask::CheckDFTEntriesToRemoveTimerTask(DFTRIBObj * dft_,
				 	 	   	   	   unsigned int address_,
								   const std::string name__)
{
	dft = dft_;
	address = address_;
	name_ = name__;
}

void CheckDFTEntriesToRemoveTimerTask::run()
{
	dft->checkDFTEntriesToRemove(address, name_);
}

} //namespace rinad
