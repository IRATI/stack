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

bool checkAllocatedFlows(IPCManager* ipcManager, unsigned int expectedFlows) {
	std::vector<Flow *> allocatedFlows = ipcManager->getAllocatedFlows();
	if (allocatedFlows.size() != expectedFlows) {
		std::cout << "ERROR: Expected " << expectedFlows
				<< " allocated flows, but only found " + allocatedFlows.size()
				<< "²n";
		return false;
	}

	std::cout << "Port-ids of allocated flows:";
	for (unsigned int i = 0; i < allocatedFlows.size(); i++) {
		std::cout << " " << allocatedFlows.at(i)->getPortId() << ",";
	}
	std::cout << "\n";

	return true;
}

bool checkRegisteredApplications(IPCManager* ipcManager,
		unsigned int expectedFlows) {
	std::vector<ApplicationRegistration *> registeredApplications =
			ipcManager->getRegisteredApplications();
	if (registeredApplications.size() != expectedFlows) {
		std::cout << "ERROR: Expected " << expectedFlows
				<< " registered applications, but only found "
						+ registeredApplications.size() << "²n";
		return false;
	}

	ApplicationRegistration* applicationRegistration;
	for (unsigned int i = 0; i < registeredApplications.size(); i++) {
		applicationRegistration = registeredApplications.at(i);
		std::cout << "Application "
				<< applicationRegistration->getApplicationName().getProcessName()
				<< " registered at DIFs: ";
		std::list<ApplicationProcessNamingInformation>::const_iterator iterator;
		for (iterator = applicationRegistration->getDIFNames().begin();
				iterator != applicationRegistration->getDIFNames().end();
				++iterator) {
			std::cout << iterator->getProcessName() << ", ";
		}
		std::cout << "\n";
	}

	return true;
}

int main(int argc, char * argv[]) {
	IPCManager * ipcManager = IPCManager::getInstance();
	ApplicationProcessNamingInformation * sourceName =
			new ApplicationProcessNamingInformation("/apps/test/source", "1");
	ApplicationProcessNamingInformation * destinationName =
			new ApplicationProcessNamingInformation("/apps/test/destination",
					"1");
	FlowSpecification * flowSpecification = new FlowSpecification();

	/* TEST ALLOCATE REQUEST */
	Flow * flow = ipcManager->allocateFlowRequest(*sourceName, *destinationName,
			*flowSpecification);
	std::cout << "Flow allocated, portId is " << flow->getPortId()
			<< "; DIF name is: " << flow->getDIFName().getProcessName() << "\n";

	/* TEST WRITE SDU */
	unsigned char sdu[] = { 45, 34, 2, 36, 8 };
	flow->writeSDU(sdu, 5);

	/* TEST ALLOCATE RESPONSE */
	Flow * flow2 = ipcManager->allocateFlowResponse(25, true, "");
	std::cout << "Accepted flow allocation, portId is " << flow2->getPortId()
			<< "; DIF name is: " << flow2->getDIFName().getProcessName()
			<< "\n";

	/* TEST READ SDU */
	int bytesRead = flow2->readSDU(sdu);
	std::cout << "Read an SDU of " << bytesRead << " bytes. Contents: \n";
	for (int i = 0; i < bytesRead; i++) {
		std::cout << "SDU[" << i << "]: " << sdu[i] << "\n";
	}

	/* TEST GET ALLOCATED FLOWS */
	if (!checkAllocatedFlows(ipcManager, 2)) {
		return 1;
	}

	/* TEST GET DIF PROPERTIES */
	std::vector<DIFProperties> difPropertiesVector =
			ipcManager->getDIFProperties(flow->getDIFName());
	DIFProperties difProperties = difPropertiesVector.at(0);
	std::cout << "DIF name: " << difProperties.getDifName().getProcessName()
			<< "; max SDU size: " << difProperties.getMaxSduSize() << "\n";

	/* TEST DEALLOCATE FLOW */
	ipcManager->deallocateFlow(flow->getPortId());
	if (!checkAllocatedFlows(ipcManager, 1)) {
		return 1;
	}

	ipcManager->deallocateFlow(flow2->getPortId());
	if (!checkAllocatedFlows(ipcManager, 0)) {
		return 1;
	}

	/* TEST REGISTER APPLICATION */
	ipcManager->registerApplication(*sourceName, difProperties.getDifName());
	ApplicationProcessNamingInformation * difName =
				new ApplicationProcessNamingInformation("/difs/PublicInternet.DIF",
						"");
	ipcManager->registerApplication(*sourceName, *difName);
	ipcManager->registerApplication(*destinationName,
			difProperties.getDifName());

	/* TEST GET REGISTERED APPLICATIONS */
	if (!checkRegisteredApplications(ipcManager, 2)) {
		return 1;
	}

	/* TEST UNREGISTER APPLICATION */
	ipcManager->unregisterApplication(*sourceName, difProperties.getDifName());
	ipcManager->unregisterApplication(*sourceName, *difName);
	if (!checkRegisteredApplications(ipcManager, 1)) {
		return 1;
	}
	ipcManager->unregisterApplication(*destinationName,
				difProperties.getDifName());
	if (!checkRegisteredApplications(ipcManager, 0)) {
			return 1;
		}

	delete flow;
	delete flow2;
	delete sourceName;
	delete destinationName;
	delete difName;

	return 0;
}
