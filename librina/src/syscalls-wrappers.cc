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

#include <unistd.h>
#include <sys/syscall.h>

#define SYS_createIPCProcess   __NR_ipc_create
#define SYS_destroyIPCProcess  __NR_ipc_destroy
#define SYS_readSDU            __NR_sdu_read
#define SYS_writeSDU           __NR_sdu_write
#define SYS_allocatePortId     __NR_allocate_port
#define SYS_deallocatePortId   __NR_deallocate_port
#define SYS_readManagementSDU  __NR_management_sdu_read
#define SYS_writeManagementSDU __NR_management_sdu_write

// These should be removed (should be checked at configuration time)
#if !defined(__NR_ipc_create)
#error No ipc_create syscall defined
#endif
#if !defined(__NR_ipc_destroy)
#error No ipc_create syscall defined
#endif
#if !defined(__NR_sdu_read)
#error No sdu_read syscall defined
#endif
#if !defined(__NR_sdu_write)
#error No sdu_write syscall defined
#endif
#if !defined(__NR_allocate_port)
#error No allocate_port syscall defined
#endif
#if !defined(__NR_deallocate_port)
#error No deallocate_port syscall defined
#endif
#if !defined(__NR_management_sdu_read)
#error No managment_sdu_read syscall defined
#endif
#if !defined(__NR_management_sdu_write)
#error No management_sdu_write syscall defined
#endif

#define RINA_PREFIX "syscalls"

#include "librina/logs.h"
#include "utils.h"
#include "rina-syscalls.h"
#include "rina-systypes.h"

#define DEBUG_SYSCALLS 1
#if DEBUG_SYSCALLS
#define DUMP_SYSCALL(X, Y) LOG_DBG("Gonna call syscall %s (%d)", X, Y);
#else
#define DUMP_SYSCALL(X, Y) do { } while (0);
#endif

namespace rina {

int syscallWriteSDU(int portId, void * sdu, int size)
{
        int result;

        DUMP_SYSCALL("SYS_writeSDU", SYS_writeSDU);

        result = syscall(SYS_writeSDU, portId, sdu, size);
        if (result < 0) {
                LOG_ERR("Syscall write SDU failed: %d", result);
        }

        return result;
}

int syscallReadSDU(int portId, void * sdu, int maxBytes)
{
        int result;

        DUMP_SYSCALL("SYS_readSDU", SYS_readSDU);

        result = syscall(SYS_readSDU, portId, sdu, maxBytes);
        if (result < 0) {
                LOG_ERR("Syscall read SDU failed: %d", result);
        }

        return result;
}

int syscallWriteManagementSDU(unsigned short ipcProcessId,
                              void *         sdu,
                              unsigned int   address,
                              unsigned int   portId,
                              int            size)
{
        int result;

        DUMP_SYSCALL("SYS_writeManagementSDU", SYS_writeManagementSDU);

        result = syscall(SYS_writeManagementSDU, ipcProcessId, address,
                         portId,sdu, size);
        if (result < 0) {
                LOG_ERR("Syscall write SDU failed: %d", result);
        }

        return result;
}

int syscallReadManagementSDU(int    ipcProcessId,
                             void * sdu,
                             int *  portId,
                             int    maxBytes)
{
        int result;

        DUMP_SYSCALL("SYS_readManagementSDU", SYS_readManagementSDU);

        result = syscall(SYS_readManagementSDU,
                         ipcProcessId,
                         sdu,
                         portId,
                         maxBytes);
        if (result < 0) {
                LOG_ERR("Syscall read SDU failed: %d", result);
        }

        return result;
}

int syscallDestroyIPCProcess(unsigned short ipcProcessId)
{
        int result;

        DUMP_SYSCALL("SYS_destroyIPCProcess", SYS_destroyIPCProcess);

        result = syscall(SYS_destroyIPCProcess, ipcProcessId);
        if (result < 0) {
                LOG_ERR("Syscall destroy IPC Process failed: %d",
                        result);
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
                         ipcProcessName.getProcessName().c_str(),
                         ipcProcessName.getProcessInstance().c_str(),
                         ipcProcessName.getEntityName().c_str(),
                         ipcProcessName.getEntityInstance().c_str(),
                         ipcProcessId,
                         difType.c_str());

        if (result < 0) {
                LOG_ERR("Syscall create IPC Process failed: %d",
                        result);
        }

        return result;
}

int syscallAllocatePortId(unsigned short ipcProcessId,
                          const ApplicationProcessNamingInformation & applicationName)
{
        int result;

        DUMP_SYSCALL("SYS_allocatePortId", SYS_allocatePortId);

        result = syscall(SYS_allocatePortId,
                         ipcProcessId,
                         applicationName.getProcessName().c_str(),
                         applicationName.getProcessInstance().c_str());

        if (result < 0) {
                LOG_ERR("Syscall allocate port id failed: %d", result);
        }

        return result;
}

int syscallDeallocatePortId(int portId)
{
        int result;

        DUMP_SYSCALL("SYS_deallocatePortId", SYS_deallocatePortId);

        result = syscall(SYS_deallocatePortId, portId);

        if (result < 0) {
                LOG_ERR("Syscall deallocate port id failed: %d",
                        result);
        }

        return result;
}

}
