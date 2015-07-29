/*
 * RNL (RINA NetLink Layer)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
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

#include <linux/export.h>

#define RINA_PREFIX "rnl"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "rnl.h"
#include "rnl-utils.h"
#include "rnl-workarounds.h"
#include "rds/rwq.h"

#define NETLINK_RINA "rina"
#define RNL_WQ_NAME  "rnl-wq"

#define NETLINK_RINA_C_MIN (RINA_C_MIN + 1)
#define NETLINK_RINA_C_MAX (RINA_C_MAX - 1)

/* FIXME: This should be done more "dynamically" */
#define NETLINK_RINA_A_MAX ICCA_ATTR_MAX

struct message_handler {
        void *             data;
        message_handler_cb cb;
};

struct rnl_set {
        spinlock_t             lock;
        struct message_handler handlers[NETLINK_RINA_C_MAX];
        rnl_sn_t               sn_counter;
};

static struct rnl_set * default_set = NULL;
struct genl_family      rnl_nl_family = {
        .id      = GENL_ID_GENERATE,
        .hdrsize = sizeof(struct rina_msg_hdr),
        .name    = NETLINK_RINA,
        .version = 1,
        .maxattr = NETLINK_RINA_A_MAX,
};

static bool is_message_type_in_range(msg_type_t msg_type)
{ return is_value_in_range(msg_type, NETLINK_RINA_C_MIN, NETLINK_RINA_C_MAX); }

static int dispatcher(struct sk_buff * skb_in, struct genl_info * info)
{
        /*
         * Message handling code goes here; return 0 on success, negative
         * values on failure
         */

        /* FIXME: What do we do if the handler returns != 0 ??? */

        message_handler_cb cb_function;
        void *             data;
        msg_type_t         msg_type;
        int                ret_val;
        struct rnl_set *   tmp;

        if (!info) {
                LOG_ERR("Can't dispatch message, info parameter is empty");
                return -1;
        }

        if (!info->genlhdr) {
                LOG_ERR("Received message has no genlhdr field, "
                        "bailing out");
                return -1;
        }

        /* FIXME: Is this ok ? */
        if (!skb_in) {
                LOG_ERR("No skb to dispatch, bailing out");
                return -1;
        }

        LOG_DBG("Dispatching message (skb-in=%pK, info=%pK, attrs=%pK)",
                skb_in, info, info->attrs);

        msg_type = (msg_type_t) info->genlhdr->cmd;
        LOG_DBG("Multiplexing message type %d", msg_type);

        if (!is_message_type_in_range(msg_type)) {
                LOG_ERR("Wrong message type %d received from Netlink, "
                        "bailing out", msg_type);
                return -1;
        }
        ASSERT(is_message_type_in_range(msg_type));

        tmp = default_set;
        if (!tmp) {
                /* FIXME: Shouldn't this failback instead (returning 0) ?*/
                LOG_ERR("There is no set registered, "
                        "please register a set first");
                return -1;
        }

        LOG_DBG("Fetching handler callback and data");
        spin_lock(&tmp->lock);
        cb_function = tmp->handlers[msg_type].cb;
        data        = tmp->handlers[msg_type].data;
        spin_unlock(&tmp->lock);

        if (!cb_function) {
                /* FIXME: Shouldn't this failback instead (returning 0) ?*/
                LOG_ERR("There's no handler callback registered for "
                        "message type %d", msg_type);
                return -1;
        }
        /* Data might be empty, no check strictly necessary */

        LOG_DBG("Gonna call %pK(%pK, %pK, %pK)",
                cb_function, data, skb_in, info);

        ret_val = cb_function(data, skb_in, info);
        if (ret_val) {
                LOG_ERR("Callback returned %d, bailing out", ret_val);
                return -1;
        }

        LOG_DBG("Message %d handled successfully", msg_type);

        return 0;
}

#define __NLA_INIT(TYPE, LEN) { .type = TYPE, .len = LEN }

#define NLA_INIT_U64    __NLA_INIT(NLA_U64,    8)
#define NLA_INIT_U32    __NLA_INIT(NLA_U32,    4)
#define NLA_INIT_U16    __NLA_INIT(NLA_U16,    2)
#define NLA_INIT_NESTED __NLA_INIT(NLA_NESTED, 0)
#define NLA_INIT_STRING __NLA_INIT(NLA_STRING, 0)
#define NLA_INIT_FLAG   __NLA_INIT(NLA_FLAG,   0)
#define NLA_INIT_UNSPEC __NLA_INIT(NLA_UNSPEC, 0)

static struct nla_policy iatdr_policy[IATDR_ATTR_MAX + 1] = {
        [IATDR_ATTR_DIF_INFORMATION] = NLA_INIT_NESTED,
};

static struct nla_policy iudcr_policy[IUDCR_ATTR_MAX + 1] = {
        [IUDCR_ATTR_DIF_CONFIGURATION] = NLA_INIT_NESTED,
};

static struct nla_policy idrn_policy[IDRN_ATTR_MAX + 1] = {
        [IDRN_ATTR_IPC_PROCESS_NAME] = NLA_INIT_NESTED,
        [IDRN_ATTR_DIF_NAME]         = NLA_INIT_NESTED,
        [IDRN_ATTR_REGISTRATION]     = NLA_INIT_FLAG,
};

static struct nla_policy idun_policy[IDUN_ATTR_MAX + 1] = {
        [IDUN_ATTR_RESULT] = NLA_INIT_U32,
};

static struct nla_policy iafrm_policy[IAFRM_ATTR_MAX + 1] = {
        [IAFRM_ATTR_SOURCE_APP_NAME] = NLA_INIT_NESTED,
        [IAFRM_ATTR_DEST_APP_NAME]   = NLA_INIT_NESTED,
        [IAFRM_ATTR_FLOW_SPEC]       = NLA_INIT_NESTED,
        [IAFRM_ATTR_DIF_NAME]        = NLA_INIT_NESTED,
};

static struct nla_policy iafra_policy[IAFRA_ATTR_MAX + 1] = {
        [IAFRA_ATTR_SOURCE_APP_NAME] = NLA_INIT_NESTED,
        [IAFRA_ATTR_DEST_APP_NAME]   = NLA_INIT_NESTED,
        [IAFRA_ATTR_FLOW_SPEC]       = NLA_INIT_NESTED,
        [IAFRA_ATTR_DIF_NAME]        = NLA_INIT_NESTED,
        [IAFRA_ATTR_PORT_ID]         = NLA_INIT_U32,
};

static struct nla_policy iafre_policy[IAFRE_ATTR_MAX + 1] = {
        [IAFRE_ATTR_RESULT]        = NLA_INIT_U32,
        [IAFRE_ATTR_NOTIFY_SOURCE] = NLA_INIT_FLAG,
};

static struct nla_policy idfrt_policy[IDFRT_ATTR_MAX + 1] = {
        [IDFRT_ATTR_PORT_ID] = NLA_INIT_U32,
};

static struct nla_policy ifdn_policy[IFDN_ATTR_MAX + 1] = {
        [IFDN_ATTR_PORT_ID] = NLA_INIT_U32,
        [IFDN_ATTR_CODE]    = NLA_INIT_U32,
};

static struct nla_policy iccrq_policy[ICCRQ_ATTR_MAX + 1] = {
        [ICCRQ_ATTR_PORT_ID]     = NLA_INIT_U32,
        [ICCRQ_ATTR_SOURCE_ADDR] = NLA_INIT_U32,
        [ICCRQ_ATTR_DEST_ADDR]   = NLA_INIT_U32,
        [ICCRQ_ATTR_QOS_ID]      = NLA_INIT_U32,
        [ICCRQ_ATTR_DTP_CONFIG]  = NLA_INIT_NESTED,
        [ICCRQ_ATTR_DTCP_CONFIG] = NLA_INIT_NESTED,
};

static struct nla_policy icca_policy[ICCA_ATTR_MAX + 1] = {
        [ICCA_ATTR_PORT_ID]           = NLA_INIT_U32,
        [ICCA_ATTR_SOURCE_ADDR]       = NLA_INIT_U32,
        [ICCA_ATTR_DEST_ADDR]         = NLA_INIT_U32,
        [ICCA_ATTR_DEST_CEP_ID]       = NLA_INIT_U32,
        [ICCA_ATTR_QOS_ID]            = NLA_INIT_U32,
        [ICCA_ATTR_FLOW_USER_IPCP_ID] = NLA_INIT_U16,
        [ICCA_ATTR_DTP_CONFIG]        = NLA_INIT_NESTED,
        [ICCA_ATTR_DTCP_CONFIG]       = NLA_INIT_NESTED,
};

static struct nla_policy icurq_policy[ICURQ_ATTR_MAX + 1] = {
        [ICURQ_ATTR_PORT_ID]           = NLA_INIT_U32,
        [ICURQ_ATTR_SOURCE_CEP_ID]     = NLA_INIT_U32,
        [ICURQ_ATTR_DEST_CEP_ID]       = NLA_INIT_U32,
        [ICURQ_ATTR_FLOW_USER_IPCP_ID] = NLA_INIT_U16,
};

static struct nla_policy icdr_policy[ICDR_ATTR_MAX + 1] = {
        [ICDR_ATTR_PORT_ID]       = NLA_INIT_U32,
        [ICDR_ATTR_SOURCE_CEP_ID] = NLA_INIT_U32,
};

static struct nla_policy irar_policy[IRAR_ATTR_MAX + 1] = {
        [IRAR_ATTR_APP_NAME] = NLA_INIT_NESTED,
        [IRAR_ATTR_DIF_NAME] = NLA_INIT_NESTED,
};

static struct nla_policy iuar_policy[IUAR_ATTR_MAX + 1] = {
        [IUAR_ATTR_APP_NAME] = NLA_INIT_NESTED,
        [IUAR_ATTR_DIF_NAME] = NLA_INIT_NESTED,
};

static struct nla_policy idqr_policy[IDQR_ATTR_MAX + 1] = {
        [IDQR_ATTR_OBJECT_CLASS]    = NLA_INIT_STRING,
        [IDQR_ATTR_OBJECT_NAME]     = NLA_INIT_STRING,
        [IDQR_ATTR_OBJECT_INSTANCE] = NLA_INIT_U64,
        [IDQR_ATTR_SCOPE]           = NLA_INIT_U32,
        [IDQR_ATTR_FILTER] 	    = NLA_INIT_STRING,
};

static struct nla_policy rmpfe_policy[RMPFE_ATTR_MAX + 1] = {
        [RMPFE_ATTR_ENTRIES] = NLA_INIT_NESTED,
        [RMPFE_ATTR_MODE]    = NLA_INIT_U32,
};

static struct nla_policy ispsp_policy[ISPSP_ATTR_MAX + 1] = {
        [ISPSP_ATTR_PATH] = NLA_INIT_STRING,
        [ISPSP_ATTR_NAME] = NLA_INIT_STRING,
        [ISPSP_ATTR_VALUE] = NLA_INIT_STRING,
};

static struct nla_policy isps_policy[ISPS_ATTR_MAX + 1] = {
        [ISPS_ATTR_PATH] = NLA_INIT_STRING,
        [ISPS_ATTR_NAME] = NLA_INIT_STRING,
};

static struct nla_policy ieerm_policy[IEERM_ATTR_MAX + 1] = {
	[IEERM_ATTR_N_1_PORT] 	 	   = NLA_INIT_U32,
        [IEERM_ATTR_EN_ENCRYPT] 	   = NLA_INIT_FLAG,
        [IEERM_ATTR_EN_DECRYPT] 	   = NLA_INIT_FLAG,
        [IEERM_ATTR_ENCRYPT_KEY] 	   = NLA_INIT_UNSPEC,
};

#define DECL_NL_OP(COMMAND, POLICY) {           \
                .cmd    = COMMAND,              \
                        .flags  = 0,            \
                        .policy = POLICY,       \
                        .doit   = dispatcher,   \
                        .dumpit = NULL,         \
                        }

static struct genl_ops nl_ops[] = {
        DECL_NL_OP(RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST, iatdr_policy),
        DECL_NL_OP(RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE, NULL),
        DECL_NL_OP(RINA_C_IPCM_UPDATE_DIF_CONFIG_REQUEST, iudcr_policy),
        DECL_NL_OP(RINA_C_IPCM_UPDATE_DIF_CONFIG_RESPONSE, NULL),
        DECL_NL_OP(RINA_C_IPCM_IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION,
                   idrn_policy),
        DECL_NL_OP(RINA_C_IPCM_IPC_PROCESS_DIF_UNREGISTRATION_NOTIFICATION,
                   idun_policy),
        DECL_NL_OP(RINA_C_IPCM_ENROLL_TO_DIF_REQUEST, NULL),
        DECL_NL_OP(RINA_C_IPCM_ENROLL_TO_DIF_RESPONSE, NULL),
        DECL_NL_OP(RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_REQUEST, NULL),
        DECL_NL_OP(RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_RESPONSE, NULL),
        DECL_NL_OP(RINA_C_IPCM_ALLOCATE_FLOW_REQUEST, iafrm_policy),
        DECL_NL_OP(RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_ARRIVED, iafra_policy),
        DECL_NL_OP(RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_RESULT, NULL),
        DECL_NL_OP(RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE, iafre_policy),
        DECL_NL_OP(RINA_C_IPCM_DEALLOCATE_FLOW_REQUEST, idfrt_policy),
        DECL_NL_OP(RINA_C_IPCM_DEALLOCATE_FLOW_RESPONSE, NULL),
        DECL_NL_OP(RINA_C_IPCM_FLOW_DEALLOCATED_NOTIFICATION, ifdn_policy),
        DECL_NL_OP(RINA_C_IPCM_REGISTER_APPLICATION_REQUEST, irar_policy),
        DECL_NL_OP(RINA_C_IPCM_REGISTER_APPLICATION_RESPONSE, NULL),
        DECL_NL_OP(RINA_C_IPCM_UNREGISTER_APPLICATION_REQUEST, iuar_policy),
        DECL_NL_OP(RINA_C_IPCM_UNREGISTER_APPLICATION_RESPONSE, NULL),
        DECL_NL_OP(RINA_C_IPCM_QUERY_RIB_REQUEST, idqr_policy),
        DECL_NL_OP(RINA_C_IPCM_QUERY_RIB_RESPONSE, NULL),
        DECL_NL_OP(RINA_C_RMT_MODIFY_FTE_REQUEST, rmpfe_policy),
        DECL_NL_OP(RINA_C_RMT_DUMP_FT_REQUEST, NULL),
        DECL_NL_OP(RINA_C_RMT_DUMP_FT_REPLY, NULL),
        DECL_NL_OP(RINA_C_IPCM_SOCKET_CLOSED_NOTIFICATION, NULL),
        DECL_NL_OP(RINA_C_IPCM_IPC_MANAGER_PRESENT, NULL),
        DECL_NL_OP(RINA_C_IPCP_CONN_CREATE_REQUEST, iccrq_policy),
        DECL_NL_OP(RINA_C_IPCP_CONN_CREATE_RESPONSE, NULL),
        DECL_NL_OP(RINA_C_IPCP_CONN_CREATE_ARRIVED, icca_policy),
        DECL_NL_OP(RINA_C_IPCP_CONN_CREATE_RESULT, NULL),
        DECL_NL_OP(RINA_C_IPCP_CONN_UPDATE_REQUEST, icurq_policy),
        DECL_NL_OP(RINA_C_IPCP_CONN_UPDATE_RESULT, NULL),
        DECL_NL_OP(RINA_C_IPCP_CONN_DESTROY_REQUEST, icdr_policy),
        DECL_NL_OP(RINA_C_IPCP_CONN_DESTROY_RESULT, NULL),
        DECL_NL_OP(RINA_C_IPCP_SET_POLICY_SET_PARAM_REQUEST, ispsp_policy),
        DECL_NL_OP(RINA_C_IPCP_SET_POLICY_SET_PARAM_RESPONSE, NULL),
        DECL_NL_OP(RINA_C_IPCP_SELECT_POLICY_SET_REQUEST, isps_policy),
        DECL_NL_OP(RINA_C_IPCP_SELECT_POLICY_SET_RESPONSE, NULL),
        DECL_NL_OP(RINA_C_IPCP_ENABLE_ENCRYPTION_REQUEST, ieerm_policy),
        DECL_NL_OP(RINA_C_IPCP_ENABLE_ENCRYPTION_RESPONSE, NULL)
};

int rnl_handler_register(struct rnl_set *   set,
                         msg_type_t         msg_type,
                         void *             data,
                         message_handler_cb handler)
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

        if (!is_message_type_in_range(msg_type)) {
                LOG_ERR("Message type %d is out-of-range, "
                        "cannot register", msg_type);
                return -1;
        }
        ASSERT(msg_type >= NETLINK_RINA_C_MIN &&
               msg_type <= NETLINK_RINA_C_MAX);
        ASSERT(handler != NULL);

        spin_lock(&set->lock);

        if (set->handlers[msg_type].cb) {
                spin_unlock(&set->lock);
                LOG_ERR("There is a handler for message type %d still "
                        "registered, unregister it first",
                        msg_type);
                return -1;
        }

        set->handlers[msg_type].cb   = handler;
        set->handlers[msg_type].data = data;

        spin_unlock(&set->lock);

        LOG_DBG("Handler %pK (data %pK) registered for message type %d",
                handler, data, msg_type);

        return 0;
}
EXPORT_SYMBOL(rnl_handler_register);

int rnl_handler_unregister(struct rnl_set * set,
                           msg_type_t       msg_type)
{
        if (!set) {
                LOG_ERR("Bogus set passed, cannot register handler");
                return -1;
        }

        LOG_DBG("Unregistering handler for message type %d", msg_type);

        if (!is_message_type_in_range(msg_type)) {
                LOG_ERR("Message type %d is out-of-range, "
                        "cannot unregister", msg_type);
                return -1;
        }
        ASSERT(msg_type >= NETLINK_RINA_C_MIN &&
               msg_type <= NETLINK_RINA_C_MAX);

        spin_lock(&set->lock);
        bzero(&set->handlers[msg_type], sizeof(set->handlers[msg_type]));
        spin_unlock(&set->lock);

        LOG_DBG("Handler for message type %d unregistered successfully",
                msg_type);

        return 0;
}
EXPORT_SYMBOL(rnl_handler_unregister);

int rnl_set_register(struct rnl_set * set)
{
        if (!set) {
                LOG_ERR("Bogus set passed, cannot register it");
                return -1;
        }

        if (default_set != NULL) {
                LOG_ERR("Default set already registered");
                return -2;
        }

        default_set = set;

        LOG_DBG("Set %pK registered", set);

        return 0;
}
EXPORT_SYMBOL(rnl_set_register);

int rnl_set_unregister(struct rnl_set * set)
{
        if (!set) {
                LOG_ERR("Bogus set passed, cannot unregister it");
                return -1;
        }

        if (default_set != set) {
                LOG_ERR("Target set is different than the registered one");
                return -2;
        }

        default_set = NULL;

        LOG_DBG("Set %pK unregistered", set);

        return 0;
}
EXPORT_SYMBOL(rnl_set_unregister);

struct rnl_set * rnl_set_create(personality_id id)
{
        struct rnl_set * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
        if (!tmp)
                return NULL;

        spin_lock_init(&tmp->lock);

        LOG_DBG("Set %pK created successfully", tmp);

        return tmp;
}
EXPORT_SYMBOL(rnl_set_create);

int rnl_set_destroy(struct rnl_set * set)
{
        int    i;
        size_t count;

        if (!set) {
                LOG_ERR("Bogus set passed, cannot destroy");
                return -1;
        }

        count = 0;

        for (i = 0; i < ARRAY_SIZE(set->handlers); i++) {
                if (set->handlers[i].cb != NULL) {
                        count++;
                        LOG_DBG("Set %pK has at least one hander still "
                                "registered, it will be unregistered", set);
                        break;
                }
        }

        if (count)
                LOG_WARN("Set %pK had %zd handler(s) that have not been "
                         "unregistered ...", set, count);
        rkfree(set);

        LOG_DBG("Set %pK destroyed %s", set, count ? "" : "successfully");

        return 0;
}
EXPORT_SYMBOL(rnl_set_destroy);

rnl_sn_t rnl_get_next_seqn(struct rnl_set * set)
{
        rnl_sn_t tmp;

        spin_lock(&set->lock);

        tmp = set->sn_counter++;
        if (set->sn_counter == 0) {
                LOG_WARN("RNL Sequence number rolled-over for set %pK!", set);
                /* FIXME: What to do about roll-over? */
        }

        spin_unlock(&set->lock);

        return set->sn_counter;
}
EXPORT_SYMBOL(rnl_get_next_seqn);

static struct rnl_notifier_data {
        struct workqueue_struct * wq;
} rnl_sock_closed_notifier_data;

struct sock_closed_notif_item {
        int portid;
        rnl_port_t ipcmanagerport;
};

static int socket_closed_worker(void * o)
{
        struct sock_closed_notif_item * item_data;

        LOG_DBG("RNL socket notification worker called");

        item_data = (struct sock_closed_notif_item *) o;
        if (!item_data) {
                LOG_ERR("No data passed to the worker!");
                return -1;
        }

        rnl_ipcm_sock_closed_notif_msg(item_data->portid,
                                       item_data->ipcmanagerport);

        rkfree(item_data);
        return 0;
}

/*
 * Invoked when an event related to a NL socket occurs. We're only interested
 * in socket closed events.
 */
static int netlink_notify_callback(struct notifier_block * nb,
                                   unsigned long           state,
                                   void *                  notification)
{
        struct netlink_notify *         notify = notification;
        rnl_port_t                      port;
        struct sock_closed_notif_item * item_data;
        struct rwq_work_item *          item;

        if (state != NETLINK_URELEASE)
                return NOTIFY_DONE;

        if (!notify) {
                LOG_ERR("Wrong data obtained in netlink notifier callback");
                return NOTIFY_BAD;
        }

        //Only consider messages of the Generic Netlink protocol
        if (notify->protocol != NETLINK_GENERIC) {
        	return NOTIFY_DONE;
        }

        port = rnl_get_ipc_manager_port();
        if (port) {
                /* Check if the IPC Manager is the process that died */
                if (port == notify->portid) {
                        rnl_set_ipc_manager_port(0);
                        LOG_WARN("IPC Manager process has been destroyed");
                } else {
                        item_data = rkzalloc(sizeof(*item_data), GFP_ATOMIC);
                        if (!item_data) {
                                LOG_DBG("Could not malloc memory for "
                                        "item_data");
                                return NOTIFY_DONE;
                        }
                        item_data->ipcmanagerport = port;
                        item_data->portid         = notify->portid;

                        item = rwq_work_create_ni(socket_closed_worker,
                                                  item_data);
                        if (!item) {
                                LOG_ERR("Error creating work item");
                                rkfree(item_data);
                                return NOTIFY_DONE;
                        }

                        if (rwq_work_post(rnl_sock_closed_notifier_data.wq,
                                          item)) {
                                LOG_ERR("Could not post item to work queue");
                                rkfree(item_data);
                                return NOTIFY_DONE;
                        }

                        LOG_DBG("Queued work item successfully!");
                }

        }

        return NOTIFY_DONE;
}

static struct notifier_block netlink_notifier = {
        .notifier_call = netlink_notify_callback,
};

static struct ipcm_rnl_port {
        rnl_port_t ipc_manager_port;
        spinlock_t lock;
} ipcm_port;

rnl_port_t rnl_get_ipc_manager_port(void)
{
        rnl_port_t ret;

        spin_lock(&ipcm_port.lock);
        ret = ipcm_port.ipc_manager_port;
        spin_unlock(&ipcm_port.lock);

        return ret;
}
EXPORT_SYMBOL(rnl_get_ipc_manager_port);

void rnl_set_ipc_manager_port(rnl_port_t port)
{
        spin_lock(&ipcm_port.lock);
        ipcm_port.ipc_manager_port = port;
        spin_unlock(&ipcm_port.lock);
}
EXPORT_SYMBOL(rnl_set_ipc_manager_port);

int rnl_init(void)
{
        int ret;

        LOG_DBG("Initializing Netlink layer");

        /*
         * FIXME:
         *   This is to allow user-space processes to exchange NL messages
         *   without requiring the CAP_NET_ADMIN capability. We should find
         *   a better way to do it since we're modifying an internal NL
         *   data structure, but apparently there's no other way around.
         */
        set_netlink_non_root_send_flag();

        ret = genl_register_family_with_ops(&rnl_nl_family, nl_ops);
        if (ret != 0) {
                LOG_ERR("Cannot register Netlink family and ops (error=%i), "
                        "bailing out", ret);
                return -1;
        }
        LOG_DBG("NL family registered (id = %d)", rnl_nl_family.id);

        /*
         * Register a NETLINK notifier so that the kernel is
         * informed when a Netlink socket in user-space is closed
         */
        ret = netlink_register_notifier(&netlink_notifier);
        if (ret) {
                LOG_ERR("Cannot register Netlink notifier (error=%i), "
                        "bailing out", ret);
                genl_unregister_family(&rnl_nl_family);
                return -1;
        }

        ipcm_port.ipc_manager_port = 0;
        spin_lock_init(&ipcm_port.lock);

        rnl_sock_closed_notifier_data.wq = rwq_create(RNL_WQ_NAME);
        if (!rnl_sock_closed_notifier_data.wq) {
                LOG_ERR("Cannot create RNL work queue, bailing out");
                return -1;
        }

        LOG_DBG("NetLink layer initialized successfully");

        return 0;
}

void rnl_exit(void)
{
        int ret;

        LOG_DBG("Finalizing Netlink layer");

        /* Unregister the notifier */
        netlink_unregister_notifier(&netlink_notifier);

        ret = genl_unregister_family(&rnl_nl_family);
        if (ret) {
                LOG_ERR("Could not unregister Netlink family (error = %i), "
                        "bailing out", ret);
                LOG_CRIT("Your system might become unstable");
                return;
        }

        /*
         * FIXME:
         *   Add checks here to prevent misses of finalizations and/or
         *   destructions
         */

        ASSERT(!default_set);

        if (rnl_sock_closed_notifier_data.wq)
                rwq_destroy(rnl_sock_closed_notifier_data.wq);

        LOG_DBG("NetLink layer finalized successfully");
}
