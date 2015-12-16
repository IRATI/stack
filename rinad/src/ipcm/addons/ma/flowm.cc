/*
 * Flow Manager
 *
 *    Marc Sune	     <marc.sune (at) bisdn.de>
 *    Bernat Gaston	 <bernat.gaston@i2cat.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#include "flowm.h"

#include <algorithm>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include "agent.h"
#include "ribf.h"
#define RINA_PREFIX "ipcm.mad.flowm"
#include <librina/likely.h>
#include <librina/logs.h>
#include <librina/cdap_v2.h>
#include <librina/rib_v2.h>
#include <librina/security-manager.h>

namespace rinad {
namespace mad {

//General events timeout
#define FM_TIMEOUT_S 5
#define FM_RETRY_NSEC 100000000 //100ms
#define _FM_1_SEC_NSEC 1000000000

//Flow allocation worker events
#define FM_FALLOC_TIMEOUT_S 10
#define FM_FALLOC_TIMEOUT_NS 0
#define FM_FALLOC_ALLOC_RETRY_US 400000 //400ms
const unsigned int max_sdu_size_in_bytes = 10000;

/**
 * @brief Worker abstract class encpasulates the main I/O loop
 */
class Worker {

 public:

	/**
	 * Constructor
	 */
	Worker(FlowManager* fm, RIBFactory *rib_factory)
			: flow_manager(fm),
			  rib_factory_(rib_factory)
	{
	}

	/**
	 * Destructor
	 */
	virtual ~Worker(void) {}

	/**
	 * Run I/O loop
	 */
	virtual void* run(void* param)=0;

	//Trampoline
	static void* run_trampoline(void* param){
		Worker* w = static_cast<Worker*>(param);
		return w->run(NULL);
	}

	/**
	 * Notify an event
	 */
	void notify(rina::IPCEvent** event){

		//Do this in mutual exclusion
		rina::ScopedLock lock(mutex);

		//Check if this event is for this worker
		if (std::find(pend_events.begin(), pend_events.end(),
			      (*event)->sequenceNumber) == pend_events.end())
			return;

		//Store and signal
		events.push_back(*event);
		cond.signal();
		*event = NULL;
	}

	/**
	 * Wait for an event
	 */
	rina::IPCEvent* waitForEvent(long seconds = 0, long nanoseconds = 0) {

		cond.timedwait(seconds, nanoseconds);
		rina::ScopedLock lock(mutex);
		rina::IPCEvent* ev = *events.begin();
		pend_events.pop_front();
		return ev;
	}

	/**
	 * Set the id
	 */
	void setId(unsigned int _id){
		id = _id;
	}

	/**
	 * Instruct the thread to stop.
	 */
	inline void stop(void){

		//Must be set strictly before deallocation of the thread
		keep_running = false;

		try{
			rina::ipcManager->requestFlowDeallocation(port_id);
		}catch(...){}
	}

	inline pthread_t* getPthreadContext() {
		return &ctx;
	}

 protected:
	//Worker id
	unsigned int id;

	//Pthread context
	pthread_t ctx;

	//Mutex
	rina::Lockable mutex;

	//Pending events
	std::list<rina::IPCEvent*> events;

	//Pending transactions (sequence numbers)
	std::list<unsigned int> pend_events;

	//Condition variable
	rina::ConditionVariable cond;

	//Wether to stop the main I/O loop
	volatile bool keep_running;

	//Connection being handled
	AppConnection con;

	//Back reference
	FlowManager* flow_manager;

	//RIBFactory
	RIBFactory* rib_factory_;

	//Flow being used
	rina::FlowInformation flow_;

	//Port-id of the flow in use
	int port_id;
};

/**
 * @brief Active Worker - performs flow allocation
 */
class ActiveWorker : public Worker {

 public:
	/**
	 * Constructor
	 */
	ActiveWorker(FlowManager* fm, RIBFactory* rib_factory,
		     const AppConnection& _con)
			: Worker(fm, rib_factory)
	{
		con = _con;
	}

	/**
	 * Run I/O loop
	 */
	virtual void* run(void* param);

	/**
	 * Destructor
	 */
	virtual ~ActiveWorker() {}

 protected:

	/**
	 * Allocate a flow and return the Flow object or NULL
	 */
	void allocateFlow();
};

//Allocate a flow
void ActiveWorker::allocateFlow()
{

	unsigned int seqnum;
	rina::IPCEvent* event;
	rina::AllocateFlowRequestResultEvent* rrevent;

	//Setup necessary QoS
	//TODO Quality of Service specification
	//FIXME: Move to connection
	rina::FlowSpecification qos;
	qos.maxAllowableGap = 0;

	//Perform the flow allocation
	seqnum = rina::ipcManager->requestFlowAllocationInDIF(
			flow_manager->getAPInfo(), con.flow_info.remoteAppName,
			con.flow_info.difName, qos);

	LOG_DBG("[w:%u] Waiting for event %u", id, seqnum);
	pend_events.push_back(seqnum);

	//Wait for the event
	try {
		event = flow_manager->wait_event(seqnum);
	} catch (rina::Exception& e) {
		//No reply... no flow
		LOG_ERR("[w:%u] Failed to allocate a flow. Operation timed-out",
			id);
		rina::ipcManager->withdrawPendingFlow(seqnum);
		flow_.portId = -1;
		return;
	}
	if (!event) {
		LOG_ERR("[w:%u] Failed to allocate a flow. Unknown error", id);
		rina::ipcManager->withdrawPendingFlow(seqnum);
		flow_.portId = -1;
		return;
	}
	LOG_DBG("[w:%u] Got event %u, waiting for %u", id,
		event->sequenceNumber, seqnum);

	//Check if it is the right event for us
	rrevent = dynamic_cast<rina::AllocateFlowRequestResultEvent*>(event);
	if (!rrevent || rrevent->portId < 0) {
		rina::ipcManager->withdrawPendingFlow(seqnum);
		flow_.portId = -1;
	}else{
		//Recover the flow
		flow_ = rina::ipcManager->commitPendingFlow(rrevent->sequenceNumber,
							   rrevent->portId,
							   rrevent->difName);
		LOG_INFO("[w:%u] Flow allocated, port id = %d", id, flow_.portId);
	}

	// Delete the event
	delete event;
}

// Flow active worker
void* ActiveWorker::run(void* param)
{

	unsigned char *buffer;
	rina::cdap_rib::ep_info_t src;
	rina::cdap_rib::ep_info_t dest;
	int bytes_read = 0;

	keep_running = true;

	while(keep_running) {

		//Allocate the flow
		allocateFlow();

		try {
			if(flow_.portId < 0) {
				usleep(FM_FALLOC_ALLOC_RETRY_US);
				continue;
			}

			//Recheck after flow alloc (prevents race)
			if(!keep_running){
				break;
			}

			//Set port id so that we can print traces even if the
			//flow is gone
			port_id = flow_.portId;

			//Fill source parameters
			src.ap_name_ = flow_.localAppName.processName;
			src.ae_name_ = "v1";
			src.ap_inst_ = flow_.localAppName.processInstance;
			src.ae_inst_ = flow_.localAppName.entityInstance;

			//Fill destination parameters
			dest.ap_name_ = flow_.remoteAppName.processName;
			dest.ae_name_ = flow_.remoteAppName.entityName;
			dest.ap_inst_ = flow_.remoteAppName.processInstance;
			dest.ae_inst_ = flow_.remoteAppName.entityInstance;
			rina::cdap_rib::auth_policy_t auth;
			auth.name = rina::IAuthPolicySet::AUTH_NONE;

			//Version
			rina::cdap_rib::vers_info_t vers;
			vers.version_ = 0x1; //TODO: do not hardcode this

			//TODO: remove this. The API should NOT require a RIB
			//instance for calling the remote API
			rib_factory_->getProxy()->remote_open_connection(vers,
							src,
							dest,
							auth,
							port_id);

			//Recover the response
			//TODO: add support for other
			buffer = new unsigned char[max_sdu_size_in_bytes];
			try{
				bytes_read = rina::ipcManager->readSDU(port_id, buffer,
							max_sdu_size_in_bytes);
			}catch(rina::ReadSDUException &e){
				LOG_ERR("Cannot read from flow with port id: %u anymore", port_id);
				delete[] buffer;
			}

			rina::ser_obj_t message;

			message.message_ = buffer;
			message.size_ = bytes_read;

			//Instruct CDAP provider to process the CACEP message
			try{
				rina::cdap::getProvider()->process_message(message,
							port_id);
			}catch(rina::WriteSDUException &e){
				LOG_ERR("Cannot write to flow with port id: %u anymore", port_id);
			}

			LOG_DBG("Connection stablished between MAD and Manager (port id: %u)", port_id);

			//I/O loop
			while(true) {
				buffer = new unsigned char[max_sdu_size_in_bytes];
				try{
					bytes_read = rina::ipcManager->readSDU(port_id,
									buffer,
									max_sdu_size_in_bytes);
				}catch(rina::ReadSDUException &e){
					LOG_ERR("Cannot read from flow with port id: %u anymore", port_id);
					delete[] buffer;
				}

				rina::ser_obj_t message;
				message.message_ = buffer;
				message.size_ = bytes_read;

				//Instruct CDAP provider to process the RIB operation message
				try{
					rina::cdap::getProvider()->process_message(
									message,
									port_id);
				}catch(rina::WriteSDUException &e){
					LOG_ERR("Cannot write to flow with port id: %u anymore", port_id);
				}catch(rina::cdap::CDAPException &e){
					LOG_ERR("Error processing message: %s", e.what());
				}
			}
		}
		catch(...){
			LOG_CRIT("Unknown error during operation with port id: %u. This is a bug, please report it", port_id);
		}
	}

	return NULL;
}

void FlowManager::process_fwd_cdap_msg_response(rina::FwdCDAPMsgEvent* fwdevent)
{
	rina::cdap::CDAPMessage rmsg;

	LOG_DBG("Received forwarded CDAP response, result %d",
			fwdevent->result);

	if (fwdevent->sermsg.message_ == 0) {
		LOG_DBG("Received empty delegated CDAP response");
		return;
	}

	rina::cdap::getProvider()->get_session_manager()->decodeCDAPMessage(fwdevent->sermsg,
									    rmsg);

	LOG_DBG("Delegated CDAP response: %s, value %p",
		rmsg.to_string().c_str(),
		rmsg.obj_value_.message_);
}

//Process an event coming from librina
void FlowManager::process_librina_event(rina::IPCEvent** event_)
{

	rina::FlowInformation flow;
	unsigned int port_id;
	rina::DeallocateFlowResponseEvent *resp;

	//Make it easy
	rina::IPCEvent* event = *event_;

	if (!event)
		return;

	switch (event->eventType) {
		case rina::REGISTER_APPLICATION_RESPONSE_EVENT:
			rina::ipcManager->commitPendingRegistration(
					event->sequenceNumber,
					dynamic_cast<rina::RegisterApplicationResponseEvent*>(event)
							->DIFName);
			break;

		case rina::UNREGISTER_APPLICATION_RESPONSE_EVENT:
			rina::ipcManager->appUnregistrationResult(
					event->sequenceNumber,
					dynamic_cast<rina::UnregisterApplicationResponseEvent*>(event)
							->result == 0);
			break;

		case rina::ALLOCATE_FLOW_REQUEST_RESULT_EVENT:
			//The event could not be distributed at first
			//This is likely because the event arrived before the
			//callee has invoked wait()
			//Store and continue
			store_event(event);
			*event_ = NULL;
			return;  //Do not delete
		case rina::FLOW_ALLOCATION_REQUESTED_EVENT:
			//TODO: add pasive worker
			flow = rina::ipcManager->allocateFlowResponse(
					*dynamic_cast<rina::FlowRequestEvent*>(event),
					0, true);
			LOG_INFO("New flow allocated [port-id = %d]",
				 flow.portId);
			break;

		case rina::FLOW_DEALLOCATED_EVENT:
			port_id = dynamic_cast<rina::FlowDeallocatedEvent*>(event)
							->portId;
			rina::ipcManager->flowDeallocated(port_id);
			LOG_INFO("Flow torn down remotely [port-id = %d]",
				 port_id);
			//TODO: add pasive worker
			//joinWorker(port_id);
			break;

		case rina::DEALLOCATE_FLOW_RESPONSE_EVENT:
			LOG_INFO("Destroying the flow after time-out");
			resp = dynamic_cast<rina::DeallocateFlowResponseEvent*>(event);
			port_id = resp->portId;
			rina::ipcManager->flowDeallocationResult(
					port_id, resp->result == 0);
			//TODO: add pasive worker
			//joinWorker(port_id);
			break;

		case rina::IPC_PROCESS_FWD_CDAP_MSG:
		{
			process_fwd_cdap_msg_response(
				dynamic_cast<rina::FwdCDAPMsgEvent*>(event));
			break;
		}

		default:
			assert(0);
			LOG_ERR("Got unknown event %d", event->eventType);
			break;
	}

	delete event;
	*event_ = NULL;
}

//Constructors destructors(singleton)
FlowManager::FlowManager(ManagementAgent* agent)
		: next_id(1),
		  agent_(agent)
{
	keep_running = true;
	LOG_DBG("Initialized");
}

FlowManager::~FlowManager()
{

	keep_running = false;

	//Join all workers
	std::map<unsigned int, Worker*>::iterator it, next;

	it = workers.begin();

	while (it != workers.end()) {
		//Stop and join
		//it->second->stop();

		//Keep next
		next = it;
		next++;

		//Join
		joinWorker(it->first);

		//Remove
		it = next;
	}

	// Delete pending elements
	for (std::map<unsigned int, rina::IPCEvent*>::const_iterator it = pending_events.begin();
			it != pending_events.end(); it++){
		delete it->second;
	}

}

//Connect manager
unsigned int FlowManager::connectTo(const AppConnection& con)
{
	Worker* w = new ActiveWorker(this, agent_->get_ribf(), con);

	//Launch worker and return handler
	return spawnWorker(&w);
}

rina::ApplicationProcessNamingInformation FlowManager::getAPInfo(void)
{
	return agent_->getAPInfo();
}

//Disconnect
void FlowManager::disconnectFrom(unsigned int worker_id)
{
	joinWorker(worker_id);
}

//Stores and notifies an event
void FlowManager::store_event(rina::IPCEvent* event)
{

	unsigned int seqnum = event->sequenceNumber;

	//Lock to access the map
	//NOTE: this is confusing and ugly as hell. wait_cond is also Lockable
	//(mutex).
	//Blame the author who made ConditionVariable inherit from
	//Lockable(mutex) instead of mutex being a member.
	wait_cond.lock();

	try {
		//Check if already exists
		if (pending_events.find(seqnum) != pending_events.end()) {
			LOG_WARN("Duplicated seqnum '%u' for event of type '%s. Overwritting previous event...",
				 seqnum,
				 rina::IPCEvent::eventTypeToString(
						 event->eventType).c_str());
			assert(0);
		}
		pending_events[seqnum] = event;
	} catch (...) {
		wait_cond.unlock();
		LOG_WARN("Unable to store event of type %s and sequence number %u. Destroying...",
			 rina::IPCEvent::eventTypeToString(event->eventType)
					 .c_str(),
			 seqnum);
		delete event;
		return;
	}

	wait_cond.unlock();
}

// Checks whether an operation has already finalised
rina::IPCEvent* FlowManager::get_event(unsigned int seqnum)
{

	rina::IPCEvent* event = NULL;

	//Lock to access the map
	//NOTE: this is confusing and ugly as hell. wait_cond is also Lockable
	//(mutex).
	//Blame the author who made ConditionVariable inherit from
	//Lockable(mutex) instead of mutex being a member.
	rina::ScopedLock lock(wait_cond);

	if (pending_events.find(seqnum) != pending_events.end()) {
		event = pending_events[seqnum];
		pending_events.erase(seqnum);
	}

	return event;
}

//Blocks until the operation has finalised, or hard timeout is reached
rina::IPCEvent* FlowManager::wait_event(unsigned int seqnum)
{

	int i;
	rina::IPCEvent* event = NULL;

	//Lock to access the map
	//NOTE: this is confusing and ugly as hell. wait_cond is also Lockable
	//(mutex).
	//Blame the author who made ConditionVariable inherit from
	//Lockable(mutex) instead of mutex being a member.
	wait_cond.lock();

	//This is here in purpose. The event could have arrived BEFORE
	//we actually are waiting for it
	if (pending_events.find(seqnum) != pending_events.end()) {
		event = pending_events[seqnum];
		pending_events.erase(seqnum);
		wait_cond.unlock();
		return event;
	}

	for (i = 0; i < FM_TIMEOUT_S * (_FM_1_SEC_NSEC / FM_RETRY_NSEC); ++i) {
		try {
			if (unlikely(keep_running == false))
				break;
			if (pending_events.find(seqnum)
					!= pending_events.end()) {
				event = pending_events[seqnum];
				pending_events.erase(seqnum);
				break;
			}
			//Just wait
			wait_cond.timedwait(0, FM_RETRY_NSEC);
		} catch (...) {
		}
	}

	wait_cond.unlock();

	return event;
}

// Blocks until the operation has finalised, or timeout is reached
rina::IPCEvent* FlowManager::timed_wait_event(unsigned int seqnum,
					      unsigned int sec,
					      unsigned int nsec)
{

	rina::IPCEvent* event = NULL;

	//Lock to access the map
	//NOTE: this is confusing and ugly as hell. wait_cond is also Lockable
	//(mutex).
	//Blame the author who made ConditionVariable inherit from
	//Lockable(mutex) instead of mutex being a member.
	wait_cond.lock();

	//This is here in purpose. The event could have arrived BEFORE
	//we actually are waiting for it
	if (pending_events.find(seqnum) != pending_events.end()) {
		event = pending_events[seqnum];
		pending_events.erase(seqnum);
		wait_cond.unlock();
		return event;
	}

	//Perform wait
	wait_cond.timedwait(sec, nsec);

	if (pending_events.find(seqnum) != pending_events.end()) {
		event = get_event(seqnum);
		pending_events.erase(seqnum);
	}

	wait_cond.unlock();

	return event;
}
//
// FIXME: spawning a thread per flow is a waste of resources in general
// but currently there is no functionality like select/poll/epoll to wait for
// read events over a bunch of flows, so this is currently
// the only way to go.
//

//Workers
unsigned int FlowManager::spawnWorker(Worker** w)
{

	unsigned int id;
	std::stringstream msg;

	//Scoped lock TODO use rwlock
	rina::ScopedLock lock(mutex);

	if (!*w) {
		assert(0);
		msg << std::string("Invalid worker") << std::endl;
		goto SPAWN_ERROR3;
	}

	//Assign a unique id to this connection
	id = next_id++;

	//Double check that there is not an existing worker for that id
	//This should never happen
	//TODO: evaluate whether to surround this with an ifdef DEBUG
	if (workers.find(id) != workers.end()) {
		msg << std::string(
				"ERROR: Corrupted FlowManager internal state or double call to spawnWorker().")
		    << id << std::endl;
		goto SPAWN_ERROR2;
	}

	//Add state to the workers map
	try {
		workers[id] = *w;
	} catch (...) {
		msg << std::string(
				"ERROR: Could not add the worker state to 'workers' FlowManager internal out of memory? Dropping worker: ")
		    << id << std::endl;
		goto SPAWN_ERROR;
	}

	//Set the unique identifier
	(*w)->setId(id);

	//Spawn pthread
	if (pthread_create((*w)->getPthreadContext(), NULL,
			   (*w)->run_trampoline, (void*) (*w)) != 0) {
		msg << std::string("ERROR: Could spawn pthread for worker id: ")
		    << id << std::endl;
		goto SPAWN_ERROR;
	}

	//Retain worker pointer
	*w = NULL;

	return id;

	SPAWN_ERROR: if (workers.find(id) != workers.end())
		workers.erase(id);
	SPAWN_ERROR2: delete *w;
	SPAWN_ERROR3: throw rina::Exception(msg.str().c_str());
}

//Join
void FlowManager::joinWorker(int id)
{

	std::stringstream msg;
	Worker* w = NULL;

	//Scoped lock
	rina::ScopedLock lock(mutex);

	if (workers.find(id) == workers.end()) {
		msg << std::string(
				"ERROR: Could not find the context of worker id: ")
		    << id << std::endl;
		goto JOIN_ERROR;
	}

	//Recover FlowWorker object
	w = workers[id];

	//Signal
	w->stop();

	//Join
	if (pthread_join(*(w->getPthreadContext()), NULL) != 0) {
		msg << std::string("ERROR: Could not join worker with id: ")
		    << id << ". Error: " << strerror(errno) << std::endl;
		goto JOIN_ERROR;
	}
	//Clean-up
	workers.erase(id);
	delete w;

	return;

	JOIN_ERROR:
	assert(0);
	throw rina::Exception(msg.str().c_str());
}

}//namespace mad
}//namespace rinad
