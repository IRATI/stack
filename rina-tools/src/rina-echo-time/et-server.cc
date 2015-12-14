//
// Echo Server
//
// Addy Bombeke <addy.bombeke@ugent.be>
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

#include <iostream>
#include <time.h>
#include <signal.h>
#include <time.h>

#define RINA_PREFIX     "rina-echo-app"
#include <librina/logs.h>
#include <librina/ipc-api.h>

#include "et-server.h"

using namespace std;
using namespace rina;

EchoTimeServerWorker::EchoTimeServerWorker(ThreadAttributes * threadAttributes,
					   const std::string& type,
			   	   	   int port,
			   	   	   int deallocate_wait,
			   	   	   int inter,
			   	   	   unsigned int max_buf_size,
			   	   	   Server * serv) : ServerWorker(threadAttributes, serv)
{
	test_type = type;
	port_id = port;
	dw = deallocate_wait;
	interval = inter;
	max_buffer_size = max_buf_size;
	last_task = 0;
}

int EchoTimeServerWorker::internal_run()
{
        if (test_type == "ping")
                servePingFlow(port_id);
        else if (test_type == "perf")
                servePerfFlow(port_id);
        else if (test_type == "flood")
                serveFloodFlow(port_id);
        else {
                /* This should not happen. The error condition
                 * must be catched before this point. */
                LOG_ERR("Unkown test type %s", test_type.c_str());

                return -1;
        }

        return 0;
}

void EchoTimeServerWorker::servePingFlow(int port_id)
{
        char *buffer = new char[max_buffer_size];

        // Setup a timer if dealloc_wait option is set */
        if (dw > 0) {
        	last_task = new CancelFlowTimerTask(port_id, this);
        	timer.scheduleTask(last_task, dw);
        }

        try {
                for(;;) {
                        int bytes_read = ipcManager->readSDU(port_id,
                        				     buffer,
                                                             max_buffer_size);
                        ipcManager->writeSDU(port_id, buffer, bytes_read);
                        if (dw > 0 && last_task) {
                                timer.cancelTask(last_task);
                        	last_task = new CancelFlowTimerTask(port_id, this);
                        	timer.scheduleTask(last_task, dw);
                        }
                }
        } catch(rina::IPCException &e) {
                // This thread was blocked in the readSDU() function
                // when the flow gets deallocated
        }

        if  (dw > 0 && last_task) {
        	timer.cancelTask(last_task);
        }

        delete [] buffer;
}

void EchoTimeServerWorker::serveFloodFlow(int port_id)
{
        char *buffer = new char[max_buffer_size];

        // Setup a timer if dealloc_wait option is set */
        if (dw > 0) {
                last_task = new CancelFlowTimerTask(port_id, this);
                timer.scheduleTask(last_task, dw);
        }

        try {
                for(;;) {
                        int bytes_read = ipcManager->readSDU(port_id,
                                                             buffer,
                                                             max_buffer_size);

                        ipcManager->writeSDU(port_id, buffer, bytes_read);
                        if (dw > 0 && last_task) {
                                timer.cancelTask(last_task);
                                last_task = new CancelFlowTimerTask(port_id, this);
                                timer.scheduleTask(last_task, dw);
                        }
                }
        } catch(rina::IPCException &e) {
                // This thread was blocked in the readSDU() function
                // when the flow gets deallocated
        }

        if  (dw > 0 && last_task) {
                timer.cancelTask(last_task);
        }

        delete [] buffer;
}

static unsigned long
timespec_diff_us(const struct timespec& before, const struct timespec& later)
{
        return ((later.tv_sec - before.tv_sec) * 1000000000 +
                        (later.tv_nsec - before.tv_nsec))/1000;
}

void EchoTimeServerWorker::servePerfFlow(int port_id)
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
        struct timespec last_timestamp;
        struct timespec now;

        // Setup a timer if dealloc_wait option is set */
        if (dw > 0) {
        	last_task = new CancelFlowTimerTask(port_id, this);
        	timer.scheduleTask(last_task, dw);
        }

        try {
                clock_gettime(CLOCK_REALTIME, &last_timestamp);
                for(;;) {
                        sdu_size = ipcManager->readSDU(port_id, buffer, max_buffer_size);
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

                                if (dw > 0 && last_task) {
                                        timer.cancelTask(last_task);
                                	last_task = new CancelFlowTimerTask(port_id, this);
                                	timer.scheduleTask(last_task, dw);
                                }
                                clock_gettime(CLOCK_REALTIME, &last_timestamp);
                                interval_cnt = interval;
                        }
                }
        } catch(rina::IPCException &e) {
                // This thread was blocked in the readSDU() function
                // when the flow gets deallocated
        }

        if  (dw > 0 && last_task) {
        	timer.cancelTask(last_task);
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

void EchoTimeServerWorker::printPerfStats(unsigned long pkt,
					  unsigned long bytes,
					  unsigned long us)
{
        LOG_INFO("%lu SDUs and %lu bytes in %lu us => %.4f Mbps",
                        pkt, bytes, us, static_cast<float>((bytes * 8.0)/us));
}

EchoTimeServer::EchoTimeServer(const string& t_type,
			       const string& dif_name,
			       const string& app_name,
			       const string& app_instance,
			       const int perf_interval,
			       const int dealloc_wait) :
        		Server(dif_name, app_name, app_instance),
        test_type(t_type), interval(perf_interval), dw(dealloc_wait)
{
}

ServerWorker * EchoTimeServer::internal_start_worker(rina::FlowInformation flow)
{
	ThreadAttributes threadAttributes;
        EchoTimeServerWorker * worker = new EchoTimeServerWorker(&threadAttributes,
        		    	    	         	 	 test_type,
        		    	    	         	 	 flow.portId,
        		    	    	         	 	 dw,
        		    	    	         	 	 interval,
        		    	    	         	 	 max_buffer_size,
        		    	    	         	 	 this);
        worker->start();
        worker->detach();
        return worker;
}
