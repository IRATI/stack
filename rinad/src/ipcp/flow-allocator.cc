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

std::string FlowRIBObject::get_displayable_value() {
	return flow_allocator_instance_->get_flow()->toString();
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
		const std::string& objectName, const void* objectValue) {
	FlowRIBObject * flowRIBObject;

	void * fai = const_cast<void *>(objectValue);
	flowRIBObject = new FlowRIBObject(ipc_process_, objectClass, objectName,
			(FlowAllocatorInstance *) fai);
	add_child(flowRIBObject);
	rib_daemon_->addRIBObject(flowRIBObject);
}

const void* FlowSetRIBObject::get_value() const {
	return flow_allocator_;
}

// Class Neighbor RIB object
QoSCubeRIBObject::QoSCubeRIBObject(IPCProcess* ipc_process,
		const std::string& object_class, const std::string& object_name,
		const rina::QoSCube* cube) :
				SimpleSetMemberRIBObject(ipc_process, object_class,
						object_name, cube) {
};

std::string QoSCubeRIBObject::get_displayable_value() {
    const rina::QoSCube * cube = (const rina::QoSCube *) get_value();
    std::stringstream ss;
    LOG_DBG("Aqui");
    ss<<"Name: "<<name_<<"; Id: "<<cube->id_;
    ss<<"; Jitter: "<<cube->jitter_<<"; Delay: "<<cube->delay_<<std::endl;
    ss<<"In oder delivery: "<<cube->ordered_delivery_;
    ss<<"; Partial delivery allowed: "<<cube->partial_delivery_<<std::endl;
    ss<<"Max allowed gap between SDUs: "<<cube->max_allowable_gap_;
    ss<<"; Undetected bit error rate: "<<cube->undetected_bit_error_rate_<<std::endl;
    ss<<"Average bandwidth (bytes/s): "<<cube->average_bandwidth_;
    ss<<"; Average SDU bandwidth (bytes/s): "<<cube->average_sdu_bandwidth_<<std::endl;
    ss<<"Peak bandwidth duration (ms): "<<cube->peak_bandwidth_duration_;
    ss<<"; Peak SDU bandwidth duration (ms): "<<cube->peak_sdu_bandwidth_duration_<<std::endl;
    rina::ConnectionPolicies con = cube->efcp_policies_;
    ss<<"EFCP policies: "<< con.toString();
    return ss.str();
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
		const std::string& objectName, const void* objectValue) {
	QoSCubeRIBObject * ribObject = new QoSCubeRIBObject(ipc_process_,
			objectClass, objectName, (const rina::QoSCube*) objectValue);
	add_child(ribObject);
	rib_daemon_->addRIBObject(ribObject);
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

void FlowAllocator::set_dif_configuration(const rina::DIFConfiguration& dif_configuration) {
	//Create QoS cubes RIB objects
	std::list<rina::QoSCube*>::const_iterator it;
	std::stringstream ss;
	const std::list<rina::QoSCube*>& cubes = dif_configuration.efcp_configuration_.qos_cubes_;
	for (it = cubes.begin(); it != cubes.end(); ++it) {
		ss<<EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_NAME<<EncoderConstants::SEPARATOR;
		ss<<(*it)->name_;
		rib_daemon_->createObject(EncoderConstants::QOS_CUBE_RIB_OBJECT_CLASS, ss.str(), (*it), 0);
		ss.str(std::string());
		ss.clear();
	}
}

void FlowAllocator::populateRIB() {
	try {
		BaseRIBObject * object = new FlowSetRIBObject(ipc_process_, this);
		rib_daemon_->addRIBObject(object);
		object = new QoSCubeSetRIBObject(ipc_process_);
		rib_daemon_->addRIBObject(object);
		object = new DataTransferConstantsRIBObject(ipc_process_);
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
				event->localApplicationName);
		LOG_DBG("Got assigned port-id %d", portId);
	} catch (Exception &e) {
		LOG_ERR(
				"Problems requesting an available port-id to the Kernel IPC Manager: %s",
				e.what());
		replyToIPCManager(*event, -1);
	}

	event->portId = portId;
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
			event.sequenceNumber, event.result);

	std::list<IFlowAllocatorInstance *> fais =
			flow_allocator_instances_.getEntries();
	std::list<IFlowAllocatorInstance *>::iterator iterator;
	for (iterator = fais.begin(); iterator != fais.end(); ++iterator) {
		if ((*iterator)->get_allocate_response_message_handle()
				== event.sequenceNumber) {
			flowAllocatorInstance = *iterator;
			flowAllocatorInstance->submitAllocateResponse(event);
			return;
		}
	}

	LOG_ERR("Could not find FAI with handle %ud", event.sequenceNumber);
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

	flowAllocatorInstance = flow_allocator_instances_.find(event.portId);
	if (!flowAllocatorInstance) {
		LOG_ERR("Problems looking for FAI at portId %d", event.portId);
		try {
			rina::extendedIPCManager->deallocatePortId(event.portId);
		} catch (Exception &e) {
			LOG_ERR(
					"Problems requesting IPC Manager to deallocate port-id %d: %s",
					event.portId, e.what());
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
		const std::list<rina::QoSCube*>& qosCubes) {
	Flow* flow;

	flow = new Flow();
	flow->destination_naming_info_ = event.remoteApplicationName;
	flow->source_naming_info_ = event.localApplicationName;
	flow->hop_count_ = 3;
	flow->max_create_flow_retries_ = 1;
	flow->source_ = true;
	flow->state_ = Flow::ALLOCATION_IN_PROGRESS;

	std::list<rina::Connection*> connections;
	rina::QoSCube * qosCube = selectQoSCube(event.flowSpecification,
			qosCubes);
	LOG_DBG("Selected qos cube with name %s and policies: %s",
			qosCube->get_name().c_str());

	rina::Connection * connection = new rina::Connection();
	connection->setQosId(1);
	connection->setFlowUserIpcProcessId(event.flowRequestorIpcProcessId);
	rina::ConnectionPolicies connectionPolicies = rina::ConnectionPolicies(
			qosCube->get_efcp_policies());
	connectionPolicies.set_in_order_delivery(qosCube->is_ordered_delivery());
	connectionPolicies.set_partial_delivery(qosCube->is_partial_delivery());
	if (event.flowSpecification.maxAllowableGap < 0) {
		connectionPolicies.set_max_sdu_gap(INT_MAX);
	} else {
		connectionPolicies.set_max_sdu_gap(qosCube->get_max_allowable_gap());
	}
	connection->setPolicies(connectionPolicies);
	connections.push_back(connection);

	flow->connections_ = connections;
	flow->current_connection_index_ = 0;
	flow->flow_specification_ = event.flowSpecification;

	return flow;
}

rina::QoSCube * SimpleNewFlowRequestPolicy::selectQoSCube(
		const rina::FlowSpecification& flowSpec,
		const std::list<rina::QoSCube*>& qosCubes) {
	if (flowSpec.maxAllowableGap < 0) {
		return qosCubes.front();
	}

	std::list<rina::QoSCube*>::const_iterator iterator;
	rina::QoSCube* cube;
	for (iterator = qosCubes.begin(); iterator != qosCubes.end(); ++iterator) {
		cube = *iterator;
		if (cube->get_efcp_policies().is_dtcp_present()) {
			if (flowSpec.maxAllowableGap >= 0 &&
					cube->get_efcp_policies().get_dtcp_configuration().is_rtx_control()) {
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
	security_manager_ = ipc_process->get_security_manager();
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
			event.remoteApplicationName);
	LOG_DBG("The directory forwarding table returned address %ud",
                destinationAddress);
	flow_->destination_address_ = destinationAddress;
	if (destinationAddress == 0){
		std::stringstream ss;
		ss << "Could not find entry in DFT for application ";
		ss << event.remoteApplicationName.toString();
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

	//2 Check if the source application process has access to the destination application process.
	// If not send negative M_CREATE_R back to the sender IPC process, and do housekeeping.
	if (!security_manager_->acceptFlow(*flow_)) {
		LOG_WARN("Security Manager denied incoming flow request from application %s",
				flow_->source_naming_info_.getEncodedString().c_str());

		const rina::CDAPMessage * responseMessage = 0;
		const rina::SerializedObject * serializedObject = 0;
		try {
			serializedObject = encoder_->encode(flow_,
					EncoderConstants::FLOW_RIB_OBJECT_CLASS);
			rina::ByteArrayObjectValue objectValue = rina::ByteArrayObjectValue(
					*serializedObject);
			responseMessage = cdap_session_manager_->getCreateObjectResponseMessage(
					rina::CDAPMessage::NONE_FLAGS,
					request_message_->get_obj_class(), 0,
					request_message_->get_obj_name(), &objectValue, -1,
					"IPC Process rejected the flow",
					request_message_->get_invoke_id());

			rib_daemon_->sendMessageToAddress(*responseMessage, underlying_port_id_,
					flow_->source_address_, 0);
		} catch (Exception &e){
			LOG_ERR("Problems sending CDAP message: %s", e.what());
		}

		delete responseMessage;
		delete serializedObject;
		releaseUnlockRemove();
		return;
	}

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
	if (event.result == 0) {
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
			flow_request_event_.portId = -1;
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
		flow_request_event_.portId = port_id_;
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
			flow_request_event_.portId = -1;
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
			flow_request_event_.portId = -1;
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

//Class DatatransferConstantsRIBObject
DataTransferConstantsRIBObject::DataTransferConstantsRIBObject(IPCProcess * ipc_process):
	BaseRIBObject(ipc_process, EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_CLASS,
			objectInstanceGenerator->getObjectInstance(),
			EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_NAME){
	cdap_session_manager_ = ipc_process->get_cdap_session_manager();
}

void DataTransferConstantsRIBObject::remoteReadObject(const rina::CDAPMessage * cdapMessage,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	const rina::CDAPMessage * respMessage = 0;
	const rina::SerializedObject * serializedObject = 0;
	try {
		serializedObject = encoder_->encode(get_value(),
				EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_CLASS);
		rina::ByteArrayObjectValue objectValue = rina::ByteArrayObjectValue(
				*serializedObject);
		respMessage = cdap_session_manager_->getReadObjectResponseMessage(rina::CDAPMessage::NONE_FLAGS,
				class_, instance_, name_, &objectValue, 0, "", cdapMessage->invoke_id_);
		rib_daemon_->sendMessage(*respMessage, cdapSessionDescriptor->port_id_, 0);
	}catch (Exception &e) {
		LOG_ERR("Problems generating or sending CDAP Message: %s", e.what());
	}

	delete respMessage;
	delete serializedObject;
}

void DataTransferConstantsRIBObject::remoteCreateObject(const rina::CDAPMessage * cdapMessage,
	rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	//Ignore, since Data Transfer Constants must be set before enrollment (via assign to DIF)
	(void) cdapMessage;
	(void) cdapSessionDescriptor;
}

void DataTransferConstantsRIBObject::createObject(const std::string& objectClass,
        const std::string& objectName, const void* objectValue) {
	(void) objectClass;
	(void) objectName;
	writeObject(objectValue);
}

void DataTransferConstantsRIBObject::writeObject(const void* objectValue) {
	(void) objectValue;
}

const void* DataTransferConstantsRIBObject::get_value() const {
	return &(ipc_process_->get_dif_information().dif_configuration_
			.efcp_configuration_.data_transfer_constants_);
}

std::string DataTransferConstantsRIBObject::get_displayable_value() {
	rina::DataTransferConstants dtc = ipc_process_->get_dif_information().
			dif_configuration_.efcp_configuration_.data_transfer_constants_;
	return dtc.toString();
}

// CLASS FlowEncoder
const rina::SerializedObject* FlowEncoder::encode(const void* object) {
	Flow *flow = (Flow*) object;
	rina::messages::Flow gpf_flow;

	// SourceNamingInfo
	gpf_flow.set_allocated_sourcenaminginfo(
			Encoder::get_applicationProcessNamingInfo_t(flow->source_naming_info_));
	// DestinationNamingInfo
	gpf_flow.set_allocated_destinationnaminginfo(
			Encoder::get_applicationProcessNamingInfo_t(flow->destination_naming_info_));
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
	gpf_flow.set_allocated_qosparameters(Encoder::get_qosSpecification_t(flow->flow_specification_));
	//optional connectionPolicies_t connectionPolicies
	gpf_flow.set_allocated_connectionpolicies(Encoder::get_connectionPolicies_t(flow->getActiveConnection()->getPolicies()));
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

	rina::ApplicationProcessNamingInformation *src_app =
			Encoder::get_ApplicationProcessNamingInformation(gpf_flow.sourcenaminginfo());
	flow->source_naming_info_ = *src_app;
	delete src_app;
	src_app = 0;

	rina::ApplicationProcessNamingInformation *dest_app =
			Encoder::get_ApplicationProcessNamingInformation(gpf_flow.destinationnaminginfo());
	flow->destination_naming_info_ = *dest_app;
	delete dest_app;
	dest_app = 0;

	flow->source_port_id_ = gpf_flow.sourceportid();
	flow->destination_port_id_ = gpf_flow.destinationportid();
	flow->source_address_ = gpf_flow.sourceaddress();
	flow->destination_address_ = gpf_flow.destinationaddress();
	for (int i = 0; i < gpf_flow.connectionids_size(); ++i)
		flow->connections_.push_back(Encoder::get_Connection(gpf_flow.connectionids(i)));
	flow->current_connection_index_ = gpf_flow.currentconnectionidindex();
	flow->state_ = static_cast<rinad::Flow::IPCPFlowState>(gpf_flow.state());
	rina::FlowSpecification *fs = Encoder::get_FlowSpecification(gpf_flow.qosparameters());
	flow->flow_specification_ = *fs;
	delete fs;
	fs = 0;

	rina::ConnectionPolicies *conn_polc = Encoder::get_ConnectionPolicies(gpf_flow.connectionpolicies());
	flow->getActiveConnection()->setPolicies(*conn_polc);
	delete conn_polc;
	conn_polc = 0;

	flow->access_control_ =  const_cast<char*>(gpf_flow.accesscontrol().c_str());
	flow->max_create_flow_retries_ = gpf_flow.maxcreateflowretries();
	flow->create_flow_retries_ = gpf_flow.createflowretries();
	flow->hop_count_ = gpf_flow.hopcount();

	return (void*) flow;
}

}
