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

#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

#define RINA_PREFIX "syscalls"

#include "logs.h"
#include "rina-syscalls.h"
#include "rina-systypes.h"

namespace rina {

        int syscallWriteSDU(int portId, void * sdu, int size)
        {
                int result;

                result = syscall(SYS_writeSDU, portId, sdu, size);
                if (result < 0) {
                        LOG_ERR("Write SDU failed (errno = %d)", errno);
                        return -errno;
                }

                return 0;
        }

        int syscallReadSDU(int portId, void * sdu, int maxBytes)
        {
                int result;

                result = syscall(SYS_readSDU, portId, sdu, maxBytes);
                if (result < 0) {
                        LOG_ERR("Read SDU failed (errno = %d)", errno);
                        return -errno;
                }

                return result;
        }

        int syscallDestroyIPCProcess(unsigned int ipcProcessId)
        {
                int result;

                result = syscall(SYS_destroyIPCProcess, ipcProcessId);
                if (result < 0) {
                        LOG_ERR("Destroy IPC Process failed (errno = %d)",
                                errno);
                        return -errno;
                }

                return 0;
        }

        int syscallCreateIPCProcess(const ApplicationProcessNamingInformation & ipcProcessName,
                                    unsigned int                                ipcProcessId,
                                    const std::string &                         difType)
        {
                name_t name;

                name.process_name     = const_cast<char *>(ipcProcessName.getProcessName().c_str());
                name.process_instance = const_cast<char *>(ipcProcessName.getProcessInstance().c_str());
                name.entity_name      = const_cast<char *>(ipcProcessName.getEntityName().c_str());
                name.entity_instance  = const_cast<char *>(ipcProcessName.getEntityInstance().c_str());

                int result;
                result = syscall(SYS_createIPCProcess,
                                 &name,
                                 ipcProcessId,
                                 difType.c_str());
                if (result < 0) {
                        LOG_ERR("Create IPC Process failed (errno = %d)",
                                errno);
                        return -errno;
                }

                return 0;
        }

}
