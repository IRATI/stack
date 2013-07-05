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
#include "librina-application.h"

using namespace rina;


void * doWorkApplication(void * arg) {
	RINAManager * rinaManager = (RINAManager *) arg;
	std::cout << "Application: started work\n";

	//1 send message and wait for reply
	ApplicationProcessNamingInformation sourceName;
	sourceName.setProcessName("/apps/source");
	sourceName.setProcessInstance("12");
	sourceName.setEntityName("database");
	sourceName.setEntityInstance("12");

	ApplicationProcessNamingInformation destName;
	destName.setProcessName("/apps/dest");
	destName.setProcessInstance("12345");
	destName.setEntityName("printer");
	destName.setEntityInstance("12623456");

	FlowSpecification flowSpec;

	AppAllocateFlowRequestMessage message;
	message.setSourceAppName(sourceName);
	message.setDestAppName(destName);
	message.setFlowSpecification(flowSpec);
	message.setRequestMessage(true);

	/* Try successfull flow allocation */
	std::cout << "Application: Sending netlink message and waiting for response\n";
	BaseNetlinkMessage* response =
			rinaManager->sendRequestMessageAndWaitForResponse(&message);
	std::cout << "Application: Got response!\n";

	AppAllocateFlowRequestResultMessage * flowResultMessage =
				dynamic_cast<AppAllocateFlowRequestResultMessage *>(response);
	std::cout << "Application: Flow allocated successfully. Port id: " <<
			flowResultMessage->getPortId() << "; DIF name: " <<
			flowResultMessage->getDifName().getProcessName() << "\n";
	delete flowResultMessage;

	message.setSourceAppName(sourceName);
	message.setDestAppName(destName);
	message.setFlowSpecification(flowSpec);
	message.setRequestMessage(true);

	/* Try unsuccessfull flow allocation */
	std::cout << "Application: Sending netlink message and waiting for response\n";
	response =
			rinaManager->sendRequestMessageAndWaitForResponse(&message);
	std::cout << "Application: Got response!\n";

	flowResultMessage =
			dynamic_cast<AppAllocateFlowRequestResultMessage *>(response);
	std::cout << "Application: Flow allocation failed. Error code: " <<
			flowResultMessage->getPortId() << "; Description: " <<
			flowResultMessage->getErrorDescription() << "\n";

	delete flowResultMessage;
	return (void *) 0;
}


void * doWorkIPCManager(void * arg) {
	RINAManager * rinaManager = (RINAManager *) arg;
	std::cout << "IPC Manager: Started work\n";
	std::cout << "IPC Manager: Waiting for incoming request/notification\n";
	BlockingFIFOQueue<IPCEvent> * eventQueue = rinaManager->getEventQueue();
	IPCEvent * event = eventQueue->take();
	std::cout<<"IPC Manager: Got event! Type: "<<event->getType()
			<<" Sequence number: "<<event->getSequenceNumber()<<"\n";

	FlowRequestEvent * flowEvent =
			dynamic_cast<FlowRequestEvent *>(event);

	ApplicationProcessNamingInformation difName;;
	difName.setProcessName("/difs/Test.DIF");

	AppAllocateFlowRequestResultMessage message;
	message.setSourceAppName(flowEvent->getSourceApplicationName());
	message.setPortId(23);
	message.setDifName(difName);
	message.setIpcProcessPortId(340);
	message.setIpcProcessId(24);
	message.setSequenceNumber(flowEvent->getSequenceNumber());
	message.setResponseMessage(true);

	std::cout<<"IPC Manager: Sending response!\n";
	rinaManager->sendResponseOrNotficationMessage(&message);
	delete flowEvent;

	std::cout << "IPC Manager: Waiting for incoming request/notification\n";
	event = eventQueue->take();
	std::cout<<"IPC Manager: Got event! Type: "<<event->getType()
						<<" Sequence number: "<<event->getSequenceNumber()<<"\n";

	flowEvent =
			dynamic_cast<FlowRequestEvent *>(event);

	message.setSourceAppName(flowEvent->getSourceApplicationName());
	message.setPortId(-15);
	message.setErrorDescription("Not enough resources to accomplish the request");
	message.setSequenceNumber(flowEvent->getSequenceNumber());
	message.setResponseMessage(true);

	std::cout<<"IPC Manager: Sending response!\n";
	rinaManager->sendResponseOrNotficationMessage(&message);
	std::cout<<"IPC Manager: Response sent, terminating!\n";

	delete flowEvent;

	return (void *) 0;
}


int main(int argc, char * argv[]) {
	std::cout << "TESTING LIBRINA-CORE\n";

	RINAManager * rinaManager1 = new RINAManager(4);
	RINAManager * rinaManager2 = new RINAManager(1);

	/* Test core RINAManager function */
	Thread * application;
	Thread * ipcManager;
	ThreadAttributes * threadAttributes = new ThreadAttributes();
	threadAttributes->setJoinable();
	application = new Thread(threadAttributes, &doWorkApplication, (void *) rinaManager1);
	ipcManager = new Thread(threadAttributes, &doWorkIPCManager, (void *) rinaManager2);
	delete threadAttributes;

	ipcManager->join(NULL);
	application->join(NULL);

	delete application;
	delete ipcManager;
	delete rinaManager1;
	delete rinaManager2;
}
