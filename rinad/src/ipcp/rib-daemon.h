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

#include "common/concurrency.h"
#include "ipcp/components.h"

namespace rinad {

/// Reads sdus from the Kernel IPC Process, and
/// passes them to the RIB Daemon
class ManagementSDUReaderData {
public:
	ManagementSDUReaderData(IPCPRIBDaemon * rib_daemon, unsigned int max_sdu_size);
	IPCPRIBDaemon * rib_daemon_;
	unsigned int max_sdu_size_;
};

/// The RIB Daemon will start a thread that continuously tries to retrieve management
/// SDUs directed to this IPC Process
void * doManagementSDUReaderWork(void* data);

class BaseRIBDaemon: public IPCPRIBDaemon {
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
class IPCPRIBDaemonImpl : public BaseRIBDaemon, public EventListener {
public:
        IPCPRIBDaemonImpl();
	void set_ipc_process(IPCProcess * ipc_process);
	void set_dif_configuration(const rina::DIFConfiguration& dif_configuration);
	void eventHappened(Event * event);
	void processQueryRIBRequestEvent(const rina::QueryRIBRequestEvent& event);
	void sendMessageSpecific(bool useAddress, const rina::CDAPMessage & cdapMessage, int sessionId,
			unsigned int address, rina::ICDAPResponseMessageHandler * cdapMessageHandler);
	void cdapMessageDelivered(char* message, int length, int portId);

private:
	IPCProcess * ipc_process_;
	INMinusOneFlowManager * n_minus_one_flow_manager_;
	rina::Thread * management_sdu_reader_;

    /// Lock to control that when sending a message requiring a reply the
    /// CDAP Session manager has been updated before receiving the response message
    rina::Lockable atomic_send_lock_;

	void subscribeToEvents();

	/// Invoked by the Resource allocator when it detects that a certain flow has been deallocated, and therefore a
	/// any CDAP sessions over it should be terminated.
	void nMinusOneFlowDeallocated(int portId);
	void nMinusOneFlowAllocated(NMinusOneFlowAllocatedEvent * event);
};

}

#endif

#endif
