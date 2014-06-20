/*
 * RIB Daemon
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

#ifndef IPCP_RIB_DAEMON_HH
#define IPCP_RIB_DAEMON_HH

#ifdef __cplusplus

#include <librina/concurrency.h>

#include "ipcp/components.h"

namespace rinad {

class RIB : public rina::Lockable {
public:
	RIB();
	~RIB() throw();

	/// Given an objectname of the form "substring\0substring\0...substring" locate
	/// the RIBObject that corresponds to it
	/// @param objectName
	/// @return
	BaseRIBObject* getRIBObject(const std::string& objectClass,
			const std::string& objectName) throw (Exception);
	void addRIBObject(BaseRIBObject* ribObject) throw (Exception);
	BaseRIBObject * removeRIBObject(const std::string& objectName) throw (Exception);
	std::list<BaseRIBObject*> getRIBObjects();

private:
	std::map<std::string, BaseRIBObject*> rib_;
};

/// Reads sdus from the Kernel IPC Process, and
/// passes them to the RIB Daemon
class ManagementSDUReaderData {
public:
	ManagementSDUReaderData(IRIBDaemon * rib_daemon, unsigned int max_sdu_size);
	IRIBDaemon * get_rib_daemon();
	unsigned int get_max_sdu_size();

private:
	IRIBDaemon * rib_daemon_;
	unsigned int max_sdu_size_;
};

/// The RIB Daemon will start a thread that continuously tries to retrieve management
/// SDUs directed to this IPC Process
void * doManagementSDUReaderWork(void* data);

/// Base functionality of the RIB Daemon implementation, deals with event management
class BaseRIBDaemon : public IRIBDaemon {
public:
	BaseRIBDaemon();
	void subscribeToEvent(const IPCProcessEventType& eventId, EventListener * eventListener);
	void unsubscribeFromEvent(const IPCProcessEventType& eventId, EventListener * eventListener);
	void deliverEvent(Event * event);

private:
	std::map<IPCProcessEventType, std::list<EventListener *> > event_listeners_;
	rina::Lockable events_lock_;
};

///Full implementation of the RIB Daemon
class RIBDaemon : public BaseRIBDaemon, public EventListener {
public:
	RIBDaemon();
	void set_ipc_process(IPCProcess * ipc_process);
	void eventHappened(Event * event);
	void addRIBObject(BaseRIBObject * ribObject) throw (Exception);
	void removeRIBObject(BaseRIBObject * ribObject) throw (Exception);
	void removeRIBObject(const std::string& objectName) throw (Exception);
	std::list<BaseRIBObject *> getRIBObjects();
	BaseRIBObject * readObject(const std::string& objectClass,
				const std::string& objectName) throw (Exception);
	void writeObject(const std::string& objectClass, const std::string& objectName,
				const void* objectValue) throw (Exception);
	void startObject(const std::string& objectClass, const std::string& objectName,
			const void* objectValue) throw (Exception);
	void stopObject(const std::string& objectClass, const std::string& objectName,
				const void* objectValue) throw (Exception);
	void processQueryRIBRequestEvent(const rina::QueryRIBRequestEvent& event);

private:
	RIB rib_;
	IPCProcess * ipc_process_;
	rina::CDAPSessionManagerInterface * cdap_session_manager_;
	IEncoder * encoder_;
	INMinusOneFlowManager * n_minus_one_flow_manager_;
	rina::Thread * management_sdu_reader_;

	///CDAP Message handlers that have sent a CDAP message and are waiting for a reply
	std::map<int, ICDAPResponseMessageHandler *> handlers_waiting_for_reply_;

	/// Lock to protect concurrent access to the handlers table
	rina::Lockable handlers_lock_;

	///Lock to control that when sending a message requiring a reply the
	/// CDAP Session manager has been updated before receiving the response message
	rina::Lockable atomic_send_lock_;

	void subscribeToEvents();

	///Invoked by the Resource allocator when it detects that a certain flow has been deallocated, and therefore a
	/// any CDAP sessions over it should be terminated.
	void nMinusOneFlowDeallocated(int portId);
	void nMinusOneFlowAllocated(NMinusOneFlowAllocatedEvent * event);
};

}

#endif

#endif
