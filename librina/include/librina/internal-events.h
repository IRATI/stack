/*
 * Internal events of an application process
 *
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
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

#ifndef LIBRINA_INTERNAL_EVENTS_H
#define LIBRINA_INTERNAL_EVENTS_H

#ifdef __cplusplus

#include "librina/application.h"

namespace rina {

/// Interface, an internal event
class InternalEvent {
public:
	InternalEvent(const std::string& event_type) : type (event_type) {};
	virtual ~InternalEvent(){};

	/// Event types
	static const std::string APP_CONNECTIVITY_TO_NEIGHBOR_LOST;
	static const std::string APP_N_MINUS_1_FLOW_ALLOCATED;
	static const std::string APP_N_MINUS_1_FLOW_ALLOCATION_FAILED;
	static const std::string APP_N_MINUS_1_FLOW_DEALLOCATED;
	static const std::string APP_NEIGHBOR_DECLARED_DEAD;
	static const std::string APP_NEIGHBOR_ADDED;
	static const std::string ADDRESS_CHANGE;
	static const std::string NEIGHBOR_ADDRESS_CHANGE;
	static const std::string IPCP_INTERNAL_FLOW_ALLOCATED;
	static const std::string IPCP_INTERNAL_FLOW_ALLOCATION_FAILED;
	static const std::string IPCP_INTERNAL_FLOW_DEALLOCATED;

	std::string type;
};

/// Interface. It is subscribed to events of certain type
class InternalEventListener {
public:
	virtual ~InternalEventListener(){};

	/// Called when a certain event has happened
	virtual void eventHappened(InternalEvent * event) = 0;
};

/// Interface
/// Manages subscriptions to events
class InternalEventManager : public ApplicationEntity {
public:
	InternalEventManager() :
		ApplicationEntity(ApplicationEntity::INTERNAL_EVENT_MANAGER_AE_NAME) { };
	virtual ~InternalEventManager(){};

	/// Subscribe to a single event
	/// @param eventId The id of the event
	/// @param eventListener The event listener
	virtual void subscribeToEvent(const std::string& type, 
				      InternalEventListener * eventListener) = 0;

	/// Unubscribe from a single event
	/// @param eventId The id of the event
	/// @param eventListener The event listener
	virtual void unsubscribeFromEvent(const std::string& type, 
					  InternalEventListener * eventListener) = 0;

	/// Invoked when a certain event has happened.
	/// The Event Manager will inform about this to
	/// all the subscribers and delete the event
	/// afterwards. Ownership of event is passed to the
	/// Event manager.
	/// @param event
	virtual void deliverEvent(InternalEvent * event) = 0;
};

class SimpleInternalEventManager: public InternalEventManager {
public:
	SimpleInternalEventManager() { };
		void set_application_process(ApplicationProcess * ap);
        void subscribeToEvent(const std::string& type,
        		      InternalEventListener * eventListener);
        void unsubscribeFromEvent(const std::string& type,
        			  InternalEventListener * eventListener);
        void deliverEvent(InternalEvent * event);

private:
        std::map<std::string, std::list<InternalEventListener *> > event_listeners_;
        rina::Lockable events_lock_;
};

/// Event that signals that an N-1 flow allocation failed
class NMinusOneFlowAllocationFailedEvent: public InternalEvent {
public:
	NMinusOneFlowAllocationFailedEvent(unsigned int handle,
				           const FlowInformation& flow_information,
				           const std::string& result_reason);
	const std::string toString();

	/// The portId of the flow denied
	unsigned int handle_;

	/// The FlowService object describing the flow
	FlowInformation flow_information_;

	/// The reason why the allocation failed
	std::string result_reason_;
};

/// Event that signals the allocation of an N-1 flow
class NMinusOneFlowAllocatedEvent: public InternalEvent {
public:
	NMinusOneFlowAllocatedEvent(unsigned int handle,
				    const FlowInformation& flow_information);
	const std::string toString();

	/// The portId of the flow denied
	unsigned int handle_;

	/// The FlowService object describing the flow
	FlowInformation flow_information_;
};

/// Event that signals the deallocation of an N-1 flow
class NMinusOneFlowDeallocatedEvent: public InternalEvent {
public:
	NMinusOneFlowDeallocatedEvent(int port_id);
	const std::string toString();

	/// The portId of the flow deallocated
	int port_id_;

	/// True if the flow deallocated was used for layer management
	bool management_flow_;
};

/// The connectivity to a neighbor has been lost
class ConnectiviyToNeighborLostEvent: public InternalEvent {
public:
	ConnectiviyToNeighborLostEvent(const Neighbor& neighbor);
	const std::string toString();

	Neighbor neighbor_;
};

/// The IPC Process has enrolled with a new neighbor
class NeighborAddedEvent: public InternalEvent {
public:
	NeighborAddedEvent(const Neighbor& neighbor, bool enrollee,
			   bool prepare_for_handover,
			   const rina::ApplicationProcessNamingInformation& disc_neigh);
	const std::string toString();

	Neighbor neighbor_;

	/// True if this IPC Process requested the enrollment operation,
	/// false if it was its neighbor.
	bool enrollee_;

	bool prepare_handover;
	rina::ApplicationProcessNamingInformation disc_neigh_name;
};

/// A connectivity to a neighbor has been lost
class NeighborDeclaredDeadEvent: public InternalEvent {
public:
	NeighborDeclaredDeadEvent(const Neighbor& neighbor);
	const std::string toString();

	Neighbor neighbor_;
};

/// The address of the IPCP has changed
class AddressChangeEvent: public InternalEvent {
public:
	AddressChangeEvent(unsigned int new_address,
			   unsigned int old_address,
			   unsigned int use_new_timeout,
			   unsigned int deprecate_old_timeout);
	const std::string toString();

	unsigned int new_address;
	unsigned int old_address;
	unsigned int use_new_timeout;
	unsigned int deprecate_old_timeout;
};

/// The address of a neighbor IPCP has changed
class NeighborAddressChangeEvent: public InternalEvent {
public:
	NeighborAddressChangeEvent(const std::string& neigh_name_,
				   unsigned int new_address,
			   	   unsigned int old_address);
	const std::string toString();

	std::string neigh_name;
	unsigned int new_address;
	unsigned int old_address;
};

class IPCPInternalFlowAllocatedEvent: public InternalEvent {
public:
	IPCPInternalFlowAllocatedEvent(unsigned int port,
				       int fd,
				       const FlowInformation& flow_information);
	const std::string toString();

	/// The portId of the flow
	unsigned int port_id;

	/// The file descriptor to read the flow
	int fd;

	/// The FlowService object describing the flow
	FlowInformation flow_info;
};

class IPCPInternalFlowAllocationFailedEvent: public InternalEvent {
public:
	IPCPInternalFlowAllocationFailedEvent(unsigned int errc,
				         const FlowInformation& flow_information,
				         const std::string& result_reason);
	const std::string toString();

	/// The portId of the flow denied
	unsigned int error_code;

	/// The FlowService object describing the flow
	FlowInformation flow_info;

	/// The reason why the allocation failed
	std::string reason;
};

class IPCPInternalFlowDeallocatedEvent: public InternalEvent {
public:
	IPCPInternalFlowDeallocatedEvent(int port);
	const std::string toString();

	/// The portId of the flow deallocated
	int port_id;
};

}

#endif

#endif
