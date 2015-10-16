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

#ifndef ET_SERVER_HPP
#define ET_SERVER_HPP

#include "server.h"

class EchoTimeServerWorker : public ServerWorker {
public:
	EchoTimeServerWorker(rina::ThreadAttributes * threadAttributes,
			     const std::string& test_type,
			     int port_id,
			     int deallocate_wait,
			     int inter,
			     unsigned int max_buffer_size,
			     Server * serv);
	~EchoTimeServerWorker() throw() { };
	int internal_run();

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
};

class EchoTimeServer: public Server
{
public:
	EchoTimeServer(const std::string& test_type,
		       const std::string& dif_name,
		       const std::string& app_name,
		       const std::string& app_instance,
		       const int perf_interval,
		       const int dealloc_wait);
        ~EchoTimeServer() { };

protected:
        ServerWorker * internal_start_worker(rina::FlowInformation flow);

private:
        std::string test_type;
        int interval;
        int dw;
};

#endif
