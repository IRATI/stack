/*
 * IPC Manager
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Vincenzo Maffione <v.maffione@nextworks.it>
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
#include <map>
#include <vector>

#define RINA_PREFIX     "ipcm"

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/logs.h>

#include "event-loop.h"
#include "rina-configuration.h"
#include "tclap/CmdLine.h"
#include "ipcm.h"

using namespace std;
using namespace TCLAP;


int main(int argc, char * argv[])
{
        // Wrap everything in a try block.  Do this every time, 
        // because exceptions will be thrown for problems. 
        try {

                // Define the command line object.
                TCLAP::CmdLine cmd("Command description message", ' ', "0.9");

                // Define a value argument and add it to the command line.
                TCLAP::ValueArg<string> nameArg("n",
                                                     "name",
                                                     "Name to print",
                                                     true,
                                                     "homer",
                                                     "string");
                cmd.add(nameArg);

                // Parse the args.
                cmd.parse(argc, argv);

                // Get the value parsed by each arg. 
                string name = nameArg.getValue();

                cout << "My name is: " << name << endl;

        } catch (ArgException &e) {
                cerr << "error: " << e.error() << " for arg "
                          << e.argId() << endl;
        }

        IPCManager ipcm;
        EventLoop loop(&ipcm);

        register_handlers_all(loop);

        loop.run();

        return EXIT_SUCCESS;
}
