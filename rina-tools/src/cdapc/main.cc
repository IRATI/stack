//
// RINA CDAP connector C++  main
//
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

#define RINA_PREFIX     "cdap-connector"
#include <librina/logs.h>

#include "tclap/CmdLine.h"

#include "config.h"
#include "connector.h"
#include "utils.h"

using namespace std;

int main(int argc, char** argv)
{
        string manager_apn;
        string manager_api;
        string manager_ws;
        list<string> dif_names;

        try {
                TCLAP::CmdLine cmd("cdapc", ' ', PACKAGE_VERSION);
                TCLAP::ValueArg<string> manager_apn_arg("n",
                                                       "manager-apn",
                                                       "Application process name for the manager",
                                                       false,
                                                       "rina.apps.manager",
                                                       "string");
                TCLAP::ValueArg<string> manager_api_arg("i",
                                                       "manager-api",
                                                       "Application process instance for the manager",
                                                       false,
                                                       "1",
                                                       "string");
                TCLAP::ValueArg<string> dif_arg("d",
                                		"difs-to-register-at",
						"The names of the DIFs to register at, separated by ',' (empty means 'any DIF')",
                                                false,
                                                "NMS.DIF",
                                                "string");
                // WS address
                TCLAP::ValueArg<string> ws_arg("",
                  "manager-ws",
                  "The ws URL to register at (empty means 'no ws connection')",
                  false,
                  "ws://localhost:8887",
                  "string");

                TCLAP::SwitchArg nows_arg("",
                  "no-ws",
                  "No ws connection is attempted",
                  false);
                TCLAP::SwitchArg norina_arg("",
                  "no-rina",
                  "No rina connection is attempted",
                  false);

                cmd.add(manager_apn_arg);
                cmd.add(dif_arg);
                cmd.add(ws_arg);
                cmd.add(nows_arg);
                cmd.add(norina_arg);

                cmd.parse(argc, argv);

                manager_apn = manager_apn_arg.getValue();
                manager_api = manager_api_arg.getValue();
                parse_dif_names(dif_names, dif_arg.getValue());
                manager_ws  = ws_arg.getValue();
                
                // no args override other settings
                if (nows_arg.getValue()) {
                  manager_ws = string(); // empty it
                }
                if (norina_arg.getValue()) {
                  manager_apn = string(); // empty it
                }
                
                cout << "Register [" << manager_apn.c_str() << "," << manager_api.c_str() << "] at [" << dif_arg.getValue() << "]" << endl;

        } catch (TCLAP::ArgException &e) {
                LOG_ERR("Error: %s for arg %d",
                        e.error().c_str(),
                        e.argId().c_str());
                return EXIT_FAILURE;
        }

        Connector m(dif_names, manager_apn, manager_api, manager_ws);
        m.run();

        return EXIT_SUCCESS;
}
