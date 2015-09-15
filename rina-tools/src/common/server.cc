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
			   Server * serv) : SimpleThread(threadAttributes)
{
	server = serv;
}

int ServerWorker::run()
{
        int result = internal_run();

        server->worker_completed(this);
        return result;
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

Server::Server(const string& dif_name,
               const string& app_name,
               const string& app_instance) :
        Application(dif_name, app_name, app_instance)
{
	ThreadAttributes threadAttrs;
	threadAttrs.setJoinable();
	cleaner = new ServerWorkerCleaner(&threadAttrs, this);
	cleaner->start();
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

void Server::run(bool blocking)
{
	applicationRegister();

        rina::FlowInformation flow;

        for(;;) {
                IPCEvent* event = ipcEventProducer->eventWait();
                int port_id = 0;
                DeallocateFlowResponseEvent *resp = NULL;

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
                        flow = ipcManager->allocateFlowResponse(*dynamic_cast<FlowRequestEvent*>(event), 0, true, blocking);
                        port_id = flow.portId;
                        LOG_INFO("New flow allocated [port-id = %d]", port_id);
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

void Server::startWorker(rina::FlowInformation flow)
{
	ServerWorker * worker = internal_start_worker(flow);
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
