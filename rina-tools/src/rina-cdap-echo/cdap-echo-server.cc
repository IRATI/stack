//
// Echo CDAP Server
// 
// Bernat Gastón <bernat.gaston@i2cat.net>
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include <iostream>
#include <thread>
#include <time.h>
#include <signal.h>
#include <time.h>

#define RINA_PREFIX     "rina-cdap-echo-server"
#include <librina/logs.h>

#include "cdap-echo-server.h"

using namespace std;
using namespace rina;

Server::Server(const string& dif_name,
               const string& app_name,
               const string& app_instance,
               const int dealloc_wait) :
        Application(dif_name, app_name, app_instance),
        dw(dealloc_wait)
{ }

void Server::run()
{
        applicationRegister();

        for(;;) {
                IPCEvent* event = ipcEventProducer->eventWait();
                Flow *flow = 0;
                unsigned int port_id;
                DeallocateFlowResponseEvent *resp = 0;

                if (!event)
                        return;

                switch (event->eventType) {

                case REGISTER_APPLICATION_RESPONSE_EVENT:
                        ipcManager->commitPendingRegistration(event->sequenceNumber,
                                                             dynamic_cast<RegisterApplicationResponseEvent*>(event)->DIFName);
                        break;

                case UNREGISTER_APPLICATION_RESPONSE_EVENT:
                        ipcManager->appUnregistrationResult(event->sequenceNumber,
                                                            dynamic_cast<UnregisterApplicationResponseEvent*>(event)->result == 0);
                        break;

                case FLOW_ALLOCATION_REQUESTED_EVENT:
                        flow = ipcManager->allocateFlowResponse(*dynamic_cast<FlowRequestEvent*>(event), 0, true);
                        LOG_INFO("New flow allocated [port-id = %d]", flow->getPortId());
                        startWorker(flow);
                        break;

                case FLOW_DEALLOCATED_EVENT:
                        port_id = dynamic_cast<FlowDeallocatedEvent*>(event)->portId;
                        ipcManager->flowDeallocated(port_id);
                        LOG_INFO("Flow torn down remotely [port-id = %d]", port_id);
                        break;

                case DEALLOCATE_FLOW_RESPONSE_EVENT:
                        LOG_INFO("Destroying the flow after time-out");
                        resp = dynamic_cast<DeallocateFlowResponseEvent*>(event);
                        port_id = resp->portId;

                        ipcManager->flowDeallocationResult(port_id, resp->result == 0);
                        break;

                default:
                        LOG_INFO("Server got new event of type %d",
                                        event->eventType);
                        break;
                }
        }
}

void Server::startWorker(Flow *flow)
{
	void (Server::*server_function)(Flow *flow);

	server_function = &Server::serveEchoFlow;

	thread t(server_function, this, flow);
	t.detach();
}

void Server::serveEchoFlow(Flow* flow)
{
        char *buffer = new char[max_buffer_size];
        struct sigevent event;
        struct itimerspec itime;
        timer_t timer_id;

        // Setup a timer if dealloc_wait option is set */
        if (dw > 0) {
                event.sigev_notify = SIGEV_THREAD;
                event.sigev_value.sival_ptr = flow;
                event.sigev_notify_function = &destroyFlow;

                timer_create(CLOCK_REALTIME, &event, &timer_id);

                itime.it_interval.tv_sec = 0;
                itime.it_interval.tv_nsec = 0;
                itime.it_value.tv_sec = dw;
                itime.it_value.tv_nsec = 0;

                timer_settime(timer_id, 0, &itime, NULL);
        }

        try {
                for(;;) {
                	// TODO Implement CDAP messages
                        int bytes_read = flow->readSDU(buffer,
                                                        max_buffer_size);
                        flow->writeSDU(buffer, bytes_read);
                        if (dw > 0)
                                timer_settime(timer_id, 0, &itime, NULL);
                }
        } catch(rina::IPCException e) {
                // This thread was blocked in the readSDU() function
                // when the flow gets deallocated
        }

        if  (timer_id) {
                timer_delete(timer_id);
        }

        delete [] buffer;
}

static unsigned long
timespec_diff_us(const struct timespec& before, const struct timespec& later)
{
        return ((later.tv_sec - before.tv_sec) * 1000000000 +
                        (later.tv_nsec - before.tv_nsec))/1000;
}

void Server::destroyFlow(sigval_t val)
{
        Flow *flow = (Flow *)val.sival_ptr;

        if (flow->getState() != FlowState::FLOW_ALLOCATED)
                return;

        int port_id = flow->getPortId();

        // TODO here we should store the seqnum (handle) returned by
        // requestFlowDeallocation() and match it in the event loop
        ipcManager->requestFlowDeallocation(port_id);
}
