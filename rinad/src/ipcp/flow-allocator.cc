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

#include <climits>
#include <sstream>
#include <vector>

#define RINA_PREFIX "flow-allocator"

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <librina/logs.h>
#include "flow-allocator.h"

namespace rinad {

//	CLASS Flow
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

	for (iterator = connections_.begin(); iterator != connections_.end();
			++iterator) {
		delete *iterator;
		*iterator = 0;
	}
}

rina::Connection * Flow::getActiveConnection() {
	rina::Connection result;
	std::list<rina::Connection*>::iterator iterator;

	unsigned int i = 0;
	for (iterator = connections_.begin(); iterator != connections_.end();
			++iterator) {
		if (i == current_connection_index_) {
			return *iterator;
		} else {
			i++;
		}
	}

	throw Exception("No active connection is currently defined");
}

std::string Flow::toString() {
	std::stringstream ss;
	ss << "* State: " << state_ << std::endl;
	ss << "* Is this IPC Process the requestor of the flow? " << source_
			<< std::endl;
	ss << "* Max create flow retries: " << max_create_flow_retries_
			<< std::endl;
	ss << "* Hop count: " << hop_count_ << std::endl;
	ss << "* Source AP Naming Info: " << source_naming_info_.toString()
			<< std::endl;
	;
	ss << "* Source address: " << source_address_ << std::endl;
	ss << "* Source port id: " << source_port_id_ << std::endl;
	ss << "* Destination AP Naming Info: "
			<< destination_naming_info_.toString();
	ss << "* Destination addres: " + destination_address_ << std::endl;
	ss << "* Destination port id: " + destination_port_id_ << std::endl;
	if (connections_.size() > 0) {
		ss << "* Connection ids of the connection supporting this flow: +\n";
		for (std::list<rina::Connection*>::const_iterator iterator =
				connections_.begin(), end = connections_.end(); iterator != end;
				++iterator) {
			ss << "Src CEP-id " << (*iterator)->getSourceCepId()
					<< "; Dest CEP-id " << (*iterator)->getDestCepId()
					<< "; Qos-id " << (*iterator)->getQosId() << std::endl;
		}
	}
	ss << "* Index of the current active connection for this flow: "
			<< current_connection_index_ << std::endl;
	return ss.str();
}

//Class Flow RIB Object
FlowRIBObject::FlowRIBObject(IPCProcess * ipc_process,
		const std::string& object_name, const std::string& object_class,
		IFlowAllocatorInstance * flow_allocator_instance) :
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
FlowSetRIBObject::FlowSetRIBObject(IPCProcess * ipc_process,
		IFlowAllocator * flow_allocator) :
		BaseRIBObject(ipc_process, EncoderConstants::FLOW_SET_RIB_OBJECT_CLASS,
				objectInstanceGenerator->getObjectInstance(),
				EncoderConstants::FLOW_SET_RIB_OBJECT_NAME) {
	flow_allocator_ = flow_allocator;
}

void FlowSetRIBObject::remoteCreateObject(const rina::CDAPMessage * cdapMessage,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	flow_allocator_->createFlowRequestMessageReceived(cdapMessage,
			cdapSessionDescriptor->get_port_id());
}

void FlowSetRIBObject::createObject(const std::string& objectClass,
		const std::string& objectName, IFlowAllocatorInstance* objectValue) {
	FlowRIBObject * flowRIBObject;

	flowRIBObject = new FlowRIBObject(ipc_process_, objectClass, objectName, objectValue);
	add_child(flowRIBObject);
	rib_daemon_->addRIBObject(flowRIBObject);
}

const void* FlowSetRIBObject::get_value() const {
	return flow_allocator_;
}

//Class QoS Cube Set RIB Object
QoSCubeSetRIBObject::QoSCubeSetRIBObject(IPCProcess * ipc_process) :
		BaseRIBObject(ipc_process,
				EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_CLASS,
				objectInstanceGenerator->getObjectInstance(),
				EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_NAME) {
}

void QoSCubeSetRIBObject::remoteCreateObject(
		const rina::CDAPMessage * cdapMessage,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	//TODO, depending on IEncoder
	LOG_ERR("Missing code %d, %d",
			cdapMessage->get_op_code(), cdapSessionDescriptor->get_port_id());
}

void QoSCubeSetRIBObject::createObject(const std::string& objectClass,
		const std::string& objectName, rina::QoSCube* objectValue) {
	SimpleSetMemberRIBObject * ribObject = new SimpleSetMemberRIBObject(ipc_process_,
			objectClass, objectName, objectValue);
	add_child(ribObject);
	rib_daemon_->addRIBObject(ribObject);
	//TODO: the QoS cube should be added into the configuration
}

void QoSCubeSetRIBObject::deleteObject(const void* objectValue) {
	if (objectValue) {
		LOG_WARN("Object value should have been NULL");
	}

	std::list<std::string> childNames;
	std::list<BaseRIBObject*>::const_iterator childrenIt;
	std::list<std::string>::const_iterator namesIt;

	for (childrenIt = get_children().begin();
			childrenIt != get_children().end(); ++childrenIt) {
		childNames.push_back((*childrenIt)->name_);
	}

	for (namesIt = childNames.begin(); namesIt != childNames.end(); ++namesIt) {
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

void FlowAllocator::createFlowRequestMessageReceived(
		const rina::CDAPMessage * cdapMessage, int underlyingPortId) {
	Flow * flow;
	IFlowAllocatorInstance * flowAllocatorInstance;
	unsigned int myAddress = 0;
	int portId = 0;

	try {
		rina::ByteArrayObjectValue * value =
				(rina::ByteArrayObjectValue*) cdapMessage->get_obj_value();
		rina::SerializedObject * serializedObject =
				(rina::SerializedObject *) value->get_value();

		flow = (Flow *) encoder_->decode(*serializedObject,
				EncoderConstants::FLOW_RIB_OBJECT_CLASS);
	} catch (Exception &e) {
		LOG_ERR("Problems decoding object value: %s", e.what());
		return;
	}

	unsigned int address = namespace_manager_->getDFTNextHop(flow->destination_naming_info_);
	myAddress = ipc_process_->get_address();
	if (address == 0){
		LOG_ERR("The directory forwarding table returned no entries when looking up %s",
				flow->destination_naming_info_.toString().c_str());
		return;
	}

	if (address == myAddress) {
		//There is an entry and the address is this IPC Process, create a FAI, extract the Flow
		//object from the CDAP message and call the FAI
		try {
			portId = rina::extendedIPCManager->allocatePortId(flow->destination_naming_info_);
		}catch (Exception &e) {
			LOG_ERR("Problems requesting an available port-id: %s. Ignoring the Flow allocation request",
					e.what());
			return;
		}

		LOG_DBG(
				"The destination application process is reachable through me. Assigning the local port-id %d to the flow",
				portId);
		flowAllocatorInstance = new FlowAllocatorInstance(ipc_process_, this,
				cdap_session_manager_, portId);
		flow_allocator_instances_.put(portId, flowAllocatorInstance);

		//TODO check if this operation throws an exception an react accordingly
		flowAllocatorInstance->createFlowRequestMessageReceived(flow,
				cdapMessage, underlyingPortId);
		return;
	}

	//The address is not this IPC process, forward the CDAP message to that address increment the hop
	//count of the Flow object extract the flow object from the CDAP message
	flow->hop_count_ = flow->hop_count_ -1;
	if (flow->hop_count_ <= 0) {
		//TODO send negative create Flow response CDAP message to the source IPC process, specifying
		//that the application process could not be found before the hop count expired
		LOG_ERR("Missing code");
	}

	LOG_ERR("Missing code");
}

void FlowAllocator::replyToIPCManager(const rina::FlowRequestEvent& event,
		int result) {
	try {
		rina::extendedIPCManager->allocateFlowRequestResult(event, result);
	} catch (Exception &e) {
		LOG_ERR("Problems communicating with the IPC Manager Daemon: %s",
				e.what());
	}
}

void FlowAllocator::submitAllocateRequest(rina::FlowRequestEvent * event) {
	int portId = 0;
	IFlowAllocatorInstance * flowAllocatorInstance;

	try {
		portId = rina::extendedIPCManager->allocatePortId(
				event->getLocalApplicationName());
		LOG_DBG("Got assigned port-id %d", portId);
	} catch (Exception &e) {
		LOG_ERR(
				"Problems requesting an available port-id to the Kernel IPC Manager: %s",
				e.what());
		replyToIPCManager(*event, -1);
	}

	event->setPortId(portId);
	flowAllocatorInstance = new FlowAllocatorInstance(ipc_process_, this,
			cdap_session_manager_, portId);
	flow_allocator_instances_.put(portId, flowAllocatorInstance);

	try {
		flowAllocatorInstance->submitAllocateRequest(*event);
	} catch (Exception &e) {
		LOG_ERR("Problems allocating flow: %s", e.what());
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

void FlowAllocator::processCreateConnectionResponseEvent(
		const rina::CreateConnectionResponseEvent& event) {
	IFlowAllocatorInstance * flowAllocatorInstance;

	flowAllocatorInstance = flow_allocator_instances_.find(event.getPortId());
	if (flowAllocatorInstance) {
		flowAllocatorInstance->processCreateConnectionResponseEvent(event);
	} else {
		LOG_ERR(
				"Received create connection response event associated to unknown port-id %d",
				event.getPortId());
	}
}

void FlowAllocator::submitAllocateResponse(
		const rina::AllocateFlowResponseEvent& event) {
	IFlowAllocatorInstance * flowAllocatorInstance;

	LOG_DBG(
			"Local application invoked allocate response with seq num %ud and result %d, ",
			event.getSequenceNumber(), event.getResult());

	std::list<IFlowAllocatorInstance *> fais =
			flow_allocator_instances_.getEntries();
	std::list<IFlowAllocatorInstance *>::iterator iterator;
	for (iterator = fais.begin(); iterator != fais.end(); ++iterator) {
		if ((*iterator)->get_allocate_response_message_handle()
				== event.getSequenceNumber()) {
			flowAllocatorInstance = *iterator;
			flowAllocatorInstance->submitAllocateResponse(event);
			return;
		}
	}

	LOG_ERR("Could not find FAI with handle %ud", event.getSequenceNumber());
}

void FlowAllocator::processCreateConnectionResultEvent(
		const rina::CreateConnectionResultEvent& event) {
	IFlowAllocatorInstance * flowAllocatorInstance;

	flowAllocatorInstance = flow_allocator_instances_.find(event.getPortId());
	if (!flowAllocatorInstance) {
		LOG_ERR("Problems looking for FAI at portId %d", event.getPortId());
		try {
			rina::extendedIPCManager->deallocatePortId(event.getPortId());
		} catch (Exception &e) {
			LOG_ERR(
					"Problems requesting IPC Manager to deallocate port-id %d: %s",
					event.getPortId(), e.what());
		}
	} else {
		flowAllocatorInstance->processCreateConnectionResultEvent(event);
	}
}

void FlowAllocator::processUpdateConnectionResponseEvent(
		const rina::UpdateConnectionResponseEvent& event) {
	IFlowAllocatorInstance * flowAllocatorInstance;

	flowAllocatorInstance = flow_allocator_instances_.find(event.getPortId());
	if (!flowAllocatorInstance) {
		LOG_ERR("Problems looking for FAI at portId %d", event.getPortId());
		try {
			rina::extendedIPCManager->deallocatePortId(event.getPortId());
		} catch (Exception &e) {
			LOG_ERR(
					"Problems requesting IPC Manager to deallocate port-id %d: %s",
					event.getPortId(), e.what());
		}
	} else {
		flowAllocatorInstance->processUpdateConnectionResponseEvent(event);
	}
}

void FlowAllocator::submitDeallocate(
		const rina::FlowDeallocateRequestEvent& event) {
	IFlowAllocatorInstance * flowAllocatorInstance;

	flowAllocatorInstance = flow_allocator_instances_.find(event.getPortId());
	if (!flowAllocatorInstance) {
		LOG_ERR("Problems looking for FAI at portId %d", event.getPortId());
		try {
			rina::extendedIPCManager->deallocatePortId(event.getPortId());
		} catch (Exception &e) {
			LOG_ERR(
					"Problems requesting IPC Manager to deallocate port-id %d: %s",
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
Flow * SimpleNewFlowRequestPolicy::generateFlowObject(
		const rina::FlowRequestEvent& event,
		const std::list<rina::QoSCube>& qosCubes) {
	Flow* flow;

	flow = new Flow();
	flow->destination_naming_info_ = event.getRemoteApplicationName();
	flow->source_naming_info_ = event.getLocalApplicationName();
	flow->hop_count_ = 3;
	flow->max_create_flow_retries_ = 1;
	flow->source_ = true;
	flow->state_ = Flow::ALLOCATION_IN_PROGRESS;

	std::list<rina::Connection*> connections;
	rina::QoSCube qosCube = selectQoSCube(event.getFlowSpecification(),
			qosCubes);
	LOG_DBG("Selected qos cube with name %s and policies: %s",
			qosCube.get_name().c_str());

	rina::Connection * connection = new rina::Connection();
	connection->setQosId(1);
	connection->setFlowUserIpcProcessId(event.getFlowRequestorIPCProcessId());
	rina::ConnectionPolicies connectionPolicies = rina::ConnectionPolicies(
			qosCube.get_efcp_policies());
	connectionPolicies.set_in_order_delivery(qosCube.is_ordered_delivery());
	connectionPolicies.set_partial_delivery(qosCube.is_partial_delivery());
	if (qosCube.get_max_allowable_gap() < 0) {
		connectionPolicies.set_max_sdu_gap(INT_MAX);
	} else {
		connectionPolicies.set_max_sdu_gap(qosCube.get_max_allowable_gap());
	}
	connection->setPolicies(connectionPolicies);
	connections.push_back(connection);

	flow->connections_ = connections;
	flow->current_connection_index_ = 0;
	flow->flow_specification_ = event.getFlowSpecification();

	return flow;
}

rina::QoSCube SimpleNewFlowRequestPolicy::selectQoSCube(
		const rina::FlowSpecification& flowSpec,
		const std::list<rina::QoSCube>& qosCubes) {
	if (flowSpec.getMaxAllowableGap() < 0) {
		return qosCubes.front();
	}

	std::list<rina::QoSCube>::const_iterator iterator;
	rina::QoSCube cube;
	for (iterator = qosCubes.begin(); iterator != qosCubes.end(); ++iterator) {
		cube = *iterator;
		if (cube.get_efcp_policies().is_dtcp_present()) {
			if (flowSpec.getMaxAllowableGap() >= 0 &&
					cube.get_efcp_policies().get_dtcp_configuration().is_rtx_control()) {
				return cube;
			}
		}
	}

	throw Exception("Could not find a QoS Cube with Rtx control disabled!");
}

//Class Flow Allocator Instance
FlowAllocatorInstance::FlowAllocatorInstance(IPCProcess * ipc_process,
		IFlowAllocator * flow_allocator,
		rina::CDAPSessionManagerInterface * cdap_session_manager, int port_id) {
	initialize(ipc_process, flow_allocator, port_id);
	cdap_session_manager_ = cdap_session_manager;
	new_flow_request_policy_ = new SimpleNewFlowRequestPolicy();
	LOG_DBG(
			"Created flow allocator instance to manage the flow identified by portId %d ",
			port_id);
}

FlowAllocatorInstance::FlowAllocatorInstance(IPCProcess * ipc_process,
		IFlowAllocator * flow_allocator, int port_id) {
	initialize(ipc_process, flow_allocator, port_id);
	new_flow_request_policy_ = 0;
	LOG_DBG(
			"Created flow allocator instance to manage the flow identified by portId %d ",
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

unsigned int FlowAllocatorInstance::get_allocate_response_message_handle() const {
	unsigned int t;

	{
		rina::AccessGuard g(*lock_);
		t = allocate_response_message_handle_;
	}

	return t;
}

void FlowAllocatorInstance::set_allocate_response_message_handle(
		unsigned int allocate_response_message_handle) {
	rina::AccessGuard g(*lock_);
	allocate_response_message_handle_ = allocate_response_message_handle;
}

void FlowAllocatorInstance::submitAllocateRequest(
		const rina::FlowRequestEvent& event) {
	rina::AccessGuard g(*lock_);

	flow_request_event_ = rina::FlowRequestEvent(event);
	flow_ =
			new_flow_request_policy_->generateFlowObject(event,
					ipc_process_->get_dif_information().get_dif_configuration().get_efcp_configuration().get_qos_cubes());

	LOG_DBG("Generated flow object");

	//1 Check directory to see to what IPC process the CDAP M_CREATE request has to be delivered
	unsigned int destinationAddress = namespace_manager_->getDFTNextHop(
			event.getRemoteApplicationName());
	LOG_DBG("The directory forwarding table returned address %ud",
                destinationAddress);
	flow_->destination_address_ = destinationAddress;
	if (destinationAddress == 0){
		std::stringstream ss;
		ss << "Could not find entry in DFT for application ";
		ss << event.getRemoteApplicationName().toString();
		throw Exception(ss.str().c_str());
	}

	//2 Check if the destination address is this IPC process (then invoke degenerated form of IPC)
	unsigned int sourceAddress = ipc_process_->get_address();
	flow_->source_address_ = sourceAddress;
	flow_->source_port_id_ = port_id_;
	std::stringstream ss;
	ss << EncoderConstants::FLOW_SET_RIB_OBJECT_NAME;
	ss << EncoderConstants::SEPARATOR << sourceAddress << "-" << port_id_;
	object_name_ = ss.str();
	if (destinationAddress == sourceAddress) {
		// At the moment we don't support allocation of flows between applications at the
		// same processing system
		throw Exception(
				"Allocation of flows between local applications not supported yet");
	}

	//3 Request the creation of the connection(s) in the Kernel
	state_ = CONNECTION_CREATE_REQUESTED;
	rina::kernelIPCProcess->createConnection(*(flow_->getActiveConnection()));
	LOG_DBG(
			"Requested the creation of a connection to the kernel, for flow with port-id %d",
			port_id_);
}

void FlowAllocatorInstance::replyToIPCManager(rina::FlowRequestEvent & event,
		int result) {
	try {
		rina::extendedIPCManager->allocateFlowRequestResult(event, result);
	} catch (Exception &e) {
		LOG_ERR("Problems communicating with the IPC Manager Daemon: %s",
				e.what());
	}
}

void FlowAllocatorInstance::releasePortId() {
	try {
		rina::extendedIPCManager->deallocatePortId(port_id_);
	} catch (Exception &e) {
		LOG_ERR("Problems releasing port-id %d", port_id_);
	}
}

void FlowAllocatorInstance::releaseUnlockRemove() {
	releasePortId();
	lock_->unlock();
	flow_allocator_->removeFlowAllocatorInstance(port_id_);
}

void FlowAllocatorInstance::processCreateConnectionResponseEvent(
		const rina::CreateConnectionResponseEvent& event) {
	lock_->lock();

	if (state_ != CONNECTION_CREATE_REQUESTED) {
		LOG_ERR(
				"Received a process Create Connection Response Event while in %d state. Ignoring it",
				state_);
		lock_->unlock();
		return;
	}

	if (event.getCepId() < 0) {
		LOG_ERR(
				"The EFCP component of the IPC Process could not create a connection instance: %d",
				event.getCepId());
		replyToIPCManager(flow_request_event_, -1);
		lock_->unlock();
		return;
	}

	LOG_DBG("Created connection with cep-id %d", event.getCepId());
	flow_->getActiveConnection()->setSourceCepId(event.getCepId());

	const rina::CDAPMessage * cdapMessage = 0;
	const rina::SerializedObject * serializedObject = 0;
	try {
		//5 get the portId of any open CDAP session
		std::vector<int> cdapSessions;
		cdap_session_manager_->getAllCDAPSessionIds(cdapSessions);
		int cdapSessionId = cdapSessions[0];

		//6 Encode the flow object and send it to the destination IPC process
		serializedObject = encoder_->encode(flow_,
				EncoderConstants::FLOW_RIB_OBJECT_CLASS);
		rina::ByteArrayObjectValue objectValue = rina::ByteArrayObjectValue(
				*serializedObject);
		cdapMessage = cdap_session_manager_->getCreateObjectRequestMessage(
				cdapSessionId, 0, rina::CDAPMessage::NONE_FLAGS,
				EncoderConstants::FLOW_RIB_OBJECT_CLASS, 0, object_name_,
				&objectValue, 0, true);

		underlying_port_id_ = cdapSessionId;
		request_message_ = cdapMessage;
		state_ = MESSAGE_TO_PEER_FAI_SENT;

		rib_daemon_->sendMessageToAddress(*request_message_, cdapSessionId, flow_->destination_address_, this);
		delete cdapMessage;
		delete serializedObject;
	} catch (Exception &e) {
		LOG_ERR("Problems sending M_CREATE <Flow> CDAP message to neighbor: %s",
				e.what());
		delete cdapMessage;
		delete serializedObject;
		replyToIPCManager(flow_request_event_, -1);
		releaseUnlockRemove();
		return;
	}

	lock_->unlock();
}

void FlowAllocatorInstance::createFlowRequestMessageReceived(Flow * flow,
		const rina::CDAPMessage * requestMessage, int underlyingPortId) {
	lock_->lock();

	LOG_DBG("Create flow request received: %s", flow->toString().c_str());
	flow_ = flow;
	if (flow_->destination_address_ == 0) {
		flow_->destination_address_ = ipc_process_->get_address();
	}
	request_message_ = requestMessage;
	underlying_port_id_ = underlyingPortId;
	flow_->destination_port_id_ = port_id_;

	//1 Reverse connection source/dest addresses and CEP-ids
	rina::Connection * connection = flow_->getActiveConnection();
	connection->setPortId(port_id_);
	unsigned int aux = connection->getSourceAddress();
	connection->setSourceAddress(connection->getDestAddress());
	connection->setDestAddress(aux);
	connection->setDestCepId(connection->getSourceCepId());
	connection->setFlowUserIpcProcessId(namespace_manager_->getRegIPCProcessId(flow_->destination_naming_info_));
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
		LOG_DBG(
				"Requested the creation of a connection to the kernel to support flow with port-id %d",
				port_id_);
	} catch (Exception &e) {
		LOG_ERR("Problems requesting a connection to the kernel: %s ",
				e.what());
		releaseUnlockRemove();
		return;
	}

	lock_->unlock();
}

void FlowAllocatorInstance::processCreateConnectionResultEvent(
		const rina::CreateConnectionResultEvent& event) {
	lock_->lock();

	if (state_ != CONNECTION_CREATE_REQUESTED) {
		LOG_ERR(
				"Received an allocate response event while not in APP_NOTIFIED_OF_INCOMING_FLOW state. Current state: %d",
				state_);
		lock_->unlock();
		return;
	}

	if (event.getSourceCepId() < 0) {
		LOG_ERR("Create connection operation was unsuccessful: %d",
				event.getSourceCepId());
		releaseUnlockRemove();
		return;
	}

	try {
		state_ = APP_NOTIFIED_OF_INCOMING_FLOW;
		allocate_response_message_handle_  = rina::extendedIPCManager->allocateFlowRequestArrived(flow_->destination_naming_info_,
				flow_->source_naming_info_, flow_->flow_specification_, port_id_);
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

void FlowAllocatorInstance::submitAllocateResponse(
		const rina::AllocateFlowResponseEvent& event) {
	lock_->lock();

	if (state_ != APP_NOTIFIED_OF_INCOMING_FLOW) {
		LOG_ERR(
				"Received an allocate response event while not in APP_NOTIFIED_OF_INCOMING_FLOW state. Current state: %d",
				state_);
		lock_->unlock();
		return;
	}

	const rina::CDAPMessage * cdapMessage = 0;
	const rina::SerializedObject * serializedObject = 0;
	if (event.getResult() == 0) {
		//Flow has been accepted
		try {
			serializedObject = encoder_->encode(flow_,
					EncoderConstants::FLOW_RIB_OBJECT_CLASS);
			rina::ByteArrayObjectValue objectValue = rina::ByteArrayObjectValue(
					*serializedObject);
			cdapMessage = cdap_session_manager_->getCreateObjectResponseMessage(
					rina::CDAPMessage::NONE_FLAGS,
					request_message_->get_obj_class(), 0,
					request_message_->get_obj_name(), &objectValue, 0, 0,
					request_message_->get_invoke_id());

			rib_daemon_->sendMessageToAddress(*cdapMessage, underlying_port_id_,
					flow_->source_address_, 0);
			delete cdapMessage;
			delete serializedObject;
		} catch (Exception &e) {
			LOG_ERR("Problems requesting RIB Daemon to send CDAP Message: %s",
					e.what());
			delete cdapMessage;
			delete serializedObject;

			try {
				rina::extendedIPCManager->flowDeallocated(port_id_);
			} catch (Exception &e) {
				LOG_ERR("Problems communicating with the IPC Manager: %s",
						e.what());
			}

			releaseUnlockRemove();
			return;
		}

		try {
			flow_->state_ = Flow::ALLOCATED;
			rib_daemon_->createObject(EncoderConstants::FLOW_RIB_OBJECT_CLASS, object_name_, this, 0);
		} catch(Exception &e) {
			LOG_WARN("Error creating Flow Rib object: %s", e.what());
		}

		state_ = FLOW_ALLOCATED;
		lock_->unlock();
		return;
	}

	//Flow has been rejected
	try {
		serializedObject = encoder_->encode(flow_,
				EncoderConstants::FLOW_RIB_OBJECT_CLASS);
		rina::ByteArrayObjectValue objectValue = rina::ByteArrayObjectValue(
				*serializedObject);
		cdapMessage = cdap_session_manager_->getCreateObjectResponseMessage(
				rina::CDAPMessage::NONE_FLAGS,
				request_message_->get_obj_class(), 0,
				request_message_->get_obj_name(), &objectValue, -1,
				"Application rejected the flow",
				request_message_->get_invoke_id());

		rib_daemon_->sendMessageToAddress(*cdapMessage, underlying_port_id_,
				flow_->source_address_, 0);
		delete cdapMessage;
		delete serializedObject;
		cdapMessage = 0;
	} catch (Exception &e) {
		LOG_ERR("Problems requesting RIB Daemon to send CDAP Message: %s",
				e.what());
		delete cdapMessage;
		delete serializedObject;
	}

	releaseUnlockRemove();
}

void FlowAllocatorInstance::processUpdateConnectionResponseEvent(
		const rina::UpdateConnectionResponseEvent& event) {
	lock_->lock();

	if (state_ != CONNECTION_UPDATE_REQUESTED) {
		LOG_ERR(
				"Received CDAP Message while not in CONNECTION_UPDATE_REQUESTED state. Current state is: %d",
				state_);
		lock_->unlock();
		return;
	}

	//Update connection was unsuccessful
	if (event.getResult() != 0) {
		LOG_ERR("The kernel denied the update of a connection: %d",
				event.getResult());

		try {
			flow_request_event_.setPortId(-1);
			rina::extendedIPCManager->allocateFlowRequestResult(
					flow_request_event_, event.getResult());
		} catch (Exception &e) {
			LOG_ERR("Problems communicating with the IPC Manager: %s",
					e.what());
		}

		releaseUnlockRemove();
		return;
	}

	//Update connection was successful
	try {
		flow_->state_ = Flow::ALLOCATED;
		rib_daemon_->createObject(EncoderConstants::FLOW_RIB_OBJECT_CLASS, object_name_, this, 0);
	} catch(Exception &e) {
		LOG_WARN("Problems requesting the RIB Daemon to create a RIB object: %s", e.what());
	}

	state_ = FLOW_ALLOCATED;

	try {
		flow_request_event_.setPortId(port_id_);
		rina::extendedIPCManager->allocateFlowRequestResult(flow_request_event_,
				0);
	} catch (Exception &e) {
		LOG_ERR("Problems communicating with the IPC Manager: %s", e.what());
	}

	lock_->unlock();
}

void FlowAllocatorInstance::submitDeallocate(
		const rina::FlowDeallocateRequestEvent& event) {
	rina::AccessGuard g(*lock_);

	(void) event; // Stop compiler barfs

	if (state_ != FLOW_ALLOCATED) {
		LOG_ERR(
				"Received deallocate request while not in FLOW_ALLOCATED state. Current state is: %d",
				state_);
		return;
	}

	try {
		//1 Update flow state
		flow_->state_ = Flow::WAITING_2_MPL_BEFORE_TEARING_DOWN;
		state_ = WAITING_2_MPL_BEFORE_TEARING_DOWN;

		//2 Send M_DELETE
		const rina::CDAPMessage * cdapMessage = 0;
		const rina::SerializedObject * serializedObject = 0;
		try {
			serializedObject = encoder_->encode(flow_,
					EncoderConstants::FLOW_RIB_OBJECT_CLASS);
			rina::ByteArrayObjectValue objectValue = rina::ByteArrayObjectValue(
					*serializedObject);
			cdapMessage = cdap_session_manager_->getDeleteObjectRequestMessage(
					underlying_port_id_, 0, rina::CDAPMessage::NONE_FLAGS,
					EncoderConstants::FLOW_RIB_OBJECT_CLASS, 0, object_name_,
					&objectValue, 0, false);

			unsigned int address = 0;
			if (ipc_process_->get_address() == flow_->source_address_) {
				address = flow_->destination_address_;
			} else {
				address = flow_->source_address_;
			}

			rib_daemon_->sendMessageToAddress(*cdapMessage, underlying_port_id_,
					address, 0);
			delete cdapMessage;
			delete serializedObject;
		} catch (Exception &e) {
			LOG_ERR("Problems sending M_DELETE flow request: %s", e.what());
			delete cdapMessage;
			delete serializedObject;
		}

		//3 Wait 2*MPL before tearing down the flow
		TearDownFlowTimerTask * timerTask = new TearDownFlowTimerTask(this,
				object_name_, true);
		timer_->scheduleTask(timerTask, TearDownFlowTimerTask::DELAY);
	} catch (Exception &e) {
		LOG_ERR("Problems processing flow deallocation request: %s", +e.what());
	}

}

void FlowAllocatorInstance::deleteFlowRequestMessageReceived(
		const rina::CDAPMessage * requestMessage, int underlyingPortId) {
	(void) underlyingPortId; // Stop compiler barfs
	(void) requestMessage; // Stop compiler barfs

	rina::AccessGuard g(*lock_);

	if (state_ != FLOW_ALLOCATED) {
		LOG_ERR(
				"Received deallocate request while not in FLOW_ALLOCATED state. Current state is: %d",
				state_);
		return;
	}

	//1 Update flow state
	flow_->state_ = Flow::WAITING_2_MPL_BEFORE_TEARING_DOWN;
	state_ = WAITING_2_MPL_BEFORE_TEARING_DOWN;

	//3 Wait 2*MPL before tearing down the flow
	TearDownFlowTimerTask * timerTask = new TearDownFlowTimerTask(this,
			object_name_, true);
	timer_->scheduleTask(timerTask, TearDownFlowTimerTask::DELAY);

	//4 Inform IPC Manager
	try {
		rina::extendedIPCManager->flowDeallocatedRemotely(port_id_, 0);
	} catch (Exception &e) {
		LOG_ERR("Error communicating with the IPC Manager: %s", e.what());
	}
}

void FlowAllocatorInstance::destroyFlowAllocatorInstance(
		const std::string& flowObjectName, bool requestor) {
	(void) flowObjectName; // Stop compiler barfs
	(void) requestor; // Stop compiler barfs

	lock_->lock();

	if (state_ != WAITING_2_MPL_BEFORE_TEARING_DOWN) {
		LOG_ERR(
				"Invoked destroy flow allocator instance while not in WAITING_2_MPL_BEFORE_TEARING_DOWN. State: %d",
				state_);
		lock_->unlock();
		return;
	}

	try {
		rib_daemon_->deleteObject(EncoderConstants::FLOW_RIB_OBJECT_CLASS,
				object_name_, 0, 0);
	} catch (Exception &e) {
		LOG_ERR("Problems deleting object from RIB: %s", e.what());
	}

	releaseUnlockRemove();
}

void FlowAllocatorInstance::createResponse(
		const rina::CDAPMessage * cdapMessage,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	(void) cdapSessionDescriptor; // Stop compiler barfs

	lock_->lock();

	if (state_ != MESSAGE_TO_PEER_FAI_SENT) {
		LOG_ERR(
				"Received CDAP Message while not in MESSAGE_TO_PEER_FAI_SENT state. Current state is: %d",
				state_);
		lock_->unlock();
		return;
	}

	if (cdapMessage->get_obj_name().compare(request_message_->get_obj_name())
			!= 0) {
		LOG_ERR(
				"Expected create flow response message for flow %s, but received create flow response message for flow %s ",
				request_message_->get_obj_name().c_str(), cdapMessage->get_obj_name().c_str());
		lock_->unlock();
		return;
	}

	//Flow allocation unsuccessful
	if (cdapMessage->get_result() != 0) {
		LOG_DBG(
				"Unsuccessful create flow response message received for flow %s",
				cdapMessage->get_obj_name().c_str());

		//Answer IPC Manager
		try {
			flow_request_event_.setPortId(-1);
			rina::extendedIPCManager->allocateFlowRequestResult(
					flow_request_event_, cdapMessage->get_result());
		} catch (Exception &e) {
			LOG_ERR("Problems communicating with the IPC Manager: %s",
					e.what());
		}

		releaseUnlockRemove();

		return;
	}

	//Flow allocation successful
	//Update the EFCP connection with the destination cep-id
	try {
		if (cdapMessage->get_obj_value()) {
			rina::ByteArrayObjectValue * value = (rina::ByteArrayObjectValue*)  cdapMessage->get_obj_value();
			rina::SerializedObject * serializedObject = (rina::SerializedObject *) value->get_value();
			Flow * receivedFlow = (Flow *) encoder_->decode(*serializedObject, EncoderConstants::FLOW_RIB_OBJECT_CLASS);
			flow_->destination_port_id_ = receivedFlow->destination_port_id_;
			flow_->getActiveConnection()->setDestCepId(receivedFlow->getActiveConnection()->getDestCepId());

			delete receivedFlow;
		}
		state_ = CONNECTION_UPDATE_REQUESTED;
		rina::kernelIPCProcess->updateConnection(
				*(flow_->getActiveConnection()));
		lock_->unlock();
	} catch (Exception &e) {
		LOG_ERR("Problems requesting kernel to update connection: %s",
				e.what());

		//Answer IPC Manager
		try {
			flow_request_event_.setPortId(-1);
			rina::extendedIPCManager->allocateFlowRequestResult(
					flow_request_event_, cdapMessage->get_result());
		} catch (Exception &e) {
			LOG_ERR("Problems communicating with the IPC Manager: %s",
					e.what());
		}

		releaseUnlockRemove();
		return;
	}
}

//CLASS TEARDOWNFLOW TIMERTASK
const long TearDownFlowTimerTask::DELAY = 5000;

TearDownFlowTimerTask::TearDownFlowTimerTask(
		FlowAllocatorInstance * flow_allocator_instance,
		const std::string& flow_object_name, bool requestor) {
	flow_allocator_instance_ = flow_allocator_instance;
	flow_object_name_ = flow_object_name;
	requestor_ = requestor;
}

void TearDownFlowTimerTask::run() {
	flow_allocator_instance_->destroyFlowAllocatorInstance(flow_object_name_,
			requestor_);
}


// CLASS FlowEncoder
const rina::SerializedObject* FlowEncoder::encode(const void* object) {
	Flow *flow = (Flow*) object;
	rina::messages::Flow gpf_flow;

	// SourceNamingInfo
	gpf_flow.set_allocated_sourcenaminginfo(
			get_applicationProcessNamingInfo_t(flow->source_naming_info_));
	// DestinationNamingInfo
	gpf_flow.set_allocated_destinationnaminginfo(
			get_applicationProcessNamingInfo_t(flow->destination_naming_info_));
	// sourcePortId
	gpf_flow.set_sourceportid(flow->source_port_id_);
	//destinationPortId
	gpf_flow.set_destinationportid(flow->destination_port_id_);
	//sourceAddress
	gpf_flow.set_sourceaddress(flow->source_address_);
	//destinationAddress
	gpf_flow.set_destinationaddress(flow->destination_address_);
	//connectionIds
	for (std::list<rina::Connection*>::const_iterator it =
			flow->connections_.begin();
			it != flow->connections_.end(); ++it) {
		rina::messages::connectionId_t *gpf_connection =
				gpf_flow.add_connectionids();
		//qosId
		gpf_connection->set_qosid((*it)->getQosId());
		//sourceCEPId
		gpf_connection->set_sourcecepid((*it)->getSourceCepId());
		//destinationCEPId
		gpf_connection->set_destinationcepid((*it)->getDestCepId());
	}
	//currentConnectionIdIndex
	gpf_flow.set_currentconnectionidindex(flow->current_connection_index_);
	//state
	gpf_flow.set_state(flow->state_);
	//qosParameters
	gpf_flow.set_allocated_qosparameters(get_qosSpecification_t(flow->flow_specification_));
	//optional connectionPolicies_t connectionPolicies
	gpf_flow.set_allocated_connectionpolicies(get_connectionPolicies_t(flow->getActiveConnection()->getPolicies()));
	//accessControl
	if (flow->access_control_ != 0)
		gpf_flow.set_accesscontrol(flow->access_control_);
	//maxCreateFlowRetries
	gpf_flow.set_maxcreateflowretries(flow->max_create_flow_retries_);
	//createFlowRetries
	gpf_flow.set_createflowretries(flow->create_flow_retries_);
	//hopCount
	gpf_flow.set_hopcount(flow->hop_count_);

	int size = gpf_flow.ByteSize();
	char *serialized_message = new char[size];
	gpf_flow.SerializeToArray(serialized_message, size);
	rina::SerializedObject *serialized_object =  new rina::SerializedObject(serialized_message,size);

	return serialized_object;
}

void* FlowEncoder::decode(
const rina::SerializedObject &serialized_object) const {
	Flow *flow = new Flow();
	rina::messages::Flow gpf_flow;

	gpf_flow.ParseFromArray(serialized_object.message_, serialized_object.size_);

	// SourceNamingInfo
	rina::ApplicationProcessNamingInformation *src_app = get_ApplicationProcessNamingInformation(gpf_flow.sourcenaminginfo());
	flow->source_naming_info_ = *src_app;
	delete src_app;
	src_app = 0;
	// DestinationNamingInfo
	rina::ApplicationProcessNamingInformation *dest_app = get_ApplicationProcessNamingInformation(gpf_flow.destinationnaminginfo());
	flow->destination_naming_info_ = *dest_app;
	delete dest_app;
	dest_app = 0;
	// sourcePortId
	flow->source_port_id_ = gpf_flow.sourceportid();
	//destinationPortId
	flow->destination_port_id_ = gpf_flow.destinationportid();
	//sourceAddress
	flow->source_address_ = gpf_flow.sourceaddress();
	//destinationAddress
	flow->destination_address_ = gpf_flow.destinationaddress();
	//connectionIds
	for (int i = 0; i < gpf_flow.connectionids_size(); ++i)
		flow->connections_.push_back(get_Connection(gpf_flow.connectionids(i)));
	//currentConnectionIdIndex
	flow->current_connection_index_ = gpf_flow.currentconnectionidindex();
	//state
	flow->state_ = static_cast<rinad::Flow::IPCPFlowState>(gpf_flow.state());
	//qosParameters
	rina::FlowSpecification *fs = get_FlowSpecification(gpf_flow.qosparameters());
	flow->flow_specification_ = *fs;
	delete fs;
	fs = 0;

	//optional connectionPolicies_t connectionPolicies
	rina::ConnectionPolicies *conn_polc = get_ConnectionPolicies(gpf_flow.connectionpolicies());
	flow->getActiveConnection()->setPolicies(*conn_polc);
	delete conn_polc;
	conn_polc = 0;
	//accessControl
	flow->access_control_ =  const_cast<char*>(gpf_flow.accesscontrol().c_str());
	//maxCreateFlowRetries
	flow->max_create_flow_retries_ = gpf_flow.maxcreateflowretries();
	//createFlowRetries
	flow->create_flow_retries_ = gpf_flow.createflowretries();
	//hopCount
	flow->hop_count_ = gpf_flow.hopcount();

	return (void*) flow;
}

rina::messages::applicationProcessNamingInfo_t* FlowEncoder::get_applicationProcessNamingInfo_t(
		const rina::ApplicationProcessNamingInformation &name) const {
	rina::messages::applicationProcessNamingInfo_t *gpf_name =
			new rina::messages::applicationProcessNamingInfo_t;
	gpf_name->set_applicationprocessname(name.getProcessName().c_str());
	gpf_name->set_applicationprocessinstance(name.getProcessInstance());
	gpf_name->set_applicationentityname(name.getEntityName());
	gpf_name->set_applicationentityinstance(name.getEntityInstance());
	return gpf_name;
}

rina::messages::qosSpecification_t* FlowEncoder::get_qosSpecification_t(
		const rina::FlowSpecification &flow_spec) const {
	rina::messages::qosSpecification_t *gpf_flow_spec =
			new rina::messages::qosSpecification_t;
	//name
	gpf_flow_spec->set_name("");
	//qosid
	gpf_flow_spec->set_qosid(0);
	//averageBandwidth
	gpf_flow_spec->set_averagebandwidth(flow_spec.getAverageBandwidth());
	//averageSDUBandwidth
	gpf_flow_spec->set_averagesdubandwidth(flow_spec.getAverageSduBandwidth());
	//peakBandwidthDuration
	gpf_flow_spec->set_peakbandwidthduration(
			flow_spec.getPeakBandwidthDuration());
	//peakSDUBandwidthDuration
	gpf_flow_spec->set_peaksdubandwidthduration(
			flow_spec.getPeakSduBandwidthDuration());
	//undetectedBitErrorRate
	gpf_flow_spec->set_undetectedbiterrorrate(
			flow_spec.getUndetectedBitErrorRate());
	//partialDelivery
	gpf_flow_spec->set_partialdelivery(flow_spec.isPartialDelivery());
	//order
	gpf_flow_spec->set_order(flow_spec.isOrderedDelivery());
	//maxAllowableGapSdu
	gpf_flow_spec->set_maxallowablegapsdu(flow_spec.getMaxAllowableGap());
	//delay
	gpf_flow_spec->set_delay(flow_spec.getDelay());
	//jitter
	gpf_flow_spec->set_jitter(flow_spec.getJitter());

	return gpf_flow_spec;
}

rina::messages::connectionPolicies_t* FlowEncoder::get_connectionPolicies_t(const rina::ConnectionPolicies &polc) const {
	rina::messages::connectionPolicies_t *gpf_polc = new rina::messages::connectionPolicies_t;

	//optional bool dtcpPresent
	gpf_polc->set_dtcppresent(polc.is_dtcp_present());
	//optional dtcpConfig_t dtcpConfiguration
	gpf_polc->set_allocated_dtcpconfiguration(get_dtcpConfig_t(polc.get_dtcp_configuration()));
	//optional policyDescriptor_t initialseqnumpolicy
	gpf_polc->set_allocated_initialseqnumpolicy(get_policyDescriptor_t(polc.get_initial_seq_num_policy()));
	//optional uint64 seqnumrolloverthreshold
	gpf_polc->set_seqnumrolloverthreshold(polc.get_seq_num_rollover_threshold());
	//optional uint32 initialATimer
	gpf_polc->set_initialatimer(polc.get_initial_a_timer());

	return gpf_polc;
}

rina::messages::dtcpConfig_t* FlowEncoder::get_dtcpConfig_t(const rina::DTCPConfig &conf) const {
	rina::messages::dtcpConfig_t *gpf_conf = new rina::messages::dtcpConfig_t;
	//optional bool flowControl
	gpf_conf->set_flowcontrol(conf.is_flow_control());
	//optional dtcpFlowControlConfig_t flowControlConfig
	gpf_conf->set_allocated_flowcontrolconfig(get_dtcpFlowControlConfig_t(conf.get_flow_control_config()));
	//optional bool rtxControl
	gpf_conf->set_rtxcontrol(conf.is_rtx_control());
	//optional dtcpRtxControlConfig_t rtxControlConfig
	gpf_conf->set_allocated_rtxcontrolconfig(get_dtcpRtxControlConfig_t(conf.get_rtx_control_config()));
	//optional uint32 initialsenderinactivitytime
	gpf_conf->set_initialsenderinactivitytime(conf.get_initial_sender_inactivity_time());
	//optional uint32 initialrecvrinactivitytime
	gpf_conf->set_initialrecvrinactivitytime(conf.get_initial_recvr_inactivity_time());
	//optional policyDescriptor_t rcvrtimerinactivitypolicy
	gpf_conf->set_allocated_rcvrtimerinactivitypolicy(get_policyDescriptor_t(conf.get_rcvr_timer_inactivity_policy()));
	//optional policyDescriptor_t sendertimerinactiviypolicy
	gpf_conf->set_allocated_sendertimerinactiviypolicy(get_policyDescriptor_t(conf.get_sender_timer_inactivity_policy()));
	//optional policyDescriptor_t lostcontrolpdupolicy
	gpf_conf->set_allocated_lostcontrolpdupolicy(get_policyDescriptor_t((conf.get_lost_control_pdu_policy())));
	//optional policyDescriptor_t rttestimatorpolicy
	gpf_conf->set_allocated_rttestimatorpolicy(get_policyDescriptor_t((conf.get_rtt_estimator_policy())));

	return gpf_conf;
}

rina::messages::policyDescriptor_t* FlowEncoder::get_policyDescriptor_t(const rina::PolicyConfig &conf) const {
	rina::messages::policyDescriptor_t *gpf_conf = new rina::messages::policyDescriptor_t;
	//optional string policyName
	gpf_conf->set_policyname(conf.get_name());
	//optional string policyImplName
	gpf_conf->set_policyimplname(conf.get_name());
	//optional string version
	gpf_conf->set_version(conf.get_version());
	//repeated property_t policyParameters
	for (std::list<rina::PolicyParameter>::const_iterator it = conf.get_parameters().begin(); it != conf.get_parameters().end(); ++it) {
		rina::messages::property_t *pro = gpf_conf->add_policyparameters();
		*pro = *get_property_t(*it);
	}

	return gpf_conf;
}

rina::messages::dtcpFlowControlConfig_t* FlowEncoder::get_dtcpFlowControlConfig_t(const rina::DTCPFlowControlConfig &conf) const {
	rina::messages::dtcpFlowControlConfig_t *gpf_conf = new rina::messages::dtcpFlowControlConfig_t ;
	//optional bool windowBased
	gpf_conf->set_windowbased(conf.is_window_based());
	//optional dtcpWindowBasedFlowControlConfig_t windowBasedConfig
	gpf_conf->set_allocated_windowbasedconfig(get_dtcpWindowBasedFlowControlConfig_t(conf.get_window_based_config()));
	//optional bool rateBased
	gpf_conf->set_ratebased(conf.is_rate_based());
	//optional dtcpRateBasedFlowControlConfig_t rateBasedConfig
	gpf_conf->set_allocated_ratebasedconfig(get_dtcpRateBasedFlowControlConfig_t(conf.get_rate_based_config()));
	//optional uint64 sentbytesthreshold
	gpf_conf->set_sentbytesthreshold(conf.get_sent_bytes_threshold());
	//optional uint64 sentbytespercentthreshold
	gpf_conf->set_sentbytespercentthreshold(conf.get_sent_bytes_percent_threshold());
	//optional uint64 sentbuffersthreshold
	gpf_conf->set_sentbuffersthreshold(conf.get_sent_buffers_threshold());
	//optional uint64 rcvbytesthreshold
	gpf_conf->set_rcvbytesthreshold(conf.get_rcv_bytes_threshold());
	//optional uint64 rcvbytespercentthreshold
	gpf_conf->set_rcvbytespercentthreshold(conf.get_rcv_bytes_percent_threshold());
	//optional uint64 rcvbuffersthreshold
	gpf_conf->set_rcvbuffersthreshold(conf.get_rcv_buffers_threshold());
	//optional policyDescriptor_t closedwindowpolicy
	gpf_conf->set_allocated_closedwindowpolicy(get_policyDescriptor_t(conf.get_closed_window_policy()));
	//optional policyDescriptor_t flowcontroloverrunpolicy
	gpf_conf->set_allocated_flowcontroloverrunpolicy(get_policyDescriptor_t(conf.get_flow_control_overrun_policy()));
	//optional policyDescriptor_t reconcileflowcontrolpolicy
	gpf_conf->set_allocated_reconcileflowcontrolpolicy(get_policyDescriptor_t(conf.get_reconcile_flow_control_policy()));
	//optional policyDescriptor_t receivingflowcontrolpolicy
	gpf_conf->set_allocated_receivingflowcontrolpolicy(get_policyDescriptor_t(conf.get_receiving_flow_control_policy()));

	return gpf_conf;
}

rina::messages::dtcpRtxControlConfig_t* FlowEncoder::get_dtcpRtxControlConfig_t(const rina::DTCPRtxControlConfig &conf) const {
	rina::messages::dtcpRtxControlConfig_t *gpf_conf = new rina::messages::dtcpRtxControlConfig_t;
	//optional uint32 datarxmsnmax
	gpf_conf->set_datarxmsnmax(conf.get_data_rxmsn_max());
	//optional policyDescriptor_t rtxtimerexpirypolicy
	gpf_conf->set_allocated_rtxtimerexpirypolicy(get_policyDescriptor_t(conf.get_rtx_timer_expiry_policy()));
	//optional policyDescriptor_t senderackpolicy
	gpf_conf->set_allocated_senderackpolicy(get_policyDescriptor_t(conf.get_sender_ack_policy()));
	//optional policyDescriptor_t recvingacklistpolicy
	gpf_conf->set_allocated_recvingacklistpolicy(get_policyDescriptor_t(conf.get_recving_ack_list_policy()));
	//optional policyDescriptor_t rcvrackpolicy
	gpf_conf->set_allocated_rcvrackpolicy(get_policyDescriptor_t(conf.get_rcvr_ack_policy()));
	//optional policyDescriptor_t sendingackpolicy
	gpf_conf->set_allocated_sendingackpolicy(get_policyDescriptor_t(conf.get_sending_ack_policy()));
	//optional policyDescriptor_t rcvrcontrolackpolicy
	gpf_conf->set_allocated_rcvrcontrolackpolicy(get_policyDescriptor_t(conf.get_rcvr_control_ack_policy()));

	return gpf_conf;
}

rina::messages::property_t* FlowEncoder::get_property_t(const rina::PolicyParameter &conf) const {
	rina::messages::property_t *gpf_conf = new rina::messages::property_t;
	//required string name
	gpf_conf->set_name(conf.get_name());
	//required string value
	gpf_conf->set_value(conf.get_value());

	return gpf_conf;
}

rina::messages::dtcpWindowBasedFlowControlConfig_t* FlowEncoder::get_dtcpWindowBasedFlowControlConfig_t(const rina::DTCPWindowBasedFlowControlConfig &conf) const {
	rina::messages::dtcpWindowBasedFlowControlConfig_t * gpf_conf = new rina::messages::dtcpWindowBasedFlowControlConfig_t;
	//optional uint64 maxclosedwindowqueuelength
	gpf_conf->set_maxclosedwindowqueuelength(conf.get_maxclosed_window_queue_length());
	//optional uint64 initialcredit
	gpf_conf->set_initialcredit(conf.get_initial_credit());
	//optional policyDescriptor_t rcvrflowcontrolpolicyç
	gpf_conf->set_allocated_rcvrflowcontrolpolicy(get_policyDescriptor_t(conf.get_rcvr_flow_control_policy()));
	//optional policyDescriptor_t txcontrolpolicy
	gpf_conf->set_allocated_txcontrolpolicy(get_policyDescriptor_t(conf.getTxControlPolicy()));

	return gpf_conf;
}
rina::messages::dtcpRateBasedFlowControlConfig_t* FlowEncoder::get_dtcpRateBasedFlowControlConfig_t(const rina::DTCPRateBasedFlowControlConfig &conf) const {
	rina::messages::dtcpRateBasedFlowControlConfig_t *gpf_conf = new rina::messages::dtcpRateBasedFlowControlConfig_t;
	//optional uint64 sendingrate
	gpf_conf->set_sendingrate(conf.get_sending_rate());
	//optional uint64 timeperiod
	gpf_conf->set_timeperiod(conf.get_time_period());
	//optional policyDescriptor_t norateslowdownpolicy
	gpf_conf->set_allocated_norateslowdownpolicy(get_policyDescriptor_t(conf.get_no_rate_slow_down_policy()));
	//optional policyDescriptor_t nooverridedefaultpeakpolicy
	gpf_conf->set_allocated_nooverridedefaultpeakpolicy(get_policyDescriptor_t(conf.get_no_override_default_peak_policy()));
	//optional policyDescriptor_t ratereductionpolicy
	gpf_conf->set_allocated_ratereductionpolicy(get_policyDescriptor_t(conf.get_rate_reduction_policy()));

	return gpf_conf;
}

rina::ConnectionPolicies* FlowEncoder::get_ConnectionPolicies(const rina::messages::connectionPolicies_t &gpf_polc) const {
	rina::ConnectionPolicies *polc = new rina::ConnectionPolicies;
	//optional bool dtcpPresent
	polc->set_dtcp_present(gpf_polc.dtcppresent());
	//optional dtcpConfig_t dtcpConfiguration
	rina::DTCPConfig *conf = get_DTCPConfig(gpf_polc.dtcpconfiguration());
	polc->set_dtcp_configuration(*conf);
	delete conf;
	conf = 0;
	//optional policyDescriptor_t initialseqnumpolicy
	rina::PolicyConfig *po_conf = get_PolicyConfig(gpf_polc.initialseqnumpolicy());
	polc->set_initial_seq_num_policy(*po_conf);
	delete po_conf;
	po_conf = 0;
	//optional uint64 seqnumrolloverthreshold
	polc->set_seq_num_rollover_threshold(gpf_polc.seqnumrolloverthreshold());
	//optional uint32 initialATimer
	polc->set_initial_a_timer(gpf_polc.initialatimer());

	return polc;
}

rina::ApplicationProcessNamingInformation* FlowEncoder::get_ApplicationProcessNamingInformation(
		const rina::messages::applicationProcessNamingInfo_t &gpf_app) const {
	rina::ApplicationProcessNamingInformation *app = new rina::ApplicationProcessNamingInformation;

	app->setProcessName(gpf_app.applicationprocessname());
	app->setProcessInstance(gpf_app.applicationprocessinstance());
	app->setEntityName(gpf_app.applicationentityname());
	app->setEntityInstance(gpf_app.applicationentityinstance());

	return app;
}

rina::Connection* FlowEncoder::get_Connection(const rina::messages::connectionId_t &gpf_conn) const {
	rina::Connection *conn = new rina::Connection;

	//qosId
	conn->setQosId(gpf_conn.qosid());
	//sourceCEPId
	conn->setSourceCepId(gpf_conn.sourcecepid());
	//destinationCEPId
	conn->setDestCepId(gpf_conn.destinationcepid());

	return conn;
}

rina::FlowSpecification* FlowEncoder::get_FlowSpecification(const rina::messages::qosSpecification_t &gpf_qos) const {
	rina::FlowSpecification *qos = new rina::FlowSpecification;

	//averageBandwidth
	qos->setAverageBandwidth(gpf_qos.averagebandwidth());
	//averageSDUBandwidth
	qos->setAverageSduBandwidth(gpf_qos.averagesdubandwidth());
	//peakBandwidthDuration
	qos->setPeakBandwidthDuration(gpf_qos.peakbandwidthduration());
	//peakSDUBandwidthDuration
	qos->setPeakSduBandwidthDuration(gpf_qos.peaksdubandwidthduration());
	//undetectedBitErrorRate
	qos->setUndetectedBitErrorRate(gpf_qos.undetectedbiterrorrate());
	//partialDelivery
	qos->setPartialDelivery(gpf_qos.partialdelivery());
	//order
	qos->setOrderedDelivery(gpf_qos.partialdelivery());
	//maxAllowableGapSdu
	qos->setMaxAllowableGap(gpf_qos.maxallowablegapsdu());
	//delay
	qos->setDelay(gpf_qos.delay());
	//jitter
	qos->setJitter(gpf_qos.jitter());

	return qos;
}

rina::DTCPConfig* FlowEncoder::get_DTCPConfig(const rina::messages::dtcpConfig_t &gpf_conf) const {
	rina::DTCPConfig *conf = new rina::DTCPConfig;

	//optional bool flowControl
	conf->flow_control_ = gpf_conf.flowcontrol();
	//optional dtcpFlowControlConfig_t flowControlConfig
	rina::DTCPFlowControlConfig *flow_conf = get_DTCPFlowControlConfig(gpf_conf.flowcontrolconfig());
	conf->set_flow_control_config(*flow_conf);
	delete flow_conf;
	flow_conf = 0;
	//optional bool rtxControl
	conf->set_rtx_control(gpf_conf.rtxcontrol());
	//optional dtcpRtxControlConfig_t rtxControlConfig
	rina::DTCPRtxControlConfig *rtx_conf = get_DTCPRtxControlConfig(gpf_conf.rtxcontrolconfig());
	conf->set_rtx_control_config(*rtx_conf);
	delete rtx_conf;
	rtx_conf = 0;
	//optional uint32 initialsenderinactivitytime
	conf->set_initial_sender_inactivity_time(gpf_conf.initialsenderinactivitytime());
	//optional uint32 initialrecvrinactivitytime
	conf->set_initial_recvr_inactivity_time(gpf_conf.initialrecvrinactivitytime());
	//optional policyDescriptor_t rcvrtimerinactivitypolicy
	rina::PolicyConfig *p_conf = get_PolicyConfig(gpf_conf.rcvrtimerinactivitypolicy());
	conf->set_rcvr_timer_inactivity_policy(*p_conf);
	delete p_conf;
	//optional policyDescriptor_t sendertimerinactiviypolicy
	p_conf = get_PolicyConfig(gpf_conf.sendertimerinactiviypolicy());
	conf->set_sender_timer_inactivity_policy(*p_conf);
	delete p_conf;
	//optional policyDescriptor_t lostcontrolpdupolicy
	p_conf = get_PolicyConfig(gpf_conf.lostcontrolpdupolicy());
	conf->set_lost_control_pdu_policy(*p_conf);
	delete p_conf;
	//optional policyDescriptor_t rttestimatorpolicy
	p_conf = get_PolicyConfig(gpf_conf.rttestimatorpolicy());
	conf->set_rtt_estimator_policy(*p_conf);
	delete p_conf;
	p_conf = 0;

	return conf;

}

rina::PolicyConfig* FlowEncoder::get_PolicyConfig(const rina::messages::policyDescriptor_t &gpf_conf) const {
	rina::PolicyConfig *conf = new rina::PolicyConfig;

	//optional string policyName
	conf->set_name(gpf_conf.policyname());
	//optional string version
	conf->set_version(gpf_conf.version());
	//repeated property_t policyParameters
	for (int i =0; i < gpf_conf.policyparameters_size(); ++i)	{
		rina::PolicyParameter *param = get_PolicyParameter(gpf_conf.policyparameters(i));
		conf->parameters_.push_back(*param);
		delete param;
	}

	return conf;
}
rina::DTCPFlowControlConfig* FlowEncoder::get_DTCPFlowControlConfig(const rina::messages::dtcpFlowControlConfig_t &gpf_conf) const {
	rina::DTCPFlowControlConfig *conf = new rina::DTCPFlowControlConfig;

	//optional bool windowBased
	conf->set_window_based(gpf_conf.windowbased());
	//optional dtcpWindowBasedFlowControlConfig_t windowBasedConfig
	rina::DTCPWindowBasedFlowControlConfig *window = get_DTCPWindowBasedFlowControlConfig(gpf_conf.windowbasedconfig());
	conf->set_window_based_config(*window);
	delete window;
	window = 0;
	//optional bool rateBased
	conf->set_rate_based(gpf_conf.ratebased());
	//optional dtcpRateBasedFlowControlConfig_t rateBasedConfig
	rina::DTCPRateBasedFlowControlConfig *rate = get_DTCPRateBasedFlowControlConfig(gpf_conf.ratebasedconfig());
	conf->set_rate_based_config(*rate);
	delete rate;
	rate = 0;
	//optional uint64 sentbytesthreshold
	conf->set_sent_bytes_threshold(gpf_conf.sentbytesthreshold());
	//optional uint64 sentbytespercentthreshold
	conf->set_sent_bytes_percent_threshold(gpf_conf.sentbytespercentthreshold());
	//optional uint64 sentbuffersthreshold
	conf->set_sent_buffers_threshold(gpf_conf.sentbuffersthreshold());
	//optional uint64 rcvbytesthreshold
	conf->set_rcv_bytes_threshold(gpf_conf.rcvbytesthreshold());
	//optional uint64 rcvbytespercentthreshold
	conf->set_rcv_bytes_percent_threshold(gpf_conf.rcvbytespercentthreshold());
	//optional uint64 rcvbuffersthreshold
	conf->set_rcv_buffers_threshold(gpf_conf.rcvbuffersthreshold());
	//optional policyDescriptor_t closedwindowpolicy
	rina::PolicyConfig *poli = get_PolicyConfig(gpf_conf.closedwindowpolicy());
	conf->set_closed_window_policy(*poli);
	delete poli;
	poli = 0;
	//optional policyDescriptor_t flowcontroloverrunpolicy
	poli = get_PolicyConfig(gpf_conf.flowcontroloverrunpolicy());
	conf->set_flow_control_overrun_policy(*poli);
	delete poli;
	poli = 0;
	//optional policyDescriptor_t reconcileflowcontrolpolicy
	poli = get_PolicyConfig(gpf_conf.reconcileflowcontrolpolicy());
	conf->set_reconcile_flow_control_policy(*poli);
	delete poli;
	poli = 0;
	//optional policyDescriptor_t receivingflowcontrolpolicy
	poli = get_PolicyConfig(gpf_conf.receivingflowcontrolpolicy());
	conf->set_receiving_flow_control_policy(*poli);
	delete poli;
	poli = 0;

	return conf;
}

rina::DTCPRtxControlConfig* FlowEncoder::get_DTCPRtxControlConfig(const rina::messages::dtcpRtxControlConfig_t &gpf_conf) const {
	rina::DTCPRtxControlConfig *conf = new rina::DTCPRtxControlConfig;
	//optional uint32 datarxmsnmax
	conf->set_data_rxmsn_max(gpf_conf.datarxmsnmax());
	//optional policyDescriptor_t rtxtimerexpirypolicy
	rina::PolicyConfig *polc = get_PolicyConfig(gpf_conf.rtxtimerexpirypolicy());
	conf->set_rtx_timer_expiry_policy(*polc);
	//optional policyDescriptor_t senderackpolicy
	polc = get_PolicyConfig(gpf_conf.senderackpolicy());
	conf->set_sender_ack_policy(*polc);
	//optional policyDescriptor_t recvingacklistpolicy
	polc = get_PolicyConfig(gpf_conf.recvingacklistpolicy());
	conf->set_recving_ack_list_policy(*polc);
	//optional policyDescriptor_t rcvrackpolicy
	polc = get_PolicyConfig(gpf_conf.rcvrackpolicy());
	conf->set_rcvr_ack_policy(*polc);
	//optional policyDescriptor_t sendingackpolicy
	polc = get_PolicyConfig(gpf_conf.sendingackpolicy());
	conf->set_sending_ack_policy(*polc);
	//optional policyDescriptor_t rcvrcontrolackpolicy
	polc = get_PolicyConfig(gpf_conf.rcvrcontrolackpolicy());
	conf->set_rcvr_control_ack_policy(*polc);

	return conf;
}

rina::PolicyParameter* FlowEncoder::get_PolicyParameter(const rina::messages::property_t &gpf_conf) const {
	rina::PolicyParameter *conf = new rina::PolicyParameter;
	//required string name
	conf->set_name(gpf_conf.name());
	//required string value
	conf->set_value(gpf_conf.value());

	return conf;
}
rina::DTCPWindowBasedFlowControlConfig* FlowEncoder::get_DTCPWindowBasedFlowControlConfig(const rina::messages::dtcpWindowBasedFlowControlConfig_t &gpf_conf) const {
	rina::DTCPWindowBasedFlowControlConfig *conf = new rina::DTCPWindowBasedFlowControlConfig;

	//optional uint64 maxclosedwindowqueuelength
	conf->set_max_closed_window_queue_length(gpf_conf.maxclosedwindowqueuelength());
	//optional uint64 initialcredit
	conf->set_initial_credit(gpf_conf.initialcredit());
	//optional policyDescriptor_t rcvrflowcontrolpolicyç
	rina::PolicyConfig *poli = get_PolicyConfig(gpf_conf.rcvrflowcontrolpolicy());
	conf->set_rcvr_flow_control_policy(*poli);
	delete poli;
	poli = 0;
	//optional policyDescriptor_t txcontrolpolicy
	poli = get_PolicyConfig(gpf_conf.txcontrolpolicy());
	conf->set_tx_control_policy(*poli);
	delete poli;
	poli = 0;

	return conf;
}
rina::DTCPRateBasedFlowControlConfig* FlowEncoder::get_DTCPRateBasedFlowControlConfig(const rina::messages::dtcpRateBasedFlowControlConfig_t &gpf_conf) const{
	rina::DTCPRateBasedFlowControlConfig *conf = new rina::DTCPRateBasedFlowControlConfig;
	//optional uint64 sendingrate
	conf->set_sending_rate(gpf_conf.sendingrate());
	//optional uint64 timeperiod
	conf->set_time_period(gpf_conf.timeperiod());
	//optional policyDescriptor_t norateslowdownpolicy
	rina::PolicyConfig *poli = get_PolicyConfig(gpf_conf.norateslowdownpolicy());
	conf->set_no_rate_slow_down_policy(*poli);
	delete poli;
	poli = 0;
	//optional policyDescriptor_t nooverridedefaultpeakpolicy
	poli = get_PolicyConfig(gpf_conf.nooverridedefaultpeakpolicy());
	conf->set_no_override_default_peak_policy(*poli);
	delete poli;
	poli = 0;
	//optional policyDescriptor_t ratereductionpolicy
	poli = get_PolicyConfig(gpf_conf.ratereductionpolicy());
	conf->set_rate_reduction_policy(*poli);
	delete poli;
	poli = 0;

	return conf;
}

}
