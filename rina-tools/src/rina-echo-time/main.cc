//
// Echo time main
//
// Addy Bombeke <addy.bombeke@ugent.be>
// Vincenzo Maffione <v.maffione@nextworks.it>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//   1. Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//   2. Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
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
#include "utils.h"

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
        int rate;
        unsigned int delay;
        unsigned int loss;
        unsigned int lost_wait;
        unsigned int partial_read;
        string test_type;
        string server_apn;
        string server_api;
        string client_apn;
        string client_api;
        list<string> dif_names;

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
                                                "difs-to-register-at",
                                                "The names of the DIFs to register at, separated by ',' (empty means 'any DIF')",
                                                false,
                                                "",
                                                "string");
                TCLAP::ValueArg<string> test_type_arg("t",
                                                      "test-type",
                                                      "Type of test (ping, perf, flood)",
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
                TCLAP::ValueArg<int> rate_arg("",
                                             "rate",
                                             "In flood mode, rate at which the data must be sent in kBps, default = 1000 kBps",
                                             false,
                                             1000,
                                             "integer");
                TCLAP::ValueArg<unsigned int> delay_arg("",
                                             "delay",
                                             "Maximum delay. It should be a value greater than zero",
                                             false,
                                             0,
                                             "unsigned integer");
                TCLAP::ValueArg<unsigned int> loss_arg("",
                                             "loss",
                                             "Maximum loss probability (1/10000). It should be a value greater than zero",
                                             false,
                                             10000,
                                             "unsigned integer");
                TCLAP::ValueArg<unsigned int> partial_read_arg("",
                                            	  	       "partial-read",
							       "Server reads SDUs byte per byte (only in ping tests)",
							       false,
							       0,
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
                cmd.add(rate_arg);
                cmd.add(delay_arg);
                cmd.add(loss_arg);
                cmd.add(partial_read_arg);

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
                parse_dif_names(dif_names, dif_arg.getValue());
                test_type = test_type_arg.getValue();
                gap = gap_arg.getValue();
                perf_interval = perf_interval_arg.getValue();
                dw = dealloc_wait_arg.getValue();
                lost_wait = lost_wait_arg.getValue();
                rate = rate_arg.getValue();
                delay = delay_arg.getValue();
                loss = loss_arg.getValue();

                partial_read = partial_read_arg.getValue();

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
                EchoTimeServer s(test_type, dif_names, server_apn, server_api,
                                 perf_interval, dw, partial_read);

                s.run(true);
        } else {
                // Client mode
                Client c(test_type, dif_names, client_apn, client_api,
                         server_apn, server_api, quiet, count,
                         registration, size, wait, gap, dw, lost_wait, rate, delay, loss);

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
