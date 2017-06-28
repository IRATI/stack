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
#include "librina/patterns.h"

#include "irati/kernel-msg.h"

#define WAIT_RESPONSE_TIMEOUT 10

namespace rina {

char * stringToCharArray(std::string s);
char * intToCharArray(int i);

/**
 * Contains the information to identify an IRATI ep
 * ctrl port ID and IPC Process id
 */
struct irati_ep {
	irati_msg_port_t ctrl_port;
	ipc_process_id_t ipcp_id;
	ApplicationProcessNamingInformation app_name;
};

/**
 * Contains mappings of application process name to netlink portId,
 * or IPC process id to netlink portId.
 */
class CtrlPortIdMap {

	/** Stores the mappings of IPC Process id to nelink portId */
	std::map<ipc_process_id_t, struct irati_ep *> ipcp_id_map;

	/** Stores the mappings of application process name to netlink port id */
	std::map<std::string, struct irati_ep *> app_name_map;

public:
	~CtrlPortIdMap();

	void add_ipcp_id_to_ctrl_port_mapping(irati_msg_port_t ctrl_port,
					      ipc_process_id_t ipcp_id);
	struct irati_ep * get_ctrl_port_from_ipcp_id(ipc_process_id_t ipcp_id);
	void add_app_name_to_ctrl_port_map(const ApplicationProcessNamingInformation& app_name,
					   irati_msg_port_t ctrl_port,
					   ipc_process_id_t ipcp_id);
	struct irati_ep * get_ctrl_port_from_app_name(const ApplicationProcessNamingInformation& app_name);
	irati_msg_port_t get_ipcm_ctrl_port();
	static void name_to_app_name_class(const struct name * name,
					   ApplicationProcessNamingInformation & app_name);

	/**
	 * Poulates the "destPortId" field for messages that have to be sent,
	 * or updates the mappings for received messages
	 * @param message
	 * @param sent
	 */
	int update_msg_or_pid_map(struct irati_msg_base * msg, bool send);

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

	/** Keeps the mappings between control port ids and app/dif names */
	CtrlPortIdMap ctrl_pid_map;

	/** Linear sequence number generator */
	unsigned int next_seq_number;

	unsigned int get_next_seq_number();

	IPCEvent * irati_ctrl_msg_to_ipc_event(struct irati_msg_base *msg);

public:
	IRATICtrlManager();
	~IRATICtrlManager();

	void initialize(void);

	irati_msg_port_t get_irati_ctrl_port(void);
	void set_irati_ctrl_port(irati_msg_port_t ctrl_port);

	int get_ctrl_fd(void);

	/** Sends a message of default maximum size (PAGE SIZE) */
	int send_msg(struct irati_msg_base *msg, bool fill_seq_num);

	/** Sends a control message of specified maximum size */
	int send_msg_max_size(struct irati_msg_base *msg,
			      size_t maxSize, bool fill_seq_num);

	IPCEvent * get_next_ctrl_msg();

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
