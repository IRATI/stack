//
// Debugging utils
//
//    Francesco Salvestrini <f.salvestrini@nextworks.it>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301  USA
//

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

        LOG_CRIT("Dumping backtrace");
        LOG_CRIT("======== CUT HERE ========");
        for (j = 0; j < nptrs; j++) {
                // NOTE: DO NOT CHANGE THE FOLLOWING LINE !!!
                std::cout << "  " << strings[j] << std::endl;
        }
        LOG_CRIT("======== CUT HERE ========");

        free(strings);
}
#else
void dump_backtrace(void)
{ LOG_DBG("Cannot dump a backtrace"); }
#endif
