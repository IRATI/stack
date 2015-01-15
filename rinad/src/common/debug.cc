//
// Debugging utils
//
//    Francesco Salvestrini <f.salvestrini@nextworks.it>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include <config.h>
#include <iostream>
#include <cstdlib>

#define RINA_PREFIX "rinad-debug"

#include <librina/logs.h>

#if defined(HAVE_EXECINFO_H) && \
    defined(HAVE_BACKTRACE)  && \
    defined(HAVE_BACKTRACE_SYMBOLS)
#include <execinfo.h>

#define SIZE 100

static void * buffer[SIZE];

void dump_backtrace(void)
{
        int     nptrs;
        char ** strings;
        
        nptrs   = backtrace(buffer, SIZE);
        strings = backtrace_symbols(buffer, nptrs);
        if (!strings) {
                LOG_CRIT("Backtrace not available");
        }
        
        int j;

        LOG_CRIT("Backtrace:");
        for (j = 0; j < nptrs; j++) {
                // NOTE: DO NOT CHANGE THE FOLLOWING LINE !!!
                std::cout << "  " << strings[j] << std::endl;
        }

        free(strings);
}
#else
void dump_backtrace(void)
{ LOG_DBG("Cannot dump a backtrace"); }
#endif
