/*
 * Listens for process events, to detect
 *
 *    Eduard Grasa <eduard.grasa@i2cat.net>
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

#include <errno.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <linux/netlink.h>
#include <linux/connector.h>
#include <linux/cn_proc.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define RINA_PREFIX     "ipcm.os-process-listener"
#include <librina/logs.h>

#include "configuration.h"
#include "ipcm.h"
#include "process-event-listener.h"

using namespace std;

namespace rinad {

//Class DIFConfigFolderMonitor
OSProcessMonitor::OSProcessMonitor()
		: rina::SimpleThread(std::string("os-process-monitor"), false)
{
	stop = false;
	nl_sock = 0;
}

OSProcessMonitor::~OSProcessMonitor() throw()
{
}

void OSProcessMonitor::do_stop()
{
	rina::ScopedLock g(lock);
	stop = true;
}

bool OSProcessMonitor::has_to_stop()
{
	rina::ScopedLock g(lock);
	return stop;
}

int OSProcessMonitor::process_events()
{
	int rc;
	pid_t pid;
	struct __attribute__ ((aligned(NLMSG_ALIGNTO))) {
	    struct nlmsghdr nl_hdr;
	    struct __attribute__ ((__packed__)) {
	    struct cn_msg cn_msg;
	    struct proc_event proc_ev;
	    };
	} nlcn_msg;

	rc = recv(nl_sock, &nlcn_msg, sizeof(nlcn_msg), 0);

	if (rc == 0) {
		/* shutdown? */
		return 0;
	} else if (rc == -1) {
		if (errno == EINTR) return 0;
		return -1;
	}

	if (nlcn_msg.proc_ev.what == 0x80000000) {
		pid = nlcn_msg.proc_ev.event_data.exit.process_pid;
		IPCManager->os_process_finalized_handler(pid);
	}

	return 0;
}

/*
 * connect to netlink
 * returns netlink socket, or -1 on error
 */
int OSProcessMonitor::nl_connect()
{
	int rc;
	int nl_sock;
	struct sockaddr_nl sa_nl;

	nl_sock = socket(PF_NETLINK, SOCK_DGRAM | SOCK_NONBLOCK, NETLINK_CONNECTOR);
	if (nl_sock == -1) {
		perror("socket");
		return -1;
	}

	sa_nl.nl_family = AF_NETLINK;
	sa_nl.nl_groups = CN_IDX_PROC;
	sa_nl.nl_pid = getpid();

	rc = bind(nl_sock, (struct sockaddr *)&sa_nl, sizeof(sa_nl));
	if (rc == -1) {
		perror("bind");
		close(nl_sock);
		return -1;
	}

	return nl_sock;
}

/*
 * subscribe on proc events (process notifications)
 */
int OSProcessMonitor::set_proc_ev_listen(int nl_sock, bool enable)
{
	int rc;
	struct __attribute__ ((aligned(NLMSG_ALIGNTO))) {
		struct nlmsghdr nl_hdr;
		struct __attribute__ ((__packed__)) {
			struct cn_msg cn_msg;
			enum proc_cn_mcast_op cn_mcast;
		};
	} nlcn_msg;

	memset(&nlcn_msg, 0, sizeof(nlcn_msg));
	nlcn_msg.nl_hdr.nlmsg_len = sizeof(nlcn_msg);
	nlcn_msg.nl_hdr.nlmsg_pid = getpid();
	nlcn_msg.nl_hdr.nlmsg_type = NLMSG_DONE;

	nlcn_msg.cn_msg.id.idx = CN_IDX_PROC;
	nlcn_msg.cn_msg.id.val = CN_VAL_PROC;
	nlcn_msg.cn_msg.len = sizeof(enum proc_cn_mcast_op);

	nlcn_msg.cn_mcast = enable ? PROC_CN_MCAST_LISTEN : PROC_CN_MCAST_IGNORE;

	rc = send(nl_sock, &nlcn_msg, sizeof(nlcn_msg), 0);
	if (rc == -1) {
		perror("netlink send");
		return -1;
	}

	return 0;
}

int OSProcessMonitor::run()
{
	int wd;
	int pollnum;
	struct pollfd fds[1];

	LOG_DBG("OS Process monitor started, opening NL socket");

	nl_sock = nl_connect();
	if (nl_sock == -1) {
		LOG_ERR("Error initializing NL socket, "
				"stopping OS process monitor");
		return -1;
	}

	if (set_proc_ev_listen(nl_sock, true) != 0) {
		LOG_ERR("Error subscribing to OS process events,"
				" stopping OS process monitor");
		close(nl_sock);
		return -1;
	}

	fds[0].fd = nl_sock;
	fds[0].events = POLLIN;

	while (!has_to_stop()) {
		pollnum = poll(fds, 1, 500);

		if (pollnum == EINVAL) {
			LOG_ERR("Poll returned EINVAL, stopping OS process monitor");
			set_proc_ev_listen(nl_sock, false);
		        close(nl_sock);
			return -1;
		}

		if (pollnum <= 0) {
			//No changes during this period or error that can be ignored
			continue;
		}

		if (fds[0].revents & POLLIN) {
			if (process_events() != 0) {
				LOG_ERR("Process events returned error, "
						"stopping OS process monitor");
				set_proc_ev_listen(nl_sock, false);
			        close(nl_sock);
				return -1;
			}
		}
	}

        /* Close NL socket */
	set_proc_ev_listen(nl_sock, false);
        close(nl_sock);

        LOG_DBG("OS Process Monitor stopped");

	return 0;
}

} //namespace rinad
