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

        /* Timer setup if dealloc_wait is set */
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

        delete [] buffer;
}

void Server::servePerfFlow(Flow* flow)
{
        std::chrono::high_resolution_clock::time_point begin;
        std::chrono::high_resolution_clock::duration dur;
        unsigned long us;
        unsigned long tot_us = 0;
        char *buffer = new char[max_buffer_size];
        unsigned long pkt_cnt = 0;
        unsigned long bytes_cnt = 0;
        unsigned long tot_pkt = 0;
        unsigned long tot_bytes = 0;
        int sdu_size;
        struct sigevent event;
        struct itimerspec itime;
        timer_t timer_id;

        /* Timer setup if dealloc_wait is set */
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
                begin = std::chrono::high_resolution_clock::now();
                for(;;) {
                        sdu_size = flow->readSDU(buffer, max_buffer_size);
                        if (sdu_size <= 0) {
                                break;
                        }
                        pkt_cnt++;
                        bytes_cnt += sdu_size;

                        /* Report stats if interval is set */
                        if (interval != -1 && pkt_cnt%interval == 0) {
                                dur = std::chrono::high_resolution_clock::now() - begin;
                                us = std::chrono::duration_cast<std::chrono::microseconds>(dur).count();
                                printPerfStats(pkt_cnt, bytes_cnt, us);

                                tot_pkt += pkt_cnt;
                                tot_bytes += bytes_cnt;
                                tot_us += us;

                                pkt_cnt = 0;
                                bytes_cnt = 0;

                                if (dw > 0)
                                        timer_settime(timer_id, 0, &itime, NULL);
                                begin = std::chrono::high_resolution_clock::now();
                        }
                }
        } catch(rina::IPCException e) {
                // This thread was blocked in the readSDU() function
                // when the flow gets deallocated
        }

        /* When dealloc_wait is not set, we can safely add the remaining packets
         * to the total count */
        if (dw <= 0) {
                dur = std::chrono::high_resolution_clock::now() - begin;
                us = std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

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
        //unsigned int seqnum;

        if (flow->getState() != FlowState::FLOW_ALLOCATED)
                return;

        int port_id = flow->getPortId();

        ipcManager->requestFlowDeallocation(port_id);
        //seqnum = ipcManager->requestFlowDeallocation(port_id);
}
