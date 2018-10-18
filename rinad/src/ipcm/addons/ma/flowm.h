/*
 * Flow Manager
 *
 *    Marc Sune <marc.sune (at) bisdn.de>
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
class ManagementAgent;

/**
* @brief Flow manager
*/
class FlowManager{

public:

	//Constructors
	FlowManager(ManagementAgent* agent_);
	~FlowManager(void);

	/**
	* Retrieve a **copy** of the AP naming information
	*/
	rina::ApplicationProcessNamingInformation getAPInfo(void);

	/**
	* @brief Spawns a worker and attempts to connect to the Manager
	*
	* Method may throw an exception if a new thread cannot be spawned. No
	* exception will be thrown if the connection to the Manager cannot be
	* temporally stablished (will keep retrying).
	*
	* @param con Connection parameters
	* @param keyManager true if it connects to a local KeyManager, false
	* if it connects to the Manager
	* @ret The connection worker handle; this handler is NOT the flow
	* port-id neither any CDAP connection parameter. This identifies
	* univocally the connection worker in the context of the FlowManager.
	*/
	unsigned int connectTo(const AppConnection& con, bool keyManager = false);

	/**
	* @brief Disconnects (if connected), and joins working thread
	*/
	void disconnectFrom(unsigned int worker_id);

	/**
	 * Distribute the flow event to the active workers.
	 *
	 * Shall only be called by the ManagementAgent
	 */
	void process_librina_event(rina::IPCEvent** event);

private:

	// Manage a CDAP response delegated to an IPC process
	void process_fwd_cdap_msg_response(rina::FwdCDAPMsgResponseEvent* fwdevent);

	//hashmap worker handler <-> Worker association
	std::map<unsigned int, Worker*> workers;

	//Mutex
	rina::Lockable mutex;

	/**
	* Wait condition (+mutex)
	*/
	rina::ConditionVariable wait_cond;


	/*
	* Create a worker
	*/
	unsigned int spawnWorker(Worker** w);

	/*
	* Join worker
	*/
	void joinWorker(int id);

	//Worker handler id
	int next_id;

	//Fast exit from event wait for workers
	bool keep_running;

	//Back reference
	ManagementAgent* agent_;
};


}; //namespace mad
}; //namespace rinad

#endif  /* __RINAD_FLOW_H__ */
