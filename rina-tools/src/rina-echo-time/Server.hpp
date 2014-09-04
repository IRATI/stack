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

#include "Application.hpp"

class Server: public Application
{
public:
        Server(const std::string & app_name_,
               const std::string & app_instance_,
               bool                debug_mes_);

        void run();

protected:
        void init();
        void runFlow(rina::Flow * f);
private:
        bool debug_mes;
};

#endif
