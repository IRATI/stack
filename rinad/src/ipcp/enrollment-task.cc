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
#include <climits>

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
const std::string NeighborRIBObj::object_name_prefix = "/difm/enr/neighs/pn=";

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
const std::string NeighborsRIBObj::object_name = "/difm/enr/neighs";

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
const std::string WatchdogRIBObject::object_name = "/difm/enr/wd";

WatchdogRIBObject::WatchdogRIBObject(IPCProcess * ipc_process,
				     int wdog_period_ms,
				     int declared_dead_int_ms) :
		IPCPRIBObj(ipc_process, class_name)
{
	wathchdog_period_ = wdog_period_ms;
	declared_dead_interval_ = declared_dead_int_ms;
	lock_ = new rina::Lockable();
	timer_ = new rina::Timer();
	//FIXME: Disabling watchdog timer, implementation has to be revised
	//timer_->scheduleTask(new WatchdogTimerTask(this,
	//					   timer_,
	//					   wathchdog_period_),
	//		     wathchdog_period_);
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
		if (!it->enrolled_) {
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
const std::string AddressRIBObject::object_name = "/difm/nam/addr";

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

//Class IEnrollmentStateMachine
const std::string IEnrollmentStateMachine::STATE_NULL = "NULL";
const std::string IEnrollmentStateMachine::STATE_ENROLLED = "ENROLLED";
const std::string IEnrollmentStateMachine::STATE_TERMINATED = "TERMINATED";

IEnrollmentStateMachine::IEnrollmentStateMachine(IPCProcess * ipcp,
						 const rina::ApplicationProcessNamingInformation& remote_naming_info,
						 int timeout,
						 const rina::ApplicationProcessNamingInformation& supporting_dif_name,
						 rina::Timer * timer_)
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
	being_destroyed = false;
	timer = timer_;
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
		timer->cancelTask(last_scheduled_task_);

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
	timer->scheduleTask(task, 0);
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
		//Commented, just send myself as a neighbor (can produce bugs in DIFs over WiFi)
		/*for (it = neighbors.begin(); it != neighbors.end(); ++it)
			neighbors_to_send.push_back(*it);*/

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

//Class RetryEnrollmentTimerTask
RetryEnrollmentTimerTask::RetryEnrollmentTimerTask(rina::IEnrollmentTask * enr_task,
			 	 	 	   const rina::EnrollmentRequest& request)
{
	etask = enr_task;
	erequest = request;
}

void RetryEnrollmentTimerTask::run()
{
	etask->initiateEnrollment(erequest);
}

//Class Enrollment Task
const std::string EnrollmentTask::ENROLL_TIMEOUT_IN_MS = "enrollTimeoutInMs";
const std::string EnrollmentTask::WATCHDOG_PERIOD_IN_MS = "watchdogPeriodInMs";
const std::string EnrollmentTask::DECLARED_DEAD_INTERVAL_IN_MS = "declaredDeadIntervalInMs";
const std::string EnrollmentTask::MAX_ENROLLMENT_RETRIES = "maxEnrollmentRetries";
const std::string EnrollmentTask::USE_RELIABLE_N_FLOW = "useReliableNFlow";
const std::string EnrollmentTask::N1_FLOWS = "n1flows";
const std::string EnrollmentTask::N1_DIFS_PEER_DISCOVERY = "n1difsPeerDiscovery";
const std::string EnrollmentTask::PEER_DISCOVERY_PERIOD_IN_MS = "peerDiscoveryPeriodMs";
const std::string EnrollmentTask::MAX_PEER_DISCOVERY_ATTEMPTS = "maxPeerDiscoveryAttempts";

EnrollmentTask::EnrollmentTask() : IPCPEnrollmentTask()
{
	namespace_manager_ = 0;
	rib_daemon_ = 0;
	irm_ = 0;
	event_manager_ = 0;
	timeout_ = 10000;
	max_num_enroll_attempts_ = 3;
	watchdog_per_ms_ = 30000;
	declared_dead_int_ms_ = 120000;
	ipcp_ps = 0;
	use_reliable_n_flow = false;
	max_peer_discovery_attempts = INT_MAX;
	peer_discovery_period_ms = 0;
}

EnrollmentTask::~EnrollmentTask()
{
	delete ps;
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
				== event->flow_info.localAppName.processName ||
		    it->second->remote_peer_.name_.processName
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
				== event->flow_info.localAppName.processName ||
		    it->second->remote_peer_.name_.processName
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
	DestroyESMTimerTask * dsm_task = 0;

	//1 Stop the internal flow SDU reader
	ribd->stop_internal_flow_sdu_reader(event->port_id);

	sm_lock.writelock();
	//2 Remove the enrollment state machine from the map
	for (it = state_machines_.begin(); it != state_machines_.end(); ++it) {
		if (it->second->remote_peer_.internal_port_id == event->port_id) {
			esm = it->second;
			state_machines_.erase(it);
			break;
		}
	}
	sm_lock.unlock();

	if (!esm){
		//Do nothing, we had already cleaned up
		return;
	}

	esm->flowDeallocated(esm->remote_peer_.underlying_port_id_);

	// Request deallocation of N-1 flow
	deallocateFlow(esm->con.port_id);

	ipcp->resource_allocator_->remove_temp_pduft_entry(esm->remote_peer_.address_);

	// Update and defer deletion of state machine to a timer task (takes long)
	esm->reset_state();
	dsm_task = new DestroyESMTimerTask(esm);
	timer.scheduleTask(dsm_task, 0);
}

void EnrollmentTask::operational_status_start(int port_id,
			      	      	      int invoke_id,
					      const rina::ser_obj_t &obj_req)
{
	std::map<int, IEnrollmentStateMachine*>::iterator it;

	sm_lock.readlock();
	for (it = state_machines_.begin(); it != state_machines_.end(); ++it) {
		if (it->second->remote_peer_.underlying_port_id_ == port_id) {
			sm_lock.unlock();
			it->second->operational_status_start(invoke_id, obj_req);
			return;
		}
	}

	sm_lock.unlock();
}

int EnrollmentTask::get_con_handle_to_ipcp_with_address(unsigned int dest_address,
			   	   		        rina::cdap_rib::con_handle_t& con)
{
	std::map<int, IEnrollmentStateMachine*>::iterator it;
	unsigned int next_hop_address = dest_address;
	std::list<unsigned int> nhop_addresses;
	std::list<unsigned int>::iterator addr_it;

	sm_lock.readlock();
	// Check if the destination address is one of our next hops
	for (it = state_machines_.begin(); it != state_machines_.end(); ++it) {
		if (it->second->remote_peer_.address_ == next_hop_address ||
				it->second->remote_peer_.old_address_ == next_hop_address) {
			con.port_id = it->second->con.port_id;
			sm_lock.unlock();
			return 0;
		}
	}
	sm_lock.unlock();

	// Check if we can find the address to the next hop via the resource allocator
	ipcp->resource_allocator_->get_next_hop_addresses(dest_address, nhop_addresses);
	if (nhop_addresses.size() == 0) {
		LOG_IPCP_ERR("Could not find next hop for destination address %d", dest_address);
		return -1;
	}

	// Get con from next hop
	for (addr_it = nhop_addresses.begin(); addr_it != nhop_addresses.end(); ++addr_it) {
		next_hop_address = *addr_it;

		sm_lock.readlock();
		for (it = state_machines_.begin(); it != state_machines_.end(); ++it) {
			if (it->second->remote_peer_.address_ == next_hop_address ||
					it->second->remote_peer_.old_address_ == next_hop_address) {
				con.port_id = it->second->con.port_id;
				sm_lock.unlock();
				return 0;
			}
		}
		sm_lock.unlock();
	}

	LOG_IPCP_ERR("Could not find neighbor with address %u", next_hop_address);
	return -1;
}

int EnrollmentTask::get_con_handle_to_ipcp(unsigned int dest_address,
			      	      	   rina::cdap_rib::con_handle_t& con)
{

	return get_con_handle_to_ipcp_with_address(dest_address, con);
}

unsigned int EnrollmentTask::get_con_handle_to_ipcp(const std::string& ipcp_name,
			   	   	            rina::cdap_rib::con_handle_t& con)
{
	std::map<int, IEnrollmentStateMachine*>::iterator it;

	sm_lock.readlock();
	// Check if the destination address is one of our next hops
	for (it = state_machines_.begin(); it != state_machines_.end(); ++it) {
		if (it->second->remote_peer_.name_.processName == ipcp_name) {
			con.port_id = it->second->con.port_id;
			sm_lock.unlock();
			return it->second->remote_peer_.address_;
		}
	}
	sm_lock.unlock();

	return 0;
}

int EnrollmentTask::get_neighbor_info(rina::Neighbor& neigh)
{
	std::map<std::string, rina::Neighbor*>::iterator it;

	rina::ReadScopedLock readLock(neigh_lock);

	it = neighbors.find(neigh.name_.getProcessNamePlusInstance());
	if (it == neighbors.end()) {
		return -1;
	}

	neigh.address_ = it->second->address_;
	neigh.enrolled_ = it->second->enrolled_;
	neigh.internal_port_id = it->second->internal_port_id;
	neigh.old_address_ = it->second->old_address_;
	neigh.underlying_port_id_ = it->second->underlying_port_id_;

	return 0;
}

void EnrollmentTask::addressChange(rina::AddressChangeEvent * event)
{
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
	std::map<int, IEnrollmentStateMachine*>::iterator it2;
	rina::NeighborAddressChangeEvent * event = 0;

	sm_lock.readlock();
	for (it2 = state_machines_.begin(); it2 != state_machines_.end(); ++it2) {
		if (it2->second->remote_peer_.name_.processName == neighbor.name_.processName) {
			it2->second->remote_peer_.old_address_ = it2->second->remote_peer_.address_;
			it2->second->remote_peer_.address_ = neighbor.address_;
			break;
		}
	}
	sm_lock.unlock();

	neigh_lock.readlock();
	it = neighbors.find(neighbor.name_.getProcessNamePlusInstance());
	if (it != neighbors.end()) {
		it->second->old_address_ = it->second->address_;
		it->second->address_ = neighbor.address_;
		neigh_lock.unlock();
		event = new rina::NeighborAddressChangeEvent(it->second->name_.processName,
							     it->second->address_,
							     it->second->old_address_);
		LOG_IPCP_INFO("Neighbor %s address changed from %d to %d",
			       neighbor.name_.getProcessNamePlusInstance().c_str(),
			       it->second->old_address_,
			       it->second->address_);
		ipcp->internal_event_manager_->deliverEvent(event);
	} else {
		neigh_lock.unlock();
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

void EnrollmentTask::parse_n1flows(const std::string& name,
		                   const std::string& value)
{
	std::vector<std::string> result;
	std::list<rina::FlowSpecification> fspecs;
	const char * str;
	int num_flows;
	int loss;
	int delay;
	rina::FlowSpecification fspec;
	std::string n1_dif, efspec, eloss, edelay;

	str = &name[0];

	do
	{
		const char *begin = str;

		while(*str != ':' && *str)
			str++;

		result.push_back(std::string(begin, str));
	} while (0 != *str++);

	n1_dif = result[1];

	result.clear();
	str = &value[0];

	do
	{
		const char *begin = str;

		while(*str != ':' && *str)
			str++;

		result.push_back(std::string(begin, str));
	} while (0 != *str++);

	if (rina::string2int(result[0], num_flows)) {
		LOG_WARN("Could not parse number of flows %s, using 1",
			 result[0].c_str());
		num_flows = 1;
	}

	for (int i=0; i<num_flows; i++) {
		efspec = result[i+1];

		edelay = efspec.substr(0, efspec.find("/"));
		if (rina::string2int(edelay, delay)) {
			LOG_WARN("Could not parse delay %s, using 0",
				 edelay.c_str());
			fspec.delay = 0;
		} else {
			fspec.delay = delay;
		}

		eloss = efspec.substr(efspec.find("/") + 1);
		if (rina::string2int(eloss, loss)) {
			LOG_WARN("Could not parse delay %s, using 10000",
				 edelay.c_str());
			fspec.loss = 10000;
		} else {
			fspec.loss = loss;
		}

		fspecs.push_back(fspec);
	}

	n1_flows_to_create[n1_dif] = fspecs;
}

void EnrollmentTask::set_dif_configuration(const rina::DIFInformation& dif_information)
{
	std::list<rina::PolicyParameter>::const_iterator it;
	std::list<std::string>::iterator it2;
	RetryEnrollmentTimerTask * timer_task = 0;
	rina::EnrollmentRequest enr_request;

	rina::PolicyConfig psconf = dif_information.dif_configuration_.et_configuration_.policy_set_;
	if (select_policy_set(std::string(), psconf.name_) != 0) {
		throw rina::Exception("Cannot create enrollment task policy-set");
	}

	// Parse policy config parameters
	try {
		timeout_ = psconf.get_param_value_as_int(ENROLL_TIMEOUT_IN_MS);
	} catch (rina::Exception &e) {
		LOG_IPCP_INFO("Could not parse timeout_value, using default value: %d", timeout_);
	}

	try {
		max_num_enroll_attempts_ = psconf.get_param_value_as_uint(MAX_ENROLLMENT_RETRIES);
	} catch (rina::Exception &e) {
		LOG_IPCP_INFO("Could not parse max_num_enroll_attempts_, using default value: %d",
			      max_num_enroll_attempts_);
	}

	try {
		watchdog_per_ms_ = psconf.get_param_value_as_int(WATCHDOG_PERIOD_IN_MS);
	} catch (rina::Exception &e) {
		LOG_IPCP_INFO("Could not parse watchdog_per_ms_, using default value: %d",
			      watchdog_per_ms_);
	}

	try {
		declared_dead_int_ms_ = psconf.get_param_value_as_int(DECLARED_DEAD_INTERVAL_IN_MS);
	} catch (rina::Exception &e) {
		LOG_IPCP_INFO("Could not parse declared_dead_int_ms_, using default value: %d",
			      declared_dead_int_ms_);
	}

	try {
		use_reliable_n_flow = psconf.get_param_value_as_bool(USE_RELIABLE_N_FLOW);
	} catch (rina::Exception &e) {
		LOG_IPCP_INFO("Could not parse use_reliable_n_flow, using default value: %d",
			      use_reliable_n_flow);
	}

	for(it = psconf.parameters_.begin(); it != psconf.parameters_.end(); ++it) {
		if (it->name_.find(N1_FLOWS) != std::string::npos) {
			parse_n1flows(it->name_, it->value_);
		} else if (it->name_ == N1_DIFS_PEER_DISCOVERY) {
			n1_difs_peer_discovery.push_back(it->value_);
		}
	}

	try {
		peer_discovery_period_ms = psconf.get_param_value_as_int(PEER_DISCOVERY_PERIOD_IN_MS);
	} catch (rina::Exception &e) {
		LOG_IPCP_INFO("Could not parse peer_discovery_period_ms, using default value: %d",
			       peer_discovery_period_ms);
	}

	try {
		max_peer_discovery_attempts = psconf.get_param_value_as_int(MAX_PEER_DISCOVERY_ATTEMPTS);
	} catch (rina::Exception &e) {
		LOG_IPCP_INFO("Could not parse max_peer_discovery_attempts, using default value: %d",
			       max_peer_discovery_attempts);
	}

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

	//Apply configuration to policy set
	IPCPEnrollmentTaskPS * ipcp_ps = dynamic_cast<IPCPEnrollmentTaskPS *>(ps);
	assert(ipcp_ps);
	ipcp_ps->set_dif_configuration(dif_information.dif_configuration_);

	//Start enrollment with peers over specified N-1 DIFs
	if (peer_discovery_period_ms == 0)
		return;

	enr_request.neighbor_.name_ = dif_information.dif_name_;
	for (it2 = n1_difs_peer_discovery.begin(); it2 != n1_difs_peer_discovery.end(); ++it2) {
		enr_request.neighbor_.supporting_dif_name_.processName = *it2;
		timer_task = new RetryEnrollmentTimerTask(this, enr_request);
		timer.scheduleTask(timer_task,
				peer_discovery_period_ms + (rand() % static_cast<int>(5001)));
	}
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
	request.enrollment_attempts = 0;
	initiateEnrollment(request);
}

void EnrollmentTask::processDisconnectNeighborRequestEvent(const rina::DisconnectNeighborRequestEvent& event)
{
	IEnrollmentStateMachine * esm = 0;
	std::map<int, IEnrollmentStateMachine *>::iterator it;
	std::stringstream ss;

	sm_lock.writelock();
	for (it = state_machines_.begin(); it != state_machines_.end(); ++it) {
		if (it->second->remote_peer_.name_.processName.compare(event.neighborName.processName) == 0 &&
				it->second->get_state() != IEnrollmentStateMachine::STATE_NULL) {
			esm = it->second;
			state_machines_.erase(it);
		}
	}
	sm_lock.unlock();

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

	deallocate_flows_and_destroy_esm(esm, esm->con.port_id, false);

	// Remove RIB object
	remove_neighbor(event.neighborName.getProcessNamePlusInstance());

	// Reply to IPC Manager
	try {
		rina::extendedIPCManager->disconnectNeighborResponse(event, 0);
	}catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems sending message to IPC Manager: %s", e.what());
	}
}

void EnrollmentTask::initiateEnrollment(const rina::EnrollmentRequest& request)
{
	rina::FlowSpecification fspec;
	std::map< std::string , std::list<rina::FlowSpecification> >::iterator it;

	if (isEnrolledTo(request.neighbor_.name_.processName)) {
		LOG_IPCP_ERR("Already enrolled to IPC Process %s",
			     request.neighbor_.name_.processName.c_str());
		return;
	}

	//Request the allocation of a new N-1 Flow to the destination IPC Process,
	//dedicated to layer management
	//FIXME not providing FlowSpec information
	//FIXME not distinguishing between AEs
	it = n1_flows_to_create.find(request.neighbor_.supporting_dif_name_.processName);
	if (it != n1_flows_to_create.end()) {
		fspec = it->second.front();
	}
	rina::FlowInformation flowInformation;
	flowInformation.remoteAppName = request.neighbor_.name_;
	flowInformation.localAppName.processName = ipcp->get_name();
	flowInformation.localAppName.processInstance = ipcp->get_instance();
	flowInformation.difName = request.neighbor_.supporting_dif_name_;
	flowInformation.flowSpecification.msg_boundaries = true;
	flowInformation.flowSpecification.orderedDelivery = true;
	flowInformation.flowSpecification.delay = fspec.delay;
	flowInformation.flowSpecification.loss = fspec.loss;
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
	to_store->enrollment_attempts = request.enrollment_attempts + 1;
	to_store->abort_timer_task = new AbortEnrollmentTimerTask(this, request.neighbor_.name_,
								  -1, handle,
								  "N-1 Flow allocation timeout", false);
	timer.scheduleTask(to_store->abort_timer_task, timeout_);
	lock_.lock();
	port_ids_pending_to_be_allocated_.put(handle, to_store);
	lock_.unlock();
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
	IEnrollmentStateMachine * stateMachine;

	LOG_IPCP_DBG("M_RELEASE_R cdapMessage from portId %u",
			con_handle.port_id);

	stateMachine = getEnrollmentStateMachine(con_handle.port_id, false);
	if (!stateMachine) {
		LOG_IPCP_ERR("Problems getting enrollment state machine");
		try {
			rib_daemon_->getProxy()->remote_close_connection(con_handle.port_id);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems closing application connection: %s",
					e.what());
		}

		deallocateFlow(con_handle.port_id);
	} else {
		stateMachine->releaseResult(res, con_handle);
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
	IEnrollmentStateMachine * esm = 0;

	rina::WriteScopedLock writeLock(sm_lock);

	it = state_machines_.find(portId);
	if (it == state_machines_.end()) {
		return NULL;
	}

	esm = it->second;

	if (remove) {
		LOG_IPCP_DBG("Removing enrollment state machine associated to %d",
				portId);
		state_machines_.erase(it);
	}

	return esm;
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
		it->second->internal_port_id = neighbor.internal_port_id;
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

	neigh_lock.writelock();
	it = neighbors.find(neighbor_key);
	if (it == neighbors.end()) {
		neigh_lock.unlock();
		LOG_IPCP_WARN("Could not find neighbor for key: %s",
			      neighbor_key.c_str());
		return;
	}

	neigh = it->second;
	neighbors.erase(it);
	neigh_lock.unlock();

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
	LOG_DBG("Trying to deallocate flow %d", portId);

	try {
		irm_->deallocateNMinus1Flow(portId);
	} catch (rina::Exception &e) {
		LOG_IPCP_ERR("Problems deallocating N-1 flow %d: %s", portId, e.what());
	}
}

void EnrollmentTask::add_enrollment_state_machine(int portId, IEnrollmentStateMachine * stateMachine)
{
	rina::WriteScopedLock writeLock(sm_lock);

	state_machines_[portId] = stateMachine;
}

void EnrollmentTask::neighborDeclaredDead(rina::NeighborDeclaredDeadEvent * deadEvent)
{
	std::list<int> flows;
	std::list<int>::iterator it;

	try{
		irm_->getNMinus1FlowInformation(deadEvent->neighbor_.underlying_port_id_);
	} catch(rina::Exception &e){
		LOG_IPCP_INFO("The N-1 flow with the dead neighbor has already been deallocated");
		return;
	}

	try{
		LOG_IPCP_INFO("Requesting the deallocation of the N-1 flows with the dead neighbor");
		deallocateFlow(deadEvent->neighbor_.underlying_port_id_);
		if (n1_flows_to_create.size() > 0) {
			flows = ipcp->resource_allocator_->get_n_minus_one_flow_manager()->
					getNMinusOneFlowsToNeighbour(deadEvent->neighbor_.name_.processName);
			for (it = flows.begin(); it != flows.end(); ++it) {
				if (*it != 0)
					deallocateFlow(*it);
			}
		}
	} catch (rina::Exception &e){
		LOG_IPCP_ERR("Problems requesting the deallocation of a N-1 flow: %s", e.what());
	}

	remove_neighbor(deadEvent->neighbor_.name_.getProcessNamePlusInstance());
}

void EnrollmentTask::nMinusOneFlowDeallocated(rina::NMinusOneFlowDeallocatedEvent * event)
{
	IEnrollmentStateMachine * esm = 0;
	rina::ConnectiviyToNeighborLostEvent * event2 = 0;
	rina::FlowDeallocateRequestEvent fd_event;
	DestroyESMTimerTask * dsm_task = 0;
	IPCPRIBDaemonImpl * ribd = dynamic_cast<IPCPRIBDaemonImpl*>(ipcp->rib_daemon_);
	rina::Neighbor neighbor;

	//1 Remove the enrollment state machine from the list
	esm = getEnrollmentStateMachine(event->port_id_, true);
	if (!esm){
		return;
	}

	esm->flowDeallocated(event->port_id_);

	//2 Request deallocation of internal reliable flow
	if (esm->remote_peer_.internal_port_id != 0) {
		fd_event.portId = esm->remote_peer_.internal_port_id;
		fd_event.internal = true;
		ipcp->flow_allocator_->submitDeallocate(fd_event);

		//Stop the internal flow SDU reader
		ribd->stop_internal_flow_sdu_reader(fd_event.portId);

		ipcp->resource_allocator_->remove_temp_pduft_entry(esm->remote_peer_.address_);
	}

	//3 Notify about lost neighbor
	event2 = new rina::ConnectiviyToNeighborLostEvent(esm->remote_peer_);
	event_manager_->deliverEvent(event2);

	//4 Move destruction of state machine to a timer task
	dsm_task = new DestroyESMTimerTask(esm);
	timer.scheduleTask(dsm_task, 0);
}

void EnrollmentTask::nMinusOneFlowAllocated(rina::NMinusOneFlowAllocatedEvent * flowEvent)
{
	rina::EnrollmentRequest * request;

	lock_.lock();
	request = port_ids_pending_to_be_allocated_.erase(flowEvent->handle_);
	lock_.unlock();

	if (!request) {
		return;
	}

	timer.cancelTask(request->abort_timer_task);

	IPCPEnrollmentTaskPS * ipcp_ps = dynamic_cast<IPCPEnrollmentTaskPS *>(ps);
	assert(ipcp_ps);
	ipcp_ps->initiate_enrollment(*flowEvent, *request);

	delete request;
}

void EnrollmentTask::nMinusOneFlowAllocationFailed(rina::NMinusOneFlowAllocationFailedEvent * event)
{
	rina::EnrollmentRequest * request;

	lock_.lock();
	request = port_ids_pending_to_be_allocated_.erase(event->handle_);
	lock_.unlock();

	if (!request){
		return;
	}

	timer.cancelTask(request->abort_timer_task);

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

	if (peer_discovery_period_ms != 0) {
		RetryEnrollmentTimerTask * timer_task = new RetryEnrollmentTimerTask(this, *request);
		timer.scheduleTask(timer_task, peer_discovery_period_ms);
	}

	delete request;
}

void EnrollmentTask::enrollmentFailed(const rina::ApplicationProcessNamingInformation& remotePeerNamingInfo,
				      int portId,
				      int internal_portId,
				      const std::string& reason,
				      bool sendReleaseMessage)
{
	IEnrollmentStateMachine * stateMachine = 0;
	RetryEnrollmentTimerTask * timer_task = 0;
	rina::EnrollmentRequest enr_request;
	rina::EnrollmentRequest * pending_req;

	LOG_IPCP_ERR("An error happened during enrollment of remote IPC Process %s because of %s",
		      remotePeerNamingInfo.getEncodedString().c_str(),
		      reason.c_str());

	if (portId >= 0) {
		//1 Remove enrollment state machine from the store
		stateMachine = getEnrollmentStateMachine(portId, true);
		if (!stateMachine) {
			// We have already cleared
			return;
		}

		//2 Send message and deallocate flow if required
		if(sendReleaseMessage){
			try {
				rib_daemon_->getProxy()->remote_close_connection(portId, false);
			} catch (rina::Exception &e) {
				LOG_IPCP_ERR("Problems closing application connection: %s",
						e.what());
			}
		}

		enr_request = stateMachine->enr_request;
		deallocate_flows_and_destroy_esm(stateMachine, portId, false);
	} else {
		//N-1 flow could not be allocated
		lock_.lock();
		pending_req = port_ids_pending_to_be_allocated_.erase(internal_portId);
		lock_.unlock();

		if (!pending_req) {
			return;
		}

		enr_request = *pending_req;

		delete pending_req;
	}

	//3 If needed, retry enrollment
	if (enr_request.enrollment_attempts < max_num_enroll_attempts_) {
		timer_task = new RetryEnrollmentTimerTask(this, enr_request);
		timer.scheduleTask(timer_task, 10);
	} else if (peer_discovery_period_ms != 0) {
		timer_task = new RetryEnrollmentTimerTask(this, enr_request);
		timer.scheduleTask(timer_task, peer_discovery_period_ms);
	} else if (enr_request.ipcm_initiated_) {
		try {
			rina::extendedIPCManager->enrollToDIFResponse(enr_request.event_,
								      -1,
								      std::list<rina::Neighbor>(),
								      ipcp->get_dif_information());
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems sending message to IPC Manager: %s", e.what());
		}
	}
}

void EnrollmentTask::enrollmentCompleted(const rina::Neighbor& neighbor, bool enrollee,
			 	 	 bool prepare_handover,
					 const rina::ApplicationProcessNamingInformation& disc_neigh_name)
{
	std::list<rina::FlowSpecification>::iterator it;
	std::list<rina::FlowSpecification> fspecs;
	std::map< std::string, std::list<rina::FlowSpecification> >::iterator fit;
	rina::FlowInformation flowInformation;

	rina::NeighborAddedEvent * event = new rina::NeighborAddedEvent(neighbor, enrollee,
									prepare_handover, disc_neigh_name);
	event_manager_->deliverEvent(event);

	//Request allocation of data transfer N-1 flows if needed
	if (enrollee) {
		flowInformation.remoteAppName.processName = neighbor.name_.processName;
		flowInformation.remoteAppName.processInstance = neighbor.name_.processInstance;
		flowInformation.localAppName.processName = ipcp->get_name();
		flowInformation.localAppName.processInstance = ipcp->get_instance();
		flowInformation.difName = neighbor.supporting_dif_name_;

		fit = n1_flows_to_create.find(neighbor.supporting_dif_name_.processName);
		if (fit == n1_flows_to_create.end())
			return;
		fspecs = fit->second;

		for (it = fspecs.begin(); it != fspecs.end(); ++it) {
			if (it == fspecs.begin())
				continue;

			flowInformation.flowSpecification.delay = it->delay;
			flowInformation.flowSpecification.loss = it->loss;

			try {
				irm_->allocateNMinus1Flow(flowInformation);
			} catch (rina::Exception &e) {
				LOG_IPCP_ERR("Problems allocating N-1 flow: %s", e.what());
			}
		}
	}
}

void EnrollmentTask::release(int invoke_id,
		     	     const rina::cdap_rib::con_handle_t &con_handle)
{
	IEnrollmentStateMachine * stateMachine;

	LOG_IPCP_DBG("M_RELEASE cdapMessage from portId %u", con_handle.port_id);

	stateMachine = getEnrollmentStateMachine(con_handle.port_id,
						 true);
	if (!stateMachine) {
		//Already cleaned up, return
		return;
	}

	stateMachine->release(invoke_id, con_handle);

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

	deallocate_flows_and_destroy_esm(stateMachine, con_handle.port_id);
}

void EnrollmentTask::clean_state(unsigned int port_id)
{
	IEnrollmentStateMachine * esm;
	rina::cdap_rib::con_handle_t con_handle;
	rina::ConnectiviyToNeighborLostEvent * cnl_event = 0;
	DestroyESMTimerTask * timer_task = 0;

	esm = getEnrollmentStateMachine(port_id, true);

	deallocateFlow(port_id);

	if (esm) {
		con_handle.port_id = port_id;
		esm->release(0, con_handle);
		cnl_event = new rina::ConnectiviyToNeighborLostEvent(esm->remote_peer_);
		event_manager_->deliverEvent(cnl_event);

		//Schedule destruction of enrollment state machine in a separate thread, since it may take time
		esm->reset_state();
		timer_task = new DestroyESMTimerTask(esm);
		timer.scheduleTask(timer_task, 0);
	}
}

void EnrollmentTask::deallocate_flows_and_destroy_esm(IEnrollmentStateMachine * esm,
						      unsigned int port_id,
						      bool call_ps)
{
	IPCPRIBDaemonImpl * ribd = dynamic_cast<IPCPRIBDaemonImpl*>(ipcp->rib_daemon_);
	DestroyESMTimerTask * timer_task = 0;
	rina::FlowDeallocateRequestEvent fd_event;
	IPCPEnrollmentTaskPS * ipcp_ps = 0;
	rina::ConnectiviyToNeighborLostEvent * cnl_event = 0;
	rina::Sleep sleep;
	std::list<int> flows;
	std::list<int>::iterator it;

	//In the case of the enrollee state machine, reply to the IPC Manager
	if (call_ps) {
		ipcp_ps = dynamic_cast<IPCPEnrollmentTaskPS *>(ps);
		assert(ipcp_ps);
		ipcp_ps->inform_ipcm_about_failure(esm);
	}

	//Deallocate internal reliable flow if required
	if (esm->remote_peer_.internal_port_id > 0) {
		try {
			fd_event.internal = true;
			fd_event.portId = esm->remote_peer_.internal_port_id;
			ipcp->flow_allocator_->submitDeallocate(fd_event);

			//Stop the internal flow SDU reader
			ribd->stop_internal_flow_sdu_reader(fd_event.portId);
		} catch (rina::Exception &e) {
			LOG_IPCP_ERR("Problems deallocating internal flow: %s", e.what());
		}

		ipcp->resource_allocator_->remove_temp_pduft_entry(esm->remote_peer_.address_);
	}

	//Deallocate N-1 flows, sleep for 20 ms first (TODO Fix this)
	sleep.sleepForMili(20);
	deallocateFlow(port_id);
	if (n1_flows_to_create.size() > 0) {
		flows = ipcp->resource_allocator_->get_n_minus_one_flow_manager()->
				getNMinusOneFlowsToNeighbour(esm->remote_peer_.name_.processName);
		for (it = flows.begin(); it != flows.end(); ++it) {
			if (*it != 0)
				deallocateFlow(*it);
		}
	}

	//Inform about connectivity to neighbor lost
	cnl_event = new rina::ConnectiviyToNeighborLostEvent(esm->remote_peer_);
	event_manager_->deliverEvent(cnl_event);

	//Schedule destruction of enrollment state machine in a separate thread, since it may take time
	esm->reset_state();
	timer_task = new DestroyESMTimerTask(esm);
	timer.scheduleTask(timer_task, 0);
}

// Class Operational Status RIB Object
const std::string OperationalStatusRIBObject::class_name = "OperationalStatus";
const std::string OperationalStatusRIBObject::object_name = "/difm/ops";

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
