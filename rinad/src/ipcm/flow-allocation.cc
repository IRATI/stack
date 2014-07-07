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

void
flow_allocation_requested_event_handler(rina::IPCEvent *e,
					EventLoopData *opaque)
{
        DOWNCAST_DECL(e, rina::FlowRequestEvent, event);
        DOWNCAST_DECL(opaque, IPCManager, ipcm);
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
                cerr << __func__ << " Error while requesting IPC process "
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
                        cerr << __func__ << "Error while notifying the "
                                "Application Manager about flow allocation"
                                << endl;
                }
        }
}

void
allocate_flow_request_result_event_handler(rina::IPCEvent *event,
					EventLoopData *opaque)
{
        (void) event; // Stop compiler barfs
        (void) opaque;    // Stop compiler barfs
}

void
allocate_flow_response_event_handler(rina::IPCEvent *event,
					EventLoopData *opaque)
{
        (void) event; // Stop compiler barfs
        (void) opaque;    // Stop compiler barfs
}

void
flow_deallocation_requested_event_handler(rina::IPCEvent *event,
					EventLoopData *opaque)
{
        (void) event; // Stop compiler barfs
        (void) opaque;    // Stop compiler barfs
}

void
deallocate_flow_response_event_handler(rina::IPCEvent *event,
					EventLoopData *opaque)
{
        (void) event; // Stop compiler barfs
        (void) opaque;    // Stop compiler barfs
}

void
flow_deallocated_event_handler(rina::IPCEvent *event,
					EventLoopData *opaque)
{
        (void) event; // Stop compiler barfs
        (void) opaque;    // Stop compiler barfs
}

void
ipcm_deallocate_flow_response_event_handler(rina::IPCEvent *event,
					EventLoopData *opaque)
{
        (void) event; // Stop compiler barfs
        (void) opaque;    // Stop compiler barfs
}

void
ipcm_allocate_flow_request_result_handler(rina::IPCEvent *event,
					EventLoopData *opaque)
{
        (void) event; // Stop compiler barfs
        (void) opaque;    // Stop compiler barfs
}

}
