#include "flowm.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include "agent.h"
#define RINA_PREFIX "mad.flowm"
#include <librina/logs.h>


namespace rinad{
namespace mad{

#define FLOWMANAGER_TIMEOUT_S 0
#define FLOWMANAGER_TIMEOUT_NS 100000000

/**
* @brief Worker abstract class encpasulates the main I/O loop
*/
class Worker{

public:

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
		return w->run(param);
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
	//Pthread context
	pthread_t ctx;

	//Wether to stop the main I/O loop
	volatile bool keep_running;

	//Connection being handled
	Connection con;
};

/**
* @brief Active Worker - performs flow allocation
*/
class ActiveWorker : public Worker{

public:
	/**
	* Constructor
	*/
	ActiveWorker(const Connection& _con){con = _con;};

	/**
	* Run I/O loop
	*/
	virtual void* run(void* param);

	/**
	* Destructor
	*/
	virtual ~ActiveWorker(){};
};

/*
* Flow active worker
*/
void* ActiveWorker::run(void* param){

	(void)param;

	keep_running = true;
	while(keep_running == true){
		//TODO: block for incoming messages
		sleep(1);
	}

	return NULL;
}

/*
* FlowManager
*/
void FlowManager_::runIOLoop(){

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

			case rina::FLOW_ALLOCATION_REQUESTED_EVENT:
				flow = rina::ipcManager->allocateFlowResponse(*dynamic_cast<rina::FlowRequestEvent*>(event), 0, true);
				LOG_INFO("New flow allocated [port-id = %d]", flow->getPortId());
				(void)flow;
				//TODO: add pasive worker
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

//Singleton instance
Singleton<FlowManager_> FlowManager;

//Constructors destructors(singleton)
FlowManager_::FlowManager_() : next_id(1){

}

FlowManager_::~FlowManager_(){

}

//Initialization and destruction routines
void FlowManager_::init(){
	LOG_DBG("Initialized");
}
void FlowManager_::destroy(){

	//Join all workers
	std::map<unsigned int, Worker*>::iterator it = workers.begin();

	while (it != workers.end()){
		std::map<unsigned int, Worker*>::iterator to_delete = it;

		//Stop and join
		it->second->stop();

		//Join
		joinWorker(it->first);

		//Remove
		workers.erase(to_delete);
		it++;
	}
}

//Connect manager
unsigned int FlowManager_::connectTo(const Connection& con){
	Worker* w = new ActiveWorker(con);

	//Launch worker and return handler
	return spawnWorker(&w);
}

//Disconnect
void FlowManager_::disconnectFrom(unsigned int worker_id){
	joinWorker(worker_id);
}

//
// FIXME: spawning a thread per flow is a waste of resources in general
// but currently there is no functionality like select/poll/epoll to wait for
// read events over a bunch of flows, so this is currently
// the only way to go.
//

//Workers
unsigned int FlowManager_::spawnWorker(Worker** w){

	unsigned int id;
	std::stringstream msg;

	//Scoped lock
	rina::AccessGuard lock(mutex);

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
	throw Exception(msg.str().c_str());
}

//Join
void FlowManager_::joinWorker(int id){

	std::stringstream msg;
	Worker* w = NULL;

	//Scoped lock
	rina::AccessGuard lock(mutex);

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
	throw Exception(msg.str().c_str());
}

}; //namespace mad
}; //namespace rinad


