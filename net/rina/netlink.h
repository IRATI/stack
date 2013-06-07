/*
 * NetLink support
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef RINA_NETLINK_SUPPORT
#define RINA_NETLINK_SUPPORT

#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <net/netlink.h>
#include <linux/genetlink.h>
#include <net/genetlink.h>
#include <linux/skbuff.h>

#define NETLINK_RINA 31

/* attributes */
enum {
	NETLINL_RINA_A_UNSPEC,
	NETLINK_RINA_A_MSG,
	__NETLINK_RINA_A_MAX,
};

#define NETLINK_RINA_A_MAX (__NETLINK_RINA_A_MAX - 1)

/* attribute policy */
static struct nla_policy nl_rina_policy[NETLINK_RINA_A_MAX + 1] = { 
	[NETLINK_RINA_A_MSG] = { .type = NLA_NUL_STRING },
};

/* family definition */
static struct genl_family nl_rina_family = { 
	.id = GENL_ID_GENERATE,
	.hdrsize = 0,
	.name = "NETLINK_RINA",
	.version = 1,
	.maxattr = NETLINK_RINA_A_MAX,
};

/* handler */
static int nl_rina_echo(struct sk_buff *skb, struct genl_info *info);

/* commands */
enum {
	NETLINK_RINA_C_UNSPEC,
	NETLINK_RINA_C_ECHO,
	__NETLINK_RINA_C_MAX,
};

#define NETLINK_RINA_C_MAX (__NETLINK_RINA_C_MAX - 1)
/* operation definition */
static struct genl_ops nl_rina_ops_echo = {
	.cmd = NETLINK_RINA_C_ECHO,
	.flags = 0,
	.policy = nl_rina_policy,
	.doit = nl_rina_echo,
	.dumpit = NULL,
};


int  rina_netlink_init(void);
void rina_netlink_exit(void);

#endif
