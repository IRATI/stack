/*
 * IPC Manager - Flow allocation/deallocation
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
 *    Marc Sune             <marc.sune (at) bisdn.de>
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
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

	try {
		ipcp = lookup_ipcp_by_id(ipcp_id);

		if(!ipcp){
			ss << "Invalid IPCP id "<< ipcp_id;
			FLUSH_LOG(ERR, ss);
			throw rina::IpcmDeallocateFlowException();
		}

		//Auto release the read lock
		rina::ReadScopedLock readlock(ipcp->rwlock, false);

		// Ask the IPC process to deallocate the flow
		// specified by the port-id
		ipcp->deallocateFlow(event.portId, 0);

		// Check in case it is an IP VPN Flow
		ip_vpn_manager->ip_vpn_flow_deallocated(event.portId,
							ipcp->get_id());

		ss << "IPC process " << ipcp->get_name().toString() <<
			" requested to deallocate flow [port-id = " << event.portId <<
			"]" << endl;
		FLUSH_LOG(INFO, ss);
	} catch (rina::Exception& e) {
		ss  << ": Error while IPC process "
			<< ipcp_id << " tries to deallocate the flow "
			"with port-id " << event.portId <<
			". Operation timed out" << endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	}

	return IPCM_SUCCESS;
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

JoinDIFAndAllocateFlowTask::JoinDIFAndAllocateFlowTask(IPCManager_ * ipcmgr, Promise * pro,
						       const rina::FlowRequestEvent& event,
						       const std::string& dname,
						       const std::list<std::string>& sdnames)
{
	ipcm = ipcmgr;
	promise = pro;
	fevent = event;
	dif_name = dname;
	sup_dif_names = sdnames;
}

void JoinDIFAndAllocateFlowTask::run()
{
	ipcm->join_dif_continue_flow_alloc(promise, fevent, dif_name, sup_dif_names);
}

void IPCManager_::join_dif_continue_flow_alloc(Promise * promise, rina::FlowRequestEvent& event,
				  	       const std::string& dif_name,
					       const std::list<std::string>& sup_dif_names)
{
	rina::ApplicationProcessNamingInformation sdif_name, dapp_name, ipcp_name;
	std::list<std::string>::const_iterator sitr;
	IPCMIPCProcess * ipcp;
	FlowAllocTransState* trans;
	std::stringstream ss;
	rinad::DIFTemplateMapping template_mapping;
	rinad::DIFTemplate dif_template;
        ipcm_res_t result;
        CreateIPCPPromise c_promise;
        Promise ipcp_promise;
        NeighborData neighbor;
        bool dif_found = false;
        rina::Sleep sleep;

	LOG_DBG("Trying to find supporting DIF and to create an IPCP");
	for (sitr = sup_dif_names.begin(); sitr != sup_dif_names.end(); ++sitr) {
		sdif_name.processName  = *sitr;
		if (is_any_ipcp_assigned_to_dif(sdif_name)) {
			LOG_DBG("Found IPCP belonging to supporting DIF %s", sitr->c_str());
			dif_found = true;
			break;
		}
	}

	if (!dif_found) {
		ss  << " Error: Cannot find an IPC process to "
			"serve flow allocation request (local-app = " <<
			event.localApplicationName.toString() << ", " <<
			"remote-app = " << event.remoteApplicationName.
			toString() << endl;
		FLUSH_LOG(ERR, ss);

		if (!promise)
			application_flow_allocation_failed_notify(&event);
		return;
	}

	// Create IPC Process of dif_name, join DIF name via supporting DIF
	// & register IPCP to supporting DIF
	dapp_name.processName = dif_name;
	if (!config.lookup_dif_template_mappings(dapp_name, template_mapping)) {
		ss << "Could not find DIF template for dif name "
				<< dif_name<< std::endl;
		FLUSH_LOG(ERR, ss);

		if (!promise)
			application_flow_allocation_failed_notify(&event);
		return;
	}

        if (dif_template_manager->get_dif_template(template_mapping.template_name,
	    	    	        		   dif_template) != 0) {
        	ss << "Cannot find template called "
        			<< template_mapping.template_name;
        	FLUSH_LOG(ERR, ss);

        	if (!promise)
        		application_flow_allocation_failed_notify(&event);
        	return;
        }

        neighbor.difName.processName = dif_name;
        neighbor.supportingDifName = sdif_name;
        neighbor.apName.processName = dif_name;
        dif_allocator->get_ipcp_name_for_dif(ipcp_name, dif_name);
        try {
        	if (create_ipcp(NULL, &c_promise, ipcp_name,
        			dif_template.difType, dapp_name.processName) == IPCM_FAILURE
        			|| c_promise.wait() != IPCM_SUCCESS) {
        		LOG_ERR("Problems creating IPCP");

        		if (!promise)
        			application_flow_allocation_failed_notify(&event);
        		return;
        	}

        	if (assign_to_dif(NULL, &ipcp_promise, c_promise.ipcp_id,
        			dif_template, dapp_name) == IPCM_FAILURE
        			|| ipcp_promise.wait() != IPCM_SUCCESS) {
        		ss << "Problems assigning IPCP " << c_promise.ipcp_id
        				<< " to DIF " << dif_name << std::endl;
        		FLUSH_LOG(ERR, ss);

        		if (!promise)
        			application_flow_allocation_failed_notify(&event);
        		return;
        	}

        	if (register_at_dif(NULL, &ipcp_promise, c_promise.ipcp_id, sdif_name)
        			== IPCM_FAILURE || ipcp_promise.wait() != IPCM_SUCCESS) {
        		ss << "Problems registering IPCP " << c_promise.ipcp_id
        				<< " to DIF " << sdif_name.processName << std::endl;
        		FLUSH_LOG(ERR, ss);

        		if (!promise)
        			application_flow_allocation_failed_notify(&event);
        		return;
        	}

                if (enroll_to_dif(NULL, &ipcp_promise, c_promise.ipcp_id, neighbor)
                         == IPCM_FAILURE || ipcp_promise.wait() != IPCM_SUCCESS) {
                     ss << ": Unknown error while enrolling IPCP " << c_promise.ipcp_id
                             << " to neighbour " << neighbor.apName.getEncodedString()
			     << std::endl;
                     FLUSH_LOG(ERR, ss);

                     if (!promise)
                	     application_flow_allocation_failed_notify(&event);
                     return;
                 }
        } catch (rina::Exception &e) {
        	LOG_ERR("Exception while creating IPCP and joining DIF: %s", e.what());

		if (!promise)
			application_flow_allocation_failed_notify(&event);
		return;
        }

        LOG_DBG("New IPCP created and joined DIF, allocating flow");
        ipcp = select_ipcp_by_dif(dapp_name);
	if (!ipcp) {
		ss  << " Error: Cannot find an IPC process to "
			"serve flow allocation request (local-app = " <<
			event.localApplicationName.toString() << ", " <<
			"remote-app = " << event.remoteApplicationName.
			toString() << endl;
		FLUSH_LOG(ERR, ss);

		if (!promise)
			application_flow_allocation_failed_notify(&event);

		return;
	}

	//Wait for routing to converge before requesting flow allocation
	// (the IPCP has just joined the DIF)
	sleep.sleepForMili(1000);

	//Auto release the read lock
	rina::ReadScopedLock readlock(ipcp->rwlock, false);
	try {
		// Ask the IPC process to allocate a flow
		trans = new FlowAllocTransState(NULL, promise, ipcp->get_id(),
						event, true);
		if(!trans){
			ss << "Unable to allocate memory for the transaction object. Out of memory! ";
			if (!promise)
				application_flow_allocation_failed_notify(&event);

			return;
		}

		//Store transaction
		if(add_transaction_state(trans) < 0){
			ss << "Unable to add transaction; out of memory? ";
			if (!promise)
				application_flow_allocation_failed_notify(&event);

			return;
		}

		ipcp->allocateFlow(event, trans->tid);
		ss << "IPC process " << ipcp->get_name().toString() <<
				" requested to allocate flow between " <<
				event.localApplicationName.toString()
				<< " and " << event.remoteApplicationName.toString()
				<< endl;
		FLUSH_LOG(INFO, ss);
	} catch (rina::AllocateFlowException& e) {
		ss  << ": Error while requesting IPC process "
				<< ipcp->get_name().toString() << " to allocate a flow "
				"between " << event.localApplicationName.toString()
				<< " and " << event.remoteApplicationName.toString()
				<< endl;
		FLUSH_LOG(ERR, ss);

		if (!promise)
			application_flow_allocation_failed_notify(&event);

		return;
	}
}

ipcm_res_t IPCManager_::flow_allocation_requested_local(Promise * promise,
						        rina::FlowRequestEvent *event)
{
	rina::ApplicationProcessNamingInformation dif_name;
	IPCMIPCProcess * ipcp;
	da_res_t dif_specified;
	FlowAllocTransState* trans;
	ostringstream ss;
	JoinDIFAndAllocateFlowTask * join_task;
	std::list<std::string> dif_names;

	// Find the name of the DIF that will provide the flow
	if (event->DIFName != rina::ApplicationProcessNamingInformation()) {
		// The requestor specified a DIF name
		dif_name = event->DIFName;
		dif_specified = DA_SUCCESS;
	} else {
		// Ask the DIF allocator
		dif_specified =  dif_allocator->lookup_dif_by_application(event->remoteApplicationName,
									  dif_name, dif_names);
		if (dif_specified == DA_PENDING)
			return IPCM_PENDING;
	}

	// Select an IPC process to serve the flow request
	if (dif_specified == DA_SUCCESS) {
		ipcp = select_ipcp_by_dif(dif_name);

		if (!ipcp && dif_names.size() > 0) {
			join_task = new JoinDIFAndAllocateFlowTask(this, promise, *event,
								   dif_name.processName, dif_names);
			timer.scheduleTask(join_task, 0);
			return IPCM_PENDING;
		}
	} else {
		ipcp = select_ipcp();
	}

	if (!ipcp) {
		ss  << " Error: Cannot find an IPC process to "
			"serve flow allocation request (local-app = " <<
			event->localApplicationName.toString() << ", " <<
			"remote-app = " << event->remoteApplicationName.
			toString() << endl;
		FLUSH_LOG(ERR, ss);

		if (!promise)
			application_flow_allocation_failed_notify(event);

		return IPCM_FAILURE;
	}

	//Auto release the read lock
	rina::ReadScopedLock readlock(ipcp->rwlock, false);

	try {
		// Ask the IPC process to allocate a flow
		trans = new FlowAllocTransState(NULL, promise, ipcp->get_id(),
						*event, dif_specified);
		if(!trans){
			ss << "Unable to allocate memory for the transaction object. Out of memory! ";
			if (!promise)
				throw rina::AllocateFlowException();

			return IPCM_FAILURE;
		}

		//Store transaction
		if(add_transaction_state(trans) < 0){
			ss << "Unable to add transaction; out of memory? ";
			if (!promise)
				throw rina::AllocateFlowException();

			return IPCM_FAILURE;
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

		if (!promise)
			application_flow_allocation_failed_notify(event);

		return IPCM_FAILURE;
	}

	return IPCM_PENDING;
}

ipcm_res_t
IPCManager_::flow_allocation_requested_remote(rina::FlowRequestEvent *event)
{
	IPCMIPCProcess *ipcp;
	ostringstream ss;
	FlowAllocTransState* trans = NULL;
	unsigned int ctrl_port = 0;

	// Retrieve the local IPC process involved in the flow allocation
	// request coming from a remote application
	ipcp = lookup_ipcp_by_id(event->ipcProcessId);
	if (!ipcp) {
		ss  << ": Error: Could not retrieve IPC process "
			"with id " << event->ipcProcessId << ", to serve "
			"remote flow allocation request" << endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	}

	if (event->localApplicationName.entityName == RINA_IP_FLOW_ENT_NAME) {
		ipcp->rwlock.unlock();
		ip_vpn_manager->ip_vpn_flow_allocation_requested(*event);
		return IPCM_PENDING;
	}

	ctrl_port = ipcp->get_fa_ctrl_port(event->localApplicationName);
	if (ctrl_port == 0) {
		ipcp->rwlock.unlock();
		ss  << ": Error: Could not find ctrl port to send a FA request "
			"to local application " << event->localApplicationName.toString() << endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	}

	//Auto release the read lock
	rina::ReadScopedLock readlock(ipcp->rwlock, false);

	try {
		// Inform the local application that a remote application
		// wants to allocate a flow
		trans = new FlowAllocTransState(NULL, NULL, ipcp->get_id(),
						*event, true);
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

		rina::applicationManager->flowRequestArrived(event->localApplicationName,
							     event->remoteApplicationName,
							     event->flowSpecification,
							     event->DIFName, event->portId,
							     trans->tid, ctrl_port);

		ss << "Arrived request for flow allocation between " <<
			event->localApplicationName.toString()
			<< " and " << event->remoteApplicationName.toString()
			<< endl;
		FLUSH_LOG(INFO, ss);
	} catch (rina::IPCException& e) {
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
			ipcp->allocateFlowResponse(*event, -1, true, 0, 0);

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

		return IPCM_FAILURE;
	}

	return IPCM_SUCCESS;
}

ipcm_res_t IPCManager_::flow_allocation_requested_event_handler(Promise * promise,
							  	rina::FlowRequestEvent* event)
{
	if (event->localRequest)
		return flow_allocation_requested_local(promise, event);
	else
		return flow_allocation_requested_remote(event);
}

void IPCManager_::ipcm_allocate_flow_request_result_handler(rina::IpcmAllocateFlowRequestResultEvent *event)
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

	if (req_event.localApplicationName.entityName == RINA_IP_FLOW_ENT_NAME) {
		if (success) {
			req_event.ipcProcessId = slave_ipcp->get_id();
			ip_vpn_manager->iporina_flow_allocated(req_event);
		}
	} else {
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
		slave_ipcp->allocateFlowResponse(req_event, event->result,
						 event->pid, event->notifySource,
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

ipcm_res_t IPCManager_::flow_deallocation_requested_event_handler(Promise * promise,
							          rina::FlowDeallocateRequestEvent* event)
{
	IPCMIPCProcess *ipcp = lookup_ipcp_by_port(event->portId);
	unsigned short ipcp_id = 0;
	ostringstream ss;

	if (!ipcp) {
		ss  << ": Cannot find the IPC process that "
			"provides the flow with port-id " << event->portId
			<< endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	}

	{
		//Auto release the read lock
		rina::ReadScopedLock readlock(ipcp->rwlock, false);
		ipcp_id = ipcp->get_id();
	}

	return deallocate_flow(promise, ipcp_id, *event);
}

void IPCManager_::flow_deallocated_event_handler(rina::FlowDeallocatedEvent* event)
{
	IPCMIPCProcess * ipcp, * user_ipcp;
	rina::FlowInformation info;
	ostringstream ss;

	ipcp = lookup_ipcp_by_port(event->portId);
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

		// Only notify app about deallocated flow if it is an IPCP
		// Otherwise app will find out because the I/O fd will close
		user_ipcp = ipcp_factory_.getIPCProcess(info.user_ipcp_id);
		if (user_ipcp) {
			rina::applicationManager->flowDeallocatedRemotely(event->portId, event->code,
									  user_ipcp->proxy_->portId);
		} else {
			//Check in case it is an IP VPN
			ip_vpn_manager->ip_vpn_flow_deallocated(event->portId,
							        ipcp->get_id());
		}

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
