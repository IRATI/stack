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

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <chrono>
#include <librina/librina.h>

#include "application.h"


class Client: public Application
{
public:
    Client(const std::string& app_name_,
           const std::string& app_instance_,
           const std::string& server_name_,
           const std::string& server_instance_,
           bool  quiet_,
           ulong echo_times_,
           bool  client_app_reg_,
           uint  data_size_);
    void run();
protected:
    rina::Flow* makeConnection();
    void sendEcho(rina::Flow* flow);

private:
    std::string server_name;
    std::string server_instance;
    bool quiet;
    ulong echo_times;// -1 is infinit
    bool client_app_reg;
    uint data_size;
};
#endif//CLIENT_HPP
