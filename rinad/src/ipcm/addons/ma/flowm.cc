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
#include "../../ipcm.h"

#include <irati/kucommon.h>
#include <irati/serdes-utils.h>
#include <rina/api.h>

namespace rinad {
namespace mad {

//General events timeout
#define FM_TIMEOUT_S 5
#define FM_RETRY_NSEC 100000000 //100 ms
#define _FM_1_SEC_NSEC 1000000000

//Flow allocation worker events
#define FM_FALLOC_TIMEOUT_S 10
#define FM_FALLOC_TIMEOUT_NS 0
#define FM_FALLOC_ALLOC_RETRY_US 5000000 //5 seconds
const unsigned int max_sdu_size_in_bytes = 10000;

/**
 * @brief Worker abstract class encpasulates the main I/O loop
 */
class Worker {

 public:

	/**
	 * Constructor
	 */
	Worker(FlowManager* fm,
	       RIBFactory *rib_factory)
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

		if (flow_.fd > 0) {
			close(flow_.fd);
		}
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
};

/**
 * @brief Active Worker - performs flow allocation
 */
class ActiveWorker : public Worker {

 public:
	/**
	 * Constructor
	 */
	ActiveWorker(FlowManager* fm,
		     RIBFactory* rib_factory,
		     const AppConnection& _con,
		     bool key_manager)
			: Worker(fm, rib_factory)
	{
		con = _con;
		auth_ps_ = 0;
		key_manager_ = key_manager;
		LOG_DBG("ActiveWorker ready");
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

	rina::IAuthPolicySet * auth_ps_;
	bool key_manager_;
};

//Allocate a flow
void ActiveWorker::allocateFlow()
{
	struct rina_flow_spec fspec;
	int flowfd;
	char * this_apn, * remote_apn;
	struct name * this_name, * remote_name;
	rina::ApplicationProcessNamingInformation my_apn;

	// Preserve SDU boundaries
	// Reliable and in-order delivery required
	rina_flow_spec_unreliable(&fspec);
	fspec.msg_boundaries = true;
	//fspec.in_order_delivery = true;
	//fspec.max_sdu_gap = 0;

	remote_name = rina_name_create();
	remote_name->process_name = strdup(con.flow_info.remoteAppName.processName.c_str());
	remote_name->process_instance = strdup(con.flow_info.remoteAppName.processInstance.c_str());
	remote_name->entity_name = strdup(con.flow_info.remoteAppName.entityName.c_str());
	remote_name->entity_instance = strdup(con.flow_info.remoteAppName.entityInstance.c_str());
	remote_apn = rina_name_to_string(remote_name);
	rina_name_free(remote_name);

	my_apn = flow_manager->getAPInfo();
	this_name = rina_name_create();
	this_name->process_name = strdup(my_apn.processName.c_str());
	this_name->process_instance = strdup(my_apn.processInstance.c_str());
	this_name->entity_name = strdup(my_apn.entityName.c_str());
	this_name->entity_instance = strdup(my_apn.entityInstance.c_str());
	this_apn = rina_name_to_string(this_name);
	rina_name_free(this_name);

	LOG_DBG("Trying to allocate flow between %s and %s", this_apn, remote_apn);
	flowfd = rina_flow_alloc(NULL, this_apn, remote_apn, &fspec, 0);
	free(remote_apn);
	free(this_apn);
	if (flowfd < 0) {
		LOG_ERR("Problems allocating flow: %d, %d, %s",
			flowfd, errno, strerror(errno));
		flow_.portId = -1;
		flow_.fd = -1;
		return;
	}

	LOG_DBG("Flow allocated, got file descriptor %d", flowfd);

	flow_.portId = flowfd;
	flow_.fd = flowfd;
	flow_.localAppName = my_apn;
	flow_.remoteAppName = con.flow_info.remoteAppName;

	rina::cdap::add_fd_to_port_id_mapping(flowfd, flowfd);
}

// Flow active worker
void* ActiveWorker::run(void* param)
{
	rina::cdap_rib::ep_info_t src;
	rina::cdap_rib::ep_info_t dest;
	int bytes_read = 0;

	keep_running = true;

	while(keep_running) {

		//Allocate the flow
		allocateFlow();

		if(flow_.portId < 0) {
			usleep(FM_FALLOC_ALLOC_RETRY_US);
			continue;
		}

		//Recheck after flow alloc (prevents race)
		if(!keep_running){
			break;
		}

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

		auth_ps_ = rib_factory_->getSecurityManager()->get_auth_policy_set(con.auth_policy_name);
		if (!auth_ps_) {
			LOG_ERR("Could not %s authentication policy set, aborting",
					rib_factory_->getSecurityManager()->get_sec_profile(con.auth_policy_name).authPolicy.name_.c_str());
			return NULL;
		}

		auth = auth_ps_->get_auth_policy(flow_.portId,
				dest,
				rib_factory_->getSecurityManager()->get_sec_profile(con.auth_policy_name));

		//Version
		rina::cdap_rib::vers_info_t vers;
		vers.version_ = 0x1; //TODO: do not hardcode this

		//TODO: remove this. The API should NOT require a RIB
		//instance for calling the remote API
		rib_factory_->getProxy()->remote_open_connection(vers,
				src,
				dest,
				auth,
				flow_.portId);

		//Recover the response
		//TODO: add support for other
		bool authentication_ongoing = true;
		rina::ser_obj_t message;
		message.message_ = new unsigned char[max_sdu_size_in_bytes];

		//I/O loop
		while(keep_running) {
			bytes_read = read(flow_.fd, message.message_,
					max_sdu_size_in_bytes);
			if (bytes_read <= 0) {
				LOG_ERR("read() error or EOF on port id %u [%s]",
						flow_.portId, strerror(errno));
				rina::cdap::getProvider()->destroy_session(flow_.portId);
				rina::cdap::remove_fd_to_port_id_mapping(flow_.portId);
				close(flow_.fd);
				flow_.fd = -1;
				break;
			}
			message.size_ = bytes_read;

			//Instruct CDAP provider to process the RIB operation message
			try{
				rina::cdap::getProvider()->process_message(
						message,
						flow_.portId);
			}catch(rina::WriteSDUException &e){
				LOG_ERR("Cannot read from flow with port id: %u anymore", flow_.portId);
				rina::cdap::getProvider()->destroy_session(flow_.portId);
				rina::cdap::remove_fd_to_port_id_mapping(flow_.portId);
				close(flow_.fd);
				flow_.fd = -1;
				break;
			}catch(rina::FlowNotAllocatedException &e) {
				rina::cdap::getProvider()->destroy_session(flow_.portId);
				rina::cdap::remove_fd_to_port_id_mapping(flow_.portId);
				close(flow_.fd);
				flow_.fd = -1;
				break;
			}catch(rina::cdap::CDAPException &e){
				LOG_ERR("Error processing message: %s", e.what());
			}catch(...){
				LOG_CRIT("Unknown error during operation with port id: %u. This is a bug, please report it",
						flow_.portId);
			}
		}
	}

	return NULL;
}

void FlowManager::process_fwd_cdap_msg_response(rina::FwdCDAPMsgResponseEvent*
                                                fwdevent)
{
	rina::cdap::cdap_m_t *rmsg = new rina::cdap::cdap_m_t;
	bool remove;

	LOG_DBG("Received forwarded CDAP response, result %d",
			fwdevent->result);

	if (fwdevent->sermsg.message_ == 0) {
		LOG_DBG("Received empty delegated CDAP response");
		return;
	}

	rina::cdap::getProvider()->get_session_manager()->decodeCDAPMessage
	                (fwdevent->sermsg, *rmsg);

	if(rmsg->flags_ == rina::cdap_rib::flags::F_RD_INCOMPLETE)
	        remove = false;
	else
	        remove = true;
	delegated_stored_t* del_sto = rinad::IPCManager->
	                get_forwarded_object(rmsg->invoke_id_, remove);
	if (!del_sto)
	{
	        LOG_ERR("Delegated object not found");
	}
	else
	{
            LOG_DBG("Delegated CDAP response:\n%s, value %p",
                    rmsg->to_string().c_str(),
                    rmsg->obj_value_.message_);

            if (rmsg->obj_class_ == "Root") {
        	    rmsg->obj_class_ = "IPCProcess";
            }
            LOG_DBG("Recovered delegated object: %s", del_sto->obj->fqn.c_str());
            del_sto->obj->forwarded_object_response(del_sto->port,
            		del_sto->invoke_id, rmsg);
            if (remove)
            	delete del_sto;
	}
}

//Process an event coming from librina
void FlowManager::process_librina_event(rina::IPCEvent** event_)
{
	rina::FlowInformation flow;
	unsigned int port_id;

	//Make it easy
	rina::IPCEvent* event = *event_;

	if (!event)
		return;

	switch (event->eventType) {
		case rina::IPC_PROCESS_FWD_CDAP_RESPONSE_MSG:
		{
			process_fwd_cdap_msg_response(
				dynamic_cast<rina::FwdCDAPMsgResponseEvent*>(event));
			break;
		}

		default:
			//Ignore, I'm not interested in this event
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

		//Keep next
		next = it;
		next++;

		//Tell worker to stop
		it->second->stop();

		//Join
		joinWorker(it->first);

		//Remove
		it = next;
	}
}

//Connect manager
unsigned int FlowManager::connectTo(const AppConnection& con, bool keyManager)
{
	Worker* w = new ActiveWorker(this,
				     agent_->get_ribf(),
				     con,
				     keyManager);

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
