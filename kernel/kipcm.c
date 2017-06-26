/*
 * KIPCM (Kernel IPC Manager)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
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

#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/mutex.h>
#include <linux/hardirq.h>

#define RINA_PREFIX "kipcm"

#include "logs.h"
#include "utils.h"
#include "kipcm.h"
#include "debug.h"
#include "ipcp-factories.h"
#include "ipcp-utils.h"
#include "kipcm-utils.h"
#include "common.h"
#include "kfa.h"
#include "kfa-utils.h"
#include "efcp-utils.h"

#define DEFAULT_FACTORY "normal-ipc"

struct flow_messages {
        struct kipcm_pmap * ingress;
        struct kipcm_smap * egress;
};

struct kipcm {
	/* FIXME: LB: instances and rset replicate the same info (list of
	 * ipcps), instances could be probably removed if we stay with robjs
	 */
	struct rset *           rset;
        struct mutex            lock;
        struct ipcp_factories * factories;
        struct ipcp_imap *      instances;
        struct flow_messages *  messages;
        struct kfa *            kfa;
};

struct kipcm * default_kipcm;
EXPORT_SYMBOL(default_kipcm);

irati_msg_handler_t kipcm_handlers[RINA_C_MAX];

#ifdef CONFIG_RINA_KIPCM_LOCKS_DEBUG

#define KIPCM_LOCK_HEADER(X)   do {                             \
                LOG_DBG("KIPCM instance %pK locking ...", X);   \
        } while (0)
#define KIPCM_LOCK_FOOTER(X)   do {                             \
                LOG_DBG("KIPCM instance %pK locked", X);        \
        } while (0)

#define KIPCM_UNLOCK_HEADER(X) do {                             \
                LOG_DBG("KIPCM instance %pK unlocking ...", X); \
        } while (0)
#define KIPCM_UNLOCK_FOOTER(X) do {                             \
                LOG_DBG("KIPCM instance %pK unlocked", X);      \
        } while (0)

#else

#define KIPCM_LOCK_HEADER(X)   do { } while (0)
#define KIPCM_LOCK_FOOTER(X)   do { } while (0)
#define KIPCM_UNLOCK_HEADER(X) do { } while (0)
#define KIPCM_UNLOCK_FOOTER(X) do { } while (0)

#endif

#define KIPCM_LOCK_INIT(X) mutex_init(&(X -> lock));
#define KIPCM_LOCK_FINI(X) mutex_destroy(&(X -> lock));
#define __KIPCM_LOCK(X)    mutex_lock(&(X -> lock))
#define __KIPCM_UNLOCK(X)  mutex_unlock(&(X -> lock))
#define KIPCM_LOCK(X)   do {                    \
                KIPCM_LOCK_HEADER(X);           \
                __KIPCM_LOCK(X);                \
                KIPCM_LOCK_FOOTER(X);           \
        } while (0)
#define KIPCM_UNLOCK(X) do {                    \
                KIPCM_UNLOCK_HEADER(X);         \
                __KIPCM_UNLOCK(X);              \
                KIPCM_UNLOCK_FOOTER(X);         \
        } while (0)

static int alloc_flow_req_reply(struct ctrldev_priv   * ctrl_dev,
                                ipc_process_id_t        id,
				int8_t                	res,
				uint32_t                seq_num,
				irati_msg_port_t        port_id,
				port_id_t 		pid)
{
	struct irati_kmsg_multi_msg resp_msg;

	resp_msg.msg_type = RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_RESULT;
	resp_msg.port_id = pid;
	resp_msg.src_ipcp_id = id;
	resp_msg.dest_ipcp_id = 0;
	resp_msg.result = res;
	resp_msg.event_id = seq_num;

        if (irati_ctrl_dev_snd_resp_msg(ctrl_dev, &resp_msg)) {
                LOG_ERR("Could not send flow_result_msg");
                return -1;
        }

        return 0;
}

/*
 * It is the responsibility of the shims to send the alloc_req_arrived
 * and the alloc_req_result.
 */
static int notify_ipcp_allocate_flow_request(struct ctrldev_priv *ctrl_dev,
                                             struct irati_kmsg_ipcm_allocate_flow * msg,
					     void * data)
{
        struct ipcp_instance * ipc_process;
        struct ipcp_instance * user_ipcp;
        ipc_process_id_t       ipc_id;
        ipc_process_id_t       user_ipc_id;
        struct kipcm *         kipcm;
        port_id_t              pid = port_id_bad();

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }

        if (!msg) {
                LOG_ERR("Bogus struct irati_kmsg_ipcm_allocate_flow passed");
                return -1;
        }

        kipcm = (struct kipcm *) data;

        ipc_id = 0;
        user_ipc_id  = msg->src_ipcp_id;
        ipc_id       = msg->dest_ipcp_id;
        /* FIXME: Here we should take the lock */
        ipc_process  = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipc_process) {
                LOG_ERR("IPC process %d not found", ipc_id);
                goto fail;
        }

        pid = kfa_port_id_reserve(kipcm->kfa, ipc_id);
        ASSERT(is_port_id_ok(pid));

        if (kipcm_pmap_add(kipcm->messages->ingress, pid, msg->event_id)) {
                LOG_ERR("Could not add map [pid, seq_num]: [%d, %d]",
                        pid, info->snd_seq);
                kfa_port_id_release(kipcm->kfa, pid);
                goto fail;
        }

        if (user_ipc_id) {
                user_ipcp = ipcp_imap_find(kipcm->instances, user_ipc_id);
                if (!user_ipcp) {
                        LOG_ERR("Could not find the user ipcp of the flow...");
                        kfa_port_id_release(kipcm->kfa, pid);
                        goto fail;
                }
        } else {
                user_ipcp = kfa_ipcp_instance(kipcm->kfa);
		/* NOTE: original function called non-blocking I/O?? */
                if (kfa_flow_create(kipcm->kfa, pid, ipc_process, ipc_id,
									NULL)) {
                        LOG_ERR("Could not find the user ipcp of the flow...");
                        kfa_port_id_release(kipcm->kfa, pid);
                        goto fail;
                }
        }

        ASSERT(ipc_process->ops);
        ASSERT(ipc_process->ops->flow_allocate_request);

        if (ipc_process->ops->flow_allocate_request(ipc_process->data,
                                                    user_ipcp,
                                                    msg->source,
                                                    msg->dest,
                                                    msg->fspec,
                                                    pid)) {
                LOG_ERR("Failed allocating flow request");
                kfa_port_id_release(kipcm->kfa, pid);
                goto fail;
        }

        rnl_msg_destroy(msg);

        return 0;

 fail:
        return alloc_flow_req_reply(ctrl_dev, ipc_id, -1, msg->event_id,
        			    msg->src_port, port_id_bad());
}

static int notify_ipcp_allocate_flow_response(struct ctrldev_priv *ctrl_dev,
                                              struct irati_kmsg_ipcm_allocate_flow_resp * msg,
					      void * data)
{
        struct kipcm *         kipcm;
        struct ipcp_instance * ipc_process;
        struct ipcp_instance * user_ipcp;
        ipc_process_id_t       ipc_id;
        ipc_process_id_t       user_ipc_id;
        port_id_t              pid;

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }

        kipcm = (struct kipcm *) data;

        if (!msg) {
                LOG_ERR("Bogus struct irati_kmsg_ipcm_allocate_flow_resp passed");
                return -1;
        }

        user_ipc_id  = msg->src_ipcp_id;
        ipc_id       = msg->dest_ipcp_id;
        ipc_process  = ipcp_imap_find(kipcm->instances, ipc_id);
        pid = kipcm_smap_find(kipcm->messages->egress, msg->event_id);
        if (!ipc_process) {
                LOG_ERR("IPC process %d not found", ipc_id);
                kfa_port_id_release(kipcm->kfa, pid);
                goto fail;
        }
        if (!is_port_id_ok(pid)) {
                LOG_ERR("Could not find port id %d for response %d",
                        pid, info->snd_seq);
                kfa_port_id_release(kipcm->kfa, pid);
                goto fail;
        }
        if (kipcm_smap_remove(kipcm->messages->egress, msg->event_id)) {
                LOG_ERR("Could not destroy egress messages map entry");
                kfa_port_id_release(kipcm->kfa, pid);
                goto fail;
        }

        user_ipcp = kfa_ipcp_instance(kipcm->kfa);
        if (user_ipc_id)
                user_ipcp = ipcp_imap_find(kipcm->instances, user_ipc_id);

        ASSERT(ipc_process->ops);
        ASSERT(ipc_process->ops->flow_allocate_response);

        if (ipc_process->ops->flow_allocate_response(ipc_process->data,
                                                     user_ipcp,
                                                     pid,
                                                     msg->result)) {
                LOG_ERR("Failed allocate flow response for port id: %d",
                        attrs->id);
        }
fail:
        return 0;
}

static int
dealloc_flow_req_reply(struct ctrldev_priv * ctrl_dev,
		       ipc_process_id_t id,
		       int8_t           res,
		       uint32_t         seq_num)
{
	struct irati_msg_base_resp resp_msg;

	resp_msg.msg_type = RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_RESULT;
	resp_msg.src_ipcp_id = id;
	resp_msg.dest_ipcp_id = 0;
	resp_msg.result = res;
	resp_msg.event_id = seq_num;

        if (irati_ctrl_dev_snd_resp_msg(ctrl_dev, &resp_msg)) {
                LOG_ERR("Could not send flow_result_msg");
                return -1;
        }

        return 0;
}

/*
 * It is the responsibility of the shims to send the alloc_req_arrived
 * and the alloc_req_result.
 */
static int notify_ipcp_deallocate_flow_request(struct ctrldev_priv *ctrl_dev,
                                               struct irati_kmsg_multi_msg * msg,
                                               void * data)
{
        struct ipcp_instance * ipc_process;
        ipc_process_id_t       ipc_id;
        struct kipcm *         kipcm;

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }

        if (!msg) {
                LOG_ERR("Bogus struct irati_kmsg_multi_msg passed");
                return -1;
        }

        kipcm  = (struct kipcm *) data;

        ipc_id      = msg->dest_ipcp_id;
        ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipc_process) {
                LOG_ERR("IPC process %d not found", ipc_id);
                goto fail;
        }

        ASSERT(ipc_process->ops);
        ASSERT(ipc_process->ops->flow_deallocate);

        if (ipc_process->ops->flow_deallocate(ipc_process->data, msg->port_id)) {
                LOG_ERR("Failed deallocate flow request "
                        "for port id: %d", msg->port_id);
                goto fail;
        }

        kfa_port_id_release(kipcm->kfa, msg->port_id);

        return dealloc_flow_req_reply(ctrl_dev, ipc_id, 0, msg->event_id);

 fail:
        return dealloc_flow_req_reply(ctrl_dev, ipc_id, -1, msg->event_id);
}

static int assign_to_dif_reply(struct ctrldev_priv * ctrl_dev,
		    	       ipc_process_id_t id,
			       int8_t           res,
			       uint32_t         seq_num)
{
	struct irati_msg_base_resp resp_msg;

	resp_msg.msg_type = RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE;
	resp_msg.src_ipcp_id = id;
	resp_msg.dest_ipcp_id = 0;
	resp_msg.result = res;
	resp_msg.event_id = seq_num;

        if (irati_ctrl_dev_snd_resp_msg(ctrl_dev, &resp_msg)) {
                LOG_ERR("Could not send flow_result_msg");
                return -1;
        }

        return 0;
}

static int notify_ipcp_assign_dif_request(struct ctrldev_priv * ctrl_dev,
                                          struct irati_kmsg_ipcm_assign_to_dif * msg,
					  void * data)
{
        struct kipcm *         kipcm;
        struct ipcp_instance * ipc_process;
        ipc_process_id_t       ipc_id;

        int retval = 0;
        ipc_id = 0;

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }
        kipcm = (struct kipcm *) data;

        if (!msg) {
                LOG_ERR("Bogus struct irati_kmsg_ipcm_assign_to_dif passed");
                return -1;
        }

        ipc_id      = msg->dest_ipcp_id;
        ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipc_process) {
                LOG_ERR("IPC process %d not found", ipc_id);
                retval = -1;
                goto fail;
        }
        LOG_DBG("Found IPC Process with id %d", ipc_id);

        ASSERT(ipc_process->ops);
        ASSERT(ipc_process->ops->assign_to_dif);

        if (ipc_process->ops->assign_to_dif(ipc_process->data,
                                            msg->dif_name,
					    msg->type,
					    msg->dif_config)) {
                char * tmp = name_tostring(msg->dif_name);
                LOG_ERR("Assign to dif %s operation failed for IPC process %d",
                        tmp, ipc_id);
                rkfree(tmp);

                retval = -1;
                goto fail;
        }

        LOG_DBG("Assign to dif operation seems ok, gonna complete it");

 fail:
        return assign_to_dif_reply(ctrl_dev, ipc_id, retval, msg->event_id);
}

static int update_dif_config_reply(struct ctrldev_priv * ctrl_dev,
				   ipc_process_id_t id,
				   int8_t           res,
				   uint32_t         seq_num)
{
	struct irati_msg_base_resp resp_msg;

	resp_msg.msg_type = RINA_C_IPCM_UPDATE_DIF_CONFIG_RESPONSE;
	resp_msg.src_ipcp_id = id;
	resp_msg.dest_ipcp_id = 0;
	resp_msg.result = res;
	resp_msg.event_id = seq_num;

	if (irati_ctrl_dev_snd_resp_msg(ctrl_dev, &resp_msg)) {
		LOG_ERR("Could not send flow_result_msg");
		return -1;
	}

	return 0;
}

static int notify_ipcp_update_dif_config_request(struct ctrldev_priv * ctrl_dev,
                                                 struct irati_kmsg_ipcm_update_config * msg,
						 void * data)
{
        struct kipcm *         kipcm;
        struct ipcp_instance * ipc_process;
        ipc_process_id_t       ipc_id;

        ipc_id = 0;

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }

        kipcm = (struct kipcm *) data;
        if (!msg) {
                LOG_ERR("Bogus struct irati_kmsg_ipcm_update_config passed");
                return -1;
        }

        ipc_id = msg->dest_ipcp_id;

        ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipc_process) {
                LOG_ERR("IPC process %d not found", ipc_id);
                goto fail;
        }
        LOG_DBG("Found IPC Process with id %d", ipc_id);

        ASSERT(ipc_process->ops);
        ASSERT(ipc_process->ops->update_dif_config);

        if (ipc_process->ops->update_dif_config(ipc_process->data,
                                                msg->dif_config)) {
                LOG_ERR("Update DIF config operation failed for "
                        "IPC process %d", ipc_id);
                goto fail;
        }

        return update_dif_config_reply(ctrl_dev, ipc_id, 0, msg->event_id);

 fail:
 	return update_dif_config_reply(ctrl_dev, ipc_id, -1, msg->event_id);
}

static int
reg_unreg_resp_free_and_reply(struct rnl_msg * msg,
                              ipc_process_id_t id,
                              uint_t           res,
                              uint_t           seq_num,
                              uint_t           port_id,
                              bool             is_register)
{
	uint_t command;

        rnl_msg_destroy(msg);

        command = is_register                               ?
                RINA_C_IPCM_REGISTER_APPLICATION_RESPONSE  :
                RINA_C_IPCM_UNREGISTER_APPLICATION_RESPONSE;

        if (rnl_base_response(id, res, 0, 0, seq_num, command, port_id))
                return -1;

        return 0;
}

static int notify_ipcp_register_app_request(void *             data,
                                            struct sk_buff *   buff,
                                            struct genl_info * info)
{
        struct kipcm *                          kipcm;
        struct rnl_ipcm_reg_app_req_msg_attrs * attrs;
        struct rnl_msg *                        msg;
        struct ipcp_instance *                  ipc_process;
        ipc_process_id_t                        ipc_id;

        ipc_id = 0;

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }

        kipcm = (struct kipcm *) data;
        if (!info) {
                LOG_ERR("Bogus struct genl_info passed, cannot parse NL msg");
                return -1;
        }

        msg = rnl_msg_create(RNL_MSG_ATTRS_REG_UNREG_REQUEST);
        if (!msg) {
                LOG_ERR("Could not allocate space for my_msg struct");
                goto fail;
        }
        attrs = msg->attrs;

        if (rnl_parse_msg(info, msg)) {
                LOG_ERR("Could not parse message");
                goto fail;
        }

        ipc_id      = msg->header.dst_ipc_id;
        ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipc_process) {
                LOG_ERR("IPC process %d not found", ipc_id);
                goto fail;
        }

        ASSERT(ipc_process->ops);
        ASSERT(ipc_process->ops->application_register);

        if (ipc_process->ops->application_register(ipc_process->data,
                                                   attrs->app_name,
						   attrs->daf_name))
                goto fail;

        return reg_unreg_resp_free_and_reply(msg,
                                             ipc_id,
                                             0,
                                             info->snd_seq,
                                             info->snd_portid,
                                             true);
 fail:
        return reg_unreg_resp_free_and_reply(msg,
                                             ipc_id,
                                             -1,
                                             info->snd_seq,
                                             info->snd_portid,
                                             true);
}

static int notify_ipcp_unregister_app_request(void *             data,
                                              struct sk_buff *   buff,
                                              struct genl_info * info)
{
        struct kipcm *                          kipcm;
        struct rnl_ipcm_reg_app_req_msg_attrs * attrs;
        struct rnl_msg *                        msg;
        struct ipcp_instance *                  ipc_process;
        ipc_process_id_t                        ipc_id;

        ipc_id = 0;

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }

        kipcm = (struct kipcm *) data;
        if (!info) {
                LOG_ERR("Bogus struct genl_info passed, cannot parse NL msg");
                return -1;
        }

        msg = rnl_msg_create(RNL_MSG_ATTRS_REG_UNREG_REQUEST);
        if (!msg) {
                LOG_ERR("Could not allocate space for my_msg struct");
                goto fail;
        }
        attrs = msg->attrs;

        if (rnl_parse_msg(info, msg)) {
                LOG_ERR("Could not parse message");
                goto fail;
        }

        ipc_id      = msg->header.dst_ipc_id;
        ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipc_process) {
                LOG_ERR("IPC process %d not found", ipc_id);
                goto fail;
        }

        ASSERT(ipc_process->ops);
        ASSERT(ipc_process->ops->application_unregister);

        if (ipc_process->ops->application_unregister(ipc_process->data,
                                                     attrs->app_name))
                goto fail;

        return reg_unreg_resp_free_and_reply(msg,
                                             ipc_id,
                                             0,
                                             info->snd_seq,
                                             info->snd_portid,
                                             false);
 fail:
        return reg_unreg_resp_free_and_reply(msg,
                                             ipc_id,
                                             -1,
                                             info->snd_seq,
                                             info->snd_portid,
                                             true);
}

static int
conn_create_resp_free_and_reply(struct rnl_msg * msg,
                                ipc_process_id_t ipc_id,
                                port_id_t        pid,
                                cep_id_t         src_cep,
                                rnl_sn_t         seq_num,
                                u32              nl_port_id)
{
        rnl_msg_destroy(msg);

        if (rnl_base_response(ipc_id, 0, pid, src_cep,seq_num,
        		      RINA_C_IPCP_CONN_CREATE_RESPONSE,
			      nl_port_id)) {
                LOG_ERR("Could not snd conn_create_resp_msg");
                return -1;
        }

        return 0;
}

static int notify_ipcp_conn_create_req(void *             data,
                                       struct sk_buff *   buff,
                                       struct genl_info * info)
{
        struct rnl_ipcp_conn_create_req_msg_attrs * attrs;
        struct rnl_msg *                            msg;
        struct ipcp_instance *                      ipcp;
        struct kipcm *                              kipcm;
        ipc_process_id_t                            ipc_id;
        ipc_process_id_t                            user_ipc_id;
        port_id_t                                   port_id;
        cep_id_t                                    src_cep;
        struct ipcp_instance *                      user_ipcp;

        ipc_id  = 0;
        port_id = 0;
        src_cep = 0;

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }

        if (!info) {
                LOG_ERR("Bogus struct genl_info passed, cannot parse NL msg");
                return -1;
        }

        kipcm = (struct kipcm *) data;

        msg   = rnl_msg_create(RNL_MSG_ATTRS_CONN_CREATE_REQUEST);
        if (!msg)
                goto fail;

        attrs = msg->attrs;

        if (rnl_parse_msg(info, msg))
                goto fail;

        port_id = attrs->port_id;
        ipc_id  = msg->header.dst_ipc_id;
        ipcp    = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipcp)
                goto fail;

        user_ipc_id = attrs->flow_user_ipc_process_id;
        user_ipcp = kfa_ipcp_instance(kipcm->kfa);
        if (user_ipc_id) {
                user_ipcp = ipcp_imap_find(kipcm->instances, user_ipc_id);
                if (!user_ipcp)
                        goto fail;
        }

        /* IPCP takes ownership of the dtp and dtcp cfg params */
        src_cep = ipcp->ops->connection_create(ipcp->data,
        				       user_ipcp,
                                               attrs->port_id,
                                               attrs->src_addr,
                                               attrs->dst_addr,
                                               attrs->qos_id,
                                               attrs->dtp_cfg,
                                               attrs->dtcp_cfg);

        /* The ownership has been passed to connection_create. */
        attrs->dtp_cfg = NULL;
        attrs->dtcp_cfg = NULL;

        if (!is_cep_id_ok(src_cep)) {
                LOG_ERR("IPC process could not create connection");
                goto fail;
        }

        return conn_create_resp_free_and_reply(msg,
                                               ipc_id,
                                               port_id,
                                               src_cep,
                                               info->snd_seq,
                                               info->snd_portid);

 fail:
        return conn_create_resp_free_and_reply(msg,
                                               ipc_id,
                                               port_id,
                                               cep_id_bad(),
                                               info->snd_seq,
                                               info->snd_portid);
}

/*
 * FIXME: create_req and create_arrived are almost identical, code should be
 *        reused
 */

static int
conn_create_result_free_and_reply(struct rnl_msg * msg,
                                  ipc_process_id_t ipc_id,
                                  port_id_t        pid,
                                  cep_id_t         src_cep,
                                  cep_id_t         dst_cep,
                                  rnl_sn_t         seq_num,
                                  u32              nl_port_id)
{
        rnl_msg_destroy(msg);

        if (rnl_ipcp_conn_create_result_msg(ipc_id,
                                            pid,
                                            src_cep,
                                            dst_cep,
                                            seq_num,
                                            nl_port_id)) {
                LOG_ERR("Could not snd conn_create_result_msg");
                return -1;
        }

        return 0;
}

static int notify_ipcp_conn_create_arrived(void *             data,
                                           struct sk_buff *   buff,
                                           struct genl_info * info)
{
        struct rnl_ipcp_conn_create_arrived_msg_attrs * attrs;
        struct rnl_msg *                                msg;
        struct ipcp_instance *                          ipcp;
        struct kipcm *                                  kipcm;
        ipc_process_id_t                                ipc_id;
        ipc_process_id_t                                user_ipc_id;
        port_id_t                                       port_id;
        cep_id_t                                        src_cep;
        struct ipcp_instance *                          user_ipcp;

        ipc_id  = 0;
        port_id = 0;
        src_cep = 0;

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }

        if (!info) {
                LOG_ERR("Bogus struct genl_info passed, cannot parse NL msg");
                return -1;
        }

        kipcm = (struct kipcm *) data;

        msg = rnl_msg_create(RNL_MSG_ATTRS_CONN_CREATE_ARRIVED);
        if (!msg)
                goto fail;

        attrs = msg->attrs;

        if (rnl_parse_msg(info, msg)) {
                goto fail;
        }

        port_id     = attrs->port_id;
        ipc_id      = msg->header.dst_ipc_id;
        user_ipc_id = attrs->flow_user_ipc_process_id;
        ipcp        = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipcp) {
                goto fail;
        }

        user_ipcp = kfa_ipcp_instance(kipcm->kfa);
        if (user_ipc_id) {
                user_ipcp = ipcp_imap_find(kipcm->instances, user_ipc_id);
                if (!user_ipcp)
                        goto fail;
        }

        src_cep = ipcp->ops->connection_create_arrived(ipcp->data,
                                                       user_ipcp,
                                                       attrs->port_id,
                                                       attrs->src_addr,
                                                       attrs->dst_addr,
                                                       attrs->qos_id,
                                                       attrs->dst_cep,
                                                       attrs->dtp_cfg,
                                                       attrs->dtcp_cfg);

        /* The ownership has been passed to connection_create_arrived. */
        attrs->dtp_cfg = NULL;
        attrs->dtcp_cfg = NULL;

        if (!is_cep_id_ok(src_cep)) {
                LOG_ERR("IPC process could not create connection");
                goto fail;
        }

        return conn_create_result_free_and_reply(msg,
                                                 ipc_id,
                                                 port_id,
                                                 src_cep,
                                                 attrs->dst_cep,
                                                 info->snd_seq,
                                                 info->snd_portid);

 fail:
        return conn_create_result_free_and_reply(msg,
                                                 ipc_id,
                                                 port_id,
                                                 cep_id_bad(),
                                                 cep_id_bad(),
                                                 info->snd_seq,
                                                 info->snd_portid);
}

static int
conn_update_result_free_and_reply(struct rnl_msg * msg,
                                  ipc_process_id_t ipc_id,
                                  uint_t           result,
                                  port_id_t        pid,
                                  rnl_sn_t         seq_num,
                                  u32              nl_port_id)
{
        rnl_msg_destroy(msg);

        if (rnl_base_response(ipc_id, result, pid, 0, seq_num,
        		      RINA_C_IPCP_CONN_UPDATE_RESULT,
			      nl_port_id)) {
                LOG_ERR("Could not snd conn_update_result_msg");
                return -1;
        }

        return 0;
}

static int notify_ipcp_conn_update_req(void *             data,
                                       struct sk_buff *   buff,
                                       struct genl_info * info)
{
        struct rnl_ipcp_conn_update_req_msg_attrs * attrs;
        struct rnl_msg *                            msg;
        struct ipcp_instance *                      ipcp;
        struct kipcm *                              kipcm;
        ipc_process_id_t                            ipc_id;
        port_id_t                                   port_id;

        ipc_id  = 0;
        port_id = 0;

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }

        if (!info) {
                LOG_ERR("Bogus struct genl_info passed, cannot parse NL msg");
                return -1;
        }

        kipcm = (struct kipcm *) data;
        msg   = rnl_msg_create(RNL_MSG_ATTRS_CONN_UPDATE_REQUEST);
        if (!msg)
                goto fail;

        attrs = msg->attrs;

        if (rnl_parse_msg(info, msg))
                goto fail;

        port_id     = attrs->port_id;
        ipc_id      = msg->header.dst_ipc_id;
        ipcp        = ipcp_imap_find(kipcm->instances, ipc_id);

        if (!ipcp)
                goto fail;

        if (ipcp->ops->connection_update(ipcp->data,
                                         port_id,
                                         attrs->src_cep,
                                         attrs->dst_cep))
                goto fail;

        return conn_update_result_free_and_reply(msg,
                                                 ipc_id,
                                                 0,
                                                 port_id,
                                                 info->snd_seq,
                                                 info->snd_portid);

 fail:
        return conn_update_result_free_and_reply(msg,
                                                 ipc_id,
                                                 -1,
                                                 port_id,
                                                 info->snd_seq,
                                                 info->snd_portid);

}

static int
conn_destroy_result_free_and_reply(struct rnl_msg * msg,
                                   ipc_process_id_t ipc_id,
                                   uint_t           result,
                                   port_id_t        pid,
                                   rnl_sn_t         seq_num,
                                   u32              nl_port_id)
{
        rnl_msg_destroy(msg);

        if (rnl_base_response(ipc_id, result, pid, 0, seq_num,
        		      RINA_C_IPCP_CONN_DESTROY_RESULT,
			      nl_port_id)) {
                LOG_ERR("Could not snd conn_destroy_result_msg");
                return -1;
        }

        return 0;
}

static int notify_ipcp_conn_destroy_req(void *             data,
                                        struct sk_buff *   buff,
                                        struct genl_info * info)
{
        struct rnl_ipcm_base_nl_msg_attrs * attrs;
        struct rnl_msg *                    msg;
        struct ipcp_instance *              ipcp;
        struct kipcm *                      kipcm;
        ipc_process_id_t                    ipc_id;
        port_id_t                           port_id;

        ipc_id  = 0;
        port_id = 0;

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }

        if (!info) {
                LOG_ERR("Bogus struct genl_info passed, cannot parse NL msg");
                return -1;
        }

        kipcm = (struct kipcm *) data;
        msg   = rnl_msg_create(RNL_MST_ATTRS_BASE_NL_MESSAGE);
        if (!msg)
                goto fail;

        attrs = msg->attrs;

        if (rnl_parse_msg(info, msg))
                goto fail;

        port_id = attrs->port_id;
        ipc_id  = msg->header.dst_ipc_id;
        ipcp    = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipcp)
                goto fail;

        if (ipcp->ops->connection_destroy(ipcp->data, attrs->cep_id))
                goto fail;

        return conn_destroy_result_free_and_reply(msg,
                                                  ipc_id,
                                                  0,
                                                  port_id,
                                                  info->snd_seq,
                                                  info->snd_portid);

 fail:
        return conn_destroy_result_free_and_reply(msg,
                                                  ipc_id,
                                                  -1,
                                                  port_id,
                                                  info->snd_seq,
                                                  info->snd_portid);
}

static int notify_ipc_manager_present(void *             data,
                                      struct sk_buff *   buff,
                                      struct genl_info * info)
{
        LOG_INFO("IPC Manager started. It is listening at NL port-id %d",
                 info->snd_portid);

        rnl_set_ipc_manager_port(info->snd_portid);

        return 0;
}

static int notify_ipcp_modify_pffe(void *             data,
                                   struct sk_buff *   buff,
                                   struct genl_info * info)
{
        struct kipcm *                      kipcm;
        struct rnl_rmt_mod_pffe_msg_attrs * attrs;
        struct rnl_msg *                    msg;
        struct ipcp_instance *              ipc_process;
        ipc_process_id_t                    ipc_id;
        struct mod_pff_entry *              entry;
        int				    result;

        int (* op)(struct ipcp_instance_data * data,
		   struct mod_pff_entry      * entry);

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }

        kipcm = (struct kipcm *) data;

        if (!info) {
                LOG_ERR("Bogus struct genl_info passed, cannot parse NL msg");
                return -1;
        }

        ipc_id = 0;
        msg    = rnl_msg_create(RNL_MSG_ATTRS_RMT_PFFE_MODIFY_REQUEST);
        if (!msg) {
                rnl_msg_destroy(msg);
                return -1;
        }

        attrs = msg->attrs;

        if (rnl_parse_msg(info, msg)) {
                rnl_msg_destroy(msg);
                return -1;
        }

        ipc_id      = msg->header.dst_ipc_id;
        ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipc_process) {
                LOG_ERR("IPC process %d not found", ipc_id);
                rnl_msg_destroy(msg);
                return -1;
        }

        switch(attrs->mode) {
        case 2:
        	result = ipc_process->ops->pff_modify(ipc_process->data, &attrs->pff_entries);
        	if (result)
                        LOG_ERR("Problems modifying PFF");

        	rnl_msg_destroy(msg);
        	return result;
        case 1:
                op = ipc_process->ops->pff_remove;
                break;
        case 0:
                op = ipc_process->ops->pff_add;
                break;
        default:
                LOG_ERR("Unknown mode for modify PFF operation %d",
                        attrs->mode);
                rnl_msg_destroy(msg);
                return -1;
                break;
        }

        ASSERT(op);
        list_for_each_entry(entry, &attrs->pff_entries, next) {
                ASSERT(entry);

                if (op(ipc_process->data, entry)) {
                        LOG_ERR("There were some problematic entries");
                        rnl_msg_destroy(msg);
                        return -1;
                }
        }

        rnl_msg_destroy(msg);

        return 0;
}

static int ipcp_dump_pff_free_and_reply(struct rnl_msg *   msg,
                                        ipc_process_id_t   ipc_id,
                                        uint_t             result,
                                        struct list_head * entries,
                                        rnl_sn_t           seq_num,
                                        u32                nl_port_id)
{
        rnl_msg_destroy(msg);

        if (rnl_ipcp_pff_dump_resp_msg(ipc_id,
                                       result,
                                       entries,
                                       seq_num,
                                       nl_port_id)) {
                LOG_ERR("Could not snd ipcp_pff_dump_resp_msg");
                return -1;
        }

        return 0;
}

static int notify_ipcp_dump_pff(void *             data,
                                struct sk_buff *   buff,
                                struct genl_info * info)
{
        struct kipcm *         kipcm;
        struct rnl_msg *       msg;
        struct ipcp_instance * ipc_process;
        ipc_process_id_t       ipc_id = 0;
        int                    result = -1;
        struct list_head       entries;

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }

        kipcm = (struct kipcm *) data;

        if (!info) {
                LOG_ERR("Bogus struct genl_info passed, cannot parse NL msg");
                return -1;
        }

        ipc_id = 0;
        msg    = rnl_msg_create(RNL_MSG_ATTRS_RMT_PFF_DUMP_REQUEST);
        if (!msg)
                goto end;

        if (rnl_parse_msg(info, msg))
                goto end;

        ipc_id      = msg->header.dst_ipc_id;
        ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipc_process) {
                LOG_ERR("IPC process %d not found", ipc_id);
                goto end;
        }

        INIT_LIST_HEAD(&entries);
        if (ipc_process->ops->pff_dump(ipc_process->data,
                                       &entries)) {
                LOG_ERR("Could not dump PFF");
                goto end;
        }

        result = 0;

 end:
        return ipcp_dump_pff_free_and_reply(msg,
                                            ipc_id,
                                            result,
                                            &entries,
                                            info->snd_seq,
                                            info->snd_portid);
}

static int ipcm_query_rib_free_and_reply(struct rnl_msg *   msg,
                                         ipc_process_id_t   ipc_id,
                                         uint_t             result,
                                         struct list_head * entries,
                                         rnl_sn_t           seq_num,
                                         u32                nl_port_id)
{
        rnl_msg_destroy(msg);

        if (rnl_ipcm_query_rib_resp_msg(ipc_id,
                                        result,
                                        entries,
                                        seq_num,
                                        nl_port_id)) {
                LOG_ERR("Could not send ipcm_query_rib_resp_msg");
                return -1;
        }

        return 0;
}

static int notify_ipcm_query_rib(void *             data,
                                struct sk_buff *   buff,
                                struct genl_info * info)
{
        struct kipcm *         kipcm;
        struct rnl_msg *       msg;
        struct rnl_ipcm_query_rib_msg_attrs * attrs;
        struct ipcp_instance * ipc_process;
        ipc_process_id_t       ipc_id = 0;
        int                    result = -1;
        struct list_head       entries;

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }

        kipcm = (struct kipcm *) data;

        if (!info) {
                LOG_ERR("Bogus struct genl_info passed, cannot parse NL msg");
                return -1;
        }

        ipc_id = 0;
        msg    = rnl_msg_create(RNL_MSG_ATTRS_QUERY_RIB_REQUEST);
        if (!msg)
                goto end;

        attrs = msg->attrs;

        if (rnl_parse_msg(info, msg))
                goto end;

        ipc_id      = msg->header.dst_ipc_id;
        ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipc_process) {
                LOG_ERR("IPC process %d not found", ipc_id);
                goto end;
        }

        INIT_LIST_HEAD(&entries);
        if (ipc_process->ops->query_rib(ipc_process->data,
        		                &entries,
        		                attrs->object_class,
        		                attrs->object_name,
        		                attrs->object_instance,
        	 	                attrs->scope,
        		                attrs->filter)) {
        	LOG_ERR("Could not query RIB, unsupported operation");
        	goto end;
        }

        result = 0;

 end:
        return ipcm_query_rib_free_and_reply(msg,
                                             ipc_id,
                                             result,
                                             &entries,
                                             info->snd_seq,
                                             info->snd_portid);
}

static int notify_ipcp_set_policy_set_param(void *             data,
                                            struct sk_buff *   buff,
                                            struct genl_info * info)
{
        struct kipcm *                                       kipcm = data;
        struct rnl_ipcp_set_policy_set_param_req_msg_attrs * attrs;
        struct rnl_msg *                                     msg;
        struct ipcp_instance *                               ipc_process;
        ipc_process_id_t                                     ipc_id = 0;
        int retval = 0;

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }

        if (!info) {
                LOG_ERR("Bogus struct genl_info passed, cannot parse NL msg");
                return -1;
        }

        msg = rnl_msg_create(RNL_MSG_ATTRS_SET_POLICY_SET_PARAM_REQUEST);
        if (!msg) {
                retval = -1;
                goto out;
        }

        attrs = msg->attrs;

        if (rnl_parse_msg(info, msg)) {
                retval = -1;
                goto out;
        }

        ipc_id      = msg->header.dst_ipc_id;
        ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipc_process) {
                LOG_ERR("IPC process %d not found", ipc_id);
                retval = -1;
                goto out;
        }
        LOG_DBG("Found IPC Process with id %d", ipc_id);

        ASSERT(ipc_process->ops);
        if (ipc_process->ops->set_policy_set_param) {
                retval = ipc_process->ops->set_policy_set_param(
                                ipc_process->data, attrs->path,
                                attrs->name, attrs->value);
                if (retval) {
                        LOG_ERR("set-policy-set-param operation failed");
                }
        } else {
                retval = -1;
                LOG_ERR("IPC process %d does not support policy "
                        "parameters", ipc_id);
        }

        LOG_DBG("set-policy-set-param request served %s %s %s",
                attrs->path, attrs->name, attrs->value);
out:
        rnl_msg_destroy(msg);

        if (rnl_base_response(ipc_id, retval, 0, 0, info->snd_seq,
        		      RINA_C_IPCP_SET_POLICY_SET_PARAM_RESPONSE,
                              info->snd_portid))
                return -1;

        return 0;
}

static int notify_ipcp_select_policy_set(void *             data,
                                         struct sk_buff *   buff,
                                         struct genl_info * info)
{
        struct kipcm *                                    kipcm = data;
        struct rnl_ipcp_select_policy_set_req_msg_attrs * attrs;
        struct rnl_msg *                                  msg;
        struct ipcp_instance *                            ipc_process;
        ipc_process_id_t                                  ipc_id = 0;
        int retval = 0;

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }

        if (!info) {
                LOG_ERR("Bogus struct genl_info passed, cannot parse NL msg");
                return -1;
        }

        msg = rnl_msg_create(RNL_MSG_ATTRS_SELECT_POLICY_SET_REQUEST);
        if (!msg) {
                retval = -1;
                goto out;
        }

        attrs = msg->attrs;

        if (rnl_parse_msg(info, msg)) {
                retval = -1;
                goto out;
        }

        ipc_id      = msg->header.dst_ipc_id;
        ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipc_process) {
                LOG_ERR("IPC process %d not found", ipc_id);
                retval = -1;
                goto out;
        }
        LOG_DBG("Found IPC Process with id %d", ipc_id);

        ASSERT(ipc_process->ops);
        if (ipc_process->ops->select_policy_set) {
                retval = ipc_process->ops->select_policy_set(
                                ipc_process->data, attrs->path,
                                attrs->name);
                if (retval) {
                        LOG_ERR("select-policy-set operation failed");
                }
        } else {
                retval = -1;
                LOG_ERR("IPC process %d does not support policies", ipc_id);
        }

        LOG_DBG("select-policy-set request served %s %s",
                attrs->path, attrs->name);
out:
        rnl_msg_destroy(msg);

        if (rnl_base_response(ipc_id, retval, 0, 0, info->snd_seq,
        		      RINA_C_IPCP_SELECT_POLICY_SET_RESPONSE,
			      info->snd_portid))
                return -1;

        return 0;
}

static int notify_ipcp_update_crypto_state(void *             data,
                                           struct sk_buff *   buff,
                                           struct genl_info * info)
{
        struct kipcm * kipcm = data;
        struct rnl_ipcp_update_crypto_state_req_msg_attrs * attrs;
        struct rnl_msg * msg;
        struct ipcp_instance * ipc_process;
        ipc_process_id_t ipc_id = 0;
        int retval = 0;
        port_id_t port_id = 0;

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }

        if (!info) {
                LOG_ERR("Bogus struct genl_info passed, cannot parse NL msg");
                return -1;
        }

        msg = rnl_msg_create(RNL_MSG_ATTRS_UPDATE_CRYPTO_STATE_REQUEST);
        if (!msg) {
                retval = -1;
                goto out;
        }

        attrs = msg->attrs;

        if (rnl_parse_msg(info, msg)) {
                retval = -1;
                goto out;
        }

        port_id = attrs->port_id;
        ipc_id      = msg->header.dst_ipc_id;
        ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipc_process) {
                LOG_ERR("IPC process %d not found", ipc_id);
                retval = -1;
                goto out;
        }
        LOG_DBG("Found IPC Process with id %d", ipc_id);

        ASSERT(ipc_process->ops);
        if (ipc_process->ops->update_crypto_state) {
                retval = ipc_process->ops->update_crypto_state(ipc_process->data,
                                			       attrs->state,
                                			       attrs->port_id);
                if (retval) {
                        LOG_ERR("Enable encryption operation failed");
                }
        } else {
                retval = -1;
                LOG_ERR("IPC process %d does not support enabling encryption", ipc_id);
        }

        LOG_DBG("enable encryption request served");
out:
        rnl_msg_destroy(msg);

        if (rnl_base_response(ipc_id, retval, port_id, 0, info->snd_seq,
        		      RINA_C_IPCP_UPDATE_CRYPTO_STATE_RESPONSE,
			      info->snd_portid))
                return -1;

        return 0;
}

static int notify_ipcp_address_change(void *             data,
                                      struct sk_buff *   buff,
                                      struct genl_info * info)
{
        struct kipcm * kipcm = data;
        struct rnl_ipcp_address_change_req_msg_attrs * attrs;
        struct rnl_msg * msg;
        struct ipcp_instance * ipc_process;
        ipc_process_id_t ipc_id = 0;
        int retval = 0;

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }

        if (!info) {
                LOG_ERR("Bogus struct genl_info passed, cannot parse NL msg");
                return -1;
        }

        msg = rnl_msg_create(RNL_MSG_ATTRS_ADDRESS_CHANGE_REQUEST);
        if (!msg) {
                retval = -1;
                goto out;
        }

        attrs = msg->attrs;

        if (rnl_parse_msg(info, msg)) {
                retval = -1;
                goto out;
        }

        ipc_id      = msg->header.dst_ipc_id;
        ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipc_process) {
                LOG_ERR("IPC process %d not found", ipc_id);
                retval = -1;
                goto out;
        }
        LOG_DBG("Found IPC Process with id %d", ipc_id);

        ASSERT(ipc_process->ops);
        if (ipc_process->ops->address_change) {
                retval = ipc_process->ops->address_change(ipc_process->data,
                                			  attrs->new_address,
                                			  attrs->old_address,
							  attrs->use_new_timeout,
							  attrs->deprecate_old_timeout);
                if (retval) {
                        LOG_ERR("Address change operation failed");
                }
        } else {
                retval = -1;
                LOG_ERR("IPC process %d does not support changing addresses",
                	ipc_id);
        }

        LOG_DBG("Address change request served");
out:
        rnl_msg_destroy(msg);

        if (retval)
        	LOG_ERR("Error processing address change request");

        return 0;
}

static int notify_allocate_port(void *             data,
				struct sk_buff *   buff,
				struct genl_info * info)
{
        struct kipcm * kipcm = data;
        struct rnl_ipcp_allocate_port_req_msg_attrs * attrs;
        struct rnl_msg * msg;
        int retval = 0;
        ipc_process_id_t ipcp_id = 0;
        port_id_t port_id = 0;

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }

        if (!info) {
                LOG_ERR("Bogus struct genl_info passed, cannot parse NL msg");
                return -1;
        }

        msg = rnl_msg_create(RNL_MSG_ATTRS_ALLOCATE_PORT_REQUEST);
        if (!msg) {
                retval = -1;
                goto out;
        }

        attrs = msg->attrs;

        if (rnl_parse_msg(info, msg)) {
                retval = -1;
                goto out;
        }

        ipcp_id = msg->header.src_ipc_id;
        port_id = kipcm_flow_create(kipcm,
        		      	    msg->header.src_ipc_id,
				    attrs->app_name);
        if (port_id == port_id_bad())
        	retval = -1;
out:
        rnl_msg_destroy(msg);

        if (rnl_base_response(ipcp_id, retval, port_id, 0, info->snd_seq,
        		      RINA_C_IPCP_ALLOCATE_PORT_RESPONSE,
			      info->snd_portid))
                return -1;

        return 0;
}

static int notify_deallocate_port(void *             data,
				  struct sk_buff *   buff,
				  struct genl_info * info)
{
        struct kipcm * kipcm = data;
        struct rnl_ipcm_base_nl_msg_attrs * attrs;
        struct rnl_msg * msg;
        int retval = 0;
        ipc_process_id_t ipcp_id = 0;
        port_id_t port_id = 0;

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }

        if (!info) {
                LOG_ERR("Bogus struct genl_info passed, cannot parse NL msg");
                return -1;
        }

        msg = rnl_msg_create(RNL_MST_ATTRS_BASE_NL_MESSAGE);
        if (!msg) {
                retval = -1;
                goto out;
        }

        attrs = msg->attrs;

        if (rnl_parse_msg(info, msg)) {
                retval = -1;
                goto out;
        }

        ipcp_id = msg->header.src_ipc_id;
        port_id = attrs->port_id;
        retval = kipcm_flow_destroy(kipcm,
        			    ipcp_id,
				    attrs->port_id);
out:
        rnl_msg_destroy(msg);

        if (rnl_base_response(ipcp_id, retval, port_id, 0, info->snd_seq,
        		      RINA_C_IPCP_DEALLOCATE_PORT_RESPONSE,
			      info->snd_portid))
                return -1;

        return 0;
}

static int notify_ipcp_write_mgmt_sdu(void *             data,
				      struct sk_buff *   buff,
				      struct genl_info * info)
{
        struct kipcm * kipcm = data;
        struct rnl_ipcp_write_mgmt_sdu_req_msg_attrs * attrs;
        struct rnl_msg * msg;
        int retval = 0;
        ipc_process_id_t ipcp_id = 0;

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }

        if (!info) {
                LOG_ERR("Bogus struct genl_info passed, cannot parse NL msg");
                return -1;
        }

        msg = rnl_msg_create(RNL_MSG_ATTRS_WRITE_MGMT_SDU_REQUEST);
        if (!msg) {
                retval = -1;
                LOG_ERR("Problems creating msg attributes");
                goto out;
        }

        attrs = msg->attrs;

        if (rnl_parse_msg(info, msg)) {
                retval = -1;
                LOG_ERR("Problems parsing NL message");
                goto out;
        }

        ipcp_id = msg->header.src_ipc_id;
        retval = kipcm_mgmt_sdu_write(kipcm,
        			      ipcp_id,
				      attrs->sdu_wpi);
out:
        rnl_msg_destroy(msg);

        if (rnl_base_response(ipcp_id, retval, 0, 0, info->snd_seq,
        		      RINA_C_IPCP_MANAGEMENT_SDU_WRITE_RESPONSE,
			      info->snd_portid))
                return -1;

        return 0;
}

static int notify_create_ipcp(void *             data,
			      struct sk_buff *   buff,
			      struct genl_info * info)
{
        struct kipcm * kipcm = data;
        struct rnl_create_ipcp_req_msg_attrs * attrs;
        struct rnl_msg * msg;
        int retval = 0;

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }

        if (!info) {
                LOG_ERR("Bogus struct genl_info passed, cannot parse NL msg");
                return -1;
        }

        msg = rnl_msg_create(RNL_MSG_ATTRS_CREATE_IPCP_REQUEST);
        if (!msg) {
                retval = -1;
                goto out;
        }

        attrs = msg->attrs;

        if (rnl_parse_msg(info, msg)) {
                retval = -1;
                goto out;
        }

        retval = kipcm_ipc_create(kipcm,
        			  attrs->ipcp_name,
				  attrs->ipcp_id,
				  attrs->nl_port_id,
				  attrs->dif_type);
out:
        rnl_msg_destroy(msg);

        if (rnl_base_response(0, retval, 0, 0, info->snd_seq,
        		      RINA_C_IPCM_CREATE_IPCP_RESPONSE,
			      info->snd_portid))
                return -1;

        return 0;
}

static int notify_destroy_ipcp(void *             data,
			       struct sk_buff *   buff,
			       struct genl_info * info)
{
        struct kipcm * kipcm = data;
        struct rnl_destroy_ipcp_req_msg_attrs * attrs;
        struct rnl_msg * msg;
        int retval = 0;

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }

        if (!info) {
                LOG_ERR("Bogus struct genl_info passed, cannot parse NL msg");
                return -1;
        }

        msg = rnl_msg_create(RNL_MSG_ATTRS_DESTROY_IPCP_REQUEST);
        if (!msg) {
                retval = -1;
                goto out;
        }

        attrs = msg->attrs;

        if (rnl_parse_msg(info, msg)) {
                retval = -1;
                goto out;
        }

        retval = kipcm_ipc_destroy(kipcm,
				   attrs->ipcp_id);
out:
        rnl_msg_destroy(msg);

        if (rnl_base_response(0, retval, 0, 0, info->snd_seq,
        		      RINA_C_IPCM_DESTROY_IPCP_RESPONSE,
			      info->snd_portid))
                return -1;

        return 0;
}

static int ctrldev_handlers_unregister()
{
        int retval = 0;

        if (irati_handler_unregister(RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST))
        	retval = -1;
        if (irati_handler_unregister(RINA_C_IPCM_ALLOCATE_FLOW_REQUEST))
        	retval = -1;
        if (irati_handler_unregister(RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE))
        	retval = -1;
        if (irati_handler_unregister(RINA_C_IPCM_REGISTER_APPLICATION_REQUEST))
        	retval = -1;
        if (irati_handler_unregister(RINA_C_IPCM_UNREGISTER_APPLICATION_REQUEST))
        	retval = -1;
        if (irati_handler_unregister(RINA_C_IPCM_DEALLOCATE_FLOW_REQUEST))
        	retval = -1;
        if (irati_handler_unregister(RINA_C_IPCM_IPC_MANAGER_PRESENT))
        	retval = -1;
        if (irati_handler_unregister(RINA_C_IPCM_UPDATE_DIF_CONFIG_REQUEST))
        	retval = -1;
        if (irati_handler_unregister(RINA_C_IPCP_CONN_CREATE_REQUEST))
        	retval = -1;
        if (irati_handler_unregister(RINA_C_IPCP_CONN_CREATE_ARRIVED))
        	retval = -1;
        if (irati_handler_unregister(RINA_C_IPCP_CONN_UPDATE_REQUEST))
        	retval = -1;
        if (irati_handler_unregister(RINA_C_IPCP_CONN_DESTROY_REQUEST))
        	retval = -1;
        if (irati_handler_unregister(RINA_C_RMT_MODIFY_FTE_REQUEST))
        	retval = -1;
        if (irati_handler_unregister(RINA_C_RMT_DUMP_FT_REQUEST))
        	retval = -1;
        if (irati_handler_unregister(RINA_C_IPCM_QUERY_RIB_REQUEST))
        	retval = -1;
        if (irati_handler_unregister(RINA_C_IPCP_SET_POLICY_SET_PARAM_REQUEST))
        	retval = -1;
        if (irati_handler_unregister(RINA_C_IPCP_SELECT_POLICY_SET_REQUEST))
        	retval = -1;
        if (irati_handler_unregister(RINA_C_IPCP_UPDATE_CRYPTO_STATE_REQUEST))
        	retval = -1;
        if (irati_handler_unregister(RINA_C_IPCP_ADDRESS_CHANGE_REQUEST))
        	retval = -1;
        if (irati_handler_unregister(RINA_C_IPCP_ALLOCATE_PORT_REQUEST))
        	retval = -1;
        if (irati_handler_unregister(RINA_C_IPCP_DEALLOCATE_PORT_REQUEST))
        	retval = -1;
        if (irati_handler_unregister(RINA_C_IPCP_MANAGEMENT_SDU_WRITE_REQUEST))
        	retval = -1;
        if (irati_handler_unregister(RINA_C_IPCM_CREATE_IPCP_REQUEST))
        	retval = -1;
        if (irati_handler_unregister(RINA_C_IPCM_DESTROY_IPCP_REQUEST))
        	retval = -1;

        LOG_INFO("Ctrl-dev handlers unregistered %s",
                 (retval == 0) ? "successfully" : "unsuccessfully");

        return retval;
}

static int ctrldev_handlers_register(struct kipcm * kipcm)
{
        int i,j;

        kipcm_handlers[RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST]          =
                notify_ipcp_assign_dif_request;
        kipcm_handlers[RINA_C_IPCM_ALLOCATE_FLOW_REQUEST]          =
                notify_ipcp_allocate_flow_request;
        kipcm_handlers[RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE]         =
                notify_ipcp_allocate_flow_response;
        kipcm_handlers[RINA_C_IPCM_REGISTER_APPLICATION_REQUEST]   =
                notify_ipcp_register_app_request;
        kipcm_handlers[RINA_C_IPCM_UNREGISTER_APPLICATION_REQUEST] =
                notify_ipcp_unregister_app_request;
        kipcm_handlers[RINA_C_IPCM_DEALLOCATE_FLOW_REQUEST]        =
                notify_ipcp_deallocate_flow_request;
        kipcm_handlers[RINA_C_IPCM_IPC_MANAGER_PRESENT]            =
                notify_ipc_manager_present;
        kipcm_handlers[RINA_C_IPCM_UPDATE_DIF_CONFIG_REQUEST]      =
                notify_ipcp_update_dif_config_request;
        kipcm_handlers[RINA_C_IPCP_CONN_CREATE_REQUEST]            =
                notify_ipcp_conn_create_req;
        kipcm_handlers[RINA_C_IPCP_CONN_CREATE_ARRIVED]            =
                notify_ipcp_conn_create_arrived;
        kipcm_handlers[RINA_C_IPCP_CONN_UPDATE_REQUEST]            =
                notify_ipcp_conn_update_req;
        kipcm_handlers[RINA_C_IPCP_CONN_DESTROY_REQUEST]           =
                notify_ipcp_conn_destroy_req;
        kipcm_handlers[RINA_C_RMT_MODIFY_FTE_REQUEST]              =
                notify_ipcp_modify_pffe;
        kipcm_handlers[RINA_C_RMT_DUMP_FT_REQUEST]                 =
                notify_ipcp_dump_pff;
        kipcm_handlers[RINA_C_IPCM_QUERY_RIB_REQUEST]      	   =
        	notify_ipcm_query_rib;
        kipcm_handlers[RINA_C_IPCP_SET_POLICY_SET_PARAM_REQUEST]   =
                notify_ipcp_set_policy_set_param;
        kipcm_handlers[RINA_C_IPCP_SELECT_POLICY_SET_REQUEST]      =
                notify_ipcp_select_policy_set;
        kipcm_handlers[RINA_C_IPCP_UPDATE_CRYPTO_STATE_REQUEST]    =
                notify_ipcp_update_crypto_state;
        kipcm_handlers[RINA_C_IPCP_ADDRESS_CHANGE_REQUEST]    	   =
                notify_ipcp_address_change;
        kipcm_handlers[RINA_C_IPCP_ALLOCATE_PORT_REQUEST]    	   =
                notify_allocate_port;
        kipcm_handlers[RINA_C_IPCP_DEALLOCATE_PORT_REQUEST]    	   =
                notify_deallocate_port;
        kipcm_handlers[RINA_C_IPCP_MANAGEMENT_SDU_WRITE_REQUEST]   =
                notify_ipcp_write_mgmt_sdu;
        kipcm_handlers[RINA_C_IPCM_CREATE_IPCP_REQUEST]   	   =
                notify_create_ipcp;
        kipcm_handlers[RINA_C_IPCM_DESTROY_IPCP_REQUEST]   	   =
                notify_destroy_ipcp;

        for (i = 1; i < RINA_C_MAX; i++) {
                if (kipcm_handlers[i] != NULL) {
                        if (irati_handler_register(i,
                        			   kipcm_handlers[i],
                                                   kipcm)) {
                                for (j = i - 1; j > 0; j--) {
                                        if (kipcm_handlers[j] != NULL) {
                                                if (irati_handler_unregister(j)) {
                                                        LOG_ERR("Failed handler unregister while bailing out");
                                                        /* FIXME: What else could be done here?" */
                                                }
                                        }
                                }

                                return -1;
                        }
                }
        }

        LOG_DBG("Ctrl-dev handlers registered successfully");
        return 0;
}

int kipcm_init(struct robject * parent)
{
        struct kipcm * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return -1;

        tmp->factories = ipcpf_init(parent);
        if (!tmp->factories) {
                LOG_ERR("Failed to build factories");
		rnl_set_destroy(tmp->rnls);
                rkfree(tmp);
                return -1;
        }

        tmp->instances = ipcp_imap_create();
        if (!tmp->instances) {
                LOG_ERR("Failed to build imap");
                if (ipcpf_fini(tmp->factories)) {
                        /* FIXME: What could we do here ? */
                }
                rkfree(tmp);
                return -1;
        }

        tmp->messages = rkzalloc(sizeof(struct flow_messages), GFP_KERNEL);
        if (!tmp->messages) {
                LOG_ERR("Failed to build flow maps");
                if (ipcpf_fini(tmp->factories)) {
                        /* FIXME: What could we do here ? */
                }
                if (ipcp_imap_destroy(tmp->instances)) {
                        /* FIXME: What could we do here ? */
                }
                rkfree(tmp);
                return -1;
        }

        tmp->messages->ingress = kipcm_pmap_create();
        tmp->messages->egress  = kipcm_smap_create();
        if (!tmp->messages->ingress || !tmp->messages->egress) {
                if (tmp->messages->ingress)
                        if (kipcm_pmap_destroy(tmp->messages->ingress)) {
                                /* FIXME: What could we do here ? */
                        }

                if (tmp->messages->egress)
                        if (kipcm_smap_destroy(tmp->messages->egress)) {
                                /* FIXME: What could we do here ? */
                        }
                rkfree(tmp);
                return -1;
        }

        tmp->kfa = kfa_create();
        if (!tmp->kfa) {
                if (kipcm_pmap_destroy(tmp->messages->ingress)) {
                        /* FIXME: What could we do here ? */
                }
                if (kipcm_smap_destroy(tmp->messages->egress)) {
                        /* FIXME: What could we do here ? */
                }
                if (ipcp_imap_destroy(tmp->instances)) {
                        /* FIXME: What could we do here ? */
                }
                if (ipcpf_fini(tmp->factories)) {
                        /* FIXME: What could we do here ? */
                }
                rkfree(tmp);
                return -1;
        }

        if (ctrldev_handlers_register(tmp)) {
                if (kipcm_pmap_destroy(tmp->messages->ingress)) {
                        /* FIXME: What could we do here ? */
                }
                if (kipcm_smap_destroy(tmp->messages->egress)) {
                        /* FIXME: What could we do here ? */
                }
                if (ipcp_imap_destroy(tmp->instances)) {
                        /* FIXME: What could we do here ? */
                }
                if (kfa_destroy(tmp->kfa)) {
                        /* FIXME: What could we do here ? */
                }
                if (ipcpf_fini(tmp->factories)) {
                        /* FIXME: What could we do here ? */
                }
                rkfree(tmp);
                return -1;
        }

	tmp->rset = rset_create_and_add("ipcps", parent);
	if (!tmp->rset) {
		LOG_ERR("Could not initialize IPCP sys entry");
        	if (ctrldev_handlers_unregister()) {
        	        /* FIXME: What should we do here ? */
        	}
                if (kipcm_pmap_destroy(tmp->messages->ingress)) {
                        /* FIXME: What could we do here ? */
                }
                if (kipcm_smap_destroy(tmp->messages->egress)) {
                        /* FIXME: What could we do here ? */
                }
                if (ipcp_imap_destroy(tmp->instances)) {
                        /* FIXME: What could we do here ? */
                }
                if (kfa_destroy(tmp->kfa)) {
                        /* FIXME: What could we do here ? */
                }
                if (ipcpf_fini(tmp->factories)) {
                        /* FIXME: What could we do here ? */
                }
                rkfree(tmp);
                return -1;
	}

        KIPCM_LOCK_INIT(tmp);

        LOG_DBG("Initialized successfully");

	default_kipcm = tmp;
	return 0;
}

int kipcm_fini(struct kipcm * kipcm)
{
        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        KIPCM_LOCK(kipcm);

        if (kfa_destroy(kipcm->kfa)) {
                /* FIXME: What could we do here ? */
        }

        /* FIXME: Destroy all the instances */
        ASSERT(ipcp_imap_empty(kipcm->instances));
        if (ipcp_imap_destroy(kipcm->instances)) {
                /* FIXME: What should we do here ? */
        }

        if (ipcpf_fini(kipcm->factories)) {
                /* FIXME: What should we do here ? */
        }

        ASSERT(kipcm_pmap_empty(kipcm->messages->ingress));
        kipcm_pmap_destroy(kipcm->messages->ingress);

        ASSERT(kipcm_smap_empty(kipcm->messages->egress));
        kipcm_smap_destroy(kipcm->messages->egress);

        if (ctrldev_handlers_unregister()) {
                /* FIXME: What should we do here ? */
        }

        KIPCM_UNLOCK(kipcm);

        KIPCM_LOCK_FINI(kipcm);

	rset_unregister(kipcm->rset);
        rkfree(kipcm);
	default_kipcm = NULL;

        LOG_INFO("KIPCM finalized successfully");

        return 0;
}

struct ipcp_factory *
kipcm_ipcp_factory_register(struct kipcm *             kipcm,
                            const  char *              name,
                            struct ipcp_factory_data * data,
                            struct ipcp_factory_ops *  ops)
{
        struct ipcp_factory * retval;

        IRQ_BARRIER;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return NULL;
        }

        KIPCM_LOCK(kipcm);
        retval = ipcpf_register(kipcm->factories, name, data, ops);
        KIPCM_UNLOCK(kipcm);

        return retval;
}
EXPORT_SYMBOL(kipcm_ipcp_factory_register);

int kipcm_ipcp_factory_unregister(struct kipcm *        kipcm,
                                  struct ipcp_factory * factory)
{
        ipc_process_id_t       id;
        struct ipcp_instance * instance;
        int                    retval;

        IRQ_BARRIER;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        if (!factory) {
                LOG_ERR("Bogus factory cannot unregister");
                return -1;
        }

        /*
         * FIXME:
         *
         *   We have to do the body of kipcm_ipcp_destroy() on all the
         *   instances remaining (and not explicitly destroyed), previously
         *   created with the factory being unregisterd ...
         *
         *     Francesco
         */
        KIPCM_LOCK(kipcm);

        id = ipcp_imap_find_factory(kipcm->instances, factory);
        while (id != 0) {
                instance = ipcp_imap_find(kipcm->instances, id);
                if (!instance) {
                        LOG_ERR("IPC process %d instance does not exist", id);
                        KIPCM_UNLOCK(kipcm);
                        return -1;
                }

                if (factory->ops->destroy(factory->data, instance)) {
                        KIPCM_UNLOCK(kipcm);
                        return -1;
                }

                if (ipcp_imap_remove(kipcm->instances, id)) {
                        KIPCM_UNLOCK(kipcm);
                        return -1;
                }
                id = ipcp_imap_find_factory(kipcm->instances, factory);
        }
        retval = ipcpf_unregister(kipcm->factories, factory);
        KIPCM_UNLOCK(kipcm);

        return retval;
}
EXPORT_SYMBOL(kipcm_ipcp_factory_unregister);

int kipcm_ipc_create(struct kipcm *      kipcm,
                     const struct name * ipcp_name,
                     ipc_process_id_t    id,
		     uint_t		 us_nl_port,
                     const char *        factory_name)
{
        char *                 name;
        struct ipcp_factory *  factory;
        struct ipcp_instance * instance;

        IRQ_BARRIER;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        if (!name_is_ok(ipcp_name)) {
                LOG_ERR("Bad IPC process name");
                return -1;
        }

        ASSERT(ipcp_name);

        name = name_tostring(ipcp_name);
        if (!name)
                return -1;

        if (!factory_name)
                factory_name = DEFAULT_FACTORY;

        ASSERT(factory_name);

        LOG_DBG("Creating IPC process:");
        LOG_DBG("  name:       %s", name);
        LOG_DBG("  id:         %d", id);
        LOG_DBG("  us nl port; %d", us_nl_port);
        LOG_DBG("  factory:    %s", factory_name);
        rkfree(name);

        KIPCM_LOCK(kipcm);
        if (ipcp_imap_find(kipcm->instances, id)) {
                LOG_ERR("Process id %d already exists", id);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        factory = ipcpf_find(kipcm->factories, factory_name);
        if (!factory) {
                LOG_ERR("Cannot find factory '%s'", factory_name);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        instance = factory->ops->create(factory->data, ipcp_name,
        				id, us_nl_port);
        if (!instance) {
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        /* FIXME: The following fixups are "ugly as hell" (TM) */
        instance->factory = factory;

        if (ipcp_imap_add(kipcm->instances, id, instance)) {
                factory->ops->destroy(factory->data, instance);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        KIPCM_UNLOCK(kipcm);

        return 0;
}

int kipcm_ipc_destroy(struct kipcm *   kipcm,
                      ipc_process_id_t id)
{
        struct ipcp_instance * instance;
        struct ipcp_factory *  factory;

        IRQ_BARRIER;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        KIPCM_LOCK(kipcm);

        instance = ipcp_imap_find(kipcm->instances, id);
        if (!instance) {
                LOG_ERR("IPC process %d instance does not exist", id);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        factory = instance->factory;
        ASSERT(factory);

        if (factory->ops->destroy(factory->data, instance)) {
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        if (ipcp_imap_remove(kipcm->instances, id)) {
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        KIPCM_UNLOCK(kipcm);

        return 0;
}

int kipcm_flow_arrived(struct kipcm *         kipcm,
                       ipc_process_id_t       ipc_id,
                       port_id_t              port_id,
                       struct name *          dif_name,
                       struct name *          source,
                       struct name *          dest,
                       struct flow_spec *     fspec)
{
        uint_t             nl_port_id;
        rnl_sn_t           seq_num;
        struct ipcp_instance * ipc_process;

        IRQ_BARRIER;

        /* FIXME: Use a constant (define) ! */
        nl_port_id = 1;

        ipc_process  = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipc_process) {
                LOG_ERR("IPC process %d not found", ipc_id);
                return -1;
        }

        seq_num = rnl_get_next_seqn(kipcm->rnls);
        if (kipcm_smap_add_ni(kipcm->messages->egress, seq_num, port_id)) {
                LOG_DBG("Could not get next sequence number");
                return -1;
        }

        if (rnl_app_alloc_flow_req_arrived_msg(ipc_id,
                                               dif_name,
                                               source,
                                               dest,
                                               fspec,
                                               seq_num,
                                               nl_port_id,
                                               port_id))
                return -1;

        return 0;
}
EXPORT_SYMBOL(kipcm_flow_arrived);

struct ipcp_instance * kipcm_find_ipcp(struct kipcm *   kipcm,
                                       ipc_process_id_t ipc_id)
{
        struct ipcp_instance * ipcp;

        KIPCM_LOCK(kipcm);

        ipcp = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipcp) {
                LOG_ERR("Couldn't find the ipc process %d", ipc_id);
                KIPCM_UNLOCK(kipcm);
                return NULL;
        }
        KIPCM_UNLOCK(kipcm);

        return ipcp;
}
EXPORT_SYMBOL(kipcm_find_ipcp);

/* ONLY USED BY APPS */
int kipcm_sdu_write(struct kipcm * kipcm,
                    port_id_t      port_id,
                    struct sdu *   sdu,
                    bool blocking)
{
        struct ipcp_instance * kfa_ipcp;

        IRQ_BARRIER;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                sdu_destroy(sdu);
                return -EINVAL;
        }

        if (!is_sdu_ok(sdu)) {
                LOG_ERR("Bogus SDU received, bailing out");
                sdu_destroy(sdu);
                return -EINVAL;
        }

        kfa_ipcp = kfa_ipcp_instance(kipcm->kfa);
        if (!kfa_ipcp) {
                LOG_ERR("Could not resolve KFA IPCP API");
                sdu_destroy(sdu);
                return -1;
        }
        LOG_DBG("Tring to write SDU to port_id %d", port_id);

        /* The SDU is ours */
        return kfa_ipcp->ops->sdu_write(kfa_ipcp->data,
        				port_id,
        				sdu, blocking);
}

int kipcm_sdu_read(struct kipcm * kipcm,
                   port_id_t      port_id,
                   struct sdu **  sdu,
		   size_t         size,
                   bool blocking)
{
        IRQ_BARRIER;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -EINVAL;
        }

        /* The SDU is theirs now */
        return kfa_flow_sdu_read(kipcm->kfa,
        			 port_id,
				 sdu,
				 size,
				 blocking);
}

int kipcm_mgmt_sdu_write(struct kipcm *   kipcm,
                         ipc_process_id_t id,
                         struct sdu_wpi * sdu_wpi)
{
        struct ipcp_instance * ipcp;

        IRQ_BARRIER;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        if (!sdu_wpi_is_ok(sdu_wpi)) {
                LOG_ERR("Bogus SDU with port-id received, bailing out");
                return -1;
        }

        KIPCM_LOCK(kipcm);
        ipcp = ipcp_imap_find(kipcm->instances, id);
        if (!ipcp) {
                LOG_ERR("Could not find IPC Process with id %d", id);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        if (!ipcp->ops) {
                LOG_ERR("Bogus IPCP ops, bailing out");
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        if (!ipcp->ops->mgmt_sdu_write) {
                LOG_ERR("The IPC Process %d doesn't support this operation",
                        id);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }
        KIPCM_UNLOCK(kipcm);

        if (ipcp->ops->mgmt_sdu_write(ipcp->data,
                                      sdu_wpi->dst_addr,
                                      sdu_wpi->port_id,
                                      sdu_wpi->sdu)) {
        	sdu_wpi_detach(sdu_wpi);
                return -1;
        }

        sdu_wpi_detach(sdu_wpi);

        return 0;
}

/* Only called by the allocate_port netlink message used only by the normal IPCP */
port_id_t kipcm_flow_create(struct kipcm     *kipcm,
			    ipc_process_id_t  ipc_id,
			    struct name      *process_name)
{
        struct ipcp_instance *ipc_process;
	struct ipcp_instance *user_ipc_process;
        port_id_t             pid;

        IRQ_BARRIER;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return port_id_bad();
        }

        KIPCM_LOCK(kipcm);

        ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);

        if (!ipc_process) {
                LOG_ERR("Couldn't find the ipc process %d", ipc_id);
                KIPCM_UNLOCK(kipcm);
                return port_id_bad();
        }

        pid = kfa_port_id_reserve(kipcm->kfa, ipc_id);
        if (!is_port_id_ok(pid)) {
                KIPCM_UNLOCK(kipcm);
                return port_id_bad();
        }

        ASSERT(ipc_process->ops);

        user_ipc_process = ipcp_imap_find_by_name(kipcm->instances,
                                                  process_name);

        if (ipc_process->ops->flow_prebind) {
                ipc_process->ops->flow_prebind(ipc_process->data,
					       user_ipc_process,
					       pid);
        }

        if (user_ipc_process) {
                KIPCM_UNLOCK(kipcm);
                LOG_DBG("This flow is for an ipcp");
                return pid;
        }
	/* creates a flow, default flow_opts */
        if (kfa_flow_create(kipcm->kfa, pid, ipc_process, ipc_id,
								process_name)) {
                KIPCM_UNLOCK(kipcm);
                kfa_port_id_release(kipcm->kfa, pid);
                return port_id_bad();
        }
        KIPCM_UNLOCK(kipcm);
        return pid;
}
EXPORT_SYMBOL(kipcm_flow_create);

int kipcm_flow_destroy(struct kipcm *   kipcm,
		       ipc_process_id_t ipc_id,
		       port_id_t        port_id)
{
        struct ipcp_instance * ipc_process;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipc_process) {
                LOG_ERR("IPC process %d not found", ipc_id);
                return -1;
        }

        ASSERT(ipc_process->ops);
        ASSERT(ipc_process->ops->flow_deallocate);

        if (ipc_process->ops->flow_deallocate(ipc_process->data, port_id)) {
                LOG_ERR("Failed deallocate flow request for port id: %d",
			port_id);
                return -1;
        }

        return 0;
}
EXPORT_SYMBOL(kipcm_flow_destroy);

int kipcm_notify_flow_alloc_req_result(struct kipcm    *kipcm,
                                       ipc_process_id_t ipc_id,
                                       port_id_t        pid,
                                       uint_t           res)
{
        rnl_sn_t seq_num;

        IRQ_BARRIER;

        if (!is_port_id_ok(pid)) {
                LOG_ERR("Flow id is not ok");
                return -1;
        }

        seq_num = kipcm_pmap_find(kipcm->messages->ingress, pid);
        if (!is_rnl_seq_num_ok(seq_num)) {
                LOG_ERR("Could not find request message id (seq num)");
                return -1;
        }

        if (kipcm_pmap_remove(kipcm->messages->ingress, pid)) {
                LOG_ERR("Could not destroy ingress messages map entry");
                return -1;
        }

        if (res)
                kfa_port_id_release(kipcm->kfa, pid);

        /* FIXME: The rnl_port_id shouldn't be hardcoded as 1 */
        if (rnl_base_response(ipc_id, res, pid, 0, seq_num,
        		      RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_RESULT, 1))
                return -1;

        return 0;
}
EXPORT_SYMBOL(kipcm_notify_flow_alloc_req_result);

int kipcm_notify_flow_dealloc(ipc_process_id_t ipc_id,
                              uint_t           code,
                              port_id_t        port_id,
                              u32              nl_port_id)
{
        IRQ_BARRIER;

        if (rnl_flow_dealloc_not_msg(ipc_id, code, port_id, nl_port_id)) {
                LOG_ERR("Could not notificate application about "
                        "flow deallocation");
                return -1;
        }

        return 0;
}
EXPORT_SYMBOL(kipcm_notify_flow_dealloc);

struct ipcp_instance * kipcm_find_ipcp_by_name(struct kipcm * kipcm,
                                               struct name * name)
{
        struct ipcp_instance * ipc_process;

        IRQ_BARRIER;

        KIPCM_LOCK(kipcm);

	if (!kipcm || !name_is_ok(name)) {
        	KIPCM_UNLOCK(kipcm);
		LOG_ERR("Wrong KIPCM or IPCP name struct passed...");
		return NULL;
	}

        ipc_process = ipcp_imap_find_by_name(kipcm->instances, name);

        KIPCM_UNLOCK(kipcm);

        return ipc_process;
}
EXPORT_SYMBOL(kipcm_find_ipcp_by_name);

/* FIXME: This "method" is only temporary, do not rely on its presence */
struct kfa * kipcm_kfa(struct kipcm * kipcm)
{
        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return NULL;
        }

        return kipcm->kfa;
}
EXPORT_SYMBOL(kipcm_kfa);

struct rset * kipcm_rset(struct kipcm * kipcm)
{
        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return NULL;
        }

        return kipcm->rset;
}
EXPORT_SYMBOL(kipcm_rset);
