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

	DIFConfiguration difConfiguration;
	difConfiguration.setDifType("shim-dummy");
	difConfiguration.setDifName(difName);

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
	std::cout<<"In order delivery: "<<
			result3->getFlowSpec().isOrderedDelivery()<<std::endl;
	std::cout<<"DIF Name: "<<result3->getDifName().getProcessName()<<std::endl;
	delete fromKernel;

	/*Test sending IpcmAllocateFlowRequestArrivedMessage */
	IpcmAllocateFlowRequestArrivedMessage message5;
	message5.setDestPortId(0);
	message5.setSourceAppName(sourceName);
	message5.setDestAppName(destName);
	message5.setFlowSpecification(flowSpec);
	message5.setDifName(difName);
	message5.setRequestMessage(true);
	message5.setSequenceNumber(source.getSequenceNumber());
	message5.setSourceIpcProcessId(6);
	message5.setDestIpcProcessId(34);
	try{
		source.sendMessage(&message5);
	}catch(NetlinkException &e){
		std::cout<<"Exception: "<<e.what()<<std::endl;
		return -1;
	}
	std::cout<<"Sent IpcmAllocateFlowRequestArrivedMessage message to Kernel"
			<<std::endl;

	fromKernel = source.getMessage();
	std::cout<<"Got message from "<<fromKernel->getSourcePortId()<<"\n";
	IpcmAllocateFlowRequestArrivedMessage * result5 =
			dynamic_cast<IpcmAllocateFlowRequestArrivedMessage *>(fromKernel);
	std::cout<<"Source IPC Process id "<<result5->getSourceIpcProcessId()
									<<std::endl;
	std::cout<<"Destination IPC Process id "<<result5->getDestIpcProcessId()
									<<std::endl;
	std::cout<<"Source application AP name "<<
			result5->getSourceAppName().getProcessName()<<std::endl;
	std::cout<<"Destination application AP name "<<
			result5->getDestAppName().getProcessName()<<std::endl;
	std::cout<<"In order delivery: "<<
				result5->getFlowSpecification().isOrderedDelivery()<<std::endl;
	std::cout<<"DIF Name: "<<result5->getDifName().getProcessName()<<std::endl;
	delete fromKernel;

	/*Test sending IpcmAllocateFlowResponseMessage */
	IpcmAllocateFlowResponseMessage message6;
	message6.setDestPortId(0);
	message6.setResult(38);
	message6.setNotifySource(true);
	message6.setPortId(234);
	message6.setRequestMessage(true);
	message6.setSequenceNumber(source.getSequenceNumber());
	message6.setSourceIpcProcessId(11);
	message6.setDestIpcProcessId(90);
	try{
		source.sendMessage(&message6);
	}catch(NetlinkException &e){
		std::cout<<"Exception: "<<e.what()<<std::endl;
		return -1;
	}
	std::cout<<"Sent IpcmAllocateFlowResponseMessage message to Kernel"
			<<std::endl;

	fromKernel = source.getMessage();
	std::cout<<"Got message from "<<fromKernel->getSourcePortId()<<"\n";
	IpcmAllocateFlowResponseMessage * result6 =
			dynamic_cast<IpcmAllocateFlowResponseMessage *>(fromKernel);
	std::cout<<"Source IPC Process id "<<result6->getSourceIpcProcessId()
												<<std::endl;
	std::cout<<"Destination IPC Process id "<<result6->getDestIpcProcessId()
												<<std::endl;
	std::cout<<"Result "<<
			result6->getResult()<<std::endl;
	std::cout<<"Notify source? "<<
			result6->isNotifySource()<<std::endl;
	std::cout<<"Port id: "<<
			result6->getPortId()<<std::endl;
	delete fromKernel;

	/*Test sending IpcmDeallocateFlowRequestMessage */
	IpcmDeallocateFlowRequestMessage message7;
	message7.setDestPortId(0);
	message7.setPortId(234);
	message7.setRequestMessage(true);
	message7.setSequenceNumber(source.getSequenceNumber());
	message7.setSourceIpcProcessId(21);
	message7.setDestIpcProcessId(54);
	try{
		source.sendMessage(&message7);
	}catch(NetlinkException &e){
		std::cout<<"Exception: "<<e.what()<<std::endl;
		return -1;
	}
	std::cout<<"Sent IpcmDeallocateFlowRequestMessage message to Kernel"
			<<std::endl;

	fromKernel = source.getMessage();
	std::cout<<"Got message from "<<fromKernel->getSourcePortId()<<"\n";
	IpcmDeallocateFlowRequestMessage * result7 =
			dynamic_cast<IpcmDeallocateFlowRequestMessage *>(fromKernel);
	std::cout<<"Source IPC Process id "<<result7->getSourceIpcProcessId()
															<<std::endl;
	std::cout<<"Destination IPC Process id "<<result7->getDestIpcProcessId()
															<<std::endl;;
	std::cout<<"Port id: "<<
			result7->getPortId()<<std::endl;
	delete fromKernel;

	/*Test sending IpcmDeallocateFlowResponseMessage */
	IpcmDeallocateFlowResponseMessage message8;
	message8.setDestPortId(0);
	message8.setResult(33);
	message8.setRequestMessage(true);
	message8.setSequenceNumber(source.getSequenceNumber());
	message8.setSourceIpcProcessId(13);
	message8.setDestIpcProcessId(98);
	try{
		source.sendMessage(&message8);
	}catch(NetlinkException &e){
		std::cout<<"Exception: "<<e.what()<<std::endl;
		return -1;
	}
	std::cout<<"Sent IpcmDeallocateFlowResponseMessage message to Kernel"
			<<std::endl;

	fromKernel = source.getMessage();
	std::cout<<"Got message from "<<fromKernel->getSourcePortId()<<"\n";
	IpcmDeallocateFlowResponseMessage * result8 =
			dynamic_cast<IpcmDeallocateFlowResponseMessage *>(fromKernel);
	std::cout<<"Source IPC Process id "<<result8->getSourceIpcProcessId()
			<<std::endl;
	std::cout<<"Destination IPC Process id "<<result8->getDestIpcProcessId()
			<<std::endl;;
	std::cout<<"Result: "<<
			result8->getResult()<<std::endl;

	/*Test sending IpcmFlowDeallocatedNotificationMessage */
	IpcmFlowDeallocatedNotificationMessage message9;
	message9.setDestPortId(0);
	message9.setCode(32);
	message9.setPortId(543);
	message9.setRequestMessage(true);
	message9.setSequenceNumber(source.getSequenceNumber());
	message9.setSourceIpcProcessId(21);
	message9.setDestIpcProcessId(34);
	try{
		source.sendMessage(&message9);
	}catch(NetlinkException &e){
		std::cout<<"Exception: "<<e.what()<<std::endl;
		return -1;
	}
	std::cout<<"Sent IpcmFlowDeallocatedNotificationMessage message to Kernel"
			<<std::endl;

	fromKernel = source.getMessage();
	std::cout<<"Got message from "<<fromKernel->getSourcePortId()<<"\n";
	IpcmFlowDeallocatedNotificationMessage * result9 =
			dynamic_cast<IpcmFlowDeallocatedNotificationMessage *>(fromKernel);
	std::cout<<"Source IPC Process id "<<result9->getSourceIpcProcessId()
			<<std::endl;
	std::cout<<"Destination IPC Process id "<<result9->getDestIpcProcessId()
			<<std::endl;;
	std::cout<<"Code: "<<
			result9->getCode()<<std::endl;
	std::cout<<"Port id: "<<
			result9->getPortId()<<std::endl;
	delete fromKernel;

	/*Test sending IpcmRegisterApplicationRequestMessage */
	IpcmRegisterApplicationRequestMessage message10;
	message10.setDestPortId(0);
	message10.setApplicationName(sourceName);
	message10.setDifName(difName);
	message10.setRequestMessage(true);
	message10.setSequenceNumber(source.getSequenceNumber());
	message10.setSourceIpcProcessId(21);
	message10.setDestIpcProcessId(34);
	try{
		source.sendMessage(&message10);
	}catch(NetlinkException &e){
		std::cout<<"Exception: "<<e.what()<<std::endl;
		return -1;
	}
	std::cout<<"Sent IpcmRegisterApplicationRequestMessage message to Kernel"
			<<std::endl;

	fromKernel = source.getMessage();
	std::cout<<"Got message from "<<fromKernel->getSourcePortId()<<"\n";
	IpcmRegisterApplicationRequestMessage * result10 =
			dynamic_cast<IpcmRegisterApplicationRequestMessage *>(fromKernel);
	std::cout<<"Source IPC Process id "<<result10->getSourceIpcProcessId()
			<<std::endl;
	std::cout<<"Destination IPC Process id "<<result10->getDestIpcProcessId()
			<<std::endl;;
	std::cout<<"Application process name: "<<
			result10->getApplicationName().getProcessName()<<std::endl;
	std::cout<<"DIF name: "<<
			result10->getDifName().getProcessName()<<std::endl;
	delete fromKernel;

	/*Test sending IpcmRegisterApplicationResponseMessage */
	IpcmRegisterApplicationResponseMessage message11;
	message11.setDestPortId(0);
	message11.setResult(231);
	message11.setRequestMessage(true);
	message11.setSequenceNumber(source.getSequenceNumber());
	message11.setSourceIpcProcessId(21);
	message11.setDestIpcProcessId(34);
	try{
		source.sendMessage(&message11);
	}catch(NetlinkException &e){
		std::cout<<"Exception: "<<e.what()<<std::endl;
		return -1;
	}
	std::cout<<"Sent IpcmRegisterApplicationResponseMessage message to Kernel"
			<<std::endl;

	fromKernel = source.getMessage();
	std::cout<<"Got message from "<<fromKernel->getSourcePortId()<<"\n";
	IpcmRegisterApplicationResponseMessage * result11 =
			dynamic_cast<IpcmRegisterApplicationResponseMessage *>(fromKernel);
	std::cout<<"Source IPC Process id "<<result11->getSourceIpcProcessId()
						<<std::endl;
	std::cout<<"Destination IPC Process id "<<result11->getDestIpcProcessId()
						<<std::endl;;
	std::cout<<"Result: "<<
			result11->getResult()<<std::endl;
	delete fromKernel;

	/*Test sending IpcmRegisterApplicationRequestMessage */
	IpcmUnregisterApplicationRequestMessage message12;
	message12.setDestPortId(0);
	message12.setApplicationName(sourceName);
	message12.setDifName(difName);
	message12.setRequestMessage(true);
	message12.setSequenceNumber(source.getSequenceNumber());
	message12.setSourceIpcProcessId(21);
	message12.setDestIpcProcessId(34);
	try{
		source.sendMessage(&message12);
	}catch(NetlinkException &e){
		std::cout<<"Exception: "<<e.what()<<std::endl;
		return -1;
	}
	std::cout<<"Sent IpcmUnregisterApplicationRequestMessage message to Kernel"
			<<std::endl;

	fromKernel = source.getMessage();
	std::cout<<"Got message from "<<fromKernel->getSourcePortId()<<"\n";
	IpcmUnregisterApplicationRequestMessage * result12 =
			dynamic_cast<IpcmUnregisterApplicationRequestMessage *>(fromKernel);
	std::cout<<"Source IPC Process id "<<result12->getSourceIpcProcessId()
						<<std::endl;
	std::cout<<"Destination IPC Process id "<<result12->getDestIpcProcessId()
						<<std::endl;;
	std::cout<<"Application process name: "<<
			result12->getApplicationName().getProcessName()<<std::endl;
	std::cout<<"DIF name: "<<
			result12->getDifName().getProcessName()<<std::endl;
	delete fromKernel;

	/*Test sending IpcmUnregisterApplicationResponseMessage */
	IpcmUnregisterApplicationResponseMessage message13;
	message13.setDestPortId(0);
	message13.setResult(431);
	message13.setRequestMessage(true);
	message13.setSequenceNumber(source.getSequenceNumber());
	message13.setSourceIpcProcessId(21);
	message13.setDestIpcProcessId(34);
	try{
		source.sendMessage(&message13);
	}catch(NetlinkException &e){
		std::cout<<"Exception: "<<e.what()<<std::endl;
		return -1;
	}
	std::cout<<"Sent IpcmUnregisterApplicationResponseMessage message to Kernel"
			<<std::endl;

	fromKernel = source.getMessage();
	std::cout<<"Got message from "<<fromKernel->getSourcePortId()<<"\n";
	IpcmUnregisterApplicationResponseMessage * result13 =
			dynamic_cast<IpcmUnregisterApplicationResponseMessage *>(fromKernel);
	std::cout<<"Source IPC Process id "<<result13->getSourceIpcProcessId()
									<<std::endl;
	std::cout<<"Destination IPC Process id "<<result13->getDestIpcProcessId()
									<<std::endl;;
	std::cout<<"Result: "<<
			result13->getResult()<<std::endl;
	delete fromKernel;

	/*Test sending IpcmAssignToDIFRequestMessage */
	IpcmAssignToDIFRequestMessage message14;
	message14.setDestPortId(0);
	message14.setDIFConfiguration(difConfiguration);
	message14.setRequestMessage(true);
	message14.setSequenceNumber(source.getSequenceNumber());
	message14.setSourceIpcProcessId(21);
	message14.setDestIpcProcessId(34);
	try{
		source.sendMessage(&message14);
	}catch(NetlinkException &e){
		std::cout<<"Exception: "<<e.what()<<std::endl;
		return -1;
	}
	std::cout<<"Sent IpcmAssignToDIFRequestMessage message to Kernel"
			<<std::endl;

	fromKernel = source.getMessage();
	std::cout<<"Got message from "<<fromKernel->getSourcePortId()<<"\n";
	IpcmAssignToDIFRequestMessage * result14 =
			dynamic_cast<IpcmAssignToDIFRequestMessage *>(fromKernel);
	std::cout<<"Source IPC Process id "<<result14->getSourceIpcProcessId()
												<<std::endl;
	std::cout<<"Destination IPC Process id "<<result14->getDestIpcProcessId()
												<<std::endl;;
	std::cout<<"DIF name: "<<
			result14->getDIFConfiguration().getDifName().getProcessName()
			<<std::endl;
	std::cout<<"DIF type: "<<
			result14->getDIFConfiguration().getDifType()<<std::endl;
	delete fromKernel;
}
