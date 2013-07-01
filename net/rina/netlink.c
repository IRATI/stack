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

#ifndef RINA_NETLINK_H
#define RINA_NETLINK_H

#include <linux/list.h>

/* FIXME: May be modified when dif_type_t is updated */
#include <"kipcm.h">

#define RINA_PREFIX "netlink"

#include "logs.h"
#include "netlink.h"

/* Table to collect callbacks lists by message type*/
list_head  message_handler_registry[NETLINK_RINA_C_MAX];

/*  Type to store callback funcion */
typedef int (* message_handler_t)(struct sk_buff *, struct genl_info *);

/* struct to use in list */
struct callback_t {
	struct list_head cb_list;
	message_handler_t cb;
}

/* Family definition */
static struct genl_family nl_rina_family = {
        .id      = GENL_ID_GENERATE,
        .hdrsize = 0,
        .name    = NETLINK_RINA,
        .version = 1,
        .maxattr = NETLINK_RINA_A_MAX,
};




int register_handler(int m_type,
                     int (*handler)(struct sk_buff *, struct genl_info *))
{
	LOG_DBG("REGISTER - m_type: %d\n\
		NETLINK_RINA_C_MAX: %d",
		m_type, NETLINK_RINA_C_MAX);

	/*  DEBUG IF */
	if (message_handler_reg[m_type] == NULL )
		LOG_DBG("message_handler_reg[%d] is NULL, OK!", m_type);
	else 
		LOG_DBG("message_handler_reg[%d] is NOT NULL, KO!", m_type);


        if (m_type < 0 ||
            m_type >= NETLINK_RINA_C_MAX ||
            message_handler_reg[m_type] != NULL){
		LOG_ERR("Message type %d unknown or handler\
			already registered",m_type);
                return -1;
	}
        else {
		message_handler_reg[m_type] = (message_handler_t) handler;
		LOG_DBG("Handler for message %d registered", m_type);
	}
        return 0;
}

int (* get_handler(int m_type))(struct sk_buff *, struct genl_info *)
{

	LOG_DBG("GET HANDLER - m_type: %d\n\
		NETLINK_RINA_C_MAX: %d",
		m_type, NETLINK_RINA_C_MAX);

	/*  DEBUG IF */
	if (message_handler_reg[m_type] == NULL )
		LOG_DBG("message_handler_reg[%d] is NULL, KO!", m_type);
	else 
		LOG_DBG("message_handler_reg[%d] is NOT NULL, OK!", m_type);

	if (m_type < 0 ||
            m_type >= NETLINK_RINA_C_MAX ||
            message_handler_reg[m_type] == NULL) {
		LOG_ERR("Message handler is NULL");
                return NULL;
	}
        else {
		LOG_DBG("Handler for message %d to be returned", m_type);
                return message_handler_reg[m_type];
	}
}

int unregister_handler(int m_type)
{
        if (m_type < 0 || m_type >= NETLINK_RINA_C_MAX)
                return -1;

        message_handler_reg[m_type] = NULL;

        return 0;
}

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

	LOG_DBG("Message type to multiplex: %d", info->genlhdr->cmd);
        
	cb_function = get_handler(info->genlhdr->cmd);
        if (cb_function == NULL) {
		LOG_ERR("Could not retrieve NL Message handler");
		return -1;
	}

        if (cb_function(skb_in, info)){
		LOG_ERR("Callback function returned error");
		return -1;
	}
	else {
		LOG_DBG("Callbck function went well");
		return 0;
	}
	
}

/* Handler */
static int nl_rina_echo(struct sk_buff *skb_in, struct genl_info *info)
{
        /*
         * Message handling code goes here; return 0 on success, negative
         * values on failure
         */

        int ret;
        void *msg_head;
	struct sk_buff *skb;

	skb = skb_copy(skb_in, GFP_KERNEL);
	if(skb == NULL){
                LOG_ERR("netlink echo: out of memory");
                return -ENOMEM;
        }

        LOG_DBG("ECHOING MESSAGE");

        if (info == NULL) {
                LOG_DBG("info input parameter is NULL");
                return -1;
        }

        msg_head = genlmsg_put(skb, 0, info->snd_seq, &nl_rina_family, 0,
                               RINA_C_APP_ALLOCATE_FLOW_REQUEST);
        genlmsg_end(skb, msg_head);
        LOG_DBG("genlmsg_end OK");

	printk("Message generated:\n"
		"\t Netlink family: %d;\n"
		"\t Version: %d; \n"
		"\t Operation code: %d; \n"
		"\t Flags: %d\n",
		info->nlhdr->nlmsg_type, info->genlhdr->version, 
		info->genlhdr->cmd, info->nlhdr->nlmsg_flags);
	LOG_ERR("Message generated:\n"
		"\t Netlink family: %d;\n"
		"\t Version: %d; \n"
		"\t Operation code: %d; \n"
		"\t Flags: %d\n",
		info->nlhdr->nlmsg_type, info->genlhdr->version, 
		info->genlhdr->cmd, info->nlhdr->nlmsg_flags);

        /* ret = genlmsg_unicast(sock_net(skb->sk),skb,info->snd_portid); */
        ret = genlmsg_unicast(&init_net,skb,info->snd_portid);
        if (ret != 0) {
                LOG_DBG("COULD NOT SEND BACK UNICAST MESSAGE");
                return -1;
        }

        LOG_DBG("genkmsg_unicast OK");

        return 0;
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


int rina_netlink_init(void)
{
        int ret, i;

        LOG_FBEGN;

        LOG_DBG("NETLINK_RINA_C_MAX = %d", NETLINK_RINA_C_MAX);

        ret = genl_register_family(&nl_rina_family);
        if (ret != 0) {
                LOG_ERR("Cannot register NL family");
                return -1;
        }

        for (i = 0; i < ARRAY_SIZE(nl_rina_ops); i++) {
                ret = genl_register_ops(&nl_rina_family, &nl_rina_ops[i]);
                if (ret < 0) {
                        LOG_ERR("Cannot register NL ops %d", ret);
                        genl_unregister_family(&nl_rina_family);
                        return -2;
                }
        }

        for (i = 0; i < ARRAY_SIZE(message_handler_registry); i++) {
                INIT_LIST_HEAD(message_handler_registry[i]);
                if (message_handler_registry[i] == NULL) {
                        LOG_ERR("Could not initialize callback list for message %d", i);
                        return -2;
                }
        }
	LOG_DBG("Callbacks structs initialized");


	LOG_DBG("NetLink layer initialized");

        /* TODO: Testing */
        register_handler(RINA_C_APP_ALLOCATE_FLOW_REQUEST, nl_rina_echo);
        LOG_DBG("nl_rina echo registered for "
                "RINA_C_APP_ALLOCATE_FLOW_REQUEST");

        LOG_FEXIT;

        return 0;
}

void rina_netlink_exit(void)
{
        int ret;

        LOG_FBEGN;

        /* Unregister the family & operations*/
        ret = genl_unregister_family(&nl_rina_family);
        if(ret != 0) {
                LOG_DBG("unregister family %i\n", ret);
        }

        LOG_DBG("nl_rina echo unregistered for "
                "RINA_C_APP_ALLOCATE_FLOW_REQUEST");

        LOG_DBG("NetLink layer finalized");

        LOG_FEXIT;
}
