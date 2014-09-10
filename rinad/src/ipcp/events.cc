//
// Internal events between IPC Process components
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

#include "events.h"

namespace rinad {

//CLASS BaseEvent
BaseEvent::BaseEvent(const IPCProcessEventType& id) {
	id_ = id;
}

IPCProcessEventType BaseEvent::get_id() const {
	return id_;
}

const std::string BaseEvent::eventIdToString(IPCProcessEventType id){
	std::string result;

	switch(id) {
	case IPCP_EVENT_CONNECTIVITY_TO_NEIGHBOR_LOST:
		result = "0_CONNECTIVITY_TO_NEIGHBOR_LOST";
		break;
	case IPCP_EVENT_EFCP_CONNECTION_CREATED:
		result = "1_EFCP_CONNECTION_CREATED";
		break;
	case IPCP_EVENT_EFCP_CONNECTION_DELETED:
		result = "2_EFCP_CONNECTION_DELETED";
		break;
	case IPCP_EVENT_MANAGEMENT_FLOW_ALLOCATED:
		result = "3_MANAGEMENT_FLOW_ALLOCATED";
		break;
	case IPCP_EVENT_MANAGEMENT_FLOW_DEALLOCATED:
		result = "4_MANAGEMENT_FLOW_DEALLOCATED";
		break;
	case IPCP_EVENT_N_MINUS_1_FLOW_ALLOCATED:
		result = "5_N_MINUS_1_FLOW_ALLOCATED";
		break;
	case IPCP_EVENT_N_MINUS_1_FLOW_ALLOCATION_FAILED:
		result = "6_N_MINUS_1_FLOW_ALLOCATION_FAILED";
		break;
	case IPCP_EVENT_N_MINUS_1_FLOW_DEALLOCATED:
		result = "7_N_MINUS_1_FLOW_DEALLOCATED";
		break;
	case IPCP_EVENT_NEIGHBOR_DECLARED_DEAD:
		result = "8_NEIGHBOR_DECLARED_DEAD";
		break;
	case IPCP_EVENT_NEIGHBOR_ADDED:
		result = "9_NEIGHBOR_ADDED";
		break;
	default:
		result = "Unknown event";
	}

	return result;
}

//CLASS NMinusOneFlowAllocationFailedEvent
NMinusOneFlowAllocationFailedEvent::NMinusOneFlowAllocationFailedEvent(unsigned int handle,
			const rina::FlowInformation& flow_information,
			const std::string& result_reason): BaseEvent(IPCP_EVENT_N_MINUS_1_FLOW_ALLOCATION_FAILED) {
	handle_ = handle;
	flow_information_ = flow_information;
	result_reason_ = result_reason;
}

const std::string NMinusOneFlowAllocationFailedEvent::toString() {
	std::stringstream ss;
	ss<<"Event id: "<<get_id()<<"; Handle: "<<handle_;
	ss<<"; Result reason: "<<result_reason_<<std::endl;
	ss<<"Flow description: "<<flow_information_.toString();
	return ss.str();
}

//CLASS NMinusOneFlowAllocatedEvent
NMinusOneFlowAllocatedEvent::NMinusOneFlowAllocatedEvent(unsigned int handle,
			const rina::FlowInformation& flow_information):
					BaseEvent(IPCP_EVENT_N_MINUS_1_FLOW_ALLOCATED) {
	handle_ = handle;
	flow_information_ = flow_information;
}

const std::string NMinusOneFlowAllocatedEvent::toString() {
	std::stringstream ss;
	ss<<"Event id: "<<get_id()<<"; Handle: "<<handle_<<std::endl;
	ss<<"Flow description: "<<flow_information_.toString();
	return ss.str();
}

//CLASS NMinusOneFlowDeallocated Event
NMinusOneFlowDeallocatedEvent::NMinusOneFlowDeallocatedEvent(int port_id,
			rina::CDAPSessionDescriptor * cdap_session_descriptor):
				BaseEvent(IPCP_EVENT_N_MINUS_1_FLOW_DEALLOCATED) {
	port_id_ = port_id;
	cdap_session_descriptor_ = cdap_session_descriptor;
}

const std::string NMinusOneFlowDeallocatedEvent::toString() {
	std::stringstream ss;
	ss<<"Event id: "<<get_id()<<"; Port-id: "<<port_id_<<std::endl;
	return ss.str();
}

//CLASS Connectivity to Neighbor lost
ConnectiviyToNeighborLostEvent::ConnectiviyToNeighborLostEvent(rina::Neighbor* neighbor):
		BaseEvent(IPCP_EVENT_CONNECTIVITY_TO_NEIGHBOR_LOST) {
	neighbor_ = neighbor;
}

const std::string ConnectiviyToNeighborLostEvent::toString() {
	std::stringstream ss;
	ss<<"Event id: "<<get_id()<<"; Neighbor: "<<neighbor_->toString()<<std::endl;
	return ss.str();
}

//CLASS NeighborAddedEvent
NeighborAddedEvent::NeighborAddedEvent(rina::Neighbor * neighbor, bool enrollee):
	BaseEvent(IPCP_EVENT_NEIGHBOR_ADDED) {
	neighbor_ = neighbor;
	enrollee_ = enrollee;
}

const std::string NeighborAddedEvent::toString() {
	std::stringstream ss;
	ss<<"Event id: "<<get_id()<<"; Neighbor: "<<neighbor_->toString()<<std::endl;
	ss<<"Enrollee: "<<enrollee_<<std::endl;
	return ss.str();
}

/// A connectivity to a neighbor has been lost
NeighborDeclaredDeadEvent::NeighborDeclaredDeadEvent(rina::Neighbor * neighbor):
	BaseEvent(IPCP_EVENT_NEIGHBOR_DECLARED_DEAD) {
	neighbor_ = neighbor;
}

const std::string NeighborDeclaredDeadEvent::toString() {
	std::stringstream ss;
	ss<<"Event id: "<<get_id()<<"; Neighbor: "<<neighbor_->toString()<<std::endl;
	return ss.str();
}

}
