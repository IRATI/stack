//
// IPC Process
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

#include <signal.h>

#include <cstdlib>
#include <sstream>

#define RINA_PREFIX "ipcp-main"

#include <librina/common.h>
#include <librina/logs.h>
#include "ipcp/ipc-process.h"
#include "common/debug.h"

int wrapped_main(int argc, char * argv[])
{
        if (argc != 7) {
                LOG_ERR("Wrong number of arguments: expected 7, got %d", argc);
                return EXIT_FAILURE;
        }
        std::string log_level = argv[5];
        std::string log_file = argv[6];

        rina::ApplicationProcessNamingInformation name(argv[1], argv[2]);

        unsigned short ipcp_id   = 0;
        unsigned int   ipcm_port = 0;

        std::stringstream ss(argv[3]);
        if (! (ss >> ipcp_id)) {
                LOG_ERR("Problems converting string to unsigned short");
                return EXIT_FAILURE;
        }

        std::stringstream ss2(argv[4]);
        if (! (ss2 >> ipcm_port)) {
                LOG_ERR("Problems converting string to unsigned int");
                return EXIT_FAILURE;
        }

        rinad::IPCProcessImpl ipcp(name, ipcp_id, ipcm_port, log_level, log_file);

        LOG_INFO("IPC Process name:     %s", argv[1]);
        LOG_INFO("IPC Process instance: %s", argv[2]);
        LOG_INFO("IPC Process id:       %u", ipcp_id);
        LOG_INFO("IPC Manager port:     %u", ipcm_port);

        LOG_INFO("IPC Process initialized, executing event loop...");

        rinad::EventLoop loop(&ipcp);

        rinad::register_handlers_all(loop);

        try {
        	loop.run();
        } catch (rina::Exception &e) {
        	LOG_ERR("Problems running event loop: %s", e.what());
        } catch (std::exception &e1) {
        	LOG_ERR("Problems running event loop: %s", e1.what());
        }

        LOG_DBG("Exited event loop");

        return EXIT_SUCCESS;
}

#define WANT_PARACHUTE 0

#if WANT_PARACHUTE
void sighandler_segv(int signum)
{
        LOG_CRIT("Got signal %d", signum);

        if (signum == SIGSEGV) {
                dump_backtrace();
                exit(EXIT_FAILURE);
        }
}
#endif

int main(int argc, char * argv[])
{
        int retval;

#if WANT_PARACHUTE
        if (signal(SIGSEGV, sighandler_segv) == SIG_ERR) {
                LOG_WARN("Cannot install SIGSEGV handler!");
        }
        LOG_DBG("SIGSEGV handler installed successfully");
#endif
        if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
                LOG_WARN("Cannot ignore SIGPIPE, bailing out");
                return EXIT_FAILURE;
        }
        LOG_DBG("SIGPIPE handler installed successfully");

        try {
                retval = wrapped_main(argc, argv);
        } catch (std::exception & e) {
                LOG_ERR("Got unhandled exception (%s)", e.what());
                retval = EXIT_FAILURE;
        }

        return retval;
}
