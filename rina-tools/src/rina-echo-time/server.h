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

private:
	int port_id;
	ServerWorker * worker;
};

class ServerWorker : public rina::SimpleThread {
public:
	ServerWorker(rina::ThreadAttributes * threadAttributes,
		     const std::string& test_type,
		     int port_id,
		     int deallocate_wait,
		     int inter,
		     unsigned int max_buffer_size,
		     Server * serv);
	~ServerWorker() throw() { };
	void destroyFlow(int port_id);
	int run();

private:
        void servePingFlow(int port_id);
        void servePerfFlow(int port_id);
        void printPerfStats(unsigned long pkt,
        		    unsigned long bytes,
        		    unsigned long us);

        std::string test_type;
        int port_id;
        int dw;
        int interval;
        unsigned int max_buffer_size;
        rina::Timer timer;
        CancelFlowTimerTask * last_task;
        Server * server;
};

class ServerWorkerCleaner : public rina::SimpleThread {
public:
	ServerWorkerCleaner(rina::ThreadAttributes * threadAttributes,
			    Server * server);
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
        Server(const std::string& test_type,
               const std::string& dif_name,
               const std::string& app_name,
               const std::string& app_instance,
               const int perf_interval,
               const int dealloc_wait);
        ~Server();

        void run();
        void worker_completed(ServerWorker * worker);
        void remove_completed_workers();

private:
        void startWorker(int port_id);

        std::string test_type;
        int interval;
        int dw;
        rina::Lockable lock;
        std::list<ServerWorker *> active_workers;
        std::list<ServerWorker *> completed_workers;
        ServerWorkerCleaner * cleaner;
};

#endif
