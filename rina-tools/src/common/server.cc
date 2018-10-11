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

ServerWorker::ServerWorker(Server * serv)
	: SimpleThread(std::string("server-worker"), false)
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
        	ipcManager->deallocate_flow(port_id);
        } catch(rina::Exception &e) {
        	//Ignore, flow was already deallocated
        }
}

ServerWorkerCleaner::ServerWorkerCleaner(Server * s)
	: SimpleThread(std::string("server-worker-cleaner"), false)
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

Server::Server(const std::list<std::string>& dif_names,
               const string& app_name,
               const string& app_instance) :
        Application(dif_names, app_name, app_instance)
{
	cleaner = new ServerWorkerCleaner(this);
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

// Preform housekeeping on the worker
void Server::worker_started(ServerWorker * worker)
{
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
