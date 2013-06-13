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
#include <netlink/msg.h>
#include <netlink/socket.h>
#include <netlink/attr.h>

namespace rina{
#define MY_MESSAGE_TYPE 0x11
#define RINA_PROCESS_NAME 1
#define RINA_PROCESS_INSTANCE 2
#define RINA_ENTITY_NAME 3
#define RINA_ENTITY_INSTANCE 4

enum {
		ATTR_PROCESS_NAME = 1,
		ATTR_PROCESS_INSTANCE,
		ATTR_ENTITY_NAME,
		ATTR_ENTITY_INSTANCE,
        __MY_ATTR_MAX,
};

#define MY_ATTR_MAX (__MY_ATTR_MAX - 1)


/* CLASS NETLINK MANAGER */

NetlinkManager::NetlinkManager(){
	this->localPort = getpid();
	LOG_DBG("Netlink Manager constructor called, using local port = %d", localPort);
	initialize();
}

NetlinkManager::NetlinkManager(unsigned int localPort){
	this->localPort = localPort;
	LOG_DBG("Netlink Manager constructor called, with netlink pid = %d", localPort);
	initialize();
}

NetlinkManager::~NetlinkManager(){
	LOG_DBG("Netlink Manager destructor called");
	nl_socket_free(socket);
}

void NetlinkManager::initialize(){
	socket = nl_socket_alloc();
	nl_socket_set_local_port(socket, localPort);
	int result = nl_connect(socket, NETLINK_GENERIC);
	if (result == 0){
		LOG_DBG("Netlink socket connected to local port %d ", nl_socket_get_local_port(socket));
	}else{
		LOG_DBG("Got error %d", result);
	}
}

void NetlinkManager::sendMessage(const ApplicationProcessNamingInformation& appName, unsigned int destination){
	//Generate the message
	struct nl_msg* netlinkMessage;

	netlinkMessage = nlmsg_alloc_simple(MY_MESSAGE_TYPE, NLM_F_REQUEST);
	NLA_PUT_STRING(netlinkMessage, RINA_PROCESS_NAME, appName.getProcessName().c_str());
	NLA_PUT_STRING(netlinkMessage, RINA_PROCESS_INSTANCE, appName.getProcessInstance().c_str());
	NLA_PUT_STRING(netlinkMessage, RINA_ENTITY_NAME, appName.getEntityName().c_str());
	NLA_PUT_STRING(netlinkMessage, RINA_ENTITY_INSTANCE, appName.getEntityInstance().c_str());

	//Set destination and send the message
	nl_socket_set_peer_port(socket, destination);
	nl_send_auto(socket, netlinkMessage);
	LOG_DBG("Sent message to %d", destination);

	//Cleanup
	nlmsg_free(netlinkMessage);
	return;

	nla_put_failure:
	        nlmsg_free(netlinkMessage);
}

ApplicationProcessNamingInformation *  NetlinkManager::getMessage(){
	unsigned char *buf = NULL;
	struct nlmsghdr *hdr;
	struct sockaddr_nl nla = {0};
	struct nl_msg *msg = NULL;
	struct ucred *creds = NULL;

	int n = nl_recv(socket, &nla, &buf, &creds);
	if (n <= 0){
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

	LOG_DBG("Received message from %d ", nla.nl_pid);
	LOG_DBG("Message type %d", hdr->nlmsg_type);
	LOG_DBG("Message sequence number %d", hdr->nlmsg_seq);
	if (hdr->nlmsg_flags & NLM_F_REQUEST) {
		LOG_DBG("It is a request message");
	}else if (hdr->nlmsg_flags & NLMSG_ERROR){
		LOG_DBG("It is an error message");
	}

	if (creds){
		nlmsg_set_creds(msg, creds);
	}

	/*
	 * The policy defines 4 attributes: 4 strings
	 */
	struct nla_policy attr_policy[MY_ATTR_MAX+1];
	attr_policy[ATTR_PROCESS_NAME].type = NLA_STRING;
	attr_policy[ATTR_PROCESS_NAME].minlen = 0;
	attr_policy[ATTR_PROCESS_NAME].maxlen = 65535;
	attr_policy[ATTR_PROCESS_INSTANCE].type = NLA_STRING;
	attr_policy[ATTR_PROCESS_INSTANCE].minlen = 0;
	attr_policy[ATTR_PROCESS_INSTANCE].maxlen = 65535;
	attr_policy[ATTR_ENTITY_NAME].type = NLA_STRING;
	attr_policy[ATTR_ENTITY_NAME].minlen = 0;
	attr_policy[ATTR_ENTITY_NAME].maxlen = 65535;
	attr_policy[ATTR_ENTITY_INSTANCE].type = NLA_STRING;
	attr_policy[ATTR_ENTITY_INSTANCE].minlen = 0;
	attr_policy[ATTR_ENTITY_INSTANCE].maxlen = 65535;
	struct nlattr *attrs[MY_ATTR_MAX+1];

	/*
	 * The nlmsg_parse() function will make sure that the message contains
	 * enough payload to hold the header (struct my_hdr), validates any
	 * attributes attached to the messages and stores a pointer to each
	 * attribute in the attrs[] array accessable by attribute type.
	 */
	int err = nlmsg_parse(hdr, 0, attrs, MY_ATTR_MAX, attr_policy);
	if (err < 0){
		 LOG_DBG("ERROR %d", err);
		 nlmsg_free(msg);
		 free(buf);
		 free(creds);
		 return NULL;
	 }

	ApplicationProcessNamingInformation * result = new ApplicationProcessNamingInformation();

	 if (attrs[ATTR_PROCESS_NAME]) {
		 /*
		  * It is safe to directly access the attribute payload without
		  * any further checks since nlmsg_parse() enforced the policy.
		  */
		 result->setProcessName(nla_get_string(attrs[ATTR_PROCESS_NAME]));
	 }

	 if (attrs[ATTR_PROCESS_INSTANCE]) {
		 result->setProcessInstance(nla_get_string(attrs[ATTR_PROCESS_INSTANCE]));
	 }

	 if (attrs[ATTR_ENTITY_NAME]) {
	 	result->setEntityName(nla_get_string(attrs[ATTR_ENTITY_NAME]));
	 }

	 if (attrs[ATTR_ENTITY_INSTANCE]) {
	 	 result->setEntityInstance(nla_get_string(attrs[ATTR_ENTITY_INSTANCE]));
	 }

	 nlmsg_free(msg);
	 free(buf);
	 free(creds);
	 return result;
}

}




