//TODO

#ifndef __RINAD_FLOW_H__
#define __RINAD_FLOW_H__

#include <pthread.h>
#include <cstdlib>
#include <iostream>
#include <list>
#include <map>
#include <vector>
#include <utility>

#include <librina/application.h>
#include <librina/common.h>
#include <librina/concurrency.h>
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

//Fwd decl
class AppConnection;
class Worker;

/**
* @brief Flow manager
*/
class FlowManager_{

public:
	/**
	* @brief Initialize FlowManager running state
	*/
	void init(void);

	/**
	* @brief Run the main I/O loop
	*/
	void runIOLoop(void);

	/**
	* @brief Run the main I/O loop
	*/
	inline void stopIOLoop(void){
		keep_running = false;
	}

	/**
	* @brief Spawns a worker and attempts to connect to the Manager
	*
	* Method may throw an exception if a new thread cannot be spawned. No
	* exception will be thrown if the connection to the Manager cannot be
	* temporally stablished (will keep retrying).
	*
	* @param con Connection parameters
	* @ret The connection worker handle; this handler is NOT the flow
	* port-id neither any CDAP connection parameter. This identifies
	* univocally the connection worker in the context of the FlowManager.
	*/
	unsigned int connectTo(const AppConnection& con);


	/**
	* @brief Disconnects (if connected), and joins working thread
	*/
	void disconnectFrom(unsigned int worker_id);

	/**
	* Destroy the running state
	*/
	void destroy(void);
private:
	//hashmap worker handler <-> Worker association
	std::map<unsigned int, Worker*> workers;

	//Mutex
	rina::Lockable mutex;

	//Run flag
	volatile bool keep_running;


	/**
	* Notify event to the worker
	*
	* Loop over all the workers and attempt to notify the event
	* TODO: this is a waste of resources, but this is currently the only
	* simple way to do this
	*
	* Being the simplest way to do it now, doesn't mean it is deep pain
	*/
	void notify(rina::IPCEvent** event);

	/*
	* Create a worker
	*/
	unsigned int spawnWorker(Worker** w);

	/*
	* Join worker
	*/
	void joinWorker(int id);

	//Constructors
	FlowManager_(void);
	~FlowManager_(void);

	friend class Singleton<FlowManager_>;

	//Worker handler id
	int next_id;
};

//Singleton instance
extern Singleton<FlowManager_> FlowManager;

}; //namespace mad
}; //namespace rinad

#endif  /* __RINAD_FLOW_H__ */
