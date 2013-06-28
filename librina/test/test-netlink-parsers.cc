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

#include "netlink-parsers.h"

using namespace rina;

int testAppAllocateFlowRequestMessage() {
	std::cout << "TESTING APP ALLOCATE FLOW REQUEST MESSAGE\n";
	int returnValue = 0;

	ApplicationProcessNamingInformation * sourceName =
			new ApplicationProcessNamingInformation();
	sourceName->setProcessName("/apps/source");
	sourceName->setProcessInstance("12");
	sourceName->setEntityName("database");
	sourceName->setEntityInstance("12");

	ApplicationProcessNamingInformation * destName =
			new ApplicationProcessNamingInformation();
	destName->setProcessName("/apps/dest");
	destName->setProcessInstance("12345");
	destName->setEntityName("printer");
	destName->setEntityInstance("12623456");

	FlowSpecification * flowSpec = new FlowSpecification();

	AppAllocateFlowRequestMessage * message =
			new AppAllocateFlowRequestMessage();
	message->setSourceAppName(*sourceName);
	message->setDestAppName(*destName);
	message->setFlowSpecification(*flowSpec);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message->getSequenceNumber(), 21,
			0, 0, message->getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, message);
	if (result < 0) {
		std::cout << "Error constructing Application Allocate Flow request "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		delete destName;
		delete sourceName;
		delete message;
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	AppAllocateFlowRequestMessage * recoveredMessage =
			dynamic_cast<AppAllocateFlowRequestMessage *>(parseBaseNetlinkMessage(
					netlinkMessageHeader));
	if (message == NULL) {
		std::cout << "Error parsing Application Allocate Flow request Message "
				<< "\n";
		returnValue = -1;
	} else if (message->getSourceAppName()
			!= recoveredMessage->getSourceAppName()) {
		std::cout
				<< "Source application name on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message->getDestAppName()
			!= recoveredMessage->getDestAppName()) {
		std::cout << "Destination application name on original and recovered "
				<< "messages are different\n";
		returnValue = -1;
	} else if (message->getFlowSpecification()
			!= recoveredMessage->getFlowSpecification()) {
		std::cout << "Destination flow specification on original and recovered "
				<< "messages are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "AppAllocateFlowRequestMessage test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete destName;
	delete sourceName;
	delete message;
	delete recoveredMessage;

	return returnValue;
}

int testAppAllocateFlowRequestResultMessage() {
	std::cout << "TESTING APP ALLOCATE FLOW REQUEST RESULT MESSAGE\n";
	int returnValue = 0;

	ApplicationProcessNamingInformation * sourceName =
			new ApplicationProcessNamingInformation();
	sourceName->setProcessName("/apps/source");
	sourceName->setProcessInstance("12");
	sourceName->setEntityName("database");
	sourceName->setEntityInstance("12");

	ApplicationProcessNamingInformation * difName =
			new ApplicationProcessNamingInformation();
	difName->setProcessName("/difs/Test.DIF");

	AppAllocateFlowRequestResultMessage * message =
			new AppAllocateFlowRequestResultMessage();
	message->setSourceAppName(*sourceName);
	message->setPortId(32);
	message->setErrorDescription("Error description");
	message->setDifName(*difName);
	message->setIpcProcessPortId(42);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message->getSequenceNumber(), 21,
			0, 0, message->getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, message);
	if (result < 0) {
		std::cout
				<< "Error constructing Application Allocate Flow Request Result "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		delete message;
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	AppAllocateFlowRequestResultMessage * recoveredMessage =
			dynamic_cast<AppAllocateFlowRequestResultMessage *>(parseBaseNetlinkMessage(
					netlinkMessageHeader));
	if (message == NULL) {
		std::cout
				<< "Error parsing Application Allocate Flow Request Result Message "
				<< "\n";
		returnValue = -1;

	} else if (message->getErrorDescription()
			!= recoveredMessage->getErrorDescription()) {
		std::cout << "Error description on original and recovered "
				<< "messages are different\n";
		returnValue = -1;
	} else if (message->getIpcProcessPortId()
			!= recoveredMessage->getIpcProcessPortId()) {
		std::cout << "ipc_process_port_id on original and recovered "
				<< "messages are different\n";
		returnValue = -1;
	} else if (message->getDifName() != recoveredMessage->getDifName()) {
		std::cout << "DIF name on original and recovered "
				<< "messages are different\n";
		returnValue = -1;
	} else if (message->getSourceAppName()
			!= recoveredMessage->getSourceAppName()) {
		std::cout << "Source app name on original and recovered "
				<< "messages are different\n";
		returnValue = -1;
	} else if (message->getPortId() != recoveredMessage->getPortId()) {
		std::cout << "PortId on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "AppAllocateFlowRequestResultMessage test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete message;
	delete recoveredMessage;
	delete sourceName;
	delete difName;

	return returnValue;
}

int testAppAllocateFlowRequestArrivedMessage() {
	std::cout << "TESTING APP ALLOCATE FLOW REQUEST ARRIVED MESSAGE\n";
	int returnValue = 0;

	ApplicationProcessNamingInformation * sourceName =
			new ApplicationProcessNamingInformation();
	sourceName->setProcessName("/apps/source");
	sourceName->setProcessInstance("12");
	sourceName->setEntityName("database");
	sourceName->setEntityInstance("12");

	ApplicationProcessNamingInformation * destName =
			new ApplicationProcessNamingInformation();
	destName->setProcessName("/apps/dest");
	destName->setProcessInstance("12345");
	destName->setEntityName("printer");
	destName->setEntityInstance("12623456");

	ApplicationProcessNamingInformation * difName =
			new ApplicationProcessNamingInformation();
	difName->setProcessName("/difs/Test1.DIF");

	FlowSpecification * flowSpec = new FlowSpecification();

	AppAllocateFlowRequestArrivedMessage * message =
			new AppAllocateFlowRequestArrivedMessage();
	message->setSourceAppName(*sourceName);
	message->setDestAppName(*destName);
	message->setFlowSpecification(*flowSpec);
	message->setPortId(42);
	message->setDifName(*difName);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message->getSequenceNumber(), 21,
			0, 0, message->getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, message);
	if (result < 0) {
		std::cout
				<< "Error constructing Application Allocate Flow Request Arrived"
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		delete destName;
		delete sourceName;
		delete message;
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	AppAllocateFlowRequestArrivedMessage * recoveredMessage =
			dynamic_cast<AppAllocateFlowRequestArrivedMessage *>(parseBaseNetlinkMessage(
					netlinkMessageHeader));
	if (message == NULL) {
		std::cout
				<< "Error parsing Application Allocate Flow Request Arrived Message "
				<< "\n";
		returnValue = -1;
	} else if (message->getSourceAppName()
			!= recoveredMessage->getSourceAppName()) {
		std::cout
				<< "Source application name on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message->getDestAppName()
			!= recoveredMessage->getDestAppName()) {
		std::cout << "Destination application name on original and recovered "
				<< "messages are different\n";
		returnValue = -1;
	} else if (message->getFlowSpecification()
			!= recoveredMessage->getFlowSpecification()) {
		std::cout << "Destination flow specification on original and recovered "
				<< "messages are different\n";
		returnValue = -1;
	} else if (message->getPortId() != recoveredMessage->getPortId()) {
		std::cout << "PortId on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message->getDifName() != recoveredMessage->getDifName()) {
		std::cout << "DIF name on original and recovered "
				<< "messages are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "AppAllocateFlowRequestArrivedMessage test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete destName;
	delete sourceName;
	delete message;
	delete recoveredMessage;

	return returnValue;
}

int testAppAllocateFlowResponseMessage() {
	std::cout << "TESTING APP ALLOCATE FLOW RESPONSE MESSAGE\n";
	int returnValue = 0;

	ApplicationProcessNamingInformation * difName =
			new ApplicationProcessNamingInformation();
	difName->setProcessName("/difs/Test3.DIF");

	AppAllocateFlowResponseMessage * message =
			new AppAllocateFlowResponseMessage();
	message->setDifName(*difName);
	message->setAccept(true);
	message->setDenyReason("No, we cannot!");
	message->setNotifySource(true);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message->getSequenceNumber(), 21,
			0, 0, message->getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, message);
	if (result < 0) {
		std::cout << "Error constructing Application Allocate Flow Response "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		delete message;
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	AppAllocateFlowResponseMessage * recoveredMessage =
			dynamic_cast<AppAllocateFlowResponseMessage *>(parseBaseNetlinkMessage(
					netlinkMessageHeader));
	if (message == NULL) {
		std::cout << "Error parsing Application Allocate Flow Response Message "
				<< "\n";
		returnValue = -1;

	} else if (message->getDifName() != recoveredMessage->getDifName()) {
		std::cout << "DIF name on original and recovered "
				<< "messages are different\n";
		returnValue = -1;
	} else if (message->isAccept() != recoveredMessage->isAccept()) {
		std::cout << "Accept flag on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message->getDenyReason() != recoveredMessage->getDenyReason()) {
		std::cout << "Deny reason on original and recovered "
				<< "messages are different\n";
		returnValue = -1;
	} else if (message->isNotifySource()
			!= recoveredMessage->isNotifySource()) {
		std::cout << "Notify source flag on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "AppAllocateFlowResponse test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete message;
	delete recoveredMessage;
	delete difName;

	return returnValue;
}

int testAppDeallocateFlowRequestMessage() {
	std::cout << "TESTING APP DEALLOCATE FLOW REQUEST MESSAGE\n";
	int returnValue = 0;

	ApplicationProcessNamingInformation * difName =
			new ApplicationProcessNamingInformation();
	difName->setProcessName("/difs/Test2.DIF");

	ApplicationProcessNamingInformation * applicationName =
			new ApplicationProcessNamingInformation();
	applicationName->setProcessName("/apps/source");
	applicationName->setProcessInstance("15");
	applicationName->setEntityName("database");
	applicationName->setEntityInstance("13");

	AppDeallocateFlowRequestMessage * message =
			new AppDeallocateFlowRequestMessage();
	message->setPortId(47);
	message->setDifName(*difName);
	message->setApplicationName(*applicationName);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message->getSequenceNumber(), 21,
			0, 0, message->getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, message);
	if (result < 0) {
		std::cout << "Error constructing Application Deallocate Flow Request "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		delete difName;
		delete applicationName;
		delete message;
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	AppDeallocateFlowRequestMessage * recoveredMessage =
			dynamic_cast<AppDeallocateFlowRequestMessage *>(parseBaseNetlinkMessage(
					netlinkMessageHeader));
	if (message == NULL) {
		std::cout
				<< "Error parsing Application Deallocate Flow Request Message "
				<< "\n";
		returnValue = -1;
	} else if (message->getPortId() != recoveredMessage->getPortId()) {
		std::cout << "PortId on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message->getDifName() != recoveredMessage->getDifName()) {
		std::cout << "DIF name on original and recovered "
				<< "messages are different\n";
		returnValue = -1;
	} else if (message->getApplicationName()
			!= recoveredMessage->getApplicationName()) {
		std::cout << "Application name on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "AppDeallocateFlowRequest test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete difName;
	delete applicationName;
	delete message;
	delete recoveredMessage;

	return returnValue;
}


int testAppDeallocateFlowResponseMessage() {
	std::cout << "TESTING APP DEALLOCATE FLOW RESPONSE MESSAGE\n";
	int returnValue = 0;

	ApplicationProcessNamingInformation * applicationName =
			new ApplicationProcessNamingInformation();
	applicationName->setProcessName("/apps/source2");
	applicationName->setProcessInstance("11");
	applicationName->setEntityName("database");
	applicationName->setEntityInstance("9");

	AppDeallocateFlowResponseMessage * message =
			new AppDeallocateFlowResponseMessage();
	message->setResult(0);
	message->setErrorDescription("Error description");
	message->setApplicationName(*applicationName);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message->getSequenceNumber(), 21,
			0, 0, message->getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, message);
	if (result < 0) {
		std::cout << "Error constructing Application Deallocate Flow Response "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		delete applicationName;
		delete message;
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	AppDeallocateFlowResponseMessage * recoveredMessage =
			dynamic_cast<AppDeallocateFlowResponseMessage *>(parseBaseNetlinkMessage(
					netlinkMessageHeader));
	if (message == NULL) {
		std::cout
				<< "Error parsing Application Deallocate Flow Response Message "
				<< "\n";
		returnValue = -1;
	} else if (message->getResult() != recoveredMessage->getResult()) {
		std::cout << "Result on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message->getErrorDescription() != recoveredMessage->getErrorDescription()) {
		std::cout << "Error description on original and recovered "
				<< "messages are different\n";
		returnValue = -1;
	} else if (message->getApplicationName()
			!= recoveredMessage->getApplicationName()) {
		std::cout << "Application name on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "AppDeallocateFlowResponse test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete applicationName;
	delete message;
	delete recoveredMessage;

	return returnValue;
}



int main(int argc, char * argv[]) {
	std::cout << "TESTING LIBRINA-NETLINK-PARSERS\n";

	int result;

	result = testAppAllocateFlowRequestMessage();
	if (result < 0) {
		return result;
	}

	result = testAppAllocateFlowRequestResultMessage();
	if (result < 0) {
		return result;
	}

	result = testAppAllocateFlowRequestArrivedMessage();
	if (result < 0) {
		return result;
	}

	result = testAppAllocateFlowResponseMessage();
	if (result < 0) {
		return result;
	}

	result = testAppDeallocateFlowRequestMessage();
	if (result < 0) {
		return result;
	}

	result = testAppDeallocateFlowResponseMessage();
		if (result < 0) {
			return result;
		}
}
