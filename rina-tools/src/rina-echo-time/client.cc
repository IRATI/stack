//
// Echo Client
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

#include <cstring>
#include <iostream>
#include <sstream>
#include <random>
#include <thread>

#define RINA_PREFIX     "rina-echo-time"
#include <librina/logs.h>

#include "client.h"

using namespace std;
using namespace rina;


static std::chrono::seconds      wait_time = std::chrono::seconds(1);
static std::chrono::seconds      thres_s   = std::chrono::seconds(9);
static std::chrono::milliseconds thres_ms  = std::chrono::milliseconds(9);
static std::chrono::microseconds thres_us  = std::chrono::microseconds(9);

static string
durationToString(const std::chrono::high_resolution_clock::duration&
                           dur)
{
        std::stringstream ss;

        if (dur > thres_s)
                ss << std::chrono::duration_cast<std::chrono::seconds>(dur).count() << "s";
        else if (dur > thres_ms)
                ss << std::chrono::duration_cast<std::chrono::milliseconds>
                        (dur).count() << "ms";
        else if (dur > thres_us)
                ss << std::chrono::duration_cast<std::chrono::microseconds>
                        (dur).count() << "us";
        else
                ss << std::chrono::duration_cast<std::chrono::nanoseconds>
                        (dur).count() << "ns";

        return ss.str();
}

Client::Client(const string& app_name_, const string& app_instance_,
               const string& server_name_, const string& server_instance_,
               bool quiet_, ulong echo_times_,
               bool client_app_reg_,
               uint data_size_) :
        Application(app_name_, app_instance_),
        server_name(server_name_), server_instance(server_instance_),
        quiet(quiet_), echo_times(echo_times_),
        client_app_reg(client_app_reg_), data_size(data_size_)
{ }

void Client::run()
{
        applicationRegister();

        Flow* flow = createFlow();

        if (flow)
                pingFlow(flow);
}

Flow* Client::createFlow()
{
        Flow* flow = nullptr;
        std::chrono::high_resolution_clock::time_point begintp =
                std::chrono::high_resolution_clock::now();
        AllocateFlowRequestResultEvent* afrrevent;
        FlowSpecification qosspec;
        IPCEvent* event;
        uint seqnum;

        seqnum = ipcManager->requestFlowAllocation(
                        ApplicationProcessNamingInformation(app_name, app_instance),
                        ApplicationProcessNamingInformation(server_name, server_instance),
                        qosspec);

        for (;;) {
                event = ipcEventProducer->eventWait();
                if (event && event->eventType == ALLOCATE_FLOW_REQUEST_RESULT_EVENT
                                && event->sequenceNumber == seqnum) {
                        break;
                }
                LOG_DBG("Client got new event %d", event->eventType);
        }

        afrrevent = reinterpret_cast<AllocateFlowRequestResultEvent*>(event);

        flow = ipcManager->commitPendingFlow(afrrevent->sequenceNumber,
                                              afrrevent->portId,
                                              afrrevent->difName);
        if (!flow || flow->getPortId() == -1) {
                LOG_ERR("Host not found");
        } else {
                LOG_DBG("[DEBUG] Port id = %d", flow->getPortId());
        }

        std::chrono::high_resolution_clock::time_point eindtp =
                std::chrono::high_resolution_clock::now();
        std::chrono::high_resolution_clock::duration dur = eindtp - begintp;

        if (!quiet) {
                cout << "Flow allocation time = " << durationToString(dur) << endl;
        }

        return flow;
}

void Client::pingFlow(Flow* flow)
{
        char buffer[max_buffer_size];
        char buffer2[max_buffer_size];
        ulong n = 0;
        random_device rd;
        default_random_engine ran(rd());
        uniform_int_distribution<int> dis(0, 255);

        while (flow) {
                IPCEvent* event = ipcEventProducer->eventPoll();

                if (event) {
                        switch(event->eventType) {
                        case FLOW_DEALLOCATED_EVENT:
                                flow = nullptr;
                                break;
                        default:
                                LOG_INFO("Client got new event %d", event->eventType);
                                break;
                        }
                } else if (n < echo_times) {
                                std::chrono::high_resolution_clock::time_point begintp, endtp;
                                int bytes_read = 0;

                                for (uint i = 0; i < data_size; i++) {
                                        buffer[i] = dis(ran);
                                }

                                try {
                                        begintp = std::chrono::high_resolution_clock::now();
                                        flow->writeSDU(buffer, data_size);
                                        bytes_read = flow->readSDU(buffer2, data_size);
                                        endtp = std::chrono::high_resolution_clock::now();
                                } catch (...) {
                                        LOG_ERR("SDU write/read failed");
                                }
                                cout << "SDU size = " << data_size << ", seq = " << n <<
                                        ", RTT = " << durationToString(endtp - begintp);
                                if (!((data_size == (uint) bytes_read) &&
                                      (memcmp(buffer, buffer2, data_size) == 0)))
                                        cout << " [bad response]";
                                cout << endl;

                                n++;
                                this_thread::sleep_for(wait_time);
                } else
                        flow = nullptr;
        }
}

