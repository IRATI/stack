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
WatchdogRIBObject::WatchdogRIBObject(IPCProcess * ipc_process, int wdog_period_ms, int declared_dead_int_ms) :
		BaseIPCPRIBObject(ipc_process, EncoderConstants::WATCHDOG_RIB_OBJECT_CLASS,
				rina::objectInstanceGenerator->getObjectInstance(), EncoderConstants::WATCHDOG_RIB_OBJECT_NAME) {
	cdap_session_manager_ = ipc_process->cdap_session_manager_;
	wathchdog_period_ = wdog_period_ms;
	declared_dead_interval_ = declared_dead_int_ms;
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
			rina::NeighborDeclaredDeadEvent * event = new rina::NeighborDeclaredDeadEvent(*(*it));
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
	EnrollmentTask * enrollmentTask = dynamic_cast<EnrollmentTask *>(ipcProcess->enrollment_task_);
	if (!enrollmentTask) {
		LOG_IPCP_ERR("Error casting IEnrollmentTask to EnrollmentTask");
		return (void *) -1;
	}
	std::list<rina::Neighbor*> neighbors;
	std::list<rina::Neighbor*>::const_iterator it;
	rina::Sleep sleepObject;

	while(true){
		neighbors = ipcProcess->get_neighbors();
		for(it = neighbors.begin(); it != neighbors.end(); ++it) {
			if (enrollmentTask->isEnrolledTo((*it)->name_.processName)) {
				//We're already enrolled to this guy, continue
				continue;
			}

			if ((*it)->number_of_enrollment_attempts_ <
					enrollmentTask->max_num_enroll_attempts_) {
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
		sleepObject.sleepForMili(enrollmentTask->neigh_enroll_per_ms_);
	}

	return (void *) 0;
}

//Class IEnrollmentStateMachine
const std::string IEnrollmentStateMachine::STATE_NULL = "NULL";
const std::string IEnrollmentStateMachine::STATE_ENROLLED = "ENROLLED";

IEnrollmentStateMachine::IEnrollmentStateMachine(IPCProcess * ipcp,
		const rina::ApplicationProcessNamingInformation& remote_naming_info,
		int timeout, rina::ApplicationProcessNamingInformation * supporting_dif_name)
{
	if (!ipcp) {
		throw rina::Exception("Bogus Application Process instance passed");
	}

	ipcp_ = ipcp;
	rib_daemon_ = ipcp_->rib_daemon_;
	enrollment_task_ = ipcp_->enrollment_task_;
	timeout_ = timeout;
	remote_peer_ = new rina::Neighbor();
	remote_peer_->name_ = remote_naming_info;
	if (supporting_dif_name) {
		remote_peer_->supporting_dif_name_ = *supporting_dif_name;
		delete supporting_dif_name;
	}
	port_id_ = 0;
	last_scheduled_task_ = 0;
	state_ = STATE_NULL;
	auth_ps_ = 0;
	enroller_ = false;
}

IEnrollmentStateMachine::~IEnrollmentStateMachine() {
}

void IEnrollmentStateMachine::release(int invoke_id, rina::CDAPSessionDescriptor * session_descriptor)
{
	rina::ScopedLock g(lock_);
	LOG_IPCP_DBG("Releasing the CDAP connection");

	if (!isValidPortId(session_descriptor->port_id_)) {
		return;
	}

	if (last_scheduled_task_) {
		timer_.cancelTask(last_scheduled_task_);
	}

	createOrUpdateNeighborInformation(false);

	state_ = STATE_NULL;
}

void IEnrollmentStateMachine::releaseResponse(int result, const std::string& result_reason,
					      rina::CDAPSessionDescriptor * session_descriptor)
{
	rina::ScopedLock g(lock_);

	if (!isValidPortId(session_descriptor->port_id_)) {
		return;
	}

	if (state_ != STATE_NULL) {
		state_ = STATE_NULL;
	}
}

void IEnrollmentStateMachine::flowDeallocated(int portId)
{
	rina::ScopedLock g(lock_);
	LOG_IPCP_INFO("The flow supporting the CDAP session identified by %d has been deallocated.",
			portId);

	if (!isValidPortId(portId)){
		return;
	}

	createOrUpdateNeighborInformation(false);

	state_ = STATE_NULL;
}

bool IEnrollmentStateMachine::isValidPortId(int portId)
{
	if (portId != port_id_) {
		LOG_ERR("Received a CDAP message form port-id %d, but was expecting it form port-id %d",
				portId, port_id_);
		return false;
	}

	return true;
}

void IEnrollmentStateMachine::abortEnrollment(const rina::ApplicationProcessNamingInformation& remotePeerNamingInfo,
			int portId, const std::string& reason, bool sendReleaseMessage)
{
	state_ = STATE_NULL;
	enrollment_task_->enrollmentFailed(remotePeerNamingInfo, portId, reason, sendReleaseMessage);
}

void IEnrollmentStateMachine::createOrUpdateNeighborInformation(bool enrolled)
{
	remote_peer_->enrolled_ = enrolled;
	remote_peer_->number_of_enrollment_attempts_ = 0;
	rina::Time currentTime;
	remote_peer_->last_heard_from_time_in_ms_ = currentTime.get_current_time_in_ms();
	if (enrolled) {
		remote_peer_->underlying_port_id_ = port_id_;
	} else {
		remote_peer_->underlying_port_id_ = 0;
	}

	try {
		std::stringstream ss;
		ss<<rina::NeighborSetRIBObject::NEIGHBOR_SET_RIB_OBJECT_NAME
				<<rina::RIBNamingConstants::SEPARATOR;
		ss<<remote_peer_->name_.processName;
		rib_daemon_->createObject(rina::NeighborSetRIBObject::NEIGHBOR_RIB_OBJECT_CLASS,
					  ss.str(), remote_peer_, 0);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems creating RIB object: %s", e.what());
	}
}

void IEnrollmentStateMachine::sendNeighbors()
{
	rina::BaseRIBObject * neighborSet;
	std::list<rina::BaseRIBObject *>::const_iterator it;
	std::list<rina::Neighbor *> neighbors;
	rina::Neighbor * myself = 0;
	std::vector<rina::ApplicationRegistration *> registrations;
	std::list<rina::ApplicationProcessNamingInformation>::const_iterator it2;
	try {
		neighborSet = rib_daemon_->readObject(rina::NeighborSetRIBObject::NEIGHBOR_SET_RIB_OBJECT_CLASS,
						      rina::NeighborSetRIBObject::NEIGHBOR_SET_RIB_OBJECT_NAME);
		for (it = neighborSet->get_children().begin();
				it != neighborSet->get_children().end(); ++it) {
			neighbors.push_back((rina::Neighbor*) (*it)->get_value());
		}

		myself = new rina::Neighbor();
		myself->address_ = ipcp_->get_address();
		myself->name_.processName = ipcp_->get_name();
		myself->name_.processInstance = ipcp_->get_instance();
		registrations = rina::extendedIPCManager->getRegisteredApplications();

		for (unsigned int i=0; i<registrations.size(); i++) {
			for(it2 = registrations[i]->DIFNames.begin();
					it2 != registrations[i]->DIFNames.end(); ++it2) {
				myself->add_supporting_dif((*it2));
			}
		}
		neighbors.push_back(myself);

		rina::RIBObjectValue robject_value;
		robject_value.type_ = rina::RIBObjectValue::complextype;
		robject_value.complex_value_ = &neighbors;
		rina::RemoteProcessId remote_id;
		remote_id.port_id_ = port_id_;

		rib_daemon_->remoteCreateObject(rina::NeighborSetRIBObject::NEIGHBOR_SET_RIB_OBJECT_CLASS,
				rina::NeighborSetRIBObject::NEIGHBOR_SET_RIB_OBJECT_NAME, robject_value,
				0, remote_id, 0);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems sending neighbors: %s", e.what());
	}

	delete myself;
}

void IEnrollmentStateMachine::sendCreateInformation(const std::string& objectClass,
		const std::string& objectName)
{
	rina::BaseRIBObject * ribObject = 0;

	try {
		ribObject = rib_daemon_->readObject(objectClass, objectName);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems reading object from RIB: %s", e.what());
		return;
	}

	if (ribObject->get_value()) {
		try {
			rina::RIBObjectValue robject_value;
			robject_value.type_ = rina::RIBObjectValue::complextype;
			robject_value.complex_value_ = const_cast<void*> (ribObject->get_value());
			rina::RemoteProcessId remote_id;
			remote_id.port_id_ = port_id_;

			rib_daemon_->remoteCreateObject(objectClass, objectName, robject_value, 0, remote_id, 0);
		} catch (rina::Exception &e) {
			LOG_ERR("Problems generating or sending CDAP message: %s", e.what());
		}
	}
}

//Class EnrollmentFailedTimerTask
EnrollmentFailedTimerTask::EnrollmentFailedTimerTask(IEnrollmentStateMachine * state_machine,
		const std::string& reason) {
	state_machine_ = state_machine;
	reason_ = reason;
}

void EnrollmentFailedTimerTask::run() {
	try {
		state_machine_->abortEnrollment(state_machine_->remote_peer_->name_,
						state_machine_->port_id_,
						reason_, true);
	} catch(rina::Exception &e) {
		LOG_ERR("Problems aborting enrollment: %s", e.what());
	}
}

//Class Enrollment Task
const std::string EnrollmentTask::ENROLL_TIMEOUT_IN_MS = "enrollTimeoutInMs";
const std::string EnrollmentTask::WATCHDOG_PERIOD_IN_MS = "watchdogPeriodInMs";
const std::string EnrollmentTask::DECLARED_DEAD_INTERVAL_IN_MS = "declaredDeadIntervalInMs";
const std::string EnrollmentTask::NEIGHBORS_ENROLLER_PERIOD_IN_MS = "neighborsEnrollerPeriodInMs";
const std::string EnrollmentTask::MAX_ENROLLMENT_RETRIES = "maxEnrollmentRetries";

EnrollmentTask::EnrollmentTask() : IPCPEnrollmentTask()
{
	namespace_manager_ = 0;
	neighbors_enroller_ = 0;
	rib_daemon_ = 0;
	irm_ = 0;
	event_manager_ = 0;
	timeout_ = 10000;
	timeout_ = 0;
	max_num_enroll_attempts_ = 0;
	watchdog_per_ms_ = 0;
	declared_dead_int_ms_ = 0;
	neigh_enroll_per_ms_ = 0;
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

void EnrollmentTask::subscribeToEvents()
{
	event_manager_->subscribeToEvent(rina::InternalEvent::APP_N_MINUS_1_FLOW_DEALLOCATED,
					 this);
	event_manager_->subscribeToEvent(rina::InternalEvent::APP_N_MINUS_1_FLOW_ALLOCATED,
					 this);
	event_manager_->subscribeToEvent(rina::InternalEvent::APP_N_MINUS_1_FLOW_ALLOCATION_FAILED,
					 this);
	event_manager_->subscribeToEvent(rina::InternalEvent::APP_NEIGHBOR_DECLARED_DEAD,
					 this);
}

void EnrollmentTask::eventHappened(rina::InternalEvent * event)
{
	if (event->type == rina::InternalEvent::APP_N_MINUS_1_FLOW_DEALLOCATED){
		rina::NMinusOneFlowDeallocatedEvent * flowEvent =
				(rina::NMinusOneFlowDeallocatedEvent *) event;
		nMinusOneFlowDeallocated(flowEvent);
	}else if (event->type == rina::InternalEvent::APP_N_MINUS_1_FLOW_ALLOCATED){
		rina::NMinusOneFlowAllocatedEvent * flowEvent =
				(rina::NMinusOneFlowAllocatedEvent *) event;
		nMinusOneFlowAllocated(flowEvent);
	}else if (event->type == rina::InternalEvent::APP_N_MINUS_1_FLOW_ALLOCATION_FAILED){
		rina::NMinusOneFlowAllocationFailedEvent * flowEvent =
				(rina::NMinusOneFlowAllocationFailedEvent *) event;
		nMinusOneFlowAllocationFailed(flowEvent);
	}else if (event->type == rina::InternalEvent::APP_NEIGHBOR_DECLARED_DEAD) {
		rina::NeighborDeclaredDeadEvent * deadEvent =
				(rina::NeighborDeclaredDeadEvent *) event;
		neighborDeclaredDead(deadEvent);
	}
}

void EnrollmentTask::set_dif_configuration(const rina::DIFConfiguration& dif_configuration)
{
	rina::PolicyConfig psconf = dif_configuration.et_configuration_.policy_set_;
	if (select_policy_set(std::string(), psconf.name_) != 0) {
		throw rina::Exception("Cannot create enrollment task policy-set");
	}

	// Parse policy config parameters
	timeout_ = psconf.get_param_value_as_int(ENROLL_TIMEOUT_IN_MS);
	max_num_enroll_attempts_ = psconf.get_param_value_as_uint(MAX_ENROLLMENT_RETRIES);
	watchdog_per_ms_ = psconf.get_param_value_as_int(WATCHDOG_PERIOD_IN_MS);
	declared_dead_int_ms_ = psconf.get_param_value_as_int(DECLARED_DEAD_INTERVAL_IN_MS);
	neigh_enroll_per_ms_ = psconf.get_param_value_as_int(NEIGHBORS_ENROLLER_PERIOD_IN_MS);

	//Add Watchdog RIB object to RIB
	try{
		BaseIPCPRIBObject * ribObject = new WatchdogRIBObject(ipcp,
								      watchdog_per_ms_,
								      declared_dead_int_ms_);
		rib_daemon_->addRIBObject(ribObject);
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems adding object to RIB Daemon: %s", e.what());
	}

	//Start Neighbors Enroller thread
	rina::ThreadAttributes * threadAttributes = new rina::ThreadAttributes();
	threadAttributes->setJoinable();
	neighbors_enroller_ = new rina::Thread(&doNeighborsEnrollerWork,
					       (void *) ipcp,
					       threadAttributes);
	neighbors_enroller_->start();
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

			rib_daemon_->openApplicationConnectionResponse(rina::AuthPolicy(),
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

void EnrollmentTask::releaseResponse(int result, const std::string& result_reason,
		rina::CDAPSessionDescriptor * session_descriptor)
{
	LOG_IPCP_DBG("Received M_RELEASE_R cdapMessage from portId %d",
			session_descriptor->port_id_);

	try{
		IEnrollmentStateMachine * stateMachine =
				getEnrollmentStateMachine(session_descriptor->port_id_, false);
		stateMachine->releaseResponse(result, result_reason, session_descriptor);
	}catch(rina::Exception &e){
		//Error getting the enrollment state machine
		LOG_IPCP_ERR("Problems getting enrollment state machine: %s", e.what());

		try {
			rina::RemoteProcessId remote_id;
			remote_id.port_id_ = session_descriptor->port_id_;

			rib_daemon_->closeApplicationConnection(remote_id, 0);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems closing application connection: %s", e.what());
		}

		deallocateFlow(session_descriptor->port_id_);
	}
}

void EnrollmentTask::process_authentication_message(const rina::CDAPMessage& message,
		rina::CDAPSessionDescriptor * session_descriptor)
{
	IPCPEnrollmentTaskPS * ipcp_ps = dynamic_cast<IPCPEnrollmentTaskPS *>(ps);
	assert(ipcp_ps);
	ipcp_ps->process_authentication_message(message, session_descriptor);
}

void EnrollmentTask::authentication_completed(int port_id, bool success)
{
	IPCPEnrollmentTaskPS * ipcp_ps = dynamic_cast<IPCPEnrollmentTaskPS *>(ps);
	assert(ipcp_ps);
	ipcp_ps->authentication_completed(port_id, success);
}

IEnrollmentStateMachine * EnrollmentTask::getEnrollmentStateMachine(int portId, bool remove)
{
	if (remove) {
		LOG_IPCP_DBG("Removing enrollment state machine associated to %d",
				portId);
		return state_machines_.erase(portId);
	} else {
		return state_machines_.find(portId);
	}
}

const std::list<rina::Neighbor *> EnrollmentTask::get_neighbors() const
{
	std::list<rina::Neighbor *> result;
	rina::BaseRIBObject * ribObject = 0;

	try{
		ribObject = rib_daemon_->readObject(rina::NeighborSetRIBObject::NEIGHBOR_SET_RIB_OBJECT_CLASS,
						    rina::NeighborSetRIBObject::NEIGHBOR_SET_RIB_OBJECT_NAME);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems reading RIB object: %s", e.what());
		return result;
	}

	if (!ribObject) {
		return result;
	}

	std::list<rina::BaseRIBObject *>::const_iterator it;
	for (it = ribObject->get_children().begin();
			it != ribObject->get_children().end(); ++it) {
		result.push_back((rina::Neighbor *) (*it)->get_value());
	}

	return result;
}

bool EnrollmentTask::isEnrolledTo(const std::string& processName)
{
	rina::ScopedLock g(lock_);

	std::list<IEnrollmentStateMachine *> machines = state_machines_.getEntries();
	std::list<IEnrollmentStateMachine *>::const_iterator it;
	for (it = machines.begin(); it != machines.end(); ++it) {
		if ((*it)->remote_peer_->name_.processName.compare(processName) == 0 &&
				(*it)->state_ != IEnrollmentStateMachine::STATE_NULL) {
			return true;
		}
	}

	return false;
}

const std::list<std::string> EnrollmentTask::get_enrolled_app_names() const
{
	std::list<std::string> result;

	std::list<IEnrollmentStateMachine *> machines = state_machines_.getEntries();
	std::list<IEnrollmentStateMachine *>::const_iterator it;
	for (it = machines.begin(); it != machines.end(); ++it) {
		result.push_back((*it)->remote_peer_->name_.processName);
	}

	return result;
}

void EnrollmentTask::deallocateFlow(int portId)
{
	try {
		irm_->deallocateNMinus1Flow(portId);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems deallocating N-1 flow: %s", e.what());
	}
}

void EnrollmentTask::add_enrollment_state_machine(int portId, IEnrollmentStateMachine * stateMachine)
{
	state_machines_.put(portId, stateMachine);
}

void EnrollmentTask::neighborDeclaredDead(rina::NeighborDeclaredDeadEvent * deadEvent)
{
	try{
		irm_->getNMinus1FlowInformation(deadEvent->neighbor_.underlying_port_id_);
	} catch(rina::Exception &e){
		LOG_IPCP_INFO("The N-1 flow with the dead neighbor has already been deallocated");
		return;
	}

	try{
		LOG_IPCP_INFO("Requesting the deallocation of the N-1 flow with the dead neighbor");
		irm_->deallocateNMinus1Flow(deadEvent->neighbor_.underlying_port_id_);
	} catch (rina::Exception &e){
		LOG_IPCP_ERR("Problems requesting the deallocation of a N-1 flow: %s", e.what());
	}
}

void EnrollmentTask::nMinusOneFlowDeallocated(rina::NMinusOneFlowDeallocatedEvent * event)
{
	rina::Neighbor * neighbor;

	//1 Remove the enrollment state machine from the list
	IEnrollmentStateMachine * enrollmentStateMachine =
			getEnrollmentStateMachine(event->port_id_, true);
	if (!enrollmentStateMachine){
		//Do nothing, we had already cleaned up
		LOG_IPCP_INFO("Could not find enrollment state machine associated to port-id %d",
				event->port_id_);
		return;
	}

	neighbor = enrollmentStateMachine->remote_peer_;
	enrollmentStateMachine->flowDeallocated(event->port_id_);

	rina::ConnectiviyToNeighborLostEvent * event2 =
			new rina::ConnectiviyToNeighborLostEvent(*neighbor);
	delete enrollmentStateMachine;
	enrollmentStateMachine = 0;
	event_manager_->deliverEvent(event2);
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
	IEnrollmentStateMachine * stateMachine =
			getEnrollmentStateMachine(portId, true);
	if (!stateMachine) {
		LOG_IPCP_ERR("Could not find the enrollment state machine associated to neighbor %s and portId %d",
				remotePeerNamingInfo.processName.c_str(), portId);
		return;
	}

	//2 In the case of the enrollee state machine, reply to the IPC Manager
	IPCPEnrollmentTaskPS * ipcp_ps = dynamic_cast<IPCPEnrollmentTaskPS *>(ps);
	assert(ipcp_ps);
	ipcp_ps->inform_ipcm_about_failure(stateMachine);

	delete stateMachine;
	stateMachine = 0;

	//3 Send message and deallocate flow if required
	if(sendReleaseMessage){
		try {
			rina::RemoteProcessId remote_id;
			remote_id.port_id_ = portId;

			rib_daemon_->closeApplicationConnection(remote_id, 0);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems closing application connection: %s", e.what());
		}
	}

	deallocateFlow(portId);
}

void EnrollmentTask::enrollmentCompleted(const rina::Neighbor& neighbor, bool enrollee)
{
	rina::NeighborAddedEvent * event = new rina::NeighborAddedEvent(neighbor, enrollee);
	event_manager_->deliverEvent(event);
}

void EnrollmentTask::release(int invoke_id, rina::CDAPSessionDescriptor * session_descriptor)
{
	LOG_DBG("Received M_RELEASE cdapMessage from portId %d",
			session_descriptor->port_id_);
	IEnrollmentStateMachine * stateMachine = 0;

	try{
		stateMachine = getEnrollmentStateMachine(session_descriptor->port_id_, true);
		stateMachine->release(invoke_id, session_descriptor);
	}catch(rina::Exception &e){
		//Error getting the enrollment state machine
		LOG_IPCP_ERR("Problems getting enrollment state machine: %s", e.what());
	}

	//2 In the case of the enrollee state machine, reply to the IPC Manager
	IPCPEnrollmentTaskPS * ipcp_ps = dynamic_cast<IPCPEnrollmentTaskPS *>(ps);
	assert(ipcp_ps);
	ipcp_ps->inform_ipcm_about_failure(stateMachine);

	delete stateMachine;
	stateMachine = 0;

	if (invoke_id != 0) {
		try {
			rina::RemoteProcessId remote_id;
			remote_id.port_id_ = session_descriptor->port_id_;

			rib_daemon_->closeApplicationConnectionResponse(0, "", invoke_id, remote_id);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems generating or sending CDAP Message: %s", e.what());
		}
	}

	deallocateFlow(session_descriptor->port_id_);
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
	try {
		if (!enrollment_task_->getEnrollmentStateMachine(cdapSessionDescriptor->port_id_, false)) {
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
	if (ipc_process_->get_operational_state() != ASSIGNED_TO_DIF) {
		ipc_process_->set_operational_state(ASSIGNED_TO_DIF);
	}
}

void OperationalStatusRIBObject::stopObject(const void* object) {
	if (ipc_process_->get_operational_state() != ASSIGNED_TO_DIF) {
		ipc_process_->set_operational_state(INITIALIZED);
	}
}

void OperationalStatusRIBObject::remoteReadObject(int invoke_id,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor)
{
	try {
		rina::RIBObjectValue robject_value;
		robject_value.type_ = rina::RIBObjectValue::inttype;
		robject_value.int_value_ = ipc_process_->get_operational_state();

		rib_daemon_->generateCDAPResponse(invoke_id, cdapSessionDescriptor,
					rina::CDAPMessage::M_READ_R,
					EncoderConstants::OPERATIONAL_STATUS_RIB_OBJECT_CLASS,
					EncoderConstants::OPERATIONAL_STATUS_RIB_OBJECT_NAME,
					robject_value);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems generating or sending CDAP Message: %s",
				e.what());
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

} //namespace rinad
