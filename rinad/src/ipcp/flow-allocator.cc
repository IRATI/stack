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

	unsigned int i=0;
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
		const std::string& object_class, IFlowAllocatorInstance * flow_allocator_instance):
	SimpleSetMemberRIBObject(ipc_process, object_class, object_name,
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

void FlowSetRIBObject::createObject(const std::string& objectClass,
                                  const std::string& objectName,
                                  IFlowAllocatorInstance* objectValue) {
	FlowRIBObject * flowRIBObject;

	flowRIBObject = new FlowRIBObject(get_ipc_process(), objectClass, objectName, objectValue);
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
	LOG_ERR("Missing code %d, %d", cdapMessage->get_op_code(),
			cdapSessionDescriptor->get_port_id());
}

void QoSCubeSetRIBObject::createObject(const std::string& objectClass,
		const std::string& objectName, rina::QoSCube* objectValue) {
	SimpleSetMemberRIBObject * ribObject = new SimpleSetMemberRIBObject(get_ipc_process(),
			objectClass, objectName, objectValue);
	add_child(ribObject);
	get_rib_daemon()->addRIBObject(ribObject);
	//TODO: the QoS cube should be added into the configuration
}

void QoSCubeSetRIBObject::deleteObject(const void* objectValue) {
	if (objectValue) {
		LOG_WARN("Object value should have been NULL");
	}

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

	try {
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
	try {
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
				const std::list<rina::QoSCube>& qosCubes) {
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

	if (timer_) {
		delete timer_;
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
	timer_ = new rina::Timer();
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

unsigned int FlowAllocatorInstance::get_allocate_response_message_handle() const
{
        unsigned int t;

        {
                rina::AccessGuard g(*lock_);
                t = allocate_response_message_handle_;
        }

	return t;
}

void FlowAllocatorInstance::set_allocate_response_message_handle(unsigned int allocate_response_message_handle)
{
        rina::AccessGuard g(*lock_);
        allocate_response_message_handle_ = allocate_response_message_handle;
}

void FlowAllocatorInstance::submitAllocateRequest(const rina::FlowRequestEvent& event)
{
	rina::AccessGuard g(*lock_);
        
	flow_request_event_ = rina::FlowRequestEvent(event);
	flow_               = new_flow_request_policy_->generateFlowObject(event,
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

void FlowAllocatorInstance::replyToIPCManager(rina::FlowRequestEvent & event,
                                              int result)
{
	try {
		rina::extendedIPCManager->allocateFlowRequestResult(event, result);
	} catch(Exception &e) {
		LOG_ERR("Problems communicating with the IPC Manager Daemon: %s", e.what());
	}
}

void FlowAllocatorInstance::releasePortId() {
	try {
		rina::extendedIPCManager->deallocatePortId(port_id_);
	} catch(Exception &e) {
		LOG_ERR("Problems releasing port-id %d", port_id_);
	}
}

void FlowAllocatorInstance::releaseUnlockRemove() {
	releasePortId();
	lock_->unlock();
	flow_allocator_->removeFlowAllocatorInstance(port_id_);
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

	const rina::CDAPMessage * cdapMessage = 0;
	try {
		//5 get the portId of any open CDAP session
		std::vector<int> cdapSessions;
		cdap_session_manager_->getAllCDAPSessionIds(cdapSessions);
		int cdapSessionId = cdapSessions[0];

		//6 Encode the flow object and send it to the destination IPC process
		rina::ByteArrayObjectValue objectValue = rina::ByteArrayObjectValue(encoder_->encode(flow_));
		cdapMessage = cdap_session_manager_->getCreateObjectRequestMessage(cdapSessionId, 0,
				rina::CDAPMessage::NONE_FLAGS, Flow::FLOW_RIB_OBJECT_CLASS, 0, object_name_, &objectValue, 0, true);

		underlying_port_id_ = cdapSessionId;
		request_message_ = cdapMessage;
		state_ = MESSAGE_TO_PEER_FAI_SENT;

		rib_daemon_->sendMessageToAddress(*request_message_, cdapSessionId, flow_->get_destination_address(), this);
		delete cdapMessage;
	} catch (Exception &e){
		LOG_ERR("Problems sending M_CREATE <Flow> CDAP message to neighbor: %s",
				e.what());
		delete cdapMessage;
		replyToIPCManager(flow_request_event_, -1);
		releaseUnlockRemove();
		return;
	}

	lock_->unlock();
}

void FlowAllocatorInstance::createFlowRequestMessageReceived(Flow * flow, const rina::CDAPMessage * requestMessage,
		int underlyingPortId){
	lock_->lock();

	LOG_DBG("Create flow request received: %s", flow->toString().c_str());
	flow_ = flow;
	if (flow_->get_destination_address() == 0) {
		flow_->set_destination_address(ipc_process_->get_address());
	}
	request_message_ = requestMessage;
	underlying_port_id_ = underlyingPortId;
	flow_->set_destination_port_id(port_id_);

	//1 Reverse connection source/dest addresses and CEP-ids
	rina::Connection * connection = flow_->getActiveConnection();
	connection->setPortId(port_id_);
	unsigned int aux = connection->getSourceAddress();
	connection->setSourceAddress(connection->getDestAddress());
	connection->setDestAddress(aux);
	connection->setDestCepId(connection->getSourceCepId());
	connection->setFlowUserIpcProcessId(namespace_manager_->getRegIPCProcessId(flow_->get_destination_naming_info()));
	LOG_DBG("Target application IPC Process id is %d", connection->getFlowUserIpcProcessId());

	//2 TODO Check if the source application process has access to the destination application process.
	// If not send negative M_CREATE_R back to the sender IPC process, and housekeeping.
	// Not done in this version, this decision is left to the application
	//3 TODO If it has, determine if the proposed policies for the flow are acceptable (invoke NewFlowREquestPolicy)
	//Not done in this version, it is assumed that the proposed policies for the flow are acceptable.

	//4 Request creation of connection
	try {
		state_ = CONNECTION_CREATE_REQUESTED;
		rina::kernelIPCProcess->createConnectionArrived(*connection);
		LOG_DBG("Requested the creation of a connection to the kernel to support flow with port-id %d",
				port_id_);
	} catch (Exception &e) {
		LOG_ERR("Problems requesting a connection to the kernel: %s ", e.what());
		releaseUnlockRemove();
		return;
	}

	lock_->unlock();
}

void FlowAllocatorInstance::processCreateConnectionResultEvent(const rina::CreateConnectionResultEvent& event) {
	lock_->lock();

	if (state_ != CONNECTION_CREATE_REQUESTED) {
		LOG_ERR("Received an allocate response event while not in APP_NOTIFIED_OF_INCOMING_FLOW state. Current state: %d",
				state_);
		lock_->unlock();
		return;
	}

	if (event.getSourceCepId() < 0) {
		LOG_ERR("Create connection operation was unsuccessful: %d", event.getSourceCepId());
		releaseUnlockRemove();
		return;
	}

	try {
		state_ = APP_NOTIFIED_OF_INCOMING_FLOW;
		allocate_response_message_handle_  = rina::extendedIPCManager->allocateFlowRequestArrived(flow_->get_destination_naming_info(),
				flow_->get_source_naming_info(), flow_->get_flow_specification(), port_id_);
		LOG_DBG("Informed IPC Manager about incoming flow allocation request, got handle: %ud"
				, allocate_response_message_handle_);
	} catch(Exception &e) {
		LOG_ERR("Problems informing the IPC Manager about an incoming flow allocation request: %s",
				e.what());
		releaseUnlockRemove();
		return;
	}

	lock_->unlock();
}

void FlowAllocatorInstance::submitAllocateResponse(const rina::AllocateFlowResponseEvent& event) {
	lock_->lock();

	if (state_ != APP_NOTIFIED_OF_INCOMING_FLOW) {
		LOG_ERR("Received an allocate response event while not in APP_NOTIFIED_OF_INCOMING_FLOW state. Current state: %d",
				state_);
		lock_->unlock();
		return;
	}

	const rina::CDAPMessage * cdapMessage = 0;
	if (event.getResult() == 0){
		//Flow has been accepted
		try {
			rina::ByteArrayObjectValue objectValue = rina::ByteArrayObjectValue(encoder_->encode(flow_));
			cdapMessage = cdap_session_manager_->getCreateObjectResponseMessage(
					rina::CDAPMessage::NONE_FLAGS, request_message_->get_obj_class(), 0,
					request_message_->get_obj_name(), &objectValue, 0, 0, request_message_->get_invoke_id());

			rib_daemon_->sendMessageToAddress(*cdapMessage, underlying_port_id_,
					flow_->get_source_address(), 0);
			delete cdapMessage;
		} catch (Exception &e){
			LOG_ERR("Problems requesting RIB Daemon to send CDAP Message: %s", e.what());
			delete cdapMessage;

			try {
				rina::extendedIPCManager->flowDeallocated(port_id_);
			} catch(Exception &e) {
				LOG_ERR("Problems communicating with the IPC Manager: %s", e.what());
			}

			releaseUnlockRemove();
			return;
		}

		try {
			flow_->set_state(Flow::ALLOCATED);
			rib_daemon_->createObject(Flow::FLOW_RIB_OBJECT_CLASS, object_name_, this, 0);
		} catch(Exception &e) {
			LOG_WARN("Error creating Flow Rib object: %s", e.what());
		}

		state_ = FLOW_ALLOCATED;
		lock_->unlock();
		return;
	}

	//Flow has been rejected
	try {
		rina::ByteArrayObjectValue objectValue = rina::ByteArrayObjectValue(encoder_->encode(flow_));
		cdapMessage = cdap_session_manager_->getCreateObjectResponseMessage(
				rina::CDAPMessage::NONE_FLAGS, request_message_->get_obj_class(), 0,
				request_message_->get_obj_name(), &objectValue, -1, "Application rejected the flow",
				request_message_->get_invoke_id());

		rib_daemon_->sendMessageToAddress(*cdapMessage, underlying_port_id_,
				flow_->get_source_address(), 0);
		delete cdapMessage;
		cdapMessage = 0;
	} catch (Exception &e){
		LOG_ERR("Problems requesting RIB Daemon to send CDAP Message: %s", e.what());
		if (cdapMessage) {
			delete cdapMessage;
		}
	}

	releaseUnlockRemove();
}

void FlowAllocatorInstance::processUpdateConnectionResponseEvent(const rina::UpdateConnectionResponseEvent& event)  {
	lock_->lock();

	if (state_ != CONNECTION_UPDATE_REQUESTED) {
		LOG_ERR("Received CDAP Message while not in CONNECTION_UPDATE_REQUESTED state. Current state is: %d", state_);
		lock_->unlock();
		return;
	}

	//Update connection was unsuccessful
	if (event.getResult() != 0) {
		LOG_ERR("The kernel denied the update of a connection: %d", event.getResult());

		try {
			flow_request_event_.setPortId(-1);
			rina::extendedIPCManager->allocateFlowRequestResult(flow_request_event_, event.getResult());
		} catch(Exception &e) {
			LOG_ERR("Problems communicating with the IPC Manager: %s", e.what());
		}

		releaseUnlockRemove();
		return;
	}

	//Update connection was successful
	try {
		flow_->set_state(Flow::ALLOCATED);
		rib_daemon_->createObject(Flow::FLOW_RIB_OBJECT_CLASS, object_name_, this, 0);
	} catch(Exception &e) {
		LOG_WARN("Problems requesting the RIB Daemon to create a RIB object: %s", e.what());
	}

	state_ = FLOW_ALLOCATED;

	try {
		flow_request_event_.setPortId(port_id_);
		rina::extendedIPCManager->allocateFlowRequestResult(flow_request_event_, 0);
	} catch(Exception &e) {
		LOG_ERR("Problems communicating with the IPC Manager: %s", e.what());
	}

	lock_->unlock();
}

void FlowAllocatorInstance::submitDeallocate(const rina::FlowDeallocateRequestEvent& event)
{
	rina::AccessGuard g(*lock_);

        (void) event; // Stop compiler barfs

	if (state_ != FLOW_ALLOCATED) {
		LOG_ERR("Received deallocate request while not in FLOW_ALLOCATED state. Current state is: %d", state_);
		return;
	}

	try {
		//1 Update flow state
		flow_->set_state(Flow::WAITING_2_MPL_BEFORE_TEARING_DOWN);
		state_ = WAITING_2_MPL_BEFORE_TEARING_DOWN;

		//2 Send M_DELETE
		const rina::CDAPMessage * cdapMessage = 0;
		try {
			rina::ByteArrayObjectValue objectValue = rina::ByteArrayObjectValue(encoder_->encode(flow_));
			cdapMessage = cdap_session_manager_->getDeleteObjectRequestMessage(underlying_port_id_, 0,
					rina::CDAPMessage::NONE_FLAGS, Flow::FLOW_RIB_OBJECT_CLASS, 0, object_name_, &objectValue,
					0, false);

			unsigned int address = 0;
			if (ipc_process_->get_address() == flow_->get_source_address()) {
				address = flow_->get_destination_address();
			} else {
				address = flow_->get_source_address();
			}

			rib_daemon_->sendMessageToAddress(*cdapMessage, underlying_port_id_, address, 0);
			delete cdapMessage;
		} catch (Exception &e){
			LOG_ERR("Problems sending M_DELETE flow request: %s", e.what());
			delete cdapMessage;
		}

		//3 Wait 2*MPL before tearing down the flow
		TearDownFlowTimerTask * timerTask = new TearDownFlowTimerTask(this, object_name_, true);
		timer_->scheduleTask(timerTask, TearDownFlowTimerTask::DELAY);
	} catch (Exception &e){
		LOG_ERR("Problems processing flow deallocation request: %s", +e.what());
	}

}

void FlowAllocatorInstance::deleteFlowRequestMessageReceived(const rina::CDAPMessage * requestMessage,
                                                             int underlyingPortId)
{
        (void) underlyingPortId; // Stop compiler barfs
        (void) requestMessage; // Stop compiler barfs

	rina::AccessGuard g(*lock_);

	if (state_ != FLOW_ALLOCATED) {
		LOG_ERR("Received deallocate request while not in FLOW_ALLOCATED state. Current state is: %d",
				state_);
		return;
	}

	//1 Update flow state
	flow_->set_state(Flow::WAITING_2_MPL_BEFORE_TEARING_DOWN);
	state_ = WAITING_2_MPL_BEFORE_TEARING_DOWN;

	//3 Wait 2*MPL before tearing down the flow
	TearDownFlowTimerTask * timerTask = new TearDownFlowTimerTask(this, object_name_, true);
	timer_->scheduleTask(timerTask, TearDownFlowTimerTask::DELAY);

	//4 Inform IPC Manager
	try {
		rina::extendedIPCManager->flowDeallocatedRemotely(port_id_, 0);
	} catch (Exception &e){
		LOG_ERR("Error communicating with the IPC Manager: %s", e.what());
	}
}

void FlowAllocatorInstance::destroyFlowAllocatorInstance(const std::string& flowObjectName,
                                                         bool requestor)
{
        (void) flowObjectName; // Stop compiler barfs
        (void) requestor; // Stop compiler barfs

	lock_->lock();

	if (state_ != WAITING_2_MPL_BEFORE_TEARING_DOWN) {
		LOG_ERR("Invoked destroy flow allocator instance while not in WAITING_2_MPL_BEFORE_TEARING_DOWN. State: %d",
				state_);
		lock_->unlock();
		return;
	}

	try {
		rib_daemon_->deleteObject(Flow::FLOW_RIB_OBJECT_CLASS, object_name_, 0, 0);
	} catch (Exception &e){
		LOG_ERR("Problems deleting object from RIB: %s", e.what());
	}

	releaseUnlockRemove();
}

void FlowAllocatorInstance::createResponse(const rina::CDAPMessage * cdapMessage,
                                           rina::CDAPSessionDescriptor * cdapSessionDescriptor)
{
        (void) cdapSessionDescriptor; // Stop compiler barfs

	lock_->lock();

	if (state_ != MESSAGE_TO_PEER_FAI_SENT) {
		LOG_ERR("Received CDAP Message while not in MESSAGE_TO_PEER_FAI_SENT state. Current state is: %d",
				state_);
		lock_->unlock();
		return;
	}

	if (cdapMessage->get_obj_name().compare(request_message_->get_obj_name()) != 0){
		LOG_ERR("Expected create flow response message for flow %s, but received create flow response message for flow %s ",
				request_message_->get_obj_name().c_str(), cdapMessage->get_obj_name().c_str());
		lock_->unlock();
		return;
	}

	//Flow allocation unsuccessful
	if (cdapMessage->get_result() != 0){
		LOG_DBG("Unsuccessful create flow response message received for flow %s",
				cdapMessage->get_obj_name().c_str());

		//Answer IPC Manager
		try {
			flow_request_event_.setPortId(-1);
			rina::extendedIPCManager->allocateFlowRequestResult(flow_request_event_,
					cdapMessage->get_result());
		} catch(Exception &e) {
			LOG_ERR("Problems communicating with the IPC Manager: %s", e.what());
		}

		releaseUnlockRemove();

		return;
	}

	//Flow allocation successful
	//Update the EFCP connection with the destination cep-id
	try {
		if (cdapMessage->get_obj_value()) {
			rina::ByteArrayObjectValue * value = (rina::ByteArrayObjectValue*)  cdapMessage->get_obj_value();
			Flow * receivedFlow = (Flow *) encoder_->decode((char*) value->get_value());
			flow_->set_destination_port_id(receivedFlow->get_destination_port_id());
			flow_->getActiveConnection()->setDestCepId(receivedFlow->getActiveConnection()->getDestCepId());

			delete receivedFlow;
		}
		state_ = CONNECTION_UPDATE_REQUESTED;
		rina::kernelIPCProcess->updateConnection(*(flow_->getActiveConnection()));
		lock_->unlock();
	} catch(Exception &e) {
		LOG_ERR("Problems requesting kernel to update connection: %s", e.what());

		//Answer IPC Manager
		try {
			flow_request_event_.setPortId(-1);
			rina::extendedIPCManager->allocateFlowRequestResult(flow_request_event_,
					cdapMessage->get_result());
		} catch(Exception &e) {
			LOG_ERR("Problems communicating with the IPC Manager: %s", e.what());
		}

		releaseUnlockRemove();
		return;
	}
}

//CLASS TEARDOWNFLOW TIMERTASK
const long TearDownFlowTimerTask::DELAY = 5000;

TearDownFlowTimerTask::TearDownFlowTimerTask(FlowAllocatorInstance * flow_allocator_instance,
const std::string& flow_object_name, bool requestor) {
	flow_allocator_instance_ = flow_allocator_instance;
	flow_object_name_ = flow_object_name;
	requestor_ = requestor;
}

void TearDownFlowTimerTask::run() {
	flow_allocator_instance_->destroyFlowAllocatorInstance(flow_object_name_, requestor_);
}

}
