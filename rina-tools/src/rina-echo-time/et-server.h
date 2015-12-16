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
        void serveFloodFlow(int port_id);
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
