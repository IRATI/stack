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

#ifndef SERVER_HPP
#define SERVER_HPP

#include <librina/concurrency.h>
#include <librina/timer.h>
#include "application.h"

class ServerWorker;
class Server;

class CancelFlowTimerTask : public rina::TimerTask {
public:
	CancelFlowTimerTask(int port_id, ServerWorker * sw);
	void run();
	std::string name() const {
		return "cancel-flow";
	}

private:
	int port_id;
	ServerWorker * worker;
};

class ServerWorker : public rina::SimpleThread {
public:
	ServerWorker(Server * serv);
	virtual ~ServerWorker() throw() { };
	void destroyFlow(int port_id);
	int run();
	virtual int internal_run() = 0;

protected:
        Server * server;
};

class ServerWorkerCleaner : public rina::SimpleThread {
public:
	ServerWorkerCleaner(Server * server);
	~ServerWorkerCleaner() throw() { };
	void do_stop();
	int run();
private:
	bool stop;
	rina::Sleep sleep_wrapper;
	Server * server;
	rina::Lockable lock;
};

class Server: public Application
{
public:
        Server(const std::list<std::string>& dif_names,
               const std::string& app_name,
               const std::string& app_instance);
        virtual ~Server();

        void run(bool blocking);
				void worker_completed(ServerWorker * worker);
				void worker_started(ServerWorker * worker);
        void remove_completed_workers();

protected:
	void startWorker(rina::FlowInformation flow);
        virtual ServerWorker * internal_start_worker(rina::FlowInformation flow) = 0;

        rina::Lockable lock;
        std::list<ServerWorker *> active_workers;
        std::list<ServerWorker *> completed_workers;
        ServerWorkerCleaner * cleaner;
};

#endif
