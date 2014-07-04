/*
 * Netlink manager
 *
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#ifndef LIBRINA_NETLINK_MANAGER_H
#define LIBRINA_NETLINK_MANAGER_H

#ifdef __cplusplus

#include <netlink/netlink.h>

#include "librina/exceptions.h"
#include "librina/patterns.h"

#include "netlink-messages.h"

#define RINA_GENERIC_NETLINK_FAMILY_NAME "rina"
#define RINA_GENERIC_NETLINK_FAMILY_VERSION 1

namespace rina{

/**
 * NetlinkException
 */
class NetlinkException: public Exception {
public:
	NetlinkException(const std::string& description);
	static const std::string error_resolving_netlink_family;
	static const std::string error_connecting_netlink_socket;
	static const std::string error_allocating_netlink_message;
	static const std::string error_receiving_netlink_message;
	static const std::string error_generating_netlink_message;
	static const std::string error_sending_netlink_message;
	static const std::string error_parsing_netlink_message;
	static const std::string error_fetching_netlink_session;
	static const std::string error_fetching_pending_netlink_request_message;
	static const std::string error_fetching_netlink_port_id;
	static const std::string unrecognized_generic_netlink_operation_code;
	static const std::string error_waiting_for_response;
};

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

	/** The numeric value of the Generic RINA Netlink family */
	int family;

	/** Creates the Netlink socket and binds it to the netlinkPid */
	void initialize(bool ipcManager);

	/** Send a Netlink message */
	void _sendMessage(BaseNetlinkMessage * message, struct nl_msg* netlinkMessage);

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

	/**
	 * Returns the port where the Netlink Manager is listening
	 * @return
	 */
	unsigned int getLocalPort();

	/**
	 * Get the next available sequence number
	 */
	unsigned int getSequenceNumber();

	void sendMessage(BaseNetlinkMessage * message);

	void sendMessageOfMaxSize(BaseNetlinkMessage * message, size_t maxSize);

	BaseNetlinkMessage *  getMessage();
};

}

#endif

#endif
