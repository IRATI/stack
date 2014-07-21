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

#ifdef __cplusplus

#include "common/concurrency.h"
#include "ipcp/components.h"

namespace rinad {

/// Defines a whatevercast name (or a name of a set of names).
/// In traditional architectures, sets that returned all members were called multicast; while
/// sets that returned one member were called anycast.  It is not clear what sets that returned
/// something in between were called.  With the more general definition here, these
/// distinctions are unnecessary.
class WhatevercastName {
public:
	bool operator==(const WhatevercastName &other);
	std::string toString();

	/// The name
	std::string name_;

	/// The members of the set
	std::list<std::string> set_members_;

	/// The rule to select one or more members from the set
	std::string rule_;
};

class WhateverCastNameSetRIBObject: public BaseRIBObject {
public:
	WhateverCastNameSetRIBObject(IPCProcess * ipc_process);
	~WhateverCastNameSetRIBObject();
	const void* get_value() const;
	void remoteCreateObject(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor);
	void createObject(const std::string& objectClass,
			const std::string& objectName,
			const void* objectValue);

private:
	void createName(WhatevercastName * name);
	rina::Lockable * lock_;
};

class DirectoryForwardingTableEntryRIBObject: public SimpleSetMemberRIBObject {
public:
	DirectoryForwardingTableEntryRIBObject(IPCProcess * ipc_process, const std::string& object_name,
			rina::DirectoryForwardingTableEntry * entry);
	void remoteCreateObject(const rina::CDAPMessage * cdapMessage,
				rina::CDAPSessionDescriptor * cdapSessionDescriptor);
	void remoteDeleteObject(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor);
	void createObject(const std::string& objectClass, const std::string& objectName,
			const void* objectValue);
	void deleteObject(const void* objectValue);

private:
	INamespaceManager * namespace_manager_;
	rina::ApplicationProcessNamingInformation ap_name_entry_;
};

class DirectoryForwardingTableEntrySetRIBObject: public BaseRIBObject, public EventListener {
public:
	DirectoryForwardingTableEntrySetRIBObject(IPCProcess * ipc_process);

	/// Called when the connectivity to a neighbor has been lost. All the
	/// applications registered from that neighbor have to be removed from the directory
	void eventHappened(Event * event);

	/// A routing update with new and/or updated entries has been received -or
	/// during enrollment-. See what parts of the update we didn't now, and tell the
	/// RIB Daemon about them (will create/update the objects and notify my neighbors
	/// except for the one that has sent me the update)
	void remoteCreateObject(const rina::CDAPMessage * cdapMessage,
					rina::CDAPSessionDescriptor * cdapSessionDescriptor);

	/// One or more local applications have registered to this DIF or a routing update
	/// has been received
	void createObject(const std::string& objectClass, const std::string& objectName,
			const void* objectValue);

	/// A routing update has been received
	void remoteDeleteObject(const rina::CDAPMessage * cdapMessage,
				rina::CDAPSessionDescriptor * cdapSessionDescriptor);

	/// One or more local applications have unregistered from this DIF or a routing
	/// update has been received
	void deleteObject(const void* objectValue);
	const void* get_value() const;

private:
	INamespaceManager * namespace_manager_;
	void deleteObjects(const std::list<std::string>& namesToDelete);
	void populateEntriesToCreateList(rina::DirectoryForwardingTableEntry* entry,
			std::list<rina::DirectoryForwardingTableEntry *> * list);
	void populateEntriesToDeleteList(rina::DirectoryForwardingTableEntry* entry,
			std::list<rina::DirectoryForwardingTableEntry *> * list);
	BaseRIBObject * getObject(const std::string& candidateKey);
};

class NamespaceManager: public INamespaceManager {
public:
	NamespaceManager();
	void set_ipc_process(IPCProcess * ipc_process);
	void set_dif_configuration(const rina::DIFConfiguration& dif_configuration);
	unsigned int getDFTNextHop(const rina::ApplicationProcessNamingInformation& apNamingInfo);
	void addDFTEntry(rina::DirectoryForwardingTableEntry * entry);
	rina::DirectoryForwardingTableEntry * getDFTEntry(
				const rina::ApplicationProcessNamingInformation& apNamingInfo);
	void removeDFTEntry(const rina::ApplicationProcessNamingInformation& apNamingInfo);
	unsigned short getRegIPCProcessId(const rina::ApplicationProcessNamingInformation& apNamingInfo);
	void processApplicationRegistrationRequestEvent(
				const rina::ApplicationRegistrationRequestEvent& event);
	void processApplicationUnregistrationRequestEvent(
				const rina::ApplicationUnregistrationRequestEvent& event);
	bool isValidAddress(unsigned int address, const std::string& ipcp_name,
			const std::string& ipcp_instance);
	unsigned int getValidAddress(const std::string& ipcp_name,
					const std::string& ipcp_instance);
	unsigned int getAdressByname(const rina::ApplicationProcessNamingInformation& name);

private:
	/// The directory forwarding table
	ThreadSafeMapOfPointers<std::string, rina::DirectoryForwardingTableEntry> dft_;

	/// Applications registered in this IPC Process
	ThreadSafeMapOfPointers<std::string, rina::ApplicationRegistrationInformation> registrations_;

	IPCProcess * ipc_process_;
	IRIBDaemon * rib_daemon_;

	void populateRIB();
	int replyToIPCManagerRegister(const rina::ApplicationRegistrationRequestEvent& event,
			int result);
	int replyToIPCManagerUnregister(const rina::ApplicationUnregistrationRequestEvent& event,
			int result);
	unsigned int getIPCProcessAddress(const std::string& process_name,
			const std::string& process_instance,
			const rina::AddressingConfiguration& address_conf);
	unsigned int getAddressPrefix(const std::string& process_name,
				const rina::AddressingConfiguration& address_conf);
	bool isAddressInUse(unsigned int address, const std::string& ipcp_name);
};

}

#endif

#endif
