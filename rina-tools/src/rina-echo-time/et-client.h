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
protected:
        int createFlow();
        void pingFlow(int port_id);
        void perfFlow(int port_id);
        void destroyFlow(int port_id);
        int readSDU(int portId, void * sdu, int maxBytes, unsigned int timout);

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
};
#endif//ET_CLIENT_HPP
