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

#include <linux/hashtable.h>

#define RINA_PREFIX "netlink"

#include "logs.h"
#include "netlink.h"

/* Attribute policy */
//static struct nla_policy nl_rina_policy[NETLINK_RINA_A_MAX + 1] = { 
//	[NETLINK_RINA_A_MSG] = { .type = NLA_NUL_STRING },
//};

/* Family definition */
static struct genl_family nl_rina_family = { 
	.id      = GENL_ID_GENERATE,
	.hdrsize = 0,
	.name    = NETLINK_RINA,
	.version = 1,
	.maxattr = NETLINK_RINA_A_MAX,
};

/*  Table to collect callbacks */
typedef int (* message_handler_t)(struct sk_buff *, struct genl_info *);

/* FIXME: this forward declaration should be removed */
int  (* get_handler(int))(struct sk_buff *, struct genl_info *);

/* Handler */
static int nl_rina_echo(struct sk_buff *skb_in, struct genl_info *info)
{
	/*
         * Message handling code goes here; return 0 on success, negative
         * values on failure
         */

	int ret;
	struct nlattr *na;
	struct sk_buff *skb;
	void *msg_head;
	char * mydata;

	LOG_DBG("ECHOING MESSAGE");
	skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
	
	if (skb == NULL) {
		LOG_DBG("skb buffer is NULL");
		return -1;
	}

	if (info == NULL) {
		LOG_DBG("info input parameter is NULL");
		return -1;
	}

	na = info->attrs[NETLINK_RINA_A_MSG];
	if (na) {
		mydata = (char *)nla_data(na);
		if (mydata == NULL) {
			LOG_DBG("error while receiving data\n");
			return -1;
		} else
			LOG_DBG("received: %s\n", mydata);
        }
	else {
		LOG_DBG("no info->attrs %i\n", NETLINK_RINA_A_MSG);
		return -1;
	}

	if (skb == NULL) {
		LOG_DBG("COULD NOT ALLOCATE sk_buff");
		return -1;
	}
	LOG_DBG("seq number received: %d\n",info->snd_seq);
	msg_head = genlmsg_put(skb, 0, info->snd_seq, &nl_rina_family, 0,
                               RINA_C_APP_ALLOCATE_FLOW_REQUEST);
	if (msg_head == NULL) {
		ret = -ENOMEM;
		return -1;
	} else {
		LOG_DBG("msg head CREATED");
	}
	ret = nla_put_string(skb, NETLINK_RINA_A_MSG,
                             "hello world from kernel space");
	if (ret!= 0) {
		LOG_DBG("Could not add string message to echo");
		return -1;
	} else {
		LOG_DBG("nla_put_string OK");
	}

	genlmsg_end(skb, msg_head);
	LOG_DBG("genlmsg_end OK");

	/* ret = genlmsg_unicast(sock_net(skb->sk),skb,info->snd_portid); */
	ret = genlmsg_unicast(&init_net,skb,info->snd_portid);
	if (ret != 0) {
		LOG_DBG("COULD NOT SEND BACK UNICAST MESSAGE");
		return -1;
	}

        LOG_DBG("genkmsg_unicast OK");

	return 0;
}

/////* operation definition */
//static struct genl_ops nl_rina_ops_echo = {
//	.cmd = NETLINK_RINA_C_ECHO,
//	.flags = 0,
//	//.policy = nl_rina_policy,
//	.doit = nl_dispatcher,
//	.dumpit = NULL,
//};

/* dispatcher */
static int nl_dispatcher(struct sk_buff *skb_in, struct genl_info *info)
{
	/* message handling code goes here; return 0 on success, negative
         * values on failure */
	int (*cb_function)(struct sk_buff *, struct genl_info *);

	LOG_DBG("Dispatching message");
	
	if (info == NULL) {
		LOG_DBG("info input parameter is NULL");
		return -1;
	}

	cb_function = get_handler(info->nlhdr->nlmsg_type);
	if (cb_function == NULL)
		return -1;

        return cb_function(skb_in, info);
}

/* operation definition */
static struct genl_ops nl_rina_ops[] = {
	{
		.cmd = RINA_C_APP_ALLOCATE_FLOW_REQUEST,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_APP_ALLOCATE_FLOW_REQUEST_RESULT,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_APP_ALLOCATE_FLOW_REQUEST_ARRIVED,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_APP_ALLOCATE_FLOW_RESPONSE,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_APP_DEALLOCATE_FLOW_REQUEST,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_APP_DEALLOCATE_FLOW_RESPONSE,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_APP_FLOW_DEALLOCATED_NOTIFICATION,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_APP_REGISTER_APPLICATION_REQUEST,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_APP_REGISTER_APPLICATION_RESPONSE,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_APP_UNREGISTER_APPLICATION_REQUEST,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_APP_UNREGISTER_APPLICATION_RESPONSE,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_APP_APPLICATION_REGISTRATION_CANCELED_NOTIFICATION,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_APP_GET_DIF_PROPERTIES_REQUEST,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_APP_GET_DIF_PROPERTIES_RESPONSE,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_IPCM_IPC_PROCESS_REGISTERED_TO_DIF_NOTIFICATION,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_IPCM_IPC_PROCESS_UNREGISTERED_FROM_DIF_NOTIFICATION,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_IPCM_ENROLL_TO_DIF_REQUEST,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_IPCM_ENROLL_TO_DIF_RESPONSE,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_REQUEST,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_RESPONSE,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_IPCM_ALLOCATE_FLOW_REQUEST,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_IPCM_QUERY_RIB_REQUEST,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_IPCM_QUERY_RIB_RESPONSE,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_RMT_ADD_FTE_REQUEST,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_RMT_DELETE_FTE_REQUEST,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_RMT_DUMP_FT_REQUEST,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
	{
		.cmd = RINA_C_RMT_DUMP_FT_REPLY,
		.flags = 0,
		//.policy = nl_rina_policy,
		.doit = nl_dispatcher,
		.dumpit = NULL,
	},
};

/* Table to collect callbacks */
message_handler_t  message_handler_reg[NETLINK_RINA_C_MAX];

int register_handler(int m_type,
                     int (*handler)(struct sk_buff *, struct genl_info *))
{

	if (m_type < 0 || 
            m_type >= NETLINK_RINA_C_MAX || 
            message_handler_reg[m_type] != NULL)
		return -1;
        else
		message_handler_reg[m_type] = (message_handler_t) handler;

	return 0;
}

int (* get_handler(int m_type))(struct sk_buff *, struct genl_info *)
{

	if (m_type < 0 || 
            m_type >= NETLINK_RINA_C_MAX ||
            message_handler_reg[m_type] == NULL)
		return NULL;
	else
		return message_handler_reg[m_type];
}

int unregister_handler(int m_type)
{
	if (m_type < 0 || m_type >= NETLINK_RINA_C_MAX)
		return -1;

	message_handler_reg[m_type] = NULL;

	return 0;
}

int rina_netlink_init(void)
{
	int ret, i;
	
	LOG_FBEGN;
	
	LOG_DBG("NETLINK_RINA_C_MAX = %d", NETLINK_RINA_C_MAX);

	ret = genl_register_family(&nl_rina_family);
	if (ret != 0) {
		LOG_DBG("Not able to register nl_rina_family");
		return -1;
	}

	for (i=0; i < ARRAY_SIZE(nl_rina_ops); i++) {
		ret = genl_register_ops(&nl_rina_family, &nl_rina_ops[i]);
		if (ret < 0) {
			LOG_DBG("Not able to register nl_rina_ops %d", ret);
			genl_unregister_family(&nl_rina_family);
			return -2;
		}
	}

        LOG_DBG("NetLink layer initialized");

	/*  TODO: Testing */
	register_handler(RINA_C_APP_ALLOCATE_FLOW_REQUEST, nl_rina_echo); 
        LOG_DBG("nl_rina echo registered for "
                "RINA_C_APP_ALLOCATE_FLOW_REQUEST");

        LOG_FEXIT;

        return 0;
}

void rina_netlink_exit(void)
{
	int ret, i;
        
	LOG_FBEGN;

	/* Unregister the functions*/
	for (i=0; i < ARRAY_SIZE(nl_rina_ops); i++) {
		ret = genl_unregister_ops(&nl_rina_family, &nl_rina_ops[i]);
		if(ret < 0) {
			LOG_DBG("unregister ops: %i\n",ret);
			return;
		}
	}
	
	/* Unregister the family */
	ret = genl_unregister_family(&nl_rina_family);
        if(ret != 0) {
		LOG_DBG("unregister family %i\n", ret);
	}

        LOG_DBG("nl_rina echo unregistered for "
                "RINA_C_APP_ALLOCATE_FLOW_REQUEST");

        LOG_DBG("NetLink layer finalized");

	LOG_FEXIT;
}
