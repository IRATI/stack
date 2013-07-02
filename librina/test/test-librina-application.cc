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

using namespace rina;

int main(int argc, char * argv[]) {
	std::cout << "TESTING LIBRINA-APPLICATION\n";

	setNetlinkPortId(1);

	pid_t pID = fork();

	if (pID < 0){
		std::cout<<"Fork failed, exiting\n";
		exit(-1);
	}

	if (pID == 0){
		std::cout<<"Child pid: "<<getpid()<<std::endl;
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

		try{
			ipcManager->allocateFlowRequest(*appName, *destName, *flowSpec);
			std::cout<<"Application# Flow allocated!\n";
		}catch(IPCException &e){
			std::cout<<"Problems allocating flow: "<<e.what()<<"\n";
			result = -1;
		}

		delete appName;
		delete difName;
		delete destName;
		delete flowSpec;
		exit(result);
	}else{
		std::cout<<"Parent pid: "<<getpid()<<std::endl;
		//Parent process, this will be the IPC Manager
		//Wait for a Register application event
		IPCEvent * event = ipcEventProducer->eventWait();
		ApplicationRegistrationRequestEvent * appRequestEvent =
				dynamic_cast<ApplicationRegistrationRequestEvent *>(event);
		std::cout<<"IPCManager# received an application registration request event\n";
		applicationManager->applicationRegistered(
				appRequestEvent->getSequenceNumber(),
				appRequestEvent->getApplicationName(),
				appRequestEvent->getDIFName(),
				1, 1, 0, "ok");
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
		applicationManager->flowAllocated(flowRequestEvent->getSequenceNumber(),
				23, "ok", 1, 1, flowRequestEvent->getSourceApplicationName(),
				*difName);
		std::cout<<"IPCManager# Replied to flow allocation\n";
		delete flowRequestEvent;
		delete difName;

		//Wait for child termination
		int childExitStatus;
		pid_t ws = waitpid(pID, &childExitStatus, 0);
		if (ws < 0){
			return -1;
		}

		return childExitStatus;
	}

}
