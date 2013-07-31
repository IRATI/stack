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
	setNetlinkPortId(5);

	usleep(1000*200);

	ApplicationProcessNamingInformation appName;
	appName.setProcessName("/apps/source");
	appName.setProcessInstance("12");
	appName.setEntityName("database");
	appName.setEntityInstance("12");

	ApplicationProcessNamingInformation difName;
	difName.setProcessName("/difs/Test.DIF");

	std::list<DIFProperties> difProperties;

	try{
		std::list<DIFProperties> difProperties =
				ipcManager->getDIFProperties(appName, difName);
		std::cout<<"Got DIF Properties"<<std::endl;
	}catch(IPCException &e){
		std::cout<<"Problems getting DIF properties application: "
				<<e.what()<<std::endl;
		exit(-1);
	}

	try{
		ApplicationRegistrationInformation appRegInfo =
				ApplicationRegistrationInformation(
						APPLICATION_REGISTRATION_SINGLE_DIF);
		appRegInfo.setDIFName(difName);
		ipcManager->registerApplication(appName, appRegInfo);
		std::cout<<"Application# Application registered!\n";
	}catch(IPCException &e){
		std::cout<<"Problems registering application: "<<e.what()<<"\n";
		exit(-1);
	}

	ApplicationProcessNamingInformation destName;
	destName.setProcessName("/apps/dest");
	destName.setProcessInstance("12345");
	destName.setEntityName("printer");
	destName.setEntityInstance("12623456");

	FlowSpecification flowSpec;

	Flow * flow = 0;
	try{
		flow = ipcManager->allocateFlowRequest(appName, destName, flowSpec);
		std::cout<<"Application# Flow allocated by DIF "<<
				flow->getDIFName().getProcessName() << std::endl;
		ipcManager->deallocateFlow(flow->getPortId());
		std::cout<<"Application# Flow deallocated!"<<std::endl;

		ipcManager->unregisterApplication(appName, difName);
		std::cout<<"Application# Application unregistered!\n";
	}catch(IPCException &e){
		std::cout<<"Problems "<<e.what()<<"\n";
		result = -1;
	}

	std::cout<<"Application# Work done, terminating!"<<std::endl;
	exit(result);
}

void doWorkIPCProcess(){
	std::cout<<"IPC process pid: "<<getpid()<<std::endl;
	//Child process, this will be the IPC Process
	int result = 0;
	//unsigned short ipcProcessId = 1;
	setNetlinkPortId(4);

	//Wait for Registration to DIF notification event
	IPCEvent * event = ipcEventProducer->eventWait();
	IPCProcessRegisteredToDIFEvent * ipcRegNotEvent =
			dynamic_cast<IPCProcessRegisteredToDIFEvent *>(event);
	std::cout<<"IPCProcess# Received an IPC Process registered to DIF event "
			<<ipcRegNotEvent->getSequenceNumber()<<std::endl;

	//Wait for an assign to DIF event
	event = ipcEventProducer->eventWait();
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

	//Wait for a flow allocation event
	event = ipcEventProducer->eventWait();
	FlowRequestEvent * flowRequestEvent =
			dynamic_cast<FlowRequestEvent *>(event);
	std::cout<<"IPCProcess# Received flow request event"
			<<std::endl;
	extendedIPCManager->allocateFlowRequestResult(
			*flowRequestEvent, 0, "ok");
	std::cout<<"IPCProcess# Replied IPC Manager"<<std::endl;
	delete flowRequestEvent;

	//Wait for a flow deallocation event
	event = ipcEventProducer->eventWait();
	FlowDeallocateRequestEvent * deallocateFlowEvent =
			dynamic_cast<FlowDeallocateRequestEvent *>(event);
	std::cout<<"IPCProcess# received a flow deallocation event\n";
	ipcProcessApplicationManager->flowDeallocated(
			*deallocateFlowEvent, 0, "ok");
	std::cout<<"ICPProcess# Replied to application\n";
	delete deallocateFlowEvent;

	//Wait for an unregister application event
	event = ipcEventProducer->eventWait();
	ApplicationUnregistrationRequestEvent * applicationUnregistrationEvent =
			dynamic_cast<ApplicationUnregistrationRequestEvent *>(event);
	std::cout<<"IPCProcess# Received application unregistration event"
			<<std::endl;
	extendedIPCManager->unregisterApplicationResponse(
			*applicationUnregistrationEvent, 0, "ok");
	std::cout<<"IPCProcess# Replied IPC Manager"<<std::endl;
	delete applicationUnregistrationEvent;


	//Wait for query RIB Event
	event = ipcEventProducer->eventWait();
	QueryRIBRequestEvent * queryRIBEvent =
			dynamic_cast<QueryRIBRequestEvent *>(event);
	std::cout<<"IPCProcess# Received query RIB request event"<<std::endl;
	std::list<RIBObject> ribObjects;
	RIBObject ribObject;
	ribObject.setClazz("flow");
	ribObject.setName("/dif/management/flows/23");
	ribObject.setInstance(123456);
	ribObjects.push_back(ribObject);
	RIBObject ribObject2;
	ribObject2.setClazz("flow");
	ribObject2.setName("/dif/management/flows/24");
	ribObject2.setInstance(12345677);
	ribObjects.push_back(ribObject2);
	extendedIPCManager->queryRIBResponse(*queryRIBEvent, 0, "", ribObjects);
	std::cout<<"IPCProcess# Replied IPC Manager"<<std::endl;
	delete queryRIBEvent;

	//Wait for Unregistration from DIF notification event
	event = ipcEventProducer->eventWait();
	IPCProcessUnregisteredFromDIFEvent * ipcUnregNotEvent =
			dynamic_cast<IPCProcessUnregisteredFromDIFEvent *>(event);
	std::cout<<"IPCProcess# Received an IPC Process unregistered from DIF event "
			<<ipcUnregNotEvent->getSequenceNumber()<<std::endl;

	std::cout<<"IPCProcess# Work done, terminating!"<<std::endl;
	exit(result);
}

int doWorkIPCManager(pid_t appPID, pid_t ipcPID){
	//Parent process, this will be the IPC Manager
	setNetlinkPortId(1);

	usleep(1000*50);
	//Create IPC Process
	ApplicationProcessNamingInformation processName;
	processName.setProcessName(
			"/ipcprocesses/Barcelona/i2CAT/25");
	processName.setProcessInstance("1");
	IPCProcess * ipcProcess = ipcProcessFactory->create(
			processName, "shim-dummy");
	ipcProcess->setPortId(4);
	std::cout<<"IPCManager# Created IPC Process with id " <<
			ipcProcess->getId()<<std::endl;

	//Inform IPC Process that it has been registered to underlying DIF
	processName.setEntityName("Management");
	processName.setEntityInstance("1234");
	ApplicationProcessNamingInformation supportingDifName;
	supportingDifName.setProcessName("/difs/supporting.DIF");
	ipcProcess->notifyRegistrationToSupportingDIF(
			processName, supportingDifName);
	std::cout<<"IPCManager# Informed IPC Process about registration to "<<
			"supporting DIF"<< supportingDifName.getProcessName()<<std::endl;

	//Assign the IPC Process to a DIF
	ApplicationProcessNamingInformation difName;
	difName.setProcessName("/difs/Test.DIF");
	DIFConfiguration difConfiguration;
	difConfiguration.setDifType("shim-dummy");
	difConfiguration.setDifName(difName);
	ipcProcess->assignToDIF(difConfiguration);
	std::cout<<"IPCManager# Assigned IPC Process to DIF "<<
			difName.getProcessName()<<std::endl;

	//Wait for Get DIF Properties Event
	IPCEvent * event = ipcEventProducer->eventWait();
	GetDIFPropertiesRequestEvent * getPropertiesEvent =
			dynamic_cast<GetDIFPropertiesRequestEvent *>(event);
	std::cout<<"IPCManager# received a get DIF properties request event\n";
	std::list<DIFProperties> difProperties;
	DIFProperties difProperty =
			DIFProperties(getPropertiesEvent->getDIFName(), 9000);
	difProperties.push_back(difProperty);
	applicationManager->getDIFPropertiesResponse(
			*getPropertiesEvent, 0, "", difProperties);
	std::cout<<"IPCManager# Replied to application\n";
	delete getPropertiesEvent;

	//Wait for a Register application event
	event = ipcEventProducer->eventWait();
	ApplicationRegistrationRequestEvent * appRequestEvent =
			dynamic_cast<ApplicationRegistrationRequestEvent *>(event);
	std::cout<<"IPCManager# received an application registration request event\n";
	ipcProcess->registerApplication(appRequestEvent->getApplicationName());
	std::cout<<"IPCManager# IPC Process successfully registered application " <<
			"to DIF "<<difName.getProcessName()<<std::endl;
	applicationManager->applicationRegistered(
			*appRequestEvent, difName, 0, "ok");
	std::cout<<"IPCManager# Replied to application\n";
	delete appRequestEvent;

	//Wait for a flow allocation event
	event = ipcEventProducer->eventWait();
	FlowRequestEvent * flowRequestEvent =
			dynamic_cast<FlowRequestEvent *>(event);
	std::cout<<"IPCManager# received a flow allocation request event\n";
	flowRequestEvent->setDIFName(difName);
	flowRequestEvent->setPortId(23);
	ipcProcess->allocateFlow(*flowRequestEvent);
	std::cout<<"IPCManager# IPC Process successfully allocated flow" <<
			"in DIF "<<difName.getProcessName()<<std::endl;
	applicationManager->flowAllocated(*flowRequestEvent, "ok");
	std::cout<<"IPCManager# Replied to flow allocation\n";
	delete flowRequestEvent;

	usleep(1000*50);

	//Wait for a unegister application event
	event = ipcEventProducer->eventWait();
	ApplicationUnregistrationRequestEvent * appUnregistrationRequestEvent =
			dynamic_cast<ApplicationUnregistrationRequestEvent *>(event);
	std::cout<<"IPCManager# received an application unregistration request event\n";
	ipcProcess->unregisterApplication(appUnregistrationRequestEvent->getApplicationName());
	std::cout<<"IPCManager# IPC Process successfully unregistered application " <<
			"to DIF "<<difName.getProcessName()<<std::endl;
	applicationManager->applicationUnregistered(*appUnregistrationRequestEvent, 0, "OK");
	std::cout<<"IPCManager# Replied to application\n";
	delete appUnregistrationRequestEvent;

	//Query RIB of IPC Process
	std::list<RIBObject> ribObjects = ipcProcess->queryRIB("list of flows",
			"/dif/management/flows/", 0, 0, "");
	std::cout<<"IPCManager# Queried RIB of IPC Process and got "<<
			ribObjects.size() << " objects back"<<std::endl;

	//Inform IPC Process that it has been unregistered from underlying DIF
	ipcProcess->notifyUnregistrationFromSupportingDIF(
			processName, supportingDifName);
	std::cout<<"IPCManager# Informed IPC Process about unregistration from "<<
			"supportind DIF"<< supportingDifName.getProcessName()<<std::endl;

	//Destroy IPC Process
	ipcProcessFactory->destroy(ipcProcess->getId());
	std::cout<<"IPCManager# Destroyed IPC Process"<<std::endl;

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
