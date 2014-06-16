/*
 * Internal events between IPC Process components
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

#ifndef IPCP_EVENTS_HH
#define IPCP_EVENTS_HH

#ifdef __cplusplus

#include <list>

namespace rinad {

/// The types of events internal to an IPC Process
enum IPCProcessEventType {
	IPCP_EVENT_CONNECTIVITY_TO_NEIGHBOR_LOST,
	IPCP_EVENT_EFCP_CONNECTION_CREATED,
	IPCP_EVENT_EFCP_CONNECTION_DELETED,
	IPCP_EVENT_MANAGEMENT_FLOW_ALLOCATED,
	IPCP_EVENT_MANAGEMENT_FLOW_DEALLOCATED,
	IPCP_EVENT_N_MINUS_1_FLOW_ALLOCATED,
	IPCP_EVENT_N_MINUS_1_FLOW_ALLOCATION_FAILED,
	IPCP_EVENT_N_MINUS_1_FLOW_DEALLOCATED,
	IPCP_EVENT_NEIGHBOR_DECLARED_DEAD,
	IPCP_EVENT_NEIGHBOR_ADDED
};

/// Interface
/// An event
class Event
{
public:
	virtual ~Event(){};
	/// The id of the event
	virtual IPCProcessEventType get_id() const = 0;
};

/// Base class common to all events
class BaseEvent: public Event{
public:
	BaseEvent();
	BaseEvent(const IPCProcessEventType& id);
	IPCProcessEventType get_id() const;

protected:
	/// The identity of the event
	IPCProcessEventType id_;
};

/// Interface
/// It is subscribed to events of certain type
class EventListener {
	public:
		virtual ~EventListener(){};

		/// Called when a certain event has happened
		virtual void eventHappened(const Event& event) = 0;
};

/// Interface
/// Manages subscriptions to events
class EventManager
{
	public:
		virtual ~EventManager(){};

		/// Subscribe to a single event
		/// @param eventId The id of the event
		/// @param eventListener The event listener
		virtual void subscribeToEvent(const IPCProcessEventType& eventId, const EventListener& eventListener) = 0;

		/// Subscribes to a list of events
		/// @param eventIds the list of event ids
		/// @param eventListener The event listener
		virtual void subscribeToEvents(const std::list<IPCProcessEventType>& eventIds, const EventListener& eventListener) = 0;

		/// Unubscribe from a single event
		/// @param eventId The id of the event
		/// @param eventListener The event listener
		virtual void unsubscribeFromEvent(const IPCProcessEventType& eventId, const EventListener& eventListener) = 0;

		/// Unsubscribe from a list of events
		/// @param eventIds the list of event ids
	    /// @param eventListener The event listener
		virtual void unsubscribeFromEvents(const std::list<IPCProcessEventType>& eventIds, const EventListener& eventListener) = 0;

		/// Invoked when a certain event has happened
		/// @param event
		virtual void deliverEvent(const Event& event) = 0;
};

}

#endif

#endif
