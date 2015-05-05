//
// Enrollment
//
//    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#define RINA_PREFIX "librina.enrollment"

#include <librina/logs.h>
#include "librina/enrollment.h"
#include "librina/irm.h"
#include "librina/ipc-process.h"

namespace rina {

// Class Neighbor RIB object
NeighborRIBObject::NeighborRIBObject(IRIBDaemon* rib_daemon,
	const std::string& object_class, const std::string& object_name,
	const rina::Neighbor* neighbor) :
		SimpleSetMemberRIBObject(rib_daemon, object_class,
					 object_name, neighbor)
{
};

std::string NeighborRIBObject::get_displayable_value()
{
	const rina::Neighbor * nei = (const rina::Neighbor *) get_value();
	std::stringstream ss;
	ss << "Name: " << nei->name_.getEncodedString();
	ss << "; Address: " << nei->address_;
	ss << "; Enrolled: " << nei->enrolled_ << std::endl;
	ss << "; Supporting DIF Name: " << nei->supporting_dif_name_.processName;
	ss << "; Underlying port-id: " << nei->underlying_port_id_;
	ss << "; Number of enroll. attempts: " << nei->number_of_enrollment_attempts_;

	return ss.str();
}

// Class Neighbor Set RIB Object
const std::string NeighborSetRIBObject::NEIGHBOR_SET_RIB_OBJECT_CLASS = "neighbor set";
const std::string NeighborSetRIBObject::NEIGHBOR_RIB_OBJECT_CLASS = "neighbor";
const std::string NeighborSetRIBObject::NEIGHBOR_SET_RIB_OBJECT_NAME =
		RIBNamingConstants::SEPARATOR + RIBNamingConstants::DAF + RIBNamingConstants::SEPARATOR +
		RIBNamingConstants::MANAGEMENT + RIBNamingConstants::SEPARATOR + RIBNamingConstants::NEIGHBORS;

NeighborSetRIBObject::NeighborSetRIBObject(ApplicationProcess * app, IRIBDaemon * rib_daemon) :
	BaseRIBObject(rib_daemon, NEIGHBOR_SET_RIB_OBJECT_CLASS,
			objectInstanceGenerator->getObjectInstance(),
			NEIGHBOR_SET_RIB_OBJECT_NAME){
	lock_ = new rina::Lockable();
	app_ = app;
}

NeighborSetRIBObject::~NeighborSetRIBObject() {
	if (lock_) {
		delete lock_;
	}
}

const void* NeighborSetRIBObject::get_value() const {
	return 0;
}

void NeighborSetRIBObject::remoteCreateObject(void * object_value, const std::string& object_name,
		int invoke_id, rina::CDAPSessionDescriptor * session_descriptor) {
	rina::ScopedLock g(*lock_);
	std::list<rina::Neighbor *> neighborsToCreate;

	(void) invoke_id;  // Stop compiler barfs
	(void) session_descriptor; // Stop compiler barfs

	try {
		if (object_name.compare(NEIGHBOR_SET_RIB_OBJECT_NAME) == 0) {
			std::list<rina::Neighbor *> * neighbors =
					(std::list<rina::Neighbor *> *) object_value;
			std::list<rina::Neighbor *>::const_iterator iterator;
			for(iterator = neighbors->begin(); iterator != neighbors->end(); ++iterator) {
				populateNeighborsToCreateList(*iterator, &neighborsToCreate);
			}

			delete neighbors;
		} else {
			rina::Neighbor * neighbor = (rina::Neighbor *) object_value;
			populateNeighborsToCreateList(neighbor, &neighborsToCreate);
		}
	} catch (rina::Exception &e) {
		LOG_ERR("Error decoding CDAP object value: %s", e.what());
	}

	if (neighborsToCreate.size() == 0) {
		LOG_DBG("No neighbors entries to create or update");
		return;
	}

	try {
		base_rib_daemon_->createObject(NEIGHBOR_SET_RIB_OBJECT_CLASS,
					       NEIGHBOR_SET_RIB_OBJECT_NAME,
					       &neighborsToCreate,
					       0);
	} catch (rina::Exception &e) {
		LOG_ERR("Problems creating RIB object: %s", e.what());
	}
}

void NeighborSetRIBObject::populateNeighborsToCreateList(rina::Neighbor* neighbor,
		std::list<rina::Neighbor *> * list) {
	const rina::Neighbor * candidate;
	std::list<BaseRIBObject*>::const_iterator it;
	bool found = false;

	for(it = get_children().begin(); it != get_children().end(); ++it) {
		candidate = (const rina::Neighbor *) (*it)->get_value();
		if (candidate->get_name().processName.compare(neighbor->name_.processName) == 0)
			found = true;
	}
	if (!found)
		list->push_back(neighbor);
	else
		delete neighbor;
}

void NeighborSetRIBObject::createObject(const std::string& objectClass,
			const std::string& objectName,
			const void* objectValue) {
	(void) objectClass; // Stop compiler barfs

	if (objectName.compare(NEIGHBOR_SET_RIB_OBJECT_NAME) == 0) {
		std::list<rina::Neighbor *>::const_iterator iterator;
		std::list<rina::Neighbor *> * neighbors =
				(std::list<rina::Neighbor *> *) objectValue;

		for (iterator = neighbors->begin(); iterator != neighbors->end(); ++iterator) {
			createNeighbor((*iterator));
		}
	} else {
		rina::Neighbor * currentNeighbor = (rina::Neighbor *) objectValue;
		createNeighbor(currentNeighbor);
	}
}

void NeighborSetRIBObject::createNeighbor(rina::Neighbor * neighbor) {
	//Avoid creating myself as a neighbor
	if (neighbor->name_.processName.compare(app_->get_name()) == 0) {
		return;
	}

	//Only create neighbours with whom I have an N-1 DIF in common
	std::list<rina::ApplicationProcessNamingInformation>::const_iterator it;
	IPCResourceManager * irm =
			dynamic_cast<IPCResourceManager*>(app_->get_ipc_resource_manager());
	bool supportingDifInCommon = false;
	for(it = neighbor->supporting_difs_.begin(); it != neighbor->supporting_difs_.end(); ++it) {
		if (irm->isSupportingDIF((*it))) {
			neighbor->supporting_dif_name_ = (*it);
			supportingDifInCommon = true;
			break;
		}
	}

	if (!supportingDifInCommon) {
		LOG_INFO("Ignoring neighbor %s because we don't have an N-1 DIF in common",
				neighbor->name_.processName.c_str());
		return;
	}

	std::stringstream ss;
	ss<<NEIGHBOR_SET_RIB_OBJECT_NAME<<RIBNamingConstants::SEPARATOR;
	ss<<neighbor->name_.processName;
	BaseRIBObject * ribObject = new NeighborRIBObject(base_rib_daemon_,
							  NEIGHBOR_RIB_OBJECT_CLASS,
							  ss.str(),
							  neighbor);
	add_child(ribObject);
	try {
		base_rib_daemon_->addRIBObject(ribObject);
	} catch(rina::Exception &e){
		LOG_ERR("Problems adding object to the RIB: %s", e.what());
	}
}

//Class IEnrollmentStateMachine
const std::string IEnrollmentStateMachine::STATE_NULL = "NULL";
const std::string IEnrollmentStateMachine::STATE_ENROLLED = "ENROLLED";

IEnrollmentStateMachine::IEnrollmentStateMachine(ApplicationProcess * app, bool isIPCP,
		const rina::ApplicationProcessNamingInformation& remote_naming_info,
		int timeout, rina::ApplicationProcessNamingInformation * supporting_dif_name)
{
	if (!app) {
		throw Exception("Bogus Application Process instance passed");
	}

	app_ = app;
	isIPCP_ = isIPCP;
	rib_daemon_ = dynamic_cast<IRIBDaemon*>(app->get_rib_daemon());
	enrollment_task_ = dynamic_cast<IEnrollmentTask*>(app->get_enrollment_task());
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
	enroller_ = false;
}

void IEnrollmentStateMachine::release(int invoke_id, CDAPSessionDescriptor * session_descriptor)
{
	ScopedLock g(lock_);
	LOG_DBG("Releasing the CDAP connection");

	if (!isValidPortId(session_descriptor)) {
		return;
	}

	createOrUpdateNeighborInformation(false);

	state_ = STATE_NULL;

	if (invoke_id == 0) {
		return;
	}

	try {
		rina::RemoteProcessId remote_id;
		remote_id.port_id_ = port_id_;

		rib_daemon_->closeApplicationConnectionResponse(0, "", invoke_id, remote_id);
	} catch (Exception &e) {
		LOG_ERR("Problems generating or sending CDAP Message: %s", e.what());
	}
}

void IEnrollmentStateMachine::releaseResponse(int result, const std::string& result_reason,
					      CDAPSessionDescriptor * session_descriptor)
{
	ScopedLock g(lock_);

	(void) result;
	(void) result_reason;

	if (!isValidPortId(session_descriptor)) {
		return;
	}

	if (state_ != STATE_NULL) {
		state_ = STATE_NULL;
	}
}

void IEnrollmentStateMachine::flowDeallocated(CDAPSessionDescriptor * cdapSessionDescriptor)
{
	ScopedLock g(lock_);
	LOG_INFO("The flow supporting the CDAP session identified by %d has been deallocated.",
			cdapSessionDescriptor->port_id_);

	if (!isValidPortId(cdapSessionDescriptor)){
		return;
	}

	createOrUpdateNeighborInformation(false);

	state_ = STATE_NULL;
}

bool IEnrollmentStateMachine::isValidPortId(const CDAPSessionDescriptor * cdapSessionDescriptor)
{
	if (cdapSessionDescriptor->port_id_ != port_id_) {
		LOG_ERR("Received a CDAP message form port-id %d, but was expecting it form port-id %d",
				cdapSessionDescriptor->port_id_, port_id_);
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
	Time currentTime;
	remote_peer_->last_heard_from_time_in_ms_ = currentTime.get_current_time_in_ms();
	if (enrolled) {
		remote_peer_->underlying_port_id_ = port_id_;
	} else {
		remote_peer_->underlying_port_id_ = 0;
	}

	try {
		std::stringstream ss;
		ss<<NeighborSetRIBObject::NEIGHBOR_SET_RIB_OBJECT_NAME<<RIBNamingConstants::SEPARATOR;
		ss<<remote_peer_->name_.processName;
		rib_daemon_->createObject(NeighborSetRIBObject::NEIGHBOR_RIB_OBJECT_CLASS,
					  ss.str(),remote_peer_, 0);
	} catch (Exception &e) {
		LOG_ERR("Problems creating RIB object: %s", e.what());
	}
}

void IEnrollmentStateMachine::sendNeighbors()
{
	BaseRIBObject * neighborSet;
	std::list<BaseRIBObject *>::const_iterator it;
	std::list<Neighbor *> neighbors;
	Neighbor * myself = 0;
	std::vector<ApplicationRegistration *> registrations;
	std::list<ApplicationProcessNamingInformation>::const_iterator it2;
	try {
		neighborSet = rib_daemon_->readObject(NeighborSetRIBObject::NEIGHBOR_SET_RIB_OBJECT_CLASS,
						      NeighborSetRIBObject::NEIGHBOR_SET_RIB_OBJECT_NAME);
		for (it = neighborSet->get_children().begin();
				it != neighborSet->get_children().end(); ++it) {
			neighbors.push_back((Neighbor*) (*it)->get_value());
		}

		myself = new Neighbor();
		myself->address_ = app_->get_address();
		myself->name_.processName = app_->get_name();
		myself->name_.processInstance = app_->get_instance();
		if (isIPCP_) {
			registrations = extendedIPCManager->getRegisteredApplications();
		} else {
			registrations = ipcManager->getRegisteredApplications();
		}
		for (unsigned int i=0; i<registrations.size(); i++) {
			for(it2 = registrations[i]->DIFNames.begin();
					it2 != registrations[i]->DIFNames.end(); ++it2) {
				myself->add_supporting_dif((*it2));
			}
		}
		neighbors.push_back(myself);

		RIBObjectValue robject_value;
		robject_value.type_ = RIBObjectValue::complextype;
		robject_value.complex_value_ = &neighbors;
		RemoteProcessId remote_id;
		remote_id.port_id_ = port_id_;

		rib_daemon_->remoteCreateObject(rina::NeighborSetRIBObject::NEIGHBOR_SET_RIB_OBJECT_CLASS,
				rina::NeighborSetRIBObject::NEIGHBOR_SET_RIB_OBJECT_NAME, robject_value,
				0, remote_id, 0);
	} catch (Exception &e) {
		LOG_ERR("Problems sending neighbors: %s", e.what());
	}

	delete myself;
}

void IEnrollmentStateMachine::sendCreateInformation(const std::string& objectClass,
		const std::string& objectName)
{
	rina::BaseRIBObject * ribObject = 0;

	try {
		ribObject = rib_daemon_->readObject(objectClass, objectName);
	} catch (Exception &e) {
		LOG_ERR("Problems reading object from RIB: %s", e.what());
		return;
	}

	if (ribObject->get_value()) {
		try {
			RIBObjectValue robject_value;
			robject_value.type_ = RIBObjectValue::complextype;
			robject_value.complex_value_ = const_cast<void*> (ribObject->get_value());
			RemoteProcessId remote_id;
			remote_id.port_id_ = port_id_;

			rib_daemon_->remoteCreateObject(objectClass, objectName, robject_value, 0, remote_id, 0);
		} catch (Exception &e) {
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
		state_machine_->abortEnrollment(state_machine_->remote_peer_->name_, state_machine_->port_id_,
				reason_, true);
	} catch(rina::Exception &e) {
		LOG_ERR("Problems aborting enrollment: %s", e.what());
	}
}

// Class Enrollment Task
BaseEnrollmentTask::BaseEnrollmentTask(int timeout)
{
	rib_daemon_ = 0;
	irm_ = 0;
	event_manager_ = 0;
	timeout_ = timeout;
	lock_ = new Lockable();
}

BaseEnrollmentTask::~BaseEnrollmentTask()
{
	if (lock_) {
		delete lock_;
	}
}

void BaseEnrollmentTask::set_application_process(rina::ApplicationProcess * ap)
{
	if (!ap)
		return;

	app = ap;
	rib_daemon_ = dynamic_cast<IRIBDaemon*>(app->get_rib_daemon());
	irm_ = dynamic_cast<IPCResourceManager*>(app->get_ipc_resource_manager());
	event_manager_ = dynamic_cast<InternalEventManager*>(app->get_internal_event_manager());
	populateRIB();
	subscribeToEvents();
}

void BaseEnrollmentTask::populateRIB()
{
	try{
		BaseRIBObject * ribObject = new NeighborSetRIBObject(app, rib_daemon_);
		rib_daemon_->addRIBObject(ribObject);
	}catch(Exception &e){
		LOG_ERR("Problems adding object to RIB Daemon: %s", e.what());
	}
}

void BaseEnrollmentTask::subscribeToEvents()
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

const std::list<Neighbor *> BaseEnrollmentTask::get_neighbors() const
{
	std::list<Neighbor *> result;
	BaseRIBObject * ribObject = 0;

	try{
		ribObject = rib_daemon_->readObject(NeighborSetRIBObject::NEIGHBOR_SET_RIB_OBJECT_CLASS,
						    NeighborSetRIBObject::NEIGHBOR_SET_RIB_OBJECT_NAME);
	} catch (Exception &e) {
		LOG_ERR("Problems reading RIB object: %s", e.what());
		return result;
	}

	if (!ribObject) {
		return result;
	}

	std::list<BaseRIBObject *>::const_iterator it;
	for (it = ribObject->get_children().begin();
			it != ribObject->get_children().end(); ++it) {
		result.push_back((Neighbor *) (*it)->get_value());
	}

	return result;
}

IEnrollmentStateMachine * BaseEnrollmentTask::getEnrollmentStateMachine(
		const std::string& apName, int portId, bool remove)
{
	std::stringstream ss;
	ss<<apName<<"-"<<portId;

	if (remove) {
		LOG_DBG("Removing enrollment state machine associated to %s %d",
				apName.c_str(), portId);
		return state_machines_.erase(ss.str());
	} else {
		return state_machines_.find(ss.str());
	}
}

bool BaseEnrollmentTask::isEnrolledTo(const std::string& processName) const
{
	ScopedLock g(*lock_);

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

const std::list<std::string> BaseEnrollmentTask::get_enrolled_app_names() const
{
	std::list<std::string> result;

	std::list<IEnrollmentStateMachine *> machines = state_machines_.getEntries();
	std::list<IEnrollmentStateMachine *>::const_iterator it;
	for (it = machines.begin(); it != machines.end(); ++it) {
		result.push_back((*it)->remote_peer_->name_.processName);
	}

	return result;
}

void BaseEnrollmentTask::initiateEnrollment(EnrollmentRequest * request)
{
	if (isEnrolledTo(request->neighbor_->name_.processName)) {
		LOG_ERR("Already enrolled to IPC Process %s", request->neighbor_->name_.processName.c_str());
		return;
	}

	//Request the allocation of a new N-1 Flow to the destination IPC Process,
	//dedicated to layer management
	//FIXME not providing FlowSpec information
	//FIXME not distinguishing between AEs
	FlowInformation flowInformation;
	flowInformation.remoteAppName = request->neighbor_->name_;
	flowInformation.localAppName.processName = app->get_name();
	flowInformation.localAppName.processInstance = app->get_instance();
	flowInformation.difName = request->neighbor_->supporting_dif_name_;
	unsigned int handle = -1;
	try {
		handle = irm_->allocateNMinus1Flow(flowInformation);
	} catch (Exception &e) {
		LOG_ERR("Problems allocating N-1 flow: %s", e.what());
		return;
	}

	port_ids_pending_to_be_allocated_.put(handle, request);
}

void BaseEnrollmentTask::deallocateFlow(int portId)
{
	try {
		irm_->deallocateNMinus1Flow(portId);
	} catch (Exception &e) {
		LOG_ERR("Problems deallocating N-1 flow: %s", e.what());
	}
}

IEnrollmentStateMachine * BaseEnrollmentTask::getEnrollmentStateMachine(
		const CDAPSessionDescriptor * cdapSessionDescriptor, bool remove)
{
	try {
		if (app->get_name().compare(cdapSessionDescriptor->src_ap_name_) == 0) {
			return getEnrollmentStateMachine(cdapSessionDescriptor->dest_ap_name_,
					cdapSessionDescriptor->port_id_, remove);
		} else {
			return 0;
		}
	} catch (Exception &e) {
		LOG_ERR("Problems retrieving state machine: &s", e.what());
		return 0;
	}
}

void BaseEnrollmentTask::release(int invoke_id, CDAPSessionDescriptor * session_descriptor)
{
	LOG_DBG("Received M_RELEASE cdapMessage from portId %d",
			session_descriptor->port_id_);

	try{
		IEnrollmentStateMachine * stateMachine =
				getEnrollmentStateMachine(session_descriptor, false);
		stateMachine->release(invoke_id, session_descriptor);
	}catch(Exception &e){
		//Error getting the enrollment state machine
		LOG_ERR("Problems getting enrollment state machine: %s", e.what());

		try {
			RemoteProcessId remote_id;
			remote_id.port_id_ = session_descriptor->port_id_;

			rib_daemon_->closeApplicationConnection(remote_id, 0);
		} catch (Exception &e) {
			LOG_ERR("Problems closing application connection: %s", e.what());
		}

		deallocateFlow(session_descriptor->port_id_);
	}
}

void BaseEnrollmentTask::releaseResponse(int result, const std::string& result_reason,
		CDAPSessionDescriptor * session_descriptor)
{
	LOG_DBG("Received M_RELEASE_R cdapMessage from portId %d",
			session_descriptor->port_id_);

	try{
		IEnrollmentStateMachine * stateMachine =
				getEnrollmentStateMachine(session_descriptor, false);
		stateMachine->releaseResponse(result, result_reason, session_descriptor);
	}catch(Exception &e){
		//Error getting the enrollment state machine
		LOG_ERR("Problems getting enrollment state machine: %s", e.what());

		try {
			RemoteProcessId remote_id;
			remote_id.port_id_ = session_descriptor->port_id_;

			rib_daemon_->closeApplicationConnection(remote_id, 0);
		} catch (Exception &e) {
			LOG_ERR("Problems closing application connection: %s", e.what());
		}

		deallocateFlow(session_descriptor->port_id_);
	}
}

void BaseEnrollmentTask::eventHappened(InternalEvent * event)
{
	if (event->type == InternalEvent::APP_N_MINUS_1_FLOW_DEALLOCATED){
		NMinusOneFlowDeallocatedEvent * flowEvent =
				(NMinusOneFlowDeallocatedEvent *) event;
		nMinusOneFlowDeallocated(flowEvent);
	}else if (event->type == InternalEvent::APP_N_MINUS_1_FLOW_ALLOCATED){
		rina::NMinusOneFlowAllocatedEvent * flowEvent =
				(NMinusOneFlowAllocatedEvent *) event;
		nMinusOneFlowAllocated(flowEvent);
	}else if (event->type == InternalEvent::APP_N_MINUS_1_FLOW_ALLOCATION_FAILED){
		NMinusOneFlowAllocationFailedEvent * flowEvent =
				(NMinusOneFlowAllocationFailedEvent *) event;
		nMinusOneFlowAllocationFailed(flowEvent);
	}else if (event->type == InternalEvent::APP_NEIGHBOR_DECLARED_DEAD) {
		NeighborDeclaredDeadEvent * deadEvent =
				(NeighborDeclaredDeadEvent *) event;
		neighborDeclaredDead(deadEvent);
	}
}

void BaseEnrollmentTask::neighborDeclaredDead(NeighborDeclaredDeadEvent * deadEvent)
{
	try{
		irm_->getNMinus1FlowInformation(deadEvent->neighbor_->underlying_port_id_);
	} catch(Exception &e){
		LOG_INFO("The N-1 flow with the dead neighbor has already been deallocated");
		return;
	}

	try{
		LOG_INFO("Requesting the deallocation of the N-1 flow with the dead neibhor");
		irm_->deallocateNMinus1Flow(deadEvent->neighbor_->underlying_port_id_);
	} catch (rina::Exception &e){
		LOG_ERR("Problems requesting the deallocation of a N-1 flow: %s", e.what());
	}
}

void BaseEnrollmentTask::nMinusOneFlowDeallocated(NMinusOneFlowDeallocatedEvent  * event)
{
	//1 Remove the enrollment state machine from the list
	try{
		IEnrollmentStateMachine * enrollmentStateMachine =
				getEnrollmentStateMachine(&(event->cdap_session_descriptor_), true);
		if (!enrollmentStateMachine){
			//Do nothing, we had already cleaned up
			return;
		}else{
			enrollmentStateMachine->flowDeallocated(&(event->cdap_session_descriptor_));
			delete enrollmentStateMachine;
		}
	}catch(Exception &e){
		LOG_ERR("Problems: %s", e.what());
	}

	//2 Check if we still have connectivity to the neighbor, if not, issue a ConnectivityLostEvent
	std::list<IEnrollmentStateMachine *> machines = state_machines_.getEntries();
	std::list<IEnrollmentStateMachine *>::const_iterator it;
	for (it = machines.begin(); it!= machines.end(); ++it) {
		if ((*it)->remote_peer_->name_.processName.compare(
				event->cdap_session_descriptor_.dest_ap_name_) == 0){
			//We still have connectivity with the neighbor, return
			return;
		}
	}

	//3 We don't have connectivity to the neighbor, issue a Connectivity lost event
	std::list<rina::Neighbor *> neighbors = get_neighbors();
	std::list<rina::Neighbor *>::const_iterator it2;
	for (it2 = neighbors.begin(); it2 != neighbors.end(); ++it2) {
		if ((*it2)->name_.processName.compare(event->cdap_session_descriptor_.dest_ap_name_) == 0) {
			rina::ConnectiviyToNeighborLostEvent * event2 =
					new rina::ConnectiviyToNeighborLostEvent((*it2));
			event_manager_->deliverEvent(event2);
			return;
		}
	}
}

void BaseEnrollmentTask::nMinusOneFlowAllocationFailed(NMinusOneFlowAllocationFailedEvent * event)
{
	rina::EnrollmentRequest * request =
			port_ids_pending_to_be_allocated_.erase(event->handle_);

	if (!request){
		return;
	}

	LOG_WARN("The allocation of management flow identified by handle %u has failed. Error code: %d",
			event->handle_, event->flow_information_.portId);

	delete request;
}

void BaseEnrollmentTask::enrollmentFailed(const ApplicationProcessNamingInformation& remotePeerNamingInfo,
		int portId, const std::string& reason, bool sendReleaseMessage)
{
	LOG_ERR("An error happened during enrollment of remote IPC Process %s because of %s",
			remotePeerNamingInfo.getEncodedString().c_str(), reason.c_str());

	//1 Remove enrollment state machine from the store
	IEnrollmentStateMachine * stateMachine =
			getEnrollmentStateMachine(remotePeerNamingInfo.processName, portId, true);
	if (!stateMachine) {
		LOG_ERR("Could not find the enrollment state machine associated to neighbor %s and portId %d",
				remotePeerNamingInfo.processName.c_str(), portId);
		return;
	}

	//2 Send message and deallocate flow if required
	if(sendReleaseMessage){
		try {
			RemoteProcessId remote_id;
			remote_id.port_id_ = portId;

			rib_daemon_->closeApplicationConnection(remote_id, 0);
		} catch (Exception &e) {
			LOG_ERR("Problems closing application connection: %s", e.what());
		}

		deallocateFlow(portId);
	}

	delete stateMachine;
}

void BaseEnrollmentTask::enrollmentCompleted(Neighbor * neighbor, bool enrollee)
{
	NeighborAddedEvent * event = new NeighborAddedEvent(neighbor, enrollee);
	event_manager_->deliverEvent(event);
}

void BaseEnrollmentTask::add_enrollment_state_machine(const std::string& key,
				  IEnrollmentStateMachine * value)
{
	state_machines_.put(key, value);
}

IEnrollmentStateMachine * BaseEnrollmentTask::remove_enrollment_state_machine(const std::string& key)
{
	return state_machines_.erase(key);
}

IEnrollmentStateMachine * BaseEnrollmentTask::get_enrollment_state_machine(const std::string& key)
{
	return state_machines_.find(key);
}

}

