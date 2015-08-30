//
// Echo Client
//
// Addy Bombeke <addy.bombeke@ugent.be>
// Vincenzo Maffione <v.maffione@nextworks.it>
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
#include <cmath>
#include <iostream>
#include <sstream>
#include <cassert>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <iomanip>

#define RINA_PREFIX     "rina-echo-time"
#include <librina/ipc-api.h>
#include <librina/logs.h>

#include "et-client.h"

using namespace std;
using namespace rina;

void get_current_time(timespec& time) {
	clock_gettime(CLOCK_REALTIME, &time);
}

double time_difference_in_ms(timespec start, timespec end) {
	 double mtime, seconds, nseconds;

	 seconds  = (double)end.tv_sec  - (double)start.tv_sec;
	 nseconds = (double)end.tv_nsec - (double)start.tv_nsec;

	 mtime = ((seconds) * 1000 + nseconds/1000000.0);

	 return mtime;
}

Client::Client(const string& t_type,
               const string& dif_nm, const string& apn, const string& api,
               const string& server_apn, const string& server_api,
               bool q, unsigned long count,
               bool registration, unsigned int size,
               int w, int g, int dw, unsigned int lw) :
        Application(dif_nm, apn, api), test_type(t_type), dif_name(dif_nm),
        server_name(server_apn), server_instance(server_api),
        quiet(q), echo_times(count),
        client_app_reg(registration), data_size(size), wait(w), gap(g),
        dealloc_wait(dw), lost_wait(lw)
{
}

void Client::run()
{
	int port_id = 0;

	cout << setprecision(5);

        if (client_app_reg) {
                applicationRegister();
        }
        port_id = createFlow();
        if (port_id < 0) {
        	LOG_ERR("Problems creating flow, exiting");
        	return;
        }

        if (test_type == "ping")
        	pingFlow(port_id);
        else if (test_type == "perf")
        	perfFlow(port_id);
        else
        	LOG_ERR("Unknown test type '%s'", test_type.c_str());

        if (dealloc_wait > 0) {
        	sleep(dealloc_wait);
        }
        destroyFlow(port_id);
}

int Client::createFlow()
{
        timespec begintp, endtp;
        AllocateFlowRequestResultEvent* afrrevent;
        FlowSpecification qosspec;
        IPCEvent* event;
        uint seqnum;

        if (gap >= 0)
                qosspec.maxAllowableGap = gap;

        get_current_time(begintp);

        if (dif_name != string()) {
                seqnum = ipcManager->requestFlowAllocationInDIF(
                        ApplicationProcessNamingInformation(app_name, app_instance),
                        ApplicationProcessNamingInformation(server_name, server_instance),
                        ApplicationProcessNamingInformation(dif_name, string()),
                        qosspec);
        } else {
                seqnum = ipcManager->requestFlowAllocation(
                        ApplicationProcessNamingInformation(app_name, app_instance),
                        ApplicationProcessNamingInformation(server_name, server_instance),
                        qosspec);
        }

        for (;;) {
                event = ipcEventProducer->eventWait();
                if (event && event->eventType == ALLOCATE_FLOW_REQUEST_RESULT_EVENT
                                && event->sequenceNumber == seqnum) {
                        break;
                }
                LOG_DBG("Client got new event %d", event->eventType);
        }

        afrrevent = dynamic_cast<AllocateFlowRequestResultEvent*>(event);

        rina::FlowInformation flow = ipcManager->commitPendingFlow(afrrevent->sequenceNumber,
                                              	      	           afrrevent->portId,
                                              	      	           afrrevent->difName);
        if (flow.portId < 0) {
                LOG_ERR("Failed to allocate a flow");
                return flow.portId;
        } else {
                LOG_DBG("[DEBUG] Port id = %d", flow.portId);
        }

        get_current_time(endtp);

        if (!quiet) {
                cout << "Flow allocation time = "
                     << time_difference_in_ms(begintp, endtp) << " ms" << endl;
        }

        return flow.portId;
}

void Client::pingFlow(int port_id)
{
        unsigned char *buffer = new unsigned char[data_size];
        unsigned char *buffer2 = new unsigned char[data_size];
        unsigned int sdus_sent = 0;
        unsigned int sdus_received = 0;
        timespec begintp, endtp;
        unsigned char counter = 0;
        double min_rtt = LONG_MAX;
        double max_rtt = 0;
        double average_rtt = 0;
        double m2 = 0;
        double delta = 0;
        double variance = 0;
        double stdev = 0;
        double current_rtt = 0;

	ipcManager->setFlowOptsBlocking(port_id, false);

        for (unsigned long n = 0; n < echo_times; n++) {
        	int bytes_read = 0;

        	for (uint i = 0; i < data_size; i++) {
        		if (counter == 255) {
        			counter = 0;
        		}
        		buffer[i] = counter;
        		counter++;
        	}

        	get_current_time(begintp);

        	try {
        		ipcManager->writeSDU(port_id, buffer, data_size);
        	} catch (rina::FlowNotAllocatedException &e) {
        		LOG_ERR("Flow has been deallocated");
        		break;
        	} catch (rina::UnknownFlowException &e) {
        		LOG_ERR("Flow does not exist");
        		break;
        	} catch (rina::Exception &e) {
        		LOG_ERR("Problems writing SDU to flow, continuing");
        		continue;
        	}

        	sdus_sent ++;

        	try {
        		bytes_read = readSDU(port_id, buffer2, data_size, lost_wait);
        	} catch (rina::FlowAllocationException &e) {
        		LOG_ERR("Flow has been deallocated");
        		break;
        	} catch (rina::UnknownFlowException &e) {
        		LOG_ERR("Flow does not exist");
        		break;
        	} catch (rina::Exception &e) {
        		LOG_ERR("Problems reading SDU from flow, continuing");
        		continue;
        	}

        	if (bytes_read == 0) {
        		LOG_WARN("Timeout waiting for reply, SDU considered lost");
        		continue;
        	}

        	sdus_received ++;
        	get_current_time(endtp);
        	current_rtt = time_difference_in_ms(begintp, endtp);
        	if (current_rtt < min_rtt) {
        		min_rtt = current_rtt;
        	}
        	if (current_rtt > max_rtt) {
        		max_rtt = current_rtt;
        	}

        	delta = current_rtt - average_rtt;
        	average_rtt = average_rtt + delta/(double)sdus_received;
        	m2 = m2 + delta*(current_rtt - average_rtt);

        	cout << "SDU size = " << data_size << ", seq = " << n <<
        			", RTT = " << current_rtt << " ms";
        	if (!((data_size == (uint) bytes_read) &&
        			(memcmp(buffer, buffer2, data_size) == 0)))
        		cout << " [bad response]";
        	cout << endl;

		if (n < echo_times - 1) {
			sleep_wrapper.sleepForMili(wait);
		}
        }

        variance = m2/((double)sdus_received -1);
        stdev = sqrt(variance);

        cout << "SDUs sent: "<< sdus_sent << "; SDUs received: " << sdus_received;
        cout << "; " << ((sdus_sent - sdus_received)*100/sdus_sent) << "% SDU loss" <<endl;
        cout << "Minimum RTT: " << min_rtt << " ms; Maximum RTT: " << max_rtt
             << " ms; Average RTT:" << average_rtt
             << " ms; Standard deviation: " << stdev<<" ms"<<endl;

        delete [] buffer;
        delete [] buffer2;
}

void Client::perfFlow(int port_id)
{
        char *buffer;
        unsigned long n = 0;

        buffer = new char[data_size];
        if (!buffer) {
                LOG_ERR("Allocation of the send buffer failed");

                return;
        }

        for (unsigned int i = 0; i < data_size; i++) {
                buffer[i] = static_cast<char>(i * i);
        }

        while (n < echo_times) {
                ipcManager->writeSDU(port_id, buffer, data_size);
                n++;
        }

        delete [] buffer;
}

void Client::destroyFlow(int port_id)
{
        DeallocateFlowResponseEvent *resp = NULL;
        unsigned int seqnum;
        IPCEvent* event;

        seqnum = ipcManager->requestFlowDeallocation(port_id);

        for (;;) {
                event = ipcEventProducer->eventWait();
                if (event && event->eventType == DEALLOCATE_FLOW_RESPONSE_EVENT
                                && event->sequenceNumber == seqnum) {
                        break;
                }
                LOG_DBG("Client got new event %d", event->eventType);
        }
        resp = dynamic_cast<DeallocateFlowResponseEvent*>(event);
        assert(resp);

        ipcManager->flowDeallocationResult(port_id, resp->result == 0);
}

int Client::readSDU(int portId, void * sdu, int maxBytes, unsigned int timeout)
{
	timespec begintp, endtp;
	get_current_time(begintp);
	int bytes_read;

	while (true) {
		try {
			bytes_read = ipcManager->readSDU(portId, sdu, maxBytes);
			if (bytes_read == 0) {
				get_current_time(endtp);
				if ((unsigned int) time_difference_in_ms(begintp, endtp) > timeout) {
					break;
				}
			} else {
				break;
			}
		} catch (rina::Exception &e) {
			throw e;
		}
	}

	return bytes_read;
}
