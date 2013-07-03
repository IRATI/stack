/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef LIBRINA_IPC_PROCESS_H
#define LIBRINA_IPC_PROCESS_H

#ifdef __cplusplus

#include "librina-common.h"

namespace rina {

/**
 * Event informing the IPC Process about a flow deallocation request
 */
class FlowDeallocateRequestEvent: public IPCEvent {
	/** The port-id that locally identifies the flow */
	int portId;

	/** The name of the DIF that is providing this flow */
	ApplicationProcessNamingInformation DIFName;

	/** The application that requested the flow deallocation*/
	ApplicationProcessNamingInformation applicationName;

	/** The id of the IPC Process that should deallocte the flow */
	unsigned short ipcProcessId;

public:
	FlowDeallocateRequestEvent(int portId,
			const ApplicationProcessNamingInformation& DIFName,
			const ApplicationProcessNamingInformation& appName,
			unsigned short ipcProcessId,
			unsigned int sequenceNumber);
	int getPortId() const;
	const ApplicationProcessNamingInformation& getDIFName() const;
	const ApplicationProcessNamingInformation& getApplicationName() const;
	unsigned short getIPCProcessId() const;
};

/**
 * Class used by IPC Processes to interact with application processes
 */
class IPCProcessApplicationManager {

public:
	/**
	 * Invoked by the IPC Process to respond to the Application Process that
	 * requested a flow deallocation
	 * @param flowDeallocateEvent Object containing information about the flow
	 * deallocate request event
	 * @param result 0 indicates success, a negative number an error code
	 * @param errorDescription optional explanation about the error (if any)
	 * @throws IPCException if there are issues replying ot the application
	 */
	void flowDeallocated(const FlowDeallocateRequestEvent flowDeallocateEvent,
			int result, std::string errorDescription) throw (IPCException);
};

/**
 * Make IPC Process Application Manager singleton
 */
extern Singleton<IPCProcessApplicationManager> ipcProcessApplicationManager;

}

#endif

#endif
