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

#include <librina/librina.h>
#include <time.h>
#include <signal.h>

#include "application.h"

class Server: public Application
{
public:
        Server(const std::string& test_type,
               const std::string& dif_name,
               const std::string& app_name,
               const std::string& app_instance,
               const int perf_interval,
               const int dealloc_wait);

        void run();

protected:
        void servePingFlow(int port_id);
        void servePerfFlow(int port_id);
        static void destroyFlow(sigval_t val);

private:
        std::string test_type;
        int interval;
        int dw;
        void startWorker(int port_id);
        void printPerfStats(unsigned long pkt, unsigned long bytes,
                unsigned long us);
};

#endif
