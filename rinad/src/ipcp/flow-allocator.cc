//
// Flow Allocator
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

#include "flow-allocator.h"

namespace rinad {

//	CLASS Flow
const std::string Flow::FLOW_SET_RIB_OBJECT_NAME = RIBObjectNames::SEPARATOR +
	RIBObjectNames::DIF + RIBObjectNames::SEPARATOR + RIBObjectNames::RESOURCE_ALLOCATION
	+ RIBObjectNames::SEPARATOR + RIBObjectNames::FLOW_ALLOCATOR + RIBObjectNames::SEPARATOR
	+ RIBObjectNames::FLOWS;
const std::string Flow::FLOW_SET_RIB_OBJECT_CLASS = "flow set";
const std::string Flow::FLOW_RIB_OBJECT_CLASS = "flow";

Flow::Flow() {
	source_port_id_ = 0;
	destination_port_id_ = 0;
	source_address_ = 0;
	destination_address_ = 0;
	current_connection_index_ = 0;
	max_create_flow_retries_ = 0;
	create_flow_retries_ = 0;
	hop_count_ = 0;
	source_ = false;
	state_ = EMPTY;
	access_control_ = 0;
}

bool Flow::is_source() const {
	return source_;
}

void Flow::set_source(bool source) {
	source_ = source;
}

const rina::ApplicationProcessNamingInformation& Flow::get_source_naming_info() const {
	return source_naming_info_;
}

void Flow::set_source_naming_info(const rina::ApplicationProcessNamingInformation& source_naming_info) {
	source_naming_info_ = source_naming_info;
}

const rina::ApplicationProcessNamingInformation& Flow::get_destination_naming_info() const {
	return destination_naming_info_;
}

void Flow::set_destination_naming_info(const rina::ApplicationProcessNamingInformation& destination_naming_info) {
	destination_naming_info_ = destination_naming_info;
}

unsigned int Flow::get_source_port_id() const {
	return source_port_id_;
}

void Flow::set_source_port_id(unsigned int source_port_id) {
	source_port_id_ = source_port_id;
}

unsigned int Flow::get_destination_port_id() const {
	return destination_port_id_;
}

void Flow::set_destination_port_id(unsigned int destination_port_id) {
	destination_port_id_ = destination_port_id;
}

unsigned int Flow::get_source_address() const {
	return source_address_;
}

void Flow::set_source_address(unsigned int source_address) {
	source_address_ = source_address;
}

unsigned int Flow::get_destination_address() const {
	return destination_address_;
}

void Flow::set_destination_address(unsigned int destination_address) {
	destination_address_ = destination_address;
}

const std::list<rina::Connection>& Flow::get_connections() const {
	return connections_;
}

void Flow::set_connections(const std::list<rina::Connection>& connections) {
	connections_ = connections;
}

unsigned int Flow::get_current_connection_index() const {
	return current_connection_index_;
}

void Flow::set_current_connection_index(unsigned int current_connection_index) {
	current_connection_index_ = current_connection_index;
}

Flow::IPCPFlowState Flow::get_state() const{
	return state_;
}

void Flow::set_state(IPCPFlowState state) {
	state_ = state;
}

const rina::FlowSpecification& Flow::get_flow_specification() const {
	return flow_specification_;
}

void Flow::set_flow_specification(const rina::FlowSpecification& flow_specification) {
	flow_specification_ = flow_specification;
}

char* Flow::get_access_control() const {
	return access_control_;
}

void Flow::set_access_control(char* access_control) {
	access_control_ = access_control;
}

unsigned int Flow::get_max_create_flow_retries() const {
	return max_create_flow_retries_;
}

void Flow::set_max_create_flow_retries(unsigned int max_create_flow_retries) {
	max_create_flow_retries_ = max_create_flow_retries;
}

unsigned int Flow::get_create_flow_retries() const {
	return create_flow_retries_;
}

void Flow::set_create_flow_retries(unsigned int create_flow_retries) {
	create_flow_retries_ = create_flow_retries;
}

unsigned int Flow::get_hop_count() const {
	return hop_count_;
}

void Flow::set_hop_count(unsigned int hop_count) {
	hop_count_ = hop_count;
}

std::string Flow::toString() {
    std::stringstream ss;
    ss << "* State: " << state_ << std::endl;
    ss << "* Is this IPC Process the requestor of the flow? " << source_ << std::endl;
    ss << "* Max create flow retries: " << max_create_flow_retries_ << std::endl;
    ss << "* Hop count: " << hop_count_ << std::endl;
    ss << "* Source AP Naming Info: " << source_naming_info_.toString() << std::endl;;
    ss << "* Source address: " << source_address_ << std::endl;
    ss << "* Source port id: " << source_port_id_ << std::endl;
    ss <<  "* Destination AP Naming Info: " << destination_naming_info_.toString();
    ss <<  "* Destination addres: " + destination_address_ << std::endl;
    ss << "* Destination port id: "+ destination_port_id_ << std::endl;
    if (connections_.size() > 0) {
		ss << "* Connection ids of the connection supporting this flow: +\n";
		for(std::list<rina::Connection>::const_iterator iterator = connections_.begin(), end = connections_.end(); iterator != end; ++iterator) {
			ss << "Src CEP-id " << iterator->getSourceCepId()
					<< "; Dest CEP-id " << iterator->getDestCepId()
					<< "; Qos-id " << iterator->getQosId() << std::endl;
		}
	}
	ss << "* Index of the current active connection for this flow: " << current_connection_index_ << std::endl;
	return ss.str();
}

}
