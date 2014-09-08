//
// Echo time main
// 
// Addy Bombeke <addy.bombeke@ugent.be>
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

#include <librina/librina.h>

#include "tclap/CmdLine.h"

#include "config.h"
#include "application-builder.h"

int wrapped_main(int argc, char** argv)
{
        // Just for testing, has to be rearranged
        TCLAP::CmdLine cmd("RINA-echo-time", ' ', PACKAGE_VERSION);

        rina::initialize("DBG", "testprog.log");

        ApplicationBuilder ab;

        ab.configure(argc, argv);
        ab.runApplication();

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
