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
	EchoTimeServerWorker(const std::string& test_type,
			     int port_id, int fd,
			     int deallocate_wait,
			     int inter,
			     unsigned int max_buffer_size,
			     unsigned int pr,
			     Server * serv);
	~EchoTimeServerWorker() throw() { };
	int internal_run();

private:
        void servePingFlow();
        void servePerfFlow();
        void serveFloodFlow();
        void printPerfStats(unsigned long pkt,
        		    unsigned long bytes,
        		    unsigned long us);
        int partial_read_bytes(int fd,
        		       char * buffer,
			       int bytes_to_read);

        std::string test_type;
        int port_id;
        int fd;
        int dw;
        int interval;
        unsigned int max_buffer_size;
        unsigned int partial_read;
        rina::Timer timer;
        CancelFlowTimerTask * last_task;
};

class EchoTimeServer: public Server
{
public:
	EchoTimeServer(const std::string& test_type,
		       const std::list<std::string>& dif_names,
		       const std::string& app_name,
		       const std::string& app_instance,
		       const int perf_interval,
		       const int dealloc_wait,
		       unsigned int partial_read);
        ~EchoTimeServer() { };

protected:
        ServerWorker * internal_start_worker(rina::FlowInformation flow);

private:
        std::string test_type;
        int interval;
        int dw;
        unsigned int partial_read;
};

#endif
