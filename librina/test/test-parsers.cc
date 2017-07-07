//
// Test parsers
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

#include "irati/serdes-utils.h"
#include "irati/kernel-msg.h"
#include "core.h"
#include "ctrl.h"

using namespace rina;

int test_irati_kmsg_ipcm_allocate_flow(irati_msg_t msg_t)
{
	struct irati_kmsg_ipcm_allocate_flow * msg, * resp;
	int ret = 0;
	char serbuf[8192];
	unsigned int serlen;
	unsigned int expected_serlen;
	ApplicationProcessNamingInformation before;
	ApplicationProcessNamingInformation after;

	std::cout << "TESTING KMSG IPCM ALOCATE FLOW (" << msg_t
		   << ")" << std::endl;

	msg = new irati_kmsg_ipcm_allocate_flow();
	msg->msg_type = msg_t;
	msg->port_id = 25;
	msg->source = new name();
	msg->source->process_name = stringToCharArray("/apps/source");
	msg->source->process_instance = stringToCharArray("12");
	msg->source->entity_name = stringToCharArray("database");
	msg->source->entity_instance = stringToCharArray("12");

	msg->dest = new name();
	msg->dest->process_name = stringToCharArray("/apps/dest");
	msg->dest->process_instance = stringToCharArray("12345");
	msg->dest->entity_name = stringToCharArray("printer");
	msg->dest->entity_instance = stringToCharArray("12623456");

	msg->fspec = new flow_spec();
	msg->dif_name = new name();
	msg->dif_name->process_name = stringToCharArray("test.DIF");

	expected_serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX,
			     	     	   (irati_msg_base *) msg);
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, (irati_msg_base *) msg);

	if (serlen <= 0) {
		std::cout << "Error serializing irati_kmsg_ipcm_allocate_flow message: "
			  << serlen << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (serlen != expected_serlen) {
		std::cout << "Expected (" << expected_serlen << ") and actual ("
			  << serlen <<") message sizes are different" << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	std::cout << "Serialized message size: " << serlen << std::endl;

	resp =  (struct irati_kmsg_ipcm_allocate_flow *) deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				    	    	    	    	    	       serbuf, serlen);
	if (!resp) {
		std::cout << "Error parsing irati_kmsg_ipcm_allocate_flow message: "
			  << ret << std::endl;
		irati_ctrl_msg_free((irati_msg_base *) msg);
		return -1;
	}

	if (msg->port_id != resp->port_id) {
		std::cout << "Port-id on original and recovered messages"
			   << " are different\n";
		irati_ctrl_msg_free((irati_msg_base *) msg);
		irati_ctrl_msg_free((irati_msg_base *) resp);
		return -1;
	}

	before = ApplicationProcessNamingInformation(msg->source);
	after = ApplicationProcessNamingInformation(resp->source);

	if (before != after) {
		std::cout << "Source application name on original and recovered messages"
			   << " are different\n";
		irati_ctrl_msg_free((irati_msg_base *) msg);
		irati_ctrl_msg_free((irati_msg_base *) resp);
		return -1;
	}

	before = ApplicationProcessNamingInformation(msg->dest);
	after = ApplicationProcessNamingInformation(((irati_kmsg_ipcm_allocate_flow*) resp)->dest);

	if (before != after) {
		std::cout << "Destination application name on original and recovered "
				<< "messages are different\n";
		irati_ctrl_msg_free((irati_msg_base *) msg);
		irati_ctrl_msg_free((irati_msg_base *) resp);
		return -1;
	}

	before = ApplicationProcessNamingInformation(msg->dif_name);
	after = ApplicationProcessNamingInformation(((irati_kmsg_ipcm_allocate_flow*) resp)->dif_name);

	if (before != after) {
		std::cout << "DIF name on original and recovered "
				<< "messages are different\n";
		irati_ctrl_msg_free((irati_msg_base *) msg);
		irati_ctrl_msg_free((irati_msg_base *) resp);
		return -1;
	}

	std::cout << "Test ok!" << std::endl;
	irati_ctrl_msg_free((irati_msg_base *) msg);
	irati_ctrl_msg_free((irati_msg_base *) resp);

	return 0;
}
/*
int testAppAllocateFlowRequestResultMessage() {
	std::cout << "TESTING APP ALLOCATE FLOW REQUEST RESULT MESSAGE\n";
	int returnValue = 0;

	ApplicationProcessNamingInformation * sourceName =
			new ApplicationProcessNamingInformation();
	sourceName->processName = "/apps/source";
	sourceName->processInstance = "12";
	sourceName->entityName = "database";
	sourceName->entityInstance = "12";

	ApplicationProcessNamingInformation * difName =
			new ApplicationProcessNamingInformation();
	difName->processName = "/difs/Test.DIF";

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
	sourceName->processName = "/apps/source";
	sourceName->processInstance = "12";
	sourceName->entityName = "database";
	sourceName->entityInstance = "12";

	ApplicationProcessNamingInformation * destName =
			new ApplicationProcessNamingInformation();
	destName->processName = "/apps/dest";
	destName->processInstance = "12345";
	destName->entityName = "printer";
	destName->entityInstance = "12623456";

	ApplicationProcessNamingInformation * difName =
			new ApplicationProcessNamingInformation();
	difName->processName = "/difs/Test1.DIF";

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
	message.result = 0;
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

	} else if (message.result != recoveredMessage->result) {
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
	applicationName.processName = "/apps/source";
	applicationName.processInstance = "15";
	applicationName.entityName = "database";
	applicationName.entityInstance = "13";

	AppDeallocateFlowRequestMessage message;;
	message.port_id = 47;
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
	} else if (message.port_id != recoveredMessage->port_id) {
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
	applicationName.processName = "/apps/source";
	applicationName.processInstance = "15";
	applicationName.entityName = "database";
	applicationName.entityInstance = "13";

	AppDeallocateFlowResponseMessage message;
	message.result = 0;
	message.port_id = 234;
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
	} else if (message.result != recoveredMessage->result) {
		std::cout << "Result on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.port_id != recoveredMessage->port_id) {
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
	applicationName.processName = "/apps/source";
	applicationName.processInstance = "15";
	applicationName.entityName = "database";
	applicationName.entityInstance = "13";

	AppFlowDeallocatedNotificationMessage message;;
	message.port_id = 47;
	message.result = 7;
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
	} else if (message.port_id != recoveredMessage->port_id) {
		std::cout << "PortId on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.result != recoveredMessage->result) {
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
	applicationName->processName = "/apps/source";
	applicationName->processInstance = "15";
	applicationName->entityName = "database";
	applicationName->entityInstance = "13";

	ApplicationProcessNamingInformation * difName =
			new ApplicationProcessNamingInformation();
	difName->processName = "/difs/Test2.DIF";

	AppRegisterApplicationRequestMessage * message =
			new AppRegisterApplicationRequestMessage();

	ApplicationRegistrationInformation appRegInfo =
		ApplicationRegistrationInformation(
				APPLICATION_REGISTRATION_SINGLE_DIF);
	appRegInfo.difName = *difName;
	appRegInfo.appName = *applicationName;

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
	} else if (message->getApplicationRegistrationInformation().appName
			!= recoveredMessage->getApplicationRegistrationInformation().
			appName) {
		std::cout << "Application name on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message->getApplicationRegistrationInformation().
			applicationRegistrationType !=
			recoveredMessage->getApplicationRegistrationInformation().
			applicationRegistrationType) {
		std::cout << "Application Registration Type on original and recovered "
				<< "messages are different\n";
		returnValue = -1;
	}  else if (message->getApplicationRegistrationInformation().difName
			!= recoveredMessage->getApplicationRegistrationInformation().
			difName) {
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
	applicationName->processName = "/apps/source";
	applicationName->processInstance = "15";
	applicationName->entityName = "database";
	applicationName->entityInstance = "13";

	ApplicationProcessNamingInformation * difName =
			new ApplicationProcessNamingInformation();
	difName->processName = "/difs/Test.DIF";

	AppRegisterApplicationResponseMessage * message =
			new AppRegisterApplicationResponseMessage();

	message->result = 1;
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
	} else if (message->result != recoveredMessage->result) {
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
	applicationName->processName = "/apps/source";
	applicationName->processInstance = "15";
	applicationName->entityName = "database";
	applicationName->entityInstance = "13";

	ApplicationProcessNamingInformation * difName =
			new ApplicationProcessNamingInformation();
	difName->processName = "/difs/Test2.DIF";

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
	applicationName->processName = "/apps/source";
	applicationName->processInstance = "15";
	applicationName->entityName = "database";
	applicationName->entityInstance = "13";

	AppUnregisterApplicationResponseMessage * message =
			new AppUnregisterApplicationResponseMessage();

	message->result = 1;
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
	} else if (message->result != recoveredMessage->result) {
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
	applicationName.processName = "/apps/source";
	applicationName.processInstance = "15";
	applicationName.entityName = "database";
	applicationName.entityInstance = "13";

	ApplicationProcessNamingInformation difName;
	difName.processName = "/difs/Test2.DIF";

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
	message.result = 0;
	rib::RIBObjectData * ribObject = new rib::RIBObjectData();
	ribObject->set_class("/test/clazz1");
	ribObject->set_name("/test/name1");
	ribObject->set_instance(1234);
	ribObject->set_displayable_value("This is my value");
	message.addRIBObject(*ribObject);
	delete ribObject;

	ribObject = new rib::RIBObjectData();
	ribObject->set_class("/test/clazz2");
	ribObject->set_name("/test/name2");
	ribObject->set_instance(343241);
	ribObject->set_displayable_value("This is my value2");
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
	} else if (message.result != recoveredMessage->result) {
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
		std::list<rib::RIBObjectData>::const_iterator iterator;
		int i = 0;
		for (iterator = recoveredMessage->getRIBObjects().begin();
				iterator != recoveredMessage->getRIBObjects().end();
				++iterator) {
			const rib::RIBObjectData& ribObject = *iterator;
			if (i == 0){
				if (ribObject.get_class().compare("/test/clazz1") != 0){
					std::cout << "RIB Object clazz on original and recovered messages"
							<< " are different\n";
					returnValue = -1;
					break;
				}else if (ribObject.get_name().compare("/test/name1") != 0){
					std::cout << "RIB Object name on original and recovered messages"
							<< " are different\n";
					returnValue = -1;
					break;
				}else if (ribObject.get_instance() != 1234){
					std::cout << "RIB Object instance on original and recovered messages"
							<< " are different\n";
					returnValue = -1;
					break;
				}else if (ribObject.get_displayable_value().compare("This is my value") != 0){
                                        std::cout << "RIB Object display value on original and recovered messages"
                                                        << " are different\n";
                                        returnValue = -1;
                                        break;
                                }

				i++;
			}else if (i == 1){
				if (ribObject.get_class().compare("/test/clazz2") != 0){
					std::cout << "RIB Object clazz on original and recovered messages"
							<< " are different " <<std::endl;
					returnValue = -1;
					break;
				} else if (ribObject.get_name().compare("/test/name2") != 0){
					std::cout << "RIB Object name on original and recovered messages"
							<< " are different\n";
					returnValue = -1;
					break;
				} else if (ribObject.get_instance() != 343241){
					std::cout << "RIB Object instance on original and recovered messages"
							<< " are different\n";
					returnValue = -1;
					break;
				} else if (ribObject.get_displayable_value().compare("This is my value2") != 0){
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

	ApplicationProcessNamingInformation applicationName;
	applicationName.processName = "/apps/source";
	applicationName.processInstance = "15";
	applicationName.entityName = "database";
	applicationName.entityInstance = "13";

	ApplicationProcessNamingInformation dafName;
	dafName.processName = "/dafs/Test12.DAF";

	ApplicationProcessNamingInformation difName;
	difName.processName = "/difs/Test2.DIF";

	IpcmRegisterApplicationRequestMessage * message =
			new IpcmRegisterApplicationRequestMessage();

	message->setDifName(difName);
	message->setApplicationName(applicationName);
	message->dafName = dafName;
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
	} else if (message->dafName
			!= recoveredMessage->dafName) {
		std::cout << "DAF name on original and recovered messages"
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
	delete message;
	delete recoveredMessage;

	return returnValue;
}

int testBaseNetlinkMessage() {
	std::cout << "TESTING BASE NETLINK MESSAGE\n";
	int returnValue = 0;

	BaseNetlinkMessage message(RINA_C_IPCM_SELECT_POLICY_SET_RESPONSE);
	message.result = 23;
	message.port_id = 3437;
	message.cep_id = 215;

	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage) {
		std::cout << "Error allocating Netlink message\n";
	}
	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
			sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

	int result = putBaseNetlinkMessage(netlinkMessage, &message);
	if (result < 0) {
		std::cout << "Error constructing Base Netlink Response "
				<< "Message \n";
		nlmsg_free(netlinkMessage);
		return result;
	}

	nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
	BaseNetlinkMessage * recoveredMessage =
			dynamic_cast<BaseNetlinkMessage *>(parseBaseNetlinkMessage(
					netlinkMessageHeader));
	if (recoveredMessage == NULL) {
		std::cout << "Error parsing Base Netlink Message "
				<< "\n";
		returnValue = -1;
	} else if (message.result != recoveredMessage->result) {
		std::cout << "Result on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.port_id != recoveredMessage->port_id) {
		std::cout << "Port_id on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.cep_id != recoveredMessage->cep_id) {
		std::cout << "Cep_id on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	}

	if (returnValue == 0) {
		std::cout << "BaseNetlinkMessage test ok\n";
	}
	nlmsg_free(netlinkMessage);
	delete recoveredMessage;

	return returnValue;
}

int testIpcmUnregisterApplicationRequestMessage() {
	std::cout << "TESTING IPCM UNREGISTER APPLICATION REQUEST MESSAGE\n";
	int returnValue = 0;

	ApplicationProcessNamingInformation applicationName;
	applicationName.processName = "/apps/source";
	applicationName.processInstance = "15";
	applicationName.entityName = "database";
	applicationName.entityInstance = "13";

	ApplicationProcessNamingInformation difName;
	difName.processName = "/difs/Test2.DIF";

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

int testIpcmAssignToDIFRequestMessage() {
	std::cout << "TESTING IPCM ASSIGN TO DIF REQUEST MESSAGE\n";
	int returnValue = 0;

	ApplicationProcessNamingInformation  difName;
	difName.processName = "/difs/Test.DIF";

	IpcmAssignToDIFRequestMessage message;
	DIFInformation difInformation;
	DIFConfiguration difConfiguration;
	difInformation.set_dif_type(NORMAL_IPC_PROCESS);
	difInformation.set_dif_name(difName);
	PolicyParameter * parameter = new PolicyParameter("interface", "eth0");
	difConfiguration.add_parameter(*parameter);
	delete parameter;
	parameter = new PolicyParameter("vlanid", "430");
	difConfiguration.add_parameter(*parameter);
	difConfiguration.set_address(34);
	delete parameter;
	EFCPConfiguration efcpConfiguration;
	DataTransferConstants dataTransferConstants;
	dataTransferConstants.set_address_length(1);
	dataTransferConstants.set_cep_id_length(2);
	dataTransferConstants.set_dif_integrity(true);
	dataTransferConstants.set_length_length(3);
	dataTransferConstants.set_rate_length(4);
	dataTransferConstants.set_frame_length(4);
	dataTransferConstants.set_max_pdu_lifetime(4);
	dataTransferConstants.set_max_pdu_size(5);
	dataTransferConstants.set_port_id_length(6);
	dataTransferConstants.set_qos_id_length(7);
	dataTransferConstants.set_sequence_number_length(8);
	dataTransferConstants.set_ctrl_sequence_number_length(4);
	efcpConfiguration.set_data_transfer_constants(dataTransferConstants);
	QoSCube * qosCube = new QoSCube("cube 1", 1);
	efcpConfiguration.add_qos_cube(qosCube);
	qosCube = new QoSCube("cube 2", 2);
	efcpConfiguration.add_qos_cube(qosCube);
	qosCube->set_dtp_config(DTPConfig());
	qosCube->set_dtcp_config(DTCPConfig());
	difConfiguration.set_efcp_configuration(efcpConfiguration);
	difConfiguration.rmt_configuration_.policy_set_.name_ = "FancySchedulingPolicy";
	difConfiguration.rmt_configuration_.pft_conf_.policy_set_.name_ = "PFTPolicySet";
	difConfiguration.ra_configuration_.pduftg_conf_.policy_set_.name_ = "PDUFTGPolicySet";
	difConfiguration.routing_configuration_.policy_set_.name_ = "RoutingPS";
	difConfiguration.nsm_configuration_.policy_set_.name_ = "NSMPS";
	difConfiguration.fa_configuration_.policy_set_.name_ = "FAPS";
	difConfiguration.et_configuration_.policy_set_.name_ = "ETPS";
	difInformation.set_dif_configuration(difConfiguration);
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
	} else if (message.getDIFInformation().get_dif_type().compare(
			recoveredMessage->getDIFInformation().get_dif_type()) != 0) {
		std::cout << "DIFInformation.difType on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.getDIFInformation().get_dif_name() !=
			recoveredMessage->getDIFInformation().get_dif_name()) {
		std::cout << "DIFInformation.difName on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.getDIFInformation().get_dif_configuration().get_parameters().size() !=
			recoveredMessage->getDIFInformation().get_dif_configuration().get_parameters().size()){
		std::cout << "DIFInformation.DIFConfiguration.parameters.size on original and recovered messages"
				<< " are different\n";
		returnValue = -1;
	} else if (message.getDIFInformation().get_dif_configuration().get_efcp_configuration().
			get_data_transfer_constants().get_address_length() !=
	                                recoveredMessage->getDIFInformation().get_dif_configuration().
	                                get_efcp_configuration().get_data_transfer_constants().get_address_length()) {
	        std::cout << "DIFInformation.DIFConfiguration.dtc.addrLength on original and recovered messages"
	                        << " are different\n";
	        returnValue = -1;
	} else if (message.getDIFInformation().get_dif_configuration().get_efcp_configuration().
			get_data_transfer_constants().get_cep_id_length() !=
                                        recoveredMessage->getDIFInformation().get_dif_configuration().
                                        get_efcp_configuration().get_data_transfer_constants().get_cep_id_length()) {
                std::cout << "DIFInformation.DIFConfiguration.dtc.cepIdLength on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getDIFInformation().get_dif_configuration().get_efcp_configuration().
        		get_data_transfer_constants().get_length_length() !=
                                        recoveredMessage->getDIFInformation().get_dif_configuration().
                                        get_efcp_configuration().get_data_transfer_constants().get_length_length()) {
                std::cout << "DIFInformation.DIFConfiguration.dtc.lengthLength on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getDIFInformation().get_dif_configuration().get_efcp_configuration().
        		get_data_transfer_constants().get_rate_length() !=
                                        recoveredMessage->getDIFInformation().get_dif_configuration().
                                        get_efcp_configuration().get_data_transfer_constants().get_rate_length()) {
                std::cout << "DIFInformation.DIFConfiguration.dtc.rateLength on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getDIFInformation().get_dif_configuration().get_efcp_configuration().
        		get_data_transfer_constants().get_frame_length() !=
                                        recoveredMessage->getDIFInformation().get_dif_configuration().
                                        get_efcp_configuration().get_data_transfer_constants().get_frame_length()) {
                std::cout << "DIFInformation.DIFConfiguration.dtc.frameLength on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getDIFInformation().get_dif_configuration().get_efcp_configuration().
        		get_data_transfer_constants().get_max_pdu_lifetime() !=
                                        recoveredMessage->getDIFInformation().get_dif_configuration().
                                        get_efcp_configuration().get_data_transfer_constants().get_max_pdu_lifetime()) {
                std::cout << "DIFInformation.DIFConfiguration.dtc.maxPDULifetime on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getDIFInformation().get_dif_configuration().get_efcp_configuration().
        		get_data_transfer_constants().get_max_pdu_size() !=
                                        recoveredMessage->getDIFInformation().get_dif_configuration().
                                        get_efcp_configuration().get_data_transfer_constants().get_max_pdu_size()) {
                std::cout << "DIFInformation.DIFConfiguration.dtc.maxPDUSize on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getDIFInformation().get_dif_configuration().get_efcp_configuration().
                        get_data_transfer_constants().get_port_id_length() !=
                                        recoveredMessage->getDIFInformation().get_dif_configuration().
                                        get_efcp_configuration().get_data_transfer_constants().get_port_id_length()) {
                std::cout << "DIFInformation.DIFConfiguration.dtc.portIdLength on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getDIFInformation().get_dif_configuration().get_efcp_configuration().
                        get_data_transfer_constants().get_qos_id_length() !=
                                        recoveredMessage->getDIFInformation().get_dif_configuration().
                                        get_efcp_configuration().get_data_transfer_constants().get_qos_id_length()) {
                std::cout << "DIFInformation.DIFConfiguration.dtc.qosIdLength on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getDIFInformation().get_dif_configuration().get_efcp_configuration().
        		get_data_transfer_constants().get_sequence_number_length() !=
                                        recoveredMessage->getDIFInformation().get_dif_configuration().
                                        get_efcp_configuration().get_data_transfer_constants().get_sequence_number_length()) {
                std::cout << "DIFInformation.DIFConfiguration.dtc.seqNumLength on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getDIFInformation().get_dif_configuration().get_efcp_configuration().
        		get_data_transfer_constants().get_ctrl_sequence_number_length() !=
                                        recoveredMessage->getDIFInformation().get_dif_configuration().
                                        get_efcp_configuration().get_data_transfer_constants().get_ctrl_sequence_number_length()) {
                std::cout << "DIFInformation.DIFConfiguration.dtc.ctrlSeqNumLength on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getDIFInformation().get_dif_configuration().get_efcp_configuration().
        		get_data_transfer_constants().is_dif_integrity() !=
                                        recoveredMessage->getDIFInformation().get_dif_configuration().
                                        get_efcp_configuration().get_data_transfer_constants().is_dif_integrity()) {
                std::cout << "DIFInformation.DIFConfiguration.dtc.difIntegrity on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getDIFInformation().get_dif_configuration().get_address() !=
                                        recoveredMessage->getDIFInformation().get_dif_configuration().
                                        get_address()) {
                std::cout << "DIFInformation.DIFConfiguration.address original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getDIFInformation().get_dif_configuration().get_efcp_configuration().get_qos_cubes().size() !=
                        recoveredMessage->getDIFInformation().get_dif_configuration().
                        get_efcp_configuration().get_qos_cubes().size()) {
                std::cout << "DIFInformation.DIFConfiguration.qosCubes.size original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.getDIFInformation().dif_configuration_.rmt_configuration_.policy_set_.name_.
        		compare(recoveredMessage->getDIFInformation().dif_configuration_.rmt_configuration_.
        				policy_set_.name_) != 0) {
        	std::cout << "DIFInformation.dif_configuration_.rmt_configuration_.policy_set_.name original and recovered messages"
        			<< " are different\n";
        	returnValue = -1;
        } else if (message.getDIFInformation().dif_configuration_.rmt_configuration_.pft_conf_.policy_set_.name_.
        		compare(recoveredMessage->getDIFInformation().dif_configuration_.rmt_configuration_.
        				pft_conf_.policy_set_.name_) != 0) {
        	std::cout << "DIFInformation.dif_configuration_.rmt_configuration_.pft_conf_.policy_set_.name_. original and recovered messages"
        			<< " are different\n";
        	returnValue = -1;
        } else if (message.getDIFInformation().dif_configuration_.ra_configuration_.pduftg_conf_.policy_set_.name_.
        		compare(recoveredMessage->getDIFInformation().dif_configuration_.ra_configuration_.
        				pduftg_conf_.policy_set_.name_) != 0) {
        	std::cout << "DIFInformation.dif_configuration_.ra_configuration_.pduftg_conf_.policy_set_.name_. original and recovered messages"
        			<< " are different\n";
        	returnValue = -1;
        } else if (message.getDIFInformation().dif_configuration_.routing_configuration_.policy_set_.name_.
        		compare(recoveredMessage->getDIFInformation().dif_configuration_.routing_configuration_.
        				policy_set_.name_) != 0) {
        	std::cout << "DIFInformation.dif_configuration_.routing_configuration_.policy_set_.name_. original and recovered messages"
        			<< " are different\n";
        	returnValue = -1;
        } else if (message.getDIFInformation().dif_configuration_.nsm_configuration_.policy_set_.name_.
        		compare(recoveredMessage->getDIFInformation().dif_configuration_.nsm_configuration_.
        				policy_set_.name_) != 0) {
        	std::cout << "DIFInformation.dif_configuration_.nsm_configuration_.policy_set_.name_. original and recovered messages"
        			<< " are different\n";
        	returnValue = -1;
        } else if (message.getDIFInformation().dif_configuration_.fa_configuration_.policy_set_.name_.
        		compare(recoveredMessage->getDIFInformation().dif_configuration_.fa_configuration_.
        				policy_set_.name_) != 0) {
        	std::cout << "DIFInformation.dif_configuration_.fa_configuration_.policy_set_.name_. original and recovered messages"
        			<< " are different\n";
        	returnValue = -1;
        } else if (message.getDIFInformation().dif_configuration_.et_configuration_.policy_set_.name_.
        		compare(recoveredMessage->getDIFInformation().dif_configuration_.et_configuration_.
        				policy_set_.name_) != 0) {
        	std::cout << "DIFInformation.dif_configuration_.et_configuration_.policy_set_.name_. original and recovered messages"
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

int testIpcmUpdateDIFConfigurationRequestMessage() {
        std::cout << "TESTING IPCM UPDATE DIF CONFIGURATION REQUEST MESSAGE\n";
        int returnValue = 0;

        IpcmUpdateDIFConfigurationRequestMessage message;
        DIFConfiguration difConfiguration;
        PolicyParameter * parameter = new PolicyParameter("interface", "eth0");
        difConfiguration.add_parameter(*parameter);
        delete parameter;
        parameter = new PolicyParameter("vlanid", "430");
        difConfiguration.add_parameter(*parameter);
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
        } else if (message.getDIFConfiguration().get_parameters().size() !=
                        recoveredMessage->getDIFConfiguration().get_parameters().size()){
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

int testIpcmEnrollToDIFRequestMessage() {
        std::cout << "TESTING IPCM ENROLL TO DIF REQUEST MESSAGE\n";
        int returnValue = 0;

        IpcmEnrollToDIFRequestMessage message;
        ApplicationProcessNamingInformation difName;
        difName.processName = "normal.DIF";
        ApplicationProcessNamingInformation supportingDifName;
        supportingDifName.processName = "100";
        ApplicationProcessNamingInformation neighborName;
        neighborName.processName = "test";
        neighborName.processInstance = "1";
        ApplicationProcessNamingInformation disc_neigh_name;
        disc_neigh_name.processName = "test2";
        disc_neigh_name.processInstance = "1";
        message.setDifName(difName);
        message.setSupportingDifName(supportingDifName);
        message.setNeighborName(neighborName);
        message.prepare_for_handover = true;
        message.disc_neigh_name = disc_neigh_name;

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
        } else if (message.prepare_for_handover != recoveredMessage->prepare_for_handover) {
                std::cout << "Prepare for handover on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.disc_neigh_name != recoveredMessage->disc_neigh_name){
                std::cout << "disc_neigh_nameon original and recovered messages"
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
        message.result = -25;
        Neighbor neighbor;
        ApplicationProcessNamingInformation name;
        name.processName = "test";
        name.processInstance = "1";
        neighbor.set_name(name);
        ApplicationProcessNamingInformation supportingDIF;
        supportingDIF.processName = "100";
        neighbor.set_supporting_dif_name(supportingDIF);
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
        } else if (message.result != recoveredMessage->result) {
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

int testIpcmDisconnectNeighborRequestMessage() {
        std::cout << "TESTING DISCONNECT NEIGHBOR REQUEST MESSAGE\n";
        int returnValue = 0;

        IpcmDisconnectNeighborRequestMessage message;
        ApplicationProcessNamingInformation neighborName;
        neighborName.processName = "test";
        neighborName.processInstance = "1";
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
        IpcmDisconnectNeighborRequestMessage * recoveredMessage =
                        dynamic_cast<IpcmDisconnectNeighborRequestMessage *>(
                                        parseBaseNetlinkMessage(netlinkMessageHeader));
        if (recoveredMessage == 0) {
                std::cout << "Error parsing Disconnect Neighbor Request Message "
                                << "\n";
                returnValue = -1;
        } else if (message.getNeighborName() != recoveredMessage->getNeighborName()){
                std::cout << "Neighbour Name on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        }

        if (returnValue == 0) {
                std::cout << "IpcmDisconnectNeighborRequestMessage test ok\n";
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
	sourceName.processName = "/apps/source";
	sourceName.processInstance = "1";
	sourceName.entityName = "database";
	sourceName.entityInstance = "1234";
	message.setSourceAppName(sourceName);
	ApplicationProcessNamingInformation destName;
	destName.processName = "/apps/dest";
	destName.processInstance = "4";
	destName.entityName = "server";
	destName.entityInstance = "342";
	message.setDestAppName(destName);
	FlowSpecification flowSpec;
	message.setFlowSpec(flowSpec);
	ApplicationProcessNamingInformation difName;
	difName.processName = "/difs/Test.DIF";
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

int testIpcmAllocateFlowRequestArrivedMessage() {
	std::cout << "TESTING IPCM ALLOCATE FLOW REQUEST ARRIVED MESSAGE\n";
	int returnValue = 0;

	ApplicationProcessNamingInformation sourceName;
	sourceName.processName = "/apps/source";
	sourceName.processInstance = "1";
	sourceName.entityName = "database";
	sourceName.entityInstance = "1234";

	ApplicationProcessNamingInformation destName;
	destName.processName = "/apps/dest";
	destName.processInstance = "4";
	destName.entityName = "server";
	destName.entityInstance = "342";

	ApplicationProcessNamingInformation difName;;
	difName.processName = "/difs/Test1.DIF";

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
	message.result = 0;
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

	} else if (message.result != recoveredMessage->result) {
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

int testIpcmIPCProcessDIFResgistrationNotification() {
	std::cout << "TESTING IPCM IPC PROCESS DIF REGISTRATION NOTIFICATION\n";
	int returnValue = 0;

	IpcmDIFRegistrationNotification message;
	ApplicationProcessNamingInformation ipcProcessName;
	ipcProcessName.processName = "/apps/dest";
	ipcProcessName.processInstance = "4";
	ipcProcessName.entityName = "server";
	ipcProcessName.entityInstance = "342";
	message.setIpcProcessName(ipcProcessName);
	ApplicationProcessNamingInformation difName;
	difName.processName = "/difs/Test.DIF";
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
	appName.processName = "/test/apps/rinaband";
	appName.processInstance = "1";
	appName.entityName = "control";
	appName.entityInstance = "23";
	ApplicationProcessNamingInformation difName;
	difName.processName = "/difs/Test.DIF";
	ApplicationProcessNamingInformation difName2;
	difName2.processName = "/difs/shim-dummy.DIF";

	AppGetDIFPropertiesResponseMessage message;
	message.result = 0;
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
	} else if (message.result != recoveredMessage->result) {
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
				if (difProperties.DIFName != difName){
					std::cout << "DIF name on original and recovered messages"
							<< " are different 1\n";
					returnValue = -1;
					break;
				}else if (difProperties.maxSDUSize != 9000){
					std::cout << "Max SDU size on original and recovered messages"
							<< " are different 1\n";
					returnValue = -1;
					break;
				}

				i++;
			}else if (i == 1){
				if (difProperties.DIFName != difName2){
					std::cout << "DIF name on original and recovered messages"
							<< " are different 2 \n";
					returnValue = -1;
					break;
				}else if (difProperties.maxSDUSize != 25000){
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
        rate.set_sending_rate(345);
        rate.set_time_period(12456);

        DTCPWindowBasedFlowControlConfig window;
        window.set_initial_credit(98);
        window.set_max_closed_window_queue_length(23);

        DTCPFlowControlConfig dtcpFlowCtrlConfig;
        dtcpFlowCtrlConfig.set_rate_based(true);
        dtcpFlowCtrlConfig.set_rate_based_config(rate);
        dtcpFlowCtrlConfig.set_window_based(true);
        dtcpFlowCtrlConfig.set_window_based_config(window);
        dtcpFlowCtrlConfig.set_rcv_buffers_threshold(123);
        dtcpFlowCtrlConfig.set_rcv_bytes_threshold(50);
        dtcpFlowCtrlConfig.set_rcv_bytes_percent_threshold(456);
        dtcpFlowCtrlConfig.set_sent_buffers_threshold(32);
        dtcpFlowCtrlConfig.set_sent_bytes_percent_threshold(23);
        dtcpFlowCtrlConfig.set_sent_bytes_threshold(675);
        PolicyConfig closedWindowPolicy =
                        PolicyConfig("test-closed-window", "23");
        dtcpFlowCtrlConfig.set_closed_window_policy(closedWindowPolicy);

        DTCPRtxControlConfig dtcpRtxConfig;
        dtcpRtxConfig.set_max_time_to_retry(2312);
        dtcpRtxConfig.set_data_rxmsn_max(23424);
        dtcpRtxConfig.set_initial_rtx_time(20);

        DTCPConfig dtcpConfig;
        dtcpConfig.set_flow_control(true);
        dtcpConfig.set_flow_control_config(dtcpFlowCtrlConfig);
        dtcpConfig.set_rtx_control(true);
        dtcpConfig.set_rtx_control_config(dtcpRtxConfig);

        DTPConfig dtpConfig;
        dtpConfig.set_dtcp_present(true);
        dtpConfig.set_seq_num_rollover_threshold(123456);
        dtpConfig.set_initial_a_timer(34562);
        dtpConfig.set_partial_delivery(true);
        dtpConfig.set_max_sdu_gap(34);
        dtpConfig.set_in_order_delivery(true);
        dtpConfig.set_incomplete_delivery(true);

        IpcpConnectionCreateRequestMessage message;
        Connection connection;
        connection.setPortId(25);
        connection.setSourceAddress(1);
        connection.setDestAddress(2);
        connection.setQosId(3);
        connection.setDTPConfig(dtpConfig);
        connection.setDTCPConfig(dtcpConfig);

        connection.flowUserIpcProcessId = 3424;
        message.connection = connection;

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
        } else if (message.connection.getPortId() !=
                        recoveredMessage->connection.getPortId()) {
                std::cout << "Port id on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.flowUserIpcProcessId !=
                        recoveredMessage->connection.flowUserIpcProcessId) {
                std::cout << "Flow user IPCP id on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getSourceAddress()
                        != recoveredMessage->connection.getSourceAddress()) {
                std::cout << "Source address  on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getDestAddress() !=
                        recoveredMessage->connection.getDestAddress()) {
                std::cout <<message.connection.getDestAddress()
                                <<" "<<recoveredMessage->connection.getDestAddress();
                std::cout << "\nDestination address on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getQosId() !=
                        recoveredMessage->connection.getQosId()) {
                std::cout << "QoS id on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getSourceCepId()!=
                        recoveredMessage->connection.getSourceCepId()) {
                std::cout << "Src CEP id on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getDestCepId()!=
                        recoveredMessage->connection.getDestCepId()) {
                std::cout << "Dest CEP id on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getDTPConfig().is_partial_delivery()!=
                        recoveredMessage->connection.getDTPConfig().is_partial_delivery()) {
                std::cout << "Partial delivery on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getDTPConfig().is_in_order_delivery()!=
                        recoveredMessage->connection.getDTPConfig().is_in_order_delivery()) {
                std::cout << "In order delivery on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getDTPConfig().is_incomplete_delivery()!=
                        recoveredMessage->connection.getDTPConfig().is_incomplete_delivery()) {
                std::cout << "Incomplete delivery on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getDTPConfig().get_max_sdu_gap()!=
                        recoveredMessage->connection.getDTPConfig().get_max_sdu_gap()) {
                std::cout << "Max SDU gap on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getDTPConfig().is_dtcp_present() !=
                        recoveredMessage->connection.getDTPConfig().is_dtcp_present()) {
                std::cout << "dtpconfig.isDTCPPresent on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getDTPConfig().get_seq_num_rollover_threshold() !=
                        recoveredMessage->connection.getDTPConfig().get_seq_num_rollover_threshold()) {
                std::cout << "dtpconfig.seqnumrolloverthreshold on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getDTCPConfig().is_flow_control() !=
                        recoveredMessage->connection.getDTCPConfig().is_flow_control()){
                std::cout << "dtcpconfig.isFlowcontrol on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getDTCPConfig().is_rtx_control() !=
                        recoveredMessage->connection.getDTCPConfig().is_rtx_control()){
                std::cout << "dtcpconfig.isRtxcontrol on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getDTCPConfig().get_rtx_control_config().get_max_time_to_retry() !=
                        recoveredMessage->connection.getDTCPConfig().
                        get_rtx_control_config().get_max_time_to_retry()){
                std::cout << "dtcpconfig.rtxctrlconfig.maxtimetorety on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getDTCPConfig().get_rtx_control_config().get_initial_rtx_time() !=
        		recoveredMessage->connection.getDTCPConfig().
        		get_rtx_control_config().get_initial_rtx_time()){
        	std::cout << "dtcpconfig.rtxctrlconfig.initialrtxtime on original and recovered messages"
        			<< " are different\n";
        	returnValue = -1;
        } else if (message.connection.getDTPConfig().get_initial_a_timer() !=
        		recoveredMessage->connection.getDTPConfig().get_initial_a_timer()){
        	std::cout << "dtpconfig.initialAtimer on original and recovered messages"
        			<< " are different\n";
        	returnValue = -1;
        } else if (message.connection.getDTCPConfig().get_flow_control_config().is_rate_based() !=
                        recoveredMessage->connection.getDTCPConfig().
                        get_flow_control_config().is_rate_based()){
                std::cout << "dtcpconfig.flowctrlconfig.ratebased on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getDTCPConfig().get_flow_control_config().is_window_based() !=
                        recoveredMessage->connection.getDTCPConfig().
                        get_flow_control_config().is_window_based()){
                std::cout << "dtcpconfig.flowctrlconfig.windowbased on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getDTCPConfig().get_flow_control_config().get_rcv_buffers_threshold() !=
                        recoveredMessage->connection.getDTCPConfig().
                        get_flow_control_config().get_rcv_buffers_threshold()){
                std::cout << "dtcpconfig.flowctrlconfig.rcvbuffersthres on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getDTCPConfig().get_flow_control_config().get_rcv_bytes_percent_threshold() !=
                        recoveredMessage->connection.getDTCPConfig().
                        get_flow_control_config().get_rcv_bytes_percent_threshold()){
                std::cout << "dtcpconfig.flowctrlconfig.rcvbytespercentthres on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getDTCPConfig().get_flow_control_config().get_rcv_bytes_threshold() !=
                        recoveredMessage->connection.getDTCPConfig().
                        get_flow_control_config().get_rcv_bytes_threshold()){
                std::cout << "dtcpconfig.flowctrlconfig.rcvbytesthres on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getDTCPConfig().get_flow_control_config().get_sent_buffers_threshold() !=
                        recoveredMessage->connection.getDTCPConfig().
                        get_flow_control_config().get_sent_buffers_threshold()){
                std::cout << "dtcpconfig.flowctrlconfig.sentbuffersthres on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getDTCPConfig().get_flow_control_config().get_sent_bytes_percent_threshold() !=
                        recoveredMessage->connection.getDTCPConfig().
                        get_flow_control_config().get_sent_bytes_percent_threshold()){
                std::cout << "dtcpconfig.flowctrlconfig.sentbytespercentthres on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getDTCPConfig().get_flow_control_config().get_sent_bytes_threshold() !=
                        recoveredMessage->connection.getDTCPConfig().
                        get_flow_control_config().get_sent_bytes_threshold()){
                std::cout << "dtcpconfig.flowctrlconfig.sentbytesthres on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getDTCPConfig().get_flow_control_config().get_closed_window_policy().get_name().
                        compare(recoveredMessage->connection.getDTCPConfig().get_flow_control_config().
                        		get_closed_window_policy().get_name()) != 0 ){
                std::cout << "dtcpconfig.flowctrlconfig.closedwindowpolicy.name on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getDTCPConfig().get_flow_control_config().get_closed_window_policy().get_version().compare(
                        recoveredMessage->connection.getDTCPConfig().get_flow_control_config().
                        get_closed_window_policy().get_version()) != 0){
                std::cout << "dtcpconfig.flowctrlconfig.closedwindowpolicy.version on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getDTCPConfig().get_flow_control_config().
                        get_rate_based_config().get_sending_rate() != recoveredMessage->connection.getDTCPConfig().get_flow_control_config().get_rate_based_config().get_sending_rate()){
                std::cout << "dtcpconfig.flowctrlconfig.ratebasedconfig.sendingrate on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getDTCPConfig().get_flow_control_config().
        		get_rate_based_config().get_time_period() != recoveredMessage->connection.getDTCPConfig().get_flow_control_config().get_rate_based_config().get_time_period()){
                std::cout << "dtcpconfig.flowctrlconfig.ratebasedconfig.timerperiod on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getDTCPConfig().get_flow_control_config().
                        get_window_based_config().get_initial_credit() != recoveredMessage->connection.getDTCPConfig().get_flow_control_config().get_window_based_config().get_initial_credit()){
                std::cout << "dtcpconfig.flowctrlconfig.windowbasedconfig.initialcredit on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getDTCPConfig().get_flow_control_config().get_window_based_config().get_maxclosed_window_queue_length() !=
        		recoveredMessage->connection.getDTCPConfig().get_flow_control_config().get_window_based_config().get_maxclosed_window_queue_length()){
                std::cout << "dtcpconfig.flowctrlconfig.windowbasedconfig.maxclosedWindowQLength on original and recovered messages"
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

int testIpcpUpdateConnectionRequest() {
        std::cout << "TESTING IPCP UPDATE CONNECTION REQUEST MESSAGE\n";
        int returnValue = 0;

        IpcpConnectionUpdateRequestMessage message;
        message.portId = 25;
        message.sourceCepId = 345;
        message.destinationCepId = 212;

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
        } else if (message.portId != recoveredMessage->portId) {
                std::cout << "Port id on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.sourceCepId
                        != recoveredMessage->sourceCepId) {
                std::cout << "Source CEP-id  on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.destinationCepId !=
                        recoveredMessage->destinationCepId) {
                std::cout << "Destination CEP-id on original and recovered messages"
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
        message.connection = connection;

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
        } else if (message.connection.getPortId() !=
                        recoveredMessage->connection.getPortId()) {
                std::cout << "Port id on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getSourceAddress()
                        != recoveredMessage->connection.getSourceAddress()) {
                std::cout << "Source address  on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getDestAddress() !=
                        recoveredMessage->connection.getDestAddress()) {
                std::cout << "Destination address on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getQosId() !=
                        recoveredMessage->connection.getQosId()) {
                std::cout << "QoS id on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getDestCepId() !=
                        recoveredMessage->connection.getDestCepId()) {
                std::cout << "Dest cep-id on original and recovered messages"
                                << " are different\n";
                returnValue = -1;
        } else if (message.connection.getFlowUserIpcProcessId() !=
                        recoveredMessage->connection.getFlowUserIpcProcessId()) {
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

int testRmtModifyPDUFTEntriesRequestMessage() {
        std::cout << "TESTING RMT MODIFY PDU FTE REQUEST MESSAGE\n";
        int returnValue = 0;
        std::list<PDUForwardingTableEntry *>::const_iterator iterator;
        std::list<PDUForwardingTableEntry *> entriesList;
        std::list<PortIdAltlist>::const_iterator iterator2;
        std::list<unsigned int>::const_iterator it3;
        std::list<PortIdAltlist> portIdsList;

        RmtModifyPDUFTEntriesRequestMessage message;
        PDUForwardingTableEntry * entry1 = new PDUForwardingTableEntry();
        entry1->setAddress(23);
        entry1->portIdAltlists.push_back(34);
        entry1->portIdAltlists.push_back(29);
        entry1->portIdAltlists.push_back(36);
        entry1->setQosId(1);
        message.addEntry(entry1);
        PDUForwardingTableEntry * entry2 = new PDUForwardingTableEntry();
        entry2->setAddress(20);
        entry2->portIdAltlists.push_back(28);
        entry2->portIdAltlists.push_back(35);
        entry2->portIdAltlists.push_back(43);
        entry2->setQosId(2);
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
                portIdsList = (*iterator)->portIdAltlists;
                if (portIdsList.size() != 3) {
                        std::cout << "Size of portids in original and recovered messages"
                                        << " are different\n";
                        returnValue = -1;
                }

                for(iterator2 = portIdsList.begin();
                                iterator2 != portIdsList.end();
                                ++iterator2) {
			for (it3 = iterator2->alts.begin();
					it3 != iterator2->alts.end(); it3++) {
				std::cout << *it3 <<std::endl;
			}
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
        std::list<PortIdAltlist>::const_iterator iterator2;
	std::list<unsigned int>::const_iterator it3;
        std::list<PortIdAltlist> portIdsList;

        RmtDumpPDUFTEntriesResponseMessage message;
        PDUForwardingTableEntry entry1, entry2;
        entry1.setAddress(23);
        entry1.portIdAltlists.push_back(34);
        entry1.portIdAltlists.push_back(29);
        entry1.portIdAltlists.push_back(36);
        entry1.setQosId(1);
        message.addEntry(entry1);
        entry2.setAddress(20);
        entry2.portIdAltlists.push_back(28);
        entry2.portIdAltlists.push_back(35);
        entry2.portIdAltlists.push_back(54);
        entry2.setQosId(2);
        message.addEntry(entry2);
        message.result = 3;

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
        } else if (message.result != recoveredMessage->result) {
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
                portIdsList = iterator->portIdAltlists;
                if (portIdsList.size() != 3) {
                        std::cout << "Size of portids in original and recovered messages"
                                        << " are different\n";
                        returnValue = -1;
                }

                for(iterator2 = portIdsList.begin();
                                iterator2 != portIdsList.end();
                                ++iterator2) {
			for (it3 = iterator2->alts.begin();
					it3 != iterator2->alts.end(); it3++) {
				std::cout << *it3 <<std::endl;
			}
                }
        }

        if (returnValue == 0) {
                std::cout << "RmtDumpPDUFTEntriesResponseMessage test ok\n";
        }
        nlmsg_free(netlinkMessage);
        delete recoveredMessage;

        return returnValue;
}

int testUpdateCryptoStateRequestMessage() {
        std::cout << "TESTING UPDATE CRYPTO STATE REQUEST MESSAGE\n";
        int returnValue = 0;

        IPCPUpdateCryptoStateRequestMessage message;
        message.state.enable_crypto_tx = true;
        message.state.enable_crypto_rx = true;
        message.state.port_id = 232;
        message.state.encrypt_key_tx.length = 16;
        message.state.encrypt_key_tx.data = new unsigned char[16];
        message.state.encrypt_key_tx.data[0] = 0x01;
        message.state.encrypt_key_tx.data[1] = 0x02;
        message.state.encrypt_key_tx.data[2] = 0x03;
        message.state.encrypt_key_tx.data[3] = 0x04;
        message.state.encrypt_key_tx.data[4] = 0x05;
        message.state.encrypt_key_tx.data[5] = 0x06;
        message.state.encrypt_key_tx.data[6] = 0x07;
        message.state.encrypt_key_tx.data[7] = 0x08;
        message.state.encrypt_key_tx.data[8] = 0x09;
        message.state.encrypt_key_tx.data[9] = 0x10;
        message.state.encrypt_key_tx.data[10] = 0x11;
        message.state.encrypt_key_tx.data[11] = 0x12;
        message.state.encrypt_key_tx.data[12] = 0x13;
        message.state.encrypt_key_tx.data[13] = 0x14;
        message.state.encrypt_key_tx.data[14] = 0x15;
        message.state.encrypt_key_tx.data[15] = 0x16;

        struct nl_msg* netlinkMessage = nlmsg_alloc();
        if (!netlinkMessage) {
                std::cout << "Error allocating Netlink message\n";
        }
        genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
                        sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

        int result = putBaseNetlinkMessage(netlinkMessage, &message);
        if (result < 0) {
                std::cout << "Error constructing IPCPEnableEncryptionRequestMessage "
                                << "message \n";
                nlmsg_free(netlinkMessage);
                return result;
        }

        nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
        IPCPUpdateCryptoStateRequestMessage * recoveredMessage =
                        dynamic_cast<IPCPUpdateCryptoStateRequestMessage *>(
                                        parseBaseNetlinkMessage(netlinkMessageHeader));

        if (recoveredMessage == 0) {
                std::cout << "Error parsing IPCPEnableEncryptionRequestMessage message "
                                << "\n";
                returnValue = -1;
        } else if (message.state.enable_crypto_rx != recoveredMessage->state.enable_crypto_rx) {
        	std::cout << "Error with enable_crypto_rx" << std::endl;
        	returnValue = -1;
        } else if (message.state.enable_crypto_tx != recoveredMessage->state.enable_crypto_tx) {
        	std::cout << "Error with enable_crypto_tx"<< std::endl;
        	returnValue = -1;
        } else if (message.state.port_id != recoveredMessage->state.port_id) {
        	std::cout << "Error with port_id"<< std::endl;
        	returnValue = -1;
        } else if (message.state.encrypt_key_tx.length != recoveredMessage->state.encrypt_key_tx.length) {
        	std::cout << "Error with encrypt_key_tx length"<< std::endl;
        	returnValue = -1;
        } else if (message.state.encrypt_key_tx.data[10] != recoveredMessage->state.encrypt_key_tx.data[10]) {
        	std::cout << "Error with encrypt_key_tx data"<< std::endl;
        	returnValue = -1;
        }

        if (returnValue == 0) {
                std::cout << "IPCPUpdateCryptoStateRequestMessage test ok\n";
        }
        nlmsg_free(netlinkMessage);
        delete recoveredMessage;

        return returnValue;
}

int testAddressChangeRequestMessage() {
        std::cout << "TESTING ADDRESS CHANGE REQUEST MESSAGE\n";
        int returnValue = 0;

        IPCPAddressChangeRequestMessage message;
        message.new_address = 25;
        message.old_address = 16;
        message.use_new_timeout = 3500;
        message.deprecate_old_timeout = 10200;

        struct nl_msg* netlinkMessage = nlmsg_alloc();
        if (!netlinkMessage) {
                std::cout << "Error allocating Netlink message\n";
        }
        genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
                        sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

        int result = putBaseNetlinkMessage(netlinkMessage, &message);
        if (result < 0) {
                std::cout << "Error constructing IPCPAddressChangeRequestMessage "
                                << "message \n";
                nlmsg_free(netlinkMessage);
                return result;
        }

        nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
        IPCPAddressChangeRequestMessage * recoveredMessage =
                        dynamic_cast<IPCPAddressChangeRequestMessage *>(
                                        parseBaseNetlinkMessage(netlinkMessageHeader));

        if (recoveredMessage == 0) {
                std::cout << "Error parsing IPCPAddressChangeRequestMessage message "
                                << "\n";
                returnValue = -1;
        } else if (message.new_address!= recoveredMessage->new_address) {
        	std::cout << "Error with new_address" << std::endl;
        	returnValue = -1;
        } else if (message.old_address != recoveredMessage->old_address) {
        	std::cout << "Error with old_address"<< std::endl;
        	returnValue = -1;
        } else if (message.use_new_timeout!= recoveredMessage->use_new_timeout) {
        	std::cout << "Error with use_new_timeout" << std::endl;
        	returnValue = -1;
        } else if (message.deprecate_old_timeout != recoveredMessage->deprecate_old_timeout) {
        	std::cout << "Error with deprecate_old_timeout"<< std::endl;
        	returnValue = -1;
        }

        if (returnValue == 0) {
                std::cout << "IPCPAddressChangeRequestMessage test ok\n";
        }
        nlmsg_free(netlinkMessage);
        delete recoveredMessage;

        return returnValue;
}

int testIpcmMediaReportMessage() {
        std::cout << "TESTING MEDIA REPORT MESSAGE\n";
        int returnValue = 0;

        IpcmMediaReportMessage message;
        MediaDIFInfo dif_info;
        BaseStationInfo bs_info;

        message.report.ipcp_id = 12;
        message.report.current_dif_name = "wi2cat";
        message.report.bs_ipcp_address = "50:60:28:2f:76:51";

        dif_info.dif_name = "wi2cat";
        dif_info.security_policies = "WPA(PSK/AES,TKIP/TKIP) WPA2(PSK/AES,TKIP/TKIP)";

        bs_info.ipcp_address = "50:60:28:2f:76:51";
        bs_info.signal_strength = -54;
        dif_info.available_bs_ipcps.push_back(bs_info);

        bs_info.ipcp_address = "50:60:28:30:31:71";
        bs_info.signal_strength = -58;
        dif_info.available_bs_ipcps.push_back(bs_info);
        message.report.available_difs["wi2cat"] = dif_info;

        dif_info.dif_name = "wi2guest";
        dif_info.security_policies = "WPA(PSK/AES/AES) WPA2(PSK/AES/AES) ";

        bs_info.ipcp_address = "50:60:28:2f:76:52";
        bs_info.signal_strength = -36;
        dif_info.available_bs_ipcps.push_back(bs_info);

        bs_info.ipcp_address = "50:60:28:30:31:70";
        bs_info.signal_strength = -49;
        dif_info.available_bs_ipcps.push_back(bs_info);
        message.report.available_difs["wi2guest"] = dif_info;

        struct nl_msg* netlinkMessage = nlmsg_alloc();
        if (!netlinkMessage) {
                std::cout << "Error allocating Netlink message\n";
        }
        genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
                        sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

        int result = putBaseNetlinkMessage(netlinkMessage, &message);
        if (result < 0) {
                std::cout << "Error constructing IpcmMediaReportMessage "
                                << "message \n";
                nlmsg_free(netlinkMessage);
                return result;
        }

        nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
        IpcmMediaReportMessage * recoveredMessage =
                        dynamic_cast<IpcmMediaReportMessage *>(
                                        parseBaseNetlinkMessage(netlinkMessageHeader));

        if (recoveredMessage == 0) {
                std::cout << "Error parsing IpcmMediaReportMessage message "
                                << "\n";
                returnValue = -1;
        } else if (message.report.ipcp_id != recoveredMessage->report.ipcp_id) {
        	std::cout << "Error with ipcp_id" << std::endl;
        	returnValue = -1;
        } else if (message.report.available_difs.size() != recoveredMessage->report.available_difs.size()) {
        	std::cout << "Error with available DIFs size"<< std::endl;
        	returnValue = -1;
        } else if (message.report.current_dif_name.compare(recoveredMessage->report.current_dif_name) != 0) {
        	std::cout << "Error with current dif name *"
        		  << message.report.current_dif_name << "* *"
			  << recoveredMessage->report.current_dif_name << "*" << std::endl;
        	returnValue = -1;
        } else if (message.report.bs_ipcp_address!= recoveredMessage->report.bs_ipcp_address) {
        	std::cout << "Error with bs IPCP address" << std::endl;
        	returnValue = -1;
        } else if (message.report.toString()!= recoveredMessage->report.toString()) {
        	std::cout << "Error comparing reports" << std::endl;
        	returnValue = -1;
        }

        if (returnValue == 0) {
                std::cout << "IpcmMediaReportMessage test ok\n";
        }
        nlmsg_free(netlinkMessage);
        delete recoveredMessage;

        return returnValue;
}

int testIPCPAllocatePortRequestMessage() {
        std::cout << "TESTING IPCP ALLOCATE PORT REQUEST MESSAGE\n";
        int returnValue = 0;

        IPCPAllocatePortRequestMessage message;
        message.app_name.processName = "test";
        message.app_name.processInstance = "1";
        message.app_name.entityName = "more";
        message.app_name.entityInstance = "3";

        struct nl_msg* netlinkMessage = nlmsg_alloc();
        if (!netlinkMessage) {
                std::cout << "Error allocating Netlink message\n";
        }
        genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
                        sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

        int result = putBaseNetlinkMessage(netlinkMessage, &message);
        if (result < 0) {
                std::cout << "Error constructing IPCPAllocatePortRequestMessage "
                                << "message \n";
                nlmsg_free(netlinkMessage);
                return result;
        }

        nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
        IPCPAllocatePortRequestMessage * recoveredMessage =
                        dynamic_cast<IPCPAllocatePortRequestMessage *>(
                                        parseBaseNetlinkMessage(netlinkMessageHeader));

        if (recoveredMessage == 0) {
                std::cout << "Error parsing IPCPAllocatePortRequestMessage message "
                                << "\n";
                returnValue = -1;
        } else if (message.app_name.processName!= recoveredMessage->app_name.processName) {
        	std::cout << "Error with processName" << std::endl;
        	returnValue = -1;
        } else if (message.app_name.processInstance != recoveredMessage->app_name.processInstance) {
        	std::cout << "Error with processInstance"<< std::endl;
        	returnValue = -1;
        } else if (message.app_name.entityName!= recoveredMessage->app_name.entityName) {
        	std::cout << "Error with entityName" << std::endl;
        	returnValue = -1;
        } else if (message.app_name.entityInstance != recoveredMessage->app_name.entityInstance) {
        	std::cout << "Error with entityInstance"<< std::endl;
        	returnValue = -1;
        }

        if (returnValue == 0) {
                std::cout << "IPCPAllocatePortRequestMessage test ok\n";
        }
        nlmsg_free(netlinkMessage);
        delete recoveredMessage;

        return returnValue;
}

int testIPCPWriteManagementSDURequestMessage()
{
        std::cout << "TESTING IPCP WRITE MANAGEMENT SDU REQUEST MESSAGE\n";
        int returnValue = 0;

        IPCPWriteMgmtSDURequestMessage message;
        message.sdu = new unsigned char[20];
        message.size = 20;
        message.port_id = 30;
        message.address = 54;
        struct nl_msg* netlinkMessage = nlmsg_alloc();
        if (!netlinkMessage) {
                std::cout << "Error allocating Netlink message\n";
        }
        genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
                        sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

        int result = putBaseNetlinkMessage(netlinkMessage, &message);
        if (result < 0) {
                std::cout << "Error constructing IPCPWriteMgmtSDURequestMessage "
                                << "message \n";
                nlmsg_free(netlinkMessage);
                return result;
        }

        nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
        IPCPWriteMgmtSDURequestMessage * recoveredMessage =
                        dynamic_cast<IPCPWriteMgmtSDURequestMessage *>(
                                        parseBaseNetlinkMessage(netlinkMessageHeader));

        if (recoveredMessage == 0) {
                std::cout << "Error parsing IPCPWriteMgmtSDURequestMessage message "
                                << "\n";
                returnValue = -1;
        } else if (message.port_id != recoveredMessage->port_id) {
        	std::cout << "Error with port_id"<< std::endl;
        	returnValue = -1;
        } else if (message.address != recoveredMessage->address) {
        	std::cout << "Error with address"<< std::endl;
        	returnValue = -1;
        } else if (message.size != recoveredMessage->size) {
        	std::cout << "Error with size"<< std::endl;
        	returnValue = -1;
        }

        if (returnValue == 0) {
                std::cout << "IPCPWriteMgmtSDURequestMessage test ok\n";
        }
        nlmsg_free(netlinkMessage);
        delete (unsigned char*) message.sdu;
        delete (unsigned char*) recoveredMessage->sdu;
        delete recoveredMessage;

        return returnValue;
}

int testIPCPReadManagementSDUNotificationMessage()
{
        std::cout << "TESTING IPCP READ MANAGEMENT SDU NOTIFICATION MESSAGE\n";
        int returnValue = 0;

        IPCPReadMgmtSDUNotificationMessage message;
        message.sdu = new unsigned char[20];
        message.size = 20;
        message.port_id = 30;
        struct nl_msg* netlinkMessage = nlmsg_alloc();
        if (!netlinkMessage) {
                std::cout << "Error allocating Netlink message\n";
        }
        genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
                        sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

        int result = putBaseNetlinkMessage(netlinkMessage, &message);
        if (result < 0) {
                std::cout << "Error constructing IPCPReadMgmtSDUNotificationMessage "
                                << "message \n";
                nlmsg_free(netlinkMessage);
                return result;
        }

        nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
        IPCPReadMgmtSDUNotificationMessage * recoveredMessage =
                        dynamic_cast<IPCPReadMgmtSDUNotificationMessage *>(
                                        parseBaseNetlinkMessage(netlinkMessageHeader));

        if (recoveredMessage == 0) {
                std::cout << "Error parsing IPCPReadMgmtSDUNotificationMessage message "
                                << "\n";
                returnValue = -1;
        } else if (message.port_id != recoveredMessage->port_id) {
        	std::cout << "Error with port_id"<< std::endl;
        	returnValue = -1;
        } else if (message.size != recoveredMessage->size) {
        	std::cout << "Error with size"<< std::endl;
        	returnValue = -1;
        }

        if (returnValue == 0) {
                std::cout << "IPCPReadMgmtSDUNotificationMessage test ok\n";
        }
        nlmsg_free(netlinkMessage);
        delete (unsigned char*) message.sdu;
        delete (unsigned char*) recoveredMessage->sdu;
        delete recoveredMessage;

        return returnValue;
}

int testIpcmCreateIPCProcessRequestMessage()
{
        std::cout << "TESTING IPCM CREATE IPCP REQUEST MESSAGE\n";
        int returnValue = 0;

        IpcmCreateIPCPRequestMessage message;
        message.dif_type = "normal-ipcp";
        message.ipcp_id = 234;
        message.nl_port_id = 8987;
        message.ipcp_name.processName = "test1.IRATI";
        message.ipcp_name.processInstance = "1";
        struct nl_msg* netlinkMessage = nlmsg_alloc();
        if (!netlinkMessage) {
                std::cout << "Error allocating Netlink message\n";
        }
        genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
                        sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

        int result = putBaseNetlinkMessage(netlinkMessage, &message);
        if (result < 0) {
                std::cout << "Error constructing IpcmCreateIPCPRequestMessage "
                                << "message \n";
                nlmsg_free(netlinkMessage);
                return result;
        }

        nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
        IpcmCreateIPCPRequestMessage * recoveredMessage =
                        dynamic_cast<IpcmCreateIPCPRequestMessage *>(
                                        parseBaseNetlinkMessage(netlinkMessageHeader));

        if (recoveredMessage == 0) {
                std::cout << "Error parsing IpcmCreateIPCPRequestMessage message "
                                << "\n";
                returnValue = -1;
        } else if (message.ipcp_id != recoveredMessage->ipcp_id) {
        	std::cout << "Error with ipcp_id"<< std::endl;
        	returnValue = -1;
        } else if (message.nl_port_id != recoveredMessage->nl_port_id) {
        	std::cout << "Error with nl_port_id"<< std::endl;
        	returnValue = -1;
        } else if (message.dif_type != recoveredMessage->dif_type) {
        	std::cout << "Error with dif_type"<< std::endl;
        	returnValue = -1;
        } else if (message.ipcp_name.processName != recoveredMessage->ipcp_name.processName) {
        	std::cout << "Error with processName"<< std::endl;
        	returnValue = -1;
        } else if (message.ipcp_name.processInstance != recoveredMessage->ipcp_name.processInstance) {
        	std::cout << "Error with processInstance"<< std::endl;
        	returnValue = -1;
        }

        if (returnValue == 0) {
                std::cout << "IpcmCreateIPCPRequestMessage test ok\n";
        }
        nlmsg_free(netlinkMessage);
        delete recoveredMessage;

        return returnValue;
}

int testIpcmDestroyIPCProcessRequestMessage()
{
        std::cout << "TESTING IPCM DESTROY IPCP REQUEST MESSAGE\n";
        int returnValue = 0;

        IpcmDestroyIPCPRequestMessage message;
        message.ipcp_id = 234;
        struct nl_msg* netlinkMessage = nlmsg_alloc();
        if (!netlinkMessage) {
                std::cout << "Error allocating Netlink message\n";
        }
        genlmsg_put(netlinkMessage, NL_AUTO_PORT, message.getSequenceNumber(), 21,
                        sizeof(struct rinaHeader), 0, message.getOperationCode(), 0);

        int result = putBaseNetlinkMessage(netlinkMessage, &message);
        if (result < 0) {
                std::cout << "Error constructing IpcmDestroyIPCPRequestMessage "
                                << "message \n";
                nlmsg_free(netlinkMessage);
                return result;
        }

        nlmsghdr* netlinkMessageHeader = nlmsg_hdr(netlinkMessage);
        IpcmDestroyIPCPRequestMessage * recoveredMessage =
                        dynamic_cast<IpcmDestroyIPCPRequestMessage *>(
                                        parseBaseNetlinkMessage(netlinkMessageHeader));

        if (recoveredMessage == 0) {
                std::cout << "Error parsing IpcmDestroyIPCPRequestMessage message "
                                << "\n";
                returnValue = -1;
        } else if (message.ipcp_id != recoveredMessage->ipcp_id) {
        	std::cout << "Error with ipcp_id"<< std::endl;
        	returnValue = -1;
        }

        if (returnValue == 0) {
                std::cout << "IpcmDestroyIPCPRequestMessage test ok\n";
        }
        nlmsg_free(netlinkMessage);
        delete recoveredMessage;

        return returnValue;
}*/

int main() {
	std::cout << "TESTING LIBRINA-PARSERS\n";

	int result;

	result = test_irati_kmsg_ipcm_allocate_flow(RINA_C_IPCM_ALLOCATE_FLOW_REQUEST);
	if (result < 0) {
		return result;
	}

	result = test_irati_kmsg_ipcm_allocate_flow(RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_ARRIVED);
	if (result < 0) {
		return result;
	}

	result = test_irati_kmsg_ipcm_allocate_flow(RINA_C_APP_ALLOCATE_FLOW_REQUEST);
	if (result < 0) {
		return result;
	}

	result = test_irati_kmsg_ipcm_allocate_flow(RINA_C_APP_ALLOCATE_FLOW_REQUEST_ARRIVED);
	if (result < 0) {
		return result;
	}

	/*result = testAppAllocateFlowRequestResultMessage();
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

	result = testBaseNetlinkMessage();
	if (result < 0) {
		return result;
	}

	result = testIpcmUnregisterApplicationRequestMessage();
	if (result < 0) {
		return result;
	}

	result = testIpcmAssignToDIFRequestMessage();
	if (result < 0) {
		return result;
	}

	result = testIpcmUpdateDIFConfigurationRequestMessage();
	if (result < 0) {
	        return result;
	}

	result = testIpcmEnrollToDIFRequestMessage();
	if (result < 0) {
	        return result;
	}

	result = testIpcmEnrollToDIFResponseMessage();
	if (result < 0) {
	        return result;
	}

	result = testIpcmDisconnectNeighborRequestMessage();
	if (result < 0) {
	        return result;
	}

	result = testIpcmAllocateFlowRequestMessage();
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

	result = testIpcpUpdateConnectionRequest();
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

	result = testRmtModifyPDUFTEntriesRequestMessage();
	if (result < 0) {
	        return result;
	}

	result = testRmtDumpPDUFTResponseMessage();
	if (result < 0) {
	        return result;
	}

	result = testUpdateCryptoStateRequestMessage();
	if (result < 0) {
		return result;
	}

	result = testAddressChangeRequestMessage();
	if (result < 0) {
		return result;
	}

	result = testIpcmMediaReportMessage();
	if (result < 0) {
		return result;
	}

	result = testIPCPAllocatePortRequestMessage();
	if (result < 0) {
		return result;
	}

	result = testIPCPWriteManagementSDURequestMessage();
	if (result < 0) {
		return result;
	}

	result = testIPCPReadManagementSDUNotificationMessage();
	if (result < 0) {
		return result;
	}

	result = testIpcmCreateIPCProcessRequestMessage();
	if (result < 0) {
		return result;
	}

	result = testIpcmDestroyIPCProcessRequestMessage();
	if (result < 0) {
		return result;
	}*/

	return 0;
}
