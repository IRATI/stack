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

#define RINA_PREFIX     "ipcm"

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/logs.h>

#include "event-loop.h"
#include "rina-configuration.h"
#include "helpers.h"
#include "ipcm.h"
#include "flow-allocation.h"

using namespace std;


namespace rinad {

int
IPCManager::deallocate_flow(rina::IPCProcess *ipcp,
                            const rina::FlowDeallocateRequestEvent& event)
{
        unsigned int seqnum;

        try {
                // Ask the IPC process to deallocate the flow
                // specified by the port-id
                seqnum = ipcp->deallocateFlow(event.portId);
                pending_flow_deallocations[seqnum] =
                                        make_pair(ipcp, event);
        } catch (rina::IpcmDeallocateFlowException) {
                cerr << __func__ << ": Error while asking IPC process "
                        << ipcp->name.toString() << " to deallocate the flow "
                        "with port-id " << event.portId << endl;
                return -1;
        }

        return 0;
}

static void
flow_allocation_requested_local(rina::FlowRequestEvent *event,
                                IPCManager *ipcm)
{
        rina::ApplicationProcessNamingInformation dif_name;
        rina::IPCProcess *ipcp = NULL;
        bool dif_specified;
        unsigned int seqnum;

        // Find the name of the DIF that will provide the flow
        dif_specified = ipcm->config.lookup_dif_by_application(event->
                                        localApplicationName, dif_name);
        if (!dif_specified) {
                if (event->DIFName.toString().size()) {
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
                cerr << __func__ << " Error: Cannot find an IPC process to "
                        "serve flow allocation request (local-app = " <<
                        event->localApplicationName.toString() << ", " <<
                        "remote-app = " << event->remoteApplicationName.
                        toString() << endl;
                return;
        }

        try {
                // Ask the IPC process to allocate a flow
                seqnum = ipcp->allocateFlow(*event);
                ipcm->pending_flow_allocations[seqnum] =
                                        PendingFlowAllocation(ipcp, *event,
                                                              dif_specified);
        } catch (rina::AllocateFlowException) {
                cerr << __func__ << ": Error while requesting IPC process "
                        << ipcp->name.toString() << " to allocate a flow "
                        "between " << event->localApplicationName.toString()
                        << " and " << event->remoteApplicationName.toString()
                        << endl;

                // Inform the Application Manager about the flow allocation
                // failure
                event->portId = -1;
                try {
                        rina::applicationManager->flowAllocated(*event);
                } catch (rina::NotifyFlowAllocatedException) {
                        cerr << __func__ << ": Error while notifying the "
                                "Application Manager about flow allocation "
                                "result" << endl;
                }
        }
}

static void
flow_allocation_requested_remote(rina::FlowRequestEvent *event,
                                 IPCManager *ipcm)
{
        // Retrieve the local IPC process involved in the flow allocation
        // request coming from a remote application
        rina::IPCProcess *ipcp = rina::ipcProcessFactory->
                                        getIPCProcess(event->ipcProcessId);
        unsigned int seqnum;

        if (!ipcp) {
                cerr << __func__ << ": Error: Could not retrieve IPC process "
                        "with id " << event->ipcProcessId << ", to serve "
                        "remote flow allocation request" << endl;
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
                ipcm->pending_flow_allocations[seqnum] =
                                PendingFlowAllocation(ipcp, *event, true);
        } catch (rina::AppFlowArrivedException) {
                cerr << __func__ << ": Error while informing application "
                        << event->localApplicationName.toString() <<
                        " about a flow request coming from remote application "
                        << event->remoteApplicationName.toString() << endl;
                try {
                        // Inform the IPC process that it was not possible
                        // to allocate the flow
                        ipcp->allocateFlowResponse(*event, -1, true, 0);
                } catch (rina::AllocateFlowException) {
                        cerr << __func__ << ": Error while informing IPC "
                                << "process " << ipcp->name.toString() <<
                                " about failed flow allocation" << endl;
                }
        }
}

void
flow_allocation_requested_event_handler(rina::IPCEvent *e,
					EventLoopData *opaque)
{
        DOWNCAST_DECL(e, rina::FlowRequestEvent, event);
        DOWNCAST_DECL(opaque, IPCManager, ipcm);

        if (event->localRequest) {
                flow_allocation_requested_local(event, ipcm);
        } else {
                flow_allocation_requested_remote(event, ipcm);
        }
}

void
ipcm_allocate_flow_request_result_handler(rina::IPCEvent *e,
					EventLoopData *opaque)
{
        DOWNCAST_DECL(e, rina::IpcmAllocateFlowRequestResultEvent, event);
        DOWNCAST_DECL(opaque, IPCManager, ipcm);
        map<unsigned int, PendingFlowAllocation>::iterator mit;
        bool success = (event->result == 0);

        mit = ipcm->pending_flow_allocations.find(event->sequenceNumber);
        if (mit == ipcm->pending_flow_allocations.end()) {
                cerr << __func__ << ": Warning: Flow allocation request result "
                        "received but no corresponding request" << endl;
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
                        cout << __func__ << ": Info: Flow allocation "
                                "from application " <<
                                req_event.localApplicationName.toString() <<
                                " to application " <<
                                req_event.remoteApplicationName.toString() <<
                                " in DIF " << slave_ipcp->getDIFInformation().
                                dif_name_.toString() <<
                                " with port-id " << event->portId <<
                                " failed" << endl;

                        // TODO retry with other DIFs
                }
        } catch (rina::AllocateFlowException) {
                cerr << __func__ << ": Error while informing the IPC process "
                        << slave_ipcp->name.toString() << " about result of "
                        "flow allocation between applications " <<
                        req_event.localApplicationName.toString() <<
                        " and " << req_event.remoteApplicationName.toString()
                        << endl;
        }

        // Inform the Application Manager about the flow allocation
        // result
        try {
                rina::applicationManager->flowAllocated(req_event);
        } catch (rina::NotifyFlowAllocatedException) {
                cerr << __func__ << ": Error while notifying the "
                        "Application Manager about flow allocation result"
                        << endl;
        }

        ipcm->pending_flow_allocations.erase(mit);
}

void
allocate_flow_request_result_event_handler(rina::IPCEvent *e,
					   EventLoopData *opaque)
{
        (void) e; // Stop compiler barfs
        (void) opaque;    // Stop compiler barfs
}

void
allocate_flow_response_event_handler(rina::IPCEvent *e,
				     EventLoopData *opaque)
{
        DOWNCAST_DECL(e, rina::AllocateFlowResponseEvent, event);
        DOWNCAST_DECL(opaque, IPCManager, ipcm);
        map<unsigned int, PendingFlowAllocation>::iterator mit;
        bool success = (event->result == 0);

        mit = ipcm->pending_flow_allocations.find(event->sequenceNumber);
        if (mit == ipcm->pending_flow_allocations.end()) {
                cerr << __func__ << ": Warning: Flow allocation response "
                        "received but no corresponding request" << endl;
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
                if (success) {
                        // TODO success info
                } else {
                        cout << __func__ << ": Info: Flow allocation "
                                "from application " <<
                                req_event.localApplicationName.toString() <<
                                " to application " <<
                                req_event.remoteApplicationName.toString() <<
                                " in DIF " << slave_ipcp->getDIFInformation().
                                dif_name_.toString() << " failed" << endl;
                        req_event.portId = -1;
                }
        } catch (rina::AllocateFlowException) {
                cerr << __func__ << ": Error while informing IPC "
                        << "process " << slave_ipcp->name.toString() <<
                        " about failed flow allocation" << endl;
                req_event.portId = -1;
        }

        ipcm->pending_flow_allocations.erase(mit);
}

void
flow_deallocation_requested_event_handler(rina::IPCEvent *e,
					  EventLoopData *opaque)
{
        DOWNCAST_DECL(e, rina::FlowDeallocateRequestEvent, event);
        DOWNCAST_DECL(opaque, IPCManager, ipcm);
        rina::IPCProcess *ipcp = lookup_ipcp_by_port(event->portId);
        int ret;

        if (!ipcp) {
                cerr << __func__ << ": Cannot find the IPC process that "
                        "provides the flow with port-id " << event->portId
                        << endl;
                return;
        }

        ret = ipcm->deallocate_flow(ipcp, *event);
        if (ret) {
                try {
                        // Inform the application about the deallocation
                        // failure
                        rina::applicationManager->flowDeallocated(*event, -1);
                } catch (rina::NotifyFlowDeallocatedException) {
                        cerr << __func__ << ": Error while informing "
                                "application " << event->applicationName.
                                toString() << " about deallocation of flow "
                                "identified by port-id " << event->portId
                                << endl;
                }
        }
}

void
ipcm_deallocate_flow_response_event_handler(rina::IPCEvent *e,
					    EventLoopData *opaque)
{
        DOWNCAST_DECL(e, rina::IpcmDeallocateFlowResponseEvent, event);
        DOWNCAST_DECL(opaque, IPCManager, ipcm);
        std::map<unsigned int,
                 std::pair<rina::IPCProcess *, rina::FlowDeallocateRequestEvent>
                >::iterator mit;
        bool success = (event->result == 0);
        int result = event->result;

        mit = ipcm->pending_flow_deallocations.find(event->sequenceNumber);
        if (mit == ipcm->pending_flow_deallocations.end()) {
                cerr << __func__ << ": Warning: Flow deallocation response "
                        "received but no corresponding request" << endl;
                return;
        }

        rina::IPCProcess *ipcp = mit->second.first;
        rina::FlowDeallocateRequestEvent& req_event = mit->second.second;

        try {
                // Inform the IPC process about the deallocation result
                ipcp->deallocateFlowResult(event->sequenceNumber, success);
                if (success) {
                        // TODO success print
                } else {
                        cout << __func__ << ": Cannot deallocate flow "
                                << "identified by port-id " <<
                                req_event.portId << " from IPC process " <<
                                ipcp->name.toString() << endl;
                }
        } catch (rina::IpcmDeallocateFlowException) {
                cerr << __func__ << ": Error while informing IPC process "
                        << ipcp->name.toString() << " about deallocation "
                        "of flow with port-id " << req_event.portId << endl;
                result = -1;
        }

        try {
                // Inform the application about the deallocation
                // result
                if (req_event.sequenceNumber > 0) {
                        rina::applicationManager->
                                flowDeallocated(req_event, result);
                }
        } catch (rina::NotifyFlowDeallocatedException) {
                cerr << __func__ << ": Error while informing "
                        "application " << req_event.applicationName.
                        toString() << " about deallocation of flow "
                        "identified by port-id " << req_event.portId
                        << endl;
        }

        ipcm->pending_flow_deallocations.erase(mit);
}

void
flow_deallocated_event_handler(rina::IPCEvent *e,
					EventLoopData *opaque)
{
        DOWNCAST_DECL(e, rina::FlowDeallocatedEvent, event);
        DOWNCAST_DECL(opaque, IPCManager, ipcm);
        rina::IPCProcess *ipcp = lookup_ipcp_by_port(event->portId);
        rina::FlowInformation info;

        (void) ipcm;

        if (!ipcp) {
                cerr << __func__ << ": Cannot find the IPC process that "
                        "provides the flow with port-id " << event->portId
                        << endl;
                return;
        }

        try {
                // Inform the IPC process that the flow corresponding to
                // the specified port-id has been deallocated
                info = ipcp->flowDeallocated(event->portId);
                rina::applicationManager->
                        flowDeallocatedRemotely(event->portId, event->code,
                                                info.localAppName);
        } catch (rina::IpcmDeallocateFlowException) {
                cerr << __func__ << ": Cannot find a flow with port-id "
                        << event->portId << endl;
        } catch (rina::NotifyFlowDeallocatedException) {
                cerr << __func__ << ": Error while informing application that "
                        "flow with port-id " << event->portId << " has been "
                        "remotely deallocated" << endl;
        }
}

void
deallocate_flow_response_event_handler(rina::IPCEvent *event,
					EventLoopData *opaque)
{
        (void) event; // Stop compiler barfs
        (void) opaque;    // Stop compiler barfs
}

}
