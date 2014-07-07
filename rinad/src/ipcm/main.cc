/*
 * IPC Manager
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>

#define RINA_PREFIX "ipcm"

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/logs.h>

#include "event-loop.h"
#include "rina-configuration.h"
#include "tclap/CmdLine.h"
#include "ipcm.h"
#include "configuration.h"

using namespace std;
using namespace TCLAP;


int main(int argc, char * argv[])
{
        std::string conf;

        // Wrap everything in a try block.  Do this every time,
        // because exceptions will be thrown for problems.
        try {

                // Define the command line object.
                TCLAP::CmdLine cmd("IPC Manager", ' ', "0.1");

                TCLAP::ValueArg<std::string> conf_arg("c",
                                                      "config",
                                                      "Configuration file to load",
                                                      true,
                                                      "ipcmanager.conf",
                                                      "string");

                cmd.add(conf_arg);

                // Parse the args.
                cmd.parse(argc, argv);

                // Get the value parsed by each arg.
                conf = conf_arg.getValue();

                LOG_DBG("Config file is: %s", conf.c_str());

        } catch (ArgException &e) {
                LOG_ERR("Error: %s for arg %d", e.error().c_str(), e.argId().c_str());
                return EXIT_FAILURE;
        }

        rinad::IPCManager ipcm;
        rinad::EventLoop loop(&ipcm);

        if (!parse_configuration(conf, &ipcm)) {
                LOG_ERR("Failed to load configuration");
                return EXIT_FAILURE;
        }

        rinad::register_handlers_all(loop);

        loop.run();

        return EXIT_SUCCESS;
}
