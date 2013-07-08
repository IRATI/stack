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
#include "debug.h"
#include "netlink.h"

/* FIXME: This define (and its related code) has to be removed */
#define TESTING 0

/*  FIXME: Fake ipc process id to get default personality */
#define RINA_FAKE_IPCP_ID 0

#define NETLINK_RINA "rina"

/* attributes */
/* FIXME: Are they really needed ??? */
enum {
	NETLINK_RINA_A_UNSPEC,
	NETLINK_RINA_A_MSG,

        /* Do not use */
	NETLINK_RINA_A_MAX,
};

#define NETLINK_RINA_A_MAX (NETLINK_RINA_A_MAX - 1)
#define NETLINK_RINA_C_MAX (RINA_C_MAX - 1)

struct message_handler {
        void *             data;
        message_handler_cb cb;
};

struct rina_nl_set {
	struct message_handler messages_handlers[NETLINK_RINA_C_MAX];
};

static struct genl_family nl_family = {
        .id      = GENL_ID_GENERATE,
        .hdrsize = 0,
        .name    = NETLINK_RINA,
        .version = 1,
        .maxattr = NETLINK_RINA_A_MAX, /* ??? */
};

static int is_message_type_in_range(int msg_type, int min_value, int max_value)
{ return ((msg_type < min_value || msg_type >= max_value) ? 0 : 1); }


/*  FIXME: Must return default personality's set */
static struct rina_nl_set * ipc_id_to_personality_set(int ipcp_id)
{
	struct rina_nl_set *pset;
	return pset;
}

static int dispatcher(struct sk_buff * skb_in, struct genl_info * info)
{
        /*
         * Message handling code goes here; return 0 on success, negative
         * values on failure
         */

        /* FIXME: What do we do if the handler returns != 0 ??? */

        message_handler_cb cb_function;
        void *             data;
        int                msg_type;
        int                ret_val;
	struct rina_nl_set *pset;

        LOG_DBG("Dispatching message (skb-in=%pK, info=%pK)", skb_in, info);

        if (!info) {
                LOG_ERR("Can't dispatch message, info parameter is empty");
                return -1;
        }

        if (!info->genlhdr) {
                LOG_ERR("Received message has no genlhdr field, "
                        "bailing out");
                return -1;
        }

        msg_type = info->genlhdr->cmd;
        LOG_DBG("Multiplexing message type %d", msg_type);

        if (!is_message_type_in_range(msg_type, 0, NETLINK_RINA_C_MAX)) {
                LOG_ERR("Wrong message type %d received from Netlink, "
                        "bailing out", msg_type);
                return -1;
        }
        ASSERT(is_message_type_in_range(msg_type, 0, NETLINK_RINA_C_MAX));

	pset = ipc_id_to_personality_set(RINA_FAKE_IPCP_ID);
	if (!pset)
		return -1;

        cb_function = pset->messages_handlers[msg_type].cb;
        if (!cb_function) {
                LOG_ERR("There's no handler callback registered for "
                        "message type %d", msg_type);
                return -1;
        }

        data = pset->messages_handlers[msg_type].data;
        /* Data might be empty */

        ret_val = cb_function(data, skb_in, info);
        if (ret_val) {
                LOG_ERR("Callback returned %d, bailing out", ret_val);
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

        LOG_ERR("Message generated:\n"
                "\t Netlink family: %d;\n"
                "\t Version: %d; \n"
                "\t Operation code: %d; \n"
                "\t Flags: %d",
                info->nlhdr->nlmsg_type, info->genlhdr->version,
                info->genlhdr->cmd, info->nlhdr->nlmsg_flags);

        /* ret = genlmsg_unicast(sock_net(skb->sk),skb,info->snd_portid); */
        ret = genlmsg_unicast(&init_net,skb,info->snd_portid);
        if (ret != 0) {
                LOG_ERR("COULD NOT SEND BACK UNICAST MESSAGE");
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
                .doit   = dispatcher,
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

int rina_netlink_register_handler(struct rina_nl_set * set,
                                  int                  msg_type,
                                  void *               data,
                                  message_handler_cb   handler)
{
        if (!set) {
                LOG_ERR("Bogus set passed, cannot register handler");
                return -1;
        }

        LOG_DBG("Registering handler callback %pK and data %pK "
                "for message type %d", handler, data, msg_type);

        if (!handler) {
                LOG_ERR("Handler for message type %d is empty, "
                        "use unregister to remove it", msg_type);
                return -1;
        }

        if (!is_message_type_in_range(msg_type, 0, NETLINK_RINA_C_MAX)) {
                LOG_ERR("Message type %d is out-of-range, "
                        "cannot register", msg_type);
                return -1;
        }

        ASSERT(handler != NULL);
        ASSERT(msg_type >= 0 && msg_type < NETLINK_RINA_C_MAX);

        if (set->messages_handlers[msg_type].cb) {
                LOG_ERR("The message handler for message type %d "
                        "has been already registered, unregister it first",
                        msg_type);
                return -1;
        }

        set->messages_handlers[msg_type].cb   = handler;
        set->messages_handlers[msg_type].data = data;

        LOG_DBG("Handler %pK (data %pK) registered for message type %d",
                handler, data, msg_type);

        return 0;
}
EXPORT_SYMBOL(rina_netlink_register_handler);

int rina_netlink_unregister_handler(struct rina_nl_set * set,
                                    int                  msg_type)
{
        if (!set) {
                LOG_ERR("Bogus set passed, cannot register handler");
                return -1;
        }

        LOG_DBG("Unregistering handler for message type %d", msg_type);

        if (!is_message_type_in_range(msg_type, 0, NETLINK_RINA_C_MAX)) {
                LOG_ERR("Message type %d is out-of-range, "
                        "cannot unregister", msg_type);
                return -1;
        }
        ASSERT(msg_type >= 0 && msg_type < NETLINK_RINA_C_MAX);

        bzero(set->messages_handlers[msg_type].cb,
              sizeof(set->messages_handlers[msg_type].cb));

	/* FIXME: Not sure if data pointer should be set to zero too */
	bzero(set->messages_handlers[msg_type].data,
               sizeof(set->messages_handlers[msg_type].data));

        LOG_DBG("Handler for message type %d unregistered successfully",
                msg_type);

        return 0;
}
EXPORT_SYMBOL(rina_netlink_unregister_handler);

struct rina_nl_set * rina_netlink_set_create(personality_id id)
{
        struct rina_nl_set * tmp;

        tmp = rkzalloc(sizeof(struct rina_nl_set), GFP_KERNEL);
        if (!tmp)
                return NULL;

        return tmp;
}
EXPORT_SYMBOL(rina_netlink_set_create);

int rina_netlink_set_destroy(struct rina_nl_set * set)
{
	int i;

        if (!set) {
                LOG_ERR("Bogus set passed, cannot destroy");
                return -1;
        }

	for (i=0; i<ARRAY_SIZE(set->messages_handlers); i++) {
		if(set->messages_handlers[i].cb != NULL) {
			LOG_WARN("Set on %pK had registered callbacks."
			" They will be unregistered", set);
			break;
		}
	}

        rkfree(set);
        return 0;
}
EXPORT_SYMBOL(rina_netlink_set_destroy);

int rina_netlink_init(void)
{
        int ret;

#if TESTING
        void * test_data;
        int test_int = 4;
        test_data = &test_int;
#endif

        LOG_DBG("Initializing Netlink layer");


        LOG_DBG("Registering family with ops");
        ret = genl_register_family_with_ops(&nl_family,
                                            nl_ops,
                                            ARRAY_SIZE(nl_ops));
        LOG_DBG("genl_register_family_with_ops() returned %i", ret);

        if (ret < 0) {
                LOG_ERR("Cannot register Netlink family and ops (error=%i), "
                        "bailing out", ret);
                return -1;
        }

#if TESTING
        /* FIXME: Remove this hard-wired test */
        rina_netlink_register_handler(RINA_C_APP_ALLOCATE_FLOW_REQUEST,
                                      test_data,
                                      nl_rina_echo);
#endif

        LOG_DBG("NetLink layer initialized successfully");

        return 0;
}

void rina_netlink_exit(void)
{
        int ret;

        LOG_DBG("Finalizing Netlink layer");

        ret = genl_unregister_family(&nl_family);
        if (ret) {
                LOG_ERR("Could not unregister Netlink family (error=%i), "
                        "bailing out. Your system might become unstable", ret);
                return;
        }

        LOG_DBG("NetLink layer finalized successfully");
}
