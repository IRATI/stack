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
#include <netlink/netlink.h>
#include <netlink/msg.h>

namespace rina{


/* CLASS NETLINK MANAGER */

NetlinkManager::NetlinkManager(){
	this->netlinkPid = getpid();
	LOG_DBG("Netlink Manager constructor called, using netlink pid = %d", netlinkPid);
	initialize();
}

NetlinkManager::NetlinkManager(unsigned int netlinkPid){
	this->netlinkPid = netlinkPid;
	LOG_DBG("Netlink Manager constructor called, with netlink pid = %d", netlinkPid);
	initialize();
}

NetlinkManager::~NetlinkManager(){
	LOG_DBG("Netlink Manager destructor called");
	nl_socket_free(socket);
}

void NetlinkManager::initialize(){
	socket = nl_socket_alloc();
	nl_socket_set_local_port(socket, netlinkPid);
	int result = nl_connect(socket, NETLINK_ROUTE);
	if (result == 0){
		LOG_DBG("Netlink socket connected");
	}else{
		LOG_DBG("Got error %d", result);
	}
}

void NetlinkManager::sendMessage(const std::string& message, unsigned int destination){
	struct nl_msg* netlinkMessage;
	netlinkMessage = nlmsg_alloc();


	nlmsg_free(netlinkMessage);
}

std::string NetlinkManager::getMessage(){
	return "message";
}

}




