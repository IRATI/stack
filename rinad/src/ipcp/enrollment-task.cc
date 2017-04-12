//
// Enrollment Task
//
//    Bernat Gaston <bernat.gaston@i2cat.net>
//    Eduard Grasa <eduard.grasa@i2cat.net>
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

#include <sstream>
#include <vector>
#include <assert.h>

//FIXME: Remove include
#include <iostream>

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#define IPCP_MODULE "enrollment-task"
#include "ipcp-logging.h"
#include <librina/timer.h>

#include "common/concurrency.h"
#include "common/encoders/EnrollmentInformationMessage.pb.h"
#include "ipcp/enrollment-task.h"
#include "common/encoder.h"
#include "common/rina-configuration.h"
#include "rib-daemon.h"

namespace rinad {

// Class Neighbor RIB object
const std::string NeighborRIBObj::class_name = "Neighbor";
const std::string NeighborRIBObj::object_name_prefix = "/difManagement/enrollment/neighbors/processName=";

NeighborRIBObj::NeighborRIBObj(const std::string& neigh_key) :
		rina::rib::RIBObj(class_name), neighbor_key(neigh_key)
{
}

const std::string NeighborRIBObj::get_displayable_value() const
{
	std::stringstream ss;
	rina::Neighbor neighbor;
	EnrollmentTask * et = (EnrollmentTask*) rinad::IPCPFactory::getIPCP()->enrollment_task_;

	try {
		neighbor = et->get_neighbor(neighbor_key);
	} catch (rina::Exception &e) {
		ss << "Could not find neighbor with key " << neighbor_key << std::endl;
		return ss.str();
	}

	ss << "Name: " << neighbor.name_.getProcessNamePlusInstance();
	ss << "; Address: " << neighbor.address_;
	ss << "; Enrolled: " << neighbor.enrolled_ << std::endl;
	ss << "; Supporting DIF Name: " << neighbor.supporting_dif_name_.processName;
	ss << "; Underlying port-id: " << neighbor.underlying_port_id_;
	ss << "; Number of enroll. attempts: " << neighbor.number_of_enrollment_attempts_;

	return ss.str();
}

void NeighborRIBObj::read(const rina::cdap_rib::con_handle_t &con,
			  const std::string& fqn,
			  const std::string& class_,
			  const rina::cdap_rib::filt_info_t &filt,
			  const int invoke_id,
			  rina::cdap_rib::obj_info_t &obj_reply,
			  rina::cdap_rib::res_info_t& res)
{
	rina::Neighbor neighbor;
	EnrollmentTask * et = (EnrollmentTask*) rinad::IPCPFactory::getIPCP()->enrollment_task_;

	try {
		neighbor = et->get_neighbor(neighbor_key);
	} catch (rina::Exception &e) {
		res.code_ == rina::cdap_rib::CDAP_ERROR;
		return;
	}

	encoders::NeighborEncoder encoder;
	encoder.encode(neighbor, obj_reply.value_);

	res.code_ = rina::cdap_rib::CDAP_SUCCESS;
}

void NeighborRIBObj::create_cb(const rina::rib::rib_handle_t rib,
	  const rina::cdap_rib::con_handle_t &con,
	  const std::string& fqn, const std::string& class_,
	  const rina::cdap_rib::filt_info_t &filt,
	  const int invoke_id, const rina::ser_obj_t &obj_req,
	  rina::cdap_rib::obj_info_t &obj_reply,
	  rina::cdap_rib::res_info_t& res)
{
	NeighborRIBObj* neighbor;

    (void) con;
    (void) filt;
    (void) invoke_id;
    (void) obj_reply;

    //Set return value
    res.code_ = rina::cdap_rib::CDAP_SUCCESS;

    if (class_ != NeighborRIBObj::class_name)
    {
            LOG_IPCP_ERR("Create operation failed: received an invalid class "
            		"name '%s' during create operation in '%s'",
                    class_.c_str(), fqn.c_str());
            res.code_ = rina::cdap_rib::CDAP_INVALID_OBJ_CLASS;
            return;
    }
    rina::Neighbor neigh;
    // TODO do the decoders/encoders
    encoders::NeighborEncoder encoder;
    encoder.decode(obj_req, neigh);
    createNeighbor(neigh);
}

bool NeighborRIBObj::createNeighbor(rina::Neighbor &object)
{
	LOG_IPCP_DBG("Enrolling to neighbor %s",
		 object.get_name().processName.c_str());

	std::list<rina::ApplicationProcessNamingInformation>::const_iterator it;

	rina::EnrollToDAFRequestEvent event;
	event.neighborName = object.get_name();
	event.dafName = rinad::IPCPFactory::getIPCP()->
    		get_dif_information().dif_name_;
	for (it = object.supporting_difs_.begin();
			it != object.supporting_difs_.end(); ++it) {
		LOG_IPCP_DBG("Supporting DIF name %s", it->processName.c_str());
		event.supportingDIFName.processName = it->processName;
	}
	rinad::IPCPFactory::getIPCP()->enrollment_task_->processEnrollmentRequestEvent(event);

	LOG_IPCP_INFO("IPC Process enrollment to neighbor %s in process",
    	object.get_name().processName.c_str());

	return true;
}


// Class Neighbor RIB object
const std::string NeighborsRIBObj::class_name = "Neighbors";
const std::string NeighborsRIBObj::object_name = "/difManagement/enrollment/neighbors";

NeighborsRIBObj::NeighborsRIBObj(IPCProcess * ipcp) :
		IPCPRIBObj(ipcp, class_name)
{
}

void NeighborsRIBObj::create(const rina::cdap_rib::con_handle_t &con,
			     const std::string& fqn,
			     const std::string& class_,
			     const rina::cdap_rib::filt_info_t &filt,
			     const int invoke_id,
			     const rina::ser_obj_t &obj_req,
			     rina::ser_obj_t &obj_reply,
			     rina::cdap_rib::res_info_t& res)
{
	rina::ScopedLock g(lock);
	encoders::NeighborListEncoder encoder;
	std::list<rina::Neighbor> neighbors;
	std::stringstream ss;
	res.code_ = rina::cdap_rib::CDAP_SUCCESS;

	//1 decode neighbor list from ser_obj_t
	encoder.decode(obj_req, neighbors);

	std::list<rina::Neighbor>::iterator iterator;
	for(iterator = neighbors.begin(); iterator != neighbors.end(); ++iterator) {
		ss.flush();
		ss << NeighborRIBObj::object_name_prefix;
		ss << iterator->name_.processName;

		//2 If neighbor is already in RIB, exit
		if (rib_daemon_->getProxy()->containsObj(rib_daemon_->get_rib_handle(),
							 ss.str()))
			continue;

		//3 Avoid creating myself as a neighbor
		if (iterator->name_.processName.compare(ipc_process_->get_name()) == 0)
			continue;

		//4 Only create neighbours with whom I have an N-1 DIF in common
		std::list<rina::ApplicationProcessNamingInformation>::iterator it;
		rina::IPCResourceManager * irm =
				dynamic_cast<rina::IPCResourceManager*>(ipc_process_->get_ipc_resource_manager());
		bool supportingDifInCommon = false;
		for(it = iterator->supporting_difs_.begin();
				it != iterator->supporting_difs_.end(); ++it) {
			if (irm->isSupportingDIF((*it))) {
				iterator->supporting_dif_name_.processName = it->processName;
				supportingDifInCommon = true;
				break;
			}
		}

		if (!supportingDifInCommon) {
			LOG_IPCP_INFO("Ignoring neighbor %s because we don't have an N-1 DIF in common",
					iterator->name_.processName.c_str());
			continue;
		}

		//5 Add the neighbor
		ipc_process_->enrollment_task_->add_neighbor(*iterator);
	}
}

void NeighborsRIBObj::write(const rina::cdap_rib::con_handle_t &con,
			    const std::string& fqn,
			    const std::string& class_,
			    const rina::cdap_rib::filt_info_t &filt,
			    const int invoke_id,
			    const rina::ser_obj_t &obj_req,
			    rina::ser_obj_t &obj_reply,
			    rina::cdap_rib::res_info_t& res)
{
	rina::ScopedLock g(lock);
	encoders::NeighborListEncoder encoder;
	std::list<rina::Neighbor> neighbors;
	std::stringstream ss;
	res.code_ = rina::cdap_rib::CDAP_SUCCESS;

	//1 decode neighbor list from ser_obj_t
	encoder.decode(obj_req, neighbors);

	std::list<rina::Neighbor>::iterator iterator;
	for(iterator = neighbors.begin(); iterator != neighbors.end(); ++iterator) {
		//Modify the neighbor's address
		ipc_process_->enrollment_task_->update_neighbor_address(*iterator);
	}
}

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
const std::string WatchdogRIBObject::class_name = "watchdog_timer";
const std::string WatchdogRIBObject::object_name = "/difManagement/enrollment/watchdog";

WatchdogRIBObject::WatchdogRIBObject(IPCProcess * ipc_process,
				     int wdog_period_ms,
				     int declared_dead_int_ms) :
		IPCPRIBObj(ipc_process, class_name)
{
	wathchdog_period_ = wdog_period_ms;
	declared_dead_interval_ = declared_dead_int_ms;
	lock_ = new rina::Lockable();
	timer_ = new rina::Timer();
	timer_->scheduleTask(new WatchdogTimerTask(this,
						   timer_,
						   wathchdog_period_),
			     wathchdog_period_);
}

WatchdogRIBObject::~WatchdogRIBObject()
{
	if (timer_) {
		delete timer_;
	}

	if (lock_) {
		delete lock_;
	}
}

void WatchdogRIBObject::read(const rina::cdap_rib::con_handle_t &con,
			     const std::string& fqn,
			     const std::string& class_,
			     const rina::cdap_rib::filt_info_t &filt,
			     const int invoke_id,
			     rina::cdap_rib::obj_info_t &obj_reply,
			     rina::cdap_rib::res_info_t& res)
{
	rina::ScopedLock g(*lock_);
	EnrollmentTask * eTask = (EnrollmentTask *) ipc_process_->enrollment_task_;

	//1 Update last heard from attribute of the relevant neighbor
	eTask->watchdog_read(con.dest_.ap_name_);

	//2 Prepare response (RIBDaemon will send it)
	rina::cdap::IntEncoder encoder;
	encoder.encode(ipc_process_->get_address(), obj_reply.value_);
	res.code_ = rina::cdap_rib::CDAP_SUCCESS;
}

void WatchdogRIBObject::sendMessages() {
	rina::ScopedLock g(*lock_);

	neighbor_statistics_.clear();
	rina::Time currentTime;
	rina::cdap_rib::con_handle_t con;
	int currentTimeInMs = currentTime.get_current_time_in_ms();
	std::list<rina::Neighbor> neighbors =
			ipc_process_->enrollment_task_->get_neighbors();
	std::list<rina::Neighbor>::const_iterator it;
	for (it = neighbors.begin(); it != neighbors.end(); ++it) {
		//Skip non enrolled neighbors
		if (it->enrolled_) {
			continue;
		}

		//Skip neighbors that have sent M_READ messages during the last period
		if (it->last_heard_from_time_in_ms_ + wathchdog_period_ > currentTimeInMs) {
			continue;
		}

		//If we have not heard from the neighbor during long enough, declare the neighbor
		//dead and fire a NEIGHBOR_DECLARED_DEAD event
		if (it->last_heard_from_time_in_ms_ != 0 &&
				it->last_heard_from_time_in_ms_ + declared_dead_interval_ < currentTimeInMs) {
			rina::NeighborDeclaredDeadEvent * event =
					new rina::NeighborDeclaredDeadEvent(*it);
			ipc_process_->internal_event_manager_->deliverEvent(event);
			continue;
		}

		try{
			rina::cdap_rib::obj_info_t obj;
			rina::cdap_rib::flags_t flags;
			rina::cdap_rib::filt_info_t filt;
			obj.class_ = class_name;
			obj.name_ = object_name;
			con.port_id = it->underlying_port_id_;
			con.use_internal_flow = true;

			rib_daemon_->getProxy()->remote_read(con,
							     obj,
							     flags,
							     filt,
							     this);

			neighbor_statistics_[it->name_.processName] = currentTimeInMs;
		}catch(rina::Exception &e){
			LOG_IPCP_ERR("Problems generating or sending CDAP message: %s", e.what());
		}
	}
}

void WatchdogRIBObject::remoteReadResult(const rina::cdap_rib::con_handle_t &con,
		      	      	         const rina::cdap_rib::obj_info_t &obj,
		      	      	         const rina::cdap_rib::res_info_t &res)
{
	rina::ScopedLock g(*lock_);
	EnrollmentTask * eTask = (EnrollmentTask *) ipc_process_->enrollment_task_;

	std::map<std::string, int>::iterator it =
			neighbor_statistics_.find(con.dest_.ap_name_);
	if (it == neighbor_statistics_.end())
		return;

	int storedTime = it->second;
	neighbor_statistics_.erase(it);
	eTask->watchdog_read_result(con.dest_.ap_name_, storedTime);
}

//Class AddressRIBObject
const std::string AddressRIBObject::class_name = "address";
const std::string AddressRIBObject::object_name = "/difManagement/naming/address";

AddressRIBObject::AddressRIBObject(IPCProcess * ipc_process):
	IPCPRIBObj(ipc_process, class_name)
{
}

AddressRIBObject::~AddressRIBObject()
{
}

const std::string AddressRIBObject::get_displayable_value() const
{
    std::stringstream ss;
    ss<<"Address: "<< ipc_process_->get_dif_information().dif_configuration_.address_;
    return ss.str();
}

void AddressRIBObject::read(const rina::cdap_rib::con_handle_t &con,
			    const std::string& fqn,
			    const std::string& class_,
			    const rina::cdap_rib::filt_info_t &filt,
			    const int invoke_id,
			    rina::cdap_rib::obj_info_t &obj_reply,
			    rina::cdap_rib::res_info_t& res)
{
	rina::cdap::IntEncoder encoder;
	encoder.encode(ipc_process_->get_address(), obj_reply.value_);
	res.code_ = rina::cdap_rib::CDAP_SUCCESS;
}

//Main function of the Neighbor Enroller thread
void * doNeighborsEnrollerWork(void * arg)
{
	IPCProcess * ipcProcess = (IPCProcess *) arg;
	IPCPRIBDaemon * ribd = ipcProcess->rib_daemon_;
	EnrollmentTask * enrollmentTask = dynamic_cast<EnrollmentTask *>(ipcProcess->enrollment_task_);
	if (!enrollmentTask) {
		LOG_IPCP_ERR("Error casting IEnrollmentTask to EnrollmentTask");
		return (void *) -1;
	}
	std::list<rina::Neighbor> neighbors;
	std::list<rina::Neighbor>::const_iterator it;
	rina::Sleep sleepObject;

	while(true){
		neighbors = enrollmentTask->get_neighbors();
		for(it = neighbors.begin(); it != neighbors.end(); ++it) {
			if (enrollmentTask->isEnrolledTo(it->name_.processName)) {
				//We're already enrolled to this guy, continue
				continue;
			}

			if (it->number_of_enrollment_attempts_ <
					enrollmentTask->max_num_enroll_attempts_) {
				enrollmentTask->incr_neigh_enroll_attempts(*it);
				rina::EnrollmentRequest request(*it);
				enrollmentTask->initiateEnrollment(request);
			} else {
				enrollmentTask->remove_neighbor(it->name_.getProcessNamePlusInstance());
			}

		}
		sleepObject.sleepForMili(enrollmentTask->neigh_enroll_per_ms_);
	}

	return (void *) 0;
}

//Class IEnrollmentStateMachine
const std::string IEnrollmentStateMachine::STATE_NULL = "NULL";
const std::string IEnrollmentStateMachine::STATE_ENROLLED = "ENROLLED";
const std::string IEnrollmentStateMachine::STATE_TERMINATED = "TERMINATED";

IEnrollmentStateMachine::IEnrollmentStateMachine(IPCProcess * ipcp,
						 const rina::ApplicationProcessNamingInformation& remote_naming_info,
						 int timeout,
						 const rina::ApplicationProcessNamingInformation& supporting_dif_name)
{
	if (!ipcp) {
		throw rina::Exception("Bogus Application Process instance passed");
	}

	ipcp_ = ipcp;
	rib_daemon_ = ipcp_->rib_daemon_;
	enrollment_task_ = ipcp_->enrollment_task_;
	timeout_ = timeout;
	remote_peer_.name_ = remote_naming_info;
	remote_peer_.supporting_dif_name_ = supporting_dif_name;
	last_scheduled_task_ = 0;
	state_ = STATE_NULL;
	auth_ps_ = 0;
	enroller_ = false;
	internal_flow_fd = 0;
}

IEnrollmentStateMachine::~IEnrollmentStateMachine() {
	LOG_IPCP_DBG("Destructor of state machine of port-id %u",
		     con.port_id);
}

void IEnrollmentStateMachine::release(int invoke_id,
		     	     	      const rina::cdap_rib::con_handle_t &con_handle)
{
	rina::ScopedLock g(lock_);
	LOG_IPCP_DBG("Releasing the CDAP connection");

	if (!isValidPortId(con_handle.port_id))
		return;

	reset_state();
}

void IEnrollmentStateMachine::releaseResult(const rina::cdap_rib::res_info_t &res,
		   	   	   	    const rina::cdap_rib::con_handle_t &con_handle)
{
	rina::ScopedLock g(lock_);

	if (!isValidPortId(con_handle.port_id))
		return;

	reset_state();
}

void IEnrollmentStateMachine::flowDeallocated(int portId)
{
	rina::ScopedLock g(lock_);
	LOG_IPCP_INFO("The flow supporting the CDAP session identified by %d has been deallocated.",
			portId);

	if (!isValidPortId(portId)){
		return;
	}

	reset_state();
}

std::string IEnrollmentStateMachine::get_state()
{
	rina::ScopedLock g(lock_);
	return state_;
}

void IEnrollmentStateMachine::reset_state()
{
	if (last_scheduled_task_)
		timer_.cancelTask(last_scheduled_task_);

	createOrUpdateNeighborInformation(false);
	state_ = STATE_TERMINATED;
}

bool IEnrollmentStateMachine::isValidPortId(int portId)
{
	if (portId != con.port_id) {
		LOG_ERR("a CDAP message form port-id %d, but was expecting it form port-id %d",
			portId,
			con.port_id);
		return false;
	}

	return true;
}

void IEnrollmentStateMachine::abortEnrollment(const std::string& reason,
					      bool sendReleaseMessage)
{
	assert(!lock_.trylock());

	reset_state();
	AbortEnrollmentTimerTask * task = new AbortEnrollmentTimerTask(enrollment_task_,
								       remote_peer_.name_,
								       con.port_id,
								       remote_peer_.internal_port_id,
								       reason,
								       sendReleaseMessage);
	timer_.scheduleTask(task, 0);
}

void IEnrollmentStateMachine::createOrUpdateNeighborInformation(bool enrolled)
{
	remote_peer_.enrolled_ = enrolled;
	remote_peer_.number_of_enrollment_attempts_ = 0;
	rina::Time currentTime;
	remote_peer_.last_heard_from_time_in_ms_ = currentTime.get_current_time_in_ms();
	if (enrolled) {
		remote_peer_.underlying_port_id_ = con.port_id;
	} else {
		remote_peer_.underlying_port_id_ = 0;
		remote_peer_.internal_port_id = 0;
	}

	enrollment_task_->add_or_update_neighbor(remote_peer_);
}

void IEnrollmentStateMachine::sendNeighbors()
{
	std::list<rina::Neighbor> neighbors = enrollment_task_->get_neighbors();
	std::list<rina::Neighbor>::iterator it;
	std::list<rina::Neighbor> neighbors_to_send;
	rina::Neighbor myself;
	std::vector<rina::ApplicationRegistration *> registrations;
	std::list<rina::ApplicationProcessNamingInformation>::const_iterator it2;

	try {
		for (it = neighbors.begin(); it != neighbors.end(); ++it)
			neighbors_to_send.push_back(*it);

		myself.address_ = ipcp_->get_address();
		myself.name_.processName = ipcp_->get_name();
		myself.name_.processInstance = ipcp_->get_instance();
		registrations = rina::extendedIPCManager->getRegisteredApplications();

		for (unsigned int i=0; i<registrations.size(); i++) {
			for(it2 = registrations[i]->DIFNames.begin();
					it2 != registrations[i]->DIFNames.end(); ++it2) {
				myself.add_supporting_dif((*it2));
			}
		}
		neighbors_to_send.push_back(myself);


		rina::cdap_rib::obj_info_t obj;
		rina::cdap_rib::flags_t flags;
		rina::cdap_rib::filt_info_t filt;
		encoders::NeighborListEncoder encoder;
		obj.class_ = NeighborsRIBObj::class_name;
		obj.name_ = NeighborsRIBObj::object_name;
		encoder.encode(neighbors_to_send, obj.value_);

		rib_daemon_->getProxy()->remote_create(con,
						       obj,
						       flags,
						       filt,
						       NULL);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems sending neighbors: %s", e.what());
	}
}

//Class AbortEnrollmentTimerTask
AbortEnrollmentTimerTask::AbortEnrollmentTimerTask(rina::IEnrollmentTask * enr_task,
						   const rina::ApplicationProcessNamingInformation& remotePeerNamingInfo,
			 	 	 	   int portId,
						   int int_portId,
						   const std::string& reason_,
						   bool sendReleaseMessage)
{
	etask = enr_task;
	peer_name = remotePeerNamingInfo;
	port_id = portId;
	internal_portId = int_portId;
	reason = reason_;
	send_message = sendReleaseMessage;
}

void AbortEnrollmentTimerTask::run()
{
	etask->enrollmentFailed(peer_name, port_id, internal_portId, reason, send_message);
}

//Class DestroyESMTimerTask
DestroyESMTimerTask::DestroyESMTimerTask(IEnrollmentStateMachine * sm)
{
	state_machine = sm;
}

void DestroyESMTimerTask::run()
{
	delete state_machine;
	state_machine = 0;
}

//Class DeallocateFlowTimerTask
DeallocateFlowTimerTask::DeallocateFlowTimerTask(IPCProcess * ipc_process,
						 int pid,
						 bool intern)
{
	ipcp = ipc_process;
	port_id = pid;
	internal = intern;
}

void DeallocateFlowTimerTask::run()
{
	rina::FlowDeallocateRequestEvent event;

	if (internal) {
		event.internal = true;
		event.portId = port_id;
		ipcp->flow_allocator_->submitDeallocate(event);
	} else {
		ipcp->resource_allocator_->get_n_minus_one_flow_manager()->deallocateNMinus1Flow(port_id);
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
	ipcp_ps = 0;
}

EnrollmentTask::~EnrollmentTask()
{
	delete ps;
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
	subscribeToEvents();
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
	event_manager_->subscribeToEvent(rina::InternalEvent::ADDRESS_CHANGE,
					 this);
	event_manager_->subscribeToEvent(rina::InternalEvent::IPCP_INTERNAL_FLOW_ALLOCATED,
					 this);
	event_manager_->subscribeToEvent(rina::InternalEvent::IPCP_INTERNAL_FLOW_DEALLOCATED,
					 this);
	event_manager_->subscribeToEvent(rina::InternalEvent::IPCP_INTERNAL_FLOW_ALLOCATION_FAILED,
					 this);
}

void EnrollmentTask::eventHappened(rina::InternalEvent * event)
{
	if (event->type == rina::InternalEvent::APP_N_MINUS_1_FLOW_DEALLOCATED){
		rina::NMinusOneFlowDeallocatedEvent * flowEvent =
				(rina::NMinusOneFlowDeallocatedEvent *) event;
		nMinusOneFlowDeallocated(flowEvent);
	} else if (event->type == rina::InternalEvent::APP_N_MINUS_1_FLOW_ALLOCATED){
		rina::NMinusOneFlowAllocatedEvent * flowEvent =
				(rina::NMinusOneFlowAllocatedEvent *) event;
		nMinusOneFlowAllocated(flowEvent);
	} else if (event->type == rina::InternalEvent::APP_N_MINUS_1_FLOW_ALLOCATION_FAILED){
		rina::NMinusOneFlowAllocationFailedEvent * flowEvent =
				(rina::NMinusOneFlowAllocationFailedEvent *) event;
		nMinusOneFlowAllocationFailed(flowEvent);
	} else if (event->type == rina::InternalEvent::APP_NEIGHBOR_DECLARED_DEAD) {
		rina::NeighborDeclaredDeadEvent * deadEvent =
				(rina::NeighborDeclaredDeadEvent *) event;
		neighborDeclaredDead(deadEvent);
	} else if (event->type == rina::InternalEvent::ADDRESS_CHANGE) {
		rina::AddressChangeEvent * addrEvent =
				(rina::AddressChangeEvent *) event;
		addressChange(addrEvent);
	} else if (event->type == rina::InternalEvent::IPCP_INTERNAL_FLOW_ALLOCATED) {
		rina::IPCPInternalFlowAllocatedEvent * inEvent =
				(rina::IPCPInternalFlowAllocatedEvent *) event;
		internal_flow_allocated(inEvent);
	} else if (event->type == rina::InternalEvent::IPCP_INTERNAL_FLOW_ALLOCATION_FAILED) {
		rina::IPCPInternalFlowAllocationFailedEvent * inEvent =
				(rina::IPCPInternalFlowAllocationFailedEvent *) event;
		internal_flow_allocation_failed(inEvent);
	} else if (event->type == rina::InternalEvent::IPCP_INTERNAL_FLOW_DEALLOCATED) {
		rina::IPCPInternalFlowDeallocatedEvent * inEvent =
				(rina::IPCPInternalFlowDeallocatedEvent *) event;
		internal_flow_deallocated(inEvent);
	}
}

void EnrollmentTask::internal_flow_allocated(rina::IPCPInternalFlowAllocatedEvent * event)
{
	std::map<int, IEnrollmentStateMachine*>::iterator it;
	IPCPRIBDaemonImpl * ribd = dynamic_cast<IPCPRIBDaemonImpl *>(ipcp->rib_daemon_);

	rina::ReadScopedLock readLock(sm_lock);

	// Notify state machine
	for (it = state_machines_.begin(); it != state_machines_.end(); ++it) {
		if (it->second->remote_peer_.name_.processName
				== event->flow_info.remoteAppName.processName) {
			ribd->start_internal_flow_sdu_reader(event->port_id,
							     event->fd,
							     it->second->remote_peer_.underlying_port_id_);

			it->second->internal_flow_allocate_result(event->port_id,
								  event->fd,
								  "");
			return;
		}
	}
}

void EnrollmentTask::internal_flow_allocation_failed(rina::IPCPInternalFlowAllocationFailedEvent * event)
{
	std::map<int, IEnrollmentStateMachine*>::iterator it;

	rina::ReadScopedLock readLock(sm_lock);

	for (it = state_machines_.begin(); it != state_machines_.end(); ++it) {
		if (it->second->remote_peer_.name_.processName
				== event->flow_info.remoteAppName.processName) {
			it->second->internal_flow_allocate_result(event->error_code,
								  0,
								  event->reason);
			return;
		}
	}
}

void EnrollmentTask::internal_flow_deallocated(rina::IPCPInternalFlowDeallocatedEvent * event)
{
	std::map<int, IEnrollmentStateMachine*>::iterator it;
	IPCPRIBDaemonImpl * ribd = dynamic_cast<IPCPRIBDaemonImpl*>(ipcp->rib_daemon_);
	IEnrollmentStateMachine * esm = 0;
	rina::ConnectiviyToNeighborLostEvent * event2 = 0;
	DeallocateFlowTimerTask * timer_task = 0;

	rina::ScopedLock g(lock_);
	rina::ReadScopedLock writeLock(sm_lock);

	//1 Stop the internal flow SDU reader
	ribd->stop_internal_flow_sdu_reader(event->port_id);

	//2 Remove the enrollment state machine from the map
	for (it = state_machines_.begin(); it != state_machines_.end(); ++it) {
		if (it->second->remote_peer_.internal_port_id == event->port_id) {
			esm = it->second;
			state_machines_.erase(it);
			break;
		}
	}

	if (!esm){
		//Do nothing, we had already cleaned up
		return;
	}

	esm->flowDeallocated(esm->remote_peer_.underlying_port_id_);

	// Request deallocation of N-1 flow
	timer_task = new DeallocateFlowTimerTask(ipcp,
						 esm->remote_peer_.underlying_port_id_,
						 false);
	timer.scheduleTask(timer_task, 0);

	event2 = new rina::ConnectiviyToNeighborLostEvent(esm->remote_peer_);
	delete esm;
	esm = 0;
	event_manager_->deliverEvent(event2);
}

int EnrollmentTask::get_fd_associated_to_n1flow(int port_id)
{
	std::map<int, IEnrollmentStateMachine*>::iterator it;

	rina::ReadScopedLock readLock(sm_lock);

	for (it = state_machines_.begin(); it != state_machines_.end(); ++it) {
		if (it->second->remote_peer_.underlying_port_id_ == port_id) {
			return it->second->internal_flow_fd;
		}
	}

	return -1;
}

void EnrollmentTask::operational_status_start(int port_id,
			      	      	      int invoke_id,
					      const rina::ser_obj_t &obj_req)
{
	std::map<int, IEnrollmentStateMachine*>::iterator it;

	rina::ReadScopedLock readLock(sm_lock);

	for (it = state_machines_.begin(); it != state_machines_.end(); ++it) {
		if (it->second->remote_peer_.underlying_port_id_ == port_id) {
			it->second->operational_status_start(invoke_id, obj_req);
		}
	}
}

void EnrollmentTask::addressChange(rina::AddressChangeEvent * event)
{
	rina::ScopedLock g(lock_);
	encoders::NeighborListEncoder encoder;
	std::list<rina::Neighbor> neighbors;
	std::map<int, IEnrollmentStateMachine*>::iterator it;
	rina::Neighbor myself;
	rina::cdap_rib::flags_t flags;
	rina::cdap_rib::filt_info_t filt;
	rina::cdap_rib::obj_info_t obj_info;
	rina::cdap_rib::con_handle_t con;

	myself.address_ = event->new_address;
	myself.name_.processName = ipcp->get_name();
	myself.name_.processInstance = ipcp->get_instance();
	neighbors.push_back(myself);
	encoder.encode(neighbors, obj_info.value_);
	obj_info.class_ = NeighborsRIBObj::class_name;
	obj_info.name_ = NeighborsRIBObj::object_name;
	obj_info.inst_ = 0;

	rina::ReadScopedLock readLock(sm_lock);

	for (it = state_machines_.begin(); it != state_machines_.end(); ++it) {
		con.port_id = it->second->remote_peer_.underlying_port_id_;
		try {
			rib_daemon_->getProxy()->remote_write(con,
					obj_info,
					flags,
					filt,
					NULL);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems encoding and sending CDAP message: %s",
					e.what());
		}
	}
}

void EnrollmentTask::update_neighbor_address(const rina::Neighbor& neighbor)
{
	std::map<std::string, rina::Neighbor *>::iterator it;
	rina::NeighborAddressChangeEvent * event = 0;

	rina::ReadScopedLock readLock(neigh_lock);

	it = neighbors.find(neighbor.name_.getProcessNamePlusInstance());
	if (it != neighbors.end()) {
		it->second->old_address_ = it->second->address_;
		it->second->address_ = neighbor.address_;
		event = new rina::NeighborAddressChangeEvent(it->second->name_.processName,
							     it->second->address_,
							     it->second->old_address_);
		LOG_IPCP_INFO("Neighbor %s address changed from %d to %d",
			       neighbor.name_.getProcessNamePlusInstance().c_str(),
			       it->second->old_address_,
			       it->second->address_);
		ipcp->internal_event_manager_->deliverEvent(event);
	} else {
		LOG_IPCP_WARN("Could not change address of neighbor with name: %s",
			      neighbor.name_.getProcessNamePlusInstance().c_str());
	}
}

void EnrollmentTask::incr_neigh_enroll_attempts(const rina::Neighbor& neighbor)
{
	std::map<std::string, rina::Neighbor *>::iterator it;

	rina::ReadScopedLock readLock(neigh_lock);
	it = neighbors.find(neighbor.name_.getProcessNamePlusInstance());
	if (it != neighbors.end()) {
		it->second->number_of_enrollment_attempts_++;
	}
}

void EnrollmentTask::watchdog_read(const std::string& remote_app_name)
{
	std::map<std::string, rina::Neighbor *>::iterator it;

	rina::ReadScopedLock readLock(neigh_lock);

	rina::Time currentTime;
	int current_time_ms = currentTime.get_current_time_in_ms();
	for (it = neighbors.begin(); it != neighbors.end(); ++it) {
		if (it->second->name_.processName == remote_app_name) {
			it->second->last_heard_from_time_in_ms_ = current_time_ms;
			break;
		}
	}
}

void EnrollmentTask::watchdog_read_result(const std::string& remote_app_name,
			  	  	  int stored_time)
{
	std::map<std::string, rina::Neighbor *>::iterator it;

	rina::ReadScopedLock readLock(neigh_lock);

	rina::Time currentTime;
	int current_time_ms = currentTime.get_current_time_in_ms();
	for (it = neighbors.begin(); it != neighbors.end(); ++it) {
		if (it->second->name_.processName == remote_app_name) {
			it->second->average_rtt_in_ms_ = current_time_ms - stored_time;
			it->second->last_heard_from_time_in_ms_ = current_time_ms;
			break;
		}
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
		rina::rib::RIBObj * ribObj = new WatchdogRIBObject(ipcp,
								   watchdog_per_ms_,
								   declared_dead_int_ms_);
		rib_daemon_->addObjRIB(WatchdogRIBObject::object_name,
				       &ribObj);
	}catch(rina::Exception &e){
		LOG_IPCP_ERR("Problems adding object to RIB Daemon: %s", e.what());
	}

	//Start Neighbors Enroller thread
	if (neigh_enroll_per_ms_ > 0) {
		rina::ThreadAttributes * threadAttributes = new rina::ThreadAttributes();
		threadAttributes->setJoinable();
		threadAttributes->setName("neighbor-enroller");
		neighbors_enroller_ = new rina::Thread(&doNeighborsEnrollerWork,
						      (void *) ipcp,
						      threadAttributes);
		neighbors_enroller_->start();
		LOG_IPCP_DBG("Started Neighbors enroller thread");
	}

	//Apply configuration to policy set
	IPCPEnrollmentTaskPS * ipcp_ps = dynamic_cast<IPCPEnrollmentTaskPS *>(ps);
	assert(ipcp_ps);
	ipcp_ps->set_dif_configuration(dif_configuration);
}

void EnrollmentTask::processEnrollmentRequestEvent(const rina::EnrollToDAFRequestEvent& event)
{
	//Can only accept enrollment requests if assigned to a DIF
	if (ipcp->get_operational_state() != ASSIGNED_TO_DIF) {
		LOG_IPCP_ERR("Rejected enrollment request since IPC Process is not ASSIGNED to a DIF");
		try {
			rina::extendedIPCManager->enrollToDIFResponse(event, -1,
					std::list<rina::Neighbor>(), ipcp->get_dif_information());
		}catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems sending message to IPC Manager: %s", e.what());
		}

		return;
	}

	//Check that the neighbor belongs to the same DIF as this IPC Process
	if (ipcp->get_dif_information().get_dif_name().processName.
			compare(event.dafName.processName) != 0) {
		LOG_IPCP_ERR("Was requested to enroll to a neighbor who is member of DIF %s, but I'm member of DIF %s",
				ipcp->get_dif_information().get_dif_name().processName.c_str(),
				event.dafName.processName.c_str());

		try {
			rina::extendedIPCManager->enrollToDIFResponse(event, -1,
					std::list<rina::Neighbor>(), ipcp->get_dif_information());
		}catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems sending message to IPC Manager: %s", e.what());
		}

		return;
	}

	INamespaceManagerPs *nsmps = dynamic_cast<INamespaceManagerPs *>(namespace_manager_->ps);
	assert(nsmps);

	rina::Neighbor neighbor;
	neighbor.supporting_dif_name_ = event.supportingDIFName;
	if (event.neighborName.processName == "") {
		//Enrolling using the DIF name, not a specific DAP
		neighbor.name_ = event.dafName;
	} else {
		neighbor.name_ = event.neighborName;
		unsigned int address = nsmps->getValidAddress(neighbor.name_.processName,
				neighbor.name_.processInstance);
		if (address != 0) {
			neighbor.address_ = address;
		}
	}

	rina::EnrollmentRequest request(neighbor, event);
	initiateEnrollment(request);
}

void EnrollmentTask::processDisconnectNeighborRequestEvent(const rina::DisconnectNeighborRequestEvent& event)
{
	rina::ScopedLock g(lock_);
	IEnrollmentStateMachine * esm = 0;
	std::map<int, IEnrollmentStateMachine *>::iterator it;
	DeallocateFlowTimerTask * df_ttask = 0;

	rina::WriteScopedLock writeLock(sm_lock);
	for (it = state_machines_.begin(); it != state_machines_.end(); ++it) {
		if (it->second->remote_peer_.name_.processName.compare(event.neighborName.processName) == 0 &&
				it->second->get_state() != IEnrollmentStateMachine::STATE_NULL) {
			esm = it->second;
		}
	}

	if (!esm) {
		LOG_IPCP_ERR("Not enrolled to IPC Process %s",
			     event.neighborName.processName.c_str());
		try {
			rina::extendedIPCManager->disconnectNeighborResponse(event, -1);
		}catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems sending message to IPC Manager: %s", e.what());
		}
		return;
	}

	// Close the application connection by sending M_RELEASE
	try {
		rib_daemon_->getProxy()->remote_close_connection(esm->con.port_id, false);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems closing application connection: %s",
			      e.what());
	}

	// Request deallocation of all N-1 flows to neighbor
	deallocateFlow(esm->con.port_id);

	// Request deallocation of reliable N flow to a timer task
	if (esm->remote_peer_.internal_port_id != 0) {
		df_ttask = new DeallocateFlowTimerTask(ipcp,
						       esm->remote_peer_.internal_port_id,
						       true);
	}

	// Update and defer deletion of state machine to a timer task (takes long)
	state_machines_.erase(esm->con.port_id);
	esm->reset_state();
	DestroyESMTimerTask * dsm_task = new DestroyESMTimerTask(esm);
	timer.scheduleTask(dsm_task, 0);

	// Reply to IPC Manager
	try {
		rina::extendedIPCManager->disconnectNeighborResponse(event, 0);
	}catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems sending message to IPC Manager: %s", e.what());
	}
}

void EnrollmentTask::initiateEnrollment(const rina::EnrollmentRequest& request)
{
	rina::ScopedLock g(lock_);

	if (isEnrolledTo(request.neighbor_.name_.processName)) {
		LOG_IPCP_ERR("Already enrolled to IPC Process %s",
			     request.neighbor_.name_.processName.c_str());
		return;
	}

	//Request the allocation of a new N-1 Flow to the destination IPC Process,
	//dedicated to layer management
	//FIXME not providing FlowSpec information
	//FIXME not distinguishing between AEs
	rina::FlowInformation flowInformation;
	flowInformation.remoteAppName = request.neighbor_.name_;
	flowInformation.localAppName.processName = ipcp->get_name();
	flowInformation.localAppName.processInstance = ipcp->get_instance();
	flowInformation.difName = request.neighbor_.supporting_dif_name_;
	unsigned int handle = -1;
	try {
		handle = irm_->allocateNMinus1Flow(flowInformation);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems allocating N-1 flow: %s", e.what());

		if (request.ipcm_initiated_) {
			try {
				rina::extendedIPCManager->enrollToDIFResponse(request.event_,
									      -1,
									      std::list<rina::Neighbor>(),
									      ipcp->get_dif_information());
			} catch (rina::Exception &e) {
				LOG_IPCP_ERR("Problems sending message to IPC Manager: %s", e.what());
			}
		}

		return;
	}

	rina::EnrollmentRequest * to_store = new rina::EnrollmentRequest(request.neighbor_,
								         request.event_);
	to_store->ipcm_initiated_ = request.ipcm_initiated_;
	port_ids_pending_to_be_allocated_.put(handle, to_store);
}

void EnrollmentTask::connect(const rina::cdap::CDAPMessage& cdap_m,
		     	     rina::cdap_rib::con_handle_t &con_handle)
{
	LOG_IPCP_DBG("M_CONNECT CDAP message from port-id %u",
		     con_handle.port_id);

	//0 Check if the request is to the DAF name, if so change to IPCP name
	if (con_handle.src_.ap_name_ == ipcp->get_dif_information().dif_name_.processName) {
		con_handle.src_.ap_name_ = ipcp->get_name();
		con_handle.src_.ap_inst_ = ipcp->get_instance();
	}

	//1 Find out if the sender is really connecting to us
	if(con_handle.src_.ap_name_ != ipcp->get_name()){
		LOG_IPCP_WARN("an M_CONNECT message whose destination was not this IPC Process, ignoring it");
		return;
	}

	rina::ScopedLock g(lock_);

	//2 Find out if we are already enrolled to the remote IPC process
	if (isEnrolledTo(con_handle.dest_.ap_name_)){
		std::string message = "an enrollment request for an IPC process I'm already enrolled to";
		LOG_IPCP_ERR("%s", message.c_str());

		try {
			rina::cdap_rib::res_info_t res;
			res.code_ = rina::cdap_rib::CDAP_APP_CONNECTION_REJECTED;

			rina::cdap::getProvider()->send_open_connection_result(con_handle,
									       res,
									       cdap_m.invoke_id_);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems sending CDAP message: %s",
				     e.what());
		}

		deallocateFlow(con_handle.port_id);

		return;
	}

	//3 Delegate further processing to the policy
	IPCPEnrollmentTaskPS * ipcp_ps = dynamic_cast<IPCPEnrollmentTaskPS *>(ps);
	assert(ipcp_ps);
	ipcp_ps->connect_received(cdap_m, con_handle);
}

void EnrollmentTask::connectResult(const rina::cdap::CDAPMessage &msg,
				   rina::cdap_rib::con_handle_t &con_handle)
{
	LOG_IPCP_DBG("M_CONNECT_R cdapMessage from portId %u",
		     con_handle.port_id);

	rina::ScopedLock g(lock_);

	//In case the application connection was to the DAF name, now
	//update it to use the specific DAP name and instance
	con_handle.dest_.ap_name_ = msg.src_ap_name_;
	con_handle.dest_.ap_inst_ = msg.src_ap_inst_;

	IPCPEnrollmentTaskPS * ipcp_ps = dynamic_cast<IPCPEnrollmentTaskPS *>(ps);
	assert(ipcp_ps);
	ipcp_ps->connect_response_received(static_cast<rina::cdap_rib::res_code_t>(msg.result_),
					   msg.result_reason_,
					   con_handle,
					   msg.auth_policy_);
}

void EnrollmentTask::releaseResult(const rina::cdap_rib::res_info_t &res,
				   const rina::cdap_rib::con_handle_t &con_handle)
{
	LOG_IPCP_DBG("M_RELEASE_R cdapMessage from portId %u",
		     con_handle.port_id);

	rina::ScopedLock g(lock_);

	try{
		IEnrollmentStateMachine * stateMachine =
				getEnrollmentStateMachine(con_handle.port_id,
							  false);
		stateMachine->releaseResult(res,
					    con_handle);
	}catch(rina::Exception &e){
		//Error getting the enrollment state machine
		LOG_IPCP_ERR("Problems getting enrollment state machine: %s",
			     e.what());

		try {

			rib_daemon_->getProxy()->remote_close_connection(con_handle.port_id);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems closing application connection: %s",
				     e.what());
		}

		deallocateFlow(con_handle.port_id);
	}
}

void EnrollmentTask::process_authentication_message(const rina::cdap::CDAPMessage& message,
					    	    const rina::cdap_rib::con_handle_t &con_handle)
{
	IPCPEnrollmentTaskPS * ipcp_ps = dynamic_cast<IPCPEnrollmentTaskPS *>(ps);
	assert(ipcp_ps);
	ipcp_ps->process_authentication_message(message,
						con_handle);
}

void EnrollmentTask::authentication_completed(int port_id, bool success)
{
	IPCPEnrollmentTaskPS * ipcp_ps = dynamic_cast<IPCPEnrollmentTaskPS *>(ps);
	assert(ipcp_ps);
	ipcp_ps->authentication_completed(port_id, success);
}

IEnrollmentStateMachine * EnrollmentTask::getEnrollmentStateMachine(int portId, bool remove)
{
	std::map<int, IEnrollmentStateMachine*>::iterator it;

	rina::WriteScopedLock writeLock(sm_lock);

	it = state_machines_.find(portId);
	if (it == state_machines_.end()) {
		return NULL;
	}

	if (remove) {
		LOG_IPCP_DBG("Removing enrollment state machine associated to %d",
				portId);
		state_machines_.erase(it);
	}

	return it->second;
}

std::list<rina::Neighbor> EnrollmentTask::get_neighbors()
{
	std::list<rina::Neighbor> result;
	std::map<std::string, rina::Neighbor *>::iterator it;

	rina::ReadScopedLock readLock(neigh_lock);

	for (it = neighbors.begin(); it != neighbors.end(); ++it) {
		result.push_back(*(it->second));
	}

	return result;
}

void EnrollmentTask::add_neighbor(const rina::Neighbor& neighbor)
{
	rina::WriteScopedLock writeLock(neigh_lock);

	if (neighbors.find(neighbor.name_.getProcessNamePlusInstance()) != neighbors.end()) {
		LOG_IPCP_WARN("Tried to add an already existing neighbor: %s",
			      neighbor.name_.getProcessNamePlusInstance().c_str());
		return;
	}

	_add_neighbor(neighbor);
}

void EnrollmentTask::add_or_update_neighbor(const rina::Neighbor& neighbor)
{
	std::map<std::string, rina::Neighbor *>::iterator it;

	rina::WriteScopedLock writeLock(neigh_lock);

	it = neighbors.find(neighbor.name_.getProcessNamePlusInstance());
	if (it != neighbors.end()) {
		it->second->enrolled_ = neighbor.enrolled_;
		it->second->underlying_port_id_ = neighbor.underlying_port_id_;
		it->second->last_heard_from_time_in_ms_ = neighbor.last_heard_from_time_in_ms_;
	} else
		_add_neighbor(neighbor);
}

void EnrollmentTask::_add_neighbor(const rina::Neighbor& neighbor)
{
	rina::Neighbor * neigh = 0;

	try {
		std::stringstream ss;
		ss << NeighborRIBObj::object_name_prefix
				<< neighbor.name_.processName;

		neigh = new rina::Neighbor(neighbor);
		rina::rib::RIBObj * nrobj = new NeighborRIBObj(neigh->name_.getProcessNamePlusInstance());
		rib_daemon_->addObjRIB(ss.str(), &nrobj);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems creating RIB object: %s",
				e.what());
	}

	neighbors[neigh->name_.getProcessNamePlusInstance()] = neigh;
}

rina::Neighbor EnrollmentTask::get_neighbor(const std::string& neighbor_key)
{
	std::map<std::string, rina::Neighbor *>::iterator it;

	rina::ReadScopedLock readLock(neigh_lock);

	it = neighbors.find(neighbor_key);
	if (it == neighbors.end()) {
		LOG_IPCP_WARN("Could not find neighbor for key: %s",
			      neighbor_key.c_str());
		throw rina::IPCException("Neighbor not found");
	}

	return *(it->second);
}

void EnrollmentTask::remove_neighbor(const std::string& neighbor_key)
{
	std::map<std::string, rina::Neighbor *>::iterator it;
	rina::Neighbor * neigh;

	rina::WriteScopedLock writeLock(neigh_lock);

	it = neighbors.find(neighbor_key);
	if (it == neighbors.end()) {
		LOG_IPCP_WARN("Could not find neighbor for key: %s",
			      neighbor_key.c_str());
		return;
	}

	neigh = it->second;
	neighbors.erase(it);
	try {
		std::stringstream ss;
		ss << NeighborRIBObj::object_name_prefix
	           << neigh->name_.processName;
		rib_daemon_->removeObjRIB(ss.str());
	} catch (rina::Exception &e){
		LOG_IPCP_ERR("Error removing object from RIB %s",
			     e.what());
	}

	delete neigh;
	neigh = 0;
}

bool EnrollmentTask::isEnrolledTo(const std::string& processName)
{
	std::map<int, IEnrollmentStateMachine *>::iterator it;

	rina::ReadScopedLock readLock(sm_lock);

	for (it = state_machines_.begin(); it != state_machines_.end(); ++it) {
		if (it->second->remote_peer_.name_.processName.compare(processName) == 0 &&
				it->second->get_state() != IEnrollmentStateMachine::STATE_NULL) {
			return true;
		}
	}

	return false;
}

std::list<std::string> EnrollmentTask::get_enrolled_app_names()
{
	std::list<std::string> result;
	std::map<int, IEnrollmentStateMachine *>::iterator it;

	rina::ReadScopedLock readLock(sm_lock);

	for (it = state_machines_.begin(); it != state_machines_.end(); ++it) {
		result.push_back(it->second->remote_peer_.name_.processName);
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
	rina::WriteScopedLock writeLock(sm_lock);

	state_machines_[portId] = stateMachine;
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
	IEnrollmentStateMachine * enrollmentStateMachine = 0;
	rina::ConnectiviyToNeighborLostEvent * event2 = 0;
	DeallocateFlowTimerTask * timer_task = 0;

	rina::ScopedLock g(lock_);

	//1 Remove the enrollment state machine from the list
	enrollmentStateMachine = getEnrollmentStateMachine(event->port_id_, true);
	if (!enrollmentStateMachine){
		//Do nothing, we had already cleaned up
		return;
	}

	enrollmentStateMachine->flowDeallocated(event->port_id_);

	// Request deallocation of internal flow
	if (enrollmentStateMachine->remote_peer_.internal_port_id != 0) {
		timer_task = new DeallocateFlowTimerTask(ipcp,
							 enrollmentStateMachine->remote_peer_.internal_port_id,
							 true);
		timer.scheduleTask(timer_task, 0);
	}

	event2 = new rina::ConnectiviyToNeighborLostEvent(enrollmentStateMachine->remote_peer_);
	delete enrollmentStateMachine;
	enrollmentStateMachine = 0;
	event_manager_->deliverEvent(event2);
}

void EnrollmentTask::nMinusOneFlowAllocated(rina::NMinusOneFlowAllocatedEvent * flowEvent)
{
	rina::EnrollmentRequest * request;

	rina::ScopedLock g(lock_);

	request = port_ids_pending_to_be_allocated_.erase(flowEvent->handle_);

	if (!request) {
		return;
	}

	IPCPEnrollmentTaskPS * ipcp_ps = dynamic_cast<IPCPEnrollmentTaskPS *>(ps);
	assert(ipcp_ps);
	ipcp_ps->initiate_enrollment(*flowEvent, *request);

	delete request;
}

void EnrollmentTask::nMinusOneFlowAllocationFailed(rina::NMinusOneFlowAllocationFailedEvent * event)
{
	rina::EnrollmentRequest * request;

	rina::ScopedLock g(lock_);

	request = port_ids_pending_to_be_allocated_.erase(event->handle_);

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
				      int portId,
				      int internal_portId,
				      const std::string& reason,
				      bool sendReleaseMessage)
{
	rina::FlowDeallocateRequestEvent fd_event;
	rina::Sleep sleep;
	IEnrollmentStateMachine * stateMachine = 0;
	IPCPEnrollmentTaskPS * ipcp_ps = 0;
	DestroyESMTimerTask * timer_task = 0;

	rina::ScopedLock g(lock_);

	LOG_IPCP_ERR("An error happened during enrollment of remote IPC Process %s because of %s",
		      remotePeerNamingInfo.getEncodedString().c_str(),
		      reason.c_str());

	//1 Remove enrollment state machine from the store
	stateMachine = getEnrollmentStateMachine(portId, true);
	if (!stateMachine) {
		LOG_IPCP_DBG("State machine associated to portId %d already removed",
			      portId);
		return;
	}

	//2 In the case of the enrollee state machine, reply to the IPC Manager
	ipcp_ps = dynamic_cast<IPCPEnrollmentTaskPS *>(ps);
	assert(ipcp_ps);
	ipcp_ps->inform_ipcm_about_failure(stateMachine);

	//3 Send message and deallocate flow if required
	if(sendReleaseMessage){
		try {
			rib_daemon_->getProxy()->remote_close_connection(portId, false);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems closing application connection: %s",
				      e.what());
		}
	}

	//4 Deallocate internal reliable flow if required
	if (internal_portId > 0) {
		try {
			fd_event.internal = true;
			fd_event.portId = internal_portId;
			fd_event.applicationName.processName = ipcp->get_name();
			fd_event.applicationName.processInstance = ipcp->get_instance();
			fd_event.applicationName.entityName = IPCProcess::MANAGEMENT_AE;
			ipcp->flow_allocator_->submitDeallocate(fd_event);

			//Sleep for a little bit so that we give time to deallocate the flow
			sleep.sleepForMili(5);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems deallocating internal flow: %s", e.what());
		}
	}

	//5 Deallocate N-1 flow
	deallocateFlow(portId);

	//6 Terminate and delete stateMachine (via timer)
	stateMachine->reset_state();
	timer_task = new DestroyESMTimerTask(stateMachine);
	timer.scheduleTask(timer_task, 0);
}

void EnrollmentTask::enrollmentCompleted(const rina::Neighbor& neighbor,
					 bool enrollee)
{
	rina::NeighborAddedEvent * event = new rina::NeighborAddedEvent(neighbor,
									enrollee);
	event_manager_->deliverEvent(event);
}

void EnrollmentTask::release(int invoke_id,
		     	     const rina::cdap_rib::con_handle_t &con_handle)
{
	IEnrollmentStateMachine * stateMachine;
	rina::FlowDeallocateRequestEvent fd_event;
	rina::Sleep sleep;

	LOG_DBG("M_RELEASE cdapMessage from portId %u",
		con_handle.port_id);

	rina::ScopedLock g(lock_);

	try{
		stateMachine = getEnrollmentStateMachine(con_handle.port_id,
							 true);
		if (!stateMachine) {
			LOG_IPCP_DBG("State machine associated to portId %d already removed",
				      con_handle.port_id);
			return;
		}

		stateMachine->release(invoke_id,
				      con_handle);
	}catch(rina::Exception &e){
		//Error getting the enrollment state machine
		LOG_IPCP_ERR("Problems getting enrollment state machine: %s",
			     e.what());
	}

	//2 In the case of the enrollee state machine, reply to the IPC Manager
	IPCPEnrollmentTaskPS * ipcp_ps = dynamic_cast<IPCPEnrollmentTaskPS *>(ps);
	assert(ipcp_ps);
	ipcp_ps->inform_ipcm_about_failure(stateMachine);

	if (invoke_id != 0) {
		try {
			rina::cdap_rib::flags_t flags;
			rina::cdap_rib::res_info_t res;
			res.code_ = rina::cdap_rib::CDAP_SUCCESS;
			rina::cdap::getProvider()->send_close_connection_result(con_handle.port_id,
										flags,
										res,
										invoke_id);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems generating or sending CDAP Message: %s",
				     e.what());
		}
	}

	//4 Deallocate internal reliable flow if required
	if (stateMachine->remote_peer_.internal_port_id > 0) {
		try {
			fd_event.internal = true;
			fd_event.portId = stateMachine->remote_peer_.internal_port_id;
			fd_event.applicationName.processName = ipcp->get_name();
			fd_event.applicationName.processInstance = ipcp->get_instance();
			fd_event.applicationName.entityName = IPCProcess::MANAGEMENT_AE;
			ipcp->flow_allocator_->submitDeallocate(fd_event);

			//Sleep for a little bit so that we give time to deallocate the flow
			sleep.sleepForMili(5);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems deallocating internal flow: %s", e.what());
		}
	}

	deallocateFlow(con_handle.port_id);

	//Schedule destruction of enrollment state machine in a separate thread, since it may take time
	DestroyESMTimerTask * timer_task = new DestroyESMTimerTask(stateMachine);
	timer.scheduleTask(timer_task, 0);
}

// Class Operational Status RIB Object
const std::string OperationalStatusRIBObject::class_name = "OperationalStatus";
const std::string OperationalStatusRIBObject::object_name = "/difManagement/opstatus";

OperationalStatusRIBObject::OperationalStatusRIBObject(IPCProcess * ipc_process) :
		IPCPRIBObj(ipc_process, class_name)
{
	enrollment_task_ = (EnrollmentTask *) ipc_process->enrollment_task_;
}

void OperationalStatusRIBObject::start(const rina::cdap_rib::con_handle_t &con_handle,
				       const std::string& fqn,
				       const std::string& class_,
				       const rina::cdap_rib::filt_info_t &filt,
				       const int invoke_id,
				       const rina::ser_obj_t &obj_req,
				       rina::ser_obj_t &obj_reply,
				       rina::cdap_rib::res_info_t& res)
{
	try {
		enrollment_task_->operational_status_start(con_handle.port_id,
							   invoke_id,
							   obj_req);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems retrieving state machine: %s", e.what());
		sendErrorMessage(con_handle);
		return;
	}

	res.code_ = rina::cdap_rib::CDAP_PENDING;

	if (ipc_process_->get_operational_state() != ASSIGNED_TO_DIF) {
		ipc_process_->set_operational_state(ASSIGNED_TO_DIF);
	}
}

void OperationalStatusRIBObject::startObject() {
	if (ipc_process_->get_operational_state() != ASSIGNED_TO_DIF) {
		ipc_process_->set_operational_state(ASSIGNED_TO_DIF);
	}
}

void OperationalStatusRIBObject::stopObject() {
	if (ipc_process_->get_operational_state() != ASSIGNED_TO_DIF) {
		ipc_process_->set_operational_state(INITIALIZED);
	}
}

void OperationalStatusRIBObject::read(const rina::cdap_rib::con_handle_t &con_handle,
				      const std::string& fqn,
				      const std::string& class_,
				      const rina::cdap_rib::filt_info_t &filt,
				      const int invoke_id,
				      rina::cdap_rib::obj_info_t &obj_reply,
				      rina::cdap_rib::res_info_t& res)
{
	rina::cdap::IntEncoder encoder;
	encoder.encode(ipc_process_->get_operational_state(), obj_reply.value_);
	res.code_ = rina::cdap_rib::CDAP_SUCCESS;
}

void OperationalStatusRIBObject::sendErrorMessage(const rina::cdap_rib::con_handle_t &con_handle)
{
	try{
		rib_daemon_->getProxy()->remote_close_connection(con_handle.port_id);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems sending CDAP message: %s", e.what());
	}
}

const std::string OperationalStatusRIBObject::get_displayable_value() const
{
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
} //namespace rinad
