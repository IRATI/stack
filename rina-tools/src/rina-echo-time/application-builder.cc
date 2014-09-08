//
// Echo Application Builder
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

#include <algorithm>
#include <unistd.h>
#include <librina/librina.h>

#include "client.h"
#include "server.h"
#include "application-builder.h"

ApplicationBuilder::ApplicationBuilder():
        client_name("rina.apps.echotime.client"), client_instance(""),
        server_name("rina.apps.echotime.server"), server_instance("1"),
        time_flow_creation(false), echo_times(-1), client_app_reg(false),
        data_size(64), server(false), help(false)
{ }

void ApplicationBuilder::configure(int argc, char** argv)
{
        char c;
        while((c = getopt(argc, argv, "lLxXrRfFhc:s:")) != -1) {
                switch(c) {
                case 'l':
                        server = true;
                        break;
                case 'L':
                        server = false;
                        break;
                case 'r':
                        client_app_reg = true;
                        break;
                case 'R':
                        client_app_reg = false;
                        break;
                case 'f':
                        time_flow_creation = true;
                        break;
                case 'F':
                        time_flow_creation = false;
                        break;
                        //			case 'h':
                        //				help = true;
                        //				break;
                case 'c':
                        echo_times = strtoul(optarg, nullptr, 0);
                        break;
                case 's':
                        data_size =
                                std::min<uint>(Application::max_buffer_size,
                                               static_cast<uint>(strtoul(optarg, nullptr, 0)));
                        break;
                default:
                        break;
                }
        }
        /*
          if (o.showhelp)
          {
          cout << "./rinaechotime [OPTIONS] [SERVER_INSTANCE]" << endl
          << "Options: \th\tshow help" << endl
          << "\t\tr\tregister the client" << endl
          << "\t\tx\tprint debug messages" << endl
          << "\t\tl\trun this as a server" << endl
          << "\t\tf\ttime flow creation" << endl;
          }
        */
}

void ApplicationBuilder::runApplication()
{
        if (server) {
                Server s(server_name, server_instance);

                s.run();
        } else {
                Client s(client_name, client_instance,
                         server_name, server_instance,
                         time_flow_creation, echo_times,
                         client_app_reg, data_size);

                s.run();
        }
}
