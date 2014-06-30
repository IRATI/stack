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

#define RINA_PREFIX "flow-allocator"

#include <librina/logs.h>
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

//Class Flow RIB Object
FlowRIBObject::FlowRIBObject(IPCProcess * ipc_process, const std::string& object_name,
		IFlowAllocatorInstance * flow_allocator_instance):
	SimpleSetMemberRIBObject(ipc_process, Flow::FLOW_RIB_OBJECT_CLASS, object_name,
			flow_allocator_instance->get_flow()) {
	flow_allocator_instance_ = flow_allocator_instance;
}

void FlowRIBObject::remoteDeleteObject(const rina::CDAPMessage * cdapMessage,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	flow_allocator_instance_->deleteFlowRequestMessageReceived(cdapMessage,
			cdapSessionDescriptor->get_port_id());
}

//Class Flow Set RIB Object
FlowSetRIBObject::FlowSetRIBObject(IPCProcess * ipc_process, IFlowAllocator * flow_allocator):
		BaseRIBObject(ipc_process, Flow::FLOW_SET_RIB_OBJECT_CLASS,
				objectInstanceGenerator->getObjectInstance(), Flow::FLOW_SET_RIB_OBJECT_NAME) {
	flow_allocator_ = flow_allocator;
}

void FlowSetRIBObject::remoteCreateObject(const rina::CDAPMessage * cdapMessage,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	flow_allocator_->createFlowRequestMessageReceived(cdapMessage,
			cdapSessionDescriptor->get_port_id());
}

void FlowSetRIBObject::createObject(const std::string& objectClass, const std::string& objectName,
		IFlowAllocatorInstance* objectValue) {
	FlowRIBObject * flowRIBObject;

	flowRIBObject = new FlowRIBObject(get_ipc_process(), objectName, objectValue);
	add_child(flowRIBObject);
	get_rib_daemon()->addRIBObject(flowRIBObject);
}

const void* FlowSetRIBObject::get_value() const {
	return flow_allocator_;
}

//Class QoS Cube Set RIB Object
const std::string QoSCubeSetRIBObject::QOS_CUBE_SET_RIB_OBJECT_NAME = RIBObjectNames::SEPARATOR +
		RIBObjectNames::DIF + RIBObjectNames::SEPARATOR + RIBObjectNames::MANAGEMENT +
		RIBObjectNames::SEPARATOR + RIBObjectNames::FLOW_ALLOCATOR + RIBObjectNames::SEPARATOR +
		RIBObjectNames::QOS_CUBES;
const std::string QoSCubeSetRIBObject::QOS_CUBE_SET_RIB_OBJECT_CLASS = "qoscube set";
const std::string QoSCubeSetRIBObject::QOS_CUBE_RIB_OBJECT_CLASS = "qoscube";

QoSCubeSetRIBObject::QoSCubeSetRIBObject(IPCProcess * ipc_process):
		BaseRIBObject(ipc_process, QOS_CUBE_SET_RIB_OBJECT_CLASS,
				objectInstanceGenerator->getObjectInstance(), QOS_CUBE_SET_RIB_OBJECT_NAME) {
}

void QoSCubeSetRIBObject::remoteCreateObject(const rina::CDAPMessage * cdapMessage,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	//TODO, depending on IEncoder
}

void QoSCubeSetRIBObject::createObject(const std::string& objectClass,
		const std::string& objectName, rina::QoSCube* objectValue) {
	SimpleSetMemberRIBObject * ribObject = new SimpleSetMemberRIBObject(get_ipc_process(),
			QOS_CUBE_RIB_OBJECT_CLASS, objectName, objectValue);
	add_child(ribObject);
	get_rib_daemon()->addRIBObject(ribObject);
	//TODO: the QoS cube should be added into the configuration
}

void QoSCubeSetRIBObject::deleteObject() {
	std::list<std::string> childNames;
	std::list<BaseRIBObject*>::const_iterator childrenIt;
	std::list<std::string>::const_iterator namesIt;

	for(childrenIt = get_children().begin();
			childrenIt != get_children().end(); ++childrenIt) {
		childNames.push_back((*childrenIt)->get_name());
	}

	for(namesIt = childNames.begin(); namesIt != childNames.end();
			++namesIt) {
		remove_child(*namesIt);
	}
}

const void* QoSCubeSetRIBObject::get_value() const {
	return 0;
}

//Class Flow Allocator
FlowAllocator::FlowAllocator() {
	ipc_process_ = 0;
	rib_daemon_ = 0;
	cdap_session_manager_ = 0;
	encoder_ = 0;
	namespace_manager_ = 0;
}

FlowAllocator::~FlowAllocator() {
	std::map<int, IFlowAllocatorInstance *>::iterator it;

	fai_map_lock_.lock();

	for(it = flow_allocator_instances_.begin();
			it != flow_allocator_instances_.end(); ++it){
		delete it->second;
	}

	fai_map_lock_.unlock();
}

void FlowAllocator::set_ipc_process(IPCProcess * ipc_process) {
	ipc_process_ = ipc_process;
	rib_daemon_ = ipc_process_->get_rib_daemon();
	encoder_ = ipc_process_->get_encoder();
	cdap_session_manager_ = ipc_process_->get_cdap_session_manager();
	namespace_manager_ = ipc_process_->get_namespace_manager();
	populateRIB();
}

void FlowAllocator::populateRIB() {
	try {
		BaseRIBObject * object = new FlowSetRIBObject(ipc_process_, this);
		rib_daemon_->addRIBObject(object);
		object = new QoSCubeSetRIBObject(ipc_process_);
		rib_daemon_->addRIBObject(object);
	} catch (Exception &e) {
		LOG_ERR("Problems adding object to the RIB : %s", e.what());
	}
}

void FlowAllocator::createFlowRequestMessageReceived(const rina::CDAPMessage * cdapMessage,
		int underlyingPortId) {
	Flow * flow;
	IFlowAllocatorInstance * flow_allocator_instance;
	unsigned int myAddress = 0;
	int portId = 0;

	try{
		rina::ByteArrayObjectValue * value = (rina::ByteArrayObjectValue*)  cdapMessage->get_obj_value();
		flow = (Flow *) encoder_->decode((char*) value->get_value());
	}catch (Exception &e){
		LOG_ERR("Problems decoding object value: %s", e.what());
		return;
	}

	unsigned int address = namespace_manager_->getDFTNextHop(flow->get_destination_naming_info());
	myAddress = ipc_process_->get_address();
	if (address == 0){
		LOG_ERR("The directory forwarding table returned no entries when looking up %s",
				flow->get_destination_naming_info().toString().c_str());
		return;
	}

	if (address == myAddress) {
		//There is an entry and the address is this IPC Process, create a FAI, extract the Flow
		//object from the CDAP message and call the FAI
		try {
			portId = rina::extendedIPCManager->allocatePortId(flow->get_destination_naming_info());
		}catch (Exception &e) {
			LOG_ERR("Problems requesting an available port-id: %s. Ignoring the Flow allocation request",
					e.what());
			return;
		}

		LOG_DBG("The destination application process is reachable through me. Assigning the local port-id %d to the flow", portId);
		//TODO create FlowAllocatorInstance
		fai_map_lock_.lock();
		flow_allocator_instances_[portId] = flow_allocator_instance;
		fai_map_lock_.unlock();

		flow_allocator_instance->createFlowRequestMessageReceived(flow, cdapMessage, underlyingPortId);
		return;
	}


	//The address is not this IPC process, forward the CDAP message to that address increment the hop
	//count of the Flow object extract the flow object from the CDAP message
	flow->set_hop_count(flow->get_hop_count() - 1);
	if (flow->get_hop_count() <= 0) {
		//TODO send negative create Flow response CDAP message to the source IPC process, specifying
		//that the application process could not be found before the hop count expired
		LOG_ERR("Missing code");
	}

	LOG_ERR("Missing code");
}

}
