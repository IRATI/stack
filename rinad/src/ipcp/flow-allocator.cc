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
#include <vector>

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

Flow::~Flow() {
	std::list<rina::Connection*>::iterator iterator;

	for(iterator = connections_.begin(); iterator != connections_.end(); ++iterator) {
		delete *iterator;
	}
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

const std::list<rina::Connection*>& Flow::get_connections() const {
	return connections_;
}

void Flow::set_connections(const std::list<rina::Connection*>& connections) {
	connections_ = connections;
}

unsigned int Flow::get_current_connection_index() const {
	return current_connection_index_;
}

void Flow::set_current_connection_index(unsigned int current_connection_index) {
	current_connection_index_ = current_connection_index;
}

rina::Connection * Flow::getActiveConnection() {
	rina::Connection result;
	std::list<rina::Connection*>::iterator iterator;

	int i=0;
	for(iterator = connections_.begin(); iterator != connections_.end(); ++iterator) {
		if (i == current_connection_index_) {
			return *iterator;
		} else {
			i++;
		}
	}

	throw Exception("No active connection is currently defined");
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
		for(std::list<rina::Connection*>::const_iterator iterator = connections_.begin(), end = connections_.end(); iterator != end; ++iterator) {
			ss << "Src CEP-id " << (*iterator)->getSourceCepId()
					<< "; Dest CEP-id " << (*iterator)->getDestCepId()
					<< "; Qos-id " << (*iterator)->getQosId() << std::endl;
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
	flow_allocator_instances_.deleteValues();
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
	IFlowAllocatorInstance * flowAllocatorInstance;
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
		flowAllocatorInstance = new FlowAllocatorInstance(ipc_process_, this, cdap_session_manager_, portId);
		flow_allocator_instances_.put(portId, flowAllocatorInstance);

		//TODO check if this operation throws an exception an react accordingly
		flowAllocatorInstance->createFlowRequestMessageReceived(flow, cdapMessage, underlyingPortId);
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

void FlowAllocator::replyToIPCManager(const rina::FlowRequestEvent& event, int result) {
	try{
		rina::extendedIPCManager->allocateFlowRequestResult(event, result);
	}catch (Exception &e){
		LOG_ERR("Problems communicating with the IPC Manager Daemon: %s", e.what());
	}
}

void FlowAllocator::submitAllocateRequest(rina::FlowRequestEvent * event) {
	int portId = 0;
	IFlowAllocatorInstance * flowAllocatorInstance;

	try {
		portId = rina::extendedIPCManager->allocatePortId(event->getLocalApplicationName());
		LOG_DBG("Got assigned port-id %d", portId);
	} catch (Exception &e) {
		LOG_ERR("Problems requesting an available port-id to the Kernel IPC Manager: %s"
				, e.what());
		replyToIPCManager(*event, -1);
	}

	event->setPortId(portId);
	flowAllocatorInstance = new FlowAllocatorInstance(ipc_process_, this, cdap_session_manager_, portId);
	flow_allocator_instances_.put(portId, flowAllocatorInstance);

	try {
		flowAllocatorInstance->submitAllocateRequest(*event);
	} catch (Exception &e) {
		flow_allocator_instances_.erase(portId);
		delete flowAllocatorInstance;

		try {
			rina::extendedIPCManager->deallocatePortId(portId);
		} catch (Exception &e) {
			LOG_ERR("Problems releasing port-id %d: %s", portId, e.what());
		}

		replyToIPCManager(*event, -1);
	}
}

void FlowAllocator::processCreateConnectionResponseEvent(const rina::CreateConnectionResponseEvent& event) {
	IFlowAllocatorInstance * flowAllocatorInstance;

	flowAllocatorInstance = flow_allocator_instances_.find(event.getPortId());
	if (flowAllocatorInstance) {
		flowAllocatorInstance->processCreateConnectionResponseEvent(event);
	} else {
		LOG_ERR("Received create connection response event associated to unknown port-id %d",
				event.getPortId());
	}
}

void FlowAllocator::submitAllocateResponse(const rina::AllocateFlowResponseEvent& event) {
	IFlowAllocatorInstance * flowAllocatorInstance;

	LOG_DBG("Local application invoked allocate response with seq num %ud and result %d, "
			, event.getSequenceNumber(), event.getResult());

	std::list<IFlowAllocatorInstance *> fais = flow_allocator_instances_.getEntries();
	std::list<IFlowAllocatorInstance *>::iterator iterator;
	for(iterator = fais.begin(); iterator != fais.end(); ++iterator) {
		if ((*iterator)->get_allocate_response_message_handle() == event.getSequenceNumber()) {
			flowAllocatorInstance = *iterator;
			flowAllocatorInstance->submitAllocateResponse(event);
			return;
		}
	}

	LOG_ERR("Could not find FAI with handle %ud", event.getSequenceNumber());
}

void FlowAllocator::processCreateConnectionResultEvent(const rina::CreateConnectionResultEvent& event) {
	IFlowAllocatorInstance * flowAllocatorInstance;

	flowAllocatorInstance = flow_allocator_instances_.find(event.getPortId());
	if (!flowAllocatorInstance) {
		LOG_ERR("Problems looking for FAI at portId %d", event.getPortId());
		try {
			rina::extendedIPCManager->deallocatePortId(event.getPortId());
		} catch (Exception &e) {
			LOG_ERR("Problems requesting IPC Manager to deallocate port-id %d: %s",
					event.getPortId(), e.what());
		}
	} else {
		flowAllocatorInstance->processCreateConnectionResultEvent(event);
	}
}

void FlowAllocator::processUpdateConnectionResponseEvent(const rina::UpdateConnectionResponseEvent& event) {
	IFlowAllocatorInstance * flowAllocatorInstance;

	flowAllocatorInstance = flow_allocator_instances_.find(event.getPortId());
	if (!flowAllocatorInstance) {
		LOG_ERR("Problems looking for FAI at portId %d", event.getPortId());
		try {
			rina::extendedIPCManager->deallocatePortId(event.getPortId());
		} catch (Exception &e) {
			LOG_ERR("Problems requesting IPC Manager to deallocate port-id %d: %s",
					event.getPortId(), e.what());
		}
	} else {
		flowAllocatorInstance->processUpdateConnectionResponseEvent(event);
	}
}

void FlowAllocator::submitDeallocate(const rina::FlowDeallocateRequestEvent& event) {
	IFlowAllocatorInstance * flowAllocatorInstance;

	flowAllocatorInstance = flow_allocator_instances_.find(event.getPortId());
	if (!flowAllocatorInstance) {
		LOG_ERR("Problems looking for FAI at portId %d", event.getPortId());
		try {
			rina::extendedIPCManager->deallocatePortId(event.getPortId());
		} catch (Exception &e) {
			LOG_ERR("Problems requesting IPC Manager to deallocate port-id %d: %s",
					event.getPortId(), e.what());
		}

		try {
			rina::extendedIPCManager->notifyflowDeallocated(event, -1);
		} catch (Exception &e) {
			LOG_ERR("Error communicating with the IPC Manager: %s", e.what());
		}
	} else {
		flowAllocatorInstance->submitDeallocate(event);
		try {
			rina::extendedIPCManager->notifyflowDeallocated(event, -1);
		} catch (Exception &e) {
			LOG_ERR("Error communicating with the IPC Manager: %s", e.what());
		}
	}
}

void FlowAllocator::removeFlowAllocatorInstance(int portId) {
	IFlowAllocatorInstance * fai = flow_allocator_instances_.erase(portId);
	if (fai) {
		delete fai;
	}
}

//Class Simple New flow Request Policy
Flow * SimpleNewFlowRequestPolicy::generateFlowObject(const rina::FlowRequestEvent& event,
				const std::string& difName, const std::list<rina::QoSCube>& qosCubes) {
	Flow* flow;

	flow = new Flow();
	flow->set_destination_naming_info(event.getRemoteApplicationName());
	flow->set_source_naming_info(event.getLocalApplicationName());
	flow->set_hop_count(3);
	flow->set_max_create_flow_retries(1);
	flow->set_source(true);
	flow->set_state(Flow::ALLOCATION_IN_PROGRESS);

	std::list<rina::Connection*> connections;
	rina::QoSCube qosCube = selectQoSCube(event.getFlowSpecification(), qosCubes);
	LOG_DBG("Selected qos cube with name %s and policies: %s",
			qosCube.get_name().c_str());

	rina::Connection * connection = new rina::Connection();
	connection->setQosId(1);
	connection->setFlowUserIpcProcessId(event.getFlowRequestorIPCProcessId());
	rina::ConnectionPolicies connectionPolicies = rina::ConnectionPolicies(qosCube.get_efcp_policies());
	connectionPolicies.set_in_order_delivery(qosCube.is_ordered_delivery());
	connectionPolicies.set_partial_delivery(qosCube.is_partial_delivery());
	connectionPolicies.set_max_sdu_gap(qosCube.get_max_allowable_gap());
	connection->setPolicies(connectionPolicies);
	connections.push_back(connection);

	flow->set_connections(connections);
	flow->set_current_connection_index(0);
	flow->set_flow_specification(event.getFlowSpecification());

	return flow;
}

rina::QoSCube SimpleNewFlowRequestPolicy::selectQoSCube(const rina::FlowSpecification& flowSpec,
		const std::list<rina::QoSCube>& qosCubes) {
	if (flowSpec.getMaxAllowableGap()< 0) {
		return qosCubes.front();
	}

	std::list<rina::QoSCube>::const_iterator iterator;
	rina::QoSCube cube;
	for(iterator = qosCubes.begin(); iterator != qosCubes.end(); ++iterator) {
		cube = *iterator;
		if (cube.get_efcp_policies().is_dtcp_present()) {
			if (flowSpec.getMaxAllowableGap() > 0 &&
					!cube.get_efcp_policies().get_dtcp_configuration().is_rtx_control()) {
				return cube;
			}

			if (flowSpec.getMaxAllowableGap() == 0 &&
					cube.get_efcp_policies().get_dtcp_configuration().is_rtx_control()) {
				return cube;
			}
		}
	}

	throw Exception("Could not find a QoS Cube with Rtx control disabled!");
}

//Class Flow Allocator Instance
FlowAllocatorInstance::FlowAllocatorInstance(IPCProcess * ipc_process, IFlowAllocator * flow_allocator,
			rina::CDAPSessionManagerInterface * cdap_session_manager, int port_id) {
	initialize(ipc_process, flow_allocator, port_id);
	cdap_session_manager_ = cdap_session_manager;
	new_flow_request_policy_ = new SimpleNewFlowRequestPolicy();
	LOG_DBG("Created flow allocator instance to manage the flow identified by portId %d ",
			port_id);
}

FlowAllocatorInstance::FlowAllocatorInstance(IPCProcess * ipc_process, IFlowAllocator * flow_allocator,
				int port_id) {
	initialize(ipc_process, flow_allocator, port_id);
	new_flow_request_policy_ = 0;
	LOG_DBG("Created flow allocator instance to manage the flow identified by portId %d ",
				port_id);
}

FlowAllocatorInstance::~FlowAllocatorInstance() {
	if (new_flow_request_policy_) {
		delete new_flow_request_policy_;
	}

	if (flow_) {
		delete flow_;
	}

	if (request_message_) {
		delete request_message_;
	}

	if (lock_) {
		delete lock_;
	}
}

void FlowAllocatorInstance::initialize(IPCProcess * ipc_process,
		IFlowAllocator * flow_allocator, int port_id) {
	flow_allocator_ = flow_allocator;
	ipc_process_ = ipc_process;
	port_id_ = port_id;
	rib_daemon_ = ipc_process->get_rib_daemon();
	encoder_ = ipc_process->get_encoder();
	namespace_manager_ = ipc_process->get_namespace_manager();
	state_ = NO_STATE;
	allocate_response_message_handle_ = 0;
	request_message_ = 0;
	underlying_port_id_ = 0;
	lock_ = new rina::Lockable();
}

int FlowAllocatorInstance::get_port_id() const {
	return port_id_;
}

Flow * FlowAllocatorInstance::get_flow() const {
	return flow_;
}

bool FlowAllocatorInstance::isFinished() const {
	return state_ == FINISHED;
}

unsigned int FlowAllocatorInstance::get_allocate_response_message_handle() const {
	rina::AccessGuard guard = rina::AccessGuard(*lock_);
	return allocate_response_message_handle_;
}

void FlowAllocatorInstance::set_allocate_response_message_handle(unsigned int allocate_response_message_handle) {
	rina::AccessGuard(*lock_);
	allocate_response_message_handle_ = allocate_response_message_handle;
}

void FlowAllocatorInstance::submitAllocateRequest(const rina::FlowRequestEvent& event) {
	rina::AccessGuard(*lock_);

	flow_request_event_ = rina::FlowRequestEvent(event);
	flow_ = new_flow_request_policy_->generateFlowObject(event,
			ipc_process_->get_dif_information().get_dif_name().getProcessName(),
			ipc_process_->get_dif_information().get_dif_configuration().get_efcp_configuration().get_qos_cubes());

	LOG_DBG("Generated flow object");

	//1 Check directory to see to what IPC process the CDAP M_CREATE request has to be delivered
	unsigned int destinationAddress =
			namespace_manager_->getDFTNextHop(event.getRemoteApplicationName());
	LOG_DBG("The directory forwarding table returned address %ud",
			destinationAddress);
	flow_->set_destination_address(destinationAddress);
	if (destinationAddress == 0){
		std::stringstream ss;
		ss<<"Could not find entry in DFT for application ";
		ss<<event.getRemoteApplicationName().toString();
		throw Exception(ss.str().c_str());
	}

	//2 Check if the destination address is this IPC process (then invoke degenerated form of IPC)
	unsigned int sourceAddress = ipc_process_->get_address();
	flow_->set_source_address(sourceAddress);
	flow_->set_source_port_id(port_id_);
	std::stringstream ss;
	ss<<Flow::FLOW_SET_RIB_OBJECT_NAME<<RIBObjectNames::SEPARATOR<<sourceAddress<<"-"<<port_id_;
	object_name_= ss.str();
	if (destinationAddress == sourceAddress){
		// At the moment we don't support allocation of flows between applications at the
		// same processing system
		throw Exception("Allocation of flows between local applications not supported yet");
	}

	//3 Request the creation of the connection(s) in the Kernel
	state_ = CONNECTION_CREATE_REQUESTED;
	rina::kernelIPCProcess->createConnection(*(flow_->getActiveConnection()));
	LOG_DBG("Requested the creation of a connection to the kernel, for flow with port-id %d",
			port_id_);
}

void FlowAllocatorInstance::replyToIPCManager(rina::FlowRequestEvent & event, int result) {
	try{
		rina::extendedIPCManager->allocateFlowRequestResult(event, result);
	} catch(Exception &e) {
		LOG_ERR("Problems communicating with the IPC Manager Daemon: %s", e.what());
	}
}

void FlowAllocatorInstance::releasePortId() {
	try{
		rina::extendedIPCManager->deallocatePortId(port_id_);
	} catch(Exception &e) {
		LOG_ERR("Problems releasing port-id %d", port_id_);
	}
}

void FlowAllocatorInstance::processCreateConnectionResponseEvent(const rina::CreateConnectionResponseEvent& event) {
	lock_->lock();

	if (state_ != CONNECTION_CREATE_REQUESTED) {
		LOG_ERR("Received a process Create Connection Response Event while in %d state. Ignoring it",
				state_);
		lock_->unlock();
		return;
	}

	if (event.getCepId() < 0) {
		LOG_ERR("The EFCP component of the IPC Process could not create a connection instance: %d",
				event.getCepId());
		replyToIPCManager(flow_request_event_, -1);
		lock_->unlock();
		return;
	}

	LOG_DBG("Created connection with cep-id %d", event.getCepId());
	flow_->getActiveConnection()->setSourceCepId(event.getCepId());

	try{
		//5 get the portId of any open CDAP session
		std::vector<int> cdapSessions;
		cdap_session_manager_->getAllCDAPSessionIds(cdapSessions);
		int cdapSessionId = cdapSessions[0];

		//6 Encode the flow object and send it to the destination IPC process
		rina::ByteArrayObjectValue objectValue = rina::ByteArrayObjectValue(encoder_->encode(flow_));
		const rina::CDAPMessage * cdapMessage = cdap_session_manager_->getCreateObjectRequestMessage(cdapSessionId, 0,
				rina::CDAPMessage::NONE_FLAGS, Flow::FLOW_RIB_OBJECT_CLASS, 0, object_name_, &objectValue, 0, true);

		underlying_port_id_ = cdapSessionId;
		request_message_ = cdapMessage;
		state_ = MESSAGE_TO_PEER_FAI_SENT;

		rib_daemon_->sendMessageToAddress(*request_message_, cdapSessionId, flow_->get_destination_address(), this);
	}catch(Exception &e){
		LOG_ERR("Problems sending M_CREATE <Flow> CDAP message to neighbor: %s",
				e.what());
		releasePortId();
		replyToIPCManager(flow_request_event_, -1);
		lock_->unlock();
		flow_allocator_->removeFlowAllocatorInstance(port_id_);
		return;
	}

	lock_->unlock();
}

void FlowAllocatorInstance::createFlowRequestMessageReceived(Flow * flow, const rina::CDAPMessage * requestMessage,
		int underlyingPortId){
	rina::AccessGuard guard = rina::AccessGuard(*lock_);
	//TODO
}

void FlowAllocatorInstance::processCreateConnectionResultEvent(const rina::CreateConnectionResultEvent& event) {
	rina::AccessGuard(*lock_);
	//TODO
}

void FlowAllocatorInstance::submitAllocateResponse(const rina::AllocateFlowResponseEvent& event) {
	rina::AccessGuard(*lock_);
	//TODO
}

void FlowAllocatorInstance::processUpdateConnectionResponseEvent(const rina::UpdateConnectionResponseEvent& event)  {
	rina::AccessGuard(*lock_);
	//TODO
}

void FlowAllocatorInstance::submitDeallocate(const rina::FlowDeallocateRequestEvent& event){
	rina::AccessGuard(*lock_);
	//TODO
}

void FlowAllocatorInstance::deleteFlowRequestMessageReceived(const rina::CDAPMessage * requestMessage,
		int underlyingPortId) {
	rina::AccessGuard(*lock_);
	//TODO
}

void FlowAllocatorInstance::destroyFlowAllocatorInstance(const std::string& flowObjectName, bool requestor) {
	rina::AccessGuard(*lock_);
	//TODO
}

void FlowAllocatorInstance::createResponse(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor){
	rina::AccessGuard(*lock_);
		//TODO
}

void FlowAllocatorInstance::deleteResponse(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
}

void FlowAllocatorInstance::readResponse(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
}

void FlowAllocatorInstance::cancelReadResponse(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
}

void FlowAllocatorInstance::writeResponse(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
}

void FlowAllocatorInstance::startResponse(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
}

void FlowAllocatorInstance::stopResponse(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
}

}
