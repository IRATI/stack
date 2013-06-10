/*
 *  User space echo netlink application test 
 *
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
 *
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
 * You should have received a copy of the GNU General Public
 * License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <netlink/netlink.h>
#include <netlink/msg.h>
#include <netlink/socket.h>
#include <linux/netlink.h>
#include <linux/genetlink.h>

enum {
	NETLINL_RINA_A_UNSPEC,
	NETLINK_RINA_A_MSG,
	__NETLINK_RINA_A_MAX,
};

enum {
	NETLINK_RINA_C_UNSPEC,
	NETLINK_RINA_C_ECHO,
	__NETLINK_RINA_C_MAX,
};

int main()
{
	struct nl_sock *sock;
	struct nl_msg *msg;
	int family, res;
	sock = nl_socket_alloc();
	genl_connect(sock);
	family = genl_ctrl_resolve(sock, "NETLINK_RINA");
	msg = nlmsg_alloc();
	genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, family, 0, NLM_F_ECHO, NETLINK_RINA_C_ECHO, 1);
	nla_put_string(msg, NETLINK_RINA_A_MSG, "Testing message");
	/*original code was using  nl_send_auto_complete() but it is deprecated*/
	nl_send_auto(sock, msg);
	nlmsg_free(msg);
	res = nl_recvmsgs_default(sock);
	printf("After receive %i.\n", res);

        exit(0);
}
