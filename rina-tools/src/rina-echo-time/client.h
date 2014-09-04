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
           bool  time_flow_creation_,
           ulong echo_times_,
           bool  client_app_reg_,
           uint  data_size_,
           bool  debug_mes_);
    void run();
protected:
    void init();
    rina::Flow* makeConnection();
    void sendEcho(rina::Flow* flow);
    static  void printDuration(const std::chrono::high_resolution_clock::duration& dur);

private:
    std::string server_name;
    std::string server_instance;
    bool time_flow_creation;
    ulong echo_times;// -1 is infinit
    bool client_app_reg;
    uint data_size;
    bool debug_mes;

    static std::chrono::seconds wait_time;
  //threshold
    static std::chrono::seconds thres_s;
    static std::chrono::milliseconds thres_ms;
    static std::chrono::microseconds thres_us;
};
#endif//CLIENT_HPP
