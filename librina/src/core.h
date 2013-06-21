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

#include "concurrency.h"
#include "patterns.h"
#include "netlink-manager.h"


namespace rina{

/**
 * Main class of libRINA. Initializes the NetlinkManager,
 * the eventsQueue, and the NetlinkMessageReader thread.
 */
class RINAManager{

	/** The netlinkManager used to send and receive netlink messages */
	NetlinkManager * netlinkManager;

	/** The events queue */
	BlockingFIFOQueue<IPCEvent> * eventQueue;

	Thread * netlinkMessageReader;

public:
	RINAManager();
	~RINAManager();

	/** Sends a Netlink message and waits for the reply*/
	BaseNetlinkMessage * sendNetlinkMessageAndWaitForReply(
			BaseNetlinkMessage * netlinkMessage) throw(NetlinkException);

	BlockingFIFOQueue<IPCEvent>* getEventQueue();
	NetlinkManager* getNetlinkManager();
};

/**
 * Make RINAManager singleton
 */
//extern Singleton<RINAManager> rinaManager;

}

#endif /* CORE_H_ */
