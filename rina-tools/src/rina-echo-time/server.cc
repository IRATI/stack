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

#define RINA_PREFIX     "rina-echo-app"
#include <librina/logs.h>

#include "server.h"

using namespace std;
using namespace rina;

Server::Server(const string& t_type,
               const string& dif_name,
               const string& app_name,
               const string& app_instance) :
        Application(dif_name, app_name, app_instance),
        test_type(t_type)
{ }

void Server::run()
{
        applicationRegister();

        for(;;) {
                IPCEvent* event = ipcEventProducer->eventWait();
                Flow *flow = nullptr;
                unsigned int port_id;

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

        try {
                for(;;) {
                        int bytes_read = flow->readSDU(buffer,
                                                        max_buffer_size);
                        flow->writeSDU(buffer, bytes_read);
                }
        } catch(rina::IPCException e) {
                // This thread was blocked in the readSDU() function
                // when the flow gets dellocated
        }

        delete [] buffer;
}

void Server::servePerfFlow(Flow* flow)
{
        std::chrono::high_resolution_clock::time_point begin;
        std::chrono::high_resolution_clock::duration dur;
        unsigned long us;
        char *buffer = new char[max_buffer_size];
        unsigned long pkt_cnt = 0;
        unsigned long bytes_cnt = 0;
        int sdu_size;

        begin = std::chrono::high_resolution_clock::now();
        try {
                for(;;) {
                        sdu_size = flow->readSDU(buffer, max_buffer_size);
                        if (sdu_size <= 0) {
                                break;
                        }
                        pkt_cnt++;
                        bytes_cnt += sdu_size;
                }
        } catch(rina::IPCException e) {
                // This thread was blocked in the readSDU() function
                // when the flow gets dellocated
        }
        dur = std::chrono::high_resolution_clock::now() - begin;
        us = std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        LOG_INFO("Received %lu SDUs and %lu bytes in %lu us",
                        pkt_cnt, bytes_cnt, us);
        LOG_INFO("Goodput: %.4f Kpps, %.4f Mbps",
                        static_cast<float>((pkt_cnt * 1000.0)/us),
                        static_cast<float>((bytes_cnt * 8.0)/us));

        delete [] buffer;
}
