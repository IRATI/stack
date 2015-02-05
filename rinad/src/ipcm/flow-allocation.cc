/*
 * IPC Manager - Flow allocation/deallocation
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
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

#define RINA_PREFIX "ipcm"
#include <librina/logs.h>

#include "rina-configuration.h"
#include "helpers.h"
#include "ipcm.h"

using namespace std;


namespace rinad {

int
IPCManager_::deallocate_flow(const int ipcp_id,
                            const rina::FlowDeallocateRequestEvent& event)
{
        ostringstream ss;
        unsigned int seqnum;
	rina::IPCProcess* ipcp;

        try {
		ipcp = lookup_ipcp_by_id(ipcp_id);

		if(!ipcp){
			ss << "Invalid IPCP id "<< ipcp_id;
                        throw Exception();
		}


                // Ask the IPC process to deallocate the flow
                // specified by the port-id
                seqnum = ipcp->deallocateFlow(event.portId);
                pending_flow_deallocations[seqnum] =
                                        make_pair(ipcp, event);

                ss << "Application " << event.applicationName.toString() <<
                        " asks IPC process " << ipcp->name.toString() <<
                        " to deallocate flow [port-id = " << event.portId <<
                        "]" << endl;
                FLUSH_LOG(INFO, ss);
        } catch (rina::IpcmDeallocateFlowException) {
                ss  << ": Error while application " <<
                        event.applicationName.toString() << "asks IPC process "
                        << ipcp->name.toString() << " to deallocate the flow "
                        "with port-id " << event.portId << endl;
                FLUSH_LOG(ERR, ss);
                return -1;
        }

        return 0;
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
        } catch (rina::NotifyFlowAllocatedException) {
                ss  << ": Error while notifying the "
                        "Application Manager about flow allocation "
                        "result" << endl;
                FLUSH_LOG(ERR, ss);
        }
}

void IPCManager_::flow_allocation_requested_local(rina::FlowRequestEvent *event)
{
        rina::ApplicationProcessNamingInformation dif_name;
        rina::IPCProcess *ipcp = NULL;
        bool dif_specified;
        unsigned int seqnum;
        ostringstream ss;

        // Find the name of the DIF that will provide the flow
        dif_specified = lookup_dif_by_application(event->
                                        localApplicationName, dif_name);
        if (!dif_specified) {
                if (event->DIFName !=
                                rina::ApplicationProcessNamingInformation()) {
                        dif_name = event->DIFName;
                        dif_specified = true;
                }
        }

        // Select an IPC process to serve the flow request
        if (!dif_specified) {
                ipcp = select_ipcp();
        } else {
                ipcp = select_ipcp_by_dif(dif_name);
        }

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

        try {
                // Ask the IPC process to allocate a flow
                seqnum = ipcp->allocateFlow(*event);
                pending_flow_allocations[seqnum] =
                                        PendingFlowAllocation(ipcp, *event,
                                                              dif_specified);

                ss << "IPC process " << ipcp->name.toString() <<
                        " requested to allocate flow between " <<
                        event->localApplicationName.toString()
                        << " and " << event->remoteApplicationName.toString()
                        << endl;
                FLUSH_LOG(INFO, ss);
        } catch (rina::AllocateFlowException) {
                ss  << ": Error while requesting IPC process "
                        << ipcp->name.toString() << " to allocate a flow "
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
        // Retrieve the local IPC process involved in the flow allocation
        // request coming from a remote application
        rina::IPCProcess *ipcp = rina::ipcProcessFactory->
                                        getIPCProcess(event->ipcProcessId);
        ostringstream ss;
        unsigned int seqnum;

        if (!ipcp) {
                ss  << ": Error: Could not retrieve IPC process "
                        "with id " << event->ipcProcessId << ", to serve "
                        "remote flow allocation request" << endl;
                FLUSH_LOG(ERR, ss);
                return;
        }

        try {
                // Inform the local application that a remote application
                // wants to allocate a flow
                seqnum = rina::applicationManager->flowRequestArrived(
                                        event->localApplicationName,
                                        event->remoteApplicationName,
                                        event->flowSpecification,
                                        event->DIFName, event->portId);
                pending_flow_allocations[seqnum] =
                                PendingFlowAllocation(ipcp, *event, true);

                ss << "Arrived request for flow allocation between " <<
                        event->localApplicationName.toString()
                        << " and " << event->remoteApplicationName.toString()
                        << endl;
                FLUSH_LOG(INFO, ss);
        } catch (rina::AppFlowArrivedException) {
                ss  << ": Error while informing application "
                        << event->localApplicationName.toString() <<
                        " about a flow request coming from remote application "
                        << event->remoteApplicationName.toString() << endl;
                FLUSH_LOG(ERR, ss);
                try {
                        // Inform the IPC process that it was not possible
                        // to allocate the flow
                        ipcp->allocateFlowResponse(*event, -1, true, 0);

                        ss << "IPC process " << ipcp->name.toString() <<
                                " informed that it was not possible to "
                                "allocate flow between" <<
                                event->localApplicationName.toString()
                                << " and " << event->remoteApplicationName.
                                toString() << endl;
                        FLUSH_LOG(INFO, ss);
                } catch (rina::AllocateFlowException) {
                        ss  << ": Error while informing IPC "
                                << "process " << ipcp->name.toString() <<
                                " about failed flow allocation" << endl;
                        FLUSH_LOG(ERR, ss);
                }
        }
}

void IPCManager_::flow_allocation_requested_event_handler(rina::IPCEvent *e)
{
        DOWNCAST_DECL(e, rina::FlowRequestEvent, event);

        if (event->localRequest) {
                flow_allocation_requested_local(event);
        } else {
                flow_allocation_requested_remote(event);
        }
}

void IPCManager_::ipcm_allocate_flow_request_result_handler(rina::IPCEvent *e)
{
        DOWNCAST_DECL(e, rina::IpcmAllocateFlowRequestResultEvent, event);
        map<unsigned int, PendingFlowAllocation>::iterator mit;
        bool success = (event->result == 0);
        ostringstream ss;

        mit = pending_flow_allocations.find(event->sequenceNumber);
        if (mit == pending_flow_allocations.end()) {
                ss  << ": Warning: Flow allocation request result "
                        "received but no corresponding request" << endl;
                FLUSH_LOG(WARN, ss);
                return;
        }

        rina::IPCProcess *slave_ipcp = mit->second.slave_ipcp;
        rina::FlowRequestEvent& req_event = mit->second.req_event;

        req_event.portId = -1;
        try {
                // Inform the IPC process about the result of the
                // flow allocation operation
                slave_ipcp->allocateFlowResult(event->sequenceNumber,
                                               success, event->portId);
                if (success) {
                        req_event.portId = event->portId;
                } else {
                        // TODO retry with other DIFs
                }

                ss << "Informing IPC process " <<
                        slave_ipcp->name.toString()
                        << " about flow allocation from application " <<
                        req_event.localApplicationName.toString() <<
                        " to application " <<
                        req_event.remoteApplicationName.toString() <<
                        " in DIF " << slave_ipcp->getDIFInformation().
                        dif_name_.toString() << " [success = " << success
                        << ", port-id = " << event->portId << "]" << endl;
                FLUSH_LOG(INFO, ss);
        } catch (rina::AllocateFlowException) {
                ss  << ": Error while informing the IPC process "
                        << slave_ipcp->name.toString() << " about result of "
                        "flow allocation between applications " <<
                        req_event.localApplicationName.toString() <<
                        " and " << req_event.remoteApplicationName.toString()
                        << endl;
                FLUSH_LOG(ERR, ss);
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
        } catch (rina::NotifyFlowAllocatedException) {
                ss  << ": Error while notifying the "
                        "Application Manager about flow allocation result"
                        << endl;
                FLUSH_LOG(ERR, ss);
        }

        pending_flow_allocations.erase(mit);
}

void IPCManager_::allocate_flow_request_result_event_handler(rina::IPCEvent *e)
{
        (void) e; // Stop compiler barfs
}

void IPCManager_::allocate_flow_response_event_handler(rina::IPCEvent *e)
{
        DOWNCAST_DECL(e, rina::AllocateFlowResponseEvent, event);
        map<unsigned int, PendingFlowAllocation>::iterator mit;
        bool success = (event->result == 0);
        ostringstream ss;

        mit = pending_flow_allocations.find(event->sequenceNumber);
        if (mit == pending_flow_allocations.end()) {
                ss  << ": Warning: Flow allocation response "
                        "received but no corresponding request" << endl;
                FLUSH_LOG(WARN, ss);
                return;
        }

        rina::IPCProcess *slave_ipcp = mit->second.slave_ipcp;
        rina::FlowRequestEvent& req_event = mit->second.req_event;

        try {
                // Inform the IPC process about the response of the flow
                // allocation procedure
                slave_ipcp->allocateFlowResponse(req_event, event->result,
                                        event->notifySource,
                                        event->flowAcceptorIpcProcessId);
                if (!success) {
                        req_event.portId = -1;
                }

                ss  << ": Informing IPC process " <<
                        slave_ipcp->name.toString()
                        << " about flow allocation from application " <<
                        req_event.localApplicationName.toString() <<
                        " to application " <<
                        req_event.remoteApplicationName.toString() <<
                        " in DIF " << slave_ipcp->getDIFInformation().
                        dif_name_.toString() << " [success = " << success
                        << ", port-id = " << req_event.portId << "]" << endl;
                FLUSH_LOG(INFO, ss);
        } catch (rina::AllocateFlowException) {
                ss  << ": Error while informing IPC "
                        << "process " << slave_ipcp->name.toString() <<
                        " about failed flow allocation" << endl;
                FLUSH_LOG(ERR, ss);
                req_event.portId = -1;
        }

        pending_flow_allocations.erase(mit);
}

void IPCManager_::flow_deallocation_requested_event_handler(rina::IPCEvent *e)
{
        DOWNCAST_DECL(e, rina::FlowDeallocateRequestEvent, event);
        rina::IPCProcess *ipcp = lookup_ipcp_by_port(event->portId);
        ostringstream ss;
        int ret;

        if (!ipcp) {
                ss  << ": Cannot find the IPC process that "
                        "provides the flow with port-id " << event->portId
                        << endl;
                FLUSH_LOG(ERR, ss);
                return;
        }

        ret = deallocate_flow(ipcp->id, *event);
        if (ret) {
                try {
                        // Inform the application about the deallocation
                        // failure
                        rina::applicationManager->flowDeallocated(*event, -1);

                        ss << "Application " << event->applicationName.
                                toString() << " informed about deallocation "
                                "failure of flow identified by port-id " <<
                                event->portId << endl;
                        FLUSH_LOG(INFO, ss);
                } catch (rina::NotifyFlowDeallocatedException) {
                        ss  << ": Error while informing "
                                "application " << event->applicationName.
                                toString() << " about deallocation of flow "
                                "identified by port-id " << event->portId
                                << endl;
                        FLUSH_LOG(ERR, ss);
                }
        }
}

void IPCManager_::ipcm_deallocate_flow_response_event_handler(rina::IPCEvent *e)
{
        DOWNCAST_DECL(e, rina::IpcmDeallocateFlowResponseEvent, event);
        std::map<unsigned int,
                 std::pair<rina::IPCProcess *, rina::FlowDeallocateRequestEvent>
                >::iterator mit;
        bool success = (event->result == 0);
        int result = event->result;
        ostringstream ss;

        mit = pending_flow_deallocations.find(event->sequenceNumber);
        if (mit == pending_flow_deallocations.end()) {
                ss  << ": Warning: Flow deallocation response "
                        "received but no corresponding request" << endl;
                FLUSH_LOG(WARN, ss);
                return;
        }

        rina::IPCProcess *ipcp = mit->second.first;
        rina::FlowDeallocateRequestEvent& req_event = mit->second.second;

        try {
                // Inform the IPC process about the deallocation result
                ipcp->deallocateFlowResult(event->sequenceNumber, success);

                ss << "Deallocating flow "
                        << "identified by port-id " <<
                        req_event.portId << " from IPC process " <<
                        ipcp->name.toString() << " [success = " <<
                        success << "]" << endl;
                FLUSH_LOG(INFO, ss);
        } catch (rina::IpcmDeallocateFlowException) {
                ss  << ": Error while informing IPC process "
                        << ipcp->name.toString() << " about deallocation "
                        "of flow with port-id " << req_event.portId << endl;
                FLUSH_LOG(ERR, ss);
                result = -1;
        }

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
        } catch (rina::NotifyFlowDeallocatedException) {
                ss  << ": Error while informing "
                        "application " << req_event.applicationName.
                        toString() << " about deallocation of flow "
                        "identified by port-id " << req_event.portId
                        << endl;
                FLUSH_LOG(ERR, ss);
        }

        pending_flow_deallocations.erase(mit);
}

void IPCManager_::flow_deallocated_event_handler(rina::IPCEvent *e)
{
        DOWNCAST_DECL(e, rina::FlowDeallocatedEvent, event);
        rina::IPCProcess *ipcp = lookup_ipcp_by_port(event->portId);
        rina::FlowInformation info;
        ostringstream ss;

        if (!ipcp) {
                ss  << ": Cannot find the IPC process that "
                        "provides the flow with port-id " << event->portId
                        << endl;
                FLUSH_LOG(ERR, ss);
                return;
        }

        try {
                // Inform the IPC process that the flow corresponding to
                // the specified port-id has been deallocated
                info = ipcp->flowDeallocated(event->portId);
                rina::applicationManager->
                        flowDeallocatedRemotely(event->portId, event->code,
                                                info.localAppName);

                ss << "IPC process " << ipcp->name.toString() <<
                        " and local application " << info.localAppName.
                        toString() << " informed that flow with port-id "
                        << event->portId << " has been deallocated remotely "
                        << endl;
                FLUSH_LOG(INFO, ss);
        } catch (rina::IpcmDeallocateFlowException) {
                ss  << ": Cannot find a flow with port-id "
                        << event->portId << endl;
                FLUSH_LOG(ERR, ss);
        } catch (rina::NotifyFlowDeallocatedException) {
                ss  << ": Error while informing application "
                        << info.localAppName.toString() <<
                        " that flow with port-id " << event->portId <<
                        " has been remotely deallocated" << endl;
                FLUSH_LOG(ERR, ss);
        }
}

void IPCManager_::deallocate_flow_response_event_handler(rina::IPCEvent *event)
{
        (void) event; // Stop compiler barfs
}

}
