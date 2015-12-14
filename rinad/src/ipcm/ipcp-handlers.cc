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

using namespace std;


namespace rinad {

#define IPCP_DAEMON_INIT_RETRIES 5

void IPCManager_::ipc_process_daemon_initialized_event_handler(
				rina::IPCProcessDaemonInitializedEvent * e)
{
	ostringstream ss;
	IPCMIPCProcess* ipcp;
	int i;
	SyscallTransState* trans = NULL;

	//There can be race condition between the caller and us (notification)
	for(i=0; i<IPCP_DAEMON_INIT_RETRIES; ++i){
		trans = get_syscall_transaction_state(e->ipcProcessId);
		if(trans)
			break;
	}

	if(!trans){
		ss << ": Warning: IPCP daemon '"<<e->ipcProcessId<<"'initialized, but no pending normal IPC process initialization. Corrupted state?"<< endl;
		assert(0);
		return;
	}

	//Recover IPCP process with writelock
	ipcp = lookup_ipcp_by_id(e->ipcProcessId, true);

	//If the ipcp is not there, there is some corruption
	if(!ipcp){
		ss << ": Warning: IPCP daemon '"<<e->ipcProcessId<<"'initialized, but no pending normal IPC process initialization. Corrupted state?"<< endl;
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

		assert(ipcp->get_type() == rina::NORMAL_IPC_PROCESS);

		//Initialize
		ipcp->setInitialized();
	}

	ss << "IPC process daemon initialized [id = " <<
		e->ipcProcessId<< "]" << endl;
	FLUSH_LOG(INFO, ss);

	//Distribute the event to the addons
	IPCMEvent addon_e(trans->callee, IPCM_IPCP_CREATED,
					e->ipcProcessId);
	Addon::distribute_ipcm_event(addon_e);

	//Set return value, mark as completed and signal
	trans->completed(IPCM_SUCCESS);
	remove_syscall_transaction_state(trans->tid);

	return;
}

void IPCManager_::ipcm_register_response_ipcp(IPCMIPCProcess * ipcp,
	rina::IpcmRegisterApplicationResponseEvent *e)
{
	ostringstream ss;
	ipcm_res_t ret = IPCM_FAILURE;

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
}

void IPCManager_::ipcm_unregister_response_ipcp(IPCMIPCProcess * ipcp,
			rina::IpcmUnregisterApplicationResponseEvent *e,
			TransactionState *t)
{
	ostringstream ss;
	bool success;
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

	IPCPTransState* trans = get_transaction_state<IPCPTransState>(event->sequenceNumber);

	if(!trans){
		ss << ": Warning: unknown enrollment to DIF response received: "<<event->sequenceNumber<<endl;
		FLUSH_LOG(WARN, ss);
		return;
	}

	ipcp = lookup_ipcp_by_id(trans->ipcp_id);
	if(!ipcp){
		ss << ": Warning: Could not complete enroll to dif action: "<<event->sequenceNumber<<
		"IPCP with id: "<<trans->ipcp_id<<" does not exist! Perhaps deleted?" << endl;
		FLUSH_LOG(WARN, ss);
	}else{
		//Auto release the read lock
		rina::ReadScopedLock readlock(ipcp->rwlock, false);

		if (success) {
			ss << "Enrollment operation completed for IPC "
				<< "process " << ipcp->get_name().toString() << endl;
			FLUSH_LOG(INFO, ss);

			ret = IPCM_SUCCESS;
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

} //namespace rinad
