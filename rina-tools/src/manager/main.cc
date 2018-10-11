//
// Network Manager main
//
// Eduard Grasa <eduard.grasa@i2cat.net>
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

#define RINA_PREFIX     "net-manager"
#include <librina/logs.h>

#include "tclap/CmdLine.h"
#include "net-manager.h"

using namespace std;

void parse_dif_names(std::list<std::string> & dif_names, const std::string& arg)
{
	const char c = ',';
	std::string::size_type s = 0;
	std::string::size_type e = arg.find(c);

	while (e != std::string::npos) {
		dif_names.push_back(arg.substr(s, e-s));
		s = ++e;
		e = arg.find(c, e);
	}
	// Grab the remaining
	if (e == std::string::npos)
		dif_names.push_back(arg.substr(s, arg.length()));
}

NetworkManager * nm_instance;

int wrapped_main(int argc, char** argv)
{
        string manager_apn;
        string manager_api;
        list<string> dif_names;
        std::string console_path;
        std::string dif_templates_path;

        try {
                TCLAP::CmdLine cmd("network-manager", ' ', PACKAGE_VERSION);
                TCLAP::ValueArg<string> manager_apn_arg("",
                                                        "manager-apn",
                                                        "Application process name for the Network Manager process",
                                                        false,
                                                        "arcfire.network.manager",
                                                        "string");
                TCLAP::ValueArg<string> manager_api_arg("",
                                                        "manager-api",
                                                        "Application process instance for the Network Manager process",
                                                        false,
                                                        "1",
                                                        "string");
                TCLAP::ValueArg<string> dif_arg("d",
                                                "difs-to-register-at",
                                                "The names of the DIFs to register at, separated by ',' (empty means 'any DIF')",
                                                false,
                                                "",
                                                "string");

                TCLAP::ValueArg<string> console_path_arg("c",
                                                         "console-path",
							 "The path to the Network Management console UNIX socket",
							 false,
							 "/var/run/nmconsole.sock",
							 "string");

                TCLAP::ValueArg<string> dif_templates_path_arg("t",
                                                               "dif-templates-path",
							       "The path to the DIF Templates folder",
							       false,
							       "/usr/local/irati/etc",
							       "string");

                cmd.add(manager_apn_arg);
                cmd.add(manager_api_arg);
                cmd.add(dif_arg);
                cmd.add(console_path_arg);
                cmd.add(dif_templates_path_arg);

                cmd.parse(argc, argv);

                manager_apn = manager_apn_arg.getValue();
                manager_api = manager_api_arg.getValue();
                if (dif_arg.getValue() != "")
                	parse_dif_names(dif_names, dif_arg.getValue());
                console_path = console_path_arg.getValue();
                dif_templates_path = dif_templates_path_arg.getValue();

                LOG_DBG("Configuration: manager_apn=%s, manager_api=%s, console_path=%s, dif_templates_path=%s",
                	manager_apn.c_str(), manager_api.c_str(), console_path.c_str(),
			dif_templates_path.c_str());
        } catch (TCLAP::ArgException &e) {
                LOG_ERR("Error: %s for arg %d",
                        e.error().c_str(),
                        e.argId().c_str());
                return EXIT_FAILURE;
        }

        rina::initialize("DEBUG", "");

        nm_instance = new NetworkManager(manager_apn, manager_api, console_path,
        				 dif_templates_path);

        nm_instance->event_loop(dif_names);

	LOG_INFO("Exited event loop");
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
