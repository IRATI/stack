//
// Test 02
//
//    Eduard Grasa          <eduard.grasa@i2cat.net>
//    Francesco Salvestrini <f.salvestrini@nextworks.it>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301  USA
//

#include <iostream>

#include "core.h"

#include "librina/librina.h"

using namespace rina;

int main() {
	std::cout << "TESTING LIBRINA-IPCMANAGER\n";

	IPCProcessFactory factory;
	/* TEST LIST IPC PROCESS TYPES */
	std::list<std::string> ipcProcessTypes = factory.getSupportedIPCProcessTypes();
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
	ApplicationProcessNamingInformation * dafName =
			new ApplicationProcessNamingInformation("/dafs/Test.DAF", "");

	IPCProcessProxy * ipcProcess1 = factory.create(*ipcProcessName1,
			"normal", 12);
	IPCProcessProxy * ipcProcess2 = factory.create(*ipcProcessName2,
			"shim-ethernet", 25);

	/* TEST DESTROY */
	factory.destroy(ipcProcess2);

	/* TEST ASSIGN TO DIF */
	DIFInformation * difInformation = new DIFInformation();
	ipcProcess1->assignToDIF(*difInformation, 35);

	/* TEST REGISTER APPLICATION */
	ipcProcess1->registerApplication(*sourceName, *dafName, 1, *difName, 45);

	/* TEST UNREGISTER APPLICATION */
	ipcProcess1->unregisterApplication(*sourceName, *difName, 34);

	/* TEST ALLOCATE FLOW */
	FlowSpecification *flowSpec = new FlowSpecification();
	FlowRequestEvent * flowRequest = new FlowRequestEvent(*flowSpec,
			true, *sourceName, *difName, 1234, 4545, 15, 20, 4);
	flowRequest->portId = 430;
	ipcProcess1->allocateFlow(*flowRequest, 23);

	/* TEST QUERY RIB */
	ipcProcess1->queryRIB("list of flows",
			"/dif/management/flows/", 0, 0, "", 526);

	/* TEST APPLICATION REGISTERED */
	ApplicationRegistrationInformation appRegInfo =
			ApplicationRegistrationInformation(APPLICATION_REGISTRATION_SINGLE_DIF);
	appRegInfo.difName = *difName;
	ApplicationRegistrationRequestEvent * event = new
			ApplicationRegistrationRequestEvent(appRegInfo, 34, 3, 4);
	applicationManager->applicationRegistered(*event, *difName, 0);

	/* TEST APPLICATION UNREGISTERED */
	ApplicationUnregistrationRequestEvent * event2 = new
			ApplicationUnregistrationRequestEvent(*sourceName, *difName, 34, 1, 2);
	applicationManager->applicationUnregistered(*event2, 0);

	/* TEST FLOW ALLOCATED */
	FlowRequestEvent * flowEvent = new FlowRequestEvent(25, *flowSpec,
			true, *sourceName, *destinationName, *difName, 3, 2323, 2, 3);
	applicationManager->flowAllocated(*flowEvent);

	factory.destroy(ipcProcess1);

	delete ipcProcessName1;
	delete ipcProcessName2;
	delete sourceName;
	delete destinationName;
	delete difName;
	delete dafName;
	delete difInformation;
	delete flowSpec;
	delete flowRequest;
	return 0;
}
