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

#include "netlink-manager.h"

using namespace rina;

int main(int argc, char * argv[]) {
	std::cout << "TESTING LIBRINA-NETLINK-MANAGER\n";

	/* Test User-space to User-space communication */
	NetlinkManager * source = new NetlinkManager(4);
	NetlinkManager * destination = new NetlinkManager(5);

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
	message->setDestPortId(5);
	message->setSourceAppName(*sourceName);
	message->setDestAppName(*destName);
	message->setFlowSpecification(*flowSpec);
	source->sendMessage(message);
	delete message;

	AppAllocateFlowRequestMessage * result;
	result = dynamic_cast<AppAllocateFlowRequestMessage *>(destination
			->getMessage());
	std::cout << "Received message from " << result->getSourcePortId()
			<< " with sequence number " << result->getSequenceNumber()
			<< "\n";
	std::cout << "Source application process name: "
			<< result->getSourceAppName().getProcessName() << "\n";
	std::cout << "Destination application process name: "
			<< result->getDestAppName().getProcessName() << "\n";
	std::cout << "In order delivery requested: "
			<< result->getFlowSpecification().isOrderedDelivery() << "\n";
	delete result;

	/* Test user-space to kernel communication */
	message =
			new AppAllocateFlowRequestMessage();
	message->setDestPortId(0);
	message->setSourceAppName(*sourceName);
	message->setDestAppName(*destName);
	message->setFlowSpecification(*flowSpec);
	source->sendMessage(message);
	delete message;

	BaseNetlinkMessage * fromKernel = source->getMessage();
	std::cout<<"Got message from "<<fromKernel->getSourcePortId()<<"\n";
	delete fromKernel;

	delete source;
	delete destination;
	delete sourceName;
	delete destName;
	delete flowSpec;
}
