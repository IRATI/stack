//
// Echo Client
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

#ifndef ET_CLIENT_HPP
#define ET_CLIENT_HPP

#include <string>
#include <librina/concurrency.h>

#include "application.h"


class Client;

class Sender: public rina::SimpleThread {
public:
        Sender(rina::ThreadAttributes * threadAttributes,
                 unsigned long echo_times,
                 unsigned int data_size,
                 int dealloc_wait,
                 unsigned int lost_wait,
                 int wait,
                 int port_id,
                 Client * client);
        ~Sender() throw() {};
        int run();
private:
        unsigned long echo_times; // -1 is infinite
        unsigned int data_size;
        int dealloc_wait;
        unsigned int lost_wait;
        int wait;
        int port_id;
        Client * client;
};

class Client: public Application {
public:
        Client(const std::string& test_type,
               const std::string& dif_name,
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
               unsigned int lw);
       void run();
       int readSDU(int portId, void * sdu, int maxBytes, unsigned int timout);
       void map_push(unsigned long sn, timespec tp);
       void set_sdus(unsigned long n);
       void set_maxTP(timespec tp);
       ~Client();
protected:
        int createFlow();
        void pingFlow(int port_id);
        void perfFlow(int port_id);
        void nonstopFlow(int port_id);
        void destroyFlow(int port_id);

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
        rina::Sleep sleep_wrapper;
        Sender * startSender(int port_id);
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
};
#endif//ET_CLIENT_HPP
