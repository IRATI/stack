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
#define FLOWMANAGER_TIMEOUT_NS 400

/*
* Flow worker
*/
void* FlowWorker::run(void* param){

	(void)param;
	//Recover the port_id of the flow
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
				spawnWorker(*flow);
				break;

			case rina::FLOW_DEALLOCATED_EVENT:
				port_id = dynamic_cast<rina::FlowDeallocatedEvent*>(event)->portId;
				rina::ipcManager->flowDeallocated(port_id);
				LOG_INFO("Flow torn down remotely [port-id = %d]", port_id);

				//TODO: join thread
				break;

			case rina::DEALLOCATE_FLOW_RESPONSE_EVENT:
				LOG_INFO("Destroying the flow after time-out");
				resp = dynamic_cast<rina::DeallocateFlowResponseEvent*>(event);
				port_id = resp->portId;
				rina::ipcManager->flowDeallocationResult(port_id, resp->result == 0);
				//TODO: join thread
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
FlowManager_::FlowManager_(){

	//TODO: register to flow events in librina and spawn workers
	pthread_mutex_init(&mutex, NULL);
}

FlowManager_::~FlowManager_(){
	pthread_mutex_destroy(&mutex);
}

//Initialization and destruction routines
void FlowManager_::init(){
	//TODO
	LOG_DBG("Initialized");
}
void FlowManager_::destroy(){
	//TODO
}

//
// FIXME: spawning a thread per flow is a waste of resources in general
// but currently there is no functionality like select/poll/epoll to wait for
// read events over a bunch of flows, so this is currently
// the only way to go.
//

//Workers
void FlowManager_::spawnWorker(rina::Flow& flow){

	FlowWorker* w = NULL;
	std::stringstream msg;
	int port_id = flow.getPortId();
	//Create worker object
	try{
		w = new FlowWorker();
	}catch(std::bad_alloc& e){
		msg <<std::string("ERROR: Unable to create worker context; out of memory? Dropping flow with port_id:")<<port_id<<std::endl;
		goto SPAWN_ERROR;
	}

	pthread_mutex_lock(&mutex);

	//Double check that there is not an existing worker for that port_id
	//This should never happen
	//TODO: evaluate whether to surround this with an ifdef DEBUG
	if(workers.find(port_id) != workers.end()){
		msg <<std::string("ERROR: Corrupted FlowManager internal state or double call to spawnWorker(). Dropping flow with port_id: ")<<port_id<<std::endl;
		goto SPAWN_ERROR;
	}

	//Add state to the workers map
	try{
		workers[port_id] = w;
	}catch(...){
		msg <<std::string("ERROR: Could not add the worker state to 'workers' FlowManager internal out of memory? Dropping flow with port_id: ")<<port_id<<std::endl;
		goto SPAWN_ERROR;
	}

	//Spawn pthread
	if(pthread_create(w->getPthreadContext(), NULL,
					w->run_trampoline, (void*)w) != 0){
		msg <<std::string("ERROR: Could spawn pthread for flow with port_id: ")<<port_id<<std::endl;
		goto SPAWN_ERROR;
	}

	pthread_mutex_unlock(&mutex);

	return;

SPAWN_ERROR:
	if(w){
		if(workers.find(port_id) != workers.end())
			workers.erase (port_id);
		delete w;
	}
	assert(0);
	pthread_mutex_unlock(&mutex);
	throw Exception(msg.str().c_str());
}

//Join
void FlowManager_::joinWorker(int port_id){

	std::stringstream msg;
	FlowWorker* w = NULL;

	pthread_mutex_lock(&mutex);

	if(workers.find(port_id) == workers.end()){
		msg <<std::string("ERROR: Could not find the context offlow with port_id: ")<<port_id<<std::endl;
		goto JOIN_ERROR;
	}

	//Recover FlowWorker object
	w = workers[port_id];

	//Signal
	w->stop();

	//Join
	if(pthread_join(*w->getPthreadContext(), NULL) !=0){
		msg <<std::string("ERROR: Could not join worker for flow id: ")<<port_id<<". Error: "<<strerror(errno)<<std::endl;
		goto JOIN_ERROR;
	}

	//Clean-up
	workers.erase(port_id);
	delete w;

	pthread_mutex_unlock(&mutex);

	return;

JOIN_ERROR:
	assert(0);
	pthread_mutex_unlock(&mutex);
	throw Exception(msg.str().c_str());

}

}; //namespace mad
}; //namespace rinad


