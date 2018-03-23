/*
 * IPC Manager - IPCP related routine handlers
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
 *    Marc Sune             <marc.sune (at) bisdn.de>
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

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
#include <utility>

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <debug.h>

#define RINA_PREFIX "ipcm.ipcp"
#include <librina/logs.h>

#include "rina-configuration.h"
#include "ipcp-handlers.h"
#include "app-handlers.h"

using namespace std;


namespace rinad {

#define IPCP_DAEMON_INIT_RETRIES 500

void IPCManager_::ipc_process_daemon_initialized_event_handler(rina::IPCProcessDaemonInitializedEvent * e)
{
	ostringstream ss;
	IPCMIPCProcess* ipcp;
	int i;
	SyscallTransState* trans = NULL;
	bool trans_completed = false;
	rina::Sleep sleep;

	//There can be race condition between the caller and us (notification)
	for(i=0; i<IPCP_DAEMON_INIT_RETRIES; ++i){
		trans = get_syscall_transaction_state(e->ipcProcessId);
		if(trans)
			break;
		sleep.sleepForMili(1);
	}

	if(!trans){
		ss << ": Warning: IPCP daemon '"<<e->ipcProcessId
		    <<"'initialized, but no pending normal IPC process "
		      "initialization. Corrupted state?"<< endl;
		assert(0);
		return;
	}

	//Recover IPCP process with writelock
	ipcp = lookup_ipcp_by_id(e->ipcProcessId, true);

	//If the ipcp is not there, there is some corruption
	if(!ipcp){
		ss << ": Warning: IPCP daemon '"<< e->ipcProcessId
		   <<"'initialized, but no pending normal IPC process "
		     "initialization. Corrupted state?"<< endl;
		FLUSH_LOG(WARN, ss);
		assert(0);

		//Set promise return
		trans->completed(IPCM_FAILURE);

		//Remove syscall transaction and return
		remove_syscall_transaction_state(e->ipcProcessId);
		return;
	}

	{
		//Auto release the read lock
		rina::WriteScopedLock writelock(ipcp->rwlock, false);

		assert(ipcp->get_type() == rina::NORMAL_IPC_PROCESS ||
				ipcp->get_type() == rina::SHIM_WIFI_IPC_PROCESS_AP ||
				ipcp->get_type() == rina::SHIM_WIFI_IPC_PROCESS_STA);

		//Initialize
		ipcp->setInitialized();

		if (ipcp->kernel_ready) {
			trans_completed = true;
		}
	}

	ss << "IPC process daemon initialized [id = " <<
		e->ipcProcessId<< "]" << endl;
	FLUSH_LOG(INFO, ss);

	if (!trans_completed) {
		return;
	}

	//Distribute the event to the addons
	IPCMEvent addon_e(trans->callee, IPCM_IPCP_CREATED,
					e->ipcProcessId);
	Addon::distribute_ipcm_event(addon_e);

	//Set return value, mark as completed and signal
	trans->completed(IPCM_SUCCESS);
	remove_syscall_transaction_state(trans->tid);

	return;
}

void IPCManager_::ipc_process_create_response_event_handler(rina::CreateIPCPResponseEvent * e)
{
	ostringstream ss;
	IPCMIPCProcess* ipcp;
	int i;
	SyscallTransState* trans = NULL;
	bool trans_completed = false;
	unsigned short ipcp_id = 0;
	std::map<unsigned int, unsigned short>::iterator it;

	rina::ScopedLock g(req_lock);

	it = pending_cipcp_req.find(e->sequenceNumber);
	if (it == pending_cipcp_req.end()) {
		ss << "Could not find pending Create IPCP request "
		   << "with seq num " << e->sequenceNumber;
		FLUSH_LOG(ERR, ss);
		return;
	}

	ipcp_id = it->second;
	pending_cipcp_req.erase(it);

	//There can be race condition between the caller and us (notification)
	for(i=0; i<IPCP_DAEMON_INIT_RETRIES; ++i){
		trans = get_syscall_transaction_state(ipcp_id);
		if(trans)
			break;
	}

	if(!trans){
		ss << ": Warning: IPCP kernel components of '"<< ipcp_id
		    <<"'created, but no pending IPC process "
		      "create transaction. Corrupted state?"<< endl;
		assert(0);
		return;
	}

	//Recover IPCP process with writelock
	ipcp = lookup_ipcp_by_id(ipcp_id, true);

	//If the ipcp is not there, there is some corruption
	if(!ipcp){
		ss << ": Warning: IPCP kernel components of '"<< ipcp_id
		   <<"'created, but no IPCMIPCProcess "
		     "state. Corrupted state?"<< endl;
		FLUSH_LOG(WARN, ss);
		assert(0);

		//Set promise return
		trans->completed(IPCM_FAILURE);

		//Remove syscall transaction and return
		remove_syscall_transaction_state(ipcp_id);
		return;
	}

	{
		//Auto release the read lock
		rina::WriteScopedLock writelock(ipcp->rwlock, false);

		//Initialize
		ipcp->kernel_ready = true;

		if (ipcp->get_state() == IPCMIPCProcess::IPCM_IPCP_INITIALIZED) {
			trans_completed = true;
		}
	}

	ss << "IPC process kernel components of [id = " <<
		ipcp_id << "] created" << endl;
	FLUSH_LOG(INFO, ss);

	if (!trans_completed) {
		return;
	}

	//Distribute the event to the addons
	IPCMEvent addon_e(trans->callee, IPCM_IPCP_CREATED, ipcp_id);
	Addon::distribute_ipcm_event(addon_e);

	//Set return value, mark as completed and signal
	trans->completed(IPCM_SUCCESS);
	remove_syscall_transaction_state(trans->tid);

	return;
}

void IPCManager_::ipc_process_destroy_response_event_handler(rina::DestroyIPCPResponseEvent *e)
{
	std::map<unsigned int, unsigned short>::iterator it;
	unsigned short ipcp_id = 0;

	rina::ScopedLock g(req_lock);

	it = pending_dipcp_req.find(e->sequenceNumber);
	if (it == pending_dipcp_req.end()) {
		LOG_ERR("Got unexpected destroy IPCP response with seq_num %d",
			 e->sequenceNumber);
		return;
	}

	ipcp_id = it->second;
	pending_dipcp_req.erase(it);

	if (e->result != 0) {
		LOG_ERR("Problems destroying kernel components of IPCP %d",
			ipcp_id);
	}
}

void IPCManager_::ipcm_register_response_ipcp(IPCMIPCProcess * ipcp,
	rina::IpcmRegisterApplicationResponseEvent *e)
{
	ostringstream ss;
	ipcm_res_t ret = IPCM_FAILURE;
	rina::ApplicationProcessNamingInformation daf_name;

	IPCPregTransState* trans = get_transaction_state<IPCPregTransState>(e->sequenceNumber);

	if(!trans){
		LOG_ERR("Transaction not of valid type");
		throw rina::Exception();
	}

	rinad::IPCMIPCProcess *slave_ipcp = lookup_ipcp_by_id(trans->slave_ipcp_id);
	if(!slave_ipcp) {
		LOG_ERR("Could not find slave IPCP");
		throw rina::Exception();
	}

	//Auto release the read lock
	rina::ReadScopedLock sreadlock(slave_ipcp->rwlock, false);

	const rina::ApplicationProcessNamingInformation& slave_dif_name =
						slave_ipcp->dif_name_;

	// Notify the registered IPC process.
	if( ipcm_register_response_common(e, ipcp->get_name(), slave_ipcp,
				slave_dif_name) ){
		try {
			ipcp->notifyRegistrationToSupportingDIF(
					slave_ipcp->get_name(),
					slave_dif_name
					);
			ss << "IPC process " << ipcp->get_name().toString() <<
				" informed about its registration "
				"to N-1 DIF " <<
				slave_dif_name.toString() << endl;
			FLUSH_LOG(INFO, ss);
		} catch (rina::NotifyRegistrationToDIFException& e) {
			ss  << ": Error while notifying "
				"IPC process " <<
				ipcp->get_name().toString() <<
				" about registration to N-1 DIF"
				<< slave_dif_name.toString() << endl;
			FLUSH_LOG(ERR, ss);
			throw e;
		}
	} else {
		ss  << "Cannot register IPC process "
			<< ipcp->get_name().toString() <<
			" to DIF " << slave_dif_name.toString() << endl;
		FLUSH_LOG(ERR, ss);
		throw rina::Exception();
	}

        //Inform DIF allocator
        if (e->result == 0) {
        	dif_allocator->app_registered(ipcp->get_name(),
        			              slave_ipcp->dif_name_.processName);

        	daf_name.processName = ipcp->dif_name_.processName;
        	daf_name.processInstance = ipcp->get_name().processName;

        	dif_allocator->app_registered(daf_name,
        			              slave_ipcp->dif_name_.processName);
        }
}

void IPCManager_::ipcm_unregister_response_ipcp(IPCMIPCProcess * ipcp,
			rina::IpcmUnregisterApplicationResponseEvent *e,
			TransactionState *t)
{
	ostringstream ss;
	bool success;
	rina::ApplicationProcessNamingInformation daf_name;
	IPCPregTransState *trans = dynamic_cast<IPCPregTransState*>(t);
	if(!trans){
		LOG_ERR("Transaction is not of the right type");
		throw rina::Exception();
	}

	rinad::IPCMIPCProcess *slave_ipcp = lookup_ipcp_by_id(trans->slave_ipcp_id);
	if(!slave_ipcp) {
		LOG_ERR("Could not find slave IPCP");
		throw  rina::Exception();
	}

	//Auto release the read lock
	rina::ReadScopedLock sreadlock(slave_ipcp->rwlock, false);

	const rina::ApplicationProcessNamingInformation& slave_dif_name =
							slave_ipcp->dif_name_;

	// Inform the supporting IPC process
	if(ipcm_unregister_response_common(e, slave_ipcp,
						  ipcp->get_name())){
		try {
			// Notify the IPCP process that it has been unregistered
			// from the DIF
			ipcp->notifyUnregistrationFromSupportingDIF(
							slave_ipcp->get_name(),
							slave_dif_name);
			ss << "IPC process " << ipcp->get_name().toString() <<
				" informed about its unregistration from DIF "
				<< slave_dif_name.toString() << endl;
			FLUSH_LOG(INFO, ss);
		} catch (rina::NotifyRegistrationToDIFException& e) {
			ss  << ": Error while reporing "
				"unregistration result for IPC process "
				<< ipcp->get_name().toString() << endl;
			FLUSH_LOG(ERR, ss);
			throw e;
		}
	} else {
		ss  << ": Cannot unregister IPC Process "
			<< ipcp->get_name().toString() << " from DIF " <<
			slave_dif_name.toString() << endl;
		FLUSH_LOG(ERR, ss);
		throw rina::Exception();
	}

        //Inform DIF allocator
        if (e->result == 0) {
        	dif_allocator->app_unregistered(ipcp->get_name(),
        			                slave_ipcp->dif_name_.processName);

        	daf_name.processName = ipcp->dif_name_.processName;
        	daf_name.processInstance = ipcp->get_name().processName;

        	dif_allocator->app_unregistered(daf_name,
        			                slave_ipcp->dif_name_.processName);
        }
}

void
IPCManager_::assign_to_dif_response_event_handler(rina::AssignToDIFResponseEvent* e)
{
	ostringstream ss;
	bool success = (e->result == 0);
	IPCMIPCProcess* ipcp;
	ipcm_res_t ret;

	IPCPTransState* trans = get_transaction_state<IPCPTransState>(e->sequenceNumber);

	if(!trans){
		ss << ": Warning: unknown assign to DIF response received: "<<e->sequenceNumber<<endl;
		FLUSH_LOG(WARN, ss);
		return;
	}

	// Inform the IPC process about the result of the
	// DIF assignment operation
	try {
		ipcp = lookup_ipcp_by_id(trans->ipcp_id);
		if(!ipcp){
			ss << ": Warning: Could not complete assign to dif action: "<<e->sequenceNumber<<
			"IPCP with id: "<<trans->ipcp_id<<"does not exist! Perhaps deleted?" << endl;
			FLUSH_LOG(WARN, ss);
			throw rina::AssignToDIFException();
		}
		//Auto release the read lock
		rina::ReadScopedLock readlock(ipcp->rwlock, false);

		ipcp->assignToDIFResult(success);

		ss << "DIF assignment operation completed for IPC "
			<< "process " << ipcp->get_name().toString() <<
			" [success=" << success << "]" << endl;
		FLUSH_LOG(INFO, ss);
		ret = (success)? IPCM_SUCCESS : IPCM_FAILURE;
	} catch (rina::AssignToDIFException& e) {
		ss << ": Error while reporting DIF "
			"assignment result for IPC process "
			<< ipcp->get_name().toString() << endl;
		FLUSH_LOG(ERR, ss);
		ret = IPCM_FAILURE;
	}
	//Mark as completed
	trans->completed(ret);
	remove_transaction_state(trans->tid);

	// If there are is a dynamic DIF Allocator in this IPCM,
	// register it to the DIF
	if (ipcp->proxy_->type == rina::NORMAL_IPC_PROCESS) {
		dif_allocator->assigned_to_dif(ipcp->dif_name_.processName);
	}
}


void
IPCManager_::update_dif_config_response_event_handler(rina::UpdateDIFConfigurationResponseEvent* e)
{
	ostringstream ss;
	bool success = (e->result == 0);
	IPCMIPCProcess* ipcp;
	ipcm_res_t ret;

	IPCPTransState* trans = get_transaction_state<IPCPTransState>(e->sequenceNumber);

	if(!trans){
		ss << ": Warning: unknown DIF config response received: "<<e->sequenceNumber<<endl;
		FLUSH_LOG(WARN, ss);
		return;
	}

	try {
		ipcp = lookup_ipcp_by_id(trans->ipcp_id);
		if(!ipcp){
			ss << ": Warning: Could not complete config to dif action: "<<e->sequenceNumber<<
			"IPCP with id: "<<trans->ipcp_id<<"does not exist! Perhaps deleted?" << endl;
			FLUSH_LOG(WARN, ss);
			throw rina::UpdateDIFConfigurationException();
		}
		//Auto release the read lock
		rina::ReadScopedLock readlock(ipcp->rwlock, false);


		// Inform the requesting IPC process about the result of
		// the configuration update operation
		ss << "Configuration update operation completed for IPC "
			<< "process " << ipcp->get_name().toString() <<
			" [success=" << success << "]" << endl;
		FLUSH_LOG(INFO, ss);
		ret = IPCM_SUCCESS;
	} catch (rina::UpdateDIFConfigurationException& e) {
		ss  << ": Error while reporting DIF "
			"configuration update for process " <<
			ipcp->get_name().toString() << endl;
		FLUSH_LOG(ERR, ss);
		ret = IPCM_FAILURE;
	}
	//Mark as completed
	trans->completed(ret);
	remove_transaction_state(trans->tid);
}

void
IPCManager_::enroll_to_dif_response_event_handler(rina::EnrollToDIFResponseEvent *event)
{
	ostringstream ss;
	IPCMIPCProcess *ipcp;
	bool success = (event->result == 0);
	ipcm_res_t ret = IPCM_FAILURE;
	std::list<rina::Neighbor>::iterator it;
	std::list<rina::Neighbor> neighbors;

	IPCPTransState* trans = get_transaction_state<IPCPTransState>(event->sequenceNumber);

	if(!trans){
		ss << ": Warning: unknown enrollment to DIF response received: "<<event->sequenceNumber<<endl;
		FLUSH_LOG(WARN, ss);
		return;
	}

	ipcp = lookup_ipcp_by_id(trans->ipcp_id, true);
	if(!ipcp){
		ss << ": Warning: Could not complete enroll to DIF action: "<<event->sequenceNumber<<
		"IPCP with id: "<<trans->ipcp_id<<" does not exist! Perhaps deleted?" << endl;
		FLUSH_LOG(WARN, ss);
	}else{
		//Auto release the write lock
		rina::WriteScopedLock writelock(ipcp->rwlock, false);

		if (success) {
			ss << "Enrollment operation completed for IPC "
				<< "process " << ipcp->get_name().toString() << endl;
			FLUSH_LOG(INFO, ss);

			ret = IPCM_SUCCESS;

			ipcp->add_neighbors(event->neighbors);
			ipcp->dif_name_.processName = event->difInformation.dif_name_.processName;
		} else {
			ss  << ": Error: Enrollment operation of "
				"process " << ipcp->get_name().toString() << " failed"
				<< endl;
			FLUSH_LOG(ERR, ss);
		}
	}

	//Mark as completed
	trans->completed(ret);
	remove_transaction_state(trans->tid);
}

void
IPCManager_::disconnect_neighbor_response_event_handler(rina::DisconnectNeighborResponseEvent *event)
{
	ostringstream ss;
	IPCMIPCProcess *ipcp;
	bool success = (event->result == 0);
	ipcm_res_t ret = IPCM_FAILURE;
	std::list<rina::Neighbor>::iterator it;

	IPCPTransState* trans = get_transaction_state<IPCPTransState>(event->sequenceNumber);

	if(!trans){
		ss << ": Warning: unknown enrollment to DIF response received: "<<event->sequenceNumber<<endl;
		FLUSH_LOG(WARN, ss);
		return;
	}

	ipcp = lookup_ipcp_by_id(trans->ipcp_id, true);
	if(!ipcp){
		ss << ": Warning: Could not complete disconnect neighbor action: "<<event->sequenceNumber<<
		"IPCP with id: "<<trans->ipcp_id<<" does not exist! Perhaps deleted?" << endl;
		FLUSH_LOG(WARN, ss);
	}else{
		//Auto release the read lock
		rina::WriteScopedLock writelock(ipcp->rwlock, false);

		ipcp->disconnectFromNeighborResult(event->sequenceNumber, success);

		if (success) {
			ss << "Disconnect from neighbor operation completed for IPC "
				<< "process " << ipcp->get_name().toString() << endl;
			FLUSH_LOG(INFO, ss);

			ret = IPCM_SUCCESS;
		} else {
			ss  << ": Error: Disconnect from neighbor operation completed for IPC "
				"process " << ipcp->get_name().toString() << " failed"
				<< endl;
			FLUSH_LOG(ERR, ss);
		}
	}

	//Mark as completed
	trans->completed(ret);
	remove_transaction_state(trans->tid);
}

} //namespace rinad
