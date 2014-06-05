//
// Netlink manager
//
//    Eduard Grasa          <eduard.grasa@i2cat.net>
//    Francesco Salvestrini <f.salvestrini@nextworks.it>
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
		Exception(description.c_str()) {
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
const std::string NetlinkException::error_fetching_netlink_session =
		"Error fetching Netlink session";
const std::string
	NetlinkException::error_fetching_pending_netlink_request_message =
		"Error fetching pending Netlink request message";
const std::string NetlinkException::error_fetching_netlink_port_id =
		"Error fetching Netlink port id";
const std::string
	NetlinkException::unrecognized_generic_netlink_operation_code =
		"Unrecognized Generic Netlink operation code";
const std::string NetlinkException::error_waiting_for_response =
		"Time out waiting for response message";

/* CLASS NETLINK MANAGER */

NetlinkManager::NetlinkManager() throw (NetlinkException) {
	this->localPort = getpid();
	LOG_DBG("Netlink Manager constructor called, using local port = %d",
			localPort);
	initialize(false);
}

NetlinkManager::NetlinkManager(unsigned int localPort)
		throw (NetlinkException) {
	this->localPort = localPort;
	LOG_DBG("Netlink Manager constructor called, with netlink pid = %d",
			localPort);
	initialize(true);
}

NetlinkManager::~NetlinkManager() {
	LOG_DBG("Netlink Manager destructor called");
	nl_socket_free(socket);
}

void NetlinkManager::initialize(bool ipcManager) throw (NetlinkException) {
        int result = 0;

	socket = nl_socket_alloc();
	nl_socket_set_local_port(socket, localPort);
	nl_socket_disable_seq_check(socket);
	nl_socket_disable_auto_ack(socket);
	if (ipcManager) {
	        result = nl_socket_set_msg_buf_size(socket, 5*4096);
	        if (result != 0) {
	                LOG_CRIT("Could not allocate enough memory to receive msg buffer: %d"
	                                , result);
	                throw NetlinkException(NetlinkException::error_connecting_netlink_socket);
	        }
	}

	result = genl_connect(socket);
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

void NetlinkManager::_sendMessage(BaseNetlinkMessage * message, struct nl_msg* netlinkMessage)
throw(NetlinkException) {
        struct rinaHeader* myHeader;

        message->setSourcePortId(localPort);
        message->setFamily(family);
        if (!netlinkMessage){
                LOG_ERR("%s",
                                NetlinkException::error_allocating_netlink_message.c_str());
                throw NetlinkException(
                                NetlinkException::error_allocating_netlink_message);
        }

        int flags = 0;
        //FIXME, apparently messages directed to the kernel without the
        //NLM_F_REQUEST flag set don't reach its destination
        if (message->isRequestMessage() || message->getDestPortId() == 0){
                flags = NLM_F_REQUEST;
        }

        myHeader = (rinaHeader*) genlmsg_put(netlinkMessage, localPort,
                        message->getSequenceNumber(),family, sizeof(struct rinaHeader),
                        flags, message->getOperationCode(),
                        RINA_GENERIC_NETLINK_FAMILY_VERSION);
        if (!myHeader){
                nlmsg_free(netlinkMessage);
                LOG_ERR("%s",
                                NetlinkException::error_generating_netlink_message.c_str());
                throw NetlinkException(
                                NetlinkException::error_generating_netlink_message);
        }
        myHeader->sourceIPCProcessId = message->getSourceIpcProcessId();
        myHeader->destIPCProcessId = message->getDestIpcProcessId();

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
        result = nl_send(socket, netlinkMessage);
        if (result < 0) {
                nlmsg_free(netlinkMessage);
                LOG_ERR("%s %d %d",
                                NetlinkException::error_sending_netlink_message.c_str(),
                                result, message->getDestPortId());
                throw NetlinkException(
                                NetlinkException::error_sending_netlink_message);
        }
        LOG_DBG("Sent message of %d bytes. %s", result,
                        message->toString().c_str());
        //Cleanup
        nlmsg_free(netlinkMessage);
}

unsigned int NetlinkManager::getLocalPort(){
	return localPort;
}

unsigned int NetlinkManager::getSequenceNumber(){
	return nl_socket_use_seq(socket);
}

void NetlinkManager::sendMessage(BaseNetlinkMessage * message)
throw (NetlinkException) {
        struct nl_msg* netlinkMessage;

        netlinkMessage = nlmsg_alloc();
        if (!netlinkMessage) {
                throw NetlinkException("Error allocating Netlink Message structure");
        }

        _sendMessage(message, netlinkMessage);
}

void NetlinkManager::sendMessageOfMaxSize(BaseNetlinkMessage * message, size_t maxSize)
throw(NetlinkException) {
        struct nl_msg* netlinkMessage;

        netlinkMessage = nlmsg_alloc_size(maxSize);
        if (!netlinkMessage) {
                throw NetlinkException("Error allocating Netlink Message structure");
        }

        _sendMessage(message, netlinkMessage);
}

BaseNetlinkMessage * NetlinkManager::getMessage() throw (NetlinkException) {
	unsigned char *buf = NULL;
	struct nlmsghdr *hdr;
	struct genlmsghdr *nlhdr;
	struct rinaHeader * myHeader;
	struct sockaddr_nl nla;
	struct nl_msg *msg = NULL;
	struct ucred *creds = NULL;
	int numBytes;

        memset(&nla, 0, sizeof(nla));
        numBytes = nl_recv(socket, &nla, &buf, &creds);
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

	LOG_DBG("Source: %d, Netlink family: %d; Version: %d; Operation code: %d; Flags: %d; Sequence number: %d",
			nla.nl_pid, hdr->nlmsg_type, nlhdr->version, nlhdr->cmd,
			hdr->nlmsg_flags, hdr->nlmsg_seq);

	if (creds) {
		nlmsg_set_creds(msg, creds);
	}

	myHeader = (rinaHeader*) genlmsg_data(nlhdr);
	if(!myHeader){
		nlmsg_free(msg);
		free(buf);
		free(creds);
		LOG_ERR("%s", NetlinkException::error_parsing_netlink_message.c_str());
		throw NetlinkException(
				NetlinkException::error_parsing_netlink_message);
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

	if(hdr->nlmsg_seq == 0){
		result->setNotificationMessage(true);
	}else if (hdr->nlmsg_flags & NLM_F_REQUEST) {
		result->setRequestMessage(true);
	}else if (hdr->nlmsg_flags & NLMSG_ERROR) {
		LOG_ERR("It is an error message");
		//TODO, decide what to do
	}else{
		result->setResponseMessage(true);
	}

	result->setFamily(family);
	result->setDestPortId(localPort);
	result->setSourcePortId(nla.nl_pid);
	result->setSequenceNumber(hdr->nlmsg_seq);
	result->setSourceIpcProcessId(myHeader->sourceIPCProcessId);
	result->setDestIpcProcessId(myHeader->destIPCProcessId);

	nlmsg_free(msg);
	free(buf);
	free(creds);
	return result;
}

}

