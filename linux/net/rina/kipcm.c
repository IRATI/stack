/*
 * KIPCM (Kernel-IPC Manager)
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
#include <linux/kobject.h>
#include <linux/export.h>
#include <linux/kfifo.h>
#include <linux/mutex.h>

#define RINA_PREFIX "kipcm"

#include "logs.h"
#include "utils.h"
#include "kipcm.h"
#include "debug.h"
#include "ipcp-factories.h"
#include "ipcp-utils.h"
#include "kipcm-utils.h"
#include "common.h"
#include "du.h"
#include "fidm.h"
#include "netlink.h"
#include "netlink-utils.h"

#define DEFAULT_FACTORY "normal-ipc"

struct kipcm {
        struct mutex            lock;
        struct ipcp_factories * factories;
        struct ipcp_imap *      instances;
        struct rnl_set *        set;

        /* Should these flows management moved to a KFM ? */
        struct {
                struct ipcp_fmap * pending;
                struct ipcp_pmap * committed;
        } flows;
};

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

struct ipcp_flow {
        /* The port-id identifying the flow */
        port_id_t              port_id;

        /*
         * The components of the IPC Process that will handle the
         * write calls to this flow
         */
        struct ipcp_instance * ipc_process;

        /*
         * True if this flow is serving a user-space application, false
         * if it is being used by an RMT
         */
        bool_t                 application_owned;

        struct kfifo           sdu_ready;

        wait_queue_head_t      wait_queue;
};

static void
alloc_flow_req_free(struct name *                              source_name,
                    struct name *                              dest_name,
                    struct flow_spec *                         fspec,
                    struct name *                              dif_name,
                    struct rnl_ipcm_alloc_flow_req_msg_attrs * attrs,
                    struct rnl_msg *                           msg)
{
        if (attrs)       rkfree(attrs);
        if (source_name) rkfree(source_name);
        if (dest_name)   rkfree(dest_name);
        if (fspec)       rkfree(fspec);
        if (dif_name)    rkfree(dif_name);
        if (msg)         rkfree(msg);
}

static int
alloc_flow_req_free_and_reply(struct name *      source_name,
                              struct name *      dest_name,
                              struct flow_spec * fspec,
                              struct name *      dif_name,
                              struct rnl_ipcm_alloc_flow_req_msg_attrs * attrs,
                              struct rnl_msg *   msg,
                              ipc_process_id_t   id,
                              uint_t             res,
                              uint_t             seq_num,
                              uint_t             port_id)
{
        alloc_flow_req_free(source_name, dest_name, fspec, dif_name,
                            attrs, msg);
        
        if (rnl_app_alloc_flow_result_msg(id, res, seq_num, port_id))
                return -1;
        
        return 0;
}

/*
 * It is the responsibility of the shims to send the alloc_req_arrived
 * and the alloc_req_result.
 */
static int notify_ipcp_allocate_flow_request(void *             data,
                                             struct sk_buff *   buff,
                                             struct genl_info * info)
{
        struct rnl_ipcm_alloc_flow_req_msg_attrs * attrs;
        struct rnl_msg *                           msg;
        struct ipcp_instance *                     ipc_process;
        ipc_process_id_t                           ipc_id;
        struct rina_msg_hdr *                      hdr;
        struct kipcm *                             kipcm;
        struct name *                              source;
        struct name *                              dest;
        struct name *                              dif_name;
        struct flow_spec *                         fspec;

        source   = NULL;
        dest     = NULL;
        dif_name = NULL;
        fspec    = NULL;
        attrs    = NULL;
        msg      = NULL;

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }

        if (!info) {
                LOG_ERR("Bogus struct genl_info passed, cannot parse NL msg");
                return -1;
        }

        kipcm = (struct kipcm *) data;
        attrs = rkzalloc(sizeof(*attrs), GFP_KERNEL);
        if (!attrs) {
                return alloc_flow_req_free_and_reply(source,
                                                     dest,
                                                     fspec,
                                                     dif_name,
                                                     attrs,
                                                     msg,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid);
        }

        source = name_create();
        if (!source) {
                return alloc_flow_req_free_and_reply(source,
                                                     dest,
                                                     fspec,
                                                     dif_name,
                                                     attrs,
                                                     msg,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid);
        }
        attrs->source = source;

        dest = name_create();
        if (!dest) {
                return alloc_flow_req_free_and_reply(source,
                                                     dest,
                                                     fspec,
                                                     dif_name,
                                                     attrs,
                                                     msg,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid);
        }
        attrs->dest = dest;

        dif_name = name_create();
        if (!dif_name) {
                return alloc_flow_req_free_and_reply(source,
                                                     dest,
                                                     fspec,
                                                     dif_name,
                                                     attrs,
                                                     msg,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid);
        }
        attrs->dif_name = dif_name;

        fspec = rkzalloc(sizeof(struct flow_spec), GFP_KERNEL);
        if (!fspec) {
                return alloc_flow_req_free_and_reply(source,
                                                     dest,
                                                     fspec,
                                                     dif_name,
                                                     attrs,
                                                     msg,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid);
        }
        attrs->fspec = fspec;

        msg = rkzalloc(sizeof(*msg), GFP_KERNEL);
        if (!msg) {
                return alloc_flow_req_free_and_reply(source,
                                                     dest,
                                                     fspec,
                                                     dif_name,
                                                     attrs,
                                                     msg,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid);
        }
        msg->attrs = attrs;

        hdr = rkzalloc(sizeof(*hdr), GFP_KERNEL);
        if (!hdr) {
                return alloc_flow_req_free_and_reply(source,
                                                     dest,
                                                     fspec,
                                                     dif_name,
                                                     attrs,
                                                     msg,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid);
        }
        msg->rina_hdr = hdr;

        if (rnl_parse_msg(info, msg)) {
                return alloc_flow_req_free_and_reply(source,
                                                     dest,
                                                     fspec,
                                                     dif_name,
                                                     attrs,
                                                     msg,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid);
        }

        ipc_id      = msg->rina_hdr->dst_ipc_id;
        ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipc_process) {
                LOG_ERR("IPC process %d not found", ipc_id);
                return alloc_flow_req_free_and_reply(source,
                                                     dest,
                                                     fspec,
                                                     dif_name,
                                                     attrs,
                                                     msg,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid);
        }

#if 0 /* FIXME: Please re-enable */
        /* The flow id MUST be ok upon calling the IPC Process ... */
        ASSERT(is_flow_id_ok(fid));

        if (ipc_process->ops->flow_allocate_request(ipc_process->data,
                                                    attrs->source,
                                                    attrs->dest,
                                                    attrs->fspec,
                                                    attrs->id,
                                                    msg->seq_num)) {
                LOG_ERR("Failed allocating flow request "
                        "for port id: %d", attrs->id);
                return alloc_flow_req_free_and_reply(source,
                                                     dest,
                                                     fspec,
                                                     dif_name,
                                                     attrs,
                                                     msg,
                                                     ipc_id,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid);
        }
#endif

        alloc_flow_req_free(source, dest, fspec, dif_name, attrs, msg);

        return 0;
}

static int notify_ipcp_allocate_flow_response(void *             data,
                                              struct sk_buff *   buff,
                                              struct genl_info * info)
{
        struct kipcm *                         kipcm;
        struct rnl_alloc_flow_resp_msg_attrs * attrs;
        struct rnl_msg *                       msg;
        struct rina_msg_hdr *                  hdr;
        struct ipcp_instance *                 ipc_process;
        ipc_process_id_t                       ipc_id;
        int                                    retval = 0;
        response_reason_t                      reason;

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }

        kipcm = (struct kipcm *) data;

        if (!info) {
                LOG_ERR("Bogus struct genl_info passed, cannot parse NL msg");
                return -1;
        }

        attrs = rkzalloc(sizeof(*attrs), GFP_KERNEL);
        if (!attrs)
                return -1;

        msg = rkzalloc(sizeof(*msg), GFP_KERNEL);
        if (!msg) {
                rkfree(attrs);
                return -1;
        }

        hdr = rkzalloc(sizeof(*hdr), GFP_KERNEL);
        if (!hdr) {
                rkfree(attrs);
                rkfree(msg);
                return -1;
        }
        msg->attrs    = attrs;
        msg->rina_hdr = hdr;

        if (rnl_parse_msg(info, msg)) {
                rkfree(hdr);
                rkfree(attrs);
                rkfree(msg);
                return -1;
        }
        ipc_id      = msg->rina_hdr->dst_ipc_id;
        ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipc_process) {
                LOG_ERR("IPC process %d not found", ipc_id);
                rkfree(hdr);
                rkfree(attrs);
                rkfree(msg);
                return -1;
        }

        reason = (response_reason_t) attrs->result;

#if 0 /* FIXME: Please re-enable */
        if (ipc_process->ops->flow_allocate_response(ipc_process->data,
                                                     attrs->id,
                                                     info->snd_seq,
                                                     &reason)) {
                LOG_ERR("Failed allocate flow response for port id: %d",
                        attrs->id);
                retval = -1;
        }
#endif
        rkfree(hdr);
        rkfree(attrs);
        rkfree(msg);

        return retval;
}

static int
dealloc_flow_req_free_and_reply(struct rnl_ipcm_dealloc_flow_req_msg_attrs * attrs,
                                struct rnl_msg * msg,
                                ipc_process_id_t id,
                                uint_t           res,
                                uint_t           seq_num,
                                uint_t           port_id)
{
        if (attrs) rkfree(attrs);
        if (msg)   rkfree(msg);

        if (rnl_app_dealloc_flow_resp_msg(id, res, seq_num, port_id))
                return -1;

        return 0;
}

/*
 * It is the responsibility of the shims to send the alloc_req_arrived
 * and the alloc_req_result.
 */
static int notify_ipcp_deallocate_flow_request(void *             data,
                                               struct sk_buff *   buff,
                                               struct genl_info * info)
{
        struct rnl_ipcm_dealloc_flow_req_msg_attrs * attrs;
        struct rnl_msg *                             msg;
        struct ipcp_instance *                       ipc_process;
        ipc_process_id_t                             ipc_id;
        struct rina_msg_hdr *                        hdr;
        struct kipcm * kipcm;

        attrs = NULL;
        msg   = NULL;

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }

        if (!info) {
                LOG_ERR("Bogus struct genl_info passed, cannot parse NL msg");
                return -1;
        }

        kipcm = (struct kipcm *) data;
        attrs = rkzalloc(sizeof(*attrs), GFP_KERNEL);
        if (!attrs)
                return dealloc_flow_req_free_and_reply(attrs,
                                                       msg,
                                                       0,
                                                       -1,
                                                       info->snd_seq,
                                                       info->snd_portid);

        msg = rkzalloc(sizeof(*msg), GFP_KERNEL);
        if (!msg)
                return dealloc_flow_req_free_and_reply(attrs,
                                                       msg,
                                                       0,
                                                       -1,
                                                       info->snd_seq,
                                                       info->snd_portid);
        msg->attrs = attrs;

        hdr = rkzalloc(sizeof(*hdr), GFP_KERNEL);
        if (!hdr)
                return dealloc_flow_req_free_and_reply(attrs,
                                                       msg,
                                                       0,
                                                       -1,
                                                       info->snd_seq,
                                                       info->snd_portid);

        msg->rina_hdr = hdr;

        if (rnl_parse_msg(info, msg))
                return dealloc_flow_req_free_and_reply(attrs,
                                                       msg,
                                                       0,
                                                       -1,
                                                       info->snd_seq,
                                                       info->snd_portid);

        ipc_id      = msg->rina_hdr->dst_ipc_id;
        ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipc_process) {
                LOG_ERR("IPC process %d not found", ipc_id);
                return dealloc_flow_req_free_and_reply(attrs,
                                                       msg,
                                                       0,
                                                       -1,
                                                       info->snd_seq,
                                                       info->snd_portid);
        }

        if (ipc_process->ops->flow_deallocate(ipc_process->data, attrs->id)) {
                LOG_ERR("Failed deallocate flow request "
                        "for port id: %d", attrs->id);
                return dealloc_flow_req_free_and_reply(attrs,
                                                       msg,
                                                       ipc_id,
                                                       -1,
                                                       info->snd_seq,
                                                       info->snd_portid);
        }

        return dealloc_flow_req_free_and_reply(attrs,
                                               msg,
                                               ipc_id,
                                               0,
                                               info->snd_seq,
                                               info->snd_portid);
}

static int
assign_to_dif_free_and_reply(struct name *       dif_name,
                             struct dif_config * dif_config,
                             struct rnl_ipcm_assign_to_dif_req_msg_attrs * attrs,
                             struct rnl_msg *    msg,
                             ipc_process_id_t    id,
                             uint_t              res,
                             uint_t              seq_num,
                             uint_t              port_id)
{
        if (attrs)      rkfree(attrs);
        if (dif_name)   rkfree(dif_name);
        if (dif_config) rkfree(dif_config);
        if (msg)        rkfree(msg);

        if (rnl_assign_dif_response(id, res, seq_num, port_id))
                return -1;

        return 0;
}

static int notify_ipcp_assign_dif_request(void *             data,
                                          struct sk_buff *   buff,
                                          struct genl_info * info)
{
        struct kipcm *                                kipcm;
        struct rnl_ipcm_assign_to_dif_req_msg_attrs * attrs;
        struct rnl_msg *                              msg;
        struct dif_config *                           dif_config;
        struct name *                                 dif_name;
        struct ipcp_instance *                        ipc_process;
        ipc_process_id_t                              ipc_id;

        attrs      = NULL;
        msg        = NULL;
        dif_name   = NULL;
        dif_config = NULL;

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }

        kipcm = (struct kipcm *) data;

        if (!info) {
                LOG_ERR("Bogus struct genl_info passed, cannot parse NL msg");
                return -1;
        }

        attrs = rkzalloc(sizeof(*attrs), GFP_KERNEL);
        if (!attrs)
                return assign_to_dif_free_and_reply(dif_name,
                                                    dif_config,
                                                    attrs,
                                                    msg,
                                                    0,
                                                    -1,
                                                    info->snd_seq,
                                                    info->snd_portid);

        dif_config = rkzalloc(sizeof(struct dif_config), GFP_KERNEL);
        if (!dif_config)
                return assign_to_dif_free_and_reply(dif_name,
                                                    dif_config,
                                                    attrs,
                                                    msg,
                                                    0,
                                                    -1,
                                                    info->snd_seq,
                                                    info->snd_portid);

        attrs->dif_config = dif_config;

        dif_name = name_create();
        if (!dif_name)
                return assign_to_dif_free_and_reply(dif_name,
                                                    dif_config,
                                                    attrs,
                                                    msg,
                                                    0,
                                                    -1,
                                                    info->snd_seq,
                                                    info->snd_portid);
        dif_config->dif_name = dif_name;

        msg = rkzalloc(sizeof(*msg), GFP_KERNEL);
        if (!msg)
                return assign_to_dif_free_and_reply(dif_name,
                                                    dif_config,
                                                    attrs,
                                                    msg,
                                                    0,
                                                    -1,
                                                    info->snd_seq,
                                                    info->snd_portid);

        msg->attrs = attrs;

        if (rnl_parse_msg(info, msg))
                return assign_to_dif_free_and_reply(dif_name,
                                                    dif_config,
                                                    attrs,
                                                    msg,
                                                    0,
                                                    -1,
                                                    info->snd_seq,
                                                    info->snd_portid);
        ipc_id = msg->rina_hdr->dst_ipc_id;

        ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipc_process) {
                LOG_ERR("IPC process %d not found", ipc_id);
                return assign_to_dif_free_and_reply(dif_name,
                                                    dif_config,
                                                    attrs,
                                                    msg,
                                                    0,
                                                    -1,
                                                    info->snd_seq,
                                                    info->snd_portid);
        }
        LOG_DBG("Found IPC Process with id %d", ipc_id);

        /* FIXME: The configuration has to be fetched from the NL message */
        LOG_MISSING;

        if (ipc_process->ops->assign_to_dif(ipc_process->data,
                                            attrs->dif_config->dif_name,
                                            NULL)) {
                char * tmp = name_tostring(attrs->dif_config->dif_name);
                LOG_ERR("Assign to dif %s operation failed for IPC process %d",
                        tmp, ipc_id);
                rkfree(tmp);

                return assign_to_dif_free_and_reply(dif_name,
                                                    dif_config,
                                                    attrs,
                                                    msg,
                                                    ipc_id,
                                                    -1,
                                                    info->snd_seq,
                                                    info->snd_portid);
        }

        return assign_to_dif_free_and_reply(dif_name,
                                            dif_config,
                                            attrs,
                                            msg,
                                            ipc_id,
                                            0,
                                            info->snd_seq,
                                            info->snd_portid);
}

static int
reg_unreg_resp_free_and_reply(struct name *     app_name,
                              struct name *     dif_name,
                              struct rnl_ipcm_reg_app_req_msg_attrs * attrs,
                              struct rnl_msg *  msg,
                              ipc_process_id_t  id,
                              uint_t            res,
                              uint_t            seq_num,
                              uint_t            port_id,
                              bool              is_register)
{
        if (app_name) rkfree(app_name);
        if (dif_name) rkfree(dif_name);
        if (attrs)    rkfree(attrs);
        if (msg)      rkfree(msg);

        if (rnl_app_register_unregister_response_msg(id,
                                                     res,
                                                     seq_num,
                                                     port_id,
                                                     is_register))
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
        struct name *                           app_name;
        struct name *                           dif_name;
        struct ipcp_instance *                  ipc_process;
        ipc_process_id_t                        ipc_id;

        attrs    = NULL;
        msg      = NULL;
        app_name = NULL;
        dif_name = NULL;

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }

        kipcm = (struct kipcm *) data;

        if (!info) {
                LOG_ERR("Bogus struct genl_info passed, cannot parse NL msg");
                return -1;
        }

        attrs = rkzalloc(sizeof(*attrs), GFP_KERNEL);
        if (!attrs) {
                return reg_unreg_resp_free_and_reply(app_name,
                                                     dif_name,
                                                     attrs,
                                                     msg,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid,
                                                     true);
        }

        app_name = name_create();
        if (!app_name) {
                return reg_unreg_resp_free_and_reply(app_name,
                                                     dif_name,
                                                     attrs,
                                                     msg,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid,
                                                     true);
        }
        attrs->app_name= app_name;

        dif_name = name_create();
        if (!dif_name) {
                return reg_unreg_resp_free_and_reply(app_name,
                                                     dif_name,
                                                     attrs,
                                                     msg,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid,
                                                     true);
        }
        attrs->dif_name= dif_name;

        msg = rkzalloc(sizeof(*msg), GFP_KERNEL);
        if (!msg) {
                LOG_ERR("Could not allocate space for my_msg struct");
                return reg_unreg_resp_free_and_reply(app_name,
                                                     dif_name,
                                                     attrs,
                                                     msg,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid,
                                                     true);
        }
        msg->attrs = attrs;

        if (rnl_parse_msg(info, msg)) {
                LOG_ERR("Could not parse message");
                return reg_unreg_resp_free_and_reply(app_name,
                                                     dif_name,
                                                     attrs,
                                                     msg,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid,
                                                     true);
        }

        ipc_id = msg->rina_hdr->dst_ipc_id;
        ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipc_process) {
                LOG_ERR("IPC process %d not found", ipc_id);
                return reg_unreg_resp_free_and_reply(app_name,
                                                     dif_name,
                                                     attrs,
                                                     msg,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid,
                                                     true);
        }

        if (ipc_process->ops->application_register(ipc_process->data,
                                                   attrs->app_name)) {
                return reg_unreg_resp_free_and_reply(app_name,
                                                     dif_name,
                                                     attrs,
                                                     msg,
                                                     ipc_id,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid,
                                                     true);
        }

        return reg_unreg_resp_free_and_reply(app_name,
                                             dif_name,
                                             attrs,
                                             msg,
                                             ipc_id,
                                             0,
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
        struct name *                           app_name;
        struct name *                           dif_name;
        struct ipcp_instance *                  ipc_process;
        ipc_process_id_t                        ipc_id;

        attrs    = NULL;
        msg      = NULL;
        app_name = NULL;
        dif_name = NULL;

        if (!data) {
                LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
                return -1;
        }

        kipcm = (struct kipcm *) data;

        if (!info) {
                LOG_ERR("Bogus struct genl_info passed, cannot parse NL msg");
                return -1;
        }

        attrs = rkzalloc(sizeof(*attrs), GFP_KERNEL);
        if (!attrs)
                return reg_unreg_resp_free_and_reply(app_name,
                                                     dif_name,
                                                     attrs,
                                                     msg,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid,
                                                     false);

        app_name = name_create();
        if (!app_name)
                return reg_unreg_resp_free_and_reply(app_name,
                                                     dif_name,
                                                     attrs,
                                                     msg,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid,
                                                     false);
        attrs->app_name= app_name;

        dif_name = name_create();
        if (!dif_name)
                return reg_unreg_resp_free_and_reply(app_name,
                                                     dif_name,
                                                     attrs,
                                                     msg,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid,
                                                     false);
        attrs->dif_name= dif_name;

        msg = rkzalloc(sizeof(*msg), GFP_KERNEL);
        if (!msg) {
                LOG_ERR("Could not allocate space for my_msg struct");
                return reg_unreg_resp_free_and_reply(app_name,
                                                     dif_name,
                                                     attrs,
                                                     msg,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid,
                                                     false);
        }
        msg->attrs = attrs;

        if (rnl_parse_msg(info, msg)) {
                LOG_ERR("Could not parse message");
                return reg_unreg_resp_free_and_reply(app_name,
                                                     dif_name,
                                                     attrs,
                                                     msg,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid,
                                                     false);
        }

        ipc_id      = msg->rina_hdr->dst_ipc_id;
        ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipc_process) {
                LOG_ERR("IPC process %d not found", ipc_id);
                return reg_unreg_resp_free_and_reply(app_name,
                                                     dif_name,
                                                     attrs,
                                                     msg,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid,
                                                     false);
        }

        if (ipc_process->ops->application_unregister(ipc_process->data,
                                                     attrs->app_name)) {
                return reg_unreg_resp_free_and_reply(app_name,
                                                     dif_name,
                                                     attrs,
                                                     msg,
                                                     ipc_id,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid,
                                                     false);
        }

        return reg_unreg_resp_free_and_reply(app_name,
                                             dif_name,
                                             attrs,
                                             msg,
                                             ipc_id,
                                             0,
                                             info->snd_seq,
                                             info->snd_portid,
                                             false);
}

static int netlink_handlers_unregister(struct rnl_set * set)
{
        int retval = 0;

        if (rnl_handler_unregister(set,
                                   RINA_C_IPCM_ALLOCATE_FLOW_REQUEST))
                retval = -1;

        if (rnl_handler_unregister(set,
                                   RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE))
                retval = -1;

        if (rnl_handler_unregister(set,
                                   RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST))
                retval = -1;

        if (rnl_handler_unregister(set,
                                   RINA_C_IPCM_REGISTER_APPLICATION_REQUEST))
                retval = -1;

        if (rnl_handler_unregister(set,
                                   RINA_C_IPCM_UNREGISTER_APPLICATION_REQUEST))
                retval = -1;

        if (rnl_handler_unregister(set,
                                   RINA_C_IPCM_DEALLOCATE_FLOW_REQUEST))
                retval = -1;

        LOG_DBG("NL handlers unregistered %s",
                (retval == 0) ? "successfully" : "unsuccessfully");

        return retval;
}

static int netlink_handlers_register(struct kipcm * kipcm)
{
        message_handler_cb handler;

        handler = notify_ipcp_assign_dif_request;
        if (rnl_handler_register(kipcm->set,
                                 RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST,
                                 kipcm,
                                 handler))
                return -1;

        handler = notify_ipcp_allocate_flow_request;
        if (rnl_handler_register(kipcm->set,
                                 RINA_C_IPCM_ALLOCATE_FLOW_REQUEST,
                                 kipcm,
                                 handler)) {
                if (rnl_handler_unregister(kipcm->set,
                                           RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST)) {
                        LOG_ERR("Failed handler unregister while bailing out");
                        /* FIXME: What else could be done here?" */
                }
                return -1;
        }

        handler = notify_ipcp_allocate_flow_response;
        if (rnl_handler_register(kipcm->set,
                                 RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE,
                                 kipcm,
                                 handler)) {
                if (rnl_handler_unregister(kipcm->set,
                                           RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST)) {
                        LOG_ERR("Failed handler unregister while bailing out");
                        /* FIXME: What else could be done here?" */
                }
                if (rnl_handler_unregister(kipcm->set,
                                           RINA_C_IPCM_ALLOCATE_FLOW_REQUEST)) {
                        LOG_ERR("Failed handler unregister while bailing out");
                        /* FIXME: What else could be done here?" */
                }
                return -1;
        }

        handler = notify_ipcp_register_app_request;
        if (rnl_handler_register(kipcm->set,
                                 RINA_C_IPCM_REGISTER_APPLICATION_REQUEST,
                                 kipcm,
                                 handler)) {
                if (rnl_handler_unregister(kipcm->set,
                                           RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST)) {
                        LOG_ERR("Failed handler unregister while bailing out");
                        /* FIXME: What else could be done here?" */
                }
                if (rnl_handler_unregister(kipcm->set,
                                           RINA_C_IPCM_ALLOCATE_FLOW_REQUEST)) {
                        LOG_ERR("Failed handler unregister while bailing out");
                        /* FIXME: What else could be done here?" */
                }
                if (rnl_handler_unregister(kipcm->set,
                                           RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE)) {
                        LOG_ERR("Failed handler unregister while bailing out");
                        /* FIXME: What else could be done here?" */
                }
                return -1;
        }

        handler = notify_ipcp_unregister_app_request;
        if (rnl_handler_register(kipcm->set,
                                 RINA_C_IPCM_UNREGISTER_APPLICATION_REQUEST,
                                 kipcm,
                                 handler)) {
                if (rnl_handler_unregister(kipcm->set,
                                           RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST)) {
                        LOG_ERR("Failed handler unregister while bailing out");
                        /* FIXME: What else could be done here?" */
                }
                if (rnl_handler_unregister(kipcm->set,
                                           RINA_C_IPCM_ALLOCATE_FLOW_REQUEST)) {
                        LOG_ERR("Failed handler unregister while bailing out");
                        /* FIXME: What else could be done here?" */
                }
                if (rnl_handler_unregister(kipcm->set,
                                           RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE)) {
                        LOG_ERR("Failed handler unregister while bailing out");
                        /* FIXME: What else could be done here?" */
                }
                return -1;
                if (rnl_handler_unregister(kipcm->set,
                                           RINA_C_IPCM_REGISTER_APPLICATION_REQUEST)) {
                        LOG_ERR("Failed handler unregister while bailing out");
                        /* FIXME: What else could be done here?" */
                }
                return -1;
        }

        handler = notify_ipcp_deallocate_flow_request;
        if (rnl_handler_register(kipcm->set,
                                 RINA_C_IPCM_DEALLOCATE_FLOW_REQUEST,
                                 kipcm,
                                 handler)) {
                if (rnl_handler_unregister(kipcm->set,
                                           RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST)) {
                        LOG_ERR("Failed handler unregister while bailing out");
                        /* FIXME: What else could be done here?" */
                }
                if (rnl_handler_unregister(kipcm->set,
                                           RINA_C_IPCM_ALLOCATE_FLOW_REQUEST)) {
                        LOG_ERR("Failed handler unregister while bailing out");
                        /* FIXME: What else could be done here?" */
                }
                if (rnl_handler_unregister(kipcm->set,
                                           RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE)) {
                        LOG_ERR("Failed handler unregister while bailing out");
                        /* FIXME: What else could be done here?" */
                }
                return -1;
                if (rnl_handler_unregister(kipcm->set,
                                           RINA_C_IPCM_REGISTER_APPLICATION_REQUEST)) {
                        LOG_ERR("Failed handler unregister while bailing out");
                        /* FIXME: What else could be done here?" */
                }
                return -1;
                if (rnl_handler_unregister(kipcm->set,
                                           RINA_C_IPCM_UNREGISTER_APPLICATION_REQUEST)) {
                        LOG_ERR("Failed handler unregister while bailing out");
                        /* FIXME: What else could be done here?" */
                }
                return -1;
        }

        LOG_DBG("NL handlers registered successfully");

        return 0;
}

struct kipcm * kipcm_create(struct kobject * parent,
                            struct rnl_set * set)
{
        struct kipcm * tmp;

        LOG_DBG("Initializing");

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->factories = ipcpf_init(parent);
        if (!tmp->factories) {
                rkfree(tmp);
                return NULL;
        }

        tmp->instances = ipcp_imap_create();
        if (!tmp->instances) {
                if (ipcpf_fini(tmp->factories)) {
                        /* FIXME: What could we do here ? */
                }
                rkfree(tmp);
                return NULL;
        }

        tmp->flows.committed = ipcp_pmap_create();
        if (!tmp->flows.committed) {
                if (ipcp_imap_destroy(tmp->instances)) {
                        /* FIXME: What could we do here ? */
                }
                if (ipcpf_fini(tmp->factories)) {
                        /* FIXME: What could we do here ? */
                }
                rkfree(tmp);
                return NULL;
        }

        if (rnl_set_register(set)) {
                if (ipcp_imap_destroy(tmp->instances)) {
                        /* FIXME: What could we do here ? */
                }
                if (ipcp_pmap_destroy(tmp->flows.committed)) {
                        /* FIXME: What could we do here ? */
                }
                if (ipcpf_fini(tmp->factories)) {
                        /* FIXME: What could we do here ? */
                }
                rkfree(tmp);
                return NULL;
        }
        tmp->set = set;

        if (netlink_handlers_register(tmp)) {
                if (ipcp_imap_destroy(tmp->instances)) {
                        /* FIXME: What could we do here ? */
                }
                if (ipcp_pmap_destroy(tmp->flows.committed)) {
                        /* FIXME: What could we do here ? */
                }
                if (ipcpf_fini(tmp->factories)) {
                        /* FIXME: What could we do here ? */
                }
                rkfree(tmp);
                return NULL;
        }

        KIPCM_LOCK_INIT(tmp);

        LOG_DBG("Initialized successfully");

        return tmp;
}

int kipcm_destroy(struct kipcm * kipcm)
{
        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        LOG_DBG("Finalizing");

        KIPCM_LOCK(kipcm);

        /* FIXME: Destroy all the committed flows */
        ASSERT(ipcp_pmap_empty(kipcm->flows.committed));
        if (ipcp_pmap_destroy(kipcm->flows.committed)) {
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        /* FIXME: Destroy all the instances */
        ASSERT(ipcp_imap_empty(kipcm->instances));
        if (ipcp_imap_destroy(kipcm->instances)) {
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        if (ipcpf_fini(kipcm->factories)) {
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        if (netlink_handlers_unregister(kipcm->set)) {
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        KIPCM_UNLOCK(kipcm);

        KIPCM_LOCK_FINI(kipcm);

        rkfree(kipcm);

        LOG_DBG("Finalized successfully");

        return 0;
}

struct ipcp_factory *
kipcm_ipcp_factory_register(struct kipcm *             kipcm,
                            const  char *              name,
                            struct ipcp_factory_data * data,
                            struct ipcp_factory_ops *  ops)
{
        struct ipcp_factory * retval;

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
        int retval;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        /* FIXME:
         *
         *   We have to do the body of kipcm_ipcp_destroy() on all the
         *   instances remaining (and not explicitly destroyed), previously
         *   created with the factory being unregisterd ...
         *
         *     Francesco
         */
        KIPCM_LOCK(kipcm);
        retval = ipcpf_unregister(kipcm->factories, factory);
        KIPCM_UNLOCK(kipcm);

        return retval;
}
EXPORT_SYMBOL(kipcm_ipcp_factory_unregister);

int kipcm_ipcp_create(struct kipcm *      kipcm,
                      const struct name * ipcp_name,
                      ipc_process_id_t    id,
                      const char *        factory_name)
{
        char *                 name;
        struct ipcp_factory *  factory;
        struct ipcp_instance * instance;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        if (!factory_name)
                factory_name = DEFAULT_FACTORY;

        name = name_tostring(ipcp_name);
        if (!name)
                return -1;

        ASSERT(ipcp_name);
        ASSERT(factory_name);

        LOG_DBG("Creating IPC process:");
        LOG_DBG("  name:      %s", name);
        LOG_DBG("  id:        %d", id);
        LOG_DBG("  factory:   %s", factory_name);
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

        instance = factory->ops->create(factory->data, ipcp_name, id);
        if (!instance) {
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        /* FIXME: Ugly as hell */
        instance->factory = factory;

        if (ipcp_imap_add(kipcm->instances, id, instance)) {
                factory->ops->destroy(factory->data, instance);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        KIPCM_UNLOCK(kipcm);

        return 0;
}

int kipcm_ipcp_destroy(struct kipcm *   kipcm,
                       ipc_process_id_t id)
{
        struct ipcp_instance * instance;
        struct ipcp_factory *  factory;

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

        if (ipcp_pmap_remove_all_for_id(kipcm->flows.committed, id)) {
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

        KIPCM_UNLOCK(kipcm);

        return 0;
}

int kipcm_flow_arrived(struct kipcm *   kipcm,
                       ipc_process_id_t ipc_id,
                       flow_id_t        flow_id)
{
        LOG_MISSING;

        return -1;
}
EXPORT_SYMBOL(kipcm_flow_arrived);

int kipcm_flow_add(struct kipcm *   kipcm,
                   ipc_process_id_t ipc_id,
                   port_id_t        port_id)
{
        struct ipcp_flow * flow;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        KIPCM_LOCK(kipcm);

        if (ipcp_pmap_find(kipcm->flows.committed, port_id)) {
                LOG_ERR("Flow on port-id %d already exists", port_id);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        flow = rkzalloc(sizeof(*flow), GFP_KERNEL);
        if (!flow) {
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        init_waitqueue_head(&flow->wait_queue);

        flow->port_id     = port_id;
        flow->ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!flow->ipc_process) {
                LOG_ERR("Couldn't find the ipc process %d", ipc_id);
                rkfree(flow);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        /*
         * FIXME: We are allowing applications, this must be changed once
         *        the RMT is implemented.
         */
        flow->application_owned = 1;
        if (kfifo_alloc(&flow->sdu_ready, PAGE_SIZE, GFP_KERNEL)) {
                LOG_ERR("Couldn't create the sdu-ready queue for "
                        "flow on port-id %d", port_id);
                rkfree(flow);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        if (ipcp_pmap_add(kipcm->flows.committed, port_id, flow, ipc_id)) {
                kfifo_free(&flow->sdu_ready);
                rkfree(flow);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        KIPCM_UNLOCK(kipcm);

        return 0;
}
EXPORT_SYMBOL(kipcm_flow_add);

int kipcm_flow_remove(struct kipcm * kipcm,
                      port_id_t      port_id)
{
        struct ipcp_flow * flow;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        KIPCM_LOCK(kipcm);

        flow = ipcp_pmap_find(kipcm->flows.committed, port_id);
        if (!flow) {
                LOG_ERR("Couldn't retrieve the flow for port-id %d", port_id);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        if (ipcp_pmap_remove(kipcm->flows.committed, port_id)) {
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        KIPCM_UNLOCK(kipcm);

        LOG_DBG("Awake flow");
        wake_up_interruptible(&flow->wait_queue);

        kfifo_free(&flow->sdu_ready);
        rkfree(flow);

        return 0;
}
EXPORT_SYMBOL(kipcm_flow_remove);

int kipcm_sdu_write(struct kipcm * kipcm,
                    port_id_t      port_id,
                    struct sdu *   sdu)
{
        struct ipcp_flow *     flow;
        struct ipcp_instance * instance;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        if (!sdu || !sdu_is_ok(sdu)) {
                LOG_ERR("Bogus SDU received, bailing out");
                return -1;
        }

        LOG_DBG("Tring to write SDU of size %zd to port_id %d",
                sdu->buffer->size, port_id);

        KIPCM_LOCK(kipcm);

        flow = ipcp_pmap_find(kipcm->flows.committed, port_id);
        if (!flow) {
                LOG_ERR("There is no flow bound to port-id %d", port_id);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        instance = flow->ipc_process;
        ASSERT(instance);
        if (instance->ops->sdu_write(instance->data, port_id, sdu)) {
                LOG_ERR("Couldn't write SDU on port-id %d", port_id);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        KIPCM_UNLOCK(kipcm);

        /* The SDU is ours */
        return 0;
}

int kipcm_sdu_read(struct kipcm * kipcm,
                   port_id_t      port_id,
                   struct sdu **  sdu)
{
        struct ipcp_flow * flow;
        size_t             size;
        char *             data;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }
        if (!sdu) {
                LOG_ERR("Bogus parameters passed, bailing out");
                return -1;
        }

        LOG_DBG("Trying to read SDU from port-id %d", port_id);

        KIPCM_LOCK(kipcm);

        flow = ipcp_pmap_find(kipcm->flows.committed, port_id);
        if (!flow) {
                LOG_ERR("There is no flow bound to port-id %d", port_id);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        while (kfifo_is_empty(&flow->sdu_ready)) {
                LOG_DBG("Going to sleep");
                KIPCM_UNLOCK(kipcm);

                interruptible_sleep_on(&flow->wait_queue);

                KIPCM_LOCK(kipcm);
                LOG_DBG("Woken up");
                flow = ipcp_pmap_find(kipcm->flows.committed, port_id);
                if (!flow) {
                        LOG_ERR("There is no flow bound to port-id %d",
                                port_id);
                        KIPCM_UNLOCK(kipcm);
                        return -1;
                }
        }

        if (kfifo_out(&flow->sdu_ready, &size, sizeof(size_t)) <
            sizeof(size_t)) {
                LOG_ERR("There is not enough data in port-id %d fifo",
                        port_id);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        /* FIXME: Is it possible to have 0 bytes sdus ??? */
        if (size == 0) {
                LOG_ERR("Zero-size SDU detected");
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        data = rkzalloc(size, GFP_KERNEL);
        if (!data) {
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        if (kfifo_out(&flow->sdu_ready, data, size) != size) {
                LOG_ERR("Could not get %zd bytes from fifo", size);
                rkfree(data);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        *sdu = sdu_create_from(data, size);
        if (!*sdu) {
                rkfree(data);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        KIPCM_UNLOCK(kipcm);

        /* The SDU is theirs now */
        return 0;
}

int kipcm_sdu_post(struct kipcm * kipcm,
                   port_id_t      port_id,
                   struct sdu *   sdu)
{
        struct ipcp_flow * flow;
        unsigned int       avail;

        LOG_DBG("Posting SDU of size %zd to port-id %d ",
                sdu->buffer->size, port_id);

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, cannot post SDU");
                return -1;
        }
        if (!sdu || !sdu_is_ok(sdu)) {
                LOG_ERR("Bogus parameters passed, bailing out");
                return -1;
        }
        /*
         * FIXME: This lock has been removed only for the shim-dummy demo. Please
         * reinstate it as soon as possible.
         *
         *      KIPCM_LOCK(kipcm);
         */
        flow = ipcp_pmap_find(kipcm->flows.committed, port_id);
        if (!flow) {
                LOG_ERR("There is no flow bound to port-id %d", port_id);
                /*
                 * FIXME: This lock has been removed only for the shim-dummy demo. Please
                 * reinstate it as soon as possible.
                 *
                 KIPCM_UNLOCK(kipcm);
                */
                return -1;
        }

        avail = kfifo_avail(&flow->sdu_ready);
        if (avail < (sdu->buffer->size + sizeof(size_t))) {
                LOG_ERR("There is no space in the port-id %d fifo", port_id);
                /*
                 * FIXME: This lock has been removed only for the shim-dummy demo. Please
                 * reinstate it as soon as possible.
                 *
                 KIPCM_UNLOCK(kipcm);
                */
                return -1;
        }

        if (kfifo_in(&flow->sdu_ready,
                     &sdu->buffer->size,
                     sizeof(size_t)) != sizeof(size_t)) {
                LOG_ERR("Could not write %zd bytes from port-id %d fifo",
                        sizeof(size_t), port_id);
                /*
                 * FIXME: This lock has been removed only for the shim-dummy demo. Please
                 * reinstate it as soon as possible.
                 *
                 KIPCM_UNLOCK(kipcm);
                */
                return -1;
        }
        if (kfifo_in(&flow->sdu_ready,
                     sdu->buffer->data,
                     sdu->buffer->size) != sdu->buffer->size) {
                LOG_ERR("Could not write %zd bytes from port-id %d fifo",
                        sdu->buffer->size, port_id);
                /*
                 * FIXME: This lock has been removed only for the shim-dummy demo. Please
                 * reinstate it as soon as possible.
                 *
                 KIPCM_UNLOCK(kipcm);
                */
                return -1;
        }

        LOG_DBG("SDU posted");

        wake_up_interruptible(&flow->wait_queue);

        LOG_DBG("Sleeping read syscall should be working now");

        /*
         * FIXME: This lock has been removed only for the shim-dummy demo.
         *        Please reinstate it as soon as possible.
         *
         * KIPCM_UNLOCK(kipcm);
         */

        /* The SDU is ours now */
        return 0;
}
EXPORT_SYMBOL(kipcm_sdu_post);
