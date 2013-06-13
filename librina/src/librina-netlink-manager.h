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

#ifndef LIBRINA_NETLINK_MANAGER_H
#define LIBRINA_NETLINK_MANAGER_H

#include "exceptions.h"
#include "patterns.h"
#include "librina-common.h"
#include <netlink/netlink.h>

namespace rina{

/**
 * Manages the creation, destruction and usage of a Netlink socket with the OS
 * process PID. The socket will be utilized by the OS process using RINA to
 * communicate with other OS processes in user space or the Kernel.
 */
class NetlinkManager{

	/** The address where the netlink socket will be bound */
	unsigned int localPort;

	/** The netlink socket structure */
	nl_sock *socket;

	/** Creates the Netlink socket and binds it to the netlinkPid */
	void initialize();
public:
	/**
	 * Creates an instance of a Netlink socket and binds it to the local port
	 * whose number is the same as the OS process PID
	 */
	NetlinkManager();

	/**
	 * Creates an instance of a Netlink socket and binds it to the specified
	 * local port
	 */
	NetlinkManager(unsigned int localPort);

	/**
	 * Closes the Netlink socket
	 */
	~NetlinkManager();

	void sendMessage(const ApplicationProcessNamingInformation& appName, unsigned int destination);

	ApplicationProcessNamingInformation *  getMessage();
};
}

#endif /* LIBRINA_NETLINK_MANAGER_H_ */
