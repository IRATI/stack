/*
 * IPC Manager - Flow allocation/deallocation
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
 *    Marc Sune             <marc.sune (at) bisdn.de>
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
#include <utility>

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <debug.h>

#define RINA_PREFIX "ipcm.flow-alloc"
#include <librina/logs.h>

#include "rina-configuration.h"
#include "ipcp-handlers.h"
#include "flow-alloc-handlers.h"

using namespace std;

#define IPCM_FA_TIMEOUT_S 1

namespace rinad {

ipcm_res_t
IPCManager_::deallocate_flow(Promise * promise, const int ipcp_id,
			    const rina::FlowDeallocateRequestEvent& event)
{
	ostringstream ss;
	IPCMIPCProcess* ipcp;
	FlowDeallocTransState* trans;
	ipcm_res_t ret = IPCM_PENDING;

	try {
		ipcp = lookup_ipcp_by_id(ipcp_id);

		if(!ipcp){
			ss << "Invalid IPCP id "<< ipcp_id;
			FLUSH_LOG(ERR, ss);
			throw rina::IpcmDeallocateFlowException();
		}

		//Auto release the read lock
		rina::ReadScopedLock readlock(ipcp->rwlock, false);

		//Create a transaction
		trans = new FlowDeallocTransState(NULL, promise,
							ipcp->get_id(), event);
		if (event.sequenceNumber == 0) {
			trans->req_by_ipcm = true;
		}
		if(!trans){
			ss << "Unable to allocate memory for the transaction object. Out of memory! ";
			FLUSH_LOG(ERR, ss);
			throw rina::IpcmDeallocateFlowException();
		}

		//Store transaction
		if(add_transaction_state(trans) < 0){
			ss << "Unable to add transaction; out of memory? ";
			FLUSH_LOG(ERR, ss);
			throw rina::IpcmDeallocateFlowException();
		}

		// Ask the IPC process to deallocate the flow
		// specified by the port-id
		ipcp->deallocateFlow(event.portId, trans->tid);

		ss << "Application " << event.applicationName.toString() <<
			" asks IPC process " << ipcp->get_name().toString() <<
			" to deallocate flow [port-id = " << event.portId <<
			"]" << endl;
		FLUSH_LOG(INFO, ss);
	} catch (rina::ConcurrentException& e) {
		ss  << ": Error while application " <<
			event.applicationName.toString() << "asks IPC process "
			<< ipcp_id << " to deallocate the flow "
			"with port-id " << event.portId <<
			". Operation timedout" << endl;
		FLUSH_LOG(ERR, ss);
		ret = IPCM_FAILURE;
	} catch (rina::IpcmDeallocateFlowException& e) {
		ss  << ": Error while application " <<
			event.applicationName.toString() << "asks IPC process "
			<< ipcp_id << " to deallocate the flow "
			"with port-id " << event.portId << endl;
		FLUSH_LOG(ERR, ss);
		ret = IPCM_FAILURE;
	}

	return ret;
}

void IPCManager_::application_flow_allocation_failed_notify(rina::FlowRequestEvent *event)
{
	ostringstream ss;

	// Inform the Application Manager about the flow allocation
	// failure
	event->portId = -1;
	try {
		rina::applicationManager->flowAllocated(*event);
		ss << "Flow allocation between " <<
			event->localApplicationName.toString()
			<< " and " << event->remoteApplicationName.toString()
			<< " failed: applications informed" << endl;
		FLUSH_LOG(INFO, ss);
	} catch (rina::NotifyFlowAllocatedException& e) {
		ss  << ": Error while notifying the "
			"Application Manager about flow allocation "
			"result" << endl;
		FLUSH_LOG(ERR, ss);
	}
}

void IPCManager_::flow_allocation_requested_local(rina::FlowRequestEvent *event)
{
	rina::ApplicationProcessNamingInformation dif_name;
	IPCMIPCProcess *ipcp;
	bool dif_specified;
	FlowAllocTransState* trans;
	ostringstream ss;

	// Find the name of the DIF that will provide the flow
	dif_specified = lookup_dif_by_application(event->localApplicationName,
								dif_name);
	if (!dif_specified && event->DIFName !=
				rina::ApplicationProcessNamingInformation()) {
		dif_name = event->DIFName;
		dif_specified = true;
	}

	// Select an IPC process to serve the flow request
	if (!dif_specified)
		ipcp = select_ipcp();
	else
		ipcp = select_ipcp_by_dif(dif_name);

	if (!ipcp) {
		ss  << " Error: Cannot find an IPC process to "
			"serve flow allocation request (local-app = " <<
			event->localApplicationName.toString() << ", " <<
			"remote-app = " << event->remoteApplicationName.
			toString() << endl;
		FLUSH_LOG(ERR, ss);

		application_flow_allocation_failed_notify(event);

		return;
	}

	//Auto release the read lock
	rina::ReadScopedLock readlock(ipcp->rwlock, false);

	try {
		// Ask the IPC process to allocate a flow
		trans = new FlowAllocTransState(NULL, NULL, ipcp->get_id(),
								*event,
								dif_specified);
		if(!trans){
			ss << "Unable to allocate memory for the transaction object. Out of memory! ";
			throw rina::AllocateFlowException();
		}

		//Store transaction
		if(add_transaction_state(trans) < 0){
			ss << "Unable to add transaction; out of memory? ";
			throw rina::AllocateFlowException();
		}

		ipcp->allocateFlow(*event, trans->tid);

		ss << "IPC process " << ipcp->get_name().toString() <<
			" requested to allocate flow between " <<
			event->localApplicationName.toString()
			<< " and " << event->remoteApplicationName.toString()
			<< endl;
		FLUSH_LOG(INFO, ss);
	} catch (rina::AllocateFlowException& e) {
		ss  << ": Error while requesting IPC process "
			<< ipcp->get_name().toString() << " to allocate a flow "
			"between " << event->localApplicationName.toString()
			<< " and " << event->remoteApplicationName.toString()
			<< endl;
		FLUSH_LOG(ERR, ss);

		application_flow_allocation_failed_notify(event);
	}
}

void
IPCManager_::flow_allocation_requested_remote(rina::FlowRequestEvent *event)
{
	IPCMIPCProcess *ipcp;
	ostringstream ss;
	FlowAllocTransState* trans = NULL;

	// Retrieve the local IPC process involved in the flow allocation
	// request coming from a remote application
	ipcp = lookup_ipcp_by_id(event->ipcProcessId);
	if (!ipcp) {
		ss  << ": Error: Could not retrieve IPC process "
			"with id " << event->ipcProcessId << ", to serve "
			"remote flow allocation request" << endl;
		FLUSH_LOG(ERR, ss);
		return;
	}

	//Auto release the read lock
	rina::ReadScopedLock readlock(ipcp->rwlock, false);

	try {
		// Inform the local application that a remote application
		// wants to allocate a flow
		trans = new FlowAllocTransState(NULL, NULL, ipcp->get_id(),
									*event,
									true);
		if(!trans){
			ss  << ": Error: Could not allocate memory to serve "
			"remote flow allocation request for ipcp:"<< event->ipcProcessId << endl;
			throw rina::AppFlowArrivedException();
		}

		//Store transaction
		if(add_transaction_state(trans) < 0){
			ss  << ": Error: Could not add tranasaction to serve "
			"remote flow allocation request for ipcp:"<< event->ipcProcessId << endl;
			throw rina::AppFlowArrivedException();
		}

		rina::applicationManager->flowRequestArrived(
					event->localApplicationName,
					event->remoteApplicationName,
					event->flowSpecification,
					event->DIFName, event->portId,
					trans->tid);

		ss << "Arrived request for flow allocation between " <<
			event->localApplicationName.toString()
			<< " and " << event->remoteApplicationName.toString()
			<< endl;
		FLUSH_LOG(INFO, ss);
	} catch (rina::AppFlowArrivedException& e) {
		ss  << ": Error while informing application "
			<< event->localApplicationName.toString() <<
			" about a flow request coming from remote application "
			<< event->remoteApplicationName.toString() << endl;
		FLUSH_LOG(ERR, ss);

		//Remove transaction
		if(trans)
			remove_transaction_state(trans->tid);

		try {
			// Inform the IPC process that it was not possible
			// to allocate the flow
			ipcp->allocateFlowResponse(*event, -1, true, 0);

			ss << "IPC process " << ipcp->get_name().toString() <<
				" informed that it was not possible to "
				"allocate flow between" <<
				event->localApplicationName.toString()
				<< " and " << event->remoteApplicationName.
				toString() << endl;
			FLUSH_LOG(INFO, ss);
		} catch (rina::AllocateFlowException) {
			ss  << ": Error while informing IPC "
				<< "process " << ipcp->get_name().toString() <<
				" about failed flow allocation" << endl;
			FLUSH_LOG(ERR, ss);
		}
	}
}

void IPCManager_::flow_allocation_requested_event_handler(rina::FlowRequestEvent* event)
{
	if (event->localRequest)
		flow_allocation_requested_local(event);
	else
		flow_allocation_requested_remote(event);
}

void IPCManager_::ipcm_allocate_flow_request_result_handler( rina::IpcmAllocateFlowRequestResultEvent *event)
{
	bool success = (event->result == 0);
	ostringstream ss;
	IPCMIPCProcess *slave_ipcp;
	ipcm_res_t ret;
	FlowAllocTransState* trans =
		get_transaction_state<FlowAllocTransState>(event->sequenceNumber);

	if (!trans){
		ss  << ": Warning: Flow allocation request result "
			"received but no corresponding request" << endl;
		FLUSH_LOG(WARN, ss);
		return;
	}
	rina::FlowRequestEvent& req_event = trans->req_event;

	try {
		slave_ipcp = lookup_ipcp_by_id(trans->slave_ipcp_id);
		if(!slave_ipcp){
			ss << "Could not complete Flow allocation request result. Invalid IPCP id "<< slave_ipcp->get_id();
			FLUSH_LOG(ERR, ss);
			throw rina::AllocateFlowException();
		}

		//Auto release the read lock
		rina::ReadScopedLock readlock(slave_ipcp->rwlock, false);

		req_event.portId = -1;

		// Inform the IPC process about the result of the
		// flow allocation operation
		slave_ipcp->allocateFlowResult(event->sequenceNumber,
					       success, event->portId);
		if (success)
			req_event.portId = event->portId;
		// TODO retry with other DIFs?

		ss << "Informing IPC process " <<
			slave_ipcp->get_name().toString()
			<< " about flow allocation from application " <<
			req_event.localApplicationName.toString() <<
			" to application " <<
			req_event.remoteApplicationName.toString() <<
			" in DIF " << slave_ipcp->dif_name_.toString() <<
			" [success = " << success
			<< ", port-id = " << event->portId << "]" << endl;
		FLUSH_LOG(INFO, ss);

		ret = (success)? IPCM_SUCCESS : IPCM_FAILURE;
	} catch (rina::AllocateFlowException& e) {
		ss  << ": Error while informing the IPC process "
			<< trans->slave_ipcp_id << " about result of "
			"flow allocation between applications " <<
			req_event.localApplicationName.toString() <<
			" and " << req_event.remoteApplicationName.toString()
			<< endl;
		FLUSH_LOG(ERR, ss);
		ret = IPCM_FAILURE;
	}

	// Inform the Application Manager about the flow allocation
	// result
	try {
		rina::applicationManager->flowAllocated(req_event);
		ss << "Applications " <<
			req_event.localApplicationName.toString() << " and "
			<< req_event.remoteApplicationName.toString()
			<< " informed about flow allocation result" << endl;
		FLUSH_LOG(INFO, ss);
	} catch (rina::NotifyFlowAllocatedException& e) {
		ss  << ": Error while notifying the "
			"Application Manager about flow allocation result"
			<< endl;
		FLUSH_LOG(ERR, ss);
		//ret = IPCM_FAILURE; <= should this be marked as error?
	}

	trans->completed(ret);
	remove_transaction_state(trans->tid);
}

void IPCManager_::allocate_flow_response_event_handler(rina::AllocateFlowResponseEvent *event)
{
	bool success = (event->result == 0);
	IPCMIPCProcess *slave_ipcp;
	ostringstream ss;
	ipcm_res_t ret;
	FlowAllocTransState* trans =
		get_transaction_state<FlowAllocTransState>(event->sequenceNumber);


	if (!trans){
		ss  << ": Warning: Flow allocation received received but no corresponding request" << endl;
		FLUSH_LOG(WARN, ss);
		return;
	}

	rina::FlowRequestEvent& req_event = trans->req_event;

	try {
		slave_ipcp = lookup_ipcp_by_id(trans->slave_ipcp_id);
		if(!slave_ipcp){
			ss << "Could not complete Flow allocation request result. Invalid IPCP id "<< slave_ipcp->get_id();
			FLUSH_LOG(ERR, ss);
			throw rina::AllocateFlowException();
		}

		//Auto release the read lock
		rina::ReadScopedLock readlock(slave_ipcp->rwlock, false);

		// Inform the IPC process about the response of the flow
		// allocation procedure
		slave_ipcp->allocateFlowResponse(
			req_event,
			event->result,
			event->notifySource,
			event->flowAcceptorIpcProcessId);
		if (!success)
			req_event.portId = -1;

		ss  << ": Informing IPC process " <<
			slave_ipcp->get_name().toString()
			<< " about flow allocation from application " <<
			req_event.localApplicationName.toString() <<
			" to application " <<
			req_event.remoteApplicationName.toString() <<
			" in DIF " << slave_ipcp->dif_name_.toString() <<
			" [success = " << success
			<< ", port-id = " << req_event.portId << "]" << endl;
		FLUSH_LOG(INFO, ss);

		ret = (success)? IPCM_SUCCESS : IPCM_FAILURE;
	} catch (rina::AllocateFlowException& e) {
		ss  << ": Error while informing IPC "
			<< "process " << trans->slave_ipcp_id <<
			" about failed flow allocation" << endl;
		FLUSH_LOG(ERR, ss);
		req_event.portId = -1;
		ret = IPCM_FAILURE;
	}
	trans->completed(ret);
	remove_transaction_state(trans->tid);
}

void IPCManager_::flow_deallocation_requested_event_handler(rina::FlowDeallocateRequestEvent* event)
{
	IPCMIPCProcess *ipcp = lookup_ipcp_by_port(event->portId);
	unsigned short ipcp_id = 0;
	ostringstream ss;

	if (!ipcp) {
		ss  << ": Cannot find the IPC process that "
			"provides the flow with port-id " << event->portId
			<< endl;
		FLUSH_LOG(ERR, ss);
		return;
	}

	{
		//Auto release the read lock
		rina::ReadScopedLock readlock(ipcp->rwlock, false);
		ipcp_id = ipcp->get_id();
	}

	deallocate_flow(NULL, ipcp_id, *event);
}

void IPCManager_::ipcm_deallocate_flow_response_event_handler(rina::IpcmDeallocateFlowResponseEvent* event)
{
	bool success = (event->result == 0);
	int result = event->result;
	ostringstream ss;
	ipcm_res_t ret;
	IPCMIPCProcess* ipcp;
	FlowDeallocTransState* trans =
		get_transaction_state<FlowDeallocTransState>(event->sequenceNumber);

	if (!trans){
		ss  << ": Warning: Flow deallocation response received but no corresponding request" << endl;
		FLUSH_LOG(WARN, ss);
		return;
	}

	rina::FlowDeallocateRequestEvent& req_event = trans->req_event;

	try {
		ipcp = lookup_ipcp_by_id(trans->slave_ipcp_id);
		if(!ipcp){
			ss << "Could not complete Flow allocation request result. Invalid IPCP";
			FLUSH_LOG(ERR, ss);
			throw rina::IpcmDeallocateFlowException();
		}

		//Auto release the read lock
		rina::ReadScopedLock readlock(ipcp->rwlock, false);

		// Inform the IPC process about the deallocation result
		ipcp->deallocateFlowResult(event->sequenceNumber, success);

		ss << "Deallocated flow "
			<< "identified by port-id " <<
			req_event.portId << " from IPC process " <<
			ipcp->get_name().toString() << " [success = " <<
			success << "]" << endl;
		FLUSH_LOG(INFO, ss);

		ret = (success)? IPCM_SUCCESS : IPCM_FAILURE;
	} catch (rina::IpcmDeallocateFlowException& e) {
		ss  << ": Error while informing IPC process "
			<< " about deallocation "
			"of flow with port-id " << req_event.portId <<
			"because " << e.what() << endl;
		FLUSH_LOG(ERR, ss);
		ret = IPCM_FAILURE;
	}

	if (!trans->req_by_ipcm) {
		try {
			// Inform the application about the deallocation
			// result
			if (req_event.sequenceNumber > 0) {
				rina::applicationManager->
				flowDeallocated(req_event, result);

				ss << "Application " << req_event.applicationName.
						toString() << " informed about deallocation "
						"of flow identified by port-id " <<
						req_event.portId << "[success = " <<
						(!result) << "]" << endl;
				FLUSH_LOG(INFO, ss);
			}
		} catch (rina::NotifyFlowDeallocatedException& e) {
			ss  << ": Error while informing "
					"application " << req_event.applicationName.
					toString() << " about deallocation of flow "
					"identified by port-id " << req_event.portId
					<< endl;
			FLUSH_LOG(ERR, ss);
		}
	}

	trans->completed(ret);
	remove_transaction_state(trans->tid);
}

void IPCManager_::flow_deallocated_event_handler(rina::FlowDeallocatedEvent* event)
{
	IPCMIPCProcess *ipcp = lookup_ipcp_by_port(event->portId);
	rina::FlowInformation info;
	ostringstream ss;

	if (!ipcp) {
		ss  << ": Cannot find the IPC process that "
			"provides the flow with port-id " << event->portId
			<< endl;
		FLUSH_LOG(ERR, ss);
		return;
	}

	//Auto release the write lock
	rina::ReadScopedLock readlock(ipcp->rwlock, false);

	try {
		// Inform the IPC process that the flow corresponding to
		// the specified port-id has been deallocated
		info = ipcp->flowDeallocated(event->portId);
		rina::applicationManager->
			flowDeallocatedRemotely(event->portId, event->code,
						info.localAppName);

		ss << "IPC process " << ipcp->get_name().toString() <<
			" and local application " << info.localAppName.
			toString() << " informed that flow with port-id "
			<< event->portId << " has been deallocated remotely "
			<< endl;
		FLUSH_LOG(INFO, ss);
	} catch (rina::IpcmDeallocateFlowException& e) {
		ss  << ": Cannot find a flow with port-id "
			<< event->portId << endl;
		FLUSH_LOG(ERR, ss);
	} catch (rina::NotifyFlowDeallocatedException& e) {
		ss  << ": Error while informing application "
			<< info.localAppName.toString() <<
			" that flow with port-id " << event->portId <<
			" has been remotely deallocated" << endl;
		FLUSH_LOG(ERR, ss);
	}
}

} //namespace rinad
