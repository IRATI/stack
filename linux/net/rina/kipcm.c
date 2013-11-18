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
#include <linux/kobject.h>
#include <linux/export.h>
#include <linux/kfifo.h>
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
#include "du.h"
#include "rnl.h"
#include "rnl-utils.h"
#include "kfa.h"
#include "kfa-utils.h"
#include "efcp-utils.h"

#define DEFAULT_FACTORY "normal-ipc"

struct flow_messages {
        struct kipcm_pmap * ingress;
        struct kipcm_smap * egress;
};

struct kipcm {
        struct mutex            lock;
        struct ipcp_factories * factories;
        struct ipcp_imap *      instances;
        struct flow_messages *  messages;
        struct rnl_set *        rnls;
        struct kfa *            kfa;
};

message_handler_cb kipcm_handlers[RINA_C_MAX];

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

static void
alloc_flow_req_free(struct name *                              source_name,
                    struct name *                              dest_name,
                    struct flow_spec *                         fspec,
                    struct name *                              dif_name,
                    struct rnl_ipcm_alloc_flow_req_msg_attrs * attrs,
                    struct rnl_msg *                           msg,
                    struct rina_msg_hdr *                      hdr)
{
        if (source_name) name_destroy(source_name);
        if (dest_name)   name_destroy(dest_name);
        if (fspec)       rkfree(fspec);
        if (dif_name)    name_destroy(dif_name);
        if (attrs) {
#if 0
                if (attrs->source)   name_destroy(attrs->source);
                if (attrs->dest)     name_destroy(attrs->dest);
                if (attrs->fspec)    rkfree(attrs->fspec);
                if (attrs->dif_name) name_destroy(attrs->dif_name);
#endif
                rkfree(attrs);
        }
        if (msg)         rkfree(msg);
        if (hdr)         rkfree(hdr);
}

static int
alloc_flow_req_free_and_reply(struct name *         source_name,
                              struct name *         dest_name,
                              struct flow_spec *    fspec,
                              struct name *         dif_name,
                              struct rnl_ipcm_alloc_flow_req_msg_attrs * attrs,
                              struct rnl_msg *      msg,
                              struct rina_msg_hdr * hdr,
                              ipc_process_id_t      id,
                              uint_t                res,
                              uint_t                seq_num,
                              uint_t                port_id,
                              port_id_t             pid)
{
        alloc_flow_req_free(source_name, dest_name, fspec, dif_name,
                            attrs, msg, hdr);

        if (rnl_app_alloc_flow_result_msg(id, res, pid, seq_num, port_id)) {
                LOG_ERR("Could not send flow_result_msg");
                return -1;
        }

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
        port_id_t                                  pid;

        source   = NULL;
        dest     = NULL;
        dif_name = NULL;
        fspec    = NULL;
        attrs    = NULL;
        msg      = NULL;
        hdr      = NULL;

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
                                                     hdr,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid,
                                                     port_id_bad());
        }

        source = name_create();
        if (!source) {
                return alloc_flow_req_free_and_reply(source,
                                                     dest,
                                                     fspec,
                                                     dif_name,
                                                     attrs,
                                                     msg,
                                                     hdr,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid,
                                                     port_id_bad());
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
                                                     hdr,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid,
                                                     port_id_bad());
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
                                                     hdr,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid,
                                                     port_id_bad());
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
                                                     hdr,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid,
                                                     port_id_bad());
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
                                                     hdr,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid,
                                                     port_id_bad());
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
                                                     hdr,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid,
                                                     port_id_bad());
        }
        msg->rina_hdr = hdr;

        if (rnl_parse_msg(info, msg)) {
                return alloc_flow_req_free_and_reply(source,
                                                     dest,
                                                     fspec,
                                                     dif_name,
                                                     attrs,
                                                     msg,
                                                     hdr,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid,
                                                     port_id_bad());
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
                                                     hdr,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid,
                                                     port_id_bad());
        }

        pid = kfa_flow_create(kipcm->kfa, ipc_id, false);
        ASSERT(is_port_id_ok(pid));
        if (kipcm_pmap_add(kipcm->messages->ingress, pid, info->snd_seq)) {
                LOG_ERR("Could not add map [pid, seq_num]: [%d, %d]",
                        pid, info->snd_seq);
                kfa_flow_deallocate(kipcm->kfa, pid);
                return alloc_flow_req_free_and_reply(source,
                                                     dest,
                                                     fspec,
                                                     dif_name,
                                                     attrs,
                                                     msg,
                                                     hdr,
                                                     0,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid,
                                                     port_id_bad());
        }

        if (ipc_process->ops->flow_allocate_request(ipc_process->data,
                                                    attrs->source,
                                                    attrs->dest,
                                                    attrs->fspec,
                                                    pid)) {
                LOG_ERR("Failed allocating flow request");
                kfa_flow_deallocate(kipcm->kfa, pid);
                return alloc_flow_req_free_and_reply(source,
                                                     dest,
                                                     fspec,
                                                     dif_name,
                                                     attrs,
                                                     msg,
                                                     hdr,
                                                     ipc_id,
                                                     -1,
                                                     info->snd_seq,
                                                     info->snd_portid,
                                                     port_id_bad());
        }

        alloc_flow_req_free(source, dest, fspec, dif_name, attrs, msg, hdr);

        return 0;
}


static void
alloc_flow_resp_free(struct rnl_alloc_flow_resp_msg_attrs * attrs,
                     struct rnl_msg *                       msg,
                     struct rina_msg_hdr *                  hdr)
{
        /* FIXME: Do it recursively ! */
        if (attrs) rkfree(attrs);
        if (hdr)   rkfree(hdr);
        if (msg)   rkfree(msg);
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
        port_id_t                              pid;

        msg         = NULL;
        hdr         = NULL;
        attrs       = NULL;
        ipc_process = NULL;

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
                alloc_flow_resp_free(attrs, msg, hdr);
                return -1;
        }

        hdr = rkzalloc(sizeof(*hdr), GFP_KERNEL);
        if (!hdr) {
                alloc_flow_resp_free(attrs, msg, hdr);
                return -1;
        }
        msg->attrs    = attrs;
        msg->rina_hdr = hdr;

        if (rnl_parse_msg(info, msg)) {
                alloc_flow_resp_free(attrs, msg, hdr);
                return -1;
        }
        ipc_id      = msg->rina_hdr->dst_ipc_id;
        ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipc_process) {
                LOG_ERR("IPC process %d not found", ipc_id);
                alloc_flow_resp_free(attrs, msg, hdr);
                return -1;
        }

        pid = kipcm_smap_find(kipcm->messages->egress, info->snd_seq);
        if (!is_port_id_ok(pid)) {
                LOG_ERR("Could not find port id %d for response %d",
                        pid, info->snd_seq);
                alloc_flow_resp_free(attrs, msg, hdr);
                return -1;
        }

        if (ipc_process->ops->flow_allocate_response(ipc_process->data,
                                                     pid,
                                                     0)) {
                LOG_ERR("Failed allocate flow response for port id: %d",
                        attrs->id);
                alloc_flow_resp_free(attrs, msg, hdr);
                return -1;
        }

        alloc_flow_resp_free(attrs, msg, hdr);

        return 0;
}

static int
dealloc_flow_req_free_and_reply(struct rnl_ipcm_dealloc_flow_req_msg_attrs * attrs,
                                struct rnl_msg *      msg,
                                struct rina_msg_hdr * hdr,
                                ipc_process_id_t      id,
                                uint_t                res,
                                uint_t                seq_num,
                                uint_t                port_id)
{
        if (attrs) rkfree(attrs);
        if (msg)   rkfree(msg);
        if (hdr)   rkfree(hdr);

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
        hdr   = NULL;

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
                                                       hdr,
                                                       0,
                                                       -1,
                                                       info->snd_seq,
                                                       info->snd_portid);

        msg = rkzalloc(sizeof(*msg), GFP_KERNEL);
        if (!msg)
                return dealloc_flow_req_free_and_reply(attrs,
                                                       msg,
                                                       hdr,
                                                       0,
                                                       -1,
                                                       info->snd_seq,
                                                       info->snd_portid);
        msg->attrs = attrs;

        hdr = rkzalloc(sizeof(*hdr), GFP_KERNEL);
        if (!hdr)
                return dealloc_flow_req_free_and_reply(attrs,
                                                       msg,
                                                       hdr,
                                                       0,
                                                       -1,
                                                       info->snd_seq,
                                                       info->snd_portid);

        msg->rina_hdr = hdr;

        if (rnl_parse_msg(info, msg))
                return dealloc_flow_req_free_and_reply(attrs,
                                                       msg,
                                                       hdr,
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
                                                       hdr,
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
                                                       hdr,
                                                       ipc_id,
                                                       -1,
                                                       info->snd_seq,
                                                       info->snd_portid);
        }

        return dealloc_flow_req_free_and_reply(attrs,
                                               msg,
                                               hdr,
                                               ipc_id,
                                               0,
                                               info->snd_seq,
                                               info->snd_portid);
}

static int
assign_to_dif_free_and_reply(struct rnl_ipcm_assign_to_dif_req_msg_attrs * attrs,
                             struct rnl_msg *    msg,
                             ipc_process_id_t    id,
                             uint_t              res,
                             uint_t              seq_num,
                             uint_t              port_id)
{

        if (attrs) {
                if (attrs->dif_info)
                        dif_info_destroy(attrs->dif_info);

                rkfree(attrs);
        }
        if (msg) rkfree(msg);

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
        struct dif_info *                             dif_info;
        struct name *                                 dif_name;
        struct dif_config *                           dif_config;
        struct ipcp_instance *                        ipc_process;
        ipc_process_id_t                              ipc_id = 0;

        attrs      = NULL;
        msg        = NULL;
        dif_name   = NULL;
        dif_info   = NULL;
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
                goto fail;

        dif_info = rkzalloc(sizeof(struct dif_info), GFP_KERNEL);
        if (!dif_info)
                goto fail;

        attrs->dif_info = dif_info;

        dif_name = name_create();
        if (!dif_name)
                goto fail;

        dif_info->dif_name = dif_name;

        dif_config = dif_config_create();
        if (!dif_config)
                goto fail;

        dif_info->configuration = dif_config;

        msg = rkzalloc(sizeof(*msg), GFP_KERNEL);
        if (!msg)
                goto fail;

        msg->attrs = attrs;

        if (rnl_parse_msg(info, msg))
                goto fail;

        ipc_id = msg->rina_hdr->dst_ipc_id;

        ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipc_process) {
                LOG_ERR("IPC process %d not found", ipc_id);
                goto fail;
        }
        LOG_DBG("Found IPC Process with id %d", ipc_id);

        if (ipc_process->ops->assign_to_dif(ipc_process->data,
                                            attrs->dif_info)) {
                char * tmp = name_tostring(attrs->dif_info->dif_name);
                LOG_ERR("Assign to dif %s operation failed for IPC process %d",
                        tmp, ipc_id);
                rkfree(tmp);

                goto fail;
        }

        return assign_to_dif_free_and_reply(attrs,
                                            msg,
                                            ipc_id,
                                            0,
                                            info->snd_seq,
                                            info->snd_portid);

 fail:
        return assign_to_dif_free_and_reply(attrs,
                                            msg,
                                            ipc_id,
                                            -1,
                                            info->snd_seq,
                                            info->snd_portid);
}

static int
update_dif_config_free_and_reply(struct dif_config * dif_config,
                                 struct rnl_ipcm_update_dif_config_req_msg_attrs * attrs,
                                 struct rnl_msg *    msg,
                                 ipc_process_id_t    id,
                                 uint_t              res,
                                 uint_t              seq_num,
                                 uint_t              port_id)
{
        if (attrs)      rkfree(attrs);
        if (dif_config) dif_config_destroy(dif_config);
        if (msg)        rkfree(msg);

        if (rnl_update_dif_config_response(id, res, seq_num, port_id))
                return -1;

        return 0;
}

static int notify_ipcp_update_dif_config_request(void *             data,
                                                 struct sk_buff *   buff,
                                                 struct genl_info * info)
{
        struct kipcm *                                kipcm;
        struct rnl_ipcm_update_dif_config_req_msg_attrs * attrs;
        struct rnl_msg *                              msg;
        struct dif_config *                           dif_config;
        struct ipcp_instance *                        ipc_process;
        ipc_process_id_t                              ipc_id;

        attrs      = NULL;
        msg        = NULL;
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
                return update_dif_config_free_and_reply(dif_config,
                                                        attrs,
                                                        msg,
                                                        0,
                                                        -1,
                                                        info->snd_seq,
                                                        info->snd_portid);

        dif_config = rkzalloc(sizeof(struct dif_config), GFP_KERNEL);
        if (!dif_config)
                return update_dif_config_free_and_reply(dif_config,
                                                        attrs,
                                                        msg,
                                                        0,
                                                        -1,
                                                        info->snd_seq,
                                                        info->snd_portid);
        INIT_LIST_HEAD(&(dif_config->ipcp_config_entries));
        attrs->dif_config = dif_config;

        msg = rkzalloc(sizeof(*msg), GFP_KERNEL);
        if (!msg)
                return update_dif_config_free_and_reply(dif_config,
                                                        attrs,
                                                        msg,
                                                        0,
                                                        -1,
                                                        info->snd_seq,
                                                        info->snd_portid);

        msg->attrs = attrs;

        if (rnl_parse_msg(info, msg))
                return update_dif_config_free_and_reply(dif_config,
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
                return update_dif_config_free_and_reply(dif_config,
                                                        attrs,
                                                        msg,
                                                        0,
                                                        -1,
                                                        info->snd_seq,
                                                        info->snd_portid);
        }
        LOG_DBG("Found IPC Process with id %d", ipc_id);

        if (ipc_process->ops->update_dif_config(ipc_process->data,
                                                attrs->dif_config)) {
                LOG_ERR("Update DIF config operation failed for IPC process %d"
                        ,ipc_id);

                return update_dif_config_free_and_reply(dif_config,
                                                        attrs,
                                                        msg,
                                                        0,
                                                        -1,
                                                        info->snd_seq,
                                                        info->snd_portid);
        }

        return update_dif_config_free_and_reply(dif_config,
                                                attrs,
                                                msg,
                                                0,
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
        if (app_name) name_destroy(app_name);
        if (dif_name) name_destroy(dif_name);
        if (attrs)    {
#if 0
                /* FIXME: Code using reg_unreg_resp_free_and_reply is bogus */
                /* FIXME: Fix it first ! */
                if (attrs->app_name) name_destroy(attrs->app_name);
                if (attrs->dif_name) name_destroy(attrs->dif_name);
#endif
                rkfree(attrs);
        }
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

        LOG_DBG("Application registered");

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
                                                     ipc_id,
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

static int
conn_create_resp_free_and_reply(struct rnl_ipcp_conn_create_req_msg_attrs * attrs,
                                struct rnl_msg *                            msg,
                                struct rina_msg_hdr *                       hdr,
                                ipc_process_id_t                            ipc_id,
                                port_id_t                                   pid,
                                cep_id_t                                    src_cep,
                                rnl_sn_t                                    seq_num,
                                u32                                         nl_port_id)
{
        if (attrs) rkfree(attrs);
        if (msg)   rkfree(msg);
        if (hdr)   rkfree(hdr);

        if (rnl_ipcp_conn_create_resp_msg(ipc_id,
                                          pid,
                                          src_cep,
                                          seq_num,
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
        struct rina_msg_hdr *                       hdr;
        struct kipcm *                              kipcm;
        ipc_process_id_t                            ipc_id = 0;
        port_id_t                                   port_id = 0;
        cep_id_t                                    src_cep;

        attrs    = NULL;
        msg      = NULL;
        hdr      = NULL;

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
        msg   = rkzalloc(sizeof(*msg), GFP_KERNEL);
        hdr   = rkzalloc(sizeof(*hdr), GFP_KERNEL);

        if (!attrs || !msg || !hdr) {
                goto process_fail;
        }

        msg->rina_hdr = hdr;
        msg->attrs    = attrs;

        if (rnl_parse_msg(info, msg)) {
                goto process_fail;
        }

        port_id = attrs->port_id;
        ipc_id  = hdr->dst_ipc_id;
        ipcp    = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipcp) {
                goto process_fail;
        }

        src_cep = ipcp->ops->connection_create(ipcp->data,
                                               attrs->port_id,
                                               attrs->src_addr,
                                               attrs->dst_addr,
                                               attrs->qos_id,
                                               attrs->policies);

        if (!is_cep_id_ok(src_cep)) {
                LOG_ERR("IPC process could not create connection");
                goto process_fail;
        }

        return conn_create_resp_free_and_reply(attrs,
                                               msg,
                                               hdr,
                                               ipc_id,
                                               port_id,
                                               src_cep,
                                               info->snd_seq,
                                               info->snd_portid);

 process_fail:
        return conn_create_resp_free_and_reply(attrs,
                                               msg,
                                               hdr,
                                               ipc_id,
                                               port_id,
                                               cep_id_bad(),
                                               info->snd_seq,
                                               info->snd_portid);


}

/* FIXME: create_req and create_arrived are almost identicall,
 *  code should be reused */

static int
conn_create_result_free_and_reply(
                                  struct rnl_ipcp_conn_create_arrived_msg_attrs * attrs,
                                  struct rnl_msg *                                msg,
                                  struct rina_msg_hdr *                           hdr,
                                  ipc_process_id_t                                ipc_id,
                                  port_id_t                                       pid,
                                  cep_id_t                                        src_cep,
                                  cep_id_t                                        dst_cep,
                                  rnl_sn_t                                        seq_num,
                                  u32                                             nl_port_id)
{
        if (attrs) rkfree(attrs);
        if (msg)   rkfree(msg);
        if (hdr)   rkfree(hdr);

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
        struct rina_msg_hdr *                           hdr;
        struct kipcm *                                  kipcm;
        ipc_process_id_t                                ipc_id = 0;
        port_id_t                                       port_id = 0;
        cep_id_t                                        src_cep;

        attrs    = NULL;
        msg      = NULL;
        hdr      = NULL;

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
        msg   = rkzalloc(sizeof(*msg), GFP_KERNEL);
        hdr   = rkzalloc(sizeof(*hdr), GFP_KERNEL);

        if (!attrs || !msg || !hdr) {
                goto process_fail;
        }

        msg->rina_hdr = hdr;
        msg->attrs    = attrs;

        if (rnl_parse_msg(info, msg)) {
                goto process_fail;
        }

        port_id = attrs->port_id;
        ipc_id  = hdr->dst_ipc_id;
        ipcp    = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipcp) {
                goto process_fail;
        }


        src_cep = ipcp->ops->connection_create_arrived(ipcp->data,
                                                       attrs->port_id,
                                                       attrs->src_addr,
                                                       attrs->dst_addr,
                                                       attrs->qos_id,
                                                       attrs->dst_cep,
                                                       attrs->policies);

        if (!is_cep_id_ok(src_cep)) {
                LOG_ERR("IPC process could not create connection");
                goto process_fail;
        }

        return conn_create_result_free_and_reply(attrs,
                                                 msg,
                                                 hdr,
                                                 ipc_id,
                                                 port_id,
                                                 src_cep,
                                                 attrs->dst_cep,
                                                 info->snd_seq,
                                                 info->snd_portid);

 process_fail:
        return conn_create_result_free_and_reply(attrs,
                                                 msg,
                                                 hdr,
                                                 ipc_id,
                                                 port_id,
                                                 cep_id_bad(),
                                                 cep_id_bad(),
                                                 info->snd_seq,
                                                 info->snd_portid);


}

static int
conn_update_result_free_and_reply(
                                  struct rnl_ipcp_conn_update_req_msg_attrs * attrs,
                                  struct rnl_msg *                            msg,
                                  struct rina_msg_hdr *                       hdr,
                                  ipc_process_id_t                            ipc_id,
                                  uint_t                                      result,
                                  port_id_t                                   pid,
                                  rnl_sn_t                                    seq_num,
                                  u32                                         nl_port_id)
{
        if (attrs) rkfree(attrs);
        if (msg)   rkfree(msg);
        if (hdr)   rkfree(hdr);

        if (rnl_ipcp_conn_update_result_msg(ipc_id,
                                            pid,
                                            result,
                                            seq_num,
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
        struct rina_msg_hdr *                       hdr;
        struct kipcm *                              kipcm;
        ipc_process_id_t                            ipc_id = 0;
        port_id_t                                   port_id = 0;

        attrs    = NULL;
        msg      = NULL;
        hdr      = NULL;

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
        msg   = rkzalloc(sizeof(*msg), GFP_KERNEL);
        hdr   = rkzalloc(sizeof(*hdr), GFP_KERNEL);

        if (!attrs || !msg || !hdr) {
                goto process_fail;
        }

        msg->rina_hdr = hdr;
        msg->attrs    = attrs;

        if (rnl_parse_msg(info, msg)) {
                goto process_fail;
        }

        port_id = attrs->port_id;
        ipc_id  = hdr->dst_ipc_id;
        ipcp    = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipcp) {
                goto process_fail;
        }


        if (ipcp->ops->connection_update(ipcp->data,
                                         attrs->src_cep,
                                         attrs->dst_cep)) {
                goto process_fail;
        }

        return conn_update_result_free_and_reply(attrs,
                                                 msg,
                                                 hdr,
                                                 ipc_id,
                                                 0,
                                                 port_id,
                                                 info->snd_seq,
                                                 info->snd_portid);

 process_fail:
        return conn_update_result_free_and_reply(attrs,
                                                 msg,
                                                 hdr,
                                                 ipc_id,
                                                 -1,
                                                 port_id,
                                                 info->snd_seq,
                                                 info->snd_portid);


}

static int
conn_destroy_result_free_and_reply(
                                   struct rnl_ipcp_conn_destroy_req_msg_attrs * attrs,
                                   struct rnl_msg *                             msg,
                                   struct rina_msg_hdr *                        hdr,
                                   ipc_process_id_t                             ipc_id,
                                   uint_t                                       result,
                                   port_id_t                                    pid,
                                   rnl_sn_t                                     seq_num,
                                   u32                                          nl_port_id)
{
        if (attrs) rkfree(attrs);
        if (msg)   rkfree(msg);
        if (hdr)   rkfree(hdr);

        if (rnl_ipcp_conn_destroy_result_msg(ipc_id,
                                             pid,
                                             result,
                                             seq_num,
                                             nl_port_id)) {
                LOG_ERR("Could not snd conn_destroy_result_msg");
                return -1;
        }

        return 0;
}

static int notify_ipcp_conn_destroy_req(void *             data,
                                        struct sk_buff *   buff,
                                        struct genl_info * info) {
        struct rnl_ipcp_conn_destroy_req_msg_attrs * attrs;
        struct rnl_msg *                             msg;
        struct ipcp_instance *                       ipcp;
        struct rina_msg_hdr *                        hdr;
        struct kipcm *                               kipcm;
        ipc_process_id_t                             ipc_id = 0;
        port_id_t                                    port_id = 0;

        attrs    = NULL;
        msg      = NULL;
        hdr      = NULL;

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
        msg   = rkzalloc(sizeof(*msg), GFP_KERNEL);
        hdr   = rkzalloc(sizeof(*hdr), GFP_KERNEL);

        if (!attrs || !msg || !hdr) {
                goto process_fail;
        }

        msg->rina_hdr = hdr;
        msg->attrs    = attrs;

        if (rnl_parse_msg(info, msg)) {
                goto process_fail;
        }

        port_id = attrs->port_id;
        ipc_id  = hdr->dst_ipc_id;
        ipcp    = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipcp) {
                goto process_fail;
        }

        if (ipcp->ops->connection_destroy(ipcp->data, attrs->src_cep)) {
                goto process_fail;
        }

        return conn_destroy_result_free_and_reply(attrs,
                                                  msg,
                                                  hdr,
                                                  ipc_id,
                                                  0,
                                                  port_id,
                                                  info->snd_seq,
                                                  info->snd_portid);

 process_fail:
        return conn_destroy_result_free_and_reply(attrs,
                                                  msg,
                                                  hdr,
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


static int netlink_handlers_unregister(struct rnl_set * rnls)
{
        int retval = 0;
        int i;

        for (i=1; i < RINA_C_MAX; i++) {
                if (kipcm_handlers[i] != NULL) {
                        if (rnl_handler_unregister(rnls, i))
                                retval = -1;
                }
        }

        LOG_DBG("NL handlers unregistered %s",
                (retval == 0) ? "successfully" : "unsuccessfully");

        return retval;
}

static int netlink_handlers_register(struct kipcm * kipcm)
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

        for (i=1; i < RINA_C_MAX; i++) {
                if (kipcm_handlers[i] != NULL) {
                        if (rnl_handler_register(kipcm->rnls,
                                                 i,
                                                 kipcm,
                                                 kipcm_handlers[i])) {
                                for (j = i-1; j > 0; j--) {
                                        if (kipcm_handlers[j] != NULL) {
                                                if (rnl_handler_unregister(kipcm->rnls, j)) {
                                                        LOG_ERR("Failed handler unregister while bailing out");
                                                        /* FIXME: What else could be done here?" */
                                                }
                                        }
                                }
                                return -1;
                        }
                }
        }

        LOG_DBG("NL handlers registered successfully");
        return 0;
}

struct kipcm * kipcm_create(struct kobject * parent,
                            struct rnl_set * rnls)
{
        struct kipcm * tmp;

        LOG_DBG("Initializing");

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->factories = ipcpf_init(parent);
        if (!tmp->factories) {
                LOG_ERR("Failed to build factories");
                rkfree(tmp);
                return NULL;
        }

        tmp->instances = ipcp_imap_create();
        if (!tmp->instances) {
                LOG_ERR("Failed to build imap");
                if (ipcpf_fini(tmp->factories)) {
                        /* FIXME: What could we do here ? */
                }
                rkfree(tmp);
                return NULL;
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
                return NULL;
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
                return NULL;
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
                return NULL;
        }

        if (rnl_set_register(rnls)) {
                if (kipcm_pmap_destroy(tmp->messages->ingress)) {
                        /* FIXME: What could we do here ? */
                }
                if (kipcm_smap_destroy(tmp->messages->egress)) {
                        /* FIXME: What could we do here ? */
                }
                if (kfa_destroy(tmp->kfa)) {
                        /* FIXME: What could we do here ? */
                }
                if (ipcp_imap_destroy(tmp->instances)) {
                        /* FIXME: What could we do here ? */
                }
                if (ipcpf_fini(tmp->factories)) {
                        /* FIXME: What could we do here ? */
                }
                rkfree(tmp);
                return NULL;
        }
        tmp->rnls = rnls;

        if (netlink_handlers_register(tmp)) {
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

        if (netlink_handlers_unregister(kipcm->rnls)) {
                /* FIXME: What should we do here ? */
        }

        if (rnl_set_unregister(kipcm->rnls)) {
                /* FIXME: What should we do here? */
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
        int retval;

        IRQ_BARRIER;

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

        IRQ_BARRIER;

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

int kipcm_ipcp_destroy(struct kipcm *   kipcm,
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

        /* FIXME: Should we look for pending flows from this IPC Process ? */
        if (kfa_remove_all_for_id(kipcm->kfa, id)) {
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

int kipcm_flow_arrived(struct kipcm *     kipcm,
                       ipc_process_id_t   ipc_id,
                       port_id_t          port_id,
                       struct name *      dif_name,
                       struct name *      source,
                       struct name *      dest,
                       struct flow_spec * fspec)
{
        uint_t             nl_port_id;
        rnl_sn_t           seq_num;
        struct ipcp_flow * flow;

        IRQ_BARRIER;

        /* FIXME: Use a constant (define) ! */
        nl_port_id = 1;

        /*
         * NB: This flow find is just a check, I think it's useful to be sure
         * the arrived flow request has been properly processed by the
         * IPC process calling this API.
         */
        flow = kfa_find_flow_by_pid(kipcm->kfa, port_id);
        if (!flow) {
                LOG_DBG("There's no flow pending for port_id: %d", port_id);
                return -1;
        }
        seq_num = rnl_get_next_seqn(kipcm->rnls);
        if (kipcm_smap_add_gfp(GFP_ATOMIC, kipcm->messages->egress,
                               seq_num, port_id)) {
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

int kipcm_flow_commit(struct kipcm *   kipcm,
                      ipc_process_id_t ipc_id,
                      port_id_t        port_id)
{
        struct ipcp_instance * ipc_process;

        IRQ_BARRIER;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        KIPCM_LOCK(kipcm);


        ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);

        if (!ipc_process) {
                LOG_ERR("Couldn't find the ipc process %d", ipc_id);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        if (kfa_flow_bind(kipcm->kfa,
                          port_id,
                          ipc_process,
                          ipc_id)) {
                LOG_ERR("Couldn't commit flow");
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        KIPCM_UNLOCK(kipcm);

        return 0;
}
EXPORT_SYMBOL(kipcm_flow_commit);

int kipcm_sdu_write(struct kipcm * kipcm,
                    port_id_t      port_id,
                    struct sdu *   sdu)
{
        IRQ_BARRIER;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        if (!sdu || !is_sdu_ok(sdu)) {
                LOG_ERR("Bogus SDU received, bailing out");
                return -1;
        }

        LOG_DBG("Tring to write SDU of size %zd to port_id %d",
                sdu->buffer->size, port_id);

        if (kfa_flow_sdu_write(kipcm->kfa, port_id, sdu))
                return -1;

        /* The SDU is ours */

        return 0;
}

int kipcm_sdu_read(struct kipcm * kipcm,
                   port_id_t      port_id,
                   struct sdu **  sdu)
{
        IRQ_BARRIER;

        /* The SDU is theirs now */
        if (kfa_flow_sdu_read(kipcm->kfa, port_id, sdu)) {
                LOG_DBG("Failed to read sdu");
                return -1;
        }

        return 0;
}

int kipcm_notify_flow_alloc_req_result(struct kipcm *   kipcm,
                                       ipc_process_id_t ipc_id,
                                       port_id_t        pid,
                                       uint_t           res)
{
        rnl_sn_t seq_num;

        IRQ_BARRIER;

        if (!is_port_id_ok(pid)) {
                LOG_ERR("Flow id is not ok");
        }

        seq_num = kipcm_pmap_find(kipcm->messages->ingress, pid);
        if (!is_seq_num_ok(seq_num)) {
                LOG_ERR("Could not find request message id (seq num)");
                return -1;
        }

        /*
         * FIXME: The rnl_port_id shouldn't be hardcoded as 1.
         */
        if (rnl_app_alloc_flow_result_msg(ipc_id, res, pid, seq_num, 1))
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

/*  FIXME: Testng code reutilization regarding connections */
#if 0
static int
conn_generic_free_and_reply(msg_type_t            op_code;
                            void *                attrs,
                            struct rnl_msg *      msg,
                            struct rina_msg_hdr * hdr,
                            ipc_process_id_t      ipc_id,
                            port_id_t             pid,
                            cep_id_t              src_cep,
                            cep_id_t              dst_cep,
                            rnl_sn_t              seq_num,
                            u32                   nl_port_id)
{

        if (attrs) rkfree(attrs);
        if (msg)   rkfree(msg);
        if (hdr)   rkfree(hdr);

        switch(op_code) {
        case RINA_C_IPCP_CONN_CREATE_REQUEST:
                if (rnl_ipcp_conn_create_resp_msg(ipc_id,
                                                  pid,
                                                  src_cep,
                                                  seq_num,
                                                  nl_port_id)) {
                        LOG_ERR("Could not snd conn_create_resp_msg");
                        return -1;
                }

        case RINA_C_IPCP_CONN_CREATE_ARRIVED:
                if (rnl_ipcp_conn_create_result_msg(ipc_id,
                                                    pid,
                                                    src_cep,
                                                    dst_cep,
                                                    seq_num,
                                                    nl_port_id)) {
                        LOG_ERR("Could not snd conn_create_result_msg");
                        return -1;
                }

        }

        return 0;
}

static int notify_ipcp_conn_create_generic(void *             data,
                                           struct sk_buff *   buff,
                                           struct genl_info * info)
{
        struct rnl_ipcp_conn_create_arrived_msg_attrs * attrs;
        struct rnl_msg *                                msg;
        struct ipcp_instance *                          ipcp;
        struct rina_msg_hdr *                           hdr;
        struct kipcm *                                  kipcm;
        ipc_process_id_t                                ipc_id = 0;
        port_id_t                                       port_id = 0;
        cep_id_t                                        src_cep;
        flow_id_t                                       fid = flow_id_bad();

        attrs    = NULL;
        msg      = NULL;
        hdr      = NULL;

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
        msg   = rkzalloc(sizeof(*msg), GFP_KERNEL);
        hdr   = rkzalloc(sizeof(*hdr), GFP_KERNEL);

        if (!attrs || !msg || !hdr) {
                goto process_fail;
        }

        msg->rina_hdr = hdr;
        msg->attrs    = attrs;

        if (rnl_parse_msg(info, msg)) {
                goto process_fail;
        }

        port_id = attrs->port_id;
        ipc_id  = hdr->dst_ipc_id;
        ipcp    = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!ipcp) {
                goto process_fail;
        }


        fid = kfa_flow_create(kipcm->kfa);
        ASSERT(is_flow_id_ok(fid));

        src_cep = ipcp->ops->connection_create_arrived(ipcp->data,
                                                       attrs->port_id,
                                                       attrs->src_addr,
                                                       attrs->dst_addr,
                                                       attrs->qos_id,
                                                       attrs->dst_cep,
                                                       attrs->policies);

        if (!is_cep_id_ok(src_cep)) {
                LOG_ERR("IPC process could not create connection");
                goto process_fail;
        }

        if (kfa_flow_bind(kipcm->kfa, fid, port_id, ipcp, ipc_id)) {
                LOG_ERR("Cound not bind flow (normal ipcp)");
                goto process_fail;
        }

        return conn_create_result_free_and_reply(attrs,
                                                 msg,
                                                 hdr,
                                                 ipc_id,
                                                 port_id,
                                                 src_cep,
                                                 attrs->dst_cep,
                                                 info->snd_seq,
                                                 info->snd_portid);

 process_fail:
        if (is_flow_id_ok(fid)) kfa_flow_deallocate(kipcm->kfa, fid);
        return conn_create_result_free_and_reply(attrs,
                                                 msg,
                                                 hdr,
                                                 ipc_id,
                                                 port_id,
                                                 cep_id_bad(),
                                                 cep_id_bad(),
                                                 info->snd_seq,
                                                 info->snd_portid);


}
#endif
