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

}
