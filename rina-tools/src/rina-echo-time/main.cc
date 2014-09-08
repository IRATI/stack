//
// Echo time main
// 
// Addy Bombeke <addy.bombeke@ugent.be>
// Vincenzo Maffione <v.maffione@nextworks.it>
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

#include <cstdlib>
#include <string>

#include <librina/librina.h>

#define RINA_PREFIX     "rina-echo-time"
#include <librina/logs.h>

#include "tclap/CmdLine.h"

#include "config.h"
#include "client.h"
#include "server.h"

using namespace std;


int wrapped_main(int argc, char** argv)
{
        bool listen;
        bool registration;
        bool quiet;
        unsigned int count;
        unsigned int size;
        unsigned int wait;
        string server_apn = "rina.apps.echotime.server";
        string server_api = "1";
        string client_apn = "rina.apps.echotime.client";
        string client_api = "";

        try {
                TCLAP::CmdLine cmd("rina-echo-time", ' ', PACKAGE_VERSION);
                TCLAP::SwitchArg listen_arg("l",
                                            "listen",
                                            "Run in server mode",
                                            false);
                TCLAP::SwitchArg registration_arg("r",
                                                  "register",
                                                  "Register the application",
                                                  false);
                TCLAP::SwitchArg quiet_arg("q",
                                                  "quiet",
                                                  "Suppress some output",
                                                  false);
                TCLAP::ValueArg<unsigned int> count_arg("c",
                                                        "count",
                                                        "Number of packets to send",
                                                        false,
                                                        1,
                                                        "unsigned integer");
                TCLAP::ValueArg<unsigned int> size_arg("s",
                                                       "size",
                                                       "Size of the packets to send",
                                                       false,
                                                       20,
                                                       "unsigned integer");
                TCLAP::ValueArg<unsigned int> wait_arg("w",
                                                       "wait-time",
                                                       "Time to wait between two packets (ms)",
                                                       false,
                                                       1000,
                                                       "unsigned integer");

                cmd.add(listen_arg);
                cmd.add(count_arg);
                cmd.add(registration_arg);
                cmd.add(quiet_arg);
                cmd.add(size_arg);
                cmd.add(wait_arg);

                cmd.parse(argc, argv);

                listen = listen_arg.getValue();
                count = count_arg.getValue();
                registration = registration_arg.getValue();
                quiet = quiet_arg.getValue();
                size = size_arg.getValue();
                wait = wait_arg.getValue();

                if (size > Application::max_buffer_size) {
                        size = Application::max_buffer_size;
                        LOG_INFO("Packet size truncated to %u", size);
                }

        } catch (TCLAP::ArgException &e) {
                LOG_ERR("Error: %s for arg %d",
                        e.error().c_str(),
                        e.argId().c_str());
                return EXIT_FAILURE;
        }

        rina::initialize("INFO", "");

        if (listen) {
                // Server mode
                Server s(server_apn, server_api);

                s.run();
        } else {
                // Client mode
                Client c(client_apn, client_api, server_apn, server_api,
                         quiet, count, registration, size, wait);

                c.run();
        }

        return EXIT_SUCCESS;
}

int main(int argc, char * argv[])
{
        int retval;

        try {
                retval = wrapped_main(argc, argv);
        } catch (std::exception & e) {
                return EXIT_FAILURE;
        }

        return retval;
}
