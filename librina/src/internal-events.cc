//
// Internal events
//
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

#define RINA_PREFIX "internal-events"

#include "librina/logs.h"
#include "librina/internal-events.h"

namespace rina {

const std::string InternalEvent::APP_CONNECTIVITY_TO_NEIGHBOR_LOST = "CONNECTIVITY_TO_NEIGHBOR_LOST";
const std::string InternalEvent::APP_N_MINUS_1_FLOW_ALLOCATED = "N_MINUS_1_FLOW_ALLOCATED";
const std::string InternalEvent::APP_N_MINUS_1_FLOW_ALLOCATION_FAILED = "N_MINUS_1_FLOW_ALLOCATION_FAILED";
const std::string InternalEvent::APP_N_MINUS_1_FLOW_DEALLOCATED = "N_MINUS_1_FLOW_DEALLOCATED";
const std::string InternalEvent::APP_NEIGHBOR_DECLARED_DEAD = "NEIGHBOR_DECLARED_DEAD";
const std::string InternalEvent::APP_NEIGHBOR_ADDED = "NEIGHBOR_ADDED";
const std::string InternalEvent::ADDRESS_CHANGE = "ADDRESS_CHANGE";
const std::string InternalEvent::NEIGHBOR_ADDRESS_CHANGE = "NEIGHBOR_ADDRESS_CHANGE";
const std::string InternalEvent::IPCP_INTERNAL_FLOW_ALLOCATED = "IPCP_INTERNAL_FLOW_ALLOCATED";
const std::string InternalEvent::IPCP_INTERNAL_FLOW_ALLOCATION_FAILED = "IPCP_INTERNAL_FLOW_ALLOCATION_FAILED";
const std::string InternalEvent::IPCP_INTERNAL_FLOW_DEALLOCATED = "IPCP_INTERNAL_FLOW_DEALLOCATED";

// Class SimpleInternalEventManager
void SimpleInternalEventManager::set_application_process(ApplicationProcess * ap)
{
	app = ap;
}

void SimpleInternalEventManager::subscribeToEvent(const std::string& type,
                                     	 	  InternalEventListener * eventListener)
{
        if (!eventListener)
                return;

        events_lock_.lock();

        std::map<std::string, std::list<InternalEventListener*> >::iterator it =
        		event_listeners_.find(type);
        if (it == event_listeners_.end()) {
                std::list<InternalEventListener *> listenersList;
                listenersList.push_back(eventListener);
                event_listeners_[type] = listenersList;
        } else {
                std::list<InternalEventListener *>::iterator listIterator;
                for (listIterator=it->second.begin(); listIterator != it->second.end(); ++listIterator) {
                        if (*listIterator == eventListener) {
                                events_lock_.unlock();
                                return;
                        }
                }

                it->second.push_back(eventListener);
        }

        LOG_INFO("EventListener subscribed to event %s", type.c_str());
        events_lock_.unlock();
}

void SimpleInternalEventManager::unsubscribeFromEvent(const std::string& type,
                                         	      InternalEventListener * eventListener)
{
        if (!eventListener)
                return;

        events_lock_.lock();
        std::map<std::string, std::list<InternalEventListener*> >::iterator it =
        		event_listeners_.find(type);
        if (it == event_listeners_.end()) {
                events_lock_.unlock();
                return;
        }

        it->second.remove(eventListener);
        if (it->second.size() == 0) {
                event_listeners_.erase(it);
        }

        LOG_INFO("EventListener unsubscribed from event %s", type.c_str());
        events_lock_.unlock();
}

void SimpleInternalEventManager::deliverEvent(InternalEvent * event)
{
        if (!event)
                return;

        LOG_INFO("Event %s has just happened. Notifying event listeners.",
                       event->type.c_str());

        events_lock_.lock();
        std::map<std::string, std::list<InternalEventListener*> >::iterator it =
        		event_listeners_.find(event->type);
        if (it == event_listeners_.end()) {
                events_lock_.unlock();
                delete event;
                return;
        }

        events_lock_.unlock();
        std::list<InternalEventListener *>::iterator listIterator;
        for (listIterator=it->second.begin(); listIterator != it->second.end(); ++listIterator) {
                (*listIterator)->eventHappened(event);
        }

        if (event) {
                delete event;
        }
}

//CLASS NMinusOneFlowAllocationFailedEvent
NMinusOneFlowAllocationFailedEvent::NMinusOneFlowAllocationFailedEvent(unsigned int handle,
			const rina::FlowInformation& flow_information,
			const std::string& result_reason):
				InternalEvent(InternalEvent::APP_N_MINUS_1_FLOW_ALLOCATION_FAILED)
{
	handle_ = handle;
	flow_information_ = flow_information;
	result_reason_ = result_reason;
}

const std::string NMinusOneFlowAllocationFailedEvent::toString()
{
	std::stringstream ss;
	ss<<"Event id: "<<type<<"; Handle: "<<handle_;
	ss<<"; Result reason: "<<result_reason_<<std::endl;
	ss<<"Flow description: "<<flow_information_.toString();
	return ss.str();
}

//CLASS NMinusOneFlowAllocatedEvent
NMinusOneFlowAllocatedEvent::NMinusOneFlowAllocatedEvent(unsigned int handle,
			const rina::FlowInformation& flow_information):
				InternalEvent(InternalEvent::APP_N_MINUS_1_FLOW_ALLOCATED)
{
	handle_ = handle;
	flow_information_ = flow_information;
}

const std::string NMinusOneFlowAllocatedEvent::toString()
{
	std::stringstream ss;
	ss<<"Event id: "<<type<<"; Handle: "<<handle_<<std::endl;
	ss<<"Flow description: "<<flow_information_.toString();
	return ss.str();
}

//CLASS NMinusOneFlowDeallocated Event
NMinusOneFlowDeallocatedEvent::NMinusOneFlowDeallocatedEvent(int port_id):
				InternalEvent(InternalEvent::APP_N_MINUS_1_FLOW_DEALLOCATED)
{
	port_id_ = port_id;
	management_flow_ = false;
}

const std::string NMinusOneFlowDeallocatedEvent::toString()
{
	std::stringstream ss;
	ss<<"Event id: "<<type<<"; Port-id: "<<port_id_<<std::endl;
	return ss.str();
}

//CLASS Connectivity to Neighbor lost
ConnectiviyToNeighborLostEvent::ConnectiviyToNeighborLostEvent(const Neighbor& neighbor):
		InternalEvent(InternalEvent::APP_CONNECTIVITY_TO_NEIGHBOR_LOST)
{
	neighbor_ = neighbor;
}

const std::string ConnectiviyToNeighborLostEvent::toString()
{
	std::stringstream ss;
	ss<<"Event id: "<<type<<"; Neighbor: "<<neighbor_.toString()<<std::endl;
	return ss.str();
}

//CLASS NeighborAddedEvent
NeighborAddedEvent::NeighborAddedEvent(const Neighbor& neighbor, bool enrollee,
				       bool prepare_for_handover,
				       const rina::ApplicationProcessNamingInformation& disc_neigh):
		InternalEvent(InternalEvent::APP_NEIGHBOR_ADDED)
{
	neighbor_ = neighbor;
	enrollee_ = enrollee;
	prepare_handover = prepare_for_handover;
	disc_neigh_name = disc_neigh;
}

const std::string NeighborAddedEvent::toString()
{
	std::stringstream ss;
	ss<<"Event id: "<<type<<"; Neighbor: "<<neighbor_.toString()<<std::endl;
	ss<<"Enrollee: "<<enrollee_<<std::endl;
	if (prepare_handover) {
		ss<<"Prepare for handover: true; Disc neighbor name: "
		  << disc_neigh_name.toString()<<std::endl;
	}
	return ss.str();
}

/// A connectivity to a neighbor has been lost
NeighborDeclaredDeadEvent::NeighborDeclaredDeadEvent(const Neighbor& neighbor):
		InternalEvent(InternalEvent::APP_NEIGHBOR_DECLARED_DEAD)
{
	neighbor_ = neighbor;
}

const std::string NeighborDeclaredDeadEvent::toString()
{
	std::stringstream ss;
	ss<<"Event id: "<<type<<"; Neighbor: "<<neighbor_.toString()<<std::endl;
	return ss.str();
}

/// The address of the IPCP has changed
AddressChangeEvent::AddressChangeEvent(unsigned int new_addr,
		   	   	       unsigned int old_addr,
				       unsigned int use_new_t,
				       unsigned int deprecate_old_t):
		InternalEvent(InternalEvent::ADDRESS_CHANGE)
{
	new_address = new_addr;
	old_address = old_addr;
	use_new_timeout = use_new_t;
	deprecate_old_timeout = deprecate_old_t;
}

const std::string AddressChangeEvent::toString()
{
	std::stringstream ss;
	ss<<"Event id: "<<type<<"; New address: "<< new_address
			<<"; Old address: " << old_address << std::endl;
	ss<<"Use new address timeout: " << use_new_timeout << " ms; "
	  <<"Deprecate old address timeout: " << deprecate_old_timeout << " ms" << std::endl;
	return ss.str();
}

/// The address of a neighbor IPCP has changed
NeighborAddressChangeEvent::NeighborAddressChangeEvent(const std::string& name,
						       unsigned int new_addr,
						       unsigned int old_addr):
		InternalEvent(InternalEvent::NEIGHBOR_ADDRESS_CHANGE)
{
	new_address = new_addr;
	old_address = old_addr;
	neigh_name = name;
}

const std::string NeighborAddressChangeEvent::toString()
{
	std::stringstream ss;
	ss<<"Event id: "<<type<<"; Name: "<<neigh_name<<"; New address: "<< new_address
			<<"; Old address: " << old_address << std::endl;
	return ss.str();
}

/// An IPCP internal flow was allocated
IPCPInternalFlowAllocatedEvent::IPCPInternalFlowAllocatedEvent(unsigned int port,
							       int file_desc,
							       const FlowInformation& flow_information):
		InternalEvent(InternalEvent::IPCP_INTERNAL_FLOW_ALLOCATED)
{
	port_id = port;
	fd = file_desc;
	flow_info = flow_information;
}

const std::string IPCPInternalFlowAllocatedEvent::toString()
{
	std::stringstream ss;
	ss<<"Event id: "<<type<<"; Port id: "<<port_id
	   <<"; File descriptor: " << fd << std::endl;
	ss<<"Flow description: "<<flow_info.toString();
	return ss.str();
}

// An IPCP internal flow allocation failed
IPCPInternalFlowAllocationFailedEvent::IPCPInternalFlowAllocationFailedEvent(unsigned int errc,
			         	 	 	 	  	     const FlowInformation& flow_information,
									     const std::string& result_reason):
		InternalEvent(InternalEvent::IPCP_INTERNAL_FLOW_ALLOCATION_FAILED)
{
	error_code = errc;
	reason = result_reason;
	flow_info = flow_information;
}

const std::string IPCPInternalFlowAllocationFailedEvent::toString()
{
	std::stringstream ss;
	ss<<"Event id: "<<type<<"; Error code: "<<error_code;
	ss<<"; Result reason: "<<reason<<std::endl;
	ss<<"Flow description: "<<flow_info.toString();
	return ss.str();
}

// An IPCP internal flow has been deallocated
IPCPInternalFlowDeallocatedEvent::IPCPInternalFlowDeallocatedEvent(int port):
		InternalEvent(InternalEvent::IPCP_INTERNAL_FLOW_DEALLOCATED)
{
	port_id = port;
}

const std::string IPCPInternalFlowDeallocatedEvent::toString()
{
	std::stringstream ss;
	ss<<"Event id: "<<type<<"; Port-id: "<<port_id;
	return ss.str();
}

}

