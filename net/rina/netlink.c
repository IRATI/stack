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

#define RINA_PREFIX "netlink"

#include "logs.h"
#include "netlink.h"

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
static int nl_rina_echo(struct sk_buff *skb_in, struct genl_info *info)
{
	/* message handling code goes here; return 0 on success, negative
	* values on failure */
	int ret;
	struct nlattr *na;
	struct sk_buff *skb;
	void *msg_head;
	char * mydata;
	skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);

	LOG_DBG("ECHOING MESSAGE");

	if (info == NULL){
		LOG_DBG("info input parameter is NULL");
		return -1;
	}

	na = info->attrs[NETLINK_RINA_A_MSG];
	if (na){
		mydata = (char *)nla_data(na);
			if (mydata == NULL){
				LOG_DBG("error while receiving data\n");
				return -1;
			}
			else
				LOG_DBG("received: %s\n", mydata);
		}
	else{
		LOG_DBG("no info->attrs %i\n", NETLINK_RINA_A_MSG);
		return -1;
	}

	if (skb == NULL){
		LOG_DBG("COULD NOT ALLOCATE sk_buff");
		return -1;
	}

	msg_head = genlmsg_put(skb, 0, info->snd_seq+1, &nl_rina_family, 0, NETLINK_RINA_C_ECHO);
	if (msg_head == NULL) {
		ret = -ENOMEM;
		return -1;
	}

	ret = nla_put_string(skb, NETLINK_RINA_A_MSG, "hello world from kernel space");
	if (ret!= 0){
		LOG_DBG("Could not add string message to echo");
		return -1;
	}

	genlmsg_end(skb, msg_head);

	ret = genlmsg_unicast(sock_net(skb->sk),skb,info->snd_portid);
	if (ret != 0){
		LOG_DBG("COULD NOT SEND BACK UNICAST MESSAGE");
		return -1;
	}

	return 0;

}

/* operation definition */
static struct genl_ops nl_rina_ops_echo = {
	.cmd = NETLINK_RINA_C_ECHO,
	.flags = 0,
	.policy = nl_rina_policy,
	.doit = nl_rina_echo,
	.dumpit = NULL,
};

int rina_netlink_init(void)
{
	int ret;
	
	LOG_FBEGN;
	
	ret = genl_register_family(&nl_rina_family);
	if (ret != 0){
		LOG_DBG("Not able to register nl_rina_family");
		return -1;
	}
	
	ret = genl_register_ops(&nl_rina_family, &nl_rina_ops_echo);
	if (ret != 0){
		LOG_DBG("Not able to register nl_rina_ops");
		genl_unregister_family(&nl_rina_family);
		return -2;
	}

        LOG_DBG("NetLink layer initialized");
        LOG_FEXIT;

        return 0;
}

void rina_netlink_exit(void)
{
	int ret;
        
	LOG_FBEGN;

	/*unregister the functions*/
	ret = genl_unregister_ops(&nl_rina_family, &nl_rina_ops_echo);

	if(ret != 0){
		LOG_DBG("unregister ops: %i\n",ret);
		return;
	}
	
	/*unregister the family*/
	ret = genl_unregister_family(&nl_rina_family);
	
	if(ret !=0){
		LOG_DBG("unregister family %i\n",ret);
	}
        LOG_DBG("NetLink layer finalized");

	LOG_FEXIT;
}

