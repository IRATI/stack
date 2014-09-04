/*
* Echo Client
*
* Addy Bombeke <addy.bombeke@ugent.be>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "Client.hpp"

#include <cstring>
#include <iostream>
#include <random>
#include <thread>

using namespace std;
using namespace rina;

Client::Client(const string& app_name_, const string& app_instance_,
               const string& server_name_, const string& server_instance_,
               bool time_flow_creation_, ulong echo_times_, bool client_app_reg_,
               uint data_size_, bool debug_mes_): Application(app_name_, app_instance_),
    server_name(server_name_), server_instance(server_instance_),
    time_flow_creation(time_flow_creation_), echo_times(echo_times_),
    client_app_reg(client_app_reg_), data_size(data_size_), debug_mes(debug_mes_)
{
}
void Client::run()
{
    init();
    Flow* flow = makeConnection();
    if(flow)
        sendEcho(flow);
}

void Client::init()
{
    applicationRegister();
}

Flow* Client::makeConnection()
{
    Flow* flow = nullptr;
    std::chrono::high_resolution_clock::time_point begintp =
        std::chrono::high_resolution_clock::now();
    FlowSpecification qosspec;
#if 0
    uint handle = ipcManager->requestFlowAllocation(
                      ApplicationProcessNamingInformation(app_name,
                                                          app_instance),
                      ApplicationProcessNamingInformation(server_name,
                                                          server_instance),
                      qosspec);
#endif
    IPCEvent* event = ipcEventProducer->eventWait();
    while(event && event->eventType != ALLOCATE_FLOW_REQUEST_RESULT_EVENT) {
        if(debug_mes)
            cerr << "[DEBUG] Client got new event " << event->eventType << endl;
        event = ipcEventProducer->eventWait();//todo this can make us wait forever
    }
    AllocateFlowRequestResultEvent* afrrevent =
        reinterpret_cast<AllocateFlowRequestResultEvent*>(event);
    Flow* hulpflow = ipcManager->commitPendingFlow(afrrevent->sequenceNumber,
                                                   afrrevent->portId,
                                                   afrrevent->difName);
    if(hulpflow->getPortId() == -1) {
        cerr << "Host not found" << endl;
    } else {
        flow = hulpflow;
    }
    std::chrono::high_resolution_clock::time_point eindtp =
        std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::duration dur = eindtp - begintp;
    if(time_flow_creation) {
        cout << "flow_creation_time=";
        printDuration(dur);
        cout << endl;
    }
    return flow;
}
void Client::sendEcho(Flow* flow)
{
    char buffer[max_buffer_size];
    char buffer2[max_buffer_size];
    ulong n = 0;
    ulong c = echo_times;
    random_device rd;
    default_random_engine ran(rd());
    uniform_int_distribution<int> dis(0, 255);
    while(flow) {
        IPCEvent* event = ipcEventProducer->eventPoll();
        if(event) {
            switch(event->eventType) {
            case FLOW_DEALLOCATED_EVENT:
                flow = nullptr;
                break;
            default:
                if(debug_mes)
                        cerr << "[DEBUG] Client got new event " << event->eventType << endl;
                break;
            }
        } else {
            if(c != 0) {
                for(uint i = 0; i < data_size; ++i)
                    buffer[i] = dis(ran);
                std::chrono::high_resolution_clock::time_point begintp =
                    std::chrono::high_resolution_clock::now();
                int bytesreaded = 0;
                try {
                    flow->writeSDU(buffer, data_size);
                    bytesreaded = flow->readSDU(buffer2, data_size);
                } catch(...) {
                    cerr << "flow I/O fail" << endl;
                }
                std::chrono::high_resolution_clock::time_point eindtp =
                    std::chrono::high_resolution_clock::now();
                std::chrono::high_resolution_clock::duration dur = eindtp - begintp;
                cout << "sdu_size=" << data_size << " seq=" << n << " time=";
                printDuration(dur);
                if (!((data_size == (uint) bytesreaded) &&
                     (memcmp(buffer, buffer2, data_size) == 0)))
                    cout << " bad check";
                cout << endl;
                n++;
                if(c != static_cast<ulong>(-1))
                    c--;
                this_thread::sleep_for(wait_time);
            } else
                flow = nullptr;
        }
    }
}


void Client::printDuration(const std::chrono::high_resolution_clock::duration&
                           dur)
{
    if(dur > thres_s)
        cout << std::chrono::duration_cast<std::chrono::seconds>(dur).count() << "s";
    else if(dur > thres_ms)
        cout << std::chrono::duration_cast<std::chrono::milliseconds>
             (dur).count() << "ms";
    else if(dur > thres_us)
        cout << std::chrono::duration_cast<std::chrono::microseconds>
             (dur).count() << "us";
    else
        cout << std::chrono::duration_cast<std::chrono::nanoseconds>
             (dur).count() << "ns";

}

std::chrono::seconds Client::wait_time = std::chrono::seconds(1);
std::chrono::seconds Client::thres_s = std::chrono::seconds(9);
std::chrono::milliseconds Client::thres_ms = std::chrono::milliseconds(9);
std::chrono::microseconds Client::thres_us = std::chrono::microseconds(9);
