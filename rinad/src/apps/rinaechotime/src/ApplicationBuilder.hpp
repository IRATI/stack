/*
* Echo Application Builder
*
* Addy Bombeke <addy.bombeke@ugent.be>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef APPLICATIONBUILDER_HPP
#define APPLICATIONBUILDER_HPP

#include <string>

class ApplicationBuilder
{
public:
    ApplicationBuilder();
    void configure(int argc, char** argv);
    void runApplication();
private:
    std::string client_name;
    std::string client_instance;
    std::string server_name;
    std::string server_instance;
    bool time_flow_creation;
    ulong echo_times;// -1 is infinit
    bool client_app_reg;
    uint data_size;
    bool debug_mes;
    bool server;
    bool help;
};

#endif//APPLICATIONBUILDER_HPP
