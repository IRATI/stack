//
// Echo Server
// 
// Addy Bombeke <addy.bombeke@ugent.be>
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

#define RINA_PREFIX     "rina-echo-app"
#include <librina/logs.h>

#include "server.h"

using namespace std;
using namespace rina;

Server::Server(const string& t_type,
               const string& dif_name,
               const string& app_name,
               const string& app_instance,
               const int perf_interval,
               const int dealloc_wait) :
        Application(dif_name, app_name, app_instance),
        test_type(t_type), interval(perf_interval), dw(dealloc_wait)
{ }

void Server::run()
{
        applicationRegister();

        for(;;) {
                IPCEvent* event = ipcEventProducer->eventWait();
                Flow *flow = nullptr;
                unsigned int port_id;
                DeallocateFlowResponseEvent *resp = nullptr;

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

        if (test_type == "ping")
                server_function = &Server::servePingFlow;
        else if (test_type == "perf")
                server_function = &Server::servePerfFlow;
        else {
                /* This should not happen. The error condition
                 * must be catched before this point. */
                LOG_ERR("Unkown test type %s", test_type.c_str());

                return;
        }

        thread t(server_function, this, flow);
        t.detach();
}

void Server::servePingFlow(Flow* flow)
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

void Server::servePerfFlow(Flow* flow)
{
        unsigned long int us;
        unsigned long tot_us = 0;
        char *buffer = new char[max_buffer_size];
        unsigned long pkt_cnt = 0;
        unsigned long bytes_cnt = 0;
        unsigned long tot_pkt = 0;
        unsigned long tot_bytes = 0;
        unsigned long interval_cnt = interval;  // counting down
        int sdu_size;
        struct sigevent event;
        struct itimerspec itime;
        timer_t timer_id;
        struct timespec last_timestamp;
        struct timespec now;

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
                clock_gettime(CLOCK_REALTIME, &last_timestamp);
                for(;;) {
                        sdu_size = flow->readSDU(buffer, max_buffer_size);
                        if (sdu_size <= 0) {
                                break;
                        }
                        pkt_cnt++;
                        bytes_cnt += sdu_size;

                        // Report periodic stats if needed
                        if (interval != -1 && --interval_cnt == 0) {
                                clock_gettime(CLOCK_REALTIME, &now);
                                us = timespec_diff_us(last_timestamp, now);
                                printPerfStats(pkt_cnt, bytes_cnt, us);

                                tot_pkt += pkt_cnt;
                                tot_bytes += bytes_cnt;
                                tot_us += us;

                                pkt_cnt = 0;
                                bytes_cnt = 0;

                                if (dw > 0)
                                        timer_settime(timer_id, 0, &itime, NULL);
                                clock_gettime(CLOCK_REALTIME, &last_timestamp);
                                interval_cnt = interval;
                        }
                }
        } catch(rina::IPCException e) {
                // This thread was blocked in the readSDU() function
                // when the flow gets deallocated
        }

        if  (timer_id) {
                timer_delete(timer_id);
        }

        /* When dealloc_wait is not set, we can safely add the remaining packets
         * to the total count */
        if (dw <= 0) {
                clock_gettime(CLOCK_REALTIME, &now);
                us = timespec_diff_us(last_timestamp, now);

                tot_pkt += pkt_cnt;
                tot_bytes += bytes_cnt;
                tot_us += us;
        } else {
                LOG_INFO("Discarded %lu SDUs", pkt_cnt);
        }

        LOG_INFO("Received %lu SDUs and %lu bytes in %lu us",
                        tot_pkt, tot_bytes, tot_us);
        LOG_INFO("Goodput: %.4f Kpps, %.4f Mbps",
                        static_cast<float>((tot_pkt * 1000.0)/tot_us),
                        static_cast<float>((tot_bytes * 8.0)/tot_us));

        delete [] buffer;
}

void Server::printPerfStats(unsigned long pkt, unsigned long bytes,
                unsigned long us)
{
        LOG_INFO("%lu SDUs and %lu bytes in %lu us => %.4f Mbps",
                        pkt, bytes, us, static_cast<float>((bytes * 8.0)/us));
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
