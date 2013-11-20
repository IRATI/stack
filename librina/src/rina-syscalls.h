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

#ifndef LIBRINA_SYSCALLS_H
#define	LIBRINA_SYSCALLS_H

#include "librina-common.h"

namespace rina {

        /**
         * Wrapper of the writeSDU system call
         * @param portId the portId where the data has to be written
         * @param sdu the data to be written
         * @param size the size of the data to be written
         * @return 0 if everything was ok, negative number indicating error
         *         otherwise
         */
        int syscallWriteSDU(int portId, void * sdu, int size);

        /**
         * Wrapper of the readSDU system call
         * @param portId the portId where the data has to be written
         * @param sdu pointer to the memory address of the first byte of the
         *        SDU
         * @param maxBytes Maximum amount of bytes to read
         * @return number of bytes read if successful, a negative number 
         *         indicating an error otherwise
         */
        int syscallReadSDU(int portId, void * sdu, int maxBytes);

        /**
         * Wrapper of ipcCreate system call
         * @param ipcProcessName The name of the IPC Process to be created
         * @param ipcProcessId The id of the IPC Process to be created
         * @param difType The DIF type
         * @return 0 if everything was ok, negative number indicating error
         *         otherwise
         */
        int syscallCreateIPCProcess(const ApplicationProcessNamingInformation & ipcProcessName,
                                    unsigned int                                ipcProcessId,
                                    const std::string &                         difType);

        /**
         * Wrapper of the ipcDestroy systen call
         * @param ipcProcessId The id of the IPC Process
         * @return 0 upon success, a negative number indicating an error
         *         otherwise
         */
        int syscallDestroyIPCProcess(unsigned int ipcProcessId);

}

#endif
