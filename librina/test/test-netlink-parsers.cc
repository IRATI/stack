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
			sizeof(struct rinaHeader), 0, message->getOperationCode(), 0);

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
	message->setIpcProcessId(234);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message->getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message->getOperationCode(), 0);

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
	} else if (message->getIpcProcessId() !=
			recoveredMessage->getIpcProcessId()) {
		std::cout << "IPC Process id on original and recovered messages"
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
			sizeof(struct rinaHeader), 0, message->getOperationCode(), 0);

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
			sizeof(struct rinaHeader), 0, message->getOperationCode(), 0);

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
			sizeof(struct rinaHeader), 0, message->getOperationCode(), 0);

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
			sizeof(struct rinaHeader), 0, message->getOperationCode(), 0);

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
	} else if (message->getErrorDescription()
			!= recoveredMessage->getErrorDescription()) {
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

int testAppFlowDeallocatedNotificationMessage() {
	std::cout << "TESTING APP FLOW DEALLOCATED NOTIFICATION MESSAGE\n";
	int returnValue = 0;

	ApplicationProcessNamingInformation * applicationName =
			new ApplicationProcessNamingInformation();
	applicationName->setProcessName("/apps/source");
	applicationName->setProcessInstance("35");
	applicationName->setEntityName("database");
	applicationName->setEntityInstance("3");

	ApplicationProcessNamingInformation * difName =
			new ApplicationProcessNamingInformation();
	difName->setProcessName("/difs/Test5.DIF");

	AppFlowDeallocatedNotificationMessage * message =
			new AppFlowDeallocatedNotificationMessage();
	message->setPortId(47);
	message->setCode(7);
	message->setReason("Reason description");
	message->setDifName(*difName);
	message->setApplicationName(*applicationName);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message->getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message->getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, message);
	if (result < 0) {
		std::cout
				<< "Error constructing Application Flow Deallocated Notification "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		delete difName;
		delete applicationName;
		delete message;
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	AppFlowDeallocatedNotificationMessage * recoveredMessage =
			dynamic_cast<AppFlowDeallocatedNotificationMessage *>(parseBaseNetlinkMessage(
					netlinkMessageHeader));
	if (message == NULL) {
		std::cout
				<< "Error parsing Application Flow Deallocated Notification Message "
				<< "\n";
		returnValue = -1;
	} else if (message->getPortId() != recoveredMessage->getPortId()) {
		std::cout << "PortId on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message->getCode() != recoveredMessage->getCode()) {
		std::cout << "Code on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message->getReason() != recoveredMessage->getReason()) {
		std::cout << "Reason on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message->getApplicationName()
			!= recoveredMessage->getApplicationName()) {
		std::cout << "Application name on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message->getDifName() != recoveredMessage->getDifName()) {
		std::cout << "DIF name on original and recovered "
				<< "messages are different\n";
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

int testAppRegisterApplicationRequestMessage() {
	std::cout << "TESTING APP REGISTER APPLICATION REQUEST MESSAGE\n";
	int returnValue = 0;

	ApplicationProcessNamingInformation * applicationName =
			new ApplicationProcessNamingInformation();
	applicationName->setProcessName("/apps/source");
	applicationName->setProcessInstance("25");
	applicationName->setEntityName("database");
	applicationName->setEntityInstance("31");

	ApplicationProcessNamingInformation * difName =
			new ApplicationProcessNamingInformation();
	difName->setProcessName("/difs/Test2.DIF");

	AppRegisterApplicationRequestMessage * message =
			new AppRegisterApplicationRequestMessage();

	message->setDifName(*difName);
	message->setApplicationName(*applicationName);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message->getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message->getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, message);
	if (result < 0) {
		std::cout << "Error constructing Register Application Request "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		delete difName;
		delete applicationName;
		delete message;
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	AppRegisterApplicationRequestMessage * recoveredMessage =
			dynamic_cast<AppRegisterApplicationRequestMessage *>(parseBaseNetlinkMessage(
					netlinkMessageHeader));
	if (message == NULL) {
		std::cout << "Error parsing Register Application Request Message "
				<< "\n";
		returnValue = -1;
	} else if (message->getApplicationName()
			!= recoveredMessage->getApplicationName()) {
		std::cout << "Application name on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message->getDifName() != recoveredMessage->getDifName()) {
		std::cout << "DIF name on original and recovered "
				<< "messages are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "AppRegisterApplicationRequest test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete difName;
	delete applicationName;
	delete message;
	delete recoveredMessage;

	return returnValue;
}

int testAppRegisterApplicationResponseMessage() {
	std::cout << "TESTING APP REGISTER APPLICATION RESPONSE MESSAGE\n";
	int returnValue = 0;

	ApplicationProcessNamingInformation * applicationName =
			new ApplicationProcessNamingInformation();
	applicationName->setProcessName("/apps/source");
	applicationName->setProcessInstance("25");
	applicationName->setEntityName("database");
	applicationName->setEntityInstance("30");

	ApplicationProcessNamingInformation * difName =
			new ApplicationProcessNamingInformation();
	difName->setProcessName("/difs/Test.DIF");

	AppRegisterApplicationResponseMessage * message =
			new AppRegisterApplicationResponseMessage();

	message->setResult(1);
	message->setIpcProcessPortId(7);
	message->setIpcProcessId(39);
	message->setErrorDescription("Error description");
	message->setDifName(*difName);
	message->setApplicationName(*applicationName);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message->getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message->getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, message);
	if (result < 0) {
		std::cout << "Error constructing Register Application Response "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		delete difName;
		delete applicationName;
		delete message;
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	AppRegisterApplicationResponseMessage * recoveredMessage =
			dynamic_cast<AppRegisterApplicationResponseMessage *>(parseBaseNetlinkMessage(
					netlinkMessageHeader));
	if (message == NULL) {
		std::cout << "Error parsing Register Application Response Message "
				<< "\n";
		returnValue = -1;
	} else if (message->getResult() != recoveredMessage->getResult()) {
		std::cout << "Result on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message->getErrorDescription()
			!= recoveredMessage->getErrorDescription()) {
		std::cout << "Error description on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message->getIpcProcessPortId()
			!= recoveredMessage->getIpcProcessPortId()) {
		std::cout << "IPC process port id on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message->getIpcProcessId()
			!= recoveredMessage->getIpcProcessId()) {
		std::cout << "IPC process id on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message->getApplicationName()
			!= recoveredMessage->getApplicationName()) {
		std::cout << "Application name on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message->getDifName() != recoveredMessage->getDifName()) {
		std::cout << "DIF name on original and recovered "
				<< "messages are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "AppRegisterApplicationResponse test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete difName;
	delete applicationName;
	delete message;
	delete recoveredMessage;

	return returnValue;
}

int testIpcmRegisterApplicationRequestMessage() {
	std::cout << "TESTING IPCM REGISTER APPLICATION REQUEST MESSAGE\n";
	int returnValue = 0;

	ApplicationProcessNamingInformation * applicationName =
			new ApplicationProcessNamingInformation();
	applicationName->setProcessName("/apps/source");
	applicationName->setProcessInstance("25");
	applicationName->setEntityName("database");
	applicationName->setEntityInstance("31");

	ApplicationProcessNamingInformation * difName =
			new ApplicationProcessNamingInformation();
	difName->setProcessName("/difs/Test2.DIF");

	IpcmRegisterApplicationRequestMessage * message =
			new IpcmRegisterApplicationRequestMessage();

	message->setDifName(*difName);
	message->setApplicationName(*applicationName);
	message->setApplicationPortId(34);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message->getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message->getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, message);
	if (result < 0) {
		std::cout << "Error constructing Ipcm Register Application Request "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		delete difName;
		delete applicationName;
		delete message;
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	IpcmRegisterApplicationRequestMessage * recoveredMessage =
			dynamic_cast<IpcmRegisterApplicationRequestMessage *>(parseBaseNetlinkMessage(
					netlinkMessageHeader));
	if (message == NULL) {
		std::cout << "Error parsing Ipcm Register Application Request Message "
				<< "\n";
		returnValue = -1;
	} else if (message->getApplicationName()
			!= recoveredMessage->getApplicationName()) {
		std::cout << "Application name on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message->getDifName() != recoveredMessage->getDifName()) {
		std::cout << "DIF name on original and recovered "
				<< "messages are different\n";
		returnValue = -1;
	} else if (message->getApplicationPortId() !=
			recoveredMessage->getApplicationPortId()) {
		std::cout << "Application port id on original and recovered "
				<< "messages are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "IpcmRegisterApplicationRequest test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete difName;
	delete applicationName;
	delete message;
	delete recoveredMessage;

	return returnValue;
}

int testIpcmRegisterApplicationResponseMessage() {
	std::cout << "TESTING IPCM REGISTER APPLICATION RESPONSE MESSAGE\n";
	int returnValue = 0;

	ApplicationProcessNamingInformation * applicationName =
			new ApplicationProcessNamingInformation();
	applicationName->setProcessName("/apps/source");
	applicationName->setProcessInstance("25");
	applicationName->setEntityName("database");
	applicationName->setEntityInstance("30");

	ApplicationProcessNamingInformation * difName =
			new ApplicationProcessNamingInformation();
	difName->setProcessName("/difs/Test.DIF");

	IpcmRegisterApplicationResponseMessage * message =
			new IpcmRegisterApplicationResponseMessage();

	message->setResult(1);
	message->setErrorDescription("Error description");
	message->setDifName(*difName);
	message->setApplicationName(*applicationName);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message->getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message->getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, message);
	if (result < 0) {
		std::cout << "Error constructing Ipcm Register Application Response "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		delete difName;
		delete applicationName;
		delete message;
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	IpcmRegisterApplicationResponseMessage * recoveredMessage =
			dynamic_cast<IpcmRegisterApplicationResponseMessage *>(parseBaseNetlinkMessage(
					netlinkMessageHeader));
	if (message == NULL) {
		std::cout << "Error parsing Ipcm Register Application Response Message "
				<< "\n";
		returnValue = -1;
	} else if (message->getResult() != recoveredMessage->getResult()) {
		std::cout << "Result on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message->getErrorDescription()
			!= recoveredMessage->getErrorDescription()) {
		std::cout << "Error description on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message->getApplicationName()
			!= recoveredMessage->getApplicationName()) {
		std::cout << "Application name on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message->getDifName() != recoveredMessage->getDifName()) {
		std::cout << "DIF name on original and recovered "
				<< "messages are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "IpcmRegisterApplicationResponse test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete difName;
	delete applicationName;
	delete message;
	delete recoveredMessage;

	return returnValue;
}

int testIpcmAssignToDIFRequestMessage() {
	std::cout << "TESTING IPCM ASSIGN TO DIF REQUEST MESSAGE\n";
	int returnValue = 0;

	ApplicationProcessNamingInformation * difName =
			new ApplicationProcessNamingInformation();
	difName->setProcessName("/difs/Test.DIF");

	IpcmAssignToDIFRequestMessage * message =
			new IpcmAssignToDIFRequestMessage();
	DIFConfiguration * difConfiguration = new DIFConfiguration();
	difConfiguration->setDifType(DIF_TYPE_SHIM_ETHERNET);
	difConfiguration->setDifName(*difName);
	message->setDIFConfiguration(*difConfiguration);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message->getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message->getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, message);
	if (result < 0) {
		std::cout << "Error constructing Ipcm Assign To DIF Request "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		delete difName;
		delete difConfiguration;
		delete message;
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	IpcmAssignToDIFRequestMessage * recoveredMessage =
			dynamic_cast<IpcmAssignToDIFRequestMessage *>(
					parseBaseNetlinkMessage(netlinkMessageHeader));
	if (message == 0) {
		std::cout << "Error parsing Ipcm Assign To DIF Request Message "
				<< "\n";
		returnValue = -1;
	} else if (message->getDIFConfiguration().getDifType() !=
			recoveredMessage->getDIFConfiguration().getDifType()) {
		std::cout << "DIFConfiguration.difType on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message->getDIFConfiguration().getDifName() !=
			recoveredMessage->getDIFConfiguration().getDifName()) {
		std::cout << "DIFConfiguration.difName on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "IpcmAssignToDIFRequest test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete difName;
	delete difConfiguration;
	delete message;
	delete recoveredMessage;

	return returnValue;
}

int testIpcmAssignToDIFResponseMessage() {
	std::cout << "TESTING IPCM ASSIGN TO DIF RESPONSE MESSAGE\n";
	int returnValue = 0;

	IpcmAssignToDIFResponseMessage * message =
			new IpcmAssignToDIFResponseMessage();
	message->setResult(-25);
	message->setErrorDescription("Something went wrong, that's life");

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message->getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message->getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, message);
	if (result < 0) {
		std::cout << "Error constructing Ipcm Assign To DIF Response "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		delete message;
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	IpcmAssignToDIFResponseMessage * recoveredMessage =
			dynamic_cast<IpcmAssignToDIFResponseMessage *>(
					parseBaseNetlinkMessage(netlinkMessageHeader));
	if (message == 0) {
		std::cout << "Error parsing Ipcm Assign To DIF Response Message "
				<< "\n";
		returnValue = -1;
	} else if (message->getResult() != recoveredMessage->getResult()) {
		std::cout << "Result on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message->getErrorDescription().compare(
			recoveredMessage->getErrorDescription()) != 0) {
		std::cout << "Error description on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "IpcmAssignToDIFResponse test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete message;
	delete recoveredMessage;

	return returnValue;
}

int testIpcmAllocateFlowRequestMessage() {
	std::cout << "TESTING IPCM ALLOCATE FLOW REQUEST MESSAGE\n";
	int returnValue = 0;

	IpcmAllocateFlowRequestMessage message;
	ApplicationProcessNamingInformation sourceName;
	sourceName.setProcessName("/apps/source");
	sourceName.setProcessInstance("1");
	sourceName.setEntityName("database");
	sourceName.setEntityName("1234");
	message.setSourceAppName(sourceName);
	ApplicationProcessNamingInformation destName;
	destName.setProcessName("/apps/dest");
	destName.setProcessInstance("4");
	destName.setEntityName("server");
	destName.setEntityName("342");
	message.setDestAppName(destName);
	FlowSpecification flowSpec;
	message.setFlowSpec(flowSpec);
	ApplicationProcessNamingInformation difName;
	difName.setProcessName("/difs/Test.DIF");
	message.setDifName(difName);
	message.setPortId(34);
	message.setApplicationPortId(123);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, &message);
	if (result < 0) {
		std::cout << "Error constructing Ipcm Allocate Flow Request "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	IpcmAllocateFlowRequestMessage * recoveredMessage =
			dynamic_cast<IpcmAllocateFlowRequestMessage *>(
					parseBaseNetlinkMessage(netlinkMessageHeader));
	if (recoveredMessage == 0) {
		std::cout << "Error parsing Ipcm Allocate Flow Request Message "
				<< "\n";
		returnValue = -1;
	} else if (message.getSourceAppName() !=
			recoveredMessage->getSourceAppName()) {
		std::cout << "Source application name on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.getDestAppName() !=
			recoveredMessage->getDestAppName()) {
		std::cout << "Dest application name on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.getFlowSpec() !=
			recoveredMessage->getFlowSpec()) {
		std::cout << "Flow specification on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.getDifName() !=
			recoveredMessage->getDifName()) {
		std::cout << "DIF name on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.getPortId()!=
			recoveredMessage->getPortId()) {
		std::cout << "Port id on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.getApplicationPortId()!=
	 		recoveredMessage->getApplicationPortId()) {
		std::cout << "Application port on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "IpcmAllocateFlowRequestMessage test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete recoveredMessage;

	return returnValue;
}

int testIpcmAllocateFlowResponseMessage() {
	std::cout << "TESTING IPCM ALLOCATE FLOW RESPONSE MESSAGE\n";
	int returnValue = 0;

	IpcmAllocateFlowResponseMessage message;
	message.setResult(-25);
	message.setErrorDescription("Something went wrong, that's life");

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, &message);
	if (result < 0) {
		std::cout << "Error constructing Ipcm Allocate Flow Response "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	IpcmAllocateFlowResponseMessage * recoveredMessage =
			dynamic_cast<IpcmAllocateFlowResponseMessage *>(
					parseBaseNetlinkMessage(netlinkMessageHeader));
	if (recoveredMessage == 0) {
		std::cout << "Error parsing Ipcm Allocate Flow Response Message "
				<< "\n";
		returnValue = -1;
	} else if (message.getResult() != recoveredMessage->getResult()) {
		std::cout << "Result on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.getErrorDescription().compare(
			recoveredMessage->getErrorDescription()) != 0) {
		std::cout << "Error description on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "IpcmAssignToDIFResponse test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete recoveredMessage;

	return returnValue;
}

int testIpcmIPCProcessRegisteredToDIFNotification() {
	std::cout << "TESTING IPCM IPC PROCESS REGISTERED TO DIF NOTIFICATION\n";
	int returnValue = 0;

	IpcmIPCProcessRegisteredToDIFNotification message;
	ApplicationProcessNamingInformation ipcProcessName;
	ipcProcessName.setProcessName("/ipcprocesses/Barcelona/i2CAT");
	ipcProcessName.setProcessInstance("1");
	ipcProcessName.setEntityName("Management");
	ipcProcessName.setProcessInstance("1234");
	message.setIpcProcessName(ipcProcessName);
	ApplicationProcessNamingInformation difName;
	difName.setProcessName("/difs/Test.DIF");
	message.setDifName(difName);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, &message);
	if (result < 0) {
		std::cout << "Error constructing Ipcm IPC Process Registered to DIF "
				<< "Notification Message \n";
		nlmsg_free(netlinkMessage);
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	IpcmIPCProcessRegisteredToDIFNotification * recoveredMessage =
			dynamic_cast<IpcmIPCProcessRegisteredToDIFNotification *>(
					parseBaseNetlinkMessage(netlinkMessageHeader));
	if (recoveredMessage == 0) {
		std::cout << "Error parsing Ipcm IPC Process Registered to DIF Message "
				<< "\n";
		returnValue = -1;
	} else if (message.getIpcProcessName() !=
			recoveredMessage->getIpcProcessName()) {
		std::cout << "IPC Process Name on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.getDifName() != recoveredMessage->getDifName()) {
		std::cout << "DIF name on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "IpcmIPCProcessRegisteredToDIFNotification test ok\n";
	}
	nlmsg_free(netlinkMessage);
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

	result = testAppFlowDeallocatedNotificationMessage();
	if (result < 0) {
		return result;
	}

	result = testAppRegisterApplicationRequestMessage();
	if (result < 0) {
		return result;
	}

	result = testAppRegisterApplicationResponseMessage();
	if (result < 0) {
		return result;
	}

	result = testIpcmRegisterApplicationRequestMessage();
	if (result < 0) {
		return result;
	}

	result = testIpcmRegisterApplicationResponseMessage();
	if (result < 0) {
		return result;
	}

	result = testIpcmAssignToDIFRequestMessage();
	if (result < 0) {
		return result;
	}

	result = testIpcmAssignToDIFResponseMessage();
	if (result < 0) {
		return result;
	}

	result = testIpcmAllocateFlowRequestMessage();
	if (result < 0) {
		return result;
	}

	result = testIpcmAllocateFlowResponseMessage();
	if (result < 0) {
		return result;
	}

	result = testIpcmIPCProcessRegisteredToDIFNotification();
	if (result < 0) {
		return result;
	}
}
