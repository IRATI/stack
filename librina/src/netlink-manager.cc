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

#include <unistd.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/socket.h>

#define RINA_PREFIX "netlink-manager"

#include "logs.h"
#include "netlink-manager.h"
#include "netlink-parsers.h"

namespace rina {

/* CLASS NETLINK EXCEPTION */
NetlinkException::NetlinkException(const std::string& description) :
		Exception(description) {
}

const std::string NetlinkException::error_resolving_netlink_family =
		"Error resolving RINA Generic Netlink family";
const std::string NetlinkException::error_connecting_netlink_socket =
		"Error connecting Netlink socket";
const std::string NetlinkException::error_allocating_netlink_message =
		"Error allocating Netlink message";
const std::string NetlinkException::error_receiving_netlink_message =
		"Error receiving Netlink message";
const std::string NetlinkException::error_generating_netlink_message =
		"Error generating Netlink message";
const std::string NetlinkException::error_sending_netlink_message =
		"Error sending Netlink message";
const std::string NetlinkException::error_parsing_netlink_message =
		"Error parsing Netlink message";

/* CLASS NETLINK MANAGER */

NetlinkManager::NetlinkManager() throw (NetlinkException) {
	this->localPort = getpid();
	LOG_DBG("Netlink Manager constructor called, using local port = %d",
			localPort);
	initialize();
}

NetlinkManager::NetlinkManager(unsigned int localPort)
		throw (NetlinkException) {
	this->localPort = localPort;
	LOG_DBG("Netlink Manager constructor called, with netlink pid = %d",
			localPort);
	initialize();
}

NetlinkManager::~NetlinkManager() {
	LOG_DBG("Netlink Manager destructor called");
	nl_socket_free(socket);
}

void NetlinkManager::initialize() throw (NetlinkException) {
	socket = nl_socket_alloc();
	nl_socket_set_local_port(socket, localPort);

	int result = genl_connect(socket);
	if (result == 0) {
		LOG_INFO("Netlink socket connected to local port %d ",
				nl_socket_get_local_port(socket));
	} else {
		LOG_CRIT("%s %d",
				NetlinkException::error_connecting_netlink_socket.c_str(),
				result);
		throw NetlinkException(
				NetlinkException::error_connecting_netlink_socket);
	}

	family = genl_ctrl_resolve(socket, RINA_GENERIC_NETLINK_FAMILY_NAME);
	if (family < 0){
		LOG_CRIT("%s %d",
				NetlinkException::error_resolving_netlink_family.c_str(),
				family);
		throw NetlinkException(
				NetlinkException::error_resolving_netlink_family);
	}
	LOG_DBG("Generic Netlink RINA family id: %d", family);
}

void NetlinkManager::sendMessage(BaseNetlinkMessage * message)
		throw (NetlinkException) {
	//Generate the message
	struct nl_msg* netlinkMessage;

	netlinkMessage = nlmsg_alloc();
	if (!netlinkMessage){
		LOG_ERR("%s",
				NetlinkException::error_allocating_netlink_message.c_str());
		throw NetlinkException(
				NetlinkException::error_allocating_netlink_message);
	}

	genlmsg_put(netlinkMessage, NL_AUTO_PORT, message->getSequenceNumber(),
			family, 0, 0, message->getOperationCode(),
			RINA_GENERIC_NETLINK_FAMILY_VERSION);

	int result = putBaseNetlinkMessage(netlinkMessage, message);
	if (result < 0) {
		nlmsg_free(netlinkMessage);
		LOG_ERR("%s %d",
				NetlinkException::error_generating_netlink_message.c_str(),
				result);
		throw NetlinkException(
				NetlinkException::error_generating_netlink_message);
	}

	//Set destination and send the message
	nl_socket_set_peer_port(socket, message->getDestPortId());
	result = nl_send_auto(socket, netlinkMessage);
	if (result < 0) {
		nlmsg_free(netlinkMessage);
		LOG_ERR("%s %d",
				NetlinkException::error_sending_netlink_message.c_str(),
				result);
		throw NetlinkException(
				NetlinkException::error_sending_netlink_message);
	}
	LOG_DBG("Sent message of %d bytes to %d", result,
			message->getDestPortId());

	//Cleanup
	nlmsg_free(netlinkMessage);
}

BaseNetlinkMessage * NetlinkManager::getMessage() throw (NetlinkException) {
	unsigned char *buf = NULL;
	struct nlmsghdr *hdr;
	struct genlmsghdr *nlhdr;
	struct sockaddr_nl nla = { 0 };
	struct nl_msg *msg = NULL;
	struct ucred *creds = NULL;

	int numBytes = nl_recv(socket, &nla, &buf, &creds);
	if (numBytes <= 0) {
		LOG_ERR("%s %d",
				NetlinkException::error_receiving_netlink_message.c_str(),
				numBytes);
		throw NetlinkException(
				NetlinkException::error_receiving_netlink_message);
	}

	LOG_DBG("Received %d bytes, parsing the message", numBytes);

	hdr = (struct nlmsghdr *) buf;
	nlhdr = (genlmsghdr *) nlmsg_data(hdr);
	msg = nlmsg_convert(hdr);
	if (!msg) {
		LOG_ERR("%s", NetlinkException::error_parsing_netlink_message.c_str());
		throw NetlinkException(
				NetlinkException::error_parsing_netlink_message);
	}

	nlmsg_set_src(msg, &nla);

	LOG_DBG("Netlink family %d", hdr->nlmsg_type);
	LOG_DBG("Version %d", nlhdr->version);
	LOG_DBG("Operation code %d", nlhdr->cmd);
	if (hdr->nlmsg_flags & NLM_F_REQUEST) {
		LOG_DBG("It is a request message");
	} else if (hdr->nlmsg_flags & NLMSG_ERROR) {
		LOG_DBG("It is an error message");
	}

	if (creds) {
		nlmsg_set_creds(msg, creds);
	}

	BaseNetlinkMessage * result = parseBaseNetlinkMessage(hdr);

	if (result == NULL) {
		nlmsg_free(msg);
		free(buf);
		free(creds);
		LOG_ERR("%s", NetlinkException::error_parsing_netlink_message.c_str());
		throw NetlinkException(
				NetlinkException::error_parsing_netlink_message);
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

