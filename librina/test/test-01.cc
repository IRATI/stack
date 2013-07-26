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

bool checkAllocatedFlows(unsigned int expectedFlows) {
	std::vector<Flow *> allocatedFlows = ipcManager->getAllocatedFlows();
	if (allocatedFlows.size() != expectedFlows) {
		std::cout << "ERROR: Expected " << expectedFlows
				<< " allocated flows, but only found " + allocatedFlows.size()
				<< "\n";
		return false;
	}

	std::cout << "Port-ids of allocated flows:";
	for (unsigned int i = 0; i < allocatedFlows.size(); i++) {
		std::cout << " " << allocatedFlows.at(i)->getPortId() << ",";
	}
	std::cout << "\n";

	return true;
}

bool checkRegisteredApplications(unsigned int expectedFlows) {
	std::vector<ApplicationRegistration *> registeredApplications = ipcManager
			->getRegisteredApplications();
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
				<< applicationRegistration->getApplicationName()
						.getProcessName() << " registered at DIFs: ";
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

bool checkRecognizedEvent(IPCEvent * event) {
	switch (event->getType()) {
	case FLOW_ALLOCATION_REQUESTED_EVENT: {
		FlowRequestEvent * incFlowEvent =
				dynamic_cast<FlowRequestEvent *>(event);
		std::cout << "Got incoming flow request event with portId "
				<< incFlowEvent->getPortId() << "\n";
		break;
	}
	case FLOW_DEALLOCATED_EVENT: {
		FlowDeallocatedEvent * flowDeallocEvent =
				dynamic_cast<FlowDeallocatedEvent *>(event);
		std::cout << "Got flow deallocated event with portId "
				<< flowDeallocEvent->getPortId() << "\n";
		break;
	}
	case APPLICATION_UNREGISTERED_EVENT: {
		ApplicationUnregisteredEvent * appUnEvent =
				dynamic_cast<ApplicationUnregisteredEvent *>(event);
		std::cout << "Got application unregistered event with app name "
				<< appUnEvent->getApplicationName().getProcessName() << "\n";
		break;
	}
	default:
		std::cout << "Unrecognized event type\n";
		return false;
	}

	return true;
}

int main(int argc, char * argv[]) {
	std::cout << "TESTING LIBRINA-APPLICATION\n";
	ApplicationProcessNamingInformation * sourceName =
			new ApplicationProcessNamingInformation("/apps/test/source", "1");
	ApplicationProcessNamingInformation * destinationName =
			new ApplicationProcessNamingInformation("/apps/test/destination",
					"1");
	FlowSpecification * flowSpecification = new FlowSpecification();

	/* TEST ALLOCATE REQUEST */
	Flow * flow = ipcManager->allocateFlowRequest(*sourceName,
			*destinationName, *flowSpecification);
	std::cout << "Flow allocated, portId is " << flow->getPortId()
			<< "; DIF name is: " << flow->getDIFName().getProcessName()
			<< "\n";

	/* TEST WRITE SDU */
	unsigned char sdu[] = { 45, 34, 2, 36, 8 };
	flow->writeSDU(sdu, 5);

	/* TEST ALLOCATE RESPONSE */
	FlowRequestEvent * flowRequestEvent = new FlowRequestEvent(25, *flowSpecification,
			*sourceName, *destinationName, *sourceName, 23);
	Flow * flow2 = ipcManager->allocateFlowResponse(*flowRequestEvent, true, "");
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
	if (!checkAllocatedFlows(2)) {
		return 1;
	}

	/* TEST GET DIF PROPERTIES */
	std::list<DIFProperties> difPropertiesList = ipcManager
			->getDIFProperties(flow->getLocalApplicationName(),
					flow->getDIFName());
	DIFProperties difProperties = *(difPropertiesList.begin());
	std::cout << "DIF name: " << difProperties.getDifName().getProcessName()
			<< "; max SDU size: " << difProperties.getMaxSduSize() << "\n";

	/* TEST DEALLOCATE FLOW */
	ipcManager->deallocateFlow(flow->getPortId());
	if (!checkAllocatedFlows(1)) {
		return 1;
	}

	ipcManager->deallocateFlow(flow2->getPortId());
	if (!checkAllocatedFlows(0)) {
		return 1;
	}

	try {
		ipcManager->deallocateFlow(234);
	} catch (IPCException &e) {
		std::cout << "Caught expected exception: " << e.what() << "\n";
	}

	/* TEST REGISTER APPLICATION */
	ApplicationRegistrationInformation info =
			ApplicationRegistrationInformation(
					APPLICATION_REGISTRATION_SINGLE_DIF);
	info.setDIFName(difProperties.getDifName());
	ipcManager->registerApplication(*sourceName, info);

	/* TEST UNREGISTER APPLICATION */
	ipcManager->unregisterApplication(*sourceName,
			difProperties.getDifName());

	/* TEST EVENT POLL */
	IPCEvent * event = ipcEventProducer->eventPoll();
	if (!checkRecognizedEvent(event)) {
		return 1;
	}

	delete event;

	/** TEST EVENT WAIT */
	event = ipcEventProducer->eventWait();
	if (!checkRecognizedEvent(event)) {
		return 1;
	}

	delete sourceName;
	delete destinationName;
	delete event;

	return 0;
}
