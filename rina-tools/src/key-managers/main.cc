//
// Key Managers main
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

#define RINA_PREFIX     "key-manager"
#include <librina/logs.h>

#include "tclap/CmdLine.h"

#include "config.h"
#include "km-common.h"
#include "utils.h"

using namespace std;

int wrapped_main(int argc, char** argv)
{
        bool central;
        bool quiet;
        string creds_folder;
        string central_apn;
        string central_api;
        string kma_apn;
        string kma_api;
        list<string> dif_names;
        AbstractKM * km_instance = 0;

        try {
                TCLAP::CmdLine cmd("key-manager", ' ', PACKAGE_VERSION);
                TCLAP::SwitchArg central_arg("c",
                                             "central",
                                             "Run as the Central Key Manager",
                                             false);
                TCLAP::SwitchArg quiet_arg("q",
                                           "quiet",
                                           "Suppress some output",
                                           false);
                TCLAP::ValueArg<string> central_apn_arg("",
                                                        "central-apn",
                                                        "Application process name for the Central Key Manager",
                                                        false,
                                                        "pristine.central.key.manager",
                                                        "string");
                TCLAP::ValueArg<string> central_api_arg("",
                                                        "central-api",
                                                        "Application process instance for the Central Key Manager",
                                                        false,
                                                        "1",
                                                        "string");
                TCLAP::ValueArg<string> kma_apn_arg("",
                                                    "local-kma-apn",
                                                    "Application process name for the local Key Management Agent",
                                                    false,
                                                    "pristine.local.key.ma",
                                                    "string");
                TCLAP::ValueArg<string> kma_api_arg("",
                                                    "local-kma-api",
                                                    "Application process instance for the local Key Management Agent",
                                                    false,
                                                    "1",
                                                    "string");
                TCLAP::ValueArg<string> dif_arg("d",
                                                "difs-to-register-at",
                                                "The names of the DIFs to register at, separated by ',' (empty means 'any DIF')",
                                                false,
                                                "",
                                                "string");
                TCLAP::ValueArg<string> creds_folder_arg("",
                                                         "creds_folder",
                                                         "Folder where the credentials are stored",
                                                         false,
                                                         "/usr/local/irati/key-manager/creds",
                                                         "string");

                cmd.add(central_arg);
                cmd.add(quiet_arg);
                cmd.add(central_apn_arg);
                cmd.add(central_api_arg);
                cmd.add(kma_apn_arg);
                cmd.add(kma_api_arg);
                cmd.add(dif_arg);
                cmd.add(creds_folder_arg);

                cmd.parse(argc, argv);

                central = central_arg.getValue();
                quiet = quiet_arg.getValue();
                central_apn = central_apn_arg.getValue();
                central_api = central_api_arg.getValue();
                kma_apn = kma_apn_arg.getValue();
                kma_api = kma_api_arg.getValue();
                creds_folder = creds_folder_arg.getValue();
                parse_dif_names(dif_names, dif_arg.getValue());

        } catch (TCLAP::ArgException &e) {
                LOG_ERR("Error: %s for arg %d",
                        e.error().c_str(),
                        e.argId().c_str());
                return EXIT_FAILURE;
        }

        rina::initialize("DEBUG", "");

        if (central) {
                // Central Key Manager
        	km_instance = KMFactory::create_central_key_manager(dif_names,
        							    central_apn,
								    central_api,
								    creds_folder);
        } else {
                // Key Management Agent
        	km_instance = KMFactory::create_key_management_agent(creds_folder,
        			       	       	       	       	     dif_names,
								     kma_apn,
								     kma_api,
								     central_apn,
								     central_api,
								     quiet);
        }

        if (!km_instance) {
        	LOG_ERR("Problems creating KM instance, exiting...");
        	return EXIT_SUCCESS;
        }

	try {
		km_instance->event_loop();
	} catch (rina::Exception &e) {
		LOG_ERR("Problems running event loop: %s", e.what());
	} catch (std::exception &e1) {
		LOG_ERR("Problems running event loop: %s", e1.what());
	} catch (...) {
		LOG_ERR("Unhandled exception!!!");
	}

	delete km_instance;

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
