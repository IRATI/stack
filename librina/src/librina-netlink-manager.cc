/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * librina-netlink-manager.cc
 *
 *  Created on: 12/06/2013
 *      Author: eduardgrasa
 */

#include "librina-netlink-manager.h"
#define RINA_PREFIX "netlink-manager"
#include "logs.h"
#include <unistd.h>
#include "librina-netlink-parsers.h"
#include <netlink/socket.h>
#define MY_MESSAGE_TYPE 17

namespace rina {

/* CLASS BASE NETLINK MESSAGE */

BaseNetlinkMessage::BaseNetlinkMessage(RINANetlinkOperationCode operationCode) {
	this->operationCode = operationCode;
	sourcePortId = 0;
	destPortId = 0;
	sequenceNumber = 0;
}

BaseNetlinkMessage::~BaseNetlinkMessage() {
}

unsigned int BaseNetlinkMessage::getDestPortId() const {
	return destPortId;
}

void BaseNetlinkMessage::setDestPortId(unsigned int destPortId) {
	this->destPortId = destPortId;
}

unsigned int BaseNetlinkMessage::getSequenceNumber() const {
	return sequenceNumber;
}

void BaseNetlinkMessage::setSequenceNumber(unsigned int sequenceNumber) {
	this->sequenceNumber = sequenceNumber;
}

unsigned int BaseNetlinkMessage::getSourcePortId() const {
	return sourcePortId;
}

void BaseNetlinkMessage::setSourcePortId(unsigned int sourcePortId) {
	this->sourcePortId = sourcePortId;
}

RINANetlinkOperationCode BaseNetlinkMessage::getOperationCode() const {
	return operationCode;
}

/* CLASS RINA APP ALLOCATE FLOW MESSAGE */

AppAllocateFlowRequestMessage::AppAllocateFlowRequestMessage() :
		BaseNetlinkMessage(RINA_C_APP_ALLOCATE_FLOW_REQUEST){
}

const ApplicationProcessNamingInformation& AppAllocateFlowRequestMessage::getDestAppName() const {
	return destAppName;
}

void AppAllocateFlowRequestMessage::setDestAppName(
		const ApplicationProcessNamingInformation& destAppName) {
	this->destAppName = destAppName;
}

const FlowSpecification& AppAllocateFlowRequestMessage::getFlowSpecification() const {
	return flowSpecification;
}

void AppAllocateFlowRequestMessage::setFlowSpecification(
		const FlowSpecification& flowSpecification) {
	this->flowSpecification = flowSpecification;
}

const ApplicationProcessNamingInformation& AppAllocateFlowRequestMessage::getSourceAppName() const {
	return sourceAppName;
}

void AppAllocateFlowRequestMessage::setSourceAppName(
		const ApplicationProcessNamingInformation& sourceAppName) {
	this->sourceAppName = sourceAppName;
}

/* CLASS NETLINK MANAGER */

NetlinkManager::NetlinkManager() {
	this->localPort = getpid();
	LOG_DBG("Netlink Manager constructor called, using local port = %d",
			localPort);
	initialize();
}

NetlinkManager::NetlinkManager(unsigned int localPort) {
	this->localPort = localPort;
	LOG_DBG("Netlink Manager constructor called, with netlink pid = %d",
			localPort);
	initialize();
}

NetlinkManager::~NetlinkManager() {
	LOG_DBG("Netlink Manager destructor called");
	nl_socket_free(socket);
}

void NetlinkManager::initialize() {
	socket = nl_socket_alloc();
	nl_socket_set_local_port(socket, localPort);
	int result = nl_connect(socket, NETLINK_GENERIC);
	if (result == 0) {
		LOG_DBG("Netlink socket connected to local port %d ",
				nl_socket_get_local_port(socket));
	} else {
		LOG_DBG("Got error %d", result);
	}
}

void NetlinkManager::sendMessage(
		BaseNetlinkMessage * message) {
	//Generate the message
	struct nl_msg* netlinkMessage;

	netlinkMessage = nlmsg_alloc_simple(MY_MESSAGE_TYPE, NLM_F_REQUEST);

	switch(message->getOperationCode()){
	case RINA_C_APP_ALLOCATE_FLOW_REQUEST:{
		AppAllocateFlowRequestMessage * allocateObject = dynamic_cast<AppAllocateFlowRequestMessage *>(message);
		if (putAppAllocateFlowRequestMessageObject(netlinkMessage, allocateObject) <0){
				LOG_WARN("Error, to do, throw Exception");
				nlmsg_free(netlinkMessage);
				return;
			}
		break;
	}
	default:{
		LOG_WARN("Error, to do, throw Exception");
		nlmsg_free(netlinkMessage);
		return;
	}
	}

	//Set destination and send the message
	nl_socket_set_peer_port(socket, message->getDestPortId());
	nl_send_auto(socket, netlinkMessage);
	LOG_DBG("Sent message to %d", message->getDestPortId());

	//Cleanup
	nlmsg_free(netlinkMessage);
	return;
}

BaseNetlinkMessage * NetlinkManager::getMessage() {
	unsigned char *buf = NULL;
	struct nlmsghdr *hdr;
	struct sockaddr_nl nla = { 0 };
	struct nl_msg *msg = NULL;
	struct ucred *creds = NULL;

	int n = nl_recv(socket, &nla, &buf, &creds);
	if (n <= 0) {
		LOG_DBG("Error receiving Netlink message %d", n);
		return NULL;
	}

	LOG_DBG("Received %d bytes, parsing the message", n);

	hdr = (struct nlmsghdr *) buf;
	msg = nlmsg_convert(hdr);
	if (!msg) {
		LOG_DBG("Error");
		return NULL;
	}

	nlmsg_set_src(msg, &nla);

	LOG_DBG("Message type %d", hdr->nlmsg_type);
	if (hdr->nlmsg_flags & NLM_F_REQUEST) {
		LOG_DBG("It is a request message");
	} else if (hdr->nlmsg_flags & NLMSG_ERROR) {
		LOG_DBG("It is an error message");
	}

	if (creds) {
		nlmsg_set_creds(msg, creds);
	}

	AppAllocateFlowRequestMessage * result =
					parseAppAllocateFlowRequestMessage(hdr);

	if (result == NULL){
		nlmsg_free(msg);
		free(buf);
		free(creds);
		return NULL;
	}

	result->setDestPortId(localPort);
	result->setSourcePortId(nla.nl_pid);
	result->setSequenceNumber(hdr->nlmsg_seq);

	nlmsg_free(msg);
	free(buf);
	free(creds);
	return result;
}

}

