//
// Echo Client
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

#ifndef ET_CLIENT_HPP
#define ET_CLIENT_HPP

#include <string>
#include <librina/concurrency.h>
#include <librina/timer.h>

#include "application.h"


class Client;

class CFloodCancelFlowTimerTask : public rina::TimerTask {
public:
	CFloodCancelFlowTimerTask(int port_id, Client * client);
	void run();
	std::string name() const {
		return "cflood-cancel-flow";
	}

private:
	int port_id;
	Client * client;
};

class Sender: public rina::SimpleThread {
public:
        Sender(unsigned long echo_times,
                 unsigned int data_size,
                 int dealloc_wait,
                 unsigned int lost_wait,
                 int rate,
                 int port_id,
                 int fd,
                 Client * client);
        ~Sender() throw() {};
        int run();
private:
        unsigned long echo_times; // -1 is infinite
        unsigned int data_size;
        int dealloc_wait;
        unsigned int lost_wait;
        int rate;
        int port_id;
        int fd;
        Client * client;
};

class Client: public Application {
public:
        Client(const std::string& test_type,
               const std::list<std::string>& dif_name,
               const std::string& apn,
               const std::string& api,
               const std::string& server_apn,
               const std::string& server_api,
               bool  quiet,
               unsigned long count,
               bool  registration,
               unsigned int size,
               int wait,
               int g,
               int dw,
               unsigned int lw,
               int rt,
               unsigned int delay,
	       unsigned int loss);
       void run();
       int readTimeout(void * sdu, int maxBytes, unsigned int timout);
       void map_push(unsigned long sn, timespec tp);
       void set_sdus(unsigned long n);
       void set_maxTP(timespec tp);
       void cancelFloodFlow();
       void startCancelFloodFlowTask();
       ~Client();
protected:
        int createFlow();
        void pingFlow();
        void perfFlow();
        void floodFlow();
        void destroyFlow();

private:
        std::string test_type;
        std::string dif_name;
        std::string server_name;
        std::string server_instance;
        bool quiet;
        unsigned long echo_times; // -1 is infinite
        bool client_app_reg;
        unsigned int data_size;
        int wait;
        int gap;
        int dealloc_wait;
        unsigned int lost_wait;
        int rate;
        unsigned int delay;
        unsigned int loss;
        rina::Sleep sleep_wrapper;
        Sender * startSender();
        rina::Lockable lock;
        std::map<unsigned long, timespec> m;
        Sender * snd;
        unsigned long nsdus;
        double m2;
        unsigned long sdus_received;
        double min_rtt;
        double max_rtt;
        double average_rtt;
        timespec maxtp;
        CFloodCancelFlowTimerTask * cflood_task;
        int port_id;
        int fd;
        rina::Timer timer;
};
#endif//ET_CLIENT_HPP
