/*
 * Core librina logic
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

#ifndef LIBRINA_CORE_H
#define LIBRINA_CORE_H

#ifdef __cplusplus

#include <map>

#include "librina/concurrency.h"
#include "librina/common.h"

#include "irati/kernel-msg.h"

#define WAIT_RESPONSE_TIMEOUT 10

namespace rina {

char * stringToCharArray(std::string s);
char * intToCharArray(int i);

/**
 * Main class of libRINA. Initializes the NetlinkManager,
 * the eventsQueue, and the NetlinkMessageReader thread.
 */
class IRATICtrlManager {

	/** The control file descriptor */
	int cfd;

	/** The local port of the file descriptor */
	irati_msg_port_t ctrl_port;

	Lockable irati_ctr_lock;

	/** The lock for send/receive operations */
	Lockable sendReceiveLock;

	/** Linear sequence number generator */
	unsigned int next_seq_number;

	unsigned int get_next_seq_number();

public:
	IRATICtrlManager();
	~IRATICtrlManager();

	void initialize(void);

	irati_msg_port_t get_irati_ctrl_port(void);
	void set_irati_ctrl_port(irati_msg_port_t ctrl_port);

	int get_ctrl_fd(void);

	/** Sends a message of default maximum size (PAGE SIZE) */
	int send_msg(struct irati_msg_base *msg, bool fill_seq_num);

	IPCEvent * get_next_ctrl_msg();

	static IPCEvent * irati_ctrl_msg_to_ipc_event(struct irati_msg_base *msg);

	/**
	 * Notify about the reception of a Netlink message
	 */
	void ctrl_msg_arrived(struct irati_msg_base *msg);

	/**
	 * An OS Process has finalized. Retrieve the information associated to
	 * the NL port-id (application name, IPC Process id if it is IPC process),
	 * and return it in the form of an OSProcessFinalized event
	 * @param nl_portid
	 * @return
	 */
	IPCEvent * os_process_finalized(irati_msg_port_t ctrl_port);
};

/**
 * Make RINAManager singleton
 */
extern Singleton<IRATICtrlManager> irati_ctrl_mgr;

}

#endif

#endif
