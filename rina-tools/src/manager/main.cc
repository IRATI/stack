//
// RINA manager main
//
// Addy Bombeke <addy.bombeke@ugent.be>
// Vincenzo Maffione <v.maffione@nextworks.it>
// Bernat Gastón <bernat.gaston@i2cat.net>
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

#define RINA_PREFIX     "rina-cdap-echo"
#include <librina/logs.h>

#include "tclap/CmdLine.h"

#include "config.h"
#include "cdap-echo-client.h"
#include "cdap-echo-server.h"

using namespace std;


int wrapped_main(int argc, char** argv)
{
        bool quiet;
        string manager_apn;
        string manager_api;
        string dif_name;

        try {
                TCLAP::CmdLine cmd("manager", ' ', PACKAGE_VERSION);
                TCLAP::SwitchArg quiet_arg("q",
                                           "quiet",
                                           "Suppress some output",
                                           false);
                TCLAP::ValueArg<string> manager_arg("",
                                                       "manager-apn",
                                                       "Application process name for the manager",
                                                       false,
                                                       "rina.apps.manager",
                                                       "string");
                TCLAP::ValueArg<string> manager_arg("",
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
                cmd.add(quiet_arg);
                cmd.add(dif_arg);

                cmd.parse(argc, argv);

                quiet = quiet_arg.getValue();
                dif_name = dif_arg.getValue();

        } catch (TCLAP::ArgException &e) {
                LOG_ERR("Error: %s for arg %d",
                        e.error().c_str(),
                        e.argId().c_str());
                return EXIT_FAILURE;
        }

        rina::initialize("INFO", "");
        Client c(dif_name, manager_apn, manager_api, quiet, wait);

        c.run();
        }

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
