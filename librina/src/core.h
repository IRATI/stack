/*
 * Core librina logic
 *
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#ifndef LIBRINA_CORE_H
#define LIBRINA_CORE_H

#ifdef __cplusplus

#include <map>

#include "librina/concurrency.h"
#include "librina/patterns.h"

#include "netlink-manager.h"

#define WAIT_RESPONSE_TIMEOUT 10

namespace rina {

char * stringToCharArray(std::string s);
char * intToCharArray(int i);

/**
 * Contains the information to identify a RINA netlink endpoing:
 * netlink port ID and IPC Process id
 */
class RINANetlinkEndpoint{
	unsigned int netlinkPortId;
	unsigned short ipcProcessId;
	ApplicationProcessNamingInformation applicationProcessName;

public:
	RINANetlinkEndpoint();
	RINANetlinkEndpoint(unsigned int netlinkPortId,
			unsigned short ipcProcessId);
	RINANetlinkEndpoint(unsigned int netlinkPortId,
				unsigned short ipcProcessId,
				const ApplicationProcessNamingInformation& appNamingInfo);
	unsigned short getIpcProcessId() const;
	void setIpcProcessId(unsigned short ipcProcessId);
	unsigned int getNetlinkPortId() const;
	void setNetlinkPortId(unsigned int netlinkPortId);
	const ApplicationProcessNamingInformation&
		getApplicationProcessName() const;
	void setApplicationProcessName(
		const ApplicationProcessNamingInformation& applicationProcessName);
};

/**
 * Contains mappings of application process name to netlink portId,
 * or IPC process id to netlink portId.
 */
class NetlinkPortIdMap {

	/** Stores the mappings of IPC Process id to nelink portId */
	std::map<unsigned short, RINANetlinkEndpoint *> ipcProcessIdMappings;

	/** Stores the mappings of application process name to netlink port id */
	std::map<std::string, RINANetlinkEndpoint *>
		applicationNameMappings;

public:
	~NetlinkPortIdMap();

	void putIPCProcessIdToNelinkPortIdMapping(
			unsigned int netlinkPortId, unsigned short ipcProcessId);
	RINANetlinkEndpoint * getNetlinkPortIdFromIPCProcessId(
			unsigned short ipcProcessId);
	void putAPNametoNetlinkPortIdMapping(
			ApplicationProcessNamingInformation apName,
			unsigned int netlinkPortId, unsigned short ipcProcessId);
	RINANetlinkEndpoint * getNetlinkPortIdFromAPName(
			ApplicationProcessNamingInformation apName);
	unsigned int getIPCManagerPortId();

	/**
	 * Poulates the "destPortId" field for messages that have to be sent,
	 * or updates the mappings for received messages
	 * @param message
	 * @param sent
	 */
	void updateMessageOrPortIdMap(BaseNetlinkMessage* message, bool send);

	/**
	 * An OS Process has finalized. Retrieve the information associated to
	 * the NL port-id (application name, IPC Process id if it is IPC process),
	 * and return it in the form of an OSProcessFinalized event
	 * @param nl_portid
	 * @return
	 */
	IPCEvent * osProcessFinalized(unsigned int nl_portid);
};

/**
 * Main class of libRINA. Initializes the NetlinkManager,
 * the eventsQueue, and the NetlinkMessageReader thread.
 */
class RINAManager {

	/** The netlinkManager used to send and receive netlink messages */
	NetlinkManager * netlinkManager;

	/** The events queue */
	BlockingFIFOQueue<IPCEvent> * eventQueue;

	/** The thread that is continuously reading incoming Netlink messages */
	Thread * netlinkMessageReader;

	/** The lock for send/receive operations */
	Lockable sendReceiveLock;

	/** Keeps the mappings between netlink port ids and app/dif names */
	NetlinkPortIdMap netlinkPortIdMap;

	void initialize();
public:
	RINAManager();
	RINAManager(unsigned int netlinkPort);
	~RINAManager();

	/** Sends a NL message of default maximum size (PAGE SIZE) */
	void sendMessage(BaseNetlinkMessage * netlinkMessage, bool fill_seq_num);

	/** Sends a NL message of specified maximum size */
	void sendMessageOfMaxSize(BaseNetlinkMessage * netlinkMessage,
			size_t maxSize, bool fill_seq_num);

	/**
	 * Notify about the reception of a Netlink message
	 */
	void netlinkMessageArrived(BaseNetlinkMessage * notification);

	/**
	 * An OS Process has finalized. Retrieve the information associated to
	 * the NL port-id (application name, IPC Process id if it is IPC process),
	 * and return it in the form of an OSProcessFinalized event
	 * @param nl_portid
	 * @return
	 */
	IPCEvent * osProcessFinalized(unsigned int nl_portid);

	BlockingFIFOQueue<IPCEvent>* getEventQueue();
	NetlinkManager* getNetlinkManager();
	bool keep_on_reading;
};

/**
 * Make RINAManager singleton
 */
extern Singleton<RINAManager> rinaManager;

void setNetlinkPortId(unsigned int netlinkPortId);
unsigned int getNelinkPortId();

}

#endif

#endif
