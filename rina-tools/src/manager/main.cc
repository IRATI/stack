//
// RINA manager main
//
// Addy Bombeke <addy.bombeke@ugent.be>
// Vincenzo Maffione <v.maffione@nextworks.it>
// Bernat Gastón <bernat.gaston@i2cat.net>
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

#define RINA_PREFIX     "rina-cdap-echo"
#include <librina/logs.h>

#include "tclap/CmdLine.h"

#include "config.h"
#include "manager.h"

using namespace std;


int wrapped_main(int argc, char** argv)
{
        string manager_apn;
        string manager_api;
        string dif_name;

        try {
                TCLAP::CmdLine cmd("manager", ' ', PACKAGE_VERSION);
                TCLAP::ValueArg<string> manager_apn_arg("",
                                                       "manager-apn",
                                                       "Application process name for the manager",
                                                       false,
                                                       "rina.apps.manager",
                                                       "string");
                TCLAP::ValueArg<string> manager_api_arg("",
                                                       "manager-api",
                                                       "Application process instance for the manager",
                                                       false,
                                                       "1",
                                                       "string");
                TCLAP::ValueArg<string> dif_arg("d",
                                                "dif-to-register-at",
                                                "The name of the DIF to register at (empty means 'any DIF')",
                                                false,
                                                "",
                                                "string");
                cmd.add(dif_arg);

                cmd.parse(argc, argv);

                manager_apn = manager_apn_arg.getValue();
                manager_api = manager_api_arg.getValue();
                dif_name = dif_arg.getValue();

        } catch (TCLAP::ArgException &e) {
                LOG_ERR("Error: %s for arg %d",
                        e.error().c_str(),
                        e.argId().c_str());
                return EXIT_FAILURE;
        }

        rina::initialize("INFO", "");
        Manager m(dif_name, manager_apn, manager_api);
        m.run();

        return EXIT_SUCCESS;
}

int main(int argc, char * argv[])
{
        int retval;

        //try {
                retval = wrapped_main(argc, argv);
       /* } catch (rina::Exception& e) {
                LOG_ERR("%s", e.what());
                return EXIT_FAILURE;

        } catch (std::exception& e) {
                LOG_ERR("Uncaught exception");
                return EXIT_FAILURE;
        }
        */

        return retval;
}
