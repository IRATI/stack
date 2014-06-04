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

	FlowSpecification flowSpec;

	ApplicationProcessNamingInformation difName;
	difName.setProcessName("test.DIF");

	AppAllocateFlowRequestMessage message;
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
		std::cout << "Error constructing Application Allocate Flow request "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	AppAllocateFlowRequestMessage * recoveredMessage =
			dynamic_cast<AppAllocateFlowRequestMessage *>(parseBaseNetlinkMessage(
					netlinkMessageHeader));
	if (recoveredMessage == NULL) {
		std::cout << "Error parsing Application Allocate Flow request Message "
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
	} else if (message.getDifName()
			!= recoveredMessage->getDifName()) {
		std::cout << "DIF name on original and recovered "
				<< "messages are different\n";
		returnValue = -1;
	} else if (message.getFlowSpecification()
                        != recoveredMessage->getFlowSpecification()) {
                std::cout << "Destination flow specification on original and recovered "
                                << "messages are different\n";
                returnValue = -1;
        }

	if (returnValue == 0) {
		std::cout << "AppAllocateFlowRequestMessage test ok\n";
	}
	nlmsg_free(netlinkMessage);
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
	message.setResult(0);
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

	} else if (message.getResult() != recoveredMessage->getResult()) {
		std::cout << "Result on original and recovered messages"
				<< " are different\n";
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
	message.setPortId(234);
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
	} else if (message.getPortId() != recoveredMessage->getPortId()) {
                std::cout << "Port id on original and recovered messages"
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
	appRegInfo.setApplicationName(*applicationName);

	message->setApplicationRegistrationInformation(appRegInfo);

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
	} else if (message->getApplicationRegistrationInformation().getApplicationName()
			!= recoveredMessage->getApplicationRegistrationInformation().
			getApplicationName()) {
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
	ribObject->setDisplayableValue("This is my value");
	message.addRIBObject(*ribObject);
	delete ribObject;

	ribObject = new RIBObject();
	ribObject->setClazz("/test/clazz2");
	ribObject->setName("/test/name2");
	ribObject->setInstance(343241);
	ribObject->setDisplayableValue("This is my value2");
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
				}else if (ribObject.getDisplayableValue().compare("This is my value") != 0){
                                        std::cout << "RIB Object display value on original and recovered messages"
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
				} else if (ribObject.getDisplayableValue().compare("This is my value2") != 0){
                                        std::cout << "RIB Object display value on original and recovered messages"
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
	message->setRegIpcProcessId(123);

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
	} else if (message->getRegIpcProcessId() !=
	                recoveredMessage->getRegIpcProcessId()) {
	        std::cout << "RegIpcProcessId on original and recovered "
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

	ApplicationProcessNamingInformation  difName;
	difName.setProcessName("/difs/Test.DIF");

	IpcmAssignToDIFRequestMessage message;
	DIFInformation difInformation;
	DIFConfiguration difConfiguration;
	difInformation.setDifType(NORMAL_IPC_PROCESS);
	difInformation.setDifName(difName);
	Parameter * parameter = new Parameter("interface", "eth0");
	difConfiguration.addParameter(*parameter);
	delete parameter;
	parameter = new Parameter("vlanid", "430");
	difConfiguration.addParameter(*parameter);
	difConfiguration.setAddress(34);
	delete parameter;
	EFCPConfiguration efcpConfiguration;
	DataTransferConstants dataTransferConstants;
	dataTransferConstants.setAddressLength(1);
	dataTransferConstants.setCepIdLength(2);
	dataTransferConstants.setDifIntegrity(true);
	dataTransferConstants.setLengthLength(3);
	dataTransferConstants.setMaxPduLifetime(4);
	dataTransferConstants.setMaxPduSize(5);
	dataTransferConstants.setPortIdLength(6);
	dataTransferConstants.setQosIdLenght(7);
	dataTransferConstants.setSequenceNumberLength(8);
	efcpConfiguration.setDataTransferConstants(dataTransferConstants);
	QoSCube * qosCube = new QoSCube("cube 1", 1);
	efcpConfiguration.addQoSCube(*qosCube);
	delete qosCube;
	qosCube = new QoSCube("cube 2", 2);
	efcpConfiguration.addQoSCube(*qosCube);
	qosCube->setEfcpPolicies(ConnectionPolicies());
	delete qosCube;
	difConfiguration.setEfcpConfiguration(efcpConfiguration);
	difInformation.setDifConfiguration(difConfiguration);
	message.setDIFInformation(difInformation);

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, &message);
	if (result < 0) {
		std::cout << "Error constructing Ipcm Assign To DIF Request "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	IpcmAssignToDIFRequestMessage * recoveredMessage =
			dynamic_cast<IpcmAssignToDIFRequestMessage *>(
					parseBaseNetlinkMessage(netlinkMessageHeader));

	if (recoveredMessage == 0) {
		std::cout << "Error parsing Ipcm Assign To DIF Request Message "
				<< "\n";
		returnValue = -1;
	} else if (message.getDIFInformation().getDifType().compare(
			recoveredMessage->getDIFInformation().getDifType()) != 0) {
		std::cout << "DIFInformation.difType on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.getDIFInformation().getDifName() !=
			recoveredMessage->getDIFInformation().getDifName()) {
		std::cout << "DIFInformation.difName on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.getDIFInformation().getDifConfiguration().getParameters().size() !=
			recoveredMessage->getDIFInformation().getDifConfiguration().getParameters().size()){
		std::cout << "DIFInformation.DIFConfiguration.parameters.size on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.getDIFInformation().getDifConfiguration().getEfcpConfiguration().
	                getDataTransferConstants().getAddressLength() !=
	                                recoveredMessage->getDIFInformation().getDifConfiguration().
	                                getEfcpConfiguration().getDataTransferConstants().getAddressLength()) {
	        std::cout << "DIFInformation.DIFConfiguration.dtc.addrLength on original and recovered messages"
	                        << " are different\n";
	        returnValue = -1;
	} else if (message.getDIFInformation().getDifConfiguration().getEfcpConfiguration().
                        getDataTransferConstants().getCepIdLength() !=
                                        recoveredMessage->getDIFInformation().getDifConfiguration().
                                        getEfcpConfiguration().getDataTransferConstants().getCepIdLength()) {
                std::cout << "DIFInformation.DIFConfiguration.dtc.cepIdLength on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getDIFInformation().getDifConfiguration().getEfcpConfiguration().
                        getDataTransferConstants().getLengthLength() !=
                                        recoveredMessage->getDIFInformation().getDifConfiguration().
                                        getEfcpConfiguration().getDataTransferConstants().getLengthLength()) {
                std::cout << "DIFInformation.DIFConfiguration.dtc.lengthLength on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getDIFInformation().getDifConfiguration().getEfcpConfiguration().
                        getDataTransferConstants().getMaxPduLifetime() !=
                                        recoveredMessage->getDIFInformation().getDifConfiguration().
                                        getEfcpConfiguration().getDataTransferConstants().getMaxPduLifetime()) {
                std::cout << "DIFInformation.DIFConfiguration.dtc.maxPDULifetime on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getDIFInformation().getDifConfiguration().getEfcpConfiguration().
                        getDataTransferConstants().getMaxPduSize() !=
                                        recoveredMessage->getDIFInformation().getDifConfiguration().
                                        getEfcpConfiguration().getDataTransferConstants().getMaxPduSize()) {
                std::cout << "DIFInformation.DIFConfiguration.dtc.maxPDUSize on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getDIFInformation().getDifConfiguration().getEfcpConfiguration().
                        getDataTransferConstants().getPortIdLength() !=
                                        recoveredMessage->getDIFInformation().getDifConfiguration().
                                        getEfcpConfiguration().getDataTransferConstants().getPortIdLength()) {
                std::cout << "DIFInformation.DIFConfiguration.dtc.portIdLength on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getDIFInformation().getDifConfiguration().getEfcpConfiguration().
                        getDataTransferConstants().getQosIdLenght() !=
                                        recoveredMessage->getDIFInformation().getDifConfiguration().
                                        getEfcpConfiguration().getDataTransferConstants().getQosIdLenght()) {
                std::cout << "DIFInformation.DIFConfiguration.dtc.qosIdLength on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getDIFInformation().getDifConfiguration().getEfcpConfiguration().
                        getDataTransferConstants().getSequenceNumberLength() !=
                                        recoveredMessage->getDIFInformation().getDifConfiguration().
                                        getEfcpConfiguration().getDataTransferConstants().getSequenceNumberLength()) {
                std::cout << "DIFInformation.DIFConfiguration.dtc.seqNumLength on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getDIFInformation().getDifConfiguration().getEfcpConfiguration().
                        getDataTransferConstants().isDifIntegrity() !=
                                        recoveredMessage->getDIFInformation().getDifConfiguration().
                                        getEfcpConfiguration().getDataTransferConstants().isDifIntegrity()) {
                std::cout << "DIFInformation.DIFConfiguration.dtc.difIntegrity on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getDIFInformation().getDifConfiguration().getAddress() !=
                                        recoveredMessage->getDIFInformation().getDifConfiguration().
                                        getAddress()) {
                std::cout << "DIFInformation.DIFConfiguration.address original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getDIFInformation().getDifConfiguration().getEfcpConfiguration().getQosCubes().size() !=
                        recoveredMessage->getDIFInformation().getDifConfiguration().
                        getEfcpConfiguration().getQosCubes().size()) {
                std::cout << "DIFInformation.DIFConfiguration.qosCubes.size original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        }


	if (returnValue == 0) {
		std::cout << "IpcmAssignToDIFRequest test ok\n";
	}
	nlmsg_free(netlinkMessage);
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

int testIpcmUpdateDIFConfigurationRequestMessage() {
        std::cout << "TESTING IPCM UPDATE DIF CONFIGURATION REQUEST MESSAGE\n";
        int returnValue = 0;

        IpcmUpdateDIFConfigurationRequestMessage message;
        DIFConfiguration difConfiguration;
        Parameter * parameter = new Parameter("interface", "eth0");
        difConfiguration.addParameter(*parameter);
        delete parameter;
        parameter = new Parameter("vlanid", "430");
        difConfiguration.addParameter(*parameter);
        delete parameter;
        message.setDIFConfiguration(difConfiguration);

        struct nl_msg* netlinkMessage;
        netlinkMessage = nlmsg_alloc();
        if (!netlinkMessage) {
                std::cout << "Error allocating Netlink message\n";
        }
        genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
                        sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

        int result = putBaseNetlinkMessage(netlinkMessage, &message);
        if (result < 0) {
                std::cout << "Error constructing Ipcm Update DIF Configuration Request "
                                << "Message \n";
                nlmsg_free(netlinkMessage);
                return result;
        }

        nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
        IpcmUpdateDIFConfigurationRequestMessage * recoveredMessage =
                        dynamic_cast<IpcmUpdateDIFConfigurationRequestMessage *>(
                                        parseBaseNetlinkMessage(netlinkMessageHeader));
        if (recoveredMessage == 0) {
                std::cout << "Error parsing Ipcm Update DIF Configuration Message "
                                << "\n";
                returnValue = -1;
        } else if (message.getDIFConfiguration().getParameters().size() !=
                        recoveredMessage->getDIFConfiguration().getParameters().size()){
                std::cout << "DIFConfiguration.parameters.size on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        }

        if (returnValue == 0) {
                std::cout << "IpcmUpdateDIFConfigurationRequest test ok\n";
        }
        nlmsg_free(netlinkMessage);
        delete recoveredMessage;

        return returnValue;
}

int testIpcmUpdateDIFConfigurationResponseMessage() {
        std::cout << "TESTING IPCM UPDATE DIF CONFIGURATION RESPONSE MESSAGE\n";
        int returnValue = 0;

        IpcmUpdateDIFConfigurationResponseMessage * message =
                        new IpcmUpdateDIFConfigurationResponseMessage();
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
                std::cout << "Error constructing Ipcm Update DIF Configuration Response "
                                << "Message \n";
                nlmsg_free(netlinkMessage);
                delete message;
                return result;
        }

        nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
        IpcmUpdateDIFConfigurationResponseMessage * recoveredMessage =
                        dynamic_cast<IpcmUpdateDIFConfigurationResponseMessage *>(
                                        parseBaseNetlinkMessage(netlinkMessageHeader));
        if (message == 0) {
                std::cout << "Error parsing Ipcm Update DIF Configuration Response Message "
                                << "\n";
                returnValue = -1;
        } else if (message->getResult() != recoveredMessage->getResult()) {
                std::cout << "Result on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        }

        if (returnValue == 0) {
                std::cout << "IpcmUpdateDIFConfigurationResponse test ok\n";
        }
        nlmsg_free(netlinkMessage);
        delete message;
        delete recoveredMessage;

        return returnValue;
}

int testIpcmEnrollToDIFRequestMessage() {
        std::cout << "TESTING IPCM ENROLL TO DIF REQUEST MESSAGE\n";
        int returnValue = 0;

        IpcmEnrollToDIFRequestMessage message;
        ApplicationProcessNamingInformation difName;
        difName.setProcessName("normal.DIF");
        ApplicationProcessNamingInformation supportingDifName;
        supportingDifName.setProcessName("100");
        ApplicationProcessNamingInformation neighborName;
        neighborName.setProcessName("test");
        neighborName.setProcessInstance("1");
        message.setDifName(difName);
        message.setSupportingDifName(supportingDifName);
        message.setNeighborName(neighborName);

        struct nl_msg* netlinkMessage;
        netlinkMessage = nlmsg_alloc();
        if (!netlinkMessage) {
                std::cout << "Error allocating Netlink message\n";
        }
        genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
                        sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

        int result = putBaseNetlinkMessage(netlinkMessage, &message);
        if (result < 0) {
                std::cout << "Error constructing Ipcm Enroll to DIF Request "
                                << "Message \n";
                nlmsg_free(netlinkMessage);
                return result;
        }

        nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
        IpcmEnrollToDIFRequestMessage * recoveredMessage =
                        dynamic_cast<IpcmEnrollToDIFRequestMessage *>(
                                        parseBaseNetlinkMessage(netlinkMessageHeader));
        if (recoveredMessage == 0) {
                std::cout << "Error parsing Enroll to DIF Request Message "
                                << "\n";
                returnValue = -1;
        } else if (message.getDifName() != recoveredMessage->getDifName()){
                std::cout << "DIF Name on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getSupportingDifName() != recoveredMessage->getSupportingDifName()){
                std::cout << "Supporting DIF Name on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getNeighborName() != recoveredMessage->getNeighborName()){
                std::cout << "Neighbour Name on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        }

        if (returnValue == 0) {
                std::cout << "IpcmEnrollToDIFRequestMessage test ok\n";
        }
        nlmsg_free(netlinkMessage);
        delete recoveredMessage;

        return returnValue;
}

int testIpcmEnrollToDIFResponseMessage() {
        std::cout << "TESTING IPCM ENROLL TO DIF RESPONSE MESSAGE\n";
        int returnValue = 0;

        IpcmEnrollToDIFResponseMessage message;
        message.setResult(-25);
        Neighbor neighbor;
        ApplicationProcessNamingInformation name;
        name.setProcessName("test");
        name.setProcessInstance("1");
        neighbor.setName(name);
        ApplicationProcessNamingInformation supportingDIF;
        supportingDIF.setProcessName("100");
        neighbor.setSupportingDifName(supportingDIF);
        message.addNeighbor(neighbor);

        struct nl_msg* netlinkMessage;
        netlinkMessage = nlmsg_alloc();
        if (!netlinkMessage) {
                std::cout << "Error allocating Netlink message\n";
        }
        genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
                        sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

        int result = putBaseNetlinkMessage(netlinkMessage, &message);
        if (result < 0) {
                std::cout << "Error constructing Ipcm Enroll To DIF Response "
                                << "Message \n";
                nlmsg_free(netlinkMessage);
                return result;
        }

        nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
        IpcmEnrollToDIFResponseMessage * recoveredMessage =
                        dynamic_cast<IpcmEnrollToDIFResponseMessage *>(
                                        parseBaseNetlinkMessage(netlinkMessageHeader));
        if (recoveredMessage == 0) {
                std::cout << "Error parsing Ipcm Enroll to DIF Response Message "
                                << "\n";
                returnValue = -1;
        } else if (message.getResult() != recoveredMessage->getResult()) {
                std::cout << "Result on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getNeighbors().size() !=
                        recoveredMessage->getNeighbors().size()) {
                std::cout << "Number of neighbors on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        }

        if (returnValue == 0) {
                std::cout << "IpcmEnrollToDIFResponseMessage test ok\n";
        }
        nlmsg_free(netlinkMessage);
        delete recoveredMessage;

        return returnValue;
}

int testIpcmNeighborsModifiedNotificaiton() {
        std::cout << "TESTING IPCM NEIGHBORS MODIFIED NOTIFICATION MESSAGE\n";
        int returnValue = 0;

        IpcmNotifyNeighborsModifiedMessage message;
        message.setAdded(false);
        Neighbor neighbor;
        ApplicationProcessNamingInformation name;
        name.setProcessName("test");
        name.setProcessInstance("1");
        neighbor.setName(name);
        ApplicationProcessNamingInformation supportingDIF;
        supportingDIF.setProcessName("100");
        neighbor.setSupportingDifName(supportingDIF);
        message.addNeighbor(neighbor);

        struct nl_msg* netlinkMessage;
        netlinkMessage = nlmsg_alloc();
        if (!netlinkMessage) {
                std::cout << "Error allocating Netlink message\n";
        }
        genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
                        sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

        int result = putBaseNetlinkMessage(netlinkMessage, &message);
        if (result < 0) {
                std::cout << "Error constructing Ipcm Modify Neighbors notificaiton "
                                << "Message \n";
                nlmsg_free(netlinkMessage);
                return result;
        }

        nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
        IpcmNotifyNeighborsModifiedMessage * recoveredMessage =
                        dynamic_cast<IpcmNotifyNeighborsModifiedMessage *>(
                                        parseBaseNetlinkMessage(netlinkMessageHeader));
        if (recoveredMessage == 0) {
                std::cout << "Error parsing Ipcm Modify Neighbors notification Message "
                                << "\n";
                returnValue = -1;
        } else if (message.isAdded() != recoveredMessage->isAdded()) {
                std::cout << "Added on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getNeighbors().size() !=
                        recoveredMessage->getNeighbors().size()) {
                std::cout << "Number of neighbors on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        }

        if (returnValue == 0) {
                std::cout << "IpcmNotifyNeighborsModifiedMessage test ok\n";
        }
        nlmsg_free(netlinkMessage);
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

int testIpcpCreateConnectionRequest() {
        std::cout << "TESTING IPCP CREATE CONNECTION REQUEST MESSAGE\n";
        int returnValue = 0;

        DTCPRateBasedFlowControlConfig rate;
        rate.setSendingrate(345);
        rate.setTimeperiod(12456);

        DTCPWindowBasedFlowControlConfig window;
        window.setInitialcredit(98);
        window.setMaxclosedwindowqueuelength(23);

        DTCPFlowControlConfig dtcpFlowCtrlConfig;
        dtcpFlowCtrlConfig.setRatebased(true);
        dtcpFlowCtrlConfig.setRatebasedconfig(rate);
        dtcpFlowCtrlConfig.setWindowbased(true);
        dtcpFlowCtrlConfig.setWindowbasedconfig(window);
        dtcpFlowCtrlConfig.setRcvbuffersthreshold(123);
        dtcpFlowCtrlConfig.setRcvbytespercentthreshold(50);
        dtcpFlowCtrlConfig.setRcvbytespercentthreshold(456);
        dtcpFlowCtrlConfig.setSentbuffersthreshold(32);
        dtcpFlowCtrlConfig.setSentbytespercentthreshold(23);
        dtcpFlowCtrlConfig.setSentbytesthreshold(675);
        PolicyConfig closedWindowPolicy =
                        PolicyConfig("test-closed-window", "23");
        dtcpFlowCtrlConfig.setClosedwindowpolicy(closedWindowPolicy);

        DTCPRtxControlConfig dtcpRtxConfig;
        dtcpRtxConfig.setDatarxmsnmax(23424);

        DTCPConfig dtcpConfig;
        dtcpConfig.setFlowcontrol(true);
        dtcpConfig.setFlowcontrolconfig(dtcpFlowCtrlConfig);
        dtcpConfig.setRtxcontrol(true);
        dtcpConfig.setRtxcontrolconfig(dtcpRtxConfig);
        dtcpConfig.setInitialrecvrinactivitytime(234);
        dtcpConfig.setInitialsenderinactivitytime(123);

        ConnectionPolicies connectionPolicies;
        connectionPolicies.setDtcpPresent(true);
        connectionPolicies.setDtcpConfiguration(dtcpConfig);
        connectionPolicies.setSeqnumrolloverthreshold(123456);
        connectionPolicies.setInitialATimer(34562);
        connectionPolicies.setPartialDelivery(true);
        connectionPolicies.setMaxSduGap(34);
        connectionPolicies.setInOrderDelivery(true);
        connectionPolicies.setIncompleteDelivery(true);

        IpcpConnectionCreateRequestMessage message;
        Connection connection;
        connection.setPortId(25);
        connection.setSourceAddress(1);
        connection.setDestAddress(2);
        connection.setQosId(3);
        connection.setPolicies(connectionPolicies);
        message.setConnection(connection);

        struct nl_msg* netlinkMessage;
        netlinkMessage = nlmsg_alloc();
        if (!netlinkMessage) {
                std::cout << "Error allocating Netlink message\n";
        }
        genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
                        sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

        int result = putBaseNetlinkMessage(netlinkMessage, &message);
        if (result < 0) {
                std::cout << "Error constructing Ipcp Create Connection request"
                                << "message \n";
                nlmsg_free(netlinkMessage);
                return result;
        }

        nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
        IpcpConnectionCreateRequestMessage * recoveredMessage =
                        dynamic_cast<IpcpConnectionCreateRequestMessage *>(
                                        parseBaseNetlinkMessage(netlinkMessageHeader));
        if (recoveredMessage == 0) {
                std::cout << "Error parsing Ipcp Create Connection request Message "
                                << "\n";
                returnValue = -1;
        } else if (message.getConnection().getPortId() !=
                        recoveredMessage->getConnection().getPortId()) {
                std::cout << "Port id on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getSourceAddress()
                        != recoveredMessage->getConnection().getSourceAddress()) {
                std::cout << "Source address  on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getDestAddress() !=
                        recoveredMessage->getConnection().getDestAddress()) {
                std::cout <<message.getConnection().getDestAddress()
                                <<" "<<recoveredMessage->getConnection().getDestAddress();
                std::cout << "\nDestination address on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getQosId() !=
                        recoveredMessage->getConnection().getQosId()) {
                std::cout << "QoS id on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getSourceCepId()!=
                        recoveredMessage->getConnection().getSourceCepId()) {
                std::cout << "Src CEP id on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getDestCepId()!=
                        recoveredMessage->getConnection().getDestCepId()) {
                std::cout << "Dest CEP id on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getPolicies().isPartialDelivery()!=
                        recoveredMessage->getConnection().getPolicies().isPartialDelivery()) {
                std::cout << "Partial delivery on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getPolicies().isInOrderDelivery()!=
                        recoveredMessage->getConnection().getPolicies().isInOrderDelivery()) {
                std::cout << "In order delivery on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getPolicies().isIncompleteDelivery()!=
                        recoveredMessage->getConnection().getPolicies().isIncompleteDelivery()) {
                std::cout << "Incomplete delivery on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getPolicies().getMaxSduGap()!=
                        recoveredMessage->getConnection().getPolicies().getMaxSduGap()) {
                std::cout << "Max SDU gap on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getPolicies().isDtcpPresent() !=
                        recoveredMessage->getConnection().getPolicies().isDtcpPresent()) {
                std::cout << "ConnPolicies.isDTCPPresent on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getPolicies().getSeqnumrolloverthreshold() !=
                        recoveredMessage->getConnection().getPolicies().getSeqnumrolloverthreshold()) {
                std::cout << "ConnPolicies.seqnumrolloverthreshold on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getPolicies().getDtcpConfiguration().isFlowcontrol() !=
                        recoveredMessage->getConnection().getPolicies().getDtcpConfiguration().isFlowcontrol()){
                std::cout << "ConnPolicies.dtcpconfig.isFlowcontrol on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getPolicies().getDtcpConfiguration().isRtxcontrol() !=
                        recoveredMessage->getConnection().getPolicies().getDtcpConfiguration().isRtxcontrol()){
                std::cout << "ConnPolicies.dtcpconfig.isRtxcontrol on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getPolicies().getDtcpConfiguration().getInitialrecvrinactivitytime() !=
                        recoveredMessage->getConnection().getPolicies().getDtcpConfiguration().getInitialrecvrinactivitytime()){
                std::cout << "ConnPolicies.dtcpconfig.initialrcvrinatcitvitytime on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getPolicies().getDtcpConfiguration().getInitialsenderinactivitytime() !=
                        recoveredMessage->getConnection().getPolicies().getDtcpConfiguration().getInitialsenderinactivitytime()){
                std::cout << "ConnPolicies.dtcpconfig.initialsenderinatcitvitytime on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getPolicies().getDtcpConfiguration().getRtxcontrolconfig().getDatarxmsnmax() !=
                        recoveredMessage->getConnection().getPolicies().getDtcpConfiguration().
                        getRtxcontrolconfig().getDatarxmsnmax()){
                std::cout << "ConnPolicies.dtcpconfig.rtxctrlconfig.datarnmsnmax on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getPolicies().getInitialATimer() !=
                        recoveredMessage->getConnection().getPolicies().getInitialATimer()){
                std::cout << "ConnPolicies.initialAtimer on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getPolicies().getDtcpConfiguration().getFlowcontrolconfig().isRatebased() !=
                        recoveredMessage->getConnection().getPolicies().getDtcpConfiguration().
                        getFlowcontrolconfig().isRatebased()){
                std::cout << "ConnPolicies.dtcpconfig.flowctrlconfig.ratebased on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getPolicies().getDtcpConfiguration().getFlowcontrolconfig().isWindowbased() !=
                        recoveredMessage->getConnection().getPolicies().getDtcpConfiguration().
                        getFlowcontrolconfig().isWindowbased()){
                std::cout << "ConnPolicies.dtcpconfig.flowctrlconfig.windowbased on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getPolicies().getDtcpConfiguration().getFlowcontrolconfig().getRcvbuffersthreshold() !=
                        recoveredMessage->getConnection().getPolicies().getDtcpConfiguration().
                        getFlowcontrolconfig().getRcvbuffersthreshold()){
                std::cout << "ConnPolicies.dtcpconfig.flowctrlconfig.rcvbuffersthres on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getPolicies().getDtcpConfiguration().getFlowcontrolconfig().getRcvbytespercentthreshold() !=
                        recoveredMessage->getConnection().getPolicies().getDtcpConfiguration().
                        getFlowcontrolconfig().getRcvbytespercentthreshold()){
                std::cout << "ConnPolicies.dtcpconfig.flowctrlconfig.rcvbytespercentthres on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getPolicies().getDtcpConfiguration().getFlowcontrolconfig().getRcvbytesthreshold() !=
                        recoveredMessage->getConnection().getPolicies().getDtcpConfiguration().
                        getFlowcontrolconfig().getRcvbytesthreshold()){
                std::cout << "ConnPolicies.dtcpconfig.flowctrlconfig.rcvbytesthres on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getPolicies().getDtcpConfiguration().getFlowcontrolconfig().getSentbuffersthreshold() !=
                        recoveredMessage->getConnection().getPolicies().getDtcpConfiguration().
                        getFlowcontrolconfig().getSentbuffersthreshold()){
                std::cout << "ConnPolicies.dtcpconfig.flowctrlconfig.sentbuffersthres on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getPolicies().getDtcpConfiguration().getFlowcontrolconfig().getSentbytespercentthreshold() !=
                        recoveredMessage->getConnection().getPolicies().getDtcpConfiguration().
                        getFlowcontrolconfig().getSentbytespercentthreshold()){
                std::cout << "ConnPolicies.dtcpconfig.flowctrlconfig.sentbytespercentthres on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getPolicies().getDtcpConfiguration().getFlowcontrolconfig().getSentbytesthreshold() !=
                        recoveredMessage->getConnection().getPolicies().getDtcpConfiguration().
                        getFlowcontrolconfig().getSentbytesthreshold()){
                std::cout << "ConnPolicies.dtcpconfig.flowctrlconfig.sentbytesthres on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getPolicies().getDtcpConfiguration().getFlowcontrolconfig().getClosedwindowpolicy().getName().
                        compare(recoveredMessage->getConnection().getPolicies().getDtcpConfiguration().getFlowcontrolconfig().
                        getClosedwindowpolicy().getName()) != 0 ){
                std::cout << "ConnPolicies.dtcpconfig.flowctrlconfig.closedwindowpolicy.name on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getPolicies().getDtcpConfiguration().getFlowcontrolconfig().getClosedwindowpolicy().getVersion().compare(
                        recoveredMessage->getConnection().getPolicies().getDtcpConfiguration().getFlowcontrolconfig().
                        getClosedwindowpolicy().getVersion()) != 0){
                std::cout << "ConnPolicies.dtcpconfig.flowctrlconfig.closedwindowpolicy.version on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getPolicies().getDtcpConfiguration().getFlowcontrolconfig().
                        getRatebasedconfig().getSendingrate() != recoveredMessage->getConnection().getPolicies().
                        getDtcpConfiguration().getFlowcontrolconfig().getRatebasedconfig().getSendingrate()){
                std::cout << "ConnPolicies.dtcpconfig.flowctrlconfig.ratebasedconfig.sendingrate on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getPolicies().getDtcpConfiguration().getFlowcontrolconfig().
                        getRatebasedconfig().getTimeperiod() != recoveredMessage->getConnection().getPolicies().
                        getDtcpConfiguration().getFlowcontrolconfig().getRatebasedconfig().getTimeperiod()){
                std::cout << "ConnPolicies.dtcpconfig.flowctrlconfig.ratebasedconfig.timerperiod on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getPolicies().getDtcpConfiguration().getFlowcontrolconfig().
                        getWindowbasedconfig().getInitialcredit() != recoveredMessage->getConnection().getPolicies().
                        getDtcpConfiguration().getFlowcontrolconfig().getWindowbasedconfig().getInitialcredit()){
                std::cout << "ConnPolicies.dtcpconfig.flowctrlconfig.windowbasedconfig.initialcredit on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getPolicies().getDtcpConfiguration().getFlowcontrolconfig().
                        getWindowbasedconfig().getMaxclosedwindowqueuelength() != recoveredMessage->getConnection().getPolicies().
                        getDtcpConfiguration().getFlowcontrolconfig().getWindowbasedconfig().getMaxclosedwindowqueuelength()){
                std::cout << "ConnPolicies.dtcpconfig.flowctrlconfig.windowbasedconfig.maxclosedWindowQLength on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        }

        if (returnValue == 0) {
                std::cout << "IpcpConnectionCreateRequestMessage test ok\n";
        }
        nlmsg_free(netlinkMessage);
        delete recoveredMessage;

        return returnValue;
}

int testIpcpCreateConnectionResponse() {
        std::cout << "TESTING IPCP CREATE CONNECTION RESPONSE MESSAGE\n";
        int returnValue = 0;

        IpcpConnectionCreateResponseMessage message;
        message.setPortId(25);
        message.setCepId(14);

        struct nl_msg* netlinkMessage;
        netlinkMessage = nlmsg_alloc();
        if (!netlinkMessage) {
                std::cout << "Error allocating Netlink message\n";
        }
        genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
                        sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

        int result = putBaseNetlinkMessage(netlinkMessage, &message);
        if (result < 0) {
                std::cout << "Error constructing Ipcp Create Connection response"
                                << "message \n";
                nlmsg_free(netlinkMessage);
                return result;
        }

        nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
        IpcpConnectionCreateResponseMessage * recoveredMessage =
                        dynamic_cast<IpcpConnectionCreateResponseMessage *>(
                                        parseBaseNetlinkMessage(netlinkMessageHeader));
        if (recoveredMessage == 0) {
                std::cout << "Error parsing Ipcp Create Connection response Message "
                                << "\n";
                returnValue = -1;
        } else if (message.getPortId() != recoveredMessage->getPortId()) {
                std::cout << "Port id on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getCepId()
                        != recoveredMessage->getCepId()) {
                std::cout << "Cep id  on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        }

        if (returnValue == 0) {
                std::cout << "IpcpConnectionCreateResponseMessage test ok\n";
        }
        nlmsg_free(netlinkMessage);
        delete recoveredMessage;

        return returnValue;
}

int testIpcpUpdateConnectionRequest() {
        std::cout << "TESTING IPCP UPDATE CONNECTION REQUEST MESSAGE\n";
        int returnValue = 0;

        IpcpConnectionUpdateRequestMessage message;
        message.setPortId(25);
        message.setSourceCepId(345);
        message.setDestinationCepId(212);
        message.setFlowUserIpcProcessId(35);

        struct nl_msg* netlinkMessage;
        netlinkMessage = nlmsg_alloc();
        if (!netlinkMessage) {
                std::cout << "Error allocating Netlink message\n";
        }
        genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
                        sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

        int result = putBaseNetlinkMessage(netlinkMessage, &message);
        if (result < 0) {
                std::cout << "Error constructing Ipcp Update Connection request"
                                << "message \n";
                nlmsg_free(netlinkMessage);
                return result;
        }

        nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
        IpcpConnectionUpdateRequestMessage * recoveredMessage =
                        dynamic_cast<IpcpConnectionUpdateRequestMessage *>(
                                        parseBaseNetlinkMessage(netlinkMessageHeader));
        if (recoveredMessage == 0) {
                std::cout << "Error parsing Ipcp Update Connection request Message "
                                << "\n";
                returnValue = -1;
        } else if (message.getPortId() != recoveredMessage->getPortId()) {
                std::cout << "Port id on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getSourceCepId()
                        != recoveredMessage->getSourceCepId()) {
                std::cout << "Source CEP-id  on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getDestinationCepId() !=
                        recoveredMessage->getDestinationCepId()) {
                std::cout << "Destination CEP-id on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getFlowUserIpcProcessId() !=
                        recoveredMessage->getFlowUserIpcProcessId()) {
                std::cout << "Flow user IPC Process id on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        }

        if (returnValue == 0) {
                std::cout << "IpcpConnectionUpdateRequestMessage test ok\n";
        }
        nlmsg_free(netlinkMessage);
        delete recoveredMessage;

        return returnValue;
}

int testIpcpUpdateConnectionResult() {
        std::cout << "TESTING IPCP UPDATE CONNECTION RESULT MESSAGE\n";
        int returnValue = 0;

        IpcpConnectionUpdateResultMessage message;
        message.setPortId(25);
        message.setResult(-34);

        struct nl_msg* netlinkMessage;
        netlinkMessage = nlmsg_alloc();
        if (!netlinkMessage) {
                std::cout << "Error allocating Netlink message\n";
        }
        genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
                        sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

        int result = putBaseNetlinkMessage(netlinkMessage, &message);
        if (result < 0) {
                std::cout << "Error constructing Ipcp Update Connection result"
                                << "message \n";
                nlmsg_free(netlinkMessage);
                return result;
        }

        nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
        IpcpConnectionUpdateResultMessage * recoveredMessage =
                        dynamic_cast<IpcpConnectionUpdateResultMessage *>(
                                        parseBaseNetlinkMessage(netlinkMessageHeader));
        if (recoveredMessage == 0) {
                std::cout << "Error parsing Ipcp Update Connection result Message "
                                << "\n";
                returnValue = -1;
        } else if (message.getPortId() != recoveredMessage->getPortId()) {
                std::cout << "Port id on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getResult()
                        != recoveredMessage->getResult()) {
                std::cout << "Result  on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        }

        if (returnValue == 0) {
                std::cout << "IpcpConnectionUpdateResultMessage test ok\n";
        }
        nlmsg_free(netlinkMessage);
        delete recoveredMessage;

        return returnValue;
}

int testIpcpCreateConnectionArrived() {
        std::cout << "TESTING IPCP CREATE CONNECTION ARRIVED MESSAGE\n";
        int returnValue = 0;

        IpcpConnectionCreateArrivedMessage message;
        Connection connection;
        connection.setPortId(25);
        connection.setSourceAddress(1);
        connection.setDestAddress(2);
        connection.setQosId(3);
        connection.setDestCepId(346);
        connection.setFlowUserIpcProcessId(23);
        message.setConnection(connection);

        struct nl_msg* netlinkMessage;
        netlinkMessage = nlmsg_alloc();
        if (!netlinkMessage) {
                std::cout << "Error allocating Netlink message\n";
        }
        genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
                        sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

        int result = putBaseNetlinkMessage(netlinkMessage, &message);
        if (result < 0) {
                std::cout << "Error constructing Ipcp Create Connection arrived"
                                << "message \n";
                nlmsg_free(netlinkMessage);
                return result;
        }

        nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
        IpcpConnectionCreateArrivedMessage * recoveredMessage =
                        dynamic_cast<IpcpConnectionCreateArrivedMessage *>(
                                        parseBaseNetlinkMessage(netlinkMessageHeader));
        if (recoveredMessage == 0) {
                std::cout << "Error parsing Ipcp Create Connection arrived Message "
                                << "\n";
                returnValue = -1;
        } else if (message.getConnection().getPortId() !=
                        recoveredMessage->getConnection().getPortId()) {
                std::cout << "Port id on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getSourceAddress()
                        != recoveredMessage->getConnection().getSourceAddress()) {
                std::cout << "Source address  on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getDestAddress() !=
                        recoveredMessage->getConnection().getDestAddress()) {
                std::cout << "Destination address on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getQosId() !=
                        recoveredMessage->getConnection().getQosId()) {
                std::cout << "QoS id on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getDestCepId() !=
                        recoveredMessage->getConnection().getDestCepId()) {
                std::cout << "Dest cep-id on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getConnection().getFlowUserIpcProcessId() !=
                        recoveredMessage->getConnection().getFlowUserIpcProcessId()) {
                std::cout << "Flow user IPC Process id on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        }

        if (returnValue == 0) {
                std::cout << "IpcpConnectionCreateArrivedMessage test ok\n";
        }
        nlmsg_free(netlinkMessage);
        delete recoveredMessage;

        return returnValue;
}

int testIpcpCreateConnectionResult() {
        std::cout << "TESTING IPCP CREATE CONNECTION RESULT MESSAGE\n";
        int returnValue = 0;

        IpcpConnectionCreateResultMessage message;
        message.setPortId(25);
        message.setSourceCepId(14);
        message.setDestCepId(234);

        struct nl_msg* netlinkMessage;
        netlinkMessage = nlmsg_alloc();
        if (!netlinkMessage) {
                std::cout << "Error allocating Netlink message\n";
        }
        genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
                        sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

        int result = putBaseNetlinkMessage(netlinkMessage, &message);
        if (result < 0) {
                std::cout << "Error constructing Ipcp Create Connection result"
                                << "message \n";
                nlmsg_free(netlinkMessage);
                return result;
        }

        nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
        IpcpConnectionCreateResultMessage * recoveredMessage =
                        dynamic_cast<IpcpConnectionCreateResultMessage *>(
                                        parseBaseNetlinkMessage(netlinkMessageHeader));
        if (recoveredMessage == 0) {
                std::cout << "Error parsing Ipcp Create Connection response result "
                                << "\n";
                returnValue = -1;
        } else if (message.getPortId() != recoveredMessage->getPortId()) {
                std::cout << "Port id on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getSourceCepId()
                        != recoveredMessage->getSourceCepId()) {
                std::cout << "Source cep id  on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getDestCepId()
                        != recoveredMessage->getDestCepId()) {
                std::cout << "Dest cep id  on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        }

        if (returnValue == 0) {
                std::cout << "IpcpConnectionCreateResultMessage test ok\n";
        }
        nlmsg_free(netlinkMessage);
        delete recoveredMessage;

        return returnValue;
}

int testIpcpDestroyConnectionRequest() {
        std::cout << "TESTING IPCP DESTROY CONNECTION REQUEST MESSAGE\n";
        int returnValue = 0;

        IpcpConnectionDestroyRequestMessage message;
        message.setPortId(25);
        message.setCepId(234);

        struct nl_msg* netlinkMessage;
        netlinkMessage = nlmsg_alloc();
        if (!netlinkMessage) {
                std::cout << "Error allocating Netlink message\n";
        }
        genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
                        sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

        int result = putBaseNetlinkMessage(netlinkMessage, &message);
        if (result < 0) {
                std::cout << "Error constructing Ipcp Destroy connection request"
                                << "message \n";
                nlmsg_free(netlinkMessage);
                return result;
        }

        nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
        IpcpConnectionDestroyRequestMessage * recoveredMessage =
                        dynamic_cast<IpcpConnectionDestroyRequestMessage *>(
                                        parseBaseNetlinkMessage(netlinkMessageHeader));
        if (recoveredMessage == 0) {
                std::cout << "Error parsing Ipcp Destroy Connection request ,essage "
                                << "\n";
                returnValue = -1;
        } else if (message.getPortId() != recoveredMessage->getPortId()) {
                std::cout << "Port id on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getCepId()
                        != recoveredMessage->getCepId()) {
                std::cout << "CEP id  on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        }

        if (returnValue == 0) {
                std::cout << "IpcpConnectionDestroyRequestMessage test ok\n";
        }
        nlmsg_free(netlinkMessage);
        delete recoveredMessage;

        return returnValue;
}

int testIpcpDestroyConnectionResult() {
        std::cout << "TESTING IPCP DESTROY CONNECTION RESULT MESSAGE\n";
        int returnValue = 0;

        IpcpConnectionDestroyResultMessage message;
        message.setPortId(25);
        message.setResult(234);

        struct nl_msg* netlinkMessage;
        netlinkMessage = nlmsg_alloc();
        if (!netlinkMessage) {
                std::cout << "Error allocating Netlink message\n";
        }
        genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
                        sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

        int result = putBaseNetlinkMessage(netlinkMessage, &message);
        if (result < 0) {
                std::cout << "Error constructing Ipcp Destroy connection result"
                                << "message \n";
                nlmsg_free(netlinkMessage);
                return result;
        }

        nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
        IpcpConnectionDestroyResultMessage * recoveredMessage =
                        dynamic_cast<IpcpConnectionDestroyResultMessage *>(
                                        parseBaseNetlinkMessage(netlinkMessageHeader));
        if (recoveredMessage == 0) {
                std::cout << "Error parsing Ipcp Destroy Connection result message "
                                << "\n";
                returnValue = -1;
        } else if (message.getPortId() != recoveredMessage->getPortId()) {
                std::cout << "Port id on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getResult()
                        != recoveredMessage->getResult()) {
                std::cout << "Result on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        }

        if (returnValue == 0) {
                std::cout << "IpcpConnectionDestroyResultMessage test ok\n";
        }
        nlmsg_free(netlinkMessage);
        delete recoveredMessage;

        return returnValue;
}

int testRmtModifyPDUFTEntriesRequestMessage() {
        std::cout << "TESTING RMT MODIFY PDU FTE REQUEST MESSAGE\n";
        int returnValue = 0;
        std::list<PDUForwardingTableEntry>::const_iterator iterator;
        std::list<PDUForwardingTableEntry> entriesList;
        std::list<unsigned int>::const_iterator iterator2;
        std::list<unsigned int> portIdsList;

        RmtModifyPDUFTEntriesRequestMessage message;
        PDUForwardingTableEntry entry1, entry2;
        entry1.setAddress(23);
        entry1.addPortId(34);
        entry1.addPortId(24);
        entry1.addPortId(39);
        entry1.setQosId(1);
        message.addEntry(entry1);
        entry2.setAddress(20);
        entry2.addPortId(28);
        entry2.addPortId(35);
        entry2.addPortId(54);
        entry2.setQosId(2);
        message.addEntry(entry2);
        message.setMode(1);

        struct nl_msg* netlinkMessage;
        netlinkMessage = nlmsg_alloc();
        if (!netlinkMessage) {
                std::cout << "Error allocating Netlink message\n";
        }
        genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
                        sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

        int result = putBaseNetlinkMessage(netlinkMessage, &message);
        if (result < 0) {
                std::cout << "Error constructing RmtModifyPDUFTEntriesRequest "
                                << "message \n";
                nlmsg_free(netlinkMessage);
                return result;
        }

        nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
        RmtModifyPDUFTEntriesRequestMessage * recoveredMessage =
                        dynamic_cast<RmtModifyPDUFTEntriesRequestMessage *>(
                                        parseBaseNetlinkMessage(netlinkMessageHeader));
        if (recoveredMessage == 0) {
                std::cout << "Error parsing RmtModifyPDUFTEntriesRequest message "
                                << "\n";
                returnValue = -1;
        } else if (message.getMode() != recoveredMessage->getMode()) {
                std::cout << "Mode on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getEntries().size()
                        != recoveredMessage->getEntries().size()) {
                std::cout << "Size of entries in original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        }


        entriesList = recoveredMessage->getEntries();
        for (iterator = entriesList.begin();
                        iterator != entriesList.end();
                        ++iterator) {
                portIdsList = iterator->getPortIds();
                if (portIdsList.size() != 3) {
                        std::cout << "Size of portids in original and recovered messages"
                                        << " are different\n";
                        returnValue = -1;
                }

                for(iterator2 = portIdsList.begin();
                                iterator2 != portIdsList.end();
                                ++iterator2) {
                        std::cout << *iterator2 <<std::endl;
                }
        }

        if (returnValue == 0) {
                std::cout << "RmtModifyPDUFTEntriesRequestMessage test ok\n";
        }
        nlmsg_free(netlinkMessage);
        delete recoveredMessage;

        return returnValue;
}

int testRmtDumpPDUFTResponseMessage() {
        std::cout << "TESTING RMT DUMP PDU FWDING TABLE RESPONSE MESSAGE\n";
        int returnValue = 0;
        std::list<PDUForwardingTableEntry>::const_iterator iterator;
        std::list<PDUForwardingTableEntry> entriesList;
        std::list<unsigned int>::const_iterator iterator2;
        std::list<unsigned int> portIdsList;

        RmtDumpPDUFTEntriesResponseMessage message;
        PDUForwardingTableEntry entry1, entry2;
        entry1.setAddress(23);
        entry1.addPortId(34);
        entry1.addPortId(24);
        entry1.addPortId(39);
        entry1.setQosId(1);
        message.addEntry(entry1);
        entry2.setAddress(20);
        entry2.addPortId(28);
        entry2.addPortId(35);
        entry2.addPortId(54);
        entry2.setQosId(2);
        message.addEntry(entry2);
        message.setResult(3);

        struct nl_msg* netlinkMessage;
        netlinkMessage = nlmsg_alloc();
        if (!netlinkMessage) {
                std::cout << "Error allocating Netlink message\n";
        }
        genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
                        sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

        int result = putBaseNetlinkMessage(netlinkMessage, &message);
        if (result < 0) {
                std::cout << "Error constructing RmtDumpPDUFTEntriesResponse "
                                << "message \n";
                nlmsg_free(netlinkMessage);
                return result;
        }

        nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
        RmtDumpPDUFTEntriesResponseMessage * recoveredMessage =
                        dynamic_cast<RmtDumpPDUFTEntriesResponseMessage *>(
                                        parseBaseNetlinkMessage(netlinkMessageHeader));
        if (recoveredMessage == 0) {
                std::cout << "Error parsing RmtDumpPDUFTEntriesResponseMessage message "
                                << "\n";
                returnValue = -1;
        } else if (message.getResult() != recoveredMessage->getResult()) {
                std::cout << "Mode on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getEntries().size()
                        != recoveredMessage->getEntries().size()) {
                std::cout << "Size of entries in original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        }


        entriesList = recoveredMessage->getEntries();
        for (iterator = entriesList.begin();
                        iterator != entriesList.end();
                        ++iterator) {
                portIdsList = iterator->getPortIds();
                if (portIdsList.size() != 3) {
                        std::cout << "Size of portids in original and recovered messages"
                                        << " are different\n";
                        returnValue = -1;
                }

                for(iterator2 = portIdsList.begin();
                                iterator2 != portIdsList.end();
                                ++iterator2) {
                        std::cout << *iterator2 <<std::endl;
                }
        }

        if (returnValue == 0) {
                std::cout << "RmtDumpPDUFTEntriesResponseMessage test ok\n";
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

	result = testIpcmUpdateDIFConfigurationRequestMessage();
	if (result < 0) {
	        return result;
	}

	result = testIpcmUpdateDIFConfigurationResponseMessage();
	if (result < 0) {
	        return result;
	}

	result = testIpcmEnrollToDIFRequestMessage();
	if (result < 0) {
	        return result;
	}

	result = testIpcmNeighborsModifiedNotificaiton();
	if (result < 0) {
	        return result;
	}

	result = testIpcmEnrollToDIFResponseMessage();
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

	result = testIpcpCreateConnectionRequest();
	if (result < 0) {
	        return result;
	}

	result = testIpcpCreateConnectionResponse();
	if (result < 0) {
	        return result;
	}

	result = testIpcpUpdateConnectionRequest();
	if (result < 0) {
	        return result;
	}

	result = testIpcpUpdateConnectionResult();
	if (result < 0) {
	        return result;
	}

	result = testIpcpCreateConnectionArrived();
	if (result < 0) {
	        return result;
	}

	result = testIpcpCreateConnectionResult();
	if (result < 0) {
	        return result;
	}

	result = testIpcpDestroyConnectionRequest();
	if (result < 0) {
	        return result;
	}

	result = testIpcpDestroyConnectionResult();
	if (result < 0) {
	        return result;
	}

	result = testRmtModifyPDUFTEntriesRequestMessage();
	if (result < 0) {
	        return result;
	}

	result = testRmtDumpPDUFTResponseMessage();
	if (result < 0) {
	        return result;
	}

	return 0;
}
