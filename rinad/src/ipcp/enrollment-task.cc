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

#define RINA_PREFIX "enrollment-task"

#include <librina/logs.h>
#include "enrollment-task.h"

namespace rinad {

//	CLASS EnrollmentInformationRequest
EnrollmentInformationRequest::EnrollmentInformationRequest() {
	address_ = 0;
}

unsigned int EnrollmentInformationRequest::get_address() const {
	return address_;
}

void EnrollmentInformationRequest::set_address(unsigned int address) {
	address_ = address;
}

const std::list<rina::ApplicationProcessNamingInformation>&
EnrollmentInformationRequest::get_supporting_difs() const {
	return supporting_difs_;
}

void EnrollmentInformationRequest::set_supporting_difs(
		const std::list<rina::ApplicationProcessNamingInformation> &supporting_difs) {
	supporting_difs_ = supporting_difs;
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

void WatchdogRIBObject::remoteReadObject(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	rina::AccessGuard g(*lock_);
	const rina::CDAPMessage * responseMessage = 0;

	//1 Send M_READ_R message
	try {
		responseMessage = cdap_session_manager_->getReadObjectResponseMessage(rina::CDAPMessage::NONE_FLAGS,
				class_, instance_, name_, 0, 0, "", cdapMessage->invoke_id_);
		rib_daemon_->sendMessage(*responseMessage, cdapSessionDescriptor->port_id_, 0);
		delete responseMessage;
	} catch(Exception &e) {
		LOG_ERR("Problems creating or sending CDAP Message: %s", e.what());
		delete responseMessage;
	}

	//2 Update last heard from attribute of the relevant neighbor
	std::list<rina::Neighbor*> neighbors = ipc_process_->get_neighbors();
	std::list<rina::Neighbor*>::const_iterator it;
	for (it = neighbors.begin(); it != neighbors.end(); ++it) {
		if ((*it)->name_.processName.compare(cdapSessionDescriptor->dest_ap_name_) == 0) {
			rina::Time currentTime;
			(*it)->last_heard_from_time_in_ms_ = currentTime.get_current_time_in_ms();
			break;
		}
	}
}

void WatchdogRIBObject::sendMessages() {
	rina::AccessGuard g(*lock_);
	const rina::CDAPMessage * cdapMessage = 0;

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
			cdapMessage = cdap_session_manager_->getReadObjectRequestMessage((*it)->underlying_port_id_,
					0, rina::CDAPMessage::NONE_FLAGS, class_, instance_, name_, 0, true);
			rib_daemon_->sendMessage(*cdapMessage, (*it)->underlying_port_id_, this);
			neighbor_statistics_[(*it)->name_.processName] = currentTimeInMs;
		}catch(Exception &e){
			LOG_ERR("Problems generating or sending CDAP message: %s", e.what());
			delete cdapMessage;
		}
	}
}

void WatchdogRIBObject::readResponse(const rina::CDAPMessage * cdapMessage,
				rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	rina::AccessGuard g(*lock_);

	if (!cdapMessage) {
		return;
	}

	std::map<std::string, int>::iterator it = neighbor_statistics_.find(cdapSessionDescriptor->dest_ap_name_);
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
		if ((*it2)->name_.processName.compare(cdapSessionDescriptor->dest_ap_name_) == 0) {
			(*it2)->average_rtt_in_ms_ = currentTimeInMs - storedTime;
			(*it2)->last_heard_from_time_in_ms_ = currentTimeInMs;
			break;
		}
	}
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

void NeighborSetRIBObject::remoteCreateObject(const rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	rina::AccessGuard g(*lock_);
	std::list<rina::Neighbor *> neighborsToCreate;

	(void) cdapSessionDescriptor; // Stop compiler barfs

	try {
		rina::ByteArrayObjectValue * value = (rina::ByteArrayObjectValue*)  cdapMessage->get_obj_value();
		rina::SerializedObject * serializedObject = (rina::SerializedObject *) value->get_value();

		if (cdapMessage->get_obj_name().compare(EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_NAME) == 0) {
			std::list<rina::Neighbor *> * neighbors =
					(std::list<rina::Neighbor *> *)
					encoder_->decode(*serializedObject,
							EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_CLASS);
			std::list<rina::Neighbor *>::const_iterator iterator;
			for(iterator = neighbors->begin(); iterator != neighbors->end(); ++iterator) {
				populateNeighborsToCreateList(*iterator, &neighborsToCreate);
			}

			delete neighbors;
		} else {
			rina::Neighbor * neighbor = (rina::Neighbor *)
									encoder_->decode(*serializedObject,
											EncoderConstants::NEIGHBOR_RIB_OBJECT_CLASS);
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
	BaseRIBObject * ribObject = new SimpleSetMemberRIBObject(ipc_process_,
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
	ipc_process_->get_dif_information().dif_configuration_.address_ = *address;
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

BaseEnrollmentStateMachine::BaseEnrollmentStateMachine(IRIBDaemon * rib_daemon,
		rina::CDAPSessionManagerInterface * cdap_session_manager, Encoder * encoder,
		const rina::ApplicationProcessNamingInformation& remote_naming_info, IEnrollmentTask * enrollment_task,
		int timeout, rina::ApplicationProcessNamingInformation * supporting_dif_name) {
	rib_daemon_ = rib_daemon;
	cdap_session_manager_ = cdap_session_manager;
	encoder_ = encoder;
	enrollment_task_ = enrollment_task;
	timeout_ = timeout;
	timer_ = new rina::Timer();
	lock_ = new rina::Lockable();
	remote_peer_ = new rina::Neighbor();
	remote_peer_->name_ = remote_naming_info;
	if (supporting_dif_name) {
		remote_peer_->supporting_dif_name_ = *supporting_dif_name;
	}
	port_id_ = 0;
	state_ = STATE_NULL;
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
	rina::AccessGuard g(*lock_);

	if (timer_) {
		delete timer_;
		timer_ = 0;
	}

	state_ = STATE_NULL;

	enrollment_task_->enrollmentFailed(remotePeerNamingInfo, portId, reason, enrollee, sendReleaseMessage);
}

bool BaseEnrollmentStateMachine::isValidPortId(rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	if (cdapSessionDescriptor->port_id_ != port_id_) {
		LOG_ERR("Received a CDAP message form port-id %d, but was expecting it form port-id %d",
				cdapSessionDescriptor->port_id_, port_id_);
		return false;
	}

	return true;
}

void BaseEnrollmentStateMachine::release(rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	LOG_DBG("Releasing the CDAP connection");

	if (!isValidPortId(cdapSessionDescriptor)) {
		return;
	}

	rina::AccessGuard g(*lock_);

	createOrUpdateNeighborInformation(false);

	state_ = STATE_NULL;
	if (timer_) {
		delete timer_;
		timer_ = 0;
	}

	if (cdapMessage->get_invoke_id() == 0) {
		return;
	}

	const rina::CDAPMessage * responseMessage = 0;
	try {
		responseMessage = cdap_session_manager_->getReleaseConnectionResponseMessage(rina::CDAPMessage::NONE_FLAGS,
				0, "", cdapMessage->invoke_id_);
		rib_daemon_->sendMessage(*responseMessage, port_id_, 0);
		delete responseMessage;
	} catch (Exception &e) {
		LOG_ERR("Problems generating or sending CDAP Message: %s", e.what());
		delete responseMessage;
	}
}

void BaseEnrollmentStateMachine::releaseResponse(rina::CDAPMessage * cdapMessage,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	if (!cdapMessage) {
		return;
	}

	if (!isValidPortId(cdapSessionDescriptor)) {
		return;
	}

	rina::AccessGuard g(*lock_);

	if (state_ != STATE_NULL) {
		state_ = STATE_NULL;
	}
}

void BaseEnrollmentStateMachine::flowDeallocated(rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	LOG_INFO("The flow supporting the CDAP session identified by %d has been deallocated.",
			cdapSessionDescriptor->port_id_);

	if (!isValidPortId(cdapSessionDescriptor)){
		return;
	}

	rina::AccessGuard g(*lock_);

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

void BaseEnrollmentStateMachine::sendDIFDynamicInformation(IPCProcess * ipcProcess) {
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
	const rina::CDAPMessage * cdapMessage = 0;
	const rina::SerializedObject * serializedObject = 0;
	try {
		neighborSet = rib_daemon_->readObject(EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_CLASS,
				EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_NAME);
		for (it = neighborSet->get_children().begin();
				it != neighborSet->get_children().end(); ++it) {
			neighbors.push_back((rina::Neighbor*) (*it)->get_value());
		}

		myself = new rina::Neighbor();
		myself->address_ = ipcProcess->get_address();
		myself->name_ = ipcProcess->get_name();
		registrations = rina::extendedIPCManager->getRegisteredApplications();
		for (unsigned int i=0; i<registrations.size(); i++) {
			for(it2 = registrations[i]->DIFNames.begin();
					it2 != registrations[i]->DIFNames.end(); ++it2) {
				myself->add_supporting_dif((*it2));
			}
		}

		serializedObject = encoder_->encode(&neighbors, EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_CLASS);
		rina::ByteArrayObjectValue objectValue = rina::ByteArrayObjectValue(
				*serializedObject);
		cdapMessage = cdap_session_manager_->getCreateObjectRequestMessage(
				port_id_, 0, rina::CDAPMessage::NONE_FLAGS, EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_CLASS,
				0, EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_NAME, &objectValue, 0, false);
		rib_daemon_->sendMessage(*cdapMessage, port_id_, 0);
	} catch (Exception &e) {
		LOG_ERR("Problems sending neighbors: %s", e.what());
	}

	delete myself;
	delete cdapMessage;
	delete serializedObject;
}

void BaseEnrollmentStateMachine::sendCreateInformation(const std::string& objectClass,
		const std::string& objectName) {
	BaseRIBObject * ribObject = 0;
	const rina::CDAPMessage * cdapMessage = 0;
	const rina::SerializedObject * serializedObject = 0;

	try {
		ribObject = rib_daemon_->readObject(objectClass, objectName);
	} catch (Exception &e) {
		LOG_ERR("Problems reading object from RIB: %s", e.what());
		return;
	}

	try {
		serializedObject = encoder_->encode(ribObject->get_value(), objectClass);
		rina::ByteArrayObjectValue objectValue = rina::ByteArrayObjectValue(
				*serializedObject);
		cdapMessage = cdap_session_manager_->getCreateObjectRequestMessage(
				port_id_, 0, rina::CDAPMessage::NONE_FLAGS, objectClass, 0, objectName,
				&objectValue, 0, false);
		rib_daemon_->sendMessage(*cdapMessage, port_id_, 0);
		delete cdapMessage;
		delete serializedObject;
	} catch (Exception &e) {
		LOG_ERR("Problems generating or sending CDAP message: %s", e.what());
		delete cdapMessage;
		delete serializedObject;
	}
}

// Class EnrolleeStateMachine
EnrolleeStateMachine::EnrolleeStateMachine(IPCProcess * ipc_process,
		const rina::ApplicationProcessNamingInformation& remote_naming_info,
		int timeout): BaseEnrollmentStateMachine(ipc_process->get_rib_daemon(),
				ipc_process->get_cdap_session_manager(), ipc_process->get_encoder(),
				remote_naming_info, ipc_process->get_enrollment_task(), timeout, 0) {
	ipc_process_ = ipc_process;
	was_dif_member_before_enrollment_ = false;
	enrollment_request_ = 0;
	lock_ = new rina::Lockable();
	last_scheduled_task_ = 0;
	allowed_to_start_early_ = false;
	stop_enrollment_request_message_ = 0;
}

EnrolleeStateMachine::~EnrolleeStateMachine() {
	if (lock_) {
		delete lock_;
	}

	if (stop_enrollment_request_message_) {
		delete stop_enrollment_request_message_;
	}
}

void EnrolleeStateMachine::initiateEnrollment(EnrollmentRequest * enrollmentRequest, int portId) {
	rina::AccessGuard g(*lock_);

	enrollment_request_ = enrollmentRequest;
	remote_peer_->address_ = enrollment_request_->neighbor_.address_;
	remote_peer_->name_ = enrollment_request_->neighbor_.name_;
	remote_peer_->supporting_dif_name_ = enrollment_request_->neighbor_.supporting_dif_name_;
	remote_peer_->underlying_port_id_ = enrollment_request_->neighbor_.underlying_port_id_;
	remote_peer_->supporting_difs_ = enrollment_request_->neighbor_.supporting_difs_;

	if (state_ != STATE_NULL) {
		throw Exception("Enrollee state machine not in NULL state");
	}

	const rina::CDAPMessage * cdapMessage = 0;
	try{
		cdapMessage = cdap_session_manager_->getOpenConnectionRequestMessage(portId,
				rina::CDAPMessage::AUTH_NONE, rina::AuthValue(), "", IPCProcess::MANAGEMENT_AE,
				remote_peer_->name_.processInstance, remote_peer_->name_.processName, "",
				IPCProcess::MANAGEMENT_AE, ipc_process_->get_name().processInstance,
				ipc_process_->get_name().processName);

		rib_daemon_->sendMessage(*cdapMessage, portId, 0);
		delete cdapMessage;
		port_id_ = portId;

		//Set timer
		last_scheduled_task_ = new EnrollmentFailedTimerTask(this, CONNECT_RESPONSE_TIMEOUT, true);
		timer_->scheduleTask(last_scheduled_task_, timeout_);

		//Update state
		state_ = STATE_WAIT_CONNECT_RESPONSE;
	}catch(Exception &e){
		LOG_ERR("Problems sending M_CONNECT message: %s", e.what());
		delete cdapMessage;
		abortEnrollment(remote_peer_->name_, port_id_, std::string(e.what()), true, false);
	}
}

void EnrolleeStateMachine::connectResponse(rina::CDAPMessage * cdapMessage,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	(void) cdapSessionDescriptor; // Stop compiler barfs

	rina::AccessGuard g(*lock_);

	if (state_ != STATE_WAIT_CONNECT_RESPONSE) {
		abortEnrollment(remote_peer_->name_, port_id_,
				"Message received in wroing order", true, true);
		return;
	}

	timer_->cancelTask(last_scheduled_task_);
	if (cdapMessage->result_ != 0) {
		state_ = STATE_NULL;
		enrollment_task_->enrollmentFailed(remote_peer_->get_name(), port_id_,
				cdapMessage->result_reason_, true, true);
		return;
	}

	//Send M_START with EnrollmentInformation object
	const rina::CDAPMessage * requestMessage = 0;
	const rina::SerializedObject * serializedObject = 0;
	try{
		EnrollmentInformationRequest eiRequest;
		std::list<rina::ApplicationProcessNamingInformation> supportingDifs;
		std::list<rina::ApplicationProcessNamingInformation>::const_iterator it;
		std::vector<rina::ApplicationRegistration *> registrations =
				rina::extendedIPCManager->getRegisteredApplications();
		for(unsigned int i=0; i<registrations.size(); i++) {
			for(it = registrations[i]->DIFNames.begin();
					it != registrations[i]->DIFNames.end(); ++it) {
				supportingDifs.push_back(*it);
			}
		}
		eiRequest.set_supporting_difs(supportingDifs);

		if (ipc_process_->get_address() != 0) {
			was_dif_member_before_enrollment_ = true;
			eiRequest.set_address(ipc_process_->get_address());
		} else {
			rina::DIFInformation difInformation;
			difInformation.dif_name_ = enrollment_request_->event_.difName;
			ipc_process_->set_dif_information(difInformation);
		}

		serializedObject = encoder_->encode(&eiRequest, EncoderConstants::ENROLLMENT_INFO_OBJECT_CLASS);
		rina::ByteArrayObjectValue objectValue = rina::ByteArrayObjectValue(
				*serializedObject);
		requestMessage = cdap_session_manager_->getStartObjectRequestMessage(port_id_, 0,
				rina::CDAPMessage::NONE_FLAGS, EncoderConstants::ENROLLMENT_INFO_OBJECT_CLASS,
				&objectValue, 0, EncoderConstants::ENROLLMENT_INFO_OBJECT_NAME, 0, true);
		rib_daemon_->sendMessage(*requestMessage, port_id_, this);

		//Set timer
		last_scheduled_task_ = new EnrollmentFailedTimerTask(this, START_RESPONSE_TIMEOUT, true);
		timer_->scheduleTask(last_scheduled_task_, timeout_);

		//Update state
		state_ = STATE_WAIT_START_ENROLLMENT_RESPONSE;
	}catch(Exception &e){
		LOG_ERR("Problems sending M_START request message: %s", e.what());
		//TODO what to do?
	}

	delete requestMessage;
	delete serializedObject;
}

void EnrolleeStateMachine::startResponse(const rina::CDAPMessage * cdapMessage,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	if (!isValidPortId(cdapSessionDescriptor)){
		return;
	}

	rina::AccessGuard g(*lock_);

	if (state_ != STATE_WAIT_START_ENROLLMENT_RESPONSE) {
		abortEnrollment(remote_peer_->name_, port_id_,
				START_IN_BAD_STATE, true, true);
		return;
	}

	timer_->cancelTask(last_scheduled_task_);
	if (cdapMessage->result_ != 0) {
		state_ = STATE_NULL;
		enrollment_task_->enrollmentFailed(remote_peer_->get_name(), port_id_,
				cdapMessage->result_reason_, true, true);
		return;
	}

	//Update address
	if (cdapMessage->obj_value_) {
		rina::ByteArrayObjectValue * value = (rina::ByteArrayObjectValue*)  cdapMessage->get_obj_value();
		rina::SerializedObject * serializedObject = (rina::SerializedObject *) value->get_value();
		EnrollmentInformationRequest * response = (EnrollmentInformationRequest *)
						encoder_->decode(*serializedObject, EncoderConstants::ENROLLMENT_INFO_OBJECT_CLASS);

		unsigned int address = response->get_address();
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

void EnrolleeStateMachine::stop(const rina::CDAPMessage * cdapMessage,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	if (!isValidPortId(cdapSessionDescriptor)){
		return;
	}

	rina::AccessGuard g(*lock_);

	if (state_ != STATE_WAIT_STOP_ENROLLMENT_RESPONSE) {
		abortEnrollment(remote_peer_->name_, port_id_,
				STOP_IN_BAD_STATE, true, true);
		return;
	}

	timer_->cancelTask(last_scheduled_task_);
	//Check if I'm allowed to start early
	if (!cdapMessage->obj_value_){
		abortEnrollment(remote_peer_->name_, port_id_, STOP_WITH_NO_OBJECT_VALUE, true, true);
		return;
	}

	rina::BooleanObjectValue * value = (rina::BooleanObjectValue*) cdapMessage->get_obj_value();
	allowed_to_start_early_ = *((bool*)value->get_value());
	stop_enrollment_request_message_ = cdapMessage;

	//Request more information or start
	try{
		requestMoreInformationOrStart();
	}catch(Exception &e){
		LOG_ERR("Problems requesting more information or starting: %s", e.what());
		abortEnrollment(remote_peer_->name_, port_id_, std::string(e.what()), true, true);
	}
}

void EnrolleeStateMachine::requestMoreInformationOrStart() {
	//Check if more information is required
	const rina::CDAPMessage * readMessage = nextObjectRequired();

	if (readMessage){
		//Request information
		try {
			rib_daemon_->sendMessage(*readMessage, port_id_, this);
		} catch (Exception &e){
			LOG_ERR("Problems sending CDAP message: %s", e.what());
		}

		delete readMessage;

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
	const rina::CDAPMessage * stopResponseMessage = 0;
	if (allowed_to_start_early_){
		try{
			commitEnrollment();
			stopResponseMessage = cdap_session_manager_->getStopObjectResponseMessage(rina::CDAPMessage::NONE_FLAGS,
					0, "", stop_enrollment_request_message_->invoke_id_);
			rib_daemon_->sendMessage(*stopResponseMessage, port_id_, 0);
			enrollmentCompleted();
		}catch(Exception &e){
			LOG_ERR("Problems sending CDAP message: %s", e.what());
			delete stopResponseMessage;
			stopResponseMessage = cdap_session_manager_->getStopObjectResponseMessage(rina::CDAPMessage::NONE_FLAGS,
					-1,PROBLEMS_COMMITTING_ENROLLMENT_INFO, stop_enrollment_request_message_->invoke_id_);
			rib_daemon_->sendMessage(*stopResponseMessage, port_id_, 0);
			abortEnrollment(remote_peer_->name_, port_id_, PROBLEMS_COMMITTING_ENROLLMENT_INFO, true, true);
		}

		delete stopResponseMessage;
		delete stop_enrollment_request_message_;
		return;
	}

	try {
		stopResponseMessage = cdap_session_manager_->getStopObjectResponseMessage(rina::CDAPMessage::NONE_FLAGS,
				0, "", stop_enrollment_request_message_->invoke_id_);
		rib_daemon_->sendMessage(*stopResponseMessage, port_id_, 0);
	}catch(Exception &e){
		LOG_ERR("Problems sending CDAP message: %s", e.what());
	}

	delete stopResponseMessage;
	delete stop_enrollment_request_message_;

	last_scheduled_task_ = new EnrollmentFailedTimerTask(this, START_TIMEOUT, true);
	timer_->scheduleTask(last_scheduled_task_, timeout_);
	state_ = STATE_WAIT_START;
}

const rina::CDAPMessage * EnrolleeStateMachine::nextObjectRequired() const {
	const rina::CDAPMessage * response = 0;
	rina::DIFInformation difInformation = ipc_process_->get_dif_information();

	if (!difInformation.dif_configuration_.efcp_configuration_.data_transfer_constants_.isInitialized()) {
		response = cdap_session_manager_->getReadObjectRequestMessage(port_id_, 0, rina::CDAPMessage::NONE_FLAGS,
				EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_CLASS, 0,
				EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_NAME, 0, true);
	} else if (difInformation.dif_configuration_.efcp_configuration_.qos_cubes_.size() == 0){
		response = cdap_session_manager_->getReadObjectRequestMessage(port_id_, 0, rina::CDAPMessage::NONE_FLAGS,
				EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_CLASS, 0,
				EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_NAME, 0, true);
	}else if (ipc_process_->get_neighbors().size() == 0){
		response = cdap_session_manager_->getReadObjectRequestMessage(port_id_, 0, rina::CDAPMessage::NONE_FLAGS,
				EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_CLASS, 0,
				EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_NAME, 0, true);
	}

	return response;
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

	enrollment_task_->enrollmentCompleted(*remote_peer_, true);

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
			neighbors.push_back(enrollment_request_->neighbor_);
			rina::extendedIPCManager->enrollToDIFResponse(enrollment_request_->event_,
					0, neighbors, ipc_process_->get_dif_information());
		} catch (Exception &e) {
			LOG_ERR("Problems sending message to IPC Manager: %s", e.what());
		}
	}

	LOG_INFO("Remote IPC Process enrolled!");
}

void EnrolleeStateMachine::readResponse(const rina::CDAPMessage * cdapMessage,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	if (!isValidPortId(cdapSessionDescriptor)){
		return;
	}

	rina::AccessGuard g(*lock_);

	if (state_ != STATE_WAIT_READ_RESPONSE) {
		abortEnrollment(remote_peer_->name_, port_id_,
				READ_RESPONSE_IN_BAD_STATE, true, true);
		return;
	}

	timer_->cancelTask(last_scheduled_task_);

	if (cdapMessage->result_ != 0 || cdapMessage->obj_value_== 0){
		abortEnrollment(remote_peer_->name_, port_id_,
				UNSUCCESSFULL_READ_RESPONSE, true, true);
		return;
	}

	if (cdapMessage->obj_name_.compare(EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_NAME) == 0){
		try{
			rina::ByteArrayObjectValue * value = (rina::ByteArrayObjectValue*)  cdapMessage->get_obj_value();
			rina::SerializedObject * serializedObject = (rina::SerializedObject *) value->get_value();
			rina::DataTransferConstants * constants = (rina::DataTransferConstants *)
					encoder_->decode(*serializedObject, EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_CLASS);
			rib_daemon_->createObject(EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_CLASS,
					EncoderConstants::DATA_TRANSFER_CONSTANTS_RIB_OBJECT_NAME, constants, 0);
		}catch(Exception &e){
			LOG_ERR("Problems creating RIB object: %s", e.what());
		}
	}else if (cdapMessage->obj_name_.compare(EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_NAME) == 0){
		try{
			rina::ByteArrayObjectValue * value = (rina::ByteArrayObjectValue*)  cdapMessage->get_obj_value();
			rina::SerializedObject * serializedObject = (rina::SerializedObject *) value->get_value();
			std::list<rina::QoSCube *> * cubes = (std::list<rina::QoSCube *> *)
					encoder_->decode(*serializedObject, EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_CLASS);
			rib_daemon_->createObject(EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_CLASS,
					EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_NAME, cubes, 0);
		}catch(Exception &e){
			LOG_ERR("Problems creating RIB object: %s", e.what());
		}
	}else if (cdapMessage->obj_name_.compare(EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_NAME) == 0){
		try{
			rina::ByteArrayObjectValue * value = (rina::ByteArrayObjectValue*)  cdapMessage->get_obj_value();
			rina::SerializedObject * serializedObject = (rina::SerializedObject *) value->get_value();
			std::list<rina::Neighbor *> * neighbors = (std::list<rina::Neighbor *> *)
					encoder_->decode(*serializedObject, EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_CLASS);
			rib_daemon_->createObject(EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_CLASS,
					EncoderConstants::NEIGHBOR_SET_RIB_OBJECT_NAME, neighbors, 0);
		}catch(Exception &e){
			LOG_ERR("Problems creating RIB object: %s", e.what());
		}
	}else{
		LOG_WARN("The object to be created is not required for enrollment: %s",
				cdapMessage->to_string().c_str());
	}

	//Request more information or proceed with the enrollment program
	requestMoreInformationOrStart();
}

void EnrolleeStateMachine::start(const rina::CDAPMessage * cdapMessage,
		rina::CDAPSessionDescriptor * cdapSessionDescriptor) {
	if (!isValidPortId(cdapSessionDescriptor)){
		return;
	}

	rina::AccessGuard g(*lock_);

	if (state_ == STATE_ENROLLED) {
		return;
	}

	if (state_ != STATE_WAIT_START) {
		abortEnrollment(remote_peer_->name_, port_id_,
				START_IN_BAD_STATE, true, true);
		return;
	}

	timer_->cancelTask(last_scheduled_task_);

	if (cdapMessage->result_ != 0){
		abortEnrollment(remote_peer_->name_, port_id_,
				UNSUCCESSFULL_START, true, true);
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

}
