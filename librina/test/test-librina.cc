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
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "core.h"
#include "librina-application.h"
#include "librina-ipc-manager.h"
#include "librina-ipc-process.h"

using namespace rina;

void doWorkApplicationProcess(){
	std::cout<<"Application process pid: "<<getpid()<<std::endl;
	//Child process, this will be the application
	int result = 0;
	setNetlinkPortId(3);

	usleep(1000*200);

	ApplicationProcessNamingInformation * appName =
			new ApplicationProcessNamingInformation();
	appName->setProcessName("/apps/source");
	appName->setProcessInstance("12");
	appName->setEntityName("database");
	appName->setEntityInstance("12");

	ApplicationProcessNamingInformation * difName =
			new ApplicationProcessNamingInformation();
	difName->setProcessName("/difs/Test.DIF");
	try{
		ipcManager->registerApplication(*appName, *difName);
		std::cout<<"Application# Application registered!\n";
	}catch(IPCException &e){
		std::cout<<"Problems registering application: "<<e.what()<<"\n";
		result = -1;
	}

	if (result == -1){
		delete appName;
		delete difName;
		exit(result);
	}

	ApplicationProcessNamingInformation * destName =
			new ApplicationProcessNamingInformation();
	destName->setProcessName("/apps/dest");
	destName->setProcessInstance("12345");
	destName->setEntityName("printer");
	destName->setEntityInstance("12623456");

	FlowSpecification * flowSpec = new FlowSpecification();

	Flow * flow = 0;
	try{
		flow = ipcManager->allocateFlowRequest(*appName, *destName, *flowSpec);
		std::cout<<"Application# Flow allocated by DIF "<<
				flow->getDIFName().getProcessName() << std::endl;
		ipcManager->deallocateFlow(flow->getPortId(), *appName);
		std::cout<<"Application# Flow deallocated!"<<std::endl;
	}catch(IPCException &e){
		std::cout<<"Problems "<<e.what()<<"\n";
		result = -1;
	}

	delete appName;
	delete difName;
	delete destName;
	delete flowSpec;
	std::cout<<"Application# Work done, terminating!"<<std::endl;
	exit(result);
}

void doWorkIPCProcess(){
	std::cout<<"IPC process pid: "<<getpid()<<std::endl;
	//Child process, this will be the IPC Process
	int result = 0;
	//unsigned short ipcProcessId = 1;
	setNetlinkPortId(2);

	//Wait for an assign to DIF event
	IPCEvent * event = ipcEventProducer->eventWait();
	AssignToDIFRequestEvent * assignToDIFEvent =
			dynamic_cast<AssignToDIFRequestEvent *>(event);
	std::cout<<"IPCProcess# Received an assign to DIF event"<<std::endl;
	extendedIPCManager->assignToDIFResponse(*assignToDIFEvent, 0, "ok");
	std::cout<<"IPCProcess# Replied IPC Manager"<<std::endl;
	delete assignToDIFEvent;

	//Wait for a register application event
	event = ipcEventProducer->eventWait();
	ApplicationRegistrationRequestEvent * applicationRegistrationEvent =
			dynamic_cast<ApplicationRegistrationRequestEvent *>(event);
	std::cout<<"IPCProcess# Received application registration event"
			<<std::endl;
	extendedIPCManager->registerApplicationResponse(
			*applicationRegistrationEvent, 0, "ok");
	std::cout<<"IPCProcess# Replied IPC Manager"<<std::endl;
	delete applicationRegistrationEvent;

	//Wait for a flow deallocation event
	event = ipcEventProducer->eventWait();
	FlowDeallocateRequestEvent * deallocateFlowEvent =
			dynamic_cast<FlowDeallocateRequestEvent *>(event);
	std::cout<<"IPCProcess# received a flow deallocation event\n";
	ipcProcessApplicationManager->flowDeallocated(
			*deallocateFlowEvent, 0, "ok");
	std::cout<<"ICPProcess# Replied to application\n";
	delete deallocateFlowEvent;

	std::cout<<"IPCProcess# Work done, terminating!"<<std::endl;
	exit(result);
}

int doWorkIPCManager(pid_t appPID, pid_t ipcPID){
	//Parent process, this will be the IPC Manager

	usleep(1000*50);
	//Create IPC Process
	ApplicationProcessNamingInformation * processName =
				new ApplicationProcessNamingInformation();
	processName->setProcessName(
			"/ipcprocesses/shimEthernet/Barcelona/i2CAT/25");
	processName->setProcessInstance("1");
	IPCProcess * ipcProcess = ipcProcessFactory->create(
			*processName, DIF_TYPE_SHIM_ETHERNET);
	std::cout<<"IPCManager# Created IPC Process with id " <<
			ipcProcess->getId()<<std::endl;

	//Assign the IPC Process to a DIF
	ApplicationProcessNamingInformation * difName =
			new ApplicationProcessNamingInformation();
	difName->setProcessName("/difs/Test.DIF");
	DIFConfiguration * difConfiguration = new DIFConfiguration();
	difConfiguration->setDifType(DIF_TYPE_SHIM_ETHERNET);
	difConfiguration->setDifName(*difName);
	ipcProcess->assignToDIF(*difConfiguration, 2);
	std::cout<<"IPCManager# Assigned IPC Process to DIF "<<
			difName->getProcessName()<<std::endl;

	//Wait for a Register application event
	IPCEvent * event = ipcEventProducer->eventWait();
	ApplicationRegistrationRequestEvent * appRequestEvent =
			dynamic_cast<ApplicationRegistrationRequestEvent *>(event);
	std::cout<<"IPCManager# received an application registration request event\n";
	ipcProcess->registerApplication(appRequestEvent->getApplicationName(), 3);
	std::cout<<"IPCManager# IPC Process successfully registered application " <<
			"to DIF "<<difName->getProcessName()<<std::endl;
	applicationManager->applicationRegistered(*appRequestEvent, 1, 2, 0, "ok");
	std::cout<<"IPCManager# Replied to application\n";
	delete appRequestEvent;

	//Wait for a flow allocation event
	event = ipcEventProducer->eventWait();
	FlowRequestEvent * flowRequestEvent =
			dynamic_cast<FlowRequestEvent *>(event);
	std::cout<<"IPCManager# received a flow allocation request event\n";
	flowRequestEvent->setDIFName(*difName);
	flowRequestEvent->setPortId(23);
	applicationManager->flowAllocated(*flowRequestEvent, "ok", 1, 2);
	std::cout<<"IPCManager# Replied to flow allocation\n";
	delete flowRequestEvent;

	//Destroy IPC Process
	ipcProcessFactory->destroy(ipcProcess->getId());
	std::cout<<"IPCManager# Destroyed IPC Process"<<std::endl;
	delete processName;
	delete difName;
	delete difConfiguration;

	//Wait for child termination
	int childExitStatus;
	pid_t ws = waitpid(appPID, &childExitStatus, 0);
	if (ws < 0){
		return -1;
	}

	ws = waitpid(ipcPID, &childExitStatus, 0);
	if (ws < 0){
		return -1;
	}

	return childExitStatus;
}

int main(int argc, char * argv[]) {
	std::cout << "TESTING LIBRINA\n";
	pid_t appPID, ipcPID;

	setNetlinkPortId(1);

	std::cout<<"Parent pid: "<<getpid()<<std::endl;

	appPID = fork();

	if (appPID < 0){
		std::cout<<"Fork failed, exiting\n";
		exit(-1);
	}

	if (appPID == 0){
		doWorkApplicationProcess();
	}else{
		ipcPID = fork();

		if (ipcPID < 0){
			std::cout<<"Fork failed, exiting\n";
			//TODO kill child
			exit(-1);
		}

		if (ipcPID == 0){
			doWorkIPCProcess();
		}else{
			return doWorkIPCManager(appPID, ipcPID);
		}
	}

}
