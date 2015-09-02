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
         * Wrapper of the writeSDU system call
         * @param portId the portId where the data has to be written
         * @param sdu the data to be written
         * @param size the size of the data to be written
         * @return 0 if everything was ok, negative number indicating error
         *         otherwise
         */
        int syscallWriteSDU(int portId,
        		    void * sdu,
        		    int size);

        /**
         * Wrapper of the readSDU system call
         * @param portId the portId where the data has to be written
         * @param sdu pointer to the memory address of the first byte of the
         *        SDU
         * @param maxBytes Maximum amount of bytes to read
         * @return number of bytes read if successful, a negative number
         *         indicating an error otherwise
         */
        int syscallReadSDU(int portId,
        		   void * sdu,
        		   int maxBytes);

        /**
         * Wrapper of the managementSDUWrite system call
         * @param ipcProcessId the ipcProcess that has to write the SDU
         * @param address the address of the IPC Process where this SDU
         * has to be sent. 0 if the N-1 portid is used instead of the
         * address
         * @param portId the N-1 portId where the data has to be written.
         * 0 is the address is used instead of portId
         * @param sdu the data to be written (SDU)
         * @param size the size of the data to be written
         * @return 0 if everything was ok, negative number indicating error
         *         otherwise
         */
        int syscallWriteManagementSDU(unsigned short ipcProcessId, void * sdu,
                        unsigned int address, unsigned int portId, int size);

        /**
         * Wrapper of the managementSDURead system call
         * @param ipcProcessId the ipcProcess where we want to read an SDU from
         * @param sdu pointer to the memory address of the first byte of data
         * @param portid The port-id where the SDU has been read from
         * @param maxBytes Maximum amount of bytes to read
         * @return number of bytes read if successful, a negative number
         *         indicating an error otherwise
         */
        int syscallReadManagementSDU(int ipcProcessId, void * sdu, int * portId,
                        int maxBytes);

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

        /**
         * Wrapper of the allocate port-id system call
         * @param ipcProcessId The id of the IPC Process that is providing
         * the flow
         * @param applicationName The AP name and AP instance of the app
         * that will be using the flow
         * @return portId to use (>0) if everything was ok, negative number
         * indicating error otherwise
         */
        int syscallAllocatePortId(unsigned short ipcProcessId,
                        	  const ApplicationProcessNamingInformation & applicationName);

        /**
         * Wrappert of the deallocate port-is system call
         * @param ipcProcessId The id of the IPC Process that is providing
         * the flow
         * @param portId the port-id to be deallocated
         * @return 0 if everything was ok, negative number indicating error
         *         otherwise
         */
        int syscallDeallocatePortId(unsigned short ipcProcessId, int portId);

	/**
         * Wrapper of the flow_io_ctl system call
         * @param portId the port-id to control
         * @param cmd, the command, only F_GETFL and F_SETFL are supported
         * @param arg, the flags to be set, only O_NONBLOCK is supported
         * @return 0 if everything was ok, negative number indicating error
         *         otherwise
         */
        int syscallFlowIOCtl(int portId, int cmd, unsigned long arg);
}

#endif

#endif
