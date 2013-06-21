//
// Core librina implementation
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

#define RINA_PREFIX "core"

#include "logs.h"
#include "core.h"

namespace rina{

//Singleton<RINAManager> rinaManager;

/** main function of the Netlink message reader thread */
void * doNetlinkMessageReaderWork(void * arg){
	RINAManager * myRINAManager = (RINAManager *) arg;
	NetlinkManager * netlinkManager = myRINAManager->getNetlinkManager();
	BlockingFIFOQueue<IPCEvent> * eventsQueue = myRINAManager->getEventQueue();
	BaseNetlinkMessage * incomingMessage;

	//Continuously try to read incoming Netlink messages
	while(true){
		//1 Receive message
		try{
			incomingMessage = netlinkManager->getMessage();
		}catch(NetlinkException &e){
			LOG_ERR("Error receiving netlink message. %s", e.what());
		}

		//2 Decide if it is a response message or a notification message

		//3a if response message, get associated request and signal the
		//thread that send the message that the response is already available

		//3b if notificaiton message, create associated IPCEvent and put it in the
		//event queue
	}

	return (void *) 0;
}

/* CLASS RINA Manager */
RINAManager::RINAManager(){
	//1 Initialize NetlinkManager
	try{
		netlinkManager = new NetlinkManager();
	}catch(NetlinkException &e){
		LOG_ERR("Error initializing Netlink Manager. %s", e.what());
		LOG_ERR("Program will exit now");
		exit(-1); 	//FIXME Is this too drastic?
	}
	LOG_DBG("Initialized Netlink Manager");

	//2 Initialie events Queue
	eventQueue = new BlockingFIFOQueue<IPCEvent>();
	LOG_DBG("Initialized event queue");

	//3 Start Netlink message reader thread
	ThreadAttributes * threadAttributes = new ThreadAttributes();
	threadAttributes->setJoinable();
	netlinkMessageReader = new Thread(threadAttributes,
			&doNetlinkMessageReaderWork, (void *) this);
	LOG_DBG("Started Netlink Message reader thread");
}

RINAManager::~RINAManager(){
	delete netlinkManager;
	delete netlinkMessageReader;
	delete eventQueue;
}


}




