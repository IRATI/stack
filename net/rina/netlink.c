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
#include "utils.h"
#include "netlink.h"

/* FIXME: This define (and its related code) has to be removed */
#define TESTING 1

#define NETLINK_RINA "rina"


/*   Table to collect callbacks */
typedef int (* message_handler_cb)(void   * data,
				   struct sk_buff *,
				   struct genl_info *); 

struct message_handler {
	 void 		      * data;
         message_handler_cb  cb;
};

struct  message_handler messages_handlers[NETLINK_RINA_C_MAX];

static struct genl_family nl_family = {
        .id      = GENL_ID_GENERATE,
        .hdrsize = 0,
        .name    = NETLINK_RINA,
        .version = 1,
        .maxattr = NETLINK_RINA_A_MAX,
};

/* FIXME: To be completely rearranged (depends on personality etc. etc.) */
static int is_message_type_in_range(int msg_type, int min_value, int max_value)
{ return ((msg_type < min_value || msg_type >= max_value) ? 0 : 1); }

static int (* get_handler_cb(int msg_type))(void *, 
					    struct sk_buff *, 
					    struct genl_info *)
{
        ASSERT(is_message_type_in_range(msg_type, 0, NETLINK_RINA_C_MAX));

        LOG_DBG("Fetching handler callback for message type %d", msg_type);

        return messages_handlers[msg_type].cb;
}

static void * get_handler_data(int msg_type)
{
        ASSERT(is_message_type_in_range(msg_type, 0, NETLINK_RINA_C_MAX));

        LOG_DBG("Fetching handler data for message type %d", msg_type);

        return messages_handlers[msg_type].data;
}

static int dispatcher(struct sk_buff * skb_in, struct genl_info * info)
{
        /*
         * Message handling code goes here; return 0 on success, negative
         * values on failure
         */
        int (* cb_function)(void *,struct sk_buff *, struct genl_info *);
	void * data;
        int msg_type;
        int ret_val;

        LOG_DBG("Dispatching message (skb-in=%pK, info=%pK)", skb_in, info);

        if (!info) {
                LOG_DBG("Can't dispatch message, info parameter is empty");
                return -1;
        }

        msg_type = info->genlhdr->cmd;
        LOG_DBG("Message type to multiplex: %d", msg_type);
        
        cb_function = get_handler_cb(msg_type);
        if (!cb_function) {
                LOG_ERR("There's no handler callback registered from message type %d",
                        msg_type);
                return -1;
        }
	data = get_handler_data(msg_type);
        if (!data) {
                LOG_ERR("There's no handler data registered from message type %d",
                        msg_type);
                return -1;
        }

        ret_val = cb_function(data, skb_in, info);
        if (ret_val) {
                LOG_ERR("Callback returned %i, bailing out", ret_val);
                return -1;
        }

        LOG_DBG("Message %d handled successfully", msg_type);
        return 0;
}

#if TESTING
static int nl_rina_echo(void * data, 
			struct sk_buff *skb_in, 
			struct genl_info *info)
{
        /*
         * Message handling code goes here; return 0 on success, negative
         * values on failure
         */

        int ret;
        void *msg_head;
        struct sk_buff *skb;

        skb = skb_copy(skb_in, GFP_KERNEL);
        if (skb == NULL) {
                LOG_ERR("netlink echo: out of memory");
                return -ENOMEM;
        }

        LOG_DBG("ECHOING MESSAGE");

        if (info == NULL) {
                LOG_DBG("info input parameter is NULL");
                return -1;
        }

        msg_head = genlmsg_put(skb, 0, info->snd_seq, &nl_family, 0,
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
#endif

static struct genl_ops nl_ops[] = {
        {
                .cmd    = RINA_C_APP_ALLOCATE_FLOW_REQUEST,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_APP_ALLOCATE_FLOW_REQUEST_RESULT,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_APP_ALLOCATE_FLOW_REQUEST_ARRIVED,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_APP_ALLOCATE_FLOW_RESPONSE,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_APP_DEALLOCATE_FLOW_REQUEST,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_APP_DEALLOCATE_FLOW_RESPONSE,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_APP_FLOW_DEALLOCATED_NOTIFICATION,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_APP_REGISTER_APPLICATION_REQUEST,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_APP_REGISTER_APPLICATION_RESPONSE,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_APP_UNREGISTER_APPLICATION_REQUEST,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_APP_UNREGISTER_APPLICATION_RESPONSE,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_APP_APPLICATION_REGISTRATION_CANCELED_NOTIFICATION,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_APP_GET_DIF_PROPERTIES_REQUEST,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_APP_GET_DIF_PROPERTIES_RESPONSE,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_IPCM_IPC_PROCESS_REGISTERED_TO_DIF_NOTIFICATION,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_IPCM_IPC_PROCESS_UNREGISTERED_FROM_DIF_NOTIFICATION,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_IPCM_ENROLL_TO_DIF_REQUEST,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_IPCM_ENROLL_TO_DIF_RESPONSE,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_REQUEST,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_RESPONSE,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_IPCM_ALLOCATE_FLOW_REQUEST,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_IPCM_QUERY_RIB_REQUEST,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_IPCM_QUERY_RIB_RESPONSE,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_RMT_ADD_FTE_REQUEST,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_RMT_DELETE_FTE_REQUEST,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_RMT_DUMP_FT_REQUEST,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
        {
                .cmd    = RINA_C_RMT_DUMP_FT_REPLY,
                .flags  = 0,
                //.policy = nl_rina_policy,
                .doit   = dispatcher,
                .dumpit = NULL,
        },
};

int rina_netlink_unregister_handler(int msg_type)
{
        LOG_DBG("Unregistering handler for message type %d", msg_type);

        if (!is_message_type_in_range(msg_type, 0, NETLINK_RINA_C_MAX)) {
                LOG_ERR("Message type %d is out-of-range", msg_type);
                return -1;
        }
        ASSERT(msg_type >= 0 && msg_type < NETLINK_RINA_C_MAX);

        messages_handlers[msg_type].data = NULL;
        messages_handlers[msg_type].cb = NULL;

        LOG_DBG("Handler for message type %d unregistered successfully",
                msg_type);

        return 0;
}

int rina_netlink_register_handler(int    msg_type,
				  void * data,
                                  int (* handler)(void *,
				  		  struct sk_buff *,
                                                  struct genl_info *))
{
        LOG_DBG("Registering handler callback %pK and data %pK "
		"for message type %d",
                handler, data, msg_type);

        if (!handler) {
                LOG_ERR("Handler for message type %d is empty, "
                        "use unregister to remove it", msg_type);
                return -1;
        }

	if (!data) {
                LOG_ERR("Data for message type %d is empty, "
                        "please pass a proper value", msg_type);
                return -1;
        }

        if (!is_message_type_in_range(msg_type, 0, NETLINK_RINA_C_MAX)) {
                LOG_ERR("Message type %d is out-of-range", msg_type);
                return -1;
        }
        ASSERT(msg_type >= 0 && msg_type < NETLINK_RINA_C_MAX);

        if (messages_handlers[msg_type].data != NULL &&
	    messages_handlers[msg_type].cb != NULL) {
                LOG_ERR("The message handler or data for message type %d "
                        "has been registered already, "
                        "unregister it first", msg_type);
                return -1;
        }

        messages_handlers[msg_type].cb = (message_handler_cb) handler;
        messages_handlers[msg_type].data = data;

        LOG_DBG("Handler %pK and data %pK registered for  message type %d",
                handler, data, msg_type);

        return 0;
}

int rina_netlink_init(void)
{
        int ret;
#if TESTING
	void * test_data;
	int test_int = 4;
	test_data = &test_int;
#endif

        LOG_FBEGN;

        LOG_DBG("Initializing Netlink layer");


        LOG_DBG("Registering family with ops");
	ret = genl_register_family_with_ops(&nl_family, 
					    nl_ops, 
					    ARRAY_SIZE(nl_ops));
        
	LOG_DBG("LEODEBUG: ret of register is %d",ret);
	
	if (ret < 0) {
        	LOG_ERR("Cannot register family and ops (error=%i)", ret);
       		return -2;
        }

        LOG_DBG("NetLink layer initialized");

#if TESTING
        /* FIXME: Remove this hard-wired test */
        rina_netlink_register_handler(RINA_C_APP_ALLOCATE_FLOW_REQUEST,
				      test_data,
                                      nl_rina_echo);
#endif

        LOG_FEXIT;

        return 0;
}

void rina_netlink_exit(void)
{
        int ret;

        LOG_FBEGN;

        LOG_DBG("Finalizing Netlink layer");

        /* Unregister the family */
        ret = genl_unregister_family(&nl_family); 
        if (ret) {
                LOG_ERR("Could not unregister family (error=%i)", ret);
                return;
        }

        LOG_DBG("NetLink layer finalized");

        LOG_FEXIT;
}
