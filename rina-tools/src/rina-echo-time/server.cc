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
#include <time.h>
#include <signal.h>
#include <time.h>

#define RINA_PREFIX     "rina-echo-app"
#include <librina/logs.h>
#include <librina/ipc-api.h>

#include "server.h"

using namespace std;
using namespace rina;

CancelFlowTimerTask::CancelFlowTimerTask(int port, ServerWorker * sw)
{
	port_id = port;
	worker = sw;
}

void CancelFlowTimerTask::run()
{
	worker->destroyFlow(port_id);
}

ServerWorker::ServerWorker(ThreadAttributes * threadAttributes,
			   const std::string& type,
			   int port,
			   int deallocate_wait,
			   int inter,
			   unsigned int max_buf_size,
			   Server * serv) : SimpleThread(threadAttributes)
{
	test_type = type;
	port_id = port;
	dw = deallocate_wait;
	interval = inter;
	max_buffer_size = max_buf_size;
	last_task = 0;
	server = serv;
}

int ServerWorker::run()
{
        if (test_type == "ping")
                servePingFlow(port_id);
        else if (test_type == "perf")
                servePerfFlow(port_id);
        else {
                /* This should not happen. The error condition
                 * must be catched before this point. */
                LOG_ERR("Unkown test type %s", test_type.c_str());

                return -1;
        }

        server->worker_completed(this);
        return 0;
}

void ServerWorker::servePingFlow(int port_id)
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

void ServerWorker::servePerfFlow(int port_id)
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

void ServerWorker::destroyFlow(int port_id)
{
        // TODO here we should store the seqnum (handle) returned by
        // requestFlowDeallocation() and match it in the event loop
        try {
        	ipcManager->requestFlowDeallocation(port_id);
        } catch(rina::Exception &e) {
        	//Ignore, flow was already deallocated
        }
}

void ServerWorker::printPerfStats(unsigned long pkt,
				  unsigned long bytes,
				  unsigned long us)
{
        LOG_INFO("%lu SDUs and %lu bytes in %lu us => %.4f Mbps",
                        pkt, bytes, us, static_cast<float>((bytes * 8.0)/us));
}

ServerWorkerCleaner::ServerWorkerCleaner(rina::ThreadAttributes * threadAttributes,
		    	    	         Server * s) : SimpleThread(threadAttributes)
{
	stop = false;
	server = s;
}

void ServerWorkerCleaner::do_stop()
{
	ScopedLock g(lock);
	stop = true;
}

int ServerWorkerCleaner::run()
{
	while (true) {
		lock.lock();
		if (stop) {
			lock.unlock();
			break;
		}

		server->remove_completed_workers();
		lock.unlock();
		sleep_wrapper.sleepForMili(1000);
	}

	return 0;
}

Server::Server(const string& t_type,
               const string& dif_name,
               const string& app_name,
               const string& app_instance,
               const int perf_interval,
               const int dealloc_wait) :
        Application(dif_name, app_name, app_instance),
        test_type(t_type), interval(perf_interval), dw(dealloc_wait)
{
	ThreadAttributes threadAttrs;
	threadAttrs.setJoinable();
	cleaner = new ServerWorkerCleaner(&threadAttrs, this);
}

Server::~Server()
{
	cleaner->do_stop();
	cleaner->join(NULL);
	lock.lock();
	if (completed_workers.size() > 0) {
		remove_completed_workers();
	}
	lock.unlock();

	delete cleaner;
}

void Server::run()
{
        applicationRegister();

        rina::FlowInformation flow;

        for(;;) {
                IPCEvent* event = ipcEventProducer->eventWait();
                int port_id = 0;
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
                        port_id = flow.portId;
                        LOG_INFO("New flow allocated [port-id = %d]", port_id);
                        startWorker(port_id);
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

void Server::startWorker(int port_id)
{
	ThreadAttributes threadAttributes;
        ServerWorker * worker = new ServerWorker(&threadAttributes,
        		    	    	         test_type,
        		    	    	         port_id,
        		    	    	         dw,
        		    	    	         interval,
        		    	    	         max_buffer_size,
        		    	    	         this);
        worker->detach();
        lock.lock();
        active_workers.push_back(worker);
        lock.unlock();
}

void Server::worker_completed(ServerWorker * worker)
{
	ScopedLock g(lock);
	active_workers.remove(worker);
	completed_workers.push_back(worker);
}

void Server::remove_completed_workers()
{
	ScopedLock g(lock);

	std::list<ServerWorker *>::iterator it = completed_workers.begin();
	while (it != completed_workers.end())
	{
            delete *it;
	    it = completed_workers.erase(it);
	    ++it;
	}
}
