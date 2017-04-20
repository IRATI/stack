/*
 * Syscalls wrapper
 *
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#ifndef LIBRINA_SYSCALLS_H
#define	LIBRINA_SYSCALLS_H

#ifdef __cplusplus

#include "librina/common.h"

namespace rina {

        /**
         * Wrapper of ipcCreate system call
         * @param ipcProcessName The name of the IPC Process to be created
         * @param ipcProcessId The id of the IPC Process to be created
         * @param difType The DIF type
         * @return 0 if everything was ok, negative number indicating error
         *         otherwise
         */
        int syscallCreateIPCProcess(const ApplicationProcessNamingInformation & ipcProcessName,
                                    unsigned short                              ipcProcessId,
                                    const std::string &                         difType);

        /**
         * Wrapper of the ipcDestroy systen call
         * @param ipcProcessId The id of the IPC Process
         * @return 0 upon success, a negative number indicating an error
         *         otherwise
         */
        int syscallDestroyIPCProcess(unsigned short ipcProcessId);
}

#endif

#endif
