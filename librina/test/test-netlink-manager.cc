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

	ApplicationProcessNamingInformation difName;
	difName.setProcessName("/difs/test.DIF");

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
	message2.setDestPortId(0);
	message2.setRequestMessage(true);
	message2.setSourceIpcProcessId(1);
	message2.setDestIpcProcessId(2);
	message2.setSequenceNumber(source.getSequenceNumber());
	message2.setResult(32);
	try{
		source.sendMessage(&message2);
	}catch(NetlinkException &e){
		std::cout<<"Exception: "<<e.what()<<std::endl;
		return -1;
	}
	std::cout<<"Sent IpcmAssignToDIFResponseMessage message to Kernel"
			<<std::endl;

	BaseNetlinkMessage * fromKernel = source.getMessage();
	std::cout<<"Got message from "<<fromKernel->getSourcePortId()<<"\n";
	IpcmAssignToDIFResponseMessage * result2 =
			dynamic_cast<IpcmAssignToDIFResponseMessage *>(fromKernel);
	std::cout<<"Source IPC Process id "<<result2->getSourceIpcProcessId()
			<<std::endl;
	std::cout<<"Destination IPC Process id "<<result2->getDestIpcProcessId()
			<<std::endl;
	std::cout<<"Result is "<<result2->getResult()<<std::endl;
	delete fromKernel;

	/* Test sending IpcmAllocateFlowRequestResultMessage */
	IpcmAllocateFlowRequestResultMessage message4;
	message4.setDestPortId(0);
	message4.setResult(76);
	message4.setRequestMessage(true);
	message4.setSequenceNumber(source.getSequenceNumber());
	message4.setSourceIpcProcessId(10);
	message4.setDestIpcProcessId(11);
	try{
		source.sendMessage(&message4);
	}catch(NetlinkException &e){
		std::cout<<"Exception: "<<e.what()<<std::endl;
		return -1;
	}
	std::cout<<"Sent IpcmAllocateFlowRequestResultMessage message to Kernel"
			<<std::endl;

	fromKernel = source.getMessage();
	std::cout<<"Got message from "<<fromKernel->getSourcePortId()<<"\n";
	IpcmAllocateFlowRequestResultMessage * result4 =
			dynamic_cast<IpcmAllocateFlowRequestResultMessage *>(fromKernel);
	std::cout<<"Source IPC Process id "<<result4->getSourceIpcProcessId()
						<<std::endl;
	std::cout<<"Destination IPC Process id "<<result4->getDestIpcProcessId()
						<<std::endl;
	std::cout<<"Result is "<<result4->getResult()<<std::endl;
	delete fromKernel;

	/*Test sending IpcmAllocateFlowRequestMessage */
	IpcmAllocateFlowRequestMessage message3;
	message3.setDestPortId(0);
	message3.setSourceAppName(sourceName);
	message3.setDestAppName(destName);
	message3.setFlowSpec(flowSpec);
	message3.setPortId(1);
	message3.setDifName(difName);
	message3.setRequestMessage(true);
	message3.setSequenceNumber(source.getSequenceNumber());
	message3.setSourceIpcProcessId(8);
	message3.setDestIpcProcessId(9);
	try{
		source.sendMessage(&message3);
	}catch(NetlinkException &e){
		std::cout<<"Exception: "<<e.what()<<std::endl;
		return -1;
	}
	std::cout<<"Sent IpcmAllocateFlowRequestMessage message to Kernel"
			<<std::endl;

	fromKernel = source.getMessage();
	std::cout<<"Got message from "<<fromKernel->getSourcePortId()<<"\n";
	IpcmAllocateFlowRequestMessage * result3 =
			dynamic_cast<IpcmAllocateFlowRequestMessage *>(fromKernel);
	std::cout<<"Source IPC Process id "<<result3->getSourceIpcProcessId()
						<<std::endl;
	std::cout<<"Destination IPC Process id "<<result3->getDestIpcProcessId()
						<<std::endl;
	std::cout<<"Source application AP name "<<
			result3->getSourceAppName().getProcessName()<<std::endl;
	std::cout<<"Destination application AP name "<<
			result3->getDestAppName().getProcessName()<<std::endl;
	std::cout<<"Port id: "<<result3->getPortId()<<std::endl;
	std::cout<<"DIF Name: "<<result3->getDifName().getProcessName()<<std::endl;
	delete fromKernel;
}
