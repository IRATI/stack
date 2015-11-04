/*
 * Namespace Manager
 *
 *    Bernat Gaston <bernat.gaston@i2cat.net>
 *    Eduard Grasa <eduard.grasa@i2cat.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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

	const static std::string class_name;
	const static std::string object_name;

private:
	INamespaceManager * namespace_manager_;
};

class NamespaceManager: public INamespaceManager {
public:
	NamespaceManager();
	void set_application_process(rina::ApplicationProcess * ap);
	void set_dif_configuration(const rina::DIFConfiguration& dif_configuration);
	unsigned int getDFTNextHop(const rina::ApplicationProcessNamingInformation& apNamingInfo);
	void addDFTEntries(const std::list<rina::DirectoryForwardingTableEntry>& entries,
			   bool notify_neighs,
			   std::list<int>& neighs_to_exclude);
	rina::DirectoryForwardingTableEntry * getDFTEntry(const std::string& key);
	std::list<rina::DirectoryForwardingTableEntry> getDFTEntries();
	void removeDFTEntry(const std::string& key,
			    bool notify_neighs,
			    bool remove_from_rib,
			    std::list<int>& neighs_to_exclude);
	unsigned short getRegIPCProcessId(const rina::ApplicationProcessNamingInformation& apNamingInfo);
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

private:
	rina::Lockable lock;

	/// The directory forwarding table
	rina::ThreadSafeMapOfPointers<std::string, rina::DirectoryForwardingTableEntry> dft_;

	/// Applications registered in this IPC Process
	rina::ThreadSafeMapOfPointers<std::string, rina::ApplicationRegistrationInformation> registrations_;

	/// Whatevercast names
	rina::ThreadSafeMapOfPointers<std::string, rina::WhatevercastName> what_names;

	IPCPRIBDaemon * rib_daemon_;

	void populateRIB();
	int replyToIPCManagerRegister(const rina::ApplicationRegistrationRequestEvent& event,
			int result);
	int replyToIPCManagerUnregister(const rina::ApplicationUnregistrationRequestEvent& event,
			int result);

	bool contains_entry(int candidate, const std::list<int>& elements);
};

} //namespace rinad

#endif //IPCP_NAMESPACE_MANAGER_HH
