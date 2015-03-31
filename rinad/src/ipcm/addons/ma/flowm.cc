#include "flowm.h"

#include <algorithm>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include "agent.h"
#define RINA_PREFIX "ipcm.mad.flowm"
#include <librina/logs.h>


namespace rinad{
namespace mad{

//General events timeout
#define FLOWMANAGER_TIMEOUT_S 0
#define FLOWMANAGER_TIMEOUT_NS 100000000

//Flow allocation worker events
#define FM_FALLOC_TIMEOUT_S 10
#define FM_FALLOC_TIMEOUT_NS 0

/**
* @brief Worker abstract class encpasulates the main I/O loop
*/
class Worker{

public:

	/**
	* Constructor
	*/
	Worker(FlowManager* fm) : flow_manager(fm){};

	/**
	* Destructor
	*/
	virtual ~Worker(void){};

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
		if(std::find(pend_events.begin(), pend_events.end(),
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
	rina::IPCEvent* waitForEvent(long seconds=0, long nanoseconds=0){

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
		keep_running = false;
	}

	inline pthread_t* getPthreadContext(){
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
};

/**
* @brief Active Worker - performs flow allocation
*/
class ActiveWorker : public Worker{

public:
	/**
	* Constructor
	*/
	ActiveWorker(FlowManager* fm, const AppConnection& _con) : Worker(fm){
		con = _con;
	};

	/**
	* Run I/O loop
	*/
	virtual void* run(void* param);

	/**
	* Destructor
	*/
	virtual ~ActiveWorker(){};

protected:

	/**
	* Allocate a flow and return the Flow object or NULL
	*/
	rina::Flow* allocateFlow();
};


//Allocate a flow
rina::Flow* ActiveWorker::allocateFlow(){

        unsigned int seqnum;
	rina::Flow* flow = NULL;
	rina::IPCEvent* event;
        rina::AllocateFlowRequestResultEvent* rrevent;

	//Setup necessary QoS
	//TODO Quality of Service specification
	//FIXME: Move to connection
	rina::FlowSpecification qos;

	{
		//we must do this in mutual exclusion to prevent a race cond.
		//between the next call and the storing of the seqnum
	  rina::ScopedLock lock(mutex);

		//Perform the flow allocation
		seqnum = rina::ipcManager->requestFlowAllocationInDIF(
				flow_manager->getAPInfo(),
				con.flow_info.remoteAppName,
				con.flow_info.difName,
				qos);

		LOG_DBG("[w:%u] Waiting for event %u", id, seqnum);
		pend_events.push_back(seqnum);
	}

	//Wait for the event
	try{
		event = waitForEvent(FM_FALLOC_TIMEOUT_S,
							FM_FALLOC_TIMEOUT_NS);
		LOG_DBG("[w:%u] Got event %u, waiting for %u", id,
						event->sequenceNumber,
						seqnum);
	}catch(rina::Exception& e){
		//No reply... no flow
		LOG_ERR("[w:%u] Failed to allocate a flow. Operation timed-out", id);
		return NULL;
	}

	if(!event){
		LOG_ERR("[w:%u] Failed to allocate a flow. Unknown error", id);
		assert(0);
		return NULL;
	}

	//Check if it is the right event for us
	if(event->eventType == rina::ALLOCATE_FLOW_REQUEST_RESULT_EVENT
                                && event->sequenceNumber == seqnum) {
        	rrevent =
		dynamic_cast<rina::AllocateFlowRequestResultEvent*>(event);
	}else{
		return NULL;
	}

	//Recover the flow
        flow = rina::ipcManager->commitPendingFlow(rrevent->sequenceNumber,
                                              rrevent->portId,
                                              rrevent->difName);

        if(!flow || flow->getPortId() == -1){
		LOG_ERR("[w:%u] Failed to allocate a flow.", id);
		return NULL;
	}
	LOG_INFO("[w:%u] Flow allocated, port id = %d", id, flow->getPortId());

	return flow;
}

// Flow active worker
void* ActiveWorker::run(void* param){

	rina::Flow* flow;

	//We don't use param
	(void)param;

	keep_running = true;
	while(keep_running == true){

		//Allocate the flow
		flow = allocateFlow();
		(void)flow;
		//TODO: block for incoming messages
		sleep(1);
	}

	return NULL;
}

/*
* FlowManager
*/
void FlowManager::runIOLoop(){

	rina::IPCEvent *event;
	rina::Flow *flow;
	unsigned int port_id;
	rina::DeallocateFlowResponseEvent *resp;

	keep_running = true;

	//wait for events
	while(keep_running){
		event = rina::ipcEventProducer->eventTimedWait(
						FLOWMANAGER_TIMEOUT_S,
						FLOWMANAGER_TIMEOUT_NS);
		flow = NULL;
		resp = NULL;

		if(!event){
			continue;
		}

		switch(event->eventType) {
			case rina::REGISTER_APPLICATION_RESPONSE_EVENT:
				rina::ipcManager->commitPendingRegistration(event->sequenceNumber,
					     dynamic_cast<rina::RegisterApplicationResponseEvent*>(event)->DIFName);
				break;

			case rina::UNREGISTER_APPLICATION_RESPONSE_EVENT:
				rina::ipcManager->appUnregistrationResult(event->sequenceNumber,
					    dynamic_cast<rina::UnregisterApplicationResponseEvent*>(event)->result == 0);
				break;


			case rina::ALLOCATE_FLOW_REQUEST_RESULT_EVENT:
				//Attempt first to notify the event to the
				//active workers

				notify(&event);
				if(event){
					assert(0);
					delete event;
				}
				break;
			case rina::FLOW_ALLOCATION_REQUESTED_EVENT:
				//TODO: add pasive worker
				LOG_INFO("New flow allocated [port-id = %d]", flow->getPortId());

				flow = rina::ipcManager->allocateFlowResponse(*dynamic_cast<rina::FlowRequestEvent*>(event), 0, true);
				(void)flow;
				break;

			case rina::FLOW_DEALLOCATED_EVENT:
				port_id = dynamic_cast<rina::FlowDeallocatedEvent*>(event)->portId;
				rina::ipcManager->flowDeallocated(port_id);
				LOG_INFO("Flow torn down remotely [port-id = %d]", port_id);
				//TODO: add pasive worker
				//joinWorker(port_id);
				break;

			case rina::DEALLOCATE_FLOW_RESPONSE_EVENT:
				LOG_INFO("Destroying the flow after time-out");
				resp = dynamic_cast<rina::DeallocateFlowResponseEvent*>(event);
				port_id = resp->portId;
				rina::ipcManager->flowDeallocationResult(port_id, resp->result == 0);
				//TODO: add pasive worker
				//joinWorker(port_id);
				break;

			default:
				assert(0);
				LOG_ERR("Got unknown event %d",
							event->eventType);
			break;
		}
	}
}

//Constructors destructors(singleton)
FlowManager::FlowManager(ManagementAgent* agent_) : next_id(1), agent(agent_){
	LOG_DBG("Initialized");
}

FlowManager::~FlowManager(){
	//Join all workers
	std::map<unsigned int, Worker*>::iterator it = workers.begin();

	while (it != workers.end()){
		//Stop and join
		it->second->stop();

		//Join
		joinWorker(it->first);

		//Remove
		it++;
	}
}


//Connect manager
unsigned int FlowManager::connectTo(const AppConnection& con){
	Worker* w = new ActiveWorker(this, con);

	//Launch worker and return handler
	return spawnWorker(&w);
}

rina::ApplicationProcessNamingInformation FlowManager::getAPInfo(void){
	return agent->getAPInfo();
}

//Disconnect
void FlowManager::disconnectFrom(unsigned int worker_id){
	joinWorker(worker_id);
}

//Notify
void FlowManager::notify(rina::IPCEvent** event){

	std::map<unsigned int, Worker*>::iterator it;
	//TODO: use rwlock
	rina::ScopedLock lock(mutex);

	for( it = workers.begin(); it!=workers.end(); ++it) {
		//Notify current worker
		it->second->notify(event);

		//Check if this worker had consumed the event
		if(!*event)
			return;
	}
}

//
// FIXME: spawning a thread per flow is a waste of resources in general
// but currently there is no functionality like select/poll/epoll to wait for
// read events over a bunch of flows, so this is currently
// the only way to go.
//

//Workers
unsigned int FlowManager::spawnWorker(Worker** w){

	unsigned int id;
	std::stringstream msg;

	//Scoped lock TODO use rwlock
	rina::ScopedLock lock(mutex);

	if(!*w){
		assert(0);
		msg <<std::string("Invalid worker")<<std::endl;
		goto SPAWN_ERROR3;
	}

	//Assign a unique id to this connection
	id = next_id++;

	//Double check that there is not an existing worker for that id
	//This should never happen
	//TODO: evaluate whether to surround this with an ifdef DEBUG
	if(workers.find(id) != workers.end()){
		msg <<std::string("ERROR: Corrupted FlowManager internal state or double call to spawnWorker().")<<id<<std::endl;
		goto SPAWN_ERROR2;
	}

	//Add state to the workers map
	try{
		workers[id] = *w;
	}catch(...){
		msg <<std::string("ERROR: Could not add the worker state to 'workers' FlowManager internal out of memory? Dropping worker: ")<<id<<std::endl;
		goto SPAWN_ERROR;
	}

	//Set the unique identifier
	(*w)->setId(id);

	//Spawn pthread
	if(pthread_create((*w)->getPthreadContext(), NULL,
				(*w)->run_trampoline, (void*)(*w)) != 0){
		msg <<std::string("ERROR: Could spawn pthread for worker id: ")<<id<<std::endl;
		goto SPAWN_ERROR;
	}

	//Retain worker pointer
	*w = NULL;

	return id;

SPAWN_ERROR:
	if(workers.find(id) != workers.end()) workers.erase(id);
SPAWN_ERROR2:
	delete *w;
SPAWN_ERROR3:
	throw rina::Exception(msg.str().c_str());
}

//Join
void FlowManager::joinWorker(int id){

	std::stringstream msg;
	Worker* w = NULL;

	//Scoped lock
	rina::ScopedLock lock(mutex);

	if(workers.find(id) == workers.end()){
		msg <<std::string("ERROR: Could not find the context of worker id: ")<<id<<std::endl;
		goto JOIN_ERROR;
	}

	//Recover FlowWorker object
	w = workers[id];

	//Signal
	w->stop();

	//Join
	if(pthread_join(*(w->getPthreadContext()), NULL) !=0){
		msg <<std::string("ERROR: Could not join worker with id: ")<<id<<". Error: "<<strerror(errno)<<std::endl;
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

}; //namespace mad
}; //namespace rinad


