//
// Syscalls wrapper
//
//    Eduard Grasa          <eduard.grasa@i2cat.net>
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

#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>

#define SYS_createIPCProcess   __NR_ipc_create
#define SYS_destroyIPCProcess  __NR_ipc_destroy

// These should be removed (should be checked at configuration time)
#if !defined(__NR_ipc_create)
#error No ipc_create syscall defined
#endif
#if !defined(__NR_ipc_destroy)
#error No ipc_create syscall defined
#endif

#define RINA_PREFIX "librina.syscalls"

#include "librina/logs.h"
#include "utils.h"
#include "rina-syscalls.h"
#include "rina-systypes.h"

#define DEBUG_SYSCALLS 1
#if DEBUG_SYSCALLS
#define DUMP_SYSCALL(X, Y) LOG_DBG("Invoking %s (%d)", X, Y);
#else
#define DUMP_SYSCALL(X, Y) do { } while (0);
#endif

namespace rina {

int syscallDestroyIPCProcess(unsigned short ipcProcessId)
{
        int result;

        DUMP_SYSCALL("SYS_destroyIPCProcess", SYS_destroyIPCProcess);

        result = syscall(SYS_destroyIPCProcess, ipcProcessId);
        if (result < 0) {
        	LOG_DBG("Syscall destroy IPC Process failed: %d", errno);
                result = -errno;
        }

        return result;
}

int syscallCreateIPCProcess(const ApplicationProcessNamingInformation & ipcProcessName,
                            unsigned short                              ipcProcessId,
                            const std::string &                         difType)
{
        int result;

        DUMP_SYSCALL("SYS_createIPCProcess", SYS_createIPCProcess);

        result = syscall(SYS_createIPCProcess,
                         ipcProcessName.processName.c_str(),
                         ipcProcessName.processInstance.c_str(),
                         ipcProcessName.entityName.c_str(),
                         ipcProcessName.entityInstance.c_str(),
                         ipcProcessId,
                         difType.c_str());
        if (result < 0) {
        	LOG_DBG("Syscall create IPC Process failed: %d", errno);
                result = -errno;
        }

        return result;
}

}
