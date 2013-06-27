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

#include <iostream>

#include "core.h"

using namespace rina;


void * doWorkManager1(void * arg) {
	RINAManager * rinaManager = (RINAManager *) arg;
	std::cout << "Manager 1: started work\n";

	//1 send message and wait for reply
	ApplicationProcessNamingInformation * sourceName =
			new ApplicationProcessNamingInformation();
	sourceName->setProcessName("/apps/source");
	sourceName->setProcessInstance("12");
	sourceName->setEntityName("database");
	sourceName->setEntityInstance("12");

	ApplicationProcessNamingInformation * destName =
			new ApplicationProcessNamingInformation();
	destName->setProcessName("/apps/dest");
	destName->setProcessInstance("12345");
	destName->setEntityName("printer");
	destName->setEntityInstance("12623456");

	FlowSpecification * flowSpec = new FlowSpecification();

	AppAllocateFlowRequestMessage * message =
			new AppAllocateFlowRequestMessage();
	message->setSourceAppName(*sourceName);
	message->setDestAppName(*destName);
	message->setFlowSpecification(*flowSpec);

	message->setRequestMessage(true);
	message->setDestPortId(5);

	std::cout << "Manager 1: Sending netlink message and waiting for response\n";
	BaseNetlinkMessage* response = rinaManager->sendRequestMessageAndWaitForReply(message);
	std::cout << "Manager 1: Got response!\n";
	delete response;

	return (void *) 0;
}


void * doWorkManager2(void * arg) {
	RINAManager * rinaManager = (RINAManager *) arg;
	std::cout << "Manager 2: Started work\n";
	std::cout << "Manager 2: Waiting for incoming request/notification\n";
	BlockingFIFOQueue<IPCEvent> * eventQueue = rinaManager->getEventQueue();
	IPCEvent * event = eventQueue->take();
	std::cout<<"Manager 2: Got event! Type: "<<event->getType()
			<<" Sequence number: "<<event->getSequenceNumber()<<"\n";

	ApplicationProcessNamingInformation * sourceName =
			new ApplicationProcessNamingInformation();
	sourceName->setProcessName("/apps/source");
	sourceName->setProcessInstance("12");
	sourceName->setEntityName("database");
	sourceName->setEntityInstance("12");

	ApplicationProcessNamingInformation * destName =
			new ApplicationProcessNamingInformation();
	destName->setProcessName("/apps/dest");
	destName->setProcessInstance("12345");
	destName->setEntityName("printer");
	destName->setEntityInstance("12623456");

	FlowSpecification * flowSpec = new FlowSpecification();

	AppAllocateFlowRequestMessage * message =
			new AppAllocateFlowRequestMessage();
	message->setSourceAppName(*sourceName);
	message->setDestAppName(*destName);
	message->setFlowSpecification(*flowSpec);

	message->setResponseMessage(true);
	message->setSequenceNumber(event->getSequenceNumber());
	message->setDestPortId(4);

	std::cout<<"Manager2: Sending response!\n";
	rinaManager->sendResponseOrNotficationMessage(message);
	std::cout<<"Manager2: Response sent, terminating!\n";
	delete event;
	delete message;

	return (void *) 0;
}


int main(int argc, char * argv[]) {
	std::cout << "TESTING LIBRINA-CORE\n";

	RINAManager * rinaManager1 = new RINAManager(4);
	RINAManager * rinaManager2 = new RINAManager(5);

	/* Test core RINAManager function */
	Thread * thread1;
	Thread * thread2;
	ThreadAttributes * threadAttributes = new ThreadAttributes();
	threadAttributes->setJoinable();
	thread1 = new Thread(threadAttributes, &doWorkManager1, (void *) rinaManager1);
	thread2 = new Thread(threadAttributes, &doWorkManager2, (void *) rinaManager2);
	delete threadAttributes;

	thread2->join(NULL);
	thread1->join(NULL);

	delete thread1;
	delete thread2;
	delete rinaManager1;
	delete rinaManager2;

	/* Test NetlinkPortIdMap */
	try{
		netlinkPortIdMap->getNetlinkPortIdFromIPCProcessId(24);
	}catch(NetlinkException &e){
		std::cout<<"Caught expected exception: "<<e.what()<<"\n";
	}

	netlinkPortIdMap->putIPCProcessIdToNelinkPortIdMapping(25, 34);
	netlinkPortIdMap->putIPCProcessIdToNelinkPortIdMapping(89, 43);
	if (netlinkPortIdMap->getNetlinkPortIdFromIPCProcessId(25) != 34){
		std::cout<<"Error retrieving netlink port id from map\n";
		return -1;
	}
	if (netlinkPortIdMap->getNetlinkPortIdFromIPCProcessId(89) != 43){
		std::cout<<"Error retrieving netlink port id from map\n";
		return -1;
	}
	netlinkPortIdMap->putIPCProcessIdToNelinkPortIdMapping(89, 876);
	if (netlinkPortIdMap->getNetlinkPortIdFromIPCProcessId(89) != 876){
		std::cout<<"Error retrieving netlink port id from map\n";
		return -1;
	}

	ApplicationProcessNamingInformation * sourceName =
			new ApplicationProcessNamingInformation();
	sourceName->setProcessName("/apps/source");
	sourceName->setProcessInstance("12");
	sourceName->setEntityName("database");
	sourceName->setEntityInstance("12");

	ApplicationProcessNamingInformation * destName =
			new ApplicationProcessNamingInformation();
	destName->setProcessName("/apps/dest");
	destName->setProcessInstance("12345");
	destName->setEntityName("printer");
	destName->setEntityInstance("12623456");

	netlinkPortIdMap->putAPNametoNetlinkPortIdMapping(*sourceName, 73);
	netlinkPortIdMap->putAPNametoNetlinkPortIdMapping(*destName, 450);
	if(netlinkPortIdMap->getNetlinkPortIdFromAPName(*sourceName) != 73){
		std::cout<<"Error retrieving netlink port id from map\n";
		return -1;
	}
	if(netlinkPortIdMap->getNetlinkPortIdFromAPName(*destName) != 450){
		std::cout<<"Error retrieving netlink port id from map\n";
		return -1;
	}

	netlinkPortIdMap->putAPNametoNetlinkPortIdMapping(*destName, 877);
	delete destName;
	destName = new ApplicationProcessNamingInformation();
	destName->setProcessName("/apps/dest");
	destName->setProcessInstance("12345");
	destName->setEntityName("printer");
	destName->setEntityInstance("12623456");
	if(netlinkPortIdMap->getNetlinkPortIdFromAPName(*destName) != 877){
		std::cout<<"Error retrieving netlink port id from map\n";
		return -1;
	}

	if(netlinkPortIdMap->getIPCManagerPortId() != 1){
		std::cout<<"Error retrieving netlink port id from map\n";
		return -1;
	}

	std::cout<<"Netlink port id map tested successfully\n";
}
