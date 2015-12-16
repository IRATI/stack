//
// Echo Client
//
// Addy Bombeke <addy.bombeke@ugent.be>
// Vincenzo Maffione <v.maffione@nextworks.it>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//   1. Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//   2. Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
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
               int w, int g, int dw, unsigned int lw, int rt) :
        Application(dif_nm, apn, api), test_type(t_type), dif_name(dif_nm),
        server_name(server_apn), server_instance(server_api),
        quiet(q), echo_times(count),
        client_app_reg(registration), data_size(size), wait(w), gap(g),
        dealloc_wait(dw), lost_wait(lw), nsdus(0), snd(0), m2(0),
        sdus_received(0), min_rtt(LONG_MAX), max_rtt(0), average_rtt(0), rate(rt)
{
        std::map<unsigned long, timespec> * tmp = new std::map<unsigned long, timespec>();
        m = *tmp;
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
        else if (test_type == "flood")
                floodFlow(port_id);
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

void Client::floodFlow(int port_id)
{
	unsigned long sdus_sent = 0;
	double variance = 0, stdev = 0;
	timespec endtp, begintp, mintp, mtp;
	unsigned long sn;
	double delta = 0;
	double current_rtt = 0;
	unsigned char *buffer2 = new unsigned char[data_size];

	ipcManager->setFlowOptsBlocking(port_id, true);
	snd = startSender(port_id);
	cout << "Sender started" << endl;
	while(true) {
		int bytes_read = 0;

		try {
			bytes_read = ipcManager->readSDU(port_id, buffer2, data_size);
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

		get_current_time(endtp);

		lock.lock();
		mtp = maxtp;
		lock.unlock();
		if (bytes_read == 0) {
			LOG_WARN("Returned 0 bytes, SDU considered lost");
			double mtime = time_difference_in_ms(mtp, endtp);
			lock.lock();
			sdus_sent = nsdus;
			lock.unlock();
			if (mtime > lost_wait && (sdus_sent == echo_times)) {
				cout << "Experiment finished: " << mtime << endl;
				break;
			}

			continue;
		}

		memcpy(&sn, buffer2, sizeof(sn));
		lock.lock();
		sdus_received++;
		if (m.find(sn) != m.end()) {
			begintp = m[sn];
			m.erase(sn);
		}
		lock.unlock();
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

		double mtime = time_difference_in_ms(mtp, endtp);
		lock.lock();
		sdus_sent = nsdus;
		lock.unlock();
		if (mtime > lost_wait ||
				(sdus_sent == echo_times && sdus_sent == sdus_received)) {
			break;
		}

	}

	lock.lock();
	sdus_sent = nsdus;
	lock.unlock();

	variance = m2/((double)sdus_received -1);
	stdev = sqrt(variance);

	unsigned long rt = 0;
	if (sdus_sent > 0) rt = ((sdus_sent - sdus_received)*100/sdus_sent);
	cout << "SDUs sent: "<< sdus_sent << "; SDUs received: " << sdus_received;
	cout << "; " << rt << "% SDU loss" <<endl;
	cout << "Minimum RTT: " << min_rtt << " ms; Maximum RTT: " << max_rtt
			<< " ms; Average RTT:" << average_rtt
			<< " ms; Standard deviation: " << stdev<<" ms"<<endl;

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
        if (snd) delete snd;
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

Sender * Client::startSender(int port_id)
{
        rina::ThreadAttributes threadAttributes;
        Sender * sender = new Sender(&threadAttributes,
                                     echo_times,
                                     data_size,
                                     dealloc_wait,
                                     lost_wait,
                                     rate,
                                     port_id,
                                     this);

        sender->start();
        sender->detach();

        return sender;
}

void Client::map_push(unsigned long sn, timespec tp)
{
	ScopedLock g(lock);
        m[sn] = tp;

}

void Client::set_sdus(unsigned long n)
{
	ScopedLock g(lock);
        nsdus = n;

}

void Client::set_maxTP(timespec tp)
{
	ScopedLock g(lock);
        maxtp = tp;
}

void Client::cancelFloodFlow(int port_id)
{
        try {
        	ipcManager->requestFlowDeallocation(port_id);
        } catch(rina::Exception &e) {
        	//Ignore, flow was already deallocated
        }
}

void Client::startCancelFloodFlowTask(int port_id)
{
	cflood_task = new CFloodCancelFlowTimerTask(port_id, this);
	timer.scheduleTask(cflood_task, lost_wait);
}

Client::~Client()
{
        unsigned long sdus_sent;

        lock.lock();
        sdus_sent = nsdus;
        lock.unlock();

        double variance = m2/((double)sdus_received -1);
        double stdev = sqrt(variance);

        unsigned long rt = 0;
        if (sdus_sent > 0) rt = ((sdus_sent - sdus_received)*100/sdus_sent);
        cout << "SDUs sent: "<< sdus_sent << "; SDUs received: " << sdus_received;
        cout << "; " << rt << "% SDU loss" <<endl;
        cout << "Minimum RTT: " << min_rtt << " ms; Maximum RTT: " << max_rtt
             << " ms; Average RTT:" << average_rtt
             << " ms; Standard deviation: " << stdev<<" ms"<<endl;
}

CFloodCancelFlowTimerTask::CFloodCancelFlowTimerTask(int pid, Client * cl)
{
	port_id = pid;
	client = cl;
}

void CFloodCancelFlowTimerTask::run()
{
	client->cancelFloodFlow(port_id);
}

Sender::Sender(rina::ThreadAttributes * threadAttributes,
                   unsigned long echo_times,
                   unsigned int data_size,
                   int dealloc_wait,
                   unsigned int lost_wait,
                   int rt,
                   int port_id,
                   Client * client) : SimpleThread(threadAttributes),
           echo_times(echo_times), data_size(data_size),
           dealloc_wait(dealloc_wait), lost_wait(lost_wait), rate(rt),
           port_id(port_id), client(client)
{
}

#ifndef MILLION
#define MILLION  1000000
#endif

#ifndef BILLION
#define BILLION  1000000000
#endif

void busy_wait_until(const struct timespec &deadline)
{
        timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        while (now.tv_sec < deadline.tv_sec)
                clock_gettime(CLOCK_REALTIME, &now);
        while (now.tv_sec == deadline.tv_sec && now.tv_nsec<deadline.tv_nsec)
                clock_gettime(CLOCK_REALTIME, &now);
}

/* add intv to t and store it in res*/
void ts_add(const timespec *t,
            const timespec *intv,
            timespec       *res)
{
        long nanos = 0;

        if (!(t && intv && res))
                return;

        nanos = t->tv_nsec + intv->tv_nsec;

        res->tv_sec = t->tv_sec + intv->tv_sec;
        while (nanos > BILLION) {
                nanos -= BILLION;
                ++(res->tv_sec);
        }
        res->tv_nsec = nanos;
}

int Sender::run(void)
{
	char buffer[data_size];
	unsigned int sdus_sent = 0;
	timespec begintp, endtp;
	timespec mintp, maxtp;
	unsigned char counter = 0;
	double interval_time = 0;
	bool out = false;

	if (rate) { /* wait in kB/s */
		interval_time = ((double) data_size) / ((double) rate); /* ms */
	}

	cout << "Experiment params" << endl;
	cout << "RATE: " << rate << endl;
	cout << "Interval: " << interval_time << endl;

	get_current_time(mintp);
	timespec next = mintp;
	for (unsigned long n = 0; n < echo_times; n++) {
		memcpy(buffer, &n, sizeof(n));

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

		client->map_push(n, begintp);
		sdus_sent++;
		get_current_time(maxtp);
		client->set_maxTP(maxtp);
		client->set_sdus(sdus_sent);
		if (rate) {
			long nanos = interval_time * MILLION;
			timespec interval = {nanos / BILLION, nanos % BILLION};
			ts_add(&next,&interval,&next);
			busy_wait_until(next);
		}
	}
	cout << "ENDED sending. SDUs sent: "<< sdus_sent << "; in TIME: " <<
			time_difference_in_ms(mintp, maxtp) << " ms" <<endl;

	client->startCancelFloodFlowTask(port_id);

	return 0;
}
