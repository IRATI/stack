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
flow_allocation_requested_event_handler(rina::IPCEvent *event,
					EventLoopData *opaque)
{
        (void) event; // Stop compiler barfs
        (void) opaque;    // Stop compiler barfs
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
