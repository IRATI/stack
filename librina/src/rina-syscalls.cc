//
// Syscalls wrapper
//
//    Eduard Grasa          <eduard.grasa@i2cat.net>
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

/* FIXME: PIGSTY HACK TO USER OUR SYSCALLS, PLEASE FIX ASAP !!! */
#undef _ASM_X86_UNISTD_32_H
#include "/usr/include/linux/include/asm-x86/unistd_32.h"

//#undef _ASM_X86_UNISTD_64_H
//#include "/usr/include/linux/include/asm-x86/unistd_64.h"

#define SYS_createIPCProcess  __NR_ipc_create
#define SYS_destroyIPCProcess __NR_ipc_destroy
#define SYS_readSDU           __NR_sdu_read
#define SYS_writeSDU          __NR_sdu_write

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

#include <sys/syscall.h>
#include <unistd.h>

#define RINA_PREFIX "syscalls"

#include "logs.h"
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

        int syscallDestroyIPCProcess(unsigned int ipcProcessId)
        {
                int result;

                DUMP_SYSCALL("SYS_destroyIPCProcess", SYS_destroyIPCProcess);

                result = syscall(SYS_destroyIPCProcess, ipcProcessId);
                if (result < 0) {
                        LOG_ERR("Syscall destroy IPC Process failed: %d", result);
                }

                return result;
        }

        int syscallCreateIPCProcess(const ApplicationProcessNamingInformation & ipcProcessName,
                                    unsigned int                                ipcProcessId,
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
                        LOG_ERR("Syscall create IPC Process failed: %d", result);
                }

                return result;
        }

}
