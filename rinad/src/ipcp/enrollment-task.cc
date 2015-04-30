//
// Enrollment Task
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
#include <assert.h>

//FIXME: Remove include
#include <iostream>

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#define IPCP_MODULE "enrollment-task"
#include "ipcp-logging.h"

#include "common/concurrency.h"
#include "common/encoders/EnrollmentInformationMessage.pb.h"
#include "ipcp/enrollment-task.h"

namespace rinad {

//Class WatchdogTimerTask
WatchdogTimerTask::WatchdogTimerTask(WatchdogRIBObject * watchdog,
                                     rina::Timer *       timer,
                                     int                 delay) {
	watchdog_ = watchdog;
	timer_    = timer;
	delay_    = delay;
}

void WatchdogTimerTask::run() {
	watchdog_->sendMessages();

	//Re-schedule the task
	timer_->scheduleTask(new WatchdogTimerTask(watchdog_, timer_ , delay_), delay_);
}

// CLASS WatchdogRIBObject
WatchdogRIBObject::WatchdogRIBObject(IPCProcess * ipc_process, const rina::DIFConfiguration& dif_configuration) :
		BaseIPCPRIBObject(ipc_process, EncoderConstants::WATCHDOG_RIB_OBJECT_CLASS,
				rina::objectInstanceGenerator->getObjectInstance(), EncoderConstants::WATCHDOG_RIB_OBJECT_NAME) {
	cdap_session_manager_ = ipc_process->cdap_session_manager_;
	wathchdog_period_ = dif_configuration.et_configuration_.watchdog_period_in_ms_;
	declared_dead_interval_ = dif_configuration.et_configuration_.declared_dead_interval_in_ms_;
	lock_ = new rina::Lockable();
	timer_ = new rina::Timer();
	timer_->scheduleTask(new WatchdogTimerTask(this, timer_, wathchdog_period_), wathchdog_period_);
}

WatchdogRIBObject::~WatchdogRIBObject() {
	if (timer_) {
		delete timer_;
	}

	if (lock_) {
		delete lock_;
	}
}

const void* WatchdogRIBObject::get_value() const {
	return 0;
}

void WatchdogRIBObject::remoteReadObject(int invoke_id, rina::CDAPSessionDescriptor * session_descriptor) {
	rina::ScopedLock g(*lock_);

	//1 Send M_READ_R message
	try {
		rina::RIBObjectValue robject_value;
		robject_value.type_ = rina::RIBObjectValue::inttype;
		robject_value.int_value_ = ipc_process_->get_address();
		rina::RemoteProcessId remote_id;
		remote_id.port_id_ = session_descriptor->port_id_;

		rib_daemon_->remoteReadObjectResponse(class_, name_, robject_value, 0,
				"", false, invoke_id, remote_id);
	} catch(rina::Exception &e) {
		LOG_IPCP_ERR("Problems creating or sending CDAP Message: %s", e.what());
	}

	//2 Update last heard from attribute of the relevant neighbor
	std::list<rina::Neighbor*> neighbors = ipc_process_->get_neighbors();
	std::list<rina::Neighbor*>::const_iterator it;
	for (it = neighbors.begin(); it != neighbors.end(); ++it) {
		if ((*it)->name_.processName.compare(session_descriptor->dest_ap_name_) == 0) {
			rina::Time currentTime;
			(*it)->last_heard_from_time_in_ms_ = currentTime.get_current_time_in_ms();
			break;
		}
	}
}

void WatchdogRIBObject::sendMessages() {
	rina::ScopedLock g(*lock_);

	neighbor_statistics_.clear();
	rina::Time currentTime;
	int currentTimeInMs = currentTime.get_current_time_in_ms();
	std::list<rina::Neighbor*> neighbors = ipc_process_->get_neighbors();
	std::list<rina::Neighbor*>::const_iterator it;
	for (it = neighbors.begin(); it != neighbors.end(); ++it) {
		//Skip non enrolled neighbors
		if (!(*it)->enrolled_) {
			continue;
		}

		//Skip neighbors that have sent M_READ messages during the last period
		if ((*it)->last_heard_from_time_in_ms_ + wathchdog_period_ > currentTimeInMs) {
			continue;
		}

		//If we have not heard from the neighbor during long enough, declare the neighbor
		//dead and fire a NEIGHBOR_DECLARED_DEAD event
		if ((*it)->last_heard_from_time_in_ms_ != 0 &&
				(*it)->last_heard_from_time_in_ms_ + declared_dead_interval_ < currentTimeInMs) {
			rina::NeighborDeclaredDeadEvent * event = new rina::NeighborDeclaredDeadEvent((*it));
			ipc_process_->internal_event_manager_->deliverEvent(event);
			continue;
		}

		try{
			rina::RemoteProcessId remote_id;
			remote_id.port_id_ = (*it)->underlying_port_id_;

			rib_daemon_->remoteReadObject(class_, name_, 0, remote_id, this);

			neighbor_statistics_[(*it)->name_.processName] = currentTimeInMs;
		}catch(rina::Exception &e){
			LOG_IPCP_ERR("Problems generating or sending CDAP message: %s", e.what());
		}
	}
}

void WatchdogRIBObject::readResponse(int result, const std::string& result_reason,
		void * object_value, const std::string& object_name,
		rina::CDAPSessionDescriptor * session_descriptor) {
	rina::ScopedLock g(*lock_);

	(void) result;
	(void) result_reason;
	(void) object_name;

	if (object_value) {
		int * address = (int *) object_value;
		delete address;
	}

	std::map<std::string, int>::iterator it = neighbor_statistics_.find(session_descriptor->dest_ap_name_);
	if (it == neighbor_statistics_.end()) {
		return;
	}

	int storedTime = it->second;
	neighbor_statistics_.erase(it);
	rina::Time currentTime;
	int currentTimeInMs = currentTime.get_current_time_in_ms();
	std::list<rina::Neighbor*> neighbors = ipc_process_->get_neighbors();
	std::list<rina::Neighbor*>::const_iterator it2;
	for (it2 = neighbors.begin(); it2 != neighbors.end(); ++it2) {
		if ((*it2)->name_.processName.compare(session_descriptor->dest_ap_name_) == 0) {
			(*it2)->average_rtt_in_ms_ = currentTimeInMs - storedTime;
			(*it2)->last_heard_from_time_in_ms_ = currentTimeInMs;
			break;
		}
	}
}

//Class AddressRIBObject
AddressRIBObject::AddressRIBObject(IPCProcess * ipc_process):
	BaseIPCPRIBObject(ipc_process, EncoderConstants::ADDRESS_RIB_OBJECT_CLASS,
			rina::objectInstanceGenerator->getObjectInstance(),
			EncoderConstants::ADDRESS_RIB_OBJECT_NAME) {
	address_ = ipc_process_->get_address();
}

AddressRIBObject::~AddressRIBObject() {
}

const void* AddressRIBObject::get_value() const {
	return &address_;
}

void AddressRIBObject::writeObject(const void* object_value) {
	int * address = (int *) object_value;
	address_ = *address;
	rina::DIFInformation dif_information = ipc_process_->get_dif_information();
	dif_information.dif_configuration_.address_ = *address;
	ipc_process_->set_dif_information(dif_information);
}

std::string AddressRIBObject::get_displayable_value() {
    std::stringstream ss;
    ss<<"Address: "<<address_;
    return ss.str();
}

//Main function of the Neighbor Enroller thread
void * doNeighborsEnrollerWork(void * arg) {
	IPCProcess * ipcProcess = (IPCProcess *) arg;
	rina::IEnrollmentTask * enrollmentTask = ipcProcess->enrollment_task_;
	std::list<rina::Neighbor*> neighbors;
	std::list<rina::Neighbor*>::const_iterator it;
	rina::EnrollmentTaskConfiguration configuration = ipcProcess->get_dif_information().
			dif_configuration_.et_configuration_;
	rina::Sleep sleepObject;

	while(true){
		neighbors = ipcProcess->get_neighbors();
		for(it = neighbors.begin(); it != neighbors.end(); ++it) {
			if (enrollmentTask->isEnrolledTo((*it)->name_.processName)) {
				//We're already enrolled to this guy, continue
				continue;
			}

			if ((*it)->number_of_enrollment_attempts_ <
					configuration.max_number_of_enrollment_attempts_) {
				(*it)->number_of_enrollment_attempts_++;
				rina::EnrollmentRequest * request = new rina::EnrollmentRequest((*it));
				enrollmentTask->initiateEnrollment(request);
			} else {
				try {
					std::stringstream ss;
					ss << rina::NeighborSetRIBObject::NEIGHBOR_SET_RIB_OBJECT_NAME
					   << EncoderConstants::SEPARATOR;
					ss<<(*it)->name_.processName;
					ipcProcess->rib_daemon_->deleteObject(rina::NeighborSetRIBObject::NEIGHBOR_RIB_OBJECT_CLASS,
							ss.str(), 0, 0);
				} catch (rina::Exception &e){
				}
			}

		}
		sleepObject.sleepForMili(configuration.neighbor_enroller_period_in_ms_);
	}

	return (void *) 0;
}

//Class Enrollment Task
EnrollmentTask::EnrollmentTask() : IPCPEnrollmentTask(10000)
{
	namespace_manager_ = 0;
	neighbors_enroller_ = 0;
}

EnrollmentTask::~EnrollmentTask()
{
	if (neighbors_enroller_) {
		delete neighbors_enroller_;
	}
}

void EnrollmentTask::set_application_process(rina::ApplicationProcess * ap)
{
	if (!ap)
			return;

	app = ap;
	ipcp = dynamic_cast<IPCProcess*>(app);
	if (!ipcp) {
			LOG_IPCP_ERR("Bogus instance of IPCP passed, return");
			return;
	}
	irm_ = ipcp->resource_allocator_->get_n_minus_one_flow_manager();
	rib_daemon_ = ipcp->rib_daemon_;
	event_manager_ = ipcp->internal_event_manager_;
	namespace_manager_ = ipcp->namespace_manager_;
	populateRIB();
	subscribeToEvents();
}

void EnrollmentTask::populateRIB()
{
	try{
		rina::BaseRIBObject * ribObject = new rina::NeighborSetRIBObject(ipcp,
									         ipcp->rib_daemon_);
		rib_daemon_->addRIBObject(ribObject);
		ribObject = new OperationalStatusRIBObject(ipcp);
		rib_daemon_->addRIBObject(ribObject);
		ribObject = new AddressRIBObject(ipcp);
		rib_daemon_->addRIBObject(ribObject);
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems adding object to RIB Daemon: %s", e.what());
	}
}

void EnrollmentTask::set_dif_configuration(const rina::DIFConfiguration& dif_configuration)
{
	timeout_ = dif_configuration.et_configuration_.enrollment_timeout_in_ms_;

	//Add Watchdog RIB object to RIB
	try{
		BaseIPCPRIBObject * ribObject = new WatchdogRIBObject(ipcp, dif_configuration);
		rib_daemon_->addRIBObject(ribObject);
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems adding object to RIB Daemon: %s", e.what());
	}

	//Start Neighbors Enroller thread
	rina::ThreadAttributes * threadAttributes = new rina::ThreadAttributes();
	threadAttributes->setJoinable();
	neighbors_enroller_ = new rina::Thread(threadAttributes,
			&doNeighborsEnrollerWork, (void *) ipcp);
	LOG_IPCP_DBG("Started Neighbors enroller thread");

	//Apply configuration to policy set
	IPCPEnrollmentTaskPS * ipcp_ps = dynamic_cast<IPCPEnrollmentTaskPS *>(ps);
	assert(ipcp_ps);
	ipcp_ps->set_dif_configuration(dif_configuration);
}

void EnrollmentTask::processEnrollmentRequestEvent(rina::EnrollToDAFRequestEvent* event)
{
	//Can only accept enrollment requests if assigned to a DIF
	if (ipcp->get_operational_state() != ASSIGNED_TO_DIF) {
		LOG_IPCP_ERR("Rejected enrollment request since IPC Process is not ASSIGNED to a DIF");
		try {
			rina::extendedIPCManager->enrollToDIFResponse(*event, -1,
					std::list<rina::Neighbor>(), ipcp->get_dif_information());
		}catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems sending message to IPC Manager: %s", e.what());
		}

		return;
	}

	//Check that the neighbor belongs to the same DIF as this IPC Process
	if (ipcp->get_dif_information().get_dif_name().processName.
			compare(event->dafName.processName) != 0) {
		LOG_IPCP_ERR("Was requested to enroll to a neighbor who is member of DIF %s, but I'm member of DIF %s",
				ipcp->get_dif_information().get_dif_name().processName.c_str(),
				event->dafName.processName.c_str());

		try {
			rina::extendedIPCManager->enrollToDIFResponse(*event, -1,
					std::list<rina::Neighbor>(), ipcp->get_dif_information());
		}catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems sending message to IPC Manager: %s", e.what());
		}

		return;
	}

	INamespaceManagerPs *nsmps = dynamic_cast<INamespaceManagerPs *>(namespace_manager_->ps);
	assert(nsmps);

	rina::Neighbor * neighbor = new rina::Neighbor();
	neighbor->name_ = event->neighborName;
	neighbor->supporting_dif_name_ = event->supportingDIFName;
	unsigned int address = nsmps->getValidAddress(neighbor->name_.processName,
			neighbor->name_.processInstance);
	if (address != 0) {
		neighbor->address_ = address;
	}

	rina::EnrollmentRequest * request = new rina::EnrollmentRequest(neighbor, *event);
	initiateEnrollment(request);
}

void EnrollmentTask::initiateEnrollment(rina::EnrollmentRequest * request)
{
	if (isEnrolledTo(request->neighbor_->name_.processName)) {
		LOG_IPCP_ERR("Already enrolled to IPC Process %s", request->neighbor_->name_.processName.c_str());
		return;
	}

	//Request the allocation of a new N-1 Flow to the destination IPC Process,
	//dedicated to layer management
	//FIXME not providing FlowSpec information
	//FIXME not distinguishing between AEs
	rina::FlowInformation flowInformation;
	flowInformation.remoteAppName = request->neighbor_->name_;
	flowInformation.localAppName.processName = ipcp->get_name();
	flowInformation.localAppName.processInstance = ipcp->get_instance();
	flowInformation.difName = request->neighbor_->supporting_dif_name_;
	unsigned int handle = -1;
	try {
		handle = irm_->allocateNMinus1Flow(flowInformation);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems allocating N-1 flow: %s", e.what());

		if (request->ipcm_initiated_) {
			try {
				rina::extendedIPCManager->enrollToDIFResponse(request->event_, -1,
						std::list<rina::Neighbor>(), ipcp->get_dif_information());
			} catch (rina::Exception &e) {
				LOG_IPCP_ERR("Problems sending message to IPC Manager: %s", e.what());
			}
		}

		return;
	}

	port_ids_pending_to_be_allocated_.put(handle, request);
}

void EnrollmentTask::connect(const rina::CDAPMessage& cdapMessage,
			     rina::CDAPSessionDescriptor * session_descriptor)
{
	LOG_IPCP_DBG("Received M_CONNECT CDAP message from port-id %d",
			session_descriptor->port_id_);

	//1 Find out if the sender is really connecting to us
	if(session_descriptor->src_ap_name_.compare(ipcp->get_name())!= 0){
		LOG_IPCP_WARN("Received an M_CONNECT message whose destination was not this IPC Process, ignoring it");
		return;
	}

	//2 Find out if we are already enrolled to the remote IPC process
	if (isEnrolledTo(session_descriptor->dest_ap_name_)){

		std::string message = "Received an enrollment request for an IPC process I'm already enrolled to";
		LOG_IPCP_ERR("%s", message.c_str());

		try {
			rina::RemoteProcessId remote_id;
			remote_id.port_id_ = session_descriptor->port_id_;

			rib_daemon_->openApplicationConnectionResponse(session_descriptor->auth_mech_,
								       rina::AuthValue(),
								       session_descriptor->dest_ae_inst_,
								       session_descriptor->dest_ae_name_,
								       session_descriptor->dest_ap_inst_,
								       session_descriptor->dest_ap_name_,
								       rina::CDAPErrorCodes::CONNECTION_REJECTED_ERROR,
								       message,
								       session_descriptor->src_ae_inst_,
								       session_descriptor->src_ae_name_,
								       session_descriptor->src_ap_inst_,
								       session_descriptor->src_ap_name_,
								       cdapMessage.invoke_id_,
								       remote_id);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems sending CDAP message: %s", e.what());
		}

		deallocateFlow(session_descriptor->port_id_);

		return;
	}

	//3 Delegate further processing to the policy
	IPCPEnrollmentTaskPS * ipcp_ps = dynamic_cast<IPCPEnrollmentTaskPS *>(ps);
	assert(ipcp_ps);
	ipcp_ps->connect_received(cdapMessage, session_descriptor);
}

void EnrollmentTask::connectResponse(int result, const std::string& result_reason,
		rina::CDAPSessionDescriptor * session_descriptor)
{
	LOG_IPCP_DBG("Received M_CONNECT_R cdapMessage from portId %d", session_descriptor->port_id_);

	IPCPEnrollmentTaskPS * ipcp_ps = dynamic_cast<IPCPEnrollmentTaskPS *>(ps);
	assert(ipcp_ps);
	ipcp_ps->connect_response_received(result, result_reason, session_descriptor);
}

void EnrollmentTask::nMinusOneFlowAllocated(rina::NMinusOneFlowAllocatedEvent * flowEvent)
{
	rina::EnrollmentRequest * request =
			port_ids_pending_to_be_allocated_.erase(flowEvent->handle_);

	if (!request) {
		return;
	}

	IPCPEnrollmentTaskPS * ipcp_ps = dynamic_cast<IPCPEnrollmentTaskPS *>(ps);
	assert(ipcp_ps);
	ipcp_ps->initiate_enrollment(*flowEvent, request);
}

void EnrollmentTask::nMinusOneFlowAllocationFailed(rina::NMinusOneFlowAllocationFailedEvent * event)
{
	rina::EnrollmentRequest * request =
			port_ids_pending_to_be_allocated_.erase(event->handle_);

	if (!request){
		return;
	}

	LOG_IPCP_WARN("The allocation of management flow identified by handle %u has failed. Error code: %d",
			event->handle_, event->flow_information_.portId);

	//TODO inform the one that triggered the enrollment?
	if (request->ipcm_initiated_) {
		try {
			rina::extendedIPCManager->enrollToDIFResponse(request->event_, -1,
					std::list<rina::Neighbor>(), ipcp->get_dif_information());
		} catch(rina::Exception &e) {
			LOG_IPCP_ERR("Could not send a message to the IPC Manager: %s", e.what());
		}
	}

	delete request;
}

void EnrollmentTask::enrollmentFailed(const rina::ApplicationProcessNamingInformation& remotePeerNamingInfo,
		int portId, const std::string& reason, bool sendReleaseMessage)
{
	LOG_IPCP_ERR("An error happened during enrollment of remote IPC Process %s because of %s",
			remotePeerNamingInfo.getEncodedString().c_str(), reason.c_str());

	//1 Remove enrollment state machine from the store
	rina::IEnrollmentStateMachine * stateMachine =
			getEnrollmentStateMachine(remotePeerNamingInfo.processName, portId, true);
	if (!stateMachine) {
		LOG_IPCP_ERR("Could not find the enrollment state machine associated to neighbor %s and portId %d",
				remotePeerNamingInfo.processName.c_str(), portId);
		return;
	}

	//2 Send message and deallocate flow if required
	if(sendReleaseMessage){
		try {
			rina::RemoteProcessId remote_id;
			remote_id.port_id_ = portId;

			rib_daemon_->closeApplicationConnection(remote_id, 0);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems closing application connection: %s", e.what());
		}

		deallocateFlow(portId);
	}

	//3 In the case of the enrollee state machine, reply to the IPC Manager
	IPCPEnrollmentTaskPS * ipcp_ps = dynamic_cast<IPCPEnrollmentTaskPS *>(ps);
	assert(ipcp_ps);
	ipcp_ps->inform_ipcm_about_failure(stateMachine);

	delete stateMachine;
}

// Class Operational Status RIB Object
OperationalStatusRIBObject::OperationalStatusRIBObject(IPCProcess * ipc_process) :
		BaseIPCPRIBObject(ipc_process, EncoderConstants::OPERATIONAL_STATUS_RIB_OBJECT_CLASS,
						rina::objectInstanceGenerator->getObjectInstance(),
						EncoderConstants::OPERATIONAL_STATUS_RIB_OBJECT_NAME) {
	enrollment_task_ = (EnrollmentTask *) ipc_process->enrollment_task_;
	cdap_session_manager_ = ipc_process->cdap_session_manager_;
}

void OperationalStatusRIBObject::remoteStartObject(void * object_value, int invoke_id,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	(void) object_value;
	(void) invoke_id;

	try {
		if (!enrollment_task_->getEnrollmentStateMachine(
				cdapSessionDescriptor->dest_ap_name_, cdapSessionDescriptor->port_id_, false)) {
			LOG_IPCP_ERR("Got a CDAP message that is not for me: %s");
			return;
		}
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems retrieving state machine: %s", e.what());
		sendErrorMessage(cdapSessionDescriptor);
		return;
	}

	if (ipc_process_->get_operational_state() != ASSIGNED_TO_DIF) {
		ipc_process_->set_operational_state(ASSIGNED_TO_DIF);
	}
}

void OperationalStatusRIBObject::startObject(const void* object) {
	(void) object; // Stop compiler barfs
	if (ipc_process_->get_operational_state() != ASSIGNED_TO_DIF) {
		ipc_process_->set_operational_state(ASSIGNED_TO_DIF);
	}
}

void OperationalStatusRIBObject::stopObject(const void* object) {
	(void) object; // Stop compiler barfs
	if (ipc_process_->get_operational_state() != ASSIGNED_TO_DIF) {
		ipc_process_->set_operational_state(INITIALIZED);
	}
}

void OperationalStatusRIBObject::sendErrorMessage(const rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	try{
		rina::RemoteProcessId remote_id;
		remote_id.port_id_ = cdapSessionDescriptor->port_id_;

		rib_daemon_->closeApplicationConnection(remote_id, 0);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems sending CDAP message: %s", e.what());
	}
}

const void* OperationalStatusRIBObject::get_value() const {
	return &(ipc_process_->get_operational_state());
}

std::string OperationalStatusRIBObject::get_displayable_value() {
	if (ipc_process_->get_operational_state() == INITIALIZED) {
		return "Initialized";
	}

	if (ipc_process_->get_operational_state() == NOT_INITIALIZED ) {
		return "Not Initialized";
	}

	if (ipc_process_->get_operational_state() == ASSIGN_TO_DIF_IN_PROCESS ) {
		return "Assign to DIF in process";
	}

	if (ipc_process_->get_operational_state() == ASSIGNED_TO_DIF ) {
		return "Assigned to DIF";
	}

	return "Unknown State";
}

// Class EnrollmentInformationRequestEncoder
const rina::SerializedObject* EnrollmentInformationRequestEncoder::encode(const void* object) {
	EnrollmentInformationRequest *eir = (EnrollmentInformationRequest*) object;
	rina::messages::enrollmentInformation_t gpb_eir;

	gpb_eir.set_address(eir->address_);
	gpb_eir.set_startearly(eir->allowed_to_start_early_);

	std::list<rina::ApplicationProcessNamingInformation>::const_iterator it;
	for(it = eir->supporting_difs_.begin(); it != eir->supporting_difs_.end(); ++it) {
		gpb_eir.add_supportingdifs(it->processName);
	}

	int size = gpb_eir.ByteSize();
	char *serialized_message = new char[size];
	gpb_eir.SerializeToArray(serialized_message, size);
	rina::SerializedObject *serialized_object =  new rina::SerializedObject(serialized_message,size);

	return serialized_object;
}

void* EnrollmentInformationRequestEncoder::decode(const rina::ObjectValueInterface * object_value) const {
	rina::messages::enrollmentInformation_t gpb_eir;

	rina::SerializedObject * serializedObject =
			Encoder::get_serialized_object(object_value);

	gpb_eir.ParseFromArray(serializedObject->message_, serializedObject->size_);

	EnrollmentInformationRequest * request = new EnrollmentInformationRequest();
	request->address_ = gpb_eir.address();
	//FIXME that should read gpb_eir.startearly() but always returns false
	request->allowed_to_start_early_ = true;

	for (int i = 0; i < gpb_eir.supportingdifs_size(); ++i) {
		request->supporting_difs_.push_back(rina::ApplicationProcessNamingInformation(
				gpb_eir.supportingdifs(i), ""));
	}

	return (void *) request;
}

}
