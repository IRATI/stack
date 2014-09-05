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

//FIXME: Remove include
#include <iostream>

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#define RINA_PREFIX "enrollment-task"

#include <librina/logs.h>
#include "common/concurrency.h"
#include "common/encoders/EnrollmentInformationMessage.pb.h"
#include "ipcp/enrollment-task.h"

namespace rinad {

//	CLASS EnrollmentInformationRequest
EnrollmentInformationRequest::EnrollmentInformationRequest() {
	address_ = 0;
}

//Class WatchdogTimerTask
WatchdogTimerTask::WatchdogTimerTask(WatchdogRIBObject * watchdog, rina::Timer * timer,
		int delay) {
	watchdog_ = watchdog;
	timer_ = timer;
	delay_ = delay;
}

void WatchdogTimerTask::run() {
	watchdog_->sendMessages();

	//Re-schedule the task
	timer_->scheduleTask(new WatchdogTimerTask(watchdog_, timer_ , delay_), delay_);
}

// CLASS WatchdogRIBObject
WatchdogRIBObject::WatchdogRIBObject(IPCProcess * ipc_process, const rina::DIFConfiguration& dif_configuration) :
		BaseRIBObject(ipc_process, EncoderConstants::WATCHDOG_RIB_OBJECT_CLASS,
				objectInstanceGenerator->getObjectInstance(), EncoderConstants::WATCHDOG_RIB_OBJECT_NAME) {
	cdap_session_manager_ = ipc_process->get_cdap_session_manager();
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
	rina::AccessGuard g(*lock_);

	//1 Send M_READ_R message
	try {
		RIBObjectValue robject_value;
		RemoteIPCProcessId remote_id;
		remote_id.port_id_ = session_descriptor->port_id_;

		rib_daemon_->remoteReadObjectResponse(class_, name_, robject_value, 0,
				"", false, invoke_id, remote_id);
	} catch(Exception &e) {
		LOG_ERR("Problems creating or sending CDAP Message: %s", e.what());
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
	rina::AccessGuard g(*lock_);

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
			NeighborDeclaredDeadEvent * event = new NeighborDeclaredDeadEvent((*it));
			rib_daemon_->deliverEvent(event);
			continue;
		}

		try{
			RemoteIPCProcessId remote_id;
			remote_id.port_id_ = (*it)->underlying_port_id_;

			rib_daemon_->remoteReadObject(class_, name_, 0, remote_id, this);

			neighbor_statistics_[(*it)->name_.processName] = currentTimeInMs;
		}catch(Exception &e){
			LOG_ERR("Problems generating or sending CDAP message: %s", e.what());
		}
	}
}

void WatchdogRIBObject::readResponse(int result, const std::string& result_reason,
		void * object_value, const std::string& object_name,
		rina::CDAPSessionDescriptor * session_descriptor) {
	rina::AccessGuard g(*lock_);

	(void) result;
	(void) result_reason;
	(void) object_value;
	(void) object_name;

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

// Class Neighbor RIB object
NeighborRIBObject::NeighborRIBObject(IPCProcess* ipc_process,
		const std::string& object_class, const std::string& object_name,
		const rina::Neighbor* neighbor) :
				SimpleSetMemberRIBObject(ipc_process, object_class,
						object_name, neighbor) {
};

std::string NeighborRIBObject::get_displayable_value() {
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
NeighborSetRIBObject::NeighborSetRIBObject(IPCProcess * ipc_process) :
	BaseRIBObject(ipc_process, EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_CLASS,
			objectInstanceGenerator->getObjectInstance(), EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_NAME){
	lock_ = new rina::Lockable();
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
	rina::AccessGuard g(*lock_);
	std::list<rina::Neighbor *> neighborsToCreate;

	(void) invoke_id;  // Stop compiler barfs
	(void) session_descriptor; // Stop compiler barfs

	try {
		if (object_name.compare(EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_NAME) == 0) {
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
	} catch (Exception &e) {
		LOG_ERR("Error decoding CDAP object value: %s", e.what());
	}

	if (neighborsToCreate.size() == 0) {
		LOG_DBG("No neighbors entries to create or update");
		return;
	}

	try {
		rib_daemon_->createObject(EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_CLASS,
				EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_NAME, &neighborsToCreate, 0);
	} catch (Exception &e) {
		LOG_ERR("Problems creating RIB object: %s", e.what());
	}
}

void NeighborSetRIBObject::populateNeighborsToCreateList(rina::Neighbor* neighbor,
		std::list<rina::Neighbor *> * list) {
	const rina::Neighbor * candidate;
	std::list<BaseRIBObject*>::const_iterator it;

	for(it = get_children().begin(); it != get_children().end(); ++it) {
		candidate = (const rina::Neighbor *) (*it)->get_value();
		if (candidate->get_name().processName.compare(neighbor->name_.processName) == 0) {
			list->push_back(neighbor);
			return;
		}
	}

	delete neighbor;
}

void NeighborSetRIBObject::createObject(const std::string& objectClass,
			const std::string& objectName,
			const void* objectValue) {
	(void) objectClass; // Stop compiler barfs

	if (objectName.compare(EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_NAME) == 0) {
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
	if (neighbor->name_.processName.compare(ipc_process_->get_name().processName) == 0) {
		return;
	}

	//Only create neighbours with whom I have an N-1 DIF in common
	std::list<rina::ApplicationProcessNamingInformation>::const_iterator it;
	INMinusOneFlowManager * nMinusOneFlowManager =
			ipc_process_->get_resource_allocator()->get_n_minus_one_flow_manager();
	bool supportingDifInCommon = false;
	for(it = neighbor->supporting_difs_.begin(); it != neighbor->supporting_difs_.end(); ++it) {
		if (nMinusOneFlowManager->isSupportingDIF((*it))) {
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
	ss<<EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_NAME<<EncoderConstants::SEPARATOR;
	ss<<neighbor->name_.processName;
	BaseRIBObject * ribObject = new NeighborRIBObject(ipc_process_,
			EncoderConstants::NEIGHBOR_RIB_OBJECT_CLASS, ss.str(), neighbor);
	add_child(ribObject);
	try {
		rib_daemon_->addRIBObject(ribObject);
	} catch(Exception &e){
		LOG_ERR("Problems adding object to the RIB: %s", e.what());
	}
}

//Class AddressRIBObject
AddressRIBObject::AddressRIBObject(IPCProcess * ipc_process):
	BaseRIBObject(ipc_process, EncoderConstants::ADDRESS_RIB_OBJECT_CLASS,
			objectInstanceGenerator->getObjectInstance(), EncoderConstants::ADDRESS_RIB_OBJECT_NAME) {
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

//Class EnrollmentFailedTimerTask
EnrollmentFailedTimerTask::EnrollmentFailedTimerTask(BaseEnrollmentStateMachine * state_machine,
		const std::string& reason, bool enrollee) {
	state_machine_ = state_machine;
	reason_ = reason;
	enrollee_ = enrollee;
}

void EnrollmentFailedTimerTask::run() {
	try {
		state_machine_->abortEnrollment(state_machine_->remote_peer_->name_, state_machine_->port_id_,
				reason_, enrollee_, true);
	} catch(Exception &e) {
		LOG_ERR("Problems aborting enrollment: %s", e.what());
	}
}

//Class BaseEnrollmentStateMachine
const std::string BaseEnrollmentStateMachine::CONNECT_RESPONSE_TIMEOUT = "Timeout waiting for connect response";
const std::string BaseEnrollmentStateMachine::START_RESPONSE_TIMEOUT = "Timeout waiting for start response";
const std::string BaseEnrollmentStateMachine::START_IN_BAD_STATE = "Received a START message in a wrong state";
const std::string BaseEnrollmentStateMachine::STOP_ENROLLMENT_TIMEOUT = "Timeout waiting for stop enrolment response";
const std::string BaseEnrollmentStateMachine::STOP_IN_BAD_STATE = "Received a STOP message in a wrong state";
const std::string BaseEnrollmentStateMachine::STOP_WITH_NO_OBJECT_VALUE = "Received STOP message with null object value";
const std::string BaseEnrollmentStateMachine::READ_RESPONSE_TIMEOUT = "Timeout waiting for read response";
const std::string BaseEnrollmentStateMachine::PROBLEMS_COMMITTING_ENROLLMENT_INFO = "Problems commiting enrollment information";
const std::string BaseEnrollmentStateMachine::START_TIMEOUT = "Timeout waiting for start";
const std::string BaseEnrollmentStateMachine::READ_RESPONSE_IN_BAD_STATE = "Received a READ_RESPONSE message in a wrong state";
const std::string BaseEnrollmentStateMachine::UNSUCCESSFULL_READ_RESPONSE =
		"Received an unsuccessful read response or a read response with a null object value";
const std::string BaseEnrollmentStateMachine::UNSUCCESSFULL_START = "Received unsuccessful start request";
const std::string BaseEnrollmentStateMachine::CONNECT_IN_NOT_NULL = "Received a CONNECT message while not in NULL state";
const std::string BaseEnrollmentStateMachine::ENROLLMENT_NOT_ALLOWED = "Enrollment rejected by security manager";
const std::string BaseEnrollmentStateMachine::START_ENROLLMENT_TIMEOUT = "Timeout waiting for start enrollment request";
const std::string BaseEnrollmentStateMachine::STOP_ENROLLMENT_RESPONSE_TIMEOUT = "Timeout waiting for stop enrollment response";
const std::string BaseEnrollmentStateMachine::STOP_RESPONSE_IN_BAD_STATE = "Received a STOP Response message in a wrong state";

BaseEnrollmentStateMachine::BaseEnrollmentStateMachine(IPCProcess * ipc_process,
			const rina::ApplicationProcessNamingInformation& remote_naming_info,
			int timeout, rina::ApplicationProcessNamingInformation * supporting_dif_name) {
	ipc_process_ = ipc_process;
	rib_daemon_ = ipc_process->get_rib_daemon();
	cdap_session_manager_ = ipc_process->get_cdap_session_manager();
	encoder_ = ipc_process->get_encoder();
	enrollment_task_ = ipc_process->get_enrollment_task();
	timeout_ = timeout;
	timer_ = new rina::Timer();
	lock_ = new rina::Lockable();
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

BaseEnrollmentStateMachine::~BaseEnrollmentStateMachine() {
	if (timer_) {
		delete timer_;
	}

	if (lock_) {
		delete lock_;
	}
}

void BaseEnrollmentStateMachine::abortEnrollment(const rina::ApplicationProcessNamingInformation& remotePeerNamingInfo,
			int portId, const std::string& reason, bool enrollee, bool sendReleaseMessage) {
	if (timer_) {
		delete timer_;
		timer_ = 0;
	}

	state_ = STATE_NULL;

	enrollment_task_->enrollmentFailed(remotePeerNamingInfo, portId, reason, enrollee, sendReleaseMessage);
}

bool BaseEnrollmentStateMachine::isValidPortId(const rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	if (cdapSessionDescriptor->port_id_ != port_id_) {
		LOG_ERR("Received a CDAP message form port-id %d, but was expecting it form port-id %d",
				cdapSessionDescriptor->port_id_, port_id_);
		return false;
	}

	return true;
}

void BaseEnrollmentStateMachine::release(int invoke_id,
		rina::CDAPSessionDescriptor * session_descriptor) {
	rina::AccessGuard g(*lock_);
	LOG_DBG("Releasing the CDAP connection");

	if (!isValidPortId(session_descriptor)) {
		return;
	}

	createOrUpdateNeighborInformation(false);

	state_ = STATE_NULL;
	if (timer_) {
		delete timer_;
		timer_ = 0;
	}

	if (invoke_id == 0) {
		return;
	}

	try {
		RemoteIPCProcessId remote_id;
		remote_id.port_id_ = port_id_;

		rib_daemon_->closeApplicationConnectionResponse(0, "", invoke_id, remote_id);
	} catch (Exception &e) {
		LOG_ERR("Problems generating or sending CDAP Message: %s", e.what());
	}
}

void BaseEnrollmentStateMachine::releaseResponse(int result, const std::string& result_reason,
		rina::CDAPSessionDescriptor * session_descriptor) {
	rina::AccessGuard g(*lock_);

	(void) result;
	(void) result_reason;

	if (!isValidPortId(session_descriptor)) {
		return;
	}

	if (state_ != STATE_NULL) {
		state_ = STATE_NULL;
	}
}

void BaseEnrollmentStateMachine::flowDeallocated(rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	rina::AccessGuard g(*lock_);
	LOG_INFO("The flow supporting the CDAP session identified by %d has been deallocated.",
			cdapSessionDescriptor->port_id_);

	if (!isValidPortId(cdapSessionDescriptor)){
		return;
	}

	createOrUpdateNeighborInformation(false);

	state_ = STATE_NULL;
	if (timer_) {
		delete timer_;
		timer_ = 0;
	}
}

void BaseEnrollmentStateMachine::createOrUpdateNeighborInformation(bool enrolled) {
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
		ss<<EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_NAME<<EncoderConstants::SEPARATOR;
		ss<<remote_peer_->name_.processName;
		rib_daemon_->createObject(EncoderConstants::NEIGHBOR_RIB_OBJECT_CLASS, ss.str(),
				remote_peer_, 0);
	} catch (Exception &e) {
		LOG_ERR("Problems creating RIB object: %s", e.what());
	}
}

void BaseEnrollmentStateMachine::sendDIFDynamicInformation() {
	//Send DirectoryForwardingTableEntries
	sendCreateInformation(EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_CLASS,
			EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_NAME);

	//Send neighbors (including myself)
	BaseRIBObject * neighborSet;
	std::list<BaseRIBObject *>::const_iterator it;
	std::list<rina::Neighbor *> neighbors;
	rina::Neighbor * myself = 0;
	std::vector<rina::ApplicationRegistration *> registrations;
	std::list<rina::ApplicationProcessNamingInformation>::const_iterator it2;
	try {
		neighborSet = rib_daemon_->readObject(EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_CLASS,
				EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_NAME);
		for (it = neighborSet->get_children().begin();
				it != neighborSet->get_children().end(); ++it) {
			neighbors.push_back((rina::Neighbor*) (*it)->get_value());
		}

		myself = new rina::Neighbor();
		myself->address_ = ipc_process_->get_address();
		myself->name_ = ipc_process_->get_name();
		registrations = rina::extendedIPCManager->getRegisteredApplications();
		for (unsigned int i=0; i<registrations.size(); i++) {
			for(it2 = registrations[i]->DIFNames.begin();
					it2 != registrations[i]->DIFNames.end(); ++it2) {
				myself->add_supporting_dif((*it2));
			}
		}

		RIBObjectValue robject_value;
		robject_value.type_ = RIBObjectValue::complextype;
		robject_value.complex_value_ = &neighbors;
		RemoteIPCProcessId remote_id;
		remote_id.port_id_ = port_id_;

		rib_daemon_->remoteCreateObject(EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_CLASS,
				EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_NAME, robject_value, 0, remote_id, 0);
	} catch (Exception &e) {
		LOG_ERR("Problems sending neighbors: %s", e.what());
	}

	delete myself;
}

void BaseEnrollmentStateMachine::sendCreateInformation(const std::string& objectClass,
		const std::string& objectName) {
	BaseRIBObject * ribObject = 0;

	try {
		ribObject = rib_daemon_->readObject(objectClass, objectName);
	} catch (Exception &e) {
		LOG_ERR("Problems reading object from RIB: %s", e.what());
		return;
	}

	try {
		RIBObjectValue robject_value;
		robject_value.type_ = RIBObjectValue::complextype;
		robject_value.complex_value_ = const_cast<void*> (ribObject->get_value());
		RemoteIPCProcessId remote_id;
		remote_id.port_id_ = port_id_;

		rib_daemon_->remoteCreateObject(objectClass, objectName, robject_value, 0, remote_id, 0);
	} catch (Exception &e) {
		LOG_ERR("Problems generating or sending CDAP message: %s", e.what());
	}
}

// Class EnrolleeStateMachine
EnrolleeStateMachine::EnrolleeStateMachine(IPCProcess * ipc_process,
		const rina::ApplicationProcessNamingInformation& remote_naming_info,
		int timeout): BaseEnrollmentStateMachine(ipc_process, remote_naming_info,
				timeout, 0) {
	was_dif_member_before_enrollment_ = false;
	enrollment_request_ = 0;
	last_scheduled_task_ = 0;
	allowed_to_start_early_ = false;
	stop_request_invoke_id_ = 0;
}

EnrolleeStateMachine::~EnrolleeStateMachine() {
}

void EnrolleeStateMachine::initiateEnrollment(EnrollmentRequest * enrollmentRequest, int portId) {
	rina::AccessGuard g(*lock_);

	enrollment_request_ = enrollmentRequest;
	remote_peer_->address_ = enrollment_request_->neighbor_->address_;
	remote_peer_->name_ = enrollment_request_->neighbor_->name_;
	remote_peer_->supporting_dif_name_ = enrollment_request_->neighbor_->supporting_dif_name_;
	remote_peer_->underlying_port_id_ = enrollment_request_->neighbor_->underlying_port_id_;
	remote_peer_->supporting_difs_ = enrollment_request_->neighbor_->supporting_difs_;

	if (state_ != STATE_NULL) {
		throw Exception("Enrollee state machine not in NULL state");
	}

	try{
		RemoteIPCProcessId remote_id;
		remote_id.port_id_ = portId;

		rib_daemon_->openApplicationConnection(rina::CDAPMessage::AUTH_NONE, rina::AuthValue(), "", IPCProcess::MANAGEMENT_AE,
				remote_peer_->name_.processInstance, remote_peer_->name_.processName, "",
				IPCProcess::MANAGEMENT_AE, ipc_process_->get_name().processInstance,
				ipc_process_->get_name().processName, remote_id);

		port_id_ = portId;

		//Set timer
		last_scheduled_task_ = new EnrollmentFailedTimerTask(this, CONNECT_RESPONSE_TIMEOUT, true);
		timer_->scheduleTask(last_scheduled_task_, timeout_);

		//Update state
		state_ = STATE_WAIT_CONNECT_RESPONSE;
	}catch(Exception &e){
		LOG_ERR("Problems sending M_CONNECT message: %s", e.what());
		abortEnrollment(remote_peer_->name_, port_id_, std::string(e.what()), true, false);
	}
}

void EnrolleeStateMachine::connectResponse(int result,
		const std::string& result_reason) {
	rina::AccessGuard g(*lock_);

	if (state_ != STATE_WAIT_CONNECT_RESPONSE) {
		abortEnrollment(remote_peer_->name_, port_id_,
				"Message received in wrong order", true, true);
		return;
	}

	timer_->cancelTask(last_scheduled_task_);
	if (result != 0) {
		state_ = STATE_NULL;
		enrollment_task_->enrollmentFailed(remote_peer_->get_name(), port_id_,
				result_reason, true, true);
		return;
	}

	//Send M_START with EnrollmentInformation object
	try{
		EnrollmentInformationRequest eiRequest;
		std::list<rina::ApplicationProcessNamingInformation>::const_iterator it;
		std::vector<rina::ApplicationRegistration *> registrations =
				rina::extendedIPCManager->getRegisteredApplications();
		for(unsigned int i=0; i<registrations.size(); i++) {
			for(it = registrations[i]->DIFNames.begin();
					it != registrations[i]->DIFNames.end(); ++it) {;
				eiRequest.supporting_difs_.push_back(*it);
			}
		}

		if (ipc_process_->get_address() != 0) {
			was_dif_member_before_enrollment_ = true;
			eiRequest.address_ = ipc_process_->get_address();
		} else {
			rina::DIFInformation difInformation;
			difInformation.dif_name_ = enrollment_request_->event_->difName;
			ipc_process_->set_dif_information(difInformation);
		}

		RIBObjectValue object_value;
		object_value.type_ = RIBObjectValue::complextype;
		object_value.complex_value_ = &eiRequest;
		RemoteIPCProcessId remote_id;
		remote_id.port_id_ = port_id_;

		rib_daemon_->remoteStartObject(EncoderConstants::ENROLLMENT_INFO_OBJECT_CLASS,
				EncoderConstants::ENROLLMENT_INFO_OBJECT_NAME, object_value, 0, remote_id, this);

		LOG_DBG("Sent a M_START Message to portid: %d", port_id_);

		//Set timer
		last_scheduled_task_ = new EnrollmentFailedTimerTask(this, START_RESPONSE_TIMEOUT, true);
		timer_->scheduleTask(last_scheduled_task_, timeout_);

		//Update state
		state_ = STATE_WAIT_START_ENROLLMENT_RESPONSE;
	}catch(Exception &e){
		LOG_ERR("Problems sending M_START request message: %s", e.what());
		//TODO what to do?
	}
}

void EnrolleeStateMachine::startResponse(int result, const std::string& result_reason,
		void * object_value, rina::CDAPSessionDescriptor * session_descriptor) {
	rina::AccessGuard g(*lock_);

	if (!isValidPortId(session_descriptor)){
		return;
	}

	if (state_ != STATE_WAIT_START_ENROLLMENT_RESPONSE) {
		abortEnrollment(remote_peer_->name_, port_id_,
				START_IN_BAD_STATE, true, true);
		return;
	}

	timer_->cancelTask(last_scheduled_task_);
	if (result != 0) {
		state_ = STATE_NULL;
		enrollment_task_->enrollmentFailed(remote_peer_->get_name(), port_id_,
				result_reason, true, true);
		return;
	}

	//Update address
	if (object_value) {
		EnrollmentInformationRequest * response =
				(EnrollmentInformationRequest *) object_value;

		unsigned int address = response->address_;
		delete response;

		try {
			rib_daemon_->writeObject(EncoderConstants::ADDRESS_RIB_OBJECT_CLASS,
					EncoderConstants::ADDRESS_RIB_OBJECT_NAME, &address);
		} catch (Exception &e) {
			LOG_ERR("Problems writing RIB object: %s", e.what());
		}
	}

	//Set timer
	last_scheduled_task_ = new EnrollmentFailedTimerTask(this, STOP_ENROLLMENT_TIMEOUT, true);
	timer_->scheduleTask(last_scheduled_task_, timeout_);

	//Update state
	state_ = STATE_WAIT_STOP_ENROLLMENT_RESPONSE;
}

void EnrolleeStateMachine::stop(bool * start_early, int invoke_id,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	rina::AccessGuard g(*lock_);

	if (!isValidPortId(cdapSessionDescriptor)){
		return;
	}

	if (state_ != STATE_WAIT_STOP_ENROLLMENT_RESPONSE) {
		abortEnrollment(remote_peer_->name_, port_id_,
				STOP_IN_BAD_STATE, true, true);
		return;
	}

	timer_->cancelTask(last_scheduled_task_);
	//Check if I'm allowed to start early
	if (!start_early){
		abortEnrollment(remote_peer_->name_, port_id_, STOP_WITH_NO_OBJECT_VALUE, true, true);
		return;
	}

	allowed_to_start_early_ = *start_early;
	stop_request_invoke_id_ = invoke_id;

	//Request more information or start
	try{
		requestMoreInformationOrStart();
	}catch(Exception &e){
		LOG_ERR("Problems requesting more information or starting: %s", e.what());
		abortEnrollment(remote_peer_->name_, port_id_, std::string(e.what()), true, true);
	}
}

void EnrolleeStateMachine::requestMoreInformationOrStart() {
	if (sendNextObjectRequired()){
		//Set timer
		last_scheduled_task_ = new EnrollmentFailedTimerTask(this, READ_RESPONSE_TIMEOUT, true);
		timer_->scheduleTask(last_scheduled_task_, timeout_);

		//Update state
		state_ = STATE_WAIT_READ_RESPONSE;
		return;
	}

	//No more information is required, if I'm allowed to start early,
	//commit the enrollment information, set operational status to true
	//and send M_STOP_R. If not, just send M_STOP_R
	RIBObjectValue object_value;
	RemoteIPCProcessId remote_id;
	remote_id.port_id_ = port_id_;

	if (allowed_to_start_early_){
		try{
			commitEnrollment();

			rib_daemon_->remoteStopObjectResponse("", "", object_value, 0, "", stop_request_invoke_id_, remote_id);

			enrollmentCompleted();
		}catch(Exception &e){
			LOG_ERR("Problems sending CDAP message: %s", e.what());

			rib_daemon_->remoteStopObjectResponse("", "", object_value, -1,
					PROBLEMS_COMMITTING_ENROLLMENT_INFO, stop_request_invoke_id_, remote_id);

			abortEnrollment(remote_peer_->name_, port_id_, PROBLEMS_COMMITTING_ENROLLMENT_INFO, true, true);
		}

		return;
	}

	try {
		rib_daemon_->remoteStopObjectResponse("", "", object_value, 0, "", stop_request_invoke_id_, remote_id);
	}catch(Exception &e){
		LOG_ERR("Problems sending CDAP message: %s", e.what());
	}

	last_scheduled_task_ = new EnrollmentFailedTimerTask(this, START_TIMEOUT, true);
	timer_->scheduleTask(last_scheduled_task_, timeout_);
	state_ = STATE_WAIT_START;
}

bool EnrolleeStateMachine::sendNextObjectRequired() {
	bool result = false;
	rina::DIFInformation difInformation = ipc_process_->get_dif_information();

	RemoteIPCProcessId remote_id;
	remote_id.port_id_ = port_id_;
	std::string object_class;
	std::string object_name;
	if (!difInformation.dif_configuration_.efcp_configuration_.data_transfer_constants_.isInitialized()) {
		object_class = EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_CLASS;
		object_name = EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_CLASS;
		result = true;
	} else if (difInformation.dif_configuration_.efcp_configuration_.qos_cubes_.size() == 0){
		object_class = EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_CLASS;
		object_name = EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_NAME;
		result = true;
	}else if (ipc_process_->get_neighbors().size() == 0){
		object_class = EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_CLASS;
		object_name = EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_NAME;
		result = true;
	}

	if (result) {
		try{
			rib_daemon_->remoteReadObject(object_class, object_name, 0, remote_id, this);
		} catch (Exception &e) {
			LOG_WARN("Problems executing remote operation: %s", e.what());
		}
	}

	return result;
}

void EnrolleeStateMachine::commitEnrollment() {
	try {
		rib_daemon_->startObject(EncoderConstants::OPERATIONAL_STATUS_RIB_OBJECT_CLASS,
				EncoderConstants::OPERATIONAL_STATUS_RIB_OBJECT_NAME, 0);
	} catch (Exception &e) {
		LOG_ERR("Problems starting RIB object: %s", e.what());
	}
}

void EnrolleeStateMachine::enrollmentCompleted() {
	delete timer_;
	timer_ = 0;
	state_ = STATE_ENROLLED;

	//Create or update the neighbor information in the RIB
	createOrUpdateNeighborInformation(true);

	//Send DirectoryForwardingTableEntries
	sendCreateInformation(EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_CLASS,
			EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_NAME);

	enrollment_task_->enrollmentCompleted(remote_peer_, true);

	//Notify the kernel
	if (!was_dif_member_before_enrollment_) {
		try {
			rina::kernelIPCProcess->assignToDIF(ipc_process_->get_dif_information());
		} catch(Exception &e) {
			LOG_ERR("Problems communicating with the Kernel components of the IPC Processs: %s",
					e.what());
		}
	}

	//Notify the IPC Manager
	if (enrollment_request_){
		try {
			std::list<rina::Neighbor> neighbors;
			neighbors.push_back(*(enrollment_request_->neighbor_));
			rina::extendedIPCManager->enrollToDIFResponse(*(enrollment_request_->event_),
					0, neighbors, ipc_process_->get_dif_information());
		} catch (Exception &e) {
			LOG_ERR("Problems sending message to IPC Manager: %s", e.what());
		}
	}

	LOG_INFO("Remote IPC Process enrolled!");
}

void EnrolleeStateMachine::readResponse(int result, const std::string& result_reason,
		void * object_value, const std::string& object_name,
		rina::CDAPSessionDescriptor * session_descriptor) {
	rina::AccessGuard g(*lock_);

	if (!isValidPortId(session_descriptor)){
		return;
	}

	if (state_ != STATE_WAIT_READ_RESPONSE) {
		abortEnrollment(remote_peer_->name_, port_id_,
				READ_RESPONSE_IN_BAD_STATE, true, true);
		return;
	}

	timer_->cancelTask(last_scheduled_task_);

	if (result != 0 || object_value == 0){
		abortEnrollment(remote_peer_->name_, port_id_,
				result_reason, true, true);
		return;
	}

	if (object_name.compare(EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_NAME) == 0){
		try{
			rina::DataTransferConstants * constants =
					(rina::DataTransferConstants *) object_value;
			rib_daemon_->createObject(EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_CLASS,
					EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_NAME, constants, 0);
		}catch(Exception &e){
			LOG_ERR("Problems creating RIB object: %s", e.what());
		}
	}else if (object_name.compare(EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_NAME) == 0){
		try{
			std::list<rina::QoSCube *> * cubes =
					(std::list<rina::QoSCube *> *) object_value;
			rib_daemon_->createObject(EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_CLASS,
					EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_NAME, cubes, 0);
		}catch(Exception &e){
			LOG_ERR("Problems creating RIB object: %s", e.what());
		}
	}else if (object_name.compare(EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_NAME) == 0){
		try{
			std::list<rina::Neighbor *> * neighbors =
					(std::list<rina::Neighbor *> *) object_value;
			rib_daemon_->createObject(EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_CLASS,
					EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_NAME, neighbors, 0);
		}catch(Exception &e){
			LOG_ERR("Problems creating RIB object: %s", e.what());
		}
	}else{
		LOG_WARN("The object to be created is not required for enrollment");
	}

	//Request more information or proceed with the enrollment program
	requestMoreInformationOrStart();
}

void EnrolleeStateMachine::start(int result, const std::string& result_reason,
		rina::CDAPSessionDescriptor * session_descriptor) {
	rina::AccessGuard g(*lock_);

	if (!isValidPortId(session_descriptor)){
		return;
	}

	if (state_ == STATE_ENROLLED) {
		return;
	}

	if (state_ != STATE_WAIT_START) {
		abortEnrollment(remote_peer_->name_, port_id_,
				START_IN_BAD_STATE, true, true);
		return;
	}

	timer_->cancelTask(last_scheduled_task_);

	if (result != 0){
		abortEnrollment(remote_peer_->name_, port_id_,
				result_reason, true, true);
		return;
	}

	try{
		commitEnrollment();
		enrollmentCompleted();
	}catch(Exception &e){
		LOG_ERR("Problems commiting enrollment: %s", e.what());
		abortEnrollment(remote_peer_->name_, port_id_,
				PROBLEMS_COMMITTING_ENROLLMENT_INFO, true, true);
	}
}

//Class EnrollerStateMachine
EnrollerStateMachine::EnrollerStateMachine(IPCProcess * ipc_process,
		const rina::ApplicationProcessNamingInformation& remote_naming_info, int timeout,
		rina::ApplicationProcessNamingInformation * supporting_dif_name):
		BaseEnrollmentStateMachine(ipc_process, remote_naming_info,
					timeout, supporting_dif_name){
	security_manager_ = ipc_process->get_security_manager();
	namespace_manager_ = ipc_process->get_namespace_manager();
	enroller_ = true;
}

EnrollerStateMachine::~EnrollerStateMachine() {
}

void EnrollerStateMachine::connect(int invoke_id, rina::CDAPSessionDescriptor * session_descriptor) {
	rina::AccessGuard g(*lock_);

	if (state_ != STATE_NULL) {
		abortEnrollment(remote_peer_->name_, session_descriptor->port_id_,
				CONNECT_IN_NOT_NULL, false, true);
		return;
	}

	LOG_DBG("Authenticating IPC process %s-%s ...", session_descriptor->dest_ap_name_.c_str(),
			session_descriptor->dest_ap_inst_.c_str());
	remote_peer_->name_.processName = session_descriptor->dest_ap_name_;
	remote_peer_->name_.processInstance = session_descriptor->dest_ap_inst_;

	//TODO Authenticate sender
	LOG_DBG("Authentication successful, deciding if new member can join the DIF...");
	if (!security_manager_->isAllowedToJoinDIF(*remote_peer_)) {
		LOG_WARN("Security Manager rejected enrollment attempt, aborting enrollment");
		abortEnrollment(remote_peer_->name_, port_id_,
				ENROLLMENT_NOT_ALLOWED, false, true);
		return;
	}


	//Send M_CONNECT_R
	port_id_ = session_descriptor->port_id_;
	try{
		RemoteIPCProcessId remote_id;
		remote_id.port_id_ = port_id_;

		rib_daemon_->openApplicationConnectionResponse(rina::CDAPMessage::AUTH_NONE,
				rina::AuthValue(), session_descriptor->dest_ae_inst_, IPCProcess::MANAGEMENT_AE,
				session_descriptor->dest_ap_inst_, session_descriptor->dest_ap_name_, 0, "", session_descriptor->src_ae_inst_,
				IPCProcess::MANAGEMENT_AE, session_descriptor->src_ap_inst_, session_descriptor->src_ap_name_,
				invoke_id, remote_id);

		//Set timer
		last_scheduled_task_ = new EnrollmentFailedTimerTask(this, START_ENROLLMENT_TIMEOUT, true);
		timer_->scheduleTask(last_scheduled_task_, timeout_);
		LOG_DBG("M_CONNECT_R sent to portID %d. Waiting for start enrollment request message", port_id_);

		state_ = STATE_WAIT_START_ENROLLMENT;
	}catch(Exception &e){
		LOG_ERR("Problems sending CDAP message: %s", e.what());
		abortEnrollment(remote_peer_->name_, port_id_,
						std::string(e.what()), false, true);
	}
}

void EnrollerStateMachine::sendNegativeStartResponseAndAbortEnrollment(int result, const std::string&
		resultReason, int invoke_id) {
	try{
		RIBObjectValue robject_value;
		RemoteIPCProcessId remote_id;
		remote_id.port_id_ = port_id_;

		rib_daemon_->remoteStartObjectResponse("", "", robject_value, result,
				resultReason, invoke_id, remote_id);

		abortEnrollment(remote_peer_->name_, port_id_, resultReason, false, true);
	}catch(Exception &e){
		LOG_ERR("Problems sending CDAP message: %s", e.what());
	}
}

void EnrollerStateMachine::sendDIFStaticInformation() {
	sendCreateInformation(EncoderConstants::WHATEVERCAST_NAME_SET_RIB_OBJECT_CLASS,
			EncoderConstants::WHATEVERCAST_NAME_SET_RIB_OBJECT_NAME);

	sendCreateInformation(EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_CLASS,
			EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_NAME);

	sendCreateInformation(EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_CLASS,
			EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_NAME);
}

void EnrollerStateMachine::start(EnrollmentInformationRequest * eiRequest, int invoke_id,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	rina::AccessGuard g(*lock_);

	if (!isValidPortId(cdapSessionDescriptor)){
		return;
	}

	if (state_ != STATE_WAIT_START_ENROLLMENT) {
		abortEnrollment(remote_peer_->name_, port_id_,
				START_IN_BAD_STATE, false, true);
		return;
	}

	timer_->cancelTask(last_scheduled_task_);

	bool requiresInitialization = false;

	LOG_DBG("Remote IPC Process address: %u", eiRequest->address_);

	if (!eiRequest) {
		requiresInitialization = true;
	} else {
		try {
			if (!namespace_manager_->isValidAddress(eiRequest->address_, remote_peer_->name_.processName,
					remote_peer_->name_.processInstance)) {
				requiresInitialization = true;
			}

			std::list<rina::ApplicationProcessNamingInformation>::const_iterator it;
			for (it = eiRequest->supporting_difs_.begin();
					it != eiRequest->supporting_difs_.end(); ++it) {
				remote_peer_->supporting_difs_.push_back(*it);
			}
		}catch (Exception &e) {
			LOG_ERR("%s", e.what());
			sendNegativeStartResponseAndAbortEnrollment(-1, std::string(e.what()), invoke_id);
			return;
		}
	}

	if (requiresInitialization){
		unsigned int address = namespace_manager_->getValidAddress(remote_peer_->name_.processName,
				remote_peer_->name_.processInstance);

		if (address == 0){
			sendNegativeStartResponseAndAbortEnrollment(-1, "Could not assign a valid address", invoke_id);
			return;
		}

		LOG_DBG("Remote IPC Process requires initialization, assigning address %u", address);
		eiRequest->address_ = address;
	}

	try {
		RemoteIPCProcessId remote_id;
		remote_id.port_id_ = port_id_;
		RIBObjectValue object_value;

		if (requiresInitialization) {
			object_value.type_ = RIBObjectValue::complextype;
			object_value.complex_value_ = eiRequest;
		}

		rib_daemon_->remoteStartObjectResponse(EncoderConstants::ENROLLMENT_INFO_OBJECT_CLASS,
				EncoderConstants::ENROLLMENT_INFO_OBJECT_NAME, object_value, 0, "",
				invoke_id, remote_id);

		remote_peer_->address_ = eiRequest->address_;
	} catch (Exception &e) {
		LOG_ERR("Problems sending CDAP message: %s", e.what());
		delete eiRequest;
		sendNegativeStartResponseAndAbortEnrollment(-1, std::string(e.what()), invoke_id);
		return;
	}

	delete eiRequest;

	//If initialization is required send the M_CREATEs
	if (requiresInitialization){
		sendDIFStaticInformation();
	}

	sendDIFDynamicInformation();

	//Send the M_STOP request
	try {
		RemoteIPCProcessId remote_id;
		remote_id.port_id_ = port_id_;
		RIBObjectValue object_value;
		object_value.type_ = RIBObjectValue::booltype;
		object_value.bool_value_ = true;

		rib_daemon_->remoteStopObject(EncoderConstants::ENROLLMENT_INFO_OBJECT_CLASS,
				EncoderConstants::ENROLLMENT_INFO_OBJECT_NAME, object_value, 0, remote_id, this);
	} catch(Exception &e) {
		LOG_ERR("Problems sending CDAP message: %s", e.what());
		sendNegativeStartResponseAndAbortEnrollment(-1, std::string(e.what()), invoke_id);
		return;
	}

	//Set timer
	last_scheduled_task_ = new EnrollmentFailedTimerTask(this, STOP_ENROLLMENT_RESPONSE_TIMEOUT, true);
	timer_->scheduleTask(last_scheduled_task_, timeout_);

	LOG_DBG("Waiting for stop enrollment response message");
	state_ = STATE_WAIT_STOP_ENROLLMENT_RESPONSE;
}

void EnrollerStateMachine::stopResponse(int result, const std::string& result_reason,
		void * object_value, rina::CDAPSessionDescriptor * session_descriptor) {
	rina::AccessGuard g(*lock_);

	(void) object_value;

	if (!isValidPortId(session_descriptor)){
		return;
	}

	if (state_ != STATE_WAIT_STOP_ENROLLMENT_RESPONSE) {
		abortEnrollment(remote_peer_->name_, port_id_,
				STOP_RESPONSE_IN_BAD_STATE, false, true);
		return;
	}

	timer_->cancelTask(last_scheduled_task_);
	if (result != 0){
		state_ = STATE_NULL;
		enrollment_task_->enrollmentFailed(remote_peer_->name_, port_id_,
				result_reason, false, true);
		return;
	}

	try{
		RIBObjectValue robject_value;
		RemoteIPCProcessId remote_id;
		remote_id.port_id_ = port_id_;

		rib_daemon_->remoteStartObject(EncoderConstants::OPERATIONAL_STATUS_RIB_OBJECT_CLASS,
				EncoderConstants::OPERATIONAL_STATUS_RIB_OBJECT_NAME, robject_value, 0, remote_id, 0);
	} catch(Exception &e){
		LOG_ERR("Problems sending CDAP Message: %s", e.what());
	}

	enrollmentCompleted();
}

void EnrollerStateMachine::enrollmentCompleted() {
	delete timer_;
	timer_ = 0;

	state_ = STATE_ENROLLED;

	createOrUpdateNeighborInformation(true);

	enrollment_task_->enrollmentCompleted(remote_peer_, false);

	try {
		std::list<rina::Neighbor> neighbors;
		neighbors.push_back(*remote_peer_);
		rina::extendedIPCManager->notifyNeighborsModified(true, neighbors);
	} catch (Exception &e) {
		LOG_ERR("Problems sending message to IPC Manager: %s", e.what());
	}

	LOG_INFO("Remote IPC Process enrolled!");
}

//Main function of the Neighbor Enroller thread
/** main function of the Netlink message reader thread */
void * doNeighborsEnrollerWork(void * arg) {
	IPCProcess * ipcProcess = (IPCProcess *) arg;
	IEnrollmentTask * enrollmentTask = ipcProcess->get_enrollment_task();
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
				EnrollmentRequest * request = new EnrollmentRequest((*it), 0);
				enrollmentTask->initiateEnrollment(request);
			} else {
				try {
					std::stringstream ss;
					ss<<EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_NAME<<EncoderConstants::SEPARATOR;
					ss<<(*it)->name_.processName;
					ipcProcess->get_rib_daemon()->deleteObject(EncoderConstants::NEIGHBOR_RIB_OBJECT_CLASS,
							ss.str(), 0, 0);
				} catch (Exception &e){
				}
			}

			sleepObject.sleepForMili(configuration.neighbor_enroller_period_in_ms_);
		}
	}

	return (void *) 0;
}

//Class Enrollment Task
EnrollmentTask::EnrollmentTask() {
	ipc_process_ = 0;
	rib_daemon_ = 0;
	resource_allocator_ = 0;
	cdap_session_manager_ = 0;
	namespace_manager_ = 0;
	timeout_ = 10000;
	lock_ = new rina::Lockable();
	neighbors_enroller_ = 0;
}

EnrollmentTask::~EnrollmentTask() {
	if (lock_) {
		delete lock_;
	}

	if (neighbors_enroller_) {
		delete neighbors_enroller_;
	}
}

void EnrollmentTask::set_ipc_process(IPCProcess * ipc_process) {
	ipc_process_ = ipc_process;
	rib_daemon_ = ipc_process->get_rib_daemon();
	cdap_session_manager_ = ipc_process_->get_cdap_session_manager();
	resource_allocator_ = ipc_process_->get_resource_allocator();
	namespace_manager_ = ipc_process_->get_namespace_manager();
	populateRIB();
	subscribeToEvents();
}

void EnrollmentTask::populateRIB() {
	try{
		BaseRIBObject * ribObject = new NeighborSetRIBObject(ipc_process_);
		rib_daemon_->addRIBObject(ribObject);
		ribObject = new EnrollmentRIBObject(ipc_process_);
		rib_daemon_->addRIBObject(ribObject);
		ribObject = new OperationalStatusRIBObject(ipc_process_);
		rib_daemon_->addRIBObject(ribObject);
		ribObject = new AddressRIBObject(ipc_process_);
		rib_daemon_->addRIBObject(ribObject);
	}catch(Exception &e){
		LOG_ERR("Problems adding object to RIB Daemon: %s", e.what());
	}
}

void EnrollmentTask::subscribeToEvents() {
	rib_daemon_->subscribeToEvent(IPCP_EVENT_N_MINUS_1_FLOW_DEALLOCATED, this);
	rib_daemon_->subscribeToEvent(IPCP_EVENT_N_MINUS_1_FLOW_ALLOCATED, this);
	rib_daemon_->subscribeToEvent(IPCP_EVENT_N_MINUS_1_FLOW_ALLOCATION_FAILED, this);
	rib_daemon_->subscribeToEvent(IPCP_EVENT_NEIGHBOR_DECLARED_DEAD, this);
}

void EnrollmentTask::set_dif_configuration(const rina::DIFConfiguration& dif_configuration) {
	timeout_ = dif_configuration.et_configuration_.enrollment_timeout_in_ms_;

	//Add Watchdog RIB object to RIB
	try{
		BaseRIBObject * ribObject = new WatchdogRIBObject(ipc_process_, dif_configuration);
		rib_daemon_->addRIBObject(ribObject);
	}catch(Exception &e){
		LOG_ERR("Problems adding object to RIB Daemon: %s", e.what());
	}

	//Start Neighbors Enroller thread
	rina::ThreadAttributes * threadAttributes = new rina::ThreadAttributes();
	threadAttributes->setJoinable();
	neighbors_enroller_ = new rina::Thread(threadAttributes,
			&doNeighborsEnrollerWork, (void *) ipc_process_);
	LOG_DBG("Started Neighbors enroller thread");
}

const std::list<rina::Neighbor *> EnrollmentTask::get_neighbors() const{
	std::list<rina::Neighbor *> result;
	BaseRIBObject * ribObject = 0;

	try{
		ribObject = rib_daemon_->readObject(EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_CLASS,
				EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_NAME);
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
		result.push_back((rina::Neighbor *) (*it)->get_value());
	}

	return result;
}

BaseEnrollmentStateMachine * EnrollmentTask::getEnrollmentStateMachine(
		const std::string& apName, int portId, bool remove) {
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

bool EnrollmentTask::isEnrolledTo(const std::string& processName) const {
	rina::AccessGuard g(*lock_);

	std::list<BaseEnrollmentStateMachine *> machines = state_machines_.getEntries();
	std::list<BaseEnrollmentStateMachine *>::const_iterator it;
	for (it = machines.begin(); it != machines.end(); ++it) {
		if ((*it)->remote_peer_->name_.processName.compare(processName) == 0 &&
				(*it)->state_ == BaseEnrollmentStateMachine::STATE_ENROLLED) {
			return true;
		}
	}

	return false;
}

const std::list<std::string> EnrollmentTask::get_enrolled_ipc_process_names() const {
	std::list<std::string> result;

	std::list<BaseEnrollmentStateMachine *> machines = state_machines_.getEntries();
	std::list<BaseEnrollmentStateMachine *>::const_iterator it;
	for (it = machines.begin(); it != machines.end(); ++it) {
		result.push_back((*it)->remote_peer_->name_.processName);
	}

	return result;
}

void EnrollmentTask::processEnrollmentRequestEvent(rina::EnrollToDIFRequestEvent* event) {
	//Can only accept enrollment requests if assigned to a DIF
	if (ipc_process_->get_operational_state() != ASSIGNED_TO_DIF) {
		LOG_ERR("Rejected enrollment request since IPC Process is not ASSIGNED to a DIF");
		try {
			rina::extendedIPCManager->enrollToDIFResponse(*event, -1,
					std::list<rina::Neighbor>(), ipc_process_->get_dif_information());
		}catch (Exception &e) {
			LOG_ERR("Problems sending message to IPC Manager: %s", e.what());
		}

		return;
	}

	//Check that the neighbor belongs to the same DIF as this IPC Process
	if (ipc_process_->get_dif_information().get_dif_name().processName.
			compare(event->difName.processName) != 0) {
		LOG_ERR("Was requested to enroll to a neighbor who is member of DIF %s, but I'm member of DIF %s",
				ipc_process_->get_dif_information().get_dif_name().processName.c_str(),
				event->difName.processName.c_str());

		try {
			rina::extendedIPCManager->enrollToDIFResponse(*event, -1,
					std::list<rina::Neighbor>(), ipc_process_->get_dif_information());
		}catch (Exception &e) {
			LOG_ERR("Problems sending message to IPC Manager: %s", e.what());
		}

		return;
	}

	rina::Neighbor * neighbor = new rina::Neighbor();
	neighbor->name_ = event->neighborName;
	neighbor->supporting_dif_name_ = event->supportingDIFName;
	unsigned int address = namespace_manager_->getValidAddress(neighbor->name_.processName,
			neighbor->name_.processInstance);
	if (address != 0) {
		neighbor->address_ = address;
	}

	EnrollmentRequest * request = new EnrollmentRequest(neighbor, event);
	initiateEnrollment(request);
}

void EnrollmentTask::initiateEnrollment(EnrollmentRequest * request) {
	if (isEnrolledTo(request->neighbor_->name_.processName)) {
		LOG_ERR("Already enrolled to IPC Process %s", request->neighbor_->name_.processName.c_str());
		return;
	}

	//Request the allocation of a new N-1 Flow to the destination IPC Process,
	//dedicated to layer management
	//FIXME not providing FlowSpec information
	//FIXME not distinguishing between AEs
	rina::FlowInformation flowInformation;
	flowInformation.remoteAppName = request->neighbor_->name_;
	flowInformation.localAppName = ipc_process_->get_name();
	flowInformation.difName = request->neighbor_->supporting_dif_name_;
	unsigned int handle = -1;
	try {
		handle = resource_allocator_->get_n_minus_one_flow_manager()->allocateNMinus1Flow(flowInformation);
	} catch (Exception &e) {
		LOG_ERR("Problems allocating N-1 flow: %s", e.what());

		if (request->event_) {
			try {
				rina::extendedIPCManager->enrollToDIFResponse(*(request->event_), -1,
						std::list<rina::Neighbor>(), ipc_process_->get_dif_information());
			} catch (Exception &e) {
				LOG_ERR("Problems sending message to IPC Manager: %s", e.what());
			}
		}

		return;
	}

	port_ids_pending_to_be_allocated_.put(handle, request);
}

void EnrollmentTask::deallocateFlow(int portId) {
	try {
		resource_allocator_->get_n_minus_one_flow_manager()->deallocateNMinus1Flow(portId);
	} catch (Exception &e) {
		LOG_ERR("Problems deallocating N-1 flow: %s", e.what());
	}
}

BaseEnrollmentStateMachine * EnrollmentTask::createEnrollmentStateMachine(
		const rina::ApplicationProcessNamingInformation& apNamingInfo, int portId,
		bool enrollee, const rina::ApplicationProcessNamingInformation& supportingDifName) {
	BaseEnrollmentStateMachine * stateMachine = 0;

	if (apNamingInfo.entityName.compare("") == 0 ||
			apNamingInfo.entityName.compare(IPCProcess::MANAGEMENT_AE) == 0) {
		if (enrollee){
			stateMachine = new EnrolleeStateMachine(ipc_process_,
					apNamingInfo, timeout_);
		}else{
			rina::ApplicationProcessNamingInformation * sdname =
					new rina::ApplicationProcessNamingInformation(supportingDifName.processName,
							supportingDifName.processInstance);
			stateMachine = new EnrollerStateMachine(ipc_process_,
					apNamingInfo, timeout_, sdname);
		}

		std::stringstream ss;
		ss<<apNamingInfo.processName<<"-"<<portId;
		state_machines_.put(ss.str(), stateMachine);

		LOG_DBG("Created a new Enrollment state machine for remote IPC process: %s",
				apNamingInfo.getEncodedString().c_str());
		return stateMachine;
	}

	throw Exception("Unknown application entity for enrollment");
}

BaseEnrollmentStateMachine * EnrollmentTask::getEnrollmentStateMachine(
		const rina::CDAPSessionDescriptor * cdapSessionDescriptor, bool remove) {
	try {
		if (ipc_process_->get_name().processName.
				compare(cdapSessionDescriptor->src_ap_name_) == 0) {
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

void EnrollmentTask::connect(int invoke_id,
		rina::CDAPSessionDescriptor * session_descriptor) {
	LOG_DBG("Received M_CONNECT CDAP message from port-id %d",
			session_descriptor->port_id_);

	//1 Find out if the sender is really connecting to us
	if(session_descriptor->src_ap_name_.compare(ipc_process_->get_name().processName)!= 0){
		LOG_WARN("Received an M_CONNECT message whose destination was not this IPC Process, ignoring it");
		return;
	}

	//2 Find out if we are already enrolled to the remote IPC process
	if (isEnrolledTo(session_descriptor->dest_ap_name_)){

		std::string message = "Received an enrollment request for an IPC process I'm already enrolled to";
		LOG_ERR("%s", message.c_str());

		try {
			RemoteIPCProcessId remote_id;
			remote_id.port_id_ = session_descriptor->port_id_;

			rib_daemon_->openApplicationConnectionResponse(rina::CDAPMessage::AUTH_NONE,
					rina::AuthValue(), session_descriptor->dest_ae_inst_, session_descriptor->dest_ae_name_,
					session_descriptor->dest_ap_inst_, session_descriptor->dest_ap_name_, -2, message,
					session_descriptor->src_ae_inst_, session_descriptor->src_ae_name_,
					session_descriptor->src_ap_inst_, session_descriptor->src_ap_name_, invoke_id, remote_id);
		} catch (Exception &e) {
			LOG_ERR("Problems sending CDAP message: %s", e.what());
		}

		deallocateFlow(session_descriptor->port_id_);

		return;
	}

	//3 Initiate the enrollment
	try{
		rina::FlowInformation flowInformation = resource_allocator_->get_n_minus_one_flow_manager()->
				getNMinus1FlowInformation(session_descriptor->port_id_);
		EnrollerStateMachine * enrollmentStateMachine = (EnrollerStateMachine *) createEnrollmentStateMachine(
				session_descriptor->get_destination_application_process_naming_info(),
				session_descriptor->port_id_, false, flowInformation.difName);
		enrollmentStateMachine->connect(invoke_id, session_descriptor);
	}catch(Exception &e){
		LOG_ERR("Problems: %s", e.what());

		try {
			RemoteIPCProcessId remote_id;
			remote_id.port_id_ = session_descriptor->port_id_;

			rib_daemon_->openApplicationConnectionResponse(rina::CDAPMessage::AUTH_NONE,
					rina::AuthValue(), session_descriptor->dest_ae_inst_, session_descriptor->dest_ae_name_,
					session_descriptor->dest_ap_inst_, session_descriptor->dest_ap_name_, -2,
					std::string(e.what()), session_descriptor->src_ae_inst_, session_descriptor->src_ae_name_,
					session_descriptor->src_ap_inst_, session_descriptor->src_ap_name_, invoke_id, remote_id);
		} catch (Exception &e) {
			LOG_ERR("Problems sending CDAP message: %s", e.what());
		}

		deallocateFlow(session_descriptor->port_id_);
	}
}

void EnrollmentTask::connectResponse(int result, const std::string& result_reason,
		rina::CDAPSessionDescriptor * session_descriptor) {
	LOG_DBG("Received M_CONNECT_R cdapMessage from portId %d", session_descriptor->port_id_);

	try{
		EnrolleeStateMachine * stateMachine = (EnrolleeStateMachine*)
				getEnrollmentStateMachine(session_descriptor, false);
		stateMachine->connectResponse(result, result_reason);
	}catch(Exception &e){
		//Error getting the enrollment state machine
		LOG_ERR("Problems getting enrollment state machine: %s", e.what());

		try {
			RemoteIPCProcessId remote_id;
			remote_id.port_id_ = session_descriptor->port_id_;

			rib_daemon_->closeApplicationConnection(remote_id, 0);
		} catch (Exception &e) {
			LOG_ERR("Problems closing application connection: %s", e.what());
		}

		deallocateFlow(session_descriptor->port_id_);
	}
}

void EnrollmentTask::release(int invoke_id,
		rina::CDAPSessionDescriptor * session_descriptor) {
	LOG_DBG("Received M_RELEASE cdapMessage from portId %d", session_descriptor->port_id_);

	try{
		BaseEnrollmentStateMachine * stateMachine = (BaseEnrollmentStateMachine*)
						getEnrollmentStateMachine(session_descriptor, false);
		stateMachine->release(invoke_id, session_descriptor);
	}catch(Exception &e){
		//Error getting the enrollment state machine
		LOG_ERR("Problems getting enrollment state machine: %s", e.what());

		try {
			RemoteIPCProcessId remote_id;
			remote_id.port_id_ = session_descriptor->port_id_;

			rib_daemon_->closeApplicationConnection(remote_id, 0);
		} catch (Exception &e) {
			LOG_ERR("Problems closing application connection: %s", e.what());
		}

		deallocateFlow(session_descriptor->port_id_);
	}
}

void EnrollmentTask::releaseResponse(int result, const std::string& result_reason,
		rina::CDAPSessionDescriptor * session_descriptor) {
	LOG_DBG("Received M_RELEASE_R cdapMessage from portId %d", session_descriptor->port_id_);

	try{
		BaseEnrollmentStateMachine * stateMachine = (BaseEnrollmentStateMachine*)
						getEnrollmentStateMachine(session_descriptor, false);
		stateMachine->releaseResponse(result, result_reason, session_descriptor);
	}catch(Exception &e){
		//Error getting the enrollment state machine
		LOG_ERR("Problems getting enrollment state machine: %s", e.what());

		try {
			RemoteIPCProcessId remote_id;
			remote_id.port_id_ = session_descriptor->port_id_;

			rib_daemon_->closeApplicationConnection(remote_id, 0);
		} catch (Exception &e) {
			LOG_ERR("Problems closing application connection: %s", e.what());
		}

		deallocateFlow(session_descriptor->port_id_);
	}
}

void EnrollmentTask::eventHappened(Event * event) {
	if (event->get_id() == IPCP_EVENT_N_MINUS_1_FLOW_DEALLOCATED){
		NMinusOneFlowDeallocatedEvent * flowEvent = (NMinusOneFlowDeallocatedEvent *) event;
		nMinusOneFlowDeallocated(flowEvent->cdap_session_descriptor_);
	}else if (event->get_id() == IPCP_EVENT_N_MINUS_1_FLOW_ALLOCATED){
		NMinusOneFlowAllocatedEvent * flowEvent = (NMinusOneFlowAllocatedEvent *) event;
		nMinusOneFlowAllocated(flowEvent);
	}else if (event->get_id() == IPCP_EVENT_N_MINUS_1_FLOW_ALLOCATION_FAILED){
		NMinusOneFlowAllocationFailedEvent * flowEvent = (NMinusOneFlowAllocationFailedEvent *) event;
		nMinusOneFlowAllocationFailed(flowEvent);
	}else if (event->get_id() == IPCP_EVENT_NEIGHBOR_DECLARED_DEAD) {
		NeighborDeclaredDeadEvent * deadEvent = (NeighborDeclaredDeadEvent *) event;
		neighborDeclaredDead(deadEvent);
	}
}

void EnrollmentTask::neighborDeclaredDead(NeighborDeclaredDeadEvent * deadEvent) {
	try{
		resource_allocator_->get_n_minus_one_flow_manager()->getNMinus1FlowInformation(
				deadEvent->neighbor_->underlying_port_id_);
	} catch(Exception &e){
		LOG_INFO("The N-1 flow with the dead neighbor has already been deallocated");
		return;
	}

	try{
		LOG_INFO("Requesting the deallocation of the N-1 flow with the dead neibhor");
		resource_allocator_->get_n_minus_one_flow_manager()->deallocateNMinus1Flow(
				deadEvent->neighbor_->underlying_port_id_);
	} catch (Exception &e){
		LOG_ERR("Problems requesting the deallocation of a N-1 flow: %s", e.what());
	}
}

void EnrollmentTask::nMinusOneFlowDeallocated(rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	//1 Check if the flow deallocated was a management flow
	if(!cdapSessionDescriptor){
		return;
	}

	//2 Remove the enrollment state machine from the list
	try{
		BaseEnrollmentStateMachine * enrollmentStateMachine =
				getEnrollmentStateMachine(cdapSessionDescriptor, true);
		if (!enrollmentStateMachine){
			//Do nothing, we had already cleaned up
			return;
		}else{
			enrollmentStateMachine->flowDeallocated(cdapSessionDescriptor);
			delete enrollmentStateMachine;
		}
	}catch(Exception &e){
		LOG_ERR("Problems: %s", e.what());
	}

	//3 Check if we still have connectivity to the neighbor, if not, issue a ConnectivityLostEvent
	std::list<BaseEnrollmentStateMachine *> machines = state_machines_.getEntries();
	std::list<BaseEnrollmentStateMachine *>::const_iterator it;
	for (it = machines.begin(); it!= machines.end(); ++it) {
		if ((*it)->remote_peer_->name_.processName.compare(
				cdapSessionDescriptor->dest_ap_name_) == 0){
			//We still have connectivity with the neighbor, return
			return;
		}
	}

	//We don't have connectivity to the neighbor, issue a Connectivity lost event
	std::list<rina::Neighbor *> neighbors = get_neighbors();
	std::list<rina::Neighbor *>::const_iterator it2;
	for (it2 = neighbors.begin(); it2 != neighbors.end(); ++it2) {
		if ((*it2)->name_.processName.compare(cdapSessionDescriptor->dest_ap_name_) == 0) {
			ConnectiviyToNeighborLostEvent * event2 = new ConnectiviyToNeighborLostEvent((*it2));
			rib_daemon_->deliverEvent(event2);

			//Notify the IPC Manager that we've lost a neighbor
			std::list<rina::Neighbor> lostNeighbors;
			lostNeighbors.push_back(*(*it2));
			try {
				rina::extendedIPCManager->notifyNeighborsModified(false, lostNeighbors);
			} catch (Exception &e) {
				LOG_ERR("Problems communicating with the IPC Manager: %s", e.what());
			}

			return;
		}
	}
}

void EnrollmentTask::nMinusOneFlowAllocated(NMinusOneFlowAllocatedEvent * flowEvent) {
	EnrollmentRequest * request =
			port_ids_pending_to_be_allocated_.erase(flowEvent->handle_);

	if (!request){
		return;
	}

	EnrolleeStateMachine * enrollmentStateMachine = 0;

	//1 Tell the enrollment task to create a new Enrollment state machine
	try{
		enrollmentStateMachine = (EnrolleeStateMachine *) createEnrollmentStateMachine(
				request->neighbor_->name_, flowEvent->flow_information_.portId, true,
				flowEvent->flow_information_.difName);
	}catch(Exception &e){
		LOG_ERR("Problem retrieving enrollment state machine: %s", e.what());
		delete request;
		return;
	}

	//2 Tell the enrollment state machine to initiate the enrollment
	// (will require an M_CONNECT message and a port Id)
	try{
		enrollmentStateMachine->initiateEnrollment(
				request, flowEvent->flow_information_.portId);
	}catch(Exception &e){
		LOG_ERR("Problems initiating enrollment: %s", e.what());
	}

	delete request;
}

void EnrollmentTask::nMinusOneFlowAllocationFailed(NMinusOneFlowAllocationFailedEvent * event) {
	EnrollmentRequest * request =
			port_ids_pending_to_be_allocated_.erase(event->handle_);

	if (!request){
		return;
	}

	LOG_WARN("The allocation of management flow identified by handle %u has failed. Error code: %d",
			event->handle_, event->flow_information_.portId);

	//TODO inform the one that triggered the enrollment?
	if (request->event_) {
		try {
			rina::extendedIPCManager->enrollToDIFResponse(*(request->event_), -1,
					std::list<rina::Neighbor>(), ipc_process_->get_dif_information());
		} catch(Exception &e) {
			LOG_ERR("Could not send a message to the IPC Manager: %s", e.what());
		}
	}

	delete request;
}

void EnrollmentTask::enrollmentFailed(const rina::ApplicationProcessNamingInformation& remotePeerNamingInfo,
		int portId, const std::string& reason, bool enrollee, bool sendReleaseMessage) {
	LOG_ERR("An error happened during enrollment of remote IPC Process %s because of %s",
			remotePeerNamingInfo.getEncodedString().c_str(), reason.c_str());

	//1 Remove enrollment state machine from the store
	BaseEnrollmentStateMachine * stateMachine =
			getEnrollmentStateMachine(remotePeerNamingInfo.processName, portId, true);
	if (!stateMachine) {
		LOG_ERR("Could not find the enrollment state machine associated to neighbor %s and portId %d",
				remotePeerNamingInfo.processName.c_str(), portId);
		return;
	}

	//2 Send message and deallocate flow if required
	if(sendReleaseMessage){
		try {
			RemoteIPCProcessId remote_id;
			remote_id.port_id_ = portId;

			rib_daemon_->closeApplicationConnection(remote_id, 0);
		} catch (Exception &e) {
			LOG_ERR("Problems closing application connection: %s", e.what());
		}

		deallocateFlow(portId);
	}

	//3 In the case of the enrollee state machine, reply to the IPC Manager
	if (enrollee) {
		EnrollmentRequest * request = ((EnrolleeStateMachine *) stateMachine)->enrollment_request_;
		if (request) {
			if (request->event_) {
				try {
					rina::extendedIPCManager->enrollToDIFResponse(*(request->event_), -1,
							std::list<rina::Neighbor>(), ipc_process_->get_dif_information());
				} catch (Exception &e) {
					LOG_ERR("Problems sending message to IPC Manager: %s", e.what());
				}
			}
		}
	}

	delete stateMachine;
}

void EnrollmentTask::enrollmentCompleted(rina::Neighbor * neighbor, bool enrollee) {
	NeighborAddedEvent * event = new NeighborAddedEvent(neighbor, enrollee);
	rib_daemon_->deliverEvent(event);
}

//Class EnrollmentRIBObject
EnrollmentRIBObject::EnrollmentRIBObject(IPCProcess * ipc_process) :
	BaseRIBObject(ipc_process, EncoderConstants::ENROLLMENT_INFO_OBJECT_CLASS,
			objectInstanceGenerator->getObjectInstance(), EncoderConstants::ENROLLMENT_INFO_OBJECT_NAME) {
	enrollment_task_ = (EnrollmentTask *) ipc_process->get_enrollment_task();
	cdap_session_manager_ = ipc_process->get_cdap_session_manager();
}

const void* EnrollmentRIBObject::get_value() const {
	return 0;
}

void EnrollmentRIBObject::remoteStartObject(void * object_value, int invoke_id,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	EnrollerStateMachine * stateMachine = 0;

	try {
		stateMachine = (EnrollerStateMachine *) enrollment_task_->getEnrollmentStateMachine(
				cdapSessionDescriptor->dest_ap_name_, cdapSessionDescriptor->port_id_, false);
	} catch (Exception &e) {
		LOG_ERR("Problems retrieving state machine: %s", e.what());
		sendErrorMessage(cdapSessionDescriptor);
		return;
	}

	if (!stateMachine) {
		LOG_ERR("Got a CDAP message that is not for me ");
		return;
	}

	EnrollmentInformationRequest * eiRequest = 0;
	if (object_value) {
		eiRequest = (EnrollmentInformationRequest *) object_value;
	}
	stateMachine->start(eiRequest, invoke_id, cdapSessionDescriptor);
}

void EnrollmentRIBObject::remoteStopObject(void * object_value, int invoke_id,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	EnrolleeStateMachine * stateMachine = 0;

	try {
		stateMachine = (EnrolleeStateMachine *) enrollment_task_->getEnrollmentStateMachine(
				cdapSessionDescriptor->dest_ap_name_, cdapSessionDescriptor->port_id_, false);
	} catch (Exception &e) {
		LOG_ERR("Problems retrieving state machine: %s", e.what());
		sendErrorMessage(cdapSessionDescriptor);
		return;
	}

	if (!stateMachine) {
		LOG_ERR("Got a CDAP message that is not for me");
		return;
	}

	bool * start_early = 0;
	if (object_value) {
		rina::BooleanObjectValue * value = (rina::BooleanObjectValue*) object_value;
		start_early = ((bool*)value->get_value());
	}
	stateMachine->stop(start_early, invoke_id , cdapSessionDescriptor);
}

void EnrollmentRIBObject::sendErrorMessage(const rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	try{
		RemoteIPCProcessId remote_id;
		remote_id.port_id_ = cdapSessionDescriptor->port_id_;

		rib_daemon_->closeApplicationConnection(remote_id, 0);
	} catch (Exception &e) {
		LOG_ERR("Problems sending CDAP message: %s", e.what());
	}
}

// Class Operational Status RIB Object
OperationalStatusRIBObject::OperationalStatusRIBObject(IPCProcess * ipc_process) :
		BaseRIBObject(ipc_process, EncoderConstants::OPERATIONAL_STATUS_RIB_OBJECT_CLASS,
						objectInstanceGenerator->getObjectInstance(),
						EncoderConstants::OPERATIONAL_STATUS_RIB_OBJECT_NAME) {
	enrollment_task_ = (EnrollmentTask *) ipc_process->get_enrollment_task();
	cdap_session_manager_ = ipc_process->get_cdap_session_manager();
}

void OperationalStatusRIBObject::remoteStartObject(void * object_value, int invoke_id,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	(void) object_value;
	(void) invoke_id;

	try {
		if (!enrollment_task_->getEnrollmentStateMachine(
				cdapSessionDescriptor->dest_ap_name_, cdapSessionDescriptor->port_id_, false)) {
			LOG_ERR("Got a CDAP message that is not for me: %s");
			return;
		}
	} catch (Exception &e) {
		LOG_ERR("Problems retrieving state machine: %s", e.what());
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
		RemoteIPCProcessId remote_id;
		remote_id.port_id_ = cdapSessionDescriptor->port_id_;

		rib_daemon_->closeApplicationConnection(remote_id, 0);
	} catch (Exception &e) {
		LOG_ERR("Problems sending CDAP message: %s", e.what());
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

	for (int i = 0; i < gpb_eir.supportingdifs_size(); ++i) {
		request->supporting_difs_.push_back(rina::ApplicationProcessNamingInformation(
				gpb_eir.supportingdifs(i), ""));
	}

	return (void *) request;
}

}
