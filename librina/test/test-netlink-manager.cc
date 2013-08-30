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
	NetlinkManager source = NetlinkManager(30);
	NetlinkManager destination = NetlinkManager(31);

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
	message.setDestPortId(31);
	message.setSourceAppName(sourceName);
	message.setDestAppName(destName);
	message.setFlowSpecification(flowSpec);
	message.setRequestMessage(true);
	message.setSequenceNumber(source.getSequenceNumber());
	message.setSourceIpcProcessId(25);
	message.setDestIpcProcessId(38);
	try{
		source.sendMessage(&message);
	}catch(NetlinkException &e){
		std::cout<<"Exception: "<<e.what()<<std::endl;
		return -1;
	}

	AppAllocateFlowRequestMessage * result;
	result = dynamic_cast<AppAllocateFlowRequestMessage *>(destination
			.getMessage());
	std::cout << "Received message from " << result->getSourcePortId()
			<< " with sequence number " << result->getSequenceNumber()
			<< " source IPC Process id "<< result->getSourceIpcProcessId()
			<< " destination IPC Process id "<< result->getDestIpcProcessId()
			<< "\n";
	std::cout << "Source application process name: "
			<< result->getSourceAppName().getProcessName() << "\n";
	std::cout << "Destination application process name: "
			<< result->getDestAppName().getProcessName() << "\n";
	std::cout << "In order delivery requested: "
			<< result->getFlowSpecification().isOrderedDelivery() << "\n";
	delete result;

	/* Test user-space to kernel communication */
	IpcmAssignToDIFResponseMessage message2;
	message.setDestPortId(0);
	try{
		source.sendMessage(&message2);
	}catch(NetlinkException &e){
		std::cout<<"Exception: "<<e.what()<<std::endl;
		return -1;
	}
	std::cout<<"Sent message to Kernel"<<std::endl;

	BaseNetlinkMessage * fromKernel = source.getMessage();
	std::cout<<"Got message from "<<fromKernel->getSourcePortId()<<"\n";
	delete fromKernel;
}
