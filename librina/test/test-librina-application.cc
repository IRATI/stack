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
	std::cout<<"Appilcation process pid: "<<getpid()<<std::endl;
	//Child process, this will be the application
	int result = 0;
	setNetlinkPortId(2);

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
	exit(result);
}

void doWorkIPCProcess(){
	std::cout<<"IPC process pid: "<<getpid()<<std::endl;
	//Child process, this will be the IPC Process
	int result = 0;
	//unsigned short ipcProcessId = 1;
	setNetlinkPortId(3);

	IPCEvent * event = ipcEventProducer->eventWait();
	FlowDeallocateRequestEvent * deallocateFlowEvent =
			dynamic_cast<FlowDeallocateRequestEvent *>(event);
	std::cout<<"IPCProcess# received a flow deallocation event\n";
	ipcProcessApplicationManager->flowDeallocated(
			*deallocateFlowEvent, 0, "ok");
	std::cout<<"ICPProcess# Replied to application\n";
	delete deallocateFlowEvent;

	exit(result);
}

int doWorkIPCManager(pid_t appPID, pid_t ipcPID){
	//Parent process, this will be the IPC Manager
	//Wait for a Register application event
	IPCEvent * event = ipcEventProducer->eventWait();
	ApplicationRegistrationRequestEvent * appRequestEvent =
			dynamic_cast<ApplicationRegistrationRequestEvent *>(event);
	std::cout<<"IPCManager# received an application registration request event\n";
	applicationManager->applicationRegistered(
			*appRequestEvent, 1, 1, 0, "ok");
	std::cout<<"IPCManager# Replied to application\n";
	delete appRequestEvent;

	//Wait for a flow allocation event
	event = ipcEventProducer->eventWait();
	FlowRequestEvent * flowRequestEvent =
			dynamic_cast<FlowRequestEvent *>(event);
	std::cout<<"IPCManager# received a flow allocation request event\n";
	ApplicationProcessNamingInformation * difName =
			new ApplicationProcessNamingInformation();
	difName->setProcessName("/difs/Test.DIF");
	flowRequestEvent->setDIFName(*difName);
	flowRequestEvent->setPortId(23);
	applicationManager->flowAllocated(*flowRequestEvent, "ok", 1, 3);
	std::cout<<"IPCManager# Replied to flow allocation\n";
	delete flowRequestEvent;
	delete difName;

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
	std::cout << "TESTING LIBRINA-APPLICATION\n";
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
