//
// Core librina logic
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

#ifndef CORE_H_
#define CORE_H_

#include <map>
#include "concurrency.h"
#include "patterns.h"
#include "netlink-manager.h"

#define WAIT_RESPONSE_TIMEOUT 10

namespace rina {

/**
 * Contains the information to identify a RINA netlink endpoing:
 * netlink port ID and IPC Process id
 */
class RINANetlinkEndpoint{
	unsigned int netlinkPortId;
	unsigned short ipcProcessId;

public:
	RINANetlinkEndpoint();
	RINANetlinkEndpoint(unsigned int netlinkPortId,
			unsigned short ipcProcessId);
	unsigned short getIpcProcessId() const;
	void setIpcProcessId(unsigned short ipcProcessId);
	unsigned int getNetlinkPortId() const;
	void setNetlinkPortId(unsigned int netlinkPortId);
};

/**
 * Contains mappings of application process name to netlink portId,
 * or IPC process id to netlink portId.
 */
class NetlinkPortIdMap {

	/** Stores the mappings of IPC Process id to nelink portId */
	std::map<unsigned short, RINANetlinkEndpoint *> ipcProcessIdMappings;

	/** Stores the mappings of application process name to netlink port id */
	std::map<ApplicationProcessNamingInformation, RINANetlinkEndpoint *>
		applicationNameMappings;

public:
	void putIPCProcessIdToNelinkPortIdMapping(
			unsigned int netlinkPortId, unsigned short ipcProcessId);
	RINANetlinkEndpoint * getNetlinkPortIdFromIPCProcessId(
			unsigned short ipcProcessId) throw(NetlinkException);
	void putAPNametoNetlinkPortIdMapping(
			ApplicationProcessNamingInformation apName,
			unsigned int netlinkPortId, unsigned short ipcProcessId);
	RINANetlinkEndpoint * getNetlinkPortIdFromAPName(
			ApplicationProcessNamingInformation apName) throw(NetlinkException);
	unsigned int getIPCManagerPortId();

	/**
	 * Poulates the "destPortId" field for messages that have to be sent,
	 * or updates the mappings for received messages
	 * @param message
	 * @param sent
	 */
	void updateMessageOrPortIdMap(BaseNetlinkMessage* message, bool send)
		throw(NetlinkException);
};

/**
 * Class used by the thread that sent a Netlink message
 * to wait for the response
 */
class PendingNetlinkMessage: public ConditionVariable {
	BaseNetlinkMessage * responseMessage;
	unsigned int sequenceNumber;

public:
	PendingNetlinkMessage(unsigned int sequenceNumber);
	~PendingNetlinkMessage() throw ();
	BaseNetlinkMessage * getResponseMessage();
	void setResponseMessage(BaseNetlinkMessage * responseMessage);
	unsigned int getSequenceNumber() const;
};

/**
 * Encapsulates the information of a Netlink session (defined as a exchange of
 * messages between two netlink sockets)
 */
class NetlinkSession {

	/** The port id where the destination Netlink socket is listening */
	unsigned int sessionId;

	/**
	 * Stores the local Netlink request messages that are waiting for a reply
	 */
	std::map<unsigned int, PendingNetlinkMessage *> localPendingMessages;

	/**
	 * Stores the Netlink request messages from peers
	 * that are waiting for a response
	 */
	std::map<unsigned int, BaseNetlinkMessage *> remotePendingMessages;

public:
	NetlinkSession(int sessionId);
	~NetlinkSession();
	void putLocalPendingMessage(PendingNetlinkMessage* pendingMessage);
	PendingNetlinkMessage* takeLocalPendingMessage(
			unsigned int sequenceNumber);
	void putRemotePendingMessage(BaseNetlinkMessage* pendingMessage);
	BaseNetlinkMessage* takeRemotePendingMessage(unsigned int sequenceNumber);
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

	/** State of the netlink sessions */
	std::map<unsigned int, NetlinkSession*> netlinkSessions;

	/** Keeps the mappings between netlink port ids and app/dif names */
	NetlinkPortIdMap netlinkPortIdMap;

	void initialize();

	/**
	 * Retrieves an existing Netlink session, or creates one if none is
	 * available
	 */
	NetlinkSession* getAndCreateNetlinkSession(unsigned int sessionId);

	/** Return an existing Netlink session -NULL if there is none -*/
	NetlinkSession* getNetlinkSession(unsigned int sessionId);

public:
	RINAManager();
	RINAManager(unsigned int netlinkPort);
	~RINAManager();

	BaseNetlinkMessage * sendRequestAndWaitForResponse(
			BaseNetlinkMessage * request, const std::string& errorDescription)
			throw(NetlinkException);

	/** Sends a request message and waits for the reply*/
	BaseNetlinkMessage * sendRequestMessageAndWaitForResponse(
			BaseNetlinkMessage * netlinkMessage) throw (NetlinkException);

	/** Send a response message or a notificaiton message */
	void sendResponseOrNotficationMessage(
			BaseNetlinkMessage * netlinkMessage) throw (NetlinkException);

	/**
	 * Notify about the reception of a Netlink response message. Will
	 * cause the thread that was waiting for the response to wake up.
	 */
	void netlinkResponseMessageArrived(BaseNetlinkMessage * response);

	/**
	 * Notify about the reception of a Netlink request message.
	 */
	void netlinkRequestMessageArrived(BaseNetlinkMessage * request);

	/**
	 * Notify about the reception of a Netlink notificaiton message
	 */
	void netlinkNotificationMessageArrived(BaseNetlinkMessage * notification);

	BlockingFIFOQueue<IPCEvent>* getEventQueue();
	NetlinkManager* getNetlinkManager();
};

/**
 * Make RINAManager singleton
 */
extern Singleton<RINAManager> rinaManager;

void setNetlinkPortId(unsigned int netlinkPortId);
unsigned int getNelinkPortId();

}

#endif /* CORE_H_ */
