/*
 * Namespace Manager
 *
 *    Bernat Gaston <bernat.gaston@i2cat.net>
 *    Eduard Grasa <eduard.grasa@i2cat.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#ifndef IPCP_NAMESPACE_MANAGER_HH
#define IPCP_NAMESPACE_MANAGER_HH

#include <librina/ipc-process.h>
#include <librina/internal-events.h>

#include "common/concurrency.h"
#include "ipcp/components.h"

namespace rinad {

class WhateverCastNameRIBObj: public rina::rib::RIBObj {
public:
	WhateverCastNameRIBObj(rina::WhatevercastName* name);
	const std::string get_displayable_value() const;
	const std::string& get_class() const {
		return class_name;
	};

	void read(const rina::cdap_rib::con_handle_t &con,
		  const std::string& fqn,
		  const std::string& class_,
		  const rina::cdap_rib::filt_info_t &filt,
		  const int invoke_id,
		  rina::cdap_rib::obj_info_t &obj_reply,
		  rina::cdap_rib::res_info_t& res);

	const static std::string class_name;
	const static std::string object_name_prefix;

private:
	rina::WhatevercastName * name;
};

class WhateverCastNamesRIBObj: public IPCPRIBObj {
public:
	WhateverCastNamesRIBObj(IPCProcess * ipc_process);
	~WhateverCastNamesRIBObj();

	const std::string& get_class() const {
		return class_name;
	};

	//Create
	void create(const rina::cdap_rib::con_handle_t &con,
		    const std::string& fqn,
		    const std::string& class_,
		    const rina::cdap_rib::filt_info_t &filt,
		    const int invoke_id,
		    const rina::ser_obj_t &obj_req,
		    rina::ser_obj_t &obj_reply,
		    rina::cdap_rib::res_info_t& res);

	const static std::string class_name;
	const static std::string object_name;

private:
	rina::Lockable lock_;
};

class DFTEntryRIBObj: public IPCPRIBObj {
public:
	DFTEntryRIBObj(IPCProcess * ipcp,
		       rina::DirectoryForwardingTableEntry * entry);
	const std::string get_displayable_value() const;
	const std::string& get_class() const {
		return class_name;
	};

	bool delete_(const rina::cdap_rib::con_handle_t &con,
		     const std::string& fqn,
		     const std::string& class_,
		     const rina::cdap_rib::filt_info_t &filt,
		     const int invoke_id,
		     rina::cdap_rib::res_info_t& res);

	void read(const rina::cdap_rib::con_handle_t &con,
		  const std::string& fqn,
		  const std::string& class_,
		  const rina::cdap_rib::filt_info_t &filt,
		  const int invoke_id,
		  rina::cdap_rib::obj_info_t &obj_reply,
		  rina::cdap_rib::res_info_t& res);

	const static std::string class_name;
	const static std::string object_name_prefix;

private:
	rina::DirectoryForwardingTableEntry * entry;
	INamespaceManager * nsm;
};

class DFTRIBObj: public IPCPRIBObj, public rina::InternalEventListener {
public:
	DFTRIBObj(IPCProcess * ipc_process);

	/// Called when the connectivity to a neighbor has been lost. All the
	/// applications registered from that neighbor have to be removed from the directory
	void eventHappened(rina::InternalEvent * event);

	/// A routing update with new and/or updated entries has been received -or
	/// during enrollment-. See what parts of the update we didn't now, and tell the
	/// RIB Daemon about them (will create/update the objects and notify my neighbors
	/// except for the one that has sent me the update)
	void create(const rina::cdap_rib::con_handle_t &con,
		    const std::string& fqn,
		    const std::string& class_,
		    const rina::cdap_rib::filt_info_t &filt,
		    const int invoke_id,
		    const rina::ser_obj_t &obj_req,
		    rina::ser_obj_t &obj_reply,
		    rina::cdap_rib::res_info_t& res);

	const std::string& get_class() const {
		return class_name;
	};

	void checkDFTEntriesToRemove(unsigned int address,
				     const std::string& name);

	const static std::string class_name;
	const static std::string object_name;

private:
	rina::Lockable lock;
	rina::Timer timer;
	INamespaceManager * namespace_manager_;
};

class AddressChangeTimerTask: public rina::TimerTask {
public:
	AddressChangeTimerTask(INamespaceManager * nsm,
			       unsigned int naddr,
			       unsigned int oaddr);
	~AddressChangeTimerTask() throw() {};
	void run();
	std::string name() const {
		return "address-change";
	}

	INamespaceManager * namespace_manager;
	unsigned int new_address;
	unsigned int old_address;
};

class NamespaceManager: public INamespaceManager, public rina::InternalEventListener {
public:
	NamespaceManager();
	~NamespaceManager();
	void eventHappened(rina::InternalEvent * event);
	void set_application_process(rina::ApplicationProcess * ap);
	void set_dif_configuration(const rina::DIFInformation& dif_information);
	unsigned int getDFTNextHop(rina::ApplicationProcessNamingInformation& apNamingInfo);
	void addDFTEntries(const std::list<rina::DirectoryForwardingTableEntry>& entries,
			   bool notify_neighs,
			   std::list<int>& neighs_to_exclude);
	rina::DirectoryForwardingTableEntry * getDFTEntry(const std::string& key);
	std::list<rina::DirectoryForwardingTableEntry> getDFTEntries();
	void removeDFTEntry(const std::string& key,
			    bool notify_neighs,
			    bool remove_from_rib,
			    std::list<int>& neighs_to_exclude);
	unsigned short getRegIPCProcessId(rina::ApplicationProcessNamingInformation& apNamingInfo);
	void processApplicationRegistrationRequestEvent(
				const rina::ApplicationRegistrationRequestEvent& event);
	void processApplicationUnregistrationRequestEvent(
				const rina::ApplicationUnregistrationRequestEvent& event);
	unsigned int getAdressByname(const rina::ApplicationProcessNamingInformation& name);
	rina::ApplicationRegistrationInformation
		get_reg_app_info(const rina::ApplicationProcessNamingInformation name);

	std::list<rina::WhatevercastName> get_whatevercast_names();
	void add_whatevercast_name(rina::WhatevercastName * name);
	void remove_whatevercast_name(const std::string& name_key);
	void addressChangeUpdateDFT(unsigned int new_address,
				    unsigned int old_address);
	void notify_neighbors_add(const std::list<rina::DirectoryForwardingTableEntry>& entries,
			          std::list<int>& neighs_to_exclude);

private:
	rina::Lockable lock;

	/// The directory forwarding table
	rina::ThreadSafeMapOfPointers<std::string, rina::DirectoryForwardingTableEntry> dft_;

	/// Applications registered in this IPC Process
	rina::ThreadSafeMapOfPointers<std::string, rina::ApplicationRegistrationInformation> registrations_;

	/// Whatevercast names
	rina::ThreadSafeMapOfPointers<std::string, rina::WhatevercastName> what_names;

	IPCPRIBDaemon * rib_daemon_;
	rina::InternalEventManager * event_manager_;
	rina::Timer timer;

	void populateRIB();
	void subscribeToEvents();
	void addressChange(rina::AddressChangeEvent * event);
	int replyToIPCManagerRegister(const rina::ApplicationRegistrationRequestEvent& event,
			int result);
	int replyToIPCManagerUnregister(const rina::ApplicationUnregistrationRequestEvent& event,
			int result);

	bool contains_entry(int candidate, const std::list<int>& elements);
};

class CheckDFTEntriesToRemoveTimerTask : public rina::TimerTask {
public:
	CheckDFTEntriesToRemoveTimerTask(DFTRIBObj * dft_,
					 unsigned int address_,
					 const std::string name);
	~CheckDFTEntriesToRemoveTimerTask() throw(){};
	void run();
	std::string name() const {
		return "check-dft-entries-to-remove";
	}

private:
	unsigned int address;
	DFTRIBObj* dft;
	std::string name_;
};

} //namespace rinad

#endif //IPCP_NAMESPACE_MANAGER_HH
