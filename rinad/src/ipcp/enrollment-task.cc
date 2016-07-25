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

#include "common/concurrency.h"
#include "common/encoders/EnrollmentInformationMessage.pb.h"
#include "ipcp/enrollment-task.h"
#include "common/encoder.h"
#include "common/rina-configuration.h"

namespace rinad {

// Class Neighbor RIB object
const std::string NeighborRIBObj::class_name = "Neighbor";
const std::string NeighborRIBObj::object_name_prefix = "/difManagement/enrollment/neighbors/processName=";

NeighborRIBObj::NeighborRIBObj(rina::Neighbor* neigh) :
		rina::rib::RIBObj(class_name), neighbor(neigh)
{
}

const std::string NeighborRIBObj::get_displayable_value() const
{
    std::stringstream ss;
    ss << "Name: " << neighbor->name_.getEncodedString();
    ss << "; Address: " << neighbor->address_;
    ss << "; Enrolled: " << neighbor->enrolled_ << std::endl;
    ss << "; Supporting DIF Name: " << neighbor->supporting_dif_name_.processName;
    ss << "; Underlying port-id: " << neighbor->underlying_port_id_;
    ss << "; Number of enroll. attempts: " << neighbor->number_of_enrollment_attempts_;

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
	if (neighbor) {
		encoders::NeighborEncoder encoder;
		encoder.encode(*neighbor, obj_reply.value_);
	}

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
            LOG_ERR("Create operation failed: received an invalid class "
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
	LOG_DBG("Enrolling to neighbor %s", object.get_name().processName.c_str());

	rina::EnrollToDAFRequestEvent* event;
	event->neighborName = object.get_name();
	event->dafName = rinad::IPCPFactory::getIPCP()->
    		get_dif_information().dif_name_;
	event->supportingDIFName = object.
			get_supporting_dif_name();
	rinad::IPCPFactory::getIPCP()->enrollment_task_->processEnrollmentRequestEvent(event);

    LOG_INFO("IPC Process enrollment to neighbor %s completed successfully",
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
	rina::Lockable lock;
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
			LOG_INFO("Ignoring neighbor %s because we don't have an N-1 DIF in common",
					iterator->name_.processName.c_str());
			continue;
		}

		//5 Add the neighbor
		ipc_process_->enrollment_task_->add_neighbor(*iterator);
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

	//1 Update last heard from attribute of the relevant neighbor
	std::list<rina::Neighbor*> neighbors =
			ipc_process_->enrollment_task_->get_neighbor_pointers();
	std::list<rina::Neighbor*>::const_iterator it;
	for (it = neighbors.begin(); it != neighbors.end(); ++it) {
		if ((*it)->name_.processName.compare(con.dest_.ap_name_) == 0) {
			rina::Time currentTime;
			(*it)->last_heard_from_time_in_ms_ =
					currentTime.get_current_time_in_ms();
			break;
		}
	}

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
	std::list<rina::Neighbor*> neighbors =
			ipc_process_->enrollment_task_->get_neighbor_pointers();
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
			rina::NeighborDeclaredDeadEvent * event =
					new rina::NeighborDeclaredDeadEvent(*(*it));
			ipc_process_->internal_event_manager_->deliverEvent(event);
			continue;
		}

		try{
			rina::cdap_rib::obj_info_t obj;
			rina::cdap_rib::flags_t flags;
			rina::cdap_rib::filt_info_t filt;
			obj.class_ = class_name;
			obj.name_ = object_name;
			con.port_id = (*it)->underlying_port_id_;

			rib_daemon_->getProxy()->remote_read(con,
							     obj,
							     flags,
							     filt,
							     this);

			neighbor_statistics_[(*it)->name_.processName] = currentTimeInMs;
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

	std::map<std::string, int>::iterator it =
			neighbor_statistics_.find(con.dest_.ap_name_);
	if (it == neighbor_statistics_.end())
		return;

	int storedTime = it->second;
	neighbor_statistics_.erase(it);
	rina::Time currentTime;
	int currentTimeInMs = currentTime.get_current_time_in_ms();
	std::list<rina::Neighbor*> neighbors =
			ipc_process_->enrollment_task_->get_neighbor_pointers();
	std::list<rina::Neighbor*>::const_iterator it2;
	for (it2 = neighbors.begin(); it2 != neighbors.end(); ++it2) {
		if ((*it2)->name_.processName.compare(con.dest_.ap_name_) == 0) {
			(*it2)->average_rtt_in_ms_ = currentTimeInMs - storedTime;
			(*it2)->last_heard_from_time_in_ms_ = currentTimeInMs;
			break;
		}
	}
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
	std::list<rina::Neighbor*> neighbors;
	std::list<rina::Neighbor*>::const_iterator it;
	rina::Sleep sleepObject;

	while(true){
		neighbors = enrollmentTask->get_neighbor_pointers();
		for(it = neighbors.begin(); it != neighbors.end(); ++it) {
			if (enrollmentTask->isEnrolledTo((*it)->name_.processName)) {
				//We're already enrolled to this guy, continue
				continue;
			}

			if ((*it)->number_of_enrollment_attempts_ <
					enrollmentTask->max_num_enroll_attempts_) {
				(*it)->number_of_enrollment_attempts_++;
				rina::EnrollmentRequest request(**it);
				enrollmentTask->initiateEnrollment(request);
			} else {
				enrollmentTask->remove_neighbor((*it)->name_.getEncodedString());
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
						 int timeout,
						 rina::ApplicationProcessNamingInformation * supporting_dif_name)
{
	if (!ipcp) {
		throw rina::Exception("Bogus Application Process instance passed");
	}

	ipcp_ = ipcp;
	rib_daemon_ = ipcp_->rib_daemon_;
	enrollment_task_ = ipcp_->enrollment_task_;
	timeout_ = timeout;
	remote_peer_.name_ = remote_naming_info;
	if (supporting_dif_name) {
		remote_peer_.supporting_dif_name_ = *supporting_dif_name;
		delete supporting_dif_name;
	}
	last_scheduled_task_ = 0;
	state_ = STATE_NULL;
	auth_ps_ = 0;
	enroller_ = false;
}

IEnrollmentStateMachine::~IEnrollmentStateMachine() {
}

void IEnrollmentStateMachine::release(int invoke_id,
		     	     	      const rina::cdap_rib::con_handle_t &con_handle)
{
	rina::ScopedLock g(lock_);
	LOG_IPCP_DBG("Releasing the CDAP connection");

	if (!isValidPortId(con_handle.port_id))
		return;

	if (last_scheduled_task_)
		timer_.cancelTask(last_scheduled_task_);

	createOrUpdateNeighborInformation(false);

	state_ = STATE_NULL;
}

void IEnrollmentStateMachine::releaseResult(const rina::cdap_rib::res_info_t &res,
		   	   	   	    const rina::cdap_rib::con_handle_t &con_handle)
{
	rina::ScopedLock g(lock_);

	if (!isValidPortId(con_handle.port_id))
		return;

	if (state_ != STATE_NULL)
		state_ = STATE_NULL;
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
	if (portId != con.port_id) {
		LOG_ERR("a CDAP message form port-id %d, but was expecting it form port-id %d",
			portId,
			con.port_id);
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
	remote_peer_.enrolled_ = enrolled;
	remote_peer_.number_of_enrollment_attempts_ = 0;
	rina::Time currentTime;
	remote_peer_.last_heard_from_time_in_ms_ = currentTime.get_current_time_in_ms();
	if (enrolled) {
		remote_peer_.underlying_port_id_ = con.port_id;
	} else {
		remote_peer_.underlying_port_id_ = 0;
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

//Class EnrollmentFailedTimerTask
EnrollmentFailedTimerTask::EnrollmentFailedTimerTask(IEnrollmentStateMachine * state_machine,
		const std::string& reason) {
	state_machine_ = state_machine;
	reason_ = reason;
}

void EnrollmentFailedTimerTask::run() {
	try {
		state_machine_->abortEnrollment(state_machine_->remote_peer_.name_,
						state_machine_->con.port_id,
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

	rina::Neighbor neighbor;
	neighbor.name_ = event->neighborName;
	neighbor.supporting_dif_name_ = event->supportingDIFName;
	unsigned int address = nsmps->getValidAddress(neighbor.name_.processName,
			neighbor.name_.processInstance);
	if (address != 0) {
		neighbor.address_ = address;
	}

	rina::EnrollmentRequest request(neighbor, *event);
	initiateEnrollment(request);
}

void EnrollmentTask::initiateEnrollment(const rina::EnrollmentRequest& request)
{
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
		     	     const rina::cdap_rib::con_handle_t &con_handle)
{
	LOG_IPCP_DBG("M_CONNECT CDAP message from port-id %u",
		     con_handle.port_id);

	//1 Find out if the sender is really connecting to us
	if(con_handle.src_.ap_name_.compare(ipcp->get_name())!= 0){
		LOG_IPCP_WARN("an M_CONNECT message whose destination was not this IPC Process, ignoring it");
		return;
	}

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

void EnrollmentTask::connectResult(const rina::cdap_rib::res_info_t &res,
				   const rina::cdap_rib::con_handle_t &con_handle,
				   const rina::cdap_rib::auth_policy_t& auth)
{
	LOG_IPCP_DBG("M_CONNECT_R cdapMessage from portId %u",
		     con_handle.port_id);

	IPCPEnrollmentTaskPS * ipcp_ps = dynamic_cast<IPCPEnrollmentTaskPS *>(ps);
	assert(ipcp_ps);
	ipcp_ps->connect_response_received(res.code_,
					   res.reason_,
					   con_handle,
					   auth);
}

void EnrollmentTask::releaseResult(const rina::cdap_rib::res_info_t &res,
				   const rina::cdap_rib::con_handle_t &con_handle)
{
	LOG_IPCP_DBG("M_RELEASE_R cdapMessage from portId %u",
		     con_handle.port_id);

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
	if (remove) {
		LOG_IPCP_DBG("Removing enrollment state machine associated to %d",
				portId);
		return state_machines_.erase(portId);
	} else {
		return state_machines_.find(portId);
	}
}

const std::list<rina::Neighbor> EnrollmentTask::get_neighbors() const
{
	return neighbors.getCopyofentries();
}

std::list<rina::Neighbor*> EnrollmentTask::get_neighbor_pointers()
{
	return neighbors.getEntries();
}

void EnrollmentTask::add_neighbor(const rina::Neighbor& neighbor)
{
	rina::ScopedLock g(lock_);

	if (neighbors.find(neighbor.name_.getEncodedString()) != 0) {
		LOG_IPCP_WARN("Tried to add an already existing neighbor: %s",
			      neighbor.name_.getEncodedString().c_str());
		return;
	}

	_add_neighbor(neighbor);
}

void EnrollmentTask::add_or_update_neighbor(const rina::Neighbor& neighbor)
{
	rina::Neighbor * neigh = 0;

	rina::ScopedLock g(lock_);

	neigh = neighbors.find(neighbor.name_.getEncodedString());
	if (neigh) {
		neigh->enrolled_ = neighbor.enrolled_;
		neigh->underlying_port_id_ = neighbor.underlying_port_id_;
		neigh->last_heard_from_time_in_ms_ = neighbor.last_heard_from_time_in_ms_;
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
		rina::rib::RIBObj * nrobj = new NeighborRIBObj(neigh);
		rib_daemon_->addObjRIB(ss.str(), &nrobj);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems creating RIB object: %s",
				e.what());
	}

	neighbors.put(neigh->name_.getEncodedString(), neigh);
}

void EnrollmentTask::remove_neighbor(const std::string& neighbor_key)
{
	rina::Neighbor * neighbor;

	rina::ScopedLock g(lock_);

	neighbor = neighbors.erase(neighbor_key);
	if (neighbor == 0) {
		LOG_IPCP_WARN("Could not find neighbor for key: %s",
			      neighbor_key.c_str());
		return;
	}

	try {
		std::stringstream ss;
		ss << NeighborRIBObj::object_name_prefix
	           << neighbor->name_.processName;
		rib_daemon_->removeObjRIB(ss.str());
	} catch (rina::Exception &e){
		LOG_IPCP_ERR("Error removing object from RIB %s",
			     e.what());
	}

	delete neighbor;
}

bool EnrollmentTask::isEnrolledTo(const std::string& processName)
{
	rina::ScopedLock g(lock_);

	std::list<IEnrollmentStateMachine *> machines = state_machines_.getEntries();
	std::list<IEnrollmentStateMachine *>::const_iterator it;
	for (it = machines.begin(); it != machines.end(); ++it) {
		if ((*it)->remote_peer_.name_.processName.compare(processName) == 0 &&
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
		result.push_back((*it)->remote_peer_.name_.processName);
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
	//1 Remove the enrollment state machine from the list
	IEnrollmentStateMachine * enrollmentStateMachine =
			getEnrollmentStateMachine(event->port_id_, true);
	if (!enrollmentStateMachine){
		//Do nothing, we had already cleaned up
		LOG_IPCP_INFO("Could not find enrollment state machine associated to port-id %d",
				event->port_id_);
		return;
	}

	enrollmentStateMachine->flowDeallocated(event->port_id_);

	rina::ConnectiviyToNeighborLostEvent * event2 =
			new rina::ConnectiviyToNeighborLostEvent(enrollmentStateMachine->remote_peer_);
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
	ipcp_ps->initiate_enrollment(*flowEvent, *request);

	delete request;
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
				      int portId,
				      const std::string& reason,
				      bool sendReleaseMessage)
{
	LOG_IPCP_ERR("An error happened during enrollment of remote IPC Process %s because of %s",
		      remotePeerNamingInfo.getEncodedString().c_str(),
		      reason.c_str());

	//1 Remove enrollment state machine from the store
	IEnrollmentStateMachine * stateMachine =
			getEnrollmentStateMachine(portId, true);
	if (!stateMachine) {
		LOG_IPCP_ERR("Could not find the enrollment state machine associated to neighbor %s and portId %d",
			     remotePeerNamingInfo.processName.c_str(),
			     portId);
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
			rib_daemon_->getProxy()->remote_close_connection(portId);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems closing application connection: %s",
				      e.what());
		}
	}

	deallocateFlow(portId);
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
	LOG_DBG("M_RELEASE cdapMessage from portId %u",
		con_handle.port_id);
	IEnrollmentStateMachine * stateMachine = 0;

	try{
		stateMachine = getEnrollmentStateMachine(con_handle.port_id,
							 true);
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

	delete stateMachine;
	stateMachine = 0;

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

	deallocateFlow(con_handle.port_id);
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
		if (!enrollment_task_->getEnrollmentStateMachine(con_handle.port_id,
								 false)) {
			LOG_IPCP_ERR("Got a CDAP message that is not for me: %s");
			return;
		}
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems retrieving state machine: %s", e.what());
		sendErrorMessage(con_handle);
		return;
	}

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
