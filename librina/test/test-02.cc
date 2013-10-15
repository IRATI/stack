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
#include "librina.h"
#include "core.h"

using namespace rina;

bool checkIPCProcesses(unsigned int expectedProcesses) {
	std::vector<IPCProcess *> ipcProcesses = ipcProcessFactory
			->listIPCProcesses();
	if (ipcProcesses.size() != expectedProcesses) {
		std::cout << "ERROR: Expected " << expectedProcesses
				<< " IPC Processes, but only found " + ipcProcesses.size()
				<< "\n";
		return false;
	}

	std::cout << "Ids of existing processes:";
	for (unsigned int i = 0; i < ipcProcesses.size(); i++) {
		std::cout << " " << ipcProcesses.at(i)->getId() << ",";
	}
	std::cout << "\n";

	return true;
}

int main(int argc, char * argv[]) {
	std::cout << "TESTING LIBRINA-IPCMANAGER\n";

	/* TEST LIST IPC PROCESS TYPES */
	std::list<std::string> ipcProcessTypes = ipcProcessFactory->getSupportedIPCProcessTypes();
	std::list<std::string>::const_iterator iterator;
	for (iterator = ipcProcessTypes.begin();
			iterator != ipcProcessTypes.end();
			++iterator) {
		std::cout<<*iterator<<std::endl;
	}

	/* TEST CREATE IPC PROCESS */
	ApplicationProcessNamingInformation * ipcProcessName1 =
			new ApplicationProcessNamingInformation(
					"/ipcprocess/i2CAT/Barcelona", "1");
	ApplicationProcessNamingInformation * ipcProcessName2 =
			new ApplicationProcessNamingInformation(
					"/ipcprocess/i2CAT/Barcelona/shim", "1");
	ApplicationProcessNamingInformation * sourceName =
			new ApplicationProcessNamingInformation("/apps/test/source", "1");
	ApplicationProcessNamingInformation * destinationName =
			new ApplicationProcessNamingInformation("/apps/test/destination",
					"1");
	ApplicationProcessNamingInformation * difName =
			new ApplicationProcessNamingInformation("/difs/Test.DIF", "");

	IPCProcess * ipcProcess1 = ipcProcessFactory->create(*ipcProcessName1,
			"normal");
	IPCProcess * ipcProcess2 = ipcProcessFactory->create(*ipcProcessName2,
			"shim-ethernet");

	/* TEST LIST IPC PROCESSES */
	if (!checkIPCProcesses(2)) {
		return 1;
	}

	/* TEST DESTROY */
	ipcProcessFactory->destroy(ipcProcess2->getId());
	if (!checkIPCProcesses(1)) {
		return 1;
	}

	/* TEST ASSIGN TO DIF */
	DIFInformation * difInformation = new DIFInformation();
	ipcProcess1->assignToDIF(*difInformation);
	ipcProcess1->assignToDIFResult(true);

	/* TEST REGISTER APPLICATION */
	ipcProcess1->registerApplication(*sourceName);

	/* TEST UNREGISTER APPLICATION */
	ipcProcess1->unregisterApplication(*sourceName);

	/* TEST ALLOCATE FLOW */
	FlowSpecification *flowSpec = new FlowSpecification();
	FlowRequestEvent * flowRequest = new FlowRequestEvent(*flowSpec,
			true, *sourceName, *difName, 1234);
	flowRequest->setPortId(430);
	ipcProcess1->allocateFlow(*flowRequest);

	/* TEST QUERY RIB */
	ipcProcess1->queryRIB("list of flows",
			"/dif/management/flows/", 0, 0, "");

	/* TEST APPLICATION REGISTERED */
	ApplicationRegistrationInformation appRegInfo =
			ApplicationRegistrationInformation(APPLICATION_REGISTRATION_SINGLE_DIF);
	appRegInfo.setDIFName(*difName);
	ApplicationRegistrationRequestEvent * event = new
			ApplicationRegistrationRequestEvent(*sourceName, appRegInfo, 34);
	applicationManager->applicationRegistered(*event, *difName, 0);

	/* TEST APPLICATION UNREGISTERED */
	ApplicationUnregistrationRequestEvent * event2 = new
			ApplicationUnregistrationRequestEvent(*sourceName, *difName, 34);
	applicationManager->applicationUnregistered(*event2, 0);

	/* TEST FLOW ALLOCATED */
	FlowRequestEvent * flowEvent = new FlowRequestEvent(25, *flowSpec,
			true, *sourceName, *destinationName, *difName, 3);
	applicationManager->flowAllocated(*flowEvent);

	ipcProcessFactory->destroy(ipcProcess1->getId());
	if (!checkIPCProcesses(0)) {
		return 1;
	}

	delete ipcProcessName1;
	delete ipcProcessName2;
	delete sourceName;
	delete destinationName;
	delete difName;
	delete difInformation;
	delete flowSpec;
	delete flowRequest;
	return 0;
}
