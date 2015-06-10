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
#include "et-client.h"
#include "et-server.h"

using namespace std;


int wrapped_main(int argc, char** argv)
{
        bool listen;
        bool registration;
        bool quiet;
        unsigned int count;
        unsigned int size;
        int wait;
        int gap;
        int perf_interval;
        int dw;
        unsigned int lost_wait;
        string test_type;
        string server_apn;
        string server_api;
        string client_apn;
        string client_api;
        string dif_name;

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
                TCLAP::ValueArg<int> wait_arg("w",
                			      "wait-time",
                			      "Time to wait between two packets (ms)",
                			      false,
                			      1000,
                			      "integer");
                TCLAP::ValueArg<string> server_apn_arg("",
                                                       "server-apn",
                                                       "Application process name for the server",
                                                       false,
                                                       "rina.apps.echotime.server",
                                                       "string");
                TCLAP::ValueArg<string> server_api_arg("",
                                                       "server-api",
                                                       "Application process instance for the server",
                                                       false,
                                                       "1",
                                                       "string");
                TCLAP::ValueArg<string> client_apn_arg("",
                                                       "client-apn",
                                                       "Application process name for the client",
                                                       false,
                                                       "rina.apps.echotime.client",
                                                       "string");
                TCLAP::ValueArg<string> client_api_arg("",
                                                       "client-api",
                                                       "Application process instance for the client",
                                                       false,
                                                       "1",
                                                       "string");
                TCLAP::ValueArg<string> dif_arg("d",
                                                "dif-to-register-at",
                                                "The name of the DIF to register at (empty means 'any DIF')",
                                                false,
                                                "",
                                                "string");
                TCLAP::ValueArg<string> test_type_arg("t",
                                                      "test-type",
                                                      "Type of test (ping, perf)",
                                                      false,
                                                      "ping",
                                                      "string");
                TCLAP::ValueArg<int> gap_arg("g",
                                             "gap",
                                             "Gap of the retransmission window",
                                             false,
                                             -1,
                                             "integer");
                TCLAP::ValueArg<int> perf_interval_arg("",
                                             "perf-interval",
                                             "packet count interval to report perf results",
                                             false,
                                             -1,
                                             "integer");
                TCLAP::ValueArg<int> dealloc_wait_arg("",
                                             "dealloc-wait",
                                             "Deallocate the flow after specified timeout (s)",
                                             false,
                                             -1,
                                             "integer");
                TCLAP::ValueArg<unsigned int> lost_wait_arg("o",
                                                            "lost-wait",
                                                            "Time to wait (ms) for echo-reply before considering SDU lost",
                                                            false,
                                                            2000,
                                                            "unsigned integer");

                cmd.add(listen_arg);
                cmd.add(count_arg);
                cmd.add(registration_arg);
                cmd.add(quiet_arg);
                cmd.add(size_arg);
                cmd.add(wait_arg);
                cmd.add(server_apn_arg);
                cmd.add(server_api_arg);
                cmd.add(client_apn_arg);
                cmd.add(client_api_arg);
                cmd.add(dif_arg);
                cmd.add(test_type_arg);
                cmd.add(gap_arg);
                cmd.add(perf_interval_arg);
                cmd.add(dealloc_wait_arg);
                cmd.add(lost_wait_arg);

                cmd.parse(argc, argv);

                listen = listen_arg.getValue();
                count = count_arg.getValue();
                registration = registration_arg.getValue();
                quiet = quiet_arg.getValue();
                size = size_arg.getValue();
                wait = wait_arg.getValue();
                server_apn = server_apn_arg.getValue();
                server_api = server_api_arg.getValue();
                client_apn = client_apn_arg.getValue();
                client_api = client_api_arg.getValue();
                dif_name = dif_arg.getValue();
                test_type = test_type_arg.getValue();
                gap = gap_arg.getValue();
                perf_interval = perf_interval_arg.getValue();
                dw = dealloc_wait_arg.getValue();
                lost_wait = lost_wait_arg.getValue();

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
                EchoTimeServer s(test_type, dif_name, server_apn, server_api,
                                 perf_interval, dw);

                s.run(true);
        } else {
                // Client mode
                Client c(test_type, dif_name, client_apn, client_api,
                         server_apn, server_api, quiet, count,
                         registration, size, wait, gap, dw, lost_wait);

                c.run();
        }

        return EXIT_SUCCESS;
}

int main(int argc, char * argv[])
{
        int retval;

        try {
                retval = wrapped_main(argc, argv);
        } catch (rina::Exception& e) {
                LOG_ERR("%s", e.what());
                return EXIT_FAILURE;

        } catch (std::exception& e) {
                LOG_ERR("Uncaught exception");
                return EXIT_FAILURE;
        }

        return retval;
}
