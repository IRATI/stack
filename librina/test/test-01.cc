//
// Test 01
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
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <sstream>

#include "core.h"

#include "librina/librina.h"

using namespace rina;

bool checkAllocatedFlows(unsigned int expectedFlows) {
        std::vector<FlowInformation> allocatedFlows = ipcManager->getAllocatedFlows();
        if (allocatedFlows.size() != expectedFlows) {
                std::cout << "ERROR: Expected " << expectedFlows
                                << " allocated flows, but only found " << allocatedFlows.size()
                                << "\n";
                return false;
        }

        std::cout << "Port-ids of allocated flows:";
        for (unsigned int i = 0; i < allocatedFlows.size(); i++) {
                std::cout << " " << allocatedFlows.at(i).portId << ",";
        }
        std::cout << "\n";

        return true;
}

bool checkRegisteredApplications(unsigned int expectedApps) {
        std::vector<ApplicationRegistration *> registeredApplications = ipcManager
                        ->getRegisteredApplications();
        if (registeredApplications.size() != expectedApps) {
                std::cout << "ERROR: Expected " << expectedApps
                                << " registered applications, but only found "
                                                << registeredApplications.size() << "²n";
                return false;
        }

        ApplicationRegistration* applicationRegistration;
        for (unsigned int i = 0; i < registeredApplications.size(); i++) {
                applicationRegistration = registeredApplications.at(i);
                std::cout << "Application "
                                << applicationRegistration->applicationName
                                                .processName << " registered at DIFs: ";
                std::list<ApplicationProcessNamingInformation>::const_iterator iterator;
                for (iterator = applicationRegistration->DIFNames.begin();
                                iterator != applicationRegistration->DIFNames.end();
                                ++iterator) {
                        std::cout << iterator->processName << ", ";
                }
                std::cout << "\n";
        }

        return true;
}

int main() {
	std::cout << "TESTING LIBRINA-APPLICATION\n";
	ApplicationProcessNamingInformation sourceName("/apps/test/source", "1");
	ApplicationProcessNamingInformation destinationName("/apps/test/destination",
					"1");
	FlowSpecification flowSpecification;
	ApplicationProcessNamingInformation difName("test.DIF", "");

	/* TEST ALLOCATE REQUEST */
	unsigned int seqNumber = ipcManager->requestFlowAllocation(
	                sourceName, destinationName, flowSpecification);

	FlowInformation flow = ipcManager->commitPendingFlow(seqNumber, 23, difName);
	std::cout << "Flow allocated, portId is " << flow.portId
			<< "; DIF name is: " << flow.difName.processName
			<< "; state is: "<<flow.state << "\n";

	/* TEST ALLOCATE RESPONSE */
	FlowRequestEvent flowRequestEvent = FlowRequestEvent(25, flowSpecification,
			true, sourceName, destinationName, difName, 23, 234, 15, 12);
	FlowInformation flow2 = ipcManager->allocateFlowResponse(flowRequestEvent, 0, true);
	std::cout << "Accepted flow allocation, portId is " << flow2.portId
			<< "; DIF name is: " << flow2.difName.processName
			<< "; state is: " << flow2.state << "\n";

	/* TEST GET ALLOCATED FLOWS */
	if (!checkAllocatedFlows(2)) {
		return 1;
	}
	/* TEST DEALLOCATE FLOW */
	ipcManager->deallocate_flow(flow.portId);
	if (!checkAllocatedFlows(1)) {
		return 1;
	}
	ipcManager->deallocate_flow(flow2.portId);
	if (!checkAllocatedFlows(0)) {
		return -1;
	}

	try {
		ipcManager->deallocate_flow(234);
	} catch (IPCException &e) {
		std::cout << "Caught expected exception: " << e.what() << "\n";
	}
	/* TEST REGISTER APPLICATION */
	ApplicationRegistrationInformation info =
			 ApplicationRegistrationInformation(
					APPLICATION_REGISTRATION_SINGLE_DIF);
	info.difName = difName;
	info.appName = sourceName;
	seqNumber = ipcManager->requestApplicationRegistration(info);
	ipcManager->commitPendingRegistration(seqNumber, difName);
	if (!checkRegisteredApplications(1)) {
	        return -1;
	}
	/* TEST UNREGISTER APPLICATION */
	seqNumber = ipcManager->requestApplicationUnregistration(sourceName, difName);
	ipcManager->appUnregistrationResult(seqNumber, true);
        if (!checkRegisteredApplications(0)) {
                return -1;
        }

	return 0;
}
