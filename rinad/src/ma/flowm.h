//TODO

#ifndef __RINAD_FLOW_H__
#define __RINAD_FLOW_H__

#include <pthread.h>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
#include <utility>

#include <librina/application.h>
#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/patterns.h>

#include "event-loop.h"

namespace rinad{
namespace mad{

/**
* @file flowm.h
* @author Marc Sune<marc.sune (at) bisdn.de>
*
* @brief Flow Manager
*/

/**
* @brief FlowWorker encpasulates the main I/O loop for CDAP message
* receiving.
*
* In the current implementation there is one instance of this class per
* allocated flow.
*/
class FlowWorker{

public:
	/**
	* Run the main loop until keep_on == false
	*/
	void* run(void* param);

	//Trampoline
	static void* run_trampoline(void* param){
		FlowWorker* w = (FlowWorker*)param;
		return w->run(param);
	}


	/**
	* Instruct the thread to stop.
	*/
	inline void stop(void){
		keep_on = false;
	}

	inline pthread_t* getPthreadContext(){
		return &ctx;
	}

protected:

	//Pthread context
	pthread_t ctx;

	//Wether to stop the main I/O loop
	volatile bool keep_on;
};

/**
* @brief Flow manager
*/
class FlowManager_{

public:
	/**
	* Initialize FlowManager running state
	*/
	void init(void);

	/**
	* Run the main I/O loop
	*/
	void runIOLoop(void);

	/**
	* Run the main I/O loop
	*/
	inline void stopIOLoop(void){
		keep_running = false;
	}


	/**
	* Destroy the running state
	*/
	void destroy(void);
private:
	//hashmap port_id/pthread_t
	std::map<int, FlowWorker*> workers;

	//Mutex
	pthread_mutex_t mutex;

	//Run flag
	volatile bool keep_running;

	/*
	* Create a worker
	*/
	void spawnWorker(rina::Flow& flow);

	/*
	* Join worker
	*/
	void joinWorker(int port_id);

	//Constructors
	FlowManager_(void);
	~FlowManager_(void);

	friend class Singleton<FlowManager_>;

};

//Singleton instance
extern Singleton<FlowManager_> FlowManager;

}; //namespace mad
}; //namespace rinad

#endif  /* __RINAD_FLOW_H__ */
