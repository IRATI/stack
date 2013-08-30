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

	AppAllocateFlowResponseMessage message;
	message.setAccept(true);
	message.setDenyReason("No, we cannot!");
	message.setNotifySource(true);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, &message);
	if (result < 0) {
		std::cout << "Error constructing Application Allocate Flow Response "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	AppAllocateFlowResponseMessage * recoveredMessage =
			dynamic_cast<AppAllocateFlowResponseMessage *>(parseBaseNetlinkMessage(
					netlinkMessageHeader));
	if (recoveredMessage == 0) {
		std::cout << "Error parsing Application Allocate Flow Response Message "
				<< "\n";
		returnValue = -1;

	} else if (message.isAccept() != recoveredMessage->isAccept()) {
		std::cout << "Accept flag on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.getDenyReason() != recoveredMessage->getDenyReason()) {
		std::cout << "Deny reason on original and recovered "
				<< "messages are different\n";
		returnValue = -1;
	} else if (message.isNotifySource()
			!= recoveredMessage->isNotifySource()) {
		std::cout << "Notify source flag on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "AppAllocateFlowResponse test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete recoveredMessage;

	return returnValue;
}

int testAppDeallocateFlowRequestMessage() {
	std::cout << "TESTING APP DEALLOCATE FLOW REQUEST MESSAGE\n";
	int returnValue = 0;

	ApplicationProcessNamingInformation applicationName;
	applicationName.setProcessName("/apps/source");
	applicationName.setProcessInstance("15");
	applicationName.setEntityName("database");
	applicationName.setEntityInstance("13");

	AppDeallocateFlowRequestMessage message;;
	message.setPortId(47);
	message.setApplicationName(applicationName);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, &message);
	if (result < 0) {
		std::cout << "Error constructing Application Deallocate Flow Request "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	AppDeallocateFlowRequestMessage * recoveredMessage =
			dynamic_cast<AppDeallocateFlowRequestMessage *>(parseBaseNetlinkMessage(
					netlinkMessageHeader));
	if (recoveredMessage == 0) {
		std::cout
		<< "Error parsing Application Deallocate Flow Request Message "
		<< "\n";
		returnValue = -1;
	} else if (message.getPortId() != recoveredMessage->getPortId()) {
		std::cout << "PortId on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.getApplicationName()
			!= recoveredMessage->getApplicationName()) {
		std::cout << "Application name on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "AppDeallocateFlowRequest test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete recoveredMessage;

	return returnValue;
}

int testAppDeallocateFlowResponseMessage() {
	std::cout << "TESTING APP DEALLOCATE FLOW RESPONSE MESSAGE\n";
	int returnValue = 0;

	ApplicationProcessNamingInformation applicationName;
	applicationName.setProcessName("/apps/source2");
	applicationName.setProcessInstance("11");
	applicationName.setEntityName("database");
	applicationName.setEntityInstance("9");

	AppDeallocateFlowResponseMessage message;
	message.setResult(0);
	message.setApplicationName(applicationName);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, &message);
	if (result < 0) {
		std::cout << "Error constructing Application Deallocate Flow Response "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	AppDeallocateFlowResponseMessage * recoveredMessage =
			dynamic_cast<AppDeallocateFlowResponseMessage *>(parseBaseNetlinkMessage(
					netlinkMessageHeader));
	if (recoveredMessage == 0) {
		std::cout
		<< "Error parsing Application Deallocate Flow Response Message "
		<< "\n";
		returnValue = -1;
	} else if (message.getResult() != recoveredMessage->getResult()) {
		std::cout << "Result on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.getApplicationName()
			!= recoveredMessage->getApplicationName()) {
		std::cout << "Application name on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "AppDeallocateFlowResponse test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete recoveredMessage;

	return returnValue;
}

int testAppFlowDeallocatedNotificationMessage() {
	std::cout << "TESTING APP FLOW DEALLOCATED NOTIFICATION MESSAGE\n";
	int returnValue = 0;

	ApplicationProcessNamingInformation applicationName;
	applicationName.setProcessName("/apps/source");
	applicationName.setProcessInstance("35");
	applicationName.setEntityName("database");
	applicationName.setEntityInstance("3");

	AppFlowDeallocatedNotificationMessage message;;
	message.setPortId(47);
	message.setCode(7);
	message.setApplicationName(applicationName);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, &message);
	if (result < 0) {
		std::cout
		<< "Error constructing Application Flow Deallocated Notification "
		<< "Message \n";
		nlmsg_free(netlinkMessage);
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	AppFlowDeallocatedNotificationMessage * recoveredMessage =
			dynamic_cast<AppFlowDeallocatedNotificationMessage *>(parseBaseNetlinkMessage(
					netlinkMessageHeader));
	if (recoveredMessage == 0) {
		std::cout
		<< "Error parsing Application Flow Deallocated Notification Message "
		<< "\n";
		returnValue = -1;
	} else if (message.getPortId() != recoveredMessage->getPortId()) {
		std::cout << "PortId on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.getCode() != recoveredMessage->getCode()) {
		std::cout << "Code on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.getApplicationName()
			!= recoveredMessage->getApplicationName()) {
		std::cout << "Application name on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "AppDeallocateFlowRequest test ok\n";
	}
	nlmsg_free(netlinkMessage);
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

	ApplicationRegistrationInformation appRegInfo =
		ApplicationRegistrationInformation(
				APPLICATION_REGISTRATION_SINGLE_DIF);
	appRegInfo.setDIFName(*difName);

	message->setApplicationRegistrationInformation(appRegInfo);
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
	} else if (message->getApplicationRegistrationInformation().
			getRegistrationType() !=
			recoveredMessage->getApplicationRegistrationInformation().
			getRegistrationType()) {
		std::cout << "Application Registration Type on original and recovered "
				<< "messages are different\n";
		returnValue = -1;
	}  else if (message->getApplicationRegistrationInformation().getDIFName()
			!= recoveredMessage->getApplicationRegistrationInformation().
			getDIFName()) {
		std::cout << "DIF name on original and recovered messages"
				<< " are different\n";
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
	} else if (message->getApplicationName()
			!= recoveredMessage->getApplicationName()) {
		std::cout << "Application name on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message->getDifName()
			!= recoveredMessage->getDifName()) {
		std::cout << "DIF name on original and recovered messages"
				<< " are different\n";
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

int testAppUnregisterApplicationRequestMessage() {
	std::cout << "TESTING APP UNREGISTER APPLICATION REQUEST MESSAGE\n";
	int returnValue = 0;

	ApplicationProcessNamingInformation * applicationName =
			new ApplicationProcessNamingInformation();
	applicationName->setProcessName("/apps/source");
	applicationName->setProcessInstance("5");
	applicationName->setEntityName("database");
	applicationName->setEntityInstance("3");

	ApplicationProcessNamingInformation * difName =
			new ApplicationProcessNamingInformation();
	difName->setProcessName("/difs/Test2.DIF");

	AppUnregisterApplicationRequestMessage * message =
			new AppUnregisterApplicationRequestMessage();

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
		std::cout << "Error constructing Unregister Application Request "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		delete difName;
		delete applicationName;
		delete message;
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	AppUnregisterApplicationRequestMessage * recoveredMessage =
			dynamic_cast<AppUnregisterApplicationRequestMessage *>(parseBaseNetlinkMessage(
					netlinkMessageHeader));
	if (message == NULL) {
		std::cout << "Error parsing Unregister Application Request Message "
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
		std::cout << "AppUnregisterApplicationRequest test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete difName;
	delete applicationName;
	delete message;
	delete recoveredMessage;

	return returnValue;
}

int testAppUnregisterApplicationResponseMessage() {
	std::cout << "TESTING APP UNREGISTER APPLICATION RESPONSE MESSAGE\n";
	int returnValue = 0;

	ApplicationProcessNamingInformation * applicationName =
			new ApplicationProcessNamingInformation();
	applicationName->setProcessName("/apps/source");
	applicationName->setProcessInstance("5");
	applicationName->setEntityName("database");
	applicationName->setEntityInstance("3");

	AppUnregisterApplicationResponseMessage * message =
			new AppUnregisterApplicationResponseMessage();

	message->setResult(1);
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
		std::cout << "Error constructing Unregister Application Response "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		delete message;
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	AppUnregisterApplicationResponseMessage * recoveredMessage =
			dynamic_cast<AppUnregisterApplicationResponseMessage *>(parseBaseNetlinkMessage(
					netlinkMessageHeader));
	if (message == NULL) {
		std::cout << "Error parsing Register Application Response Message "
				<< "\n";
		returnValue = -1;
	} else if (message->getApplicationName()
			!= recoveredMessage->getApplicationName()) {
		std::cout << "Application name on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message->getResult() != recoveredMessage->getResult()) {
		std::cout << "Result on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "AppRegisterApplicationResponse test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete message;
	delete recoveredMessage;

	return returnValue;
}

int testAppGetDIFPropertiesRequestMessage() {
	std::cout << "TESTING APP GET DIF PROPERTIES REQUEST MESSAGE\n";
	int returnValue = 0;

	ApplicationProcessNamingInformation applicationName;
	applicationName.setProcessName("/apps/source");
	applicationName.setProcessInstance("5");
	applicationName.setEntityName("database");
	applicationName.setEntityInstance("3");

	ApplicationProcessNamingInformation difName;
	difName.setProcessName("/difs/Test2.DIF");

	AppGetDIFPropertiesRequestMessage message;
	message.setDifName(difName);
	message.setApplicationName(applicationName);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, &message);
	if (result < 0) {
		std::cout << "Error constructing Get DIF Properties Request "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	AppGetDIFPropertiesRequestMessage * recoveredMessage =
			dynamic_cast<AppGetDIFPropertiesRequestMessage *>(parseBaseNetlinkMessage(
					netlinkMessageHeader));
	if (recoveredMessage == 0) {
		std::cout << "Error parsing Get DIF Properties Request Message "
				<< "\n";
		returnValue = -1;
	} else if (message.getApplicationName()
			!= recoveredMessage->getApplicationName()) {
		std::cout << "Application name on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.getDifName() != recoveredMessage->getDifName()) {
		std::cout << "DIF name on original and recovered "
				<< "messages are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "AppGetDIFPropertiesRequest test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete recoveredMessage;

	return returnValue;
}

int testIpcmQueryRIBResponseMessage() {
	std::cout << "TESTING IPCM QUERY RIB RESPONSE MESSAGE\n";
	int returnValue = 0;

	IpcmDIFQueryRIBResponseMessage message;
	message.setResult(0);
	RIBObject * ribObject = new RIBObject();
	ribObject->setClazz("/test/clazz1");
	ribObject->setName("/test/name1");
	ribObject->setInstance(1234);
	message.addRIBObject(*ribObject);
	delete ribObject;

	ribObject = new RIBObject();
	ribObject->setClazz("/test/clazz2");
	ribObject->setName("/test/name2");
	ribObject->setInstance(343241);
	message.addRIBObject(*ribObject);
	delete ribObject;

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, &message);
	if (result < 0) {
		std::cout << "Error constructing Ipcm Query RIB response"
				<< "message \n";
		nlmsg_free(netlinkMessage);
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	IpcmDIFQueryRIBResponseMessage * recoveredMessage =
			dynamic_cast<IpcmDIFQueryRIBResponseMessage *>(
					parseBaseNetlinkMessage(netlinkMessageHeader));
	if (recoveredMessage == 0) {
		std::cout << "Error parsing Ipcm Query RIB response Message "
				<< "\n";
		returnValue = -1;
	} else if (message.getResult() != recoveredMessage->getResult()) {
		std::cout << "Result on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.getRIBObjects().size() !=
			recoveredMessage->getRIBObjects().size()){
		std::cout << "Number of RIB Objects on original and recovered messages"
				<< " are different " << message.getRIBObjects().size()<<" "
				<< recoveredMessage->getRIBObjects().size() <<std::endl;
		returnValue = -1;
	} else {
		std::list<RIBObject>::const_iterator iterator;
		int i = 0;
		for (iterator = recoveredMessage->getRIBObjects().begin();
				iterator != recoveredMessage->getRIBObjects().end();
				++iterator) {
			const RIBObject& ribObject = *iterator;
			if (i == 0){
				if (ribObject.getClazz().compare("/test/clazz1") != 0){
					std::cout << "RIB Object clazz on original and recovered messages"
							<< " are different\n";
					returnValue = -1;
					break;
				}else if (ribObject.getName().compare("/test/name1") != 0){
					std::cout << "RIB Object name on original and recovered messages"
							<< " are different\n";
					returnValue = -1;
					break;
				}else if (ribObject.getInstance() != 1234){
					std::cout << "RIB Object instance on original and recovered messages"
							<< " are different\n";
					returnValue = -1;
					break;
				}

				i++;
			}else if (i == 1){
				if (ribObject.getClazz().compare("/test/clazz2") != 0){
					std::cout << "RIB Object clazz on original and recovered messages"
							<< " are different " <<std::endl;
					returnValue = -1;
					break;
				} else if (ribObject.getName().compare("/test/name2") != 0){
					std::cout << "RIB Object name on original and recovered messages"
							<< " are different\n";
					returnValue = -1;
					break;
				} else if (ribObject.getInstance() != 343241){
					std::cout << "RIB Object instance on original and recovered messages"
							<< " are different\n";
					returnValue = -1;
					break;
				}
			}
		}
	}

	if (returnValue == 0) {
		std::cout << "IpcmDIFQueryRIBResponseMessage test ok\n";
	}
	nlmsg_free(netlinkMessage);
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

int testIpcmUnregisterApplicationRequestMessage() {
	std::cout << "TESTING IPCM UNREGISTER APPLICATION REQUEST MESSAGE\n";
	int returnValue = 0;

	ApplicationProcessNamingInformation applicationName;
	applicationName.setProcessName("/apps/source");
	applicationName.setProcessInstance("25");
	applicationName.setEntityName("database");
	applicationName.setEntityInstance("31");

	ApplicationProcessNamingInformation difName;
	difName.setProcessName("/difs/Test2.DIF");

	IpcmUnregisterApplicationRequestMessage message;
	message.setDifName(difName);
	message.setApplicationName(applicationName);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, &message);
	if (result < 0) {
		std::cout << "Error constructing Ipcm Unregister Application Request "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	IpcmUnregisterApplicationRequestMessage * recoveredMessage =
			dynamic_cast<IpcmUnregisterApplicationRequestMessage *>(
					parseBaseNetlinkMessage(netlinkMessageHeader));
	if (recoveredMessage == 0) {
		std::cout << "Error parsing Ipcm Unregister Application Request Message "
				<< "\n";
		returnValue = -1;
	} else if (message.getApplicationName()
			!= recoveredMessage->getApplicationName()) {
		std::cout << "Application name on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.getDifName() != recoveredMessage->getDifName()) {
		std::cout << "DIF name on original and recovered "
				<< "messages are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "IpcmUnregisterApplicationRequest test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete recoveredMessage;

	return returnValue;
}

int testIpcmUnregisterApplicationResponseMessage() {
	std::cout << "TESTING IPCM UNREGISTER APPLICATION RESPONSE MESSAGE\n";
	int returnValue = 0;

	IpcmUnregisterApplicationResponseMessage message;

	message.setResult(1);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, &message);
	if (result < 0) {
		std::cout << "Error constructing Ipcm Unregister Application Response "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	IpcmUnregisterApplicationResponseMessage * recoveredMessage =
			dynamic_cast<IpcmUnregisterApplicationResponseMessage *>(parseBaseNetlinkMessage(
					netlinkMessageHeader));
	if (recoveredMessage == 0) {
		std::cout << "Error parsing Ipcm Unregister Application Response Message "
				<< "\n";
		returnValue = -1;
	} else if (message.getResult() != recoveredMessage->getResult()) {
		std::cout << "Result on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "IpcmUnregisterApplicationResponse test ok\n";
	}
	nlmsg_free(netlinkMessage);
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
	difConfiguration->setDifType("shim-ethernet");
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
	} else if (message->getDIFConfiguration().getDifType().compare(
			recoveredMessage->getDIFConfiguration().getDifType()) != 0) {
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
	}

	if (returnValue == 0) {
		std::cout << "IpcmAllocateFlowRequestMessage test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete recoveredMessage;

	return returnValue;
}

int testIpcmAllocateFlowRequestResultMessage() {
	std::cout << "TESTING IPCM ALLOCATE FLOW REQUEST RESULT MESSAGE\n";
	int returnValue = 0;

	IpcmAllocateFlowRequestResultMessage message;
	message.setResult(-25);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, &message);
	if (result < 0) {
		std::cout << "Error constructing Ipcm Allocate Flow Request result "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	IpcmAllocateFlowRequestResultMessage * recoveredMessage =
			dynamic_cast<IpcmAllocateFlowRequestResultMessage *>(
					parseBaseNetlinkMessage(netlinkMessageHeader));
	if (recoveredMessage == 0) {
		std::cout << "Error parsing Ipcm Allocate Flow Request Result "
				<< "\n";
		returnValue = -1;
	} else if (message.getResult() != recoveredMessage->getResult()) {
		std::cout << "Result on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "IpcmAllocateFlowRequestResult test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete recoveredMessage;

	return returnValue;
}

int testIpcmAllocateFlowRequestArrivedMessage() {
	std::cout << "TESTING IPCM ALLOCATE FLOW REQUEST ARRIVED MESSAGE\n";
	int returnValue = 0;

	ApplicationProcessNamingInformation sourceName;
	sourceName.setProcessName("/apps/source");
	sourceName.setProcessInstance("12");
	sourceName.setEntityName("database");
	sourceName.setEntityInstance("12");

	ApplicationProcessNamingInformation destName;
	destName.setProcessName("/apps/dest");
	destName.setProcessInstance("12345");
	destName.setEntityName("printer");
	destName.setEntityInstance("12623456");

	ApplicationProcessNamingInformation difName;;
	difName.setProcessName("/difs/Test1.DIF");

	FlowSpecification flowSpec;

	IpcmAllocateFlowRequestArrivedMessage message;
	message.setSourceAppName(sourceName);
	message.setDestAppName(destName);
	message.setFlowSpecification(flowSpec);
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
		std::cout
		<< "Error constructing IPCM Allocate Flow Request Arrived"
		<< "Message \n";
		nlmsg_free(netlinkMessage);
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	IpcmAllocateFlowRequestArrivedMessage * recoveredMessage =
			dynamic_cast<IpcmAllocateFlowRequestArrivedMessage *>(parseBaseNetlinkMessage(
					netlinkMessageHeader));
	if (recoveredMessage == 0) {
		std::cout
		<< "Error parsing IPCM Allocate Flow Request Arrived Message "
		<< "\n";
		returnValue = -1;
	} else if (message.getSourceAppName()
			!= recoveredMessage->getSourceAppName()) {
		std::cout
		<< "Source application name on original and recovered messages"
		<< " are different\n";
		returnValue = -1;
	} else if (message.getDestAppName()
			!= recoveredMessage->getDestAppName()) {
		std::cout << "Destination application name on original and recovered "
				<< "messages are different\n";
		returnValue = -1;
	} else if (message.getFlowSpecification()
			!= recoveredMessage->getFlowSpecification()) {
		std::cout << "Destination flow specification on original and recovered "
				<< "messages are different\n";
		returnValue = -1;
	} else if (message.getDifName() != recoveredMessage->getDifName()) {
		std::cout << "DIF name on original and recovered "
				<< "messages are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "IpcmAllocateFlowRequestArrivedMessage test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete recoveredMessage;

	return returnValue;
}

int testIpcmAllocateFlowResponseMessage() {
	std::cout << "TESTING IPCM ALLOCATE FLOW RESPONSE MESSAGE\n";
	int returnValue = 0;

	IpcmAllocateFlowResponseMessage message;
	message.setResult(0);
	message.setNotifySource(true);
	message.setPortId(345);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, &message);
	if (result < 0) {
		std::cout << "Error constructing IPCM Allocate Flow Response "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	IpcmAllocateFlowResponseMessage * recoveredMessage =
			dynamic_cast<IpcmAllocateFlowResponseMessage *>(parseBaseNetlinkMessage(
					netlinkMessageHeader));
	if (recoveredMessage == 0) {
		std::cout << "Error parsing IPCM Allocate Flow Response Message "
				<< "\n";
		returnValue = -1;

	} else if (message.getResult() != recoveredMessage->getResult()) {
		std::cout << "Result on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.isNotifySource()
			!= recoveredMessage->isNotifySource()) {
		std::cout << "Notify source flag on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.getPortId() != recoveredMessage->getPortId()) {
		std::cout << "Port id on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "IpcmAllocateFlowResponse test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete recoveredMessage;

	return returnValue;
}

int testIpcmDeallocateFlowRequestMessage() {
	std::cout << "TESTING IPCM DEALLOCATE FLOW REQUEST MESSAGE\n";
	int returnValue = 0;

	IpcmDeallocateFlowRequestMessage message;;
	message.setPortId(47);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, &message);
	if (result < 0) {
		std::cout << "Error constructing IPCM Deallocate Flow Request "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	IpcmDeallocateFlowRequestMessage * recoveredMessage =
			dynamic_cast<IpcmDeallocateFlowRequestMessage *>(parseBaseNetlinkMessage(
					netlinkMessageHeader));
	if (recoveredMessage == 0) {
		std::cout
		<< "Error parsing IPCM Deallocate Flow Request Message "
		<< "\n";
		returnValue = -1;
	} else if (message.getPortId() != recoveredMessage->getPortId()) {
		std::cout << "PortId on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "IpcmDeallocateFlowRequest test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete recoveredMessage;

	return returnValue;
}

int testIpcmDeallocateFlowResponseMessage() {
	std::cout << "TESTING IPCM DEALLOCATE FLOW RESPONSE MESSAGE\n";
	int returnValue = 0;

	IpcmDeallocateFlowResponseMessage message;
	message.setResult(0);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, &message);
	if (result < 0) {
		std::cout << "Error constructing IPCM Deallocate Flow Response "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	IpcmDeallocateFlowResponseMessage * recoveredMessage =
			dynamic_cast<IpcmDeallocateFlowResponseMessage *>(parseBaseNetlinkMessage(
					netlinkMessageHeader));
	if (recoveredMessage == 0) {
		std::cout
		<< "Error parsing IPCM Deallocate Flow Response Message "
		<< "\n";
		returnValue = -1;
	} else if (message.getResult() != recoveredMessage->getResult()) {
		std::cout << "Result on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "IpcmDeallocateFlowResponse test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete recoveredMessage;

	return returnValue;
}

int testIpcmFlowDeallocatedNotificationMessage() {
	std::cout << "TESTING IPCM FLOW DEALLOCATED NOTIFICATION MESSAGE\n";
	int returnValue = 0;

	IpcmFlowDeallocatedNotificationMessage message;;
	message.setPortId(47);
	message.setCode(7);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, &message);
	if (result < 0) {
		std::cout
		<< "Error constructing IPCM Flow Deallocated Notification "
		<< "Message \n";
		nlmsg_free(netlinkMessage);
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	IpcmFlowDeallocatedNotificationMessage * recoveredMessage =
			dynamic_cast<IpcmFlowDeallocatedNotificationMessage *>(parseBaseNetlinkMessage(
					netlinkMessageHeader));
	if (recoveredMessage == 0) {
		std::cout
		<< "Error parsing IPCM Flow Deallocated Notification Message "
		<< "\n";
		returnValue = -1;
	} else if (message.getPortId() != recoveredMessage->getPortId()) {
		std::cout << "PortId on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.getCode() != recoveredMessage->getCode()) {
		std::cout << "Code on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "IpcmDeallocateFlowRequest test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete recoveredMessage;

	return returnValue;
}

int testIpcmIPCProcessDIFResgistrationNotification() {
	std::cout << "TESTING IPCM IPC PROCESS DIF REGISTRATION NOTIFICATION\n";
	int returnValue = 0;

	IpcmDIFRegistrationNotification message;
	ApplicationProcessNamingInformation ipcProcessName;
	ipcProcessName.setProcessName("/ipcprocesses/Barcelona/i2CAT");
	ipcProcessName.setProcessInstance("1");
	ipcProcessName.setEntityName("Management");
	ipcProcessName.setProcessInstance("1234");
	message.setIpcProcessName(ipcProcessName);
	ApplicationProcessNamingInformation difName;
	difName.setProcessName("/difs/Test.DIF");
	message.setDifName(difName);
	message.setRegistered(true);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, &message);
	if (result < 0) {
		std::cout << "Error constructing Ipcm IPC Process DIF registration"
				<< "Notification Message \n";
		nlmsg_free(netlinkMessage);
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	IpcmDIFRegistrationNotification * recoveredMessage =
			dynamic_cast<IpcmDIFRegistrationNotification *>(
					parseBaseNetlinkMessage(netlinkMessageHeader));
	if (recoveredMessage == 0) {
		std::cout << "Error parsing Ipcm IPC Process DIF Registration Message "
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
	} else if (message.isRegistered() != recoveredMessage->isRegistered()) {
		std::cout << "Registered on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "IpcmDIFRegistrationNotification test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete recoveredMessage;

	return returnValue;
}

int testIpcmQueryRIBRequestMessage() {
	std::cout << "TESTING IPCM QUERY RIB REQUEST MESSAGE\n";
	int returnValue = 0;

	IpcmDIFQueryRIBRequestMessage message;
	message.setObjectClass("flow");
	message.setObjectName("/dif/management/flows/234556");
	message.setObjectInstance(1234847);
	message.setScope(3);
	message.setFilter("sourcePortId=5");

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, &message);
	if (result < 0) {
		std::cout << "Error constructing Ipcm Query RIB request"
				<< "message \n";
		nlmsg_free(netlinkMessage);
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	IpcmDIFQueryRIBRequestMessage * recoveredMessage =
			dynamic_cast<IpcmDIFQueryRIBRequestMessage *>(
					parseBaseNetlinkMessage(netlinkMessageHeader));
	if (recoveredMessage == 0) {
		std::cout << "Error parsing Ipcm Query RIB request Message "
				<< "\n";
		returnValue = -1;
	} else if (message.getObjectClass().compare(
			recoveredMessage->getObjectClass()) != 0) {
		std::cout << "Object class on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.getObjectName().compare(
			recoveredMessage->getObjectName())!=0) {
		std::cout << "Object name on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.getObjectInstance() !=
			recoveredMessage->getObjectInstance()) {
		std::cout << "Object instance on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.getScope() != recoveredMessage->getScope()) {
		std::cout << "Scope on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.getFilter().compare(
			recoveredMessage->getFilter())!=0) {
		std::cout << "Filter on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "IpcmDIFQueryRIBRequestMessage test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete recoveredMessage;

	return returnValue;
}

int testAppGetDIFPropertiesResponseMessage() {
	std::cout << "TESTING APP QUERY DIF PROPERTIES RESPONSE MESSAGE\n";
	int returnValue = 0;

	ApplicationProcessNamingInformation appName;
	appName.setProcessName("/test/apps/rinaband");
	appName.setProcessInstance("1");
	appName.setEntityName("control");
	appName.setEntityInstance("23");
	ApplicationProcessNamingInformation difName;
	difName.setProcessName("/difs/Test.DIF");
	ApplicationProcessNamingInformation difName2;
	difName2.setProcessName("/difs/shim-dummy.DIF");

	AppGetDIFPropertiesResponseMessage message;
	message.setResult(0);
	message.setApplicationName(appName);
	DIFProperties * difProperties = new DIFProperties(difName, 9000);
	message.addDIFProperty(*difProperties);
	delete difProperties;
	difProperties = new DIFProperties(difName2, 25000);
	message.addDIFProperty(*difProperties);
	delete difProperties;

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, &message);
	if (result < 0) {
		std::cout << "Error constructing App Get DIF Properties response"
				<< "message \n";
		nlmsg_free(netlinkMessage);
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	AppGetDIFPropertiesResponseMessage * recoveredMessage =
			dynamic_cast<AppGetDIFPropertiesResponseMessage *>(
					parseBaseNetlinkMessage(netlinkMessageHeader));
	if (recoveredMessage == 0) {
		std::cout << "Error parsing App Get DIF Properties Response Message "
				<< "\n";
		returnValue = -1;
	} else if (message.getResult() != recoveredMessage->getResult()) {
		std::cout << "Result on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.getDIFProperties().size() !=
			recoveredMessage->getDIFProperties().size()){
		std::cout << "Number of DIF Properties on original and recovered messages"
				<< " are different " << message.getDIFProperties().size()<<" "
				<< recoveredMessage->getDIFProperties().size() <<std::endl;
		returnValue = -1;
	} else {
		std::list<DIFProperties>::const_iterator iterator;
		int i = 0;
		for (iterator = recoveredMessage->getDIFProperties().begin();
				iterator != recoveredMessage->getDIFProperties().end();
				++iterator) {
			const DIFProperties& difProperties = *iterator;
			if (i == 0){
				if (difProperties.getDifName() != difName){
					std::cout << "DIF name on original and recovered messages"
							<< " are different 1\n";
					returnValue = -1;
					break;
				}else if (difProperties.getMaxSduSize()!= 9000){
					std::cout << "Max SDU size on original and recovered messages"
							<< " are different 1\n";
					returnValue = -1;
					break;
				}

				i++;
			}else if (i == 1){
				if (difProperties.getDifName() != difName2){
					std::cout << "DIF name on original and recovered messages"
							<< " are different 2 \n";
					returnValue = -1;
					break;
				}else if (difProperties.getMaxSduSize()!= 25000){
					std::cout << "Max SDU size on original and recovered messages"
							<< " are different 2 \n";
					returnValue = -1;
					break;
				}
			}
		}
	}

	if (returnValue == 0) {
		std::cout << "AppGetDIFPropertiesResponse test ok\n";
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

	result = testAppUnregisterApplicationRequestMessage();
	if (result < 0) {
		return result;
	}

	result = testAppUnregisterApplicationResponseMessage();
	if (result < 0) {
		return result;
	}

	result = testAppGetDIFPropertiesRequestMessage();
	if (result < 0) {
		return result;
	}

	result = testAppGetDIFPropertiesResponseMessage();
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

	result = testIpcmUnregisterApplicationRequestMessage();
	if (result < 0) {
		return result;
	}

	result = testIpcmUnregisterApplicationResponseMessage();
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

	result = testIpcmAllocateFlowRequestResultMessage();
	if (result < 0) {
		return result;
	}

	result = testIpcmAllocateFlowRequestArrivedMessage();
	if (result < 0) {
		return result;
	}

	result = testIpcmAllocateFlowResponseMessage();
	if (result < 0) {
		return result;
	}

	result = testIpcmDeallocateFlowRequestMessage();
	if (result < 0) {
		return result;
	}

	result = testIpcmDeallocateFlowResponseMessage();
	if (result < 0) {
		return result;
	}

	result = testAppFlowDeallocatedNotificationMessage();
	if (result < 0) {
		return result;
	}

	result = testIpcmIPCProcessDIFResgistrationNotification();
	if (result < 0) {
		return result;
	}

	result = testIpcmQueryRIBRequestMessage();
	if (result < 0) {
		return result;
	}

	result = testIpcmQueryRIBResponseMessage();
	if (result < 0) {
		return result;
	}
}
