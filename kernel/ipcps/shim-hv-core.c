/*
 * Shim IPC process for hypervisors
 *
 *   Vincenzo Maffione <v.maffione@nextworks.it>
 *
 * CONTRIBUTORS:
 *
 *   Leonardo Bergesio <leonardo.bergesio@i2cat.net>
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/mutex.h>

#define SHIM_NAME   "shim-hv"
#define RINA_PREFIX SHIM_NAME

#include "logs.h"
#include "common.h"
#include "kipcm.h"
#include "debug.h"
#include "utils.h"
#include "sdu.h"
#include "ipcp-utils.h"
#include "ipcp-factories.h"
#include "vmpi.h"
#include "rds/robjects.h"

/* FIXME: Pigsty workaround, to be removed immediately */
#if defined(CONFIG_VMPI_KVM_GUEST) && !defined(CONFIG_VMPI_KVM_GUEST_MODULE)
#error VMPI guest must be a module
#endif

/* FIXME: Pigsty workaround, to be removed immediately */
#if defined(CONFIG_VMPI_KVM_HOST) && !defined(CONFIG_VMPI_KVM_HOST_MODULE)
#error VMPI host must be a module
#endif

/* FIXME: To be removed ABSOLUTELY */
extern struct kipcm *default_kipcm;

static unsigned int vmpi_num_channels = 0;

/* Private data associated to shim IPC process factory. */
static struct ipcp_factory_data {
        struct list_head        instances;
} shim_hv_factory_data;

enum channel_state {
        CHANNEL_STATE_NULL = 0,
        CHANNEL_STATE_PENDING,
        CHANNEL_STATE_ALLOCATED,
};

struct shim_hv_channel {
        /* Internal state associated to the channel. */
        enum channel_state   state;
        /* In ALLOCATED state, this is the port-id supported by the channel. */
        port_id_t            port_id;
        /* In PENDING or ALLOCATED state, this is the application that
           currently holds the channel. */
        struct name          application_name;
        /* The user IPCP that uses this channel */
        struct ipcp_instance *user_ipcp;
};

enum shim_hv_command {
        CMD_ALLOCATE_REQ = 0,
        CMD_ALLOCATE_RESP,
        CMD_DEALLOCATE,
};

enum shim_hv_response {
        RESP_OK = 0,
        RESP_KO = 1
};

/* Per-IPC-process data structure that contains all
 * the VMPI-related information (among the other).
 */
struct shim_hv_vmpi {
        struct vmpi_ops        ops;
        struct shim_hv_channel *channels;
        unsigned int id;
};

/* Private data associated to a shim IPC process. */
struct ipcp_instance_data {
        struct list_head    node;
        ipc_process_id_t    id;
        struct name         name;
        int                 assigned;
        struct name         dif_name;
        struct flow_spec    fspec;
        struct kfa          *kfa;
        struct list_head    registered_applications;
        struct mutex        reg_lock;
        struct mutex        vc_lock;
        struct shim_hv_vmpi vmpi;
};

struct name_list_element {
        struct list_head node;
        struct name      application_name;
};

static ssize_t shim_hv_ipcp_attr_show(struct robject *        robj,
                         	     struct robj_attribute * attr,
                                     char *                  buf)
{
	struct ipcp_instance * instance;

	instance = container_of(robj, struct ipcp_instance, robj);
	if (!instance || !instance->data)
		return 0;

	if (strcmp(robject_attr_name(attr), "name") == 0)
		return sprintf(buf, "%s\n",
			name_tostring(&instance->data->name));
	if (strcmp(robject_attr_name(attr), "dif") == 0)
		return sprintf(buf, "%s\n",
			name_tostring(&instance->data->dif_name));
	if (strcmp(robject_attr_name(attr), "type") == 0)
		return sprintf(buf, "shim_hv\n");

	return 0;
}
RINA_SYSFS_OPS(shim_hv_ipcp);
RINA_ATTRS(shim_hv_ipcp, name, tpye, dif);
RINA_KTYPE(shim_hv_ipcp);

static unsigned int
port_id_to_channel(struct ipcp_instance_data *priv, port_id_t port_id)
{
        int i;

        for (i = 0; i < vmpi_num_channels; i++)
                if (priv->vmpi.channels[i].port_id == port_id)
                        return i;

        /* The channel 0 is the control channel, and cannot be bound to a
         * port id. For this reason we use 0 to report failure.
         */
        return 0;
}

/* TODO reuse this function across all the ipcps. */
static struct ipcp_instance_data *
find_instance(struct ipcp_factory_data *factory_data, ipc_process_id_t id)
{

        struct ipcp_instance_data *cur;

        list_for_each_entry(cur, &(factory_data->instances), node) {
                if (cur->id == id) {
                        return cur;
                }
        }

        return NULL;

}

static void ser_uint8(uint8_t **to, uint8_t x)
{
        *(*to) = x;
        *to += 1;
}

static void ser_uint32(uint8_t **to, uint32_t x)
{
        uint32_t *buf = (uint32_t*)(*to);

        *buf = x;
        *to += sizeof(uint32_t);
}

static void ser_string(uint8_t **to, string_t *str, uint32_t len)
{
        memcpy(*to, str, len);
        *to += len;
        *(*to) = '\0';
        *to += 1;
}

static int des_uint8(const uint8_t **from, uint8_t *x, int *len)
{
        if (*len == 0)
                return -1;

        *x = *(*from);
        *from += sizeof(uint8_t);
        *len -= sizeof(uint8_t);

        return 0;
}

static int des_uint32(const uint8_t **from, uint32_t *x, int *len)
{
        if (*len < sizeof(uint32_t))
                return -1;

        *x = *((uint32_t *)(*from));
        *from += sizeof(uint32_t);
        *len -= sizeof(uint32_t);

        return 0;
}

static int des_string(const uint8_t **from, const string_t **str, int *len)
{
        int l = *len;
        const uint8_t *f = *from;

        while (l && *f != '\0') {
                f++;
                l--;
        }

        if (*f != '\0')
                return -1;
        /* Skip the NULL character also. */
        f++;
        l--;

        *str = *from;
        *from = f;
        *len = l;

        return 0;
}

static int
shim_hv_send_ctrl_msg(struct ipcp_instance_data *priv, struct vmpi_buf *vb)
{
        int ret;

        ret = priv->vmpi.ops.write(&priv->vmpi.ops, 0, vb);
        LOG_DBGF("vmpi_write_kernel(0) --> %d", (int) ret);

        return ret < 0 ? -1 : 0;
}

/* Build and send an ALLOCATE_RESP message on the control channel. */
static int
shim_hv_send_allocate_resp(struct ipcp_instance_data *priv,
                           unsigned int ch, int response)
{
        struct vmpi_buf *vb;
        uint8_t *msg_ptr;

        vb = vmpi_buf_alloc(sizeof(uint8_t) + sizeof(uint32_t)
                               + sizeof(uint8_t), 0, GFP_ATOMIC);
        if (!vb) {
                LOG_ERR("%s: Failed to alloc buffer", __func__);
                return -1;
        }

        /* Build the message by serialization. */
        msg_ptr = vmpi_buf_data(vb);
        ser_uint8(&msg_ptr, CMD_ALLOCATE_RESP);
        ser_uint32(&msg_ptr, ch);
        ser_uint8(&msg_ptr, response);

        return shim_hv_send_ctrl_msg(priv, vb);
}

static int
shim_hv_send_deallocate(struct ipcp_instance_data *priv, unsigned int ch)
{
        struct vmpi_buf *vb;
        uint8_t *msg_ptr;

        vb = vmpi_buf_alloc(sizeof(uint8_t) + sizeof(uint32_t),
                            0, GFP_ATOMIC);
        if (!vb) {
                LOG_ERR("%s: Failed to alloc buffer", __func__);
                return -1;
        }

        /* Build the message by serialization. */
        msg_ptr = vmpi_buf_data(vb);
        ser_uint8(&msg_ptr, CMD_DEALLOCATE);
        ser_uint32(&msg_ptr, ch);

        return shim_hv_send_ctrl_msg(priv, vb);
}

/* Invoked from the RINA stack when an application issues a
 * FlowAllocateRequest to this shim IPC process.
 */
static int
shim_hv_flow_allocate_request(struct ipcp_instance_data *priv,
                              struct ipcp_instance      *user_ipcp,
                              const struct name *src_application,
                              const struct name *dst_application,
                              const struct flow_spec *fspec,
                              port_id_t port_id)
{
        uint32_t ch;
        string_t *src_name;
        string_t *dst_name;
        struct vmpi_buf *vb;
        size_t msg_len;
        uint8_t *msg_ptr;
        uint32_t slen, dlen;
        int err = -ENOMEM;

        /* TODO move this checks to the caller. */
        ASSERT(priv);
        ASSERT(src_application);
        ASSERT(dst_application);
        ASSERT(user_ipcp);

        if (unlikely(!priv->assigned)) {
                LOG_ERR("%s: IPC process not ready", __func__);
                return -ENOENT;
        }

        mutex_lock(&priv->vc_lock);

        /* Select an unused channel. */
        for (ch = 1; ch < vmpi_num_channels; ch++)
                if (priv->vmpi.channels[ch].state == CHANNEL_STATE_NULL)
                        break;

        if (ch == vmpi_num_channels) {
                LOG_INFO("no free channels available, try later");
                err = -EBUSY;
                goto src_name_alloc;
        }

        src_name = name_tostring(src_application);
        if (!src_name) {
                LOG_ERR("%s: allocating source naming info", __func__);
                goto src_name_alloc;
        }

        dst_name = name_tostring(dst_application);
        if (!dst_name) {
                LOG_ERR("%s: allocating destination naming info", __func__);
                goto dst_name_alloc;
        }

        /* Compute the length of the ALLOCATE-REQ message and allocate
         * a buffer for the message itself.
         */
        slen = strlen(src_name);
        dlen = strlen(dst_name);
        msg_len = 1 + sizeof(ch) + slen + 1 + dlen + 1;
        if (msg_len >= 2000) {
                LOG_ERR("%s: message too long %d", __func__, (int)msg_len);
                goto msg_alloc;
        }

        vb = vmpi_buf_alloc(msg_len, 0, GFP_KERNEL);
        if (vb == NULL) {
                LOG_ERR("%s: vmpi buffer allocation failed", __func__);
                goto msg_alloc;
        }

        /* Build the message by serialization. */
        msg_ptr = vmpi_buf_data(vb);
        ser_uint8(&msg_ptr, CMD_ALLOCATE_REQ);
        ser_uint32(&msg_ptr, ch);
        ser_string(&msg_ptr, src_name, slen);
        ser_string(&msg_ptr, dst_name, dlen);

        /* Send the message on the control channel. */
        shim_hv_send_ctrl_msg(priv, vb);

        err = 0;
        priv->vmpi.channels[ch].state = CHANNEL_STATE_PENDING;
        priv->vmpi.channels[ch].port_id = port_id;
        priv->vmpi.channels[ch].user_ipcp = user_ipcp;
        name_cpy(src_application, &priv->vmpi.channels[ch].application_name);

        LOG_DBGF("channel %d --> PENDING", ch);

 msg_alloc:
        rkfree(dst_name);
 dst_name_alloc:
        rkfree(src_name);
 src_name_alloc:
        mutex_unlock(&priv->vc_lock);

        return err;
}

/* Invoked from the RINA stack when an application issues a
 * FlowAllocateResponse to this shim IPC process.
 */
static int
shim_hv_flow_allocate_response(struct ipcp_instance_data *priv,
                               struct ipcp_instance      *user_ipcp,
                               port_id_t port_id, int result)
{
        unsigned int ch;
        int response = RESP_KO;
        struct ipcp_instance * ipcp;

        ASSERT(priv);
        ASSERT(is_port_id_ok(port_id));

        if (!user_ipcp) {
                LOG_ERR("Wrong user_ipcp passed, bailing out");
                kfa_port_id_release(priv->kfa, port_id);
                return -1;
        }

        if (unlikely(!priv->assigned)) {
                LOG_ERR("%s: IPC process not ready", __func__);
                kfa_port_id_release(priv->kfa, port_id);
                return -ENOENT;
        }

        mutex_lock(&priv->vc_lock);

        ch = port_id_to_channel(priv, port_id);

        if (!ch) {
                LOG_ERR("%s: unknown port-id %d", __func__, port_id);
                mutex_unlock(&priv->vc_lock);
                kfa_port_id_release(priv->kfa, port_id);
                return -1;
        }

        if (priv->vmpi.channels[ch].state != CHANNEL_STATE_PENDING) {
                LOG_ERR("%s: channel %u in invalid state %d", __func__, ch,
                        priv->vmpi.channels[ch].state);
                goto resp;
        }

        if (result == 0) {
                /* Positive response */
                ipcp = kipcm_find_ipcp(default_kipcm, priv->id);
                if (!ipcp) {
                        LOG_ERR("KIPCM could not retrieve this IPCP");
                        goto resp;
                }
                ASSERT(user_ipcp);
                priv->vmpi.channels[ch].user_ipcp = user_ipcp;
                ASSERT(user_ipcp->ops);
                ASSERT(user_ipcp->ops->flow_binding_ipcp);
                if (user_ipcp->ops->flow_binding_ipcp(user_ipcp->data,
                                                      port_id,
                                                      ipcp)) {
                        LOG_ERR("Could not bind flow with user_ipcp");
                        goto resp;
                }
                /* Let's do the transition to the ALLOCATED state. */
                priv->vmpi.channels[ch].state = CHANNEL_STATE_ALLOCATED;
                response = RESP_OK;
                LOG_DBGF("channel %d --> ALLOCATED", ch);
        } else {
                /* Negative response */
                kfa_port_id_release(priv->kfa, port_id);
                /* XXX The following call should be useless because of the last
                 * call.
                 */
                priv->vmpi.channels[ch].state = CHANNEL_STATE_NULL;
                priv->vmpi.channels[ch].port_id = port_id_bad();
                priv->vmpi.channels[ch].user_ipcp = NULL;
                name_fini(&priv->vmpi.channels[ch].application_name);
                LOG_DBGF("channel %d --> NULL", ch);
        }
 resp:
        mutex_unlock(&priv->vc_lock);

        shim_hv_send_allocate_resp(priv, ch, response);

        if (response != RESP_OK)
                kfa_port_id_release(priv->kfa, port_id);
        return (response == RESP_OK) ? 0 : -1;
}

static int
shim_hv_flow_deallocate_common(struct ipcp_instance_data *priv,
                               unsigned int ch, bool locked)
{
        int ret = 0;
        port_id_t port_id;
        struct ipcp_instance * user_ipcp;

        if (!ch || ch >= vmpi_num_channels) {
                LOG_ERR("%s: invalid channel %u", __func__, ch);
                return -1;
        }

        if (locked)
                mutex_lock(&priv->vc_lock);

        port_id = priv->vmpi.channels[ch].port_id;

        if (priv->vmpi.channels[ch].state == CHANNEL_STATE_NULL) {
                LOG_INFO("channel state is already NULL");
                goto out;
        }

        user_ipcp = priv->vmpi.channels[ch].user_ipcp;
        if(user_ipcp) {
                user_ipcp->ops->flow_unbinding_ipcp(user_ipcp->data,
                                                    port_id);
        }
        priv->vmpi.channels[ch].state = CHANNEL_STATE_NULL;
        priv->vmpi.channels[ch].port_id = port_id_bad();
        priv->vmpi.channels[ch].user_ipcp = NULL;
        name_fini(&priv->vmpi.channels[ch].application_name);
        LOG_DBGF("channel %d --> NULL", ch);

 out:
        if (locked)
                mutex_unlock(&priv->vc_lock);

        return ret;
}

/* Invoked from the RINA stack when an application issues a
 * FlowDeallocate to this shim IPC process.
 */
static int
shim_hv_flow_deallocate(struct ipcp_instance_data *priv, port_id_t port_id)
{
        int ret = 0;
        unsigned int ch;

        if (unlikely(!priv->assigned)) {
                LOG_ERR("%s: IPC process not ready", __func__);
                return -ENOENT;
        }

        mutex_lock(&priv->vc_lock);

        ch = port_id_to_channel(priv, port_id);
        /* If channel is equal to 0, it means that the channel
         * for this port-id has already been deallocated, so
         * we don't have to call shim_hv_flow_deallocate_common().
         */
        if (ch)
                ret = shim_hv_flow_deallocate_common(priv, ch, 0);

        mutex_unlock(&priv->vc_lock);

        if (ch && ch < vmpi_num_channels)
                shim_hv_send_deallocate(priv, ch);

        return ret;
}

static int shim_hv_unbind_user_ipcp(struct ipcp_instance_data * priv,
                                    port_id_t                   port_id)
{
        unsigned int ch;

        if (unlikely(!priv->assigned)) {
                LOG_ERR("%s: IPC process not ready", __func__);
                return -ENOENT;
        }

        mutex_lock(&priv->vc_lock);

        ch = port_id_to_channel(priv, port_id);
        if (ch)
                priv->vmpi.channels[ch].user_ipcp = NULL;

        mutex_unlock(&priv->vc_lock);

        return 0;
}


/* Handler invoked when receiving an ALLOCATE_REQ message from the control
 * channel.
 */
static void shim_hv_handle_allocate_req(struct ipcp_instance_data *priv,
                                        const uint8_t *msg, int len)
{
        struct name *src_application = NULL;
        struct name *dst_application = NULL;
        const string_t *src_name;
        const string_t *dst_name;
        port_id_t port_id;
        uint32_t ch;
        int err = -1;
        struct ipcp_instance * ipcp, * user_ipcp;

        if (des_uint32(&msg, &ch, &len)) {
                LOG_ERR("%s: truncated msg: while reading channel", __func__);
                goto out;
        }

        if (des_string(&msg, &src_name, &len)) {
                LOG_ERR("%s: truncated msg: while reading source application "
                        "name", __func__);
                goto out;
        }

        if (des_string(&msg, &dst_name, &len)) {
                LOG_ERR("%s: truncated msg: while reading destination "
                        "application name", __func__);
                goto out;
        }

        LOG_DBGF("received ALLOCATE_REQ(ch = %u, src = %s, dst = %s)",
                 ch, src_name, dst_name);

        if (ch >= vmpi_num_channels) {
                LOG_ERR("%s: bogus channel %u", __func__, ch);
                goto out;
        }

        mutex_lock(&priv->vc_lock);

        if (priv->vmpi.channels[ch].state != CHANNEL_STATE_NULL) {
                LOG_ERR("%s: channel %u in invalid state %d", __func__, ch,
                        priv->vmpi.channels[ch].state);
                goto reject;
        }

        src_application = string_toname(src_name);
        if (src_application == NULL) {
                LOG_ERR("%s: invalid src name %s", __func__, src_name);
                goto reject;
        }

        dst_application = string_toname(dst_name);
        if (dst_application == NULL) {
                LOG_ERR("%s: invalid dst name %s", __func__, dst_name);
                goto dst_app;
        }

        user_ipcp = kipcm_find_ipcp_by_name(default_kipcm,
                                            dst_application);
        if (!user_ipcp)
                user_ipcp = kfa_ipcp_instance(priv->kfa);
        ipcp = kipcm_find_ipcp(default_kipcm, priv->id);
        if (!user_ipcp || !ipcp) {
                LOG_ERR("Could not find required ipcps");
                goto port_alloc;
        }

        port_id = kfa_port_id_reserve(priv->kfa, priv->id);
        if (!is_port_id_ok(port_id)) {
                LOG_ERR("%s: kfa_port_id_reserve() failed", __func__);
                goto port_alloc;
        }

        if (!user_ipcp->ops->ipcp_name(user_ipcp->data)) {
                LOG_DBG("This flow goes for an app");
                if (kfa_flow_create(priv->kfa, port_id, ipcp)) {
                        LOG_ERR("Could not create flow in KFA");
                        goto flow_arrived;
                }
        }

        err = kipcm_flow_arrived(default_kipcm, priv->id, port_id,
                                 &priv->dif_name, dst_application,
                                 src_application, &priv->fspec);
        if (err) {
                LOG_ERR("%s: kipcm_flow_arrived() failed", __func__);
                goto flow_arrived;
        }

        /* Everything is ok: Let's do the transition to the PENDING
         * state.
         */
        err = 0;
        priv->vmpi.channels[ch].state = CHANNEL_STATE_PENDING;
        priv->vmpi.channels[ch].port_id = port_id;
        name_cpy(dst_application, &priv->vmpi.channels[ch].application_name);
        LOG_DBGF("channel %d --> PENDING", ch);

 flow_arrived:
        if (err)
                kfa_port_id_release(priv->kfa, port_id);
 port_alloc:
        if (dst_application)
                rkfree(dst_application);
 dst_app:
        if (src_application)
                rkfree(src_application);
 reject:
        mutex_unlock(&priv->vc_lock);
        if (err)
                shim_hv_send_allocate_resp(priv, ch, RESP_KO);
 out:
        return;
}

/* Handler invoked when receiving an ALLOCATE_RESP message from the control
 * channel.
 */
static void shim_hv_handle_allocate_resp(struct ipcp_instance_data *priv,
                                         const uint8_t *msg, int len)
{
        uint32_t ch;
        uint8_t response;
        port_id_t port_id;
        int ret = -1;
        struct ipcp_instance * ipcp;
        struct ipcp_instance * user_ipcp;

        if (des_uint32(&msg, &ch, &len)) {
                LOG_ERR("%s: truncated msg: while reading channel", __func__);
                goto out;
        }

        if (des_uint8(&msg, &response, &len)) {
                LOG_ERR("%s: truncated msg: while reading response", __func__);
                goto out;
        }

        LOG_DBGF("received ALLOCATE_RESP(ch = %d, resp = %u)",
                 ch, response);

        if (ch >= vmpi_num_channels) {
                LOG_ERR("%s: bogus channel %u", __func__, ch);
                goto out;
        }

        mutex_lock(&priv->vc_lock);

        if (priv->vmpi.channels[ch].state != CHANNEL_STATE_PENDING) {
                LOG_ERR("%s: channel %u in invalid state %d", __func__, ch,
                        priv->vmpi.channels[ch].state);
                goto out;
        }
        port_id = priv->vmpi.channels[ch].port_id;
        user_ipcp = priv->vmpi.channels[ch].user_ipcp;
        ASSERT(user_ipcp);

        if (response == RESP_OK) {
                /* Everything is ok: Let's do the transition to the ALLOCATED
                 * state. This is done **before** invoking
                 * kipcm_notify_alloc_req_result() so that we avoid a race
                 * condition with the application invoking the lockless
                 * sdu_write() when the channel is yet in the PENDING state.
                 */
                priv->vmpi.channels[ch].state = CHANNEL_STATE_ALLOCATED;
                LOG_DBGF("channel %d --> ALLOCATED", ch);

                wmb();
        }

        ipcp = kipcm_find_ipcp(default_kipcm, priv->id);
        if (!ipcp) {
                LOG_ERR("KIPCM could not retrieve this IPCP");
                response = RESP_KO;
                goto resp_ko;
        }
        ASSERT(user_ipcp->ops);
        ASSERT(user_ipcp->ops->flow_binding_ipcp);
        if (user_ipcp->ops->flow_binding_ipcp(user_ipcp->data,
                                              port_id,
                                              ipcp)) {
                LOG_ERR("Could not bind flow with user_ipcp");
                response = RESP_KO;
                goto resp_ko;
        }

        ret = kipcm_notify_flow_alloc_req_result(default_kipcm, priv->id,
                                                 port_id,
                                                 (response == RESP_OK) ? 0:1);
        if (ret) {
                LOG_ERR("%s: kipcm_notify_flow_alloc_req_result() failed",
                        __func__);
        }

 resp_ko:
        if (ret || response == RESP_KO) {
                priv->vmpi.channels[ch].state = CHANNEL_STATE_NULL;
                priv->vmpi.channels[ch].port_id = port_id_bad();
                priv->vmpi.channels[ch].user_ipcp = NULL;
                name_fini(&priv->vmpi.channels[ch].application_name);
                LOG_DBGF("channel %d --> NULL", ch);
                kfa_port_id_release(priv->kfa, port_id);
                user_ipcp->ops->flow_unbinding_ipcp(user_ipcp->data,
                                                    port_id);
        }
 out:
        mutex_unlock(&priv->vc_lock);

        return;
}

/* Handler invoked when receiving an DEALLOCATE message from the control
 * channel.
 */
static void shim_hv_handle_deallocate(struct ipcp_instance_data *priv,
                                      const uint8_t *msg, int len)
{
        uint32_t ch;

        if (des_uint32(&msg, &ch, &len)) {
                LOG_ERR("%s: truncated msg: while reading channel", __func__);
                return;
        }

        LOG_DBGF("received DEALLOCATE(ch = %d)", ch);

        kfa_port_id_release(priv->kfa, priv->vmpi.channels[ch].port_id);
        shim_hv_flow_deallocate_common(priv, ch, 1);
}

static void shim_hv_handle_control_msg(struct ipcp_instance_data *priv,
                                       struct vmpi_buf *vb)
{
        const uint8_t *msg;
        int len;
        uint8_t cmd;

        msg = vmpi_buf_data(vb);
        len = vmpi_buf_len(vb);

        /* Deserialize the control command code. */
        if (des_uint8(&msg, &cmd, &len)) {
                LOG_ERR("%s: truncated msg: while reading command", __func__);
                return;
        }

        /* Call the handler associated to command. */
        switch (cmd) {
        case CMD_ALLOCATE_REQ:
                shim_hv_handle_allocate_req(priv, msg, len);
                break;
        case CMD_ALLOCATE_RESP:
                shim_hv_handle_allocate_resp(priv, msg, len);
                break;
        case CMD_DEALLOCATE:
                shim_hv_handle_deallocate(priv, msg, len);
                break;
        default:
                LOG_ERR("%s: unknown cmd %u", __func__, cmd);
                break;
        }

        vmpi_buf_free(vb);
}

/* Invoked from the VMPI worker thread when a new message comes from
 * a channel.
 */
static void
shim_hv_recv_cb(void *opaque, unsigned int ch, struct vmpi_buf *vb)
{
        struct ipcp_instance_data *priv = opaque;
        port_id_t port_id;
        struct shim_hv_channel channel;
        int ret = 0;

        if (unlikely(ch == 0)) {
                /* Control channel. */
                shim_hv_handle_control_msg(priv, vb);
                return;
        }

        /* User data channel. */
        if (unlikely(ch >= vmpi_num_channels)) {
                LOG_ERR("%s: invalid channel %u", __func__, ch);
                return;
        }

        if (unlikely(priv->vmpi.channels[ch].state !=
                     CHANNEL_STATE_ALLOCATED)) {
                LOG_INFO("dropping packet from channel %u: no "
                         "associated flow", ch);
                return;
        }

        channel = priv->vmpi.channels[ch];
        port_id = channel.port_id;

        if (!channel.user_ipcp){
        	LOG_ERR("Flow is being deallocated, dropping SDU");
        	return;
        }

        ASSERT(channel.user_ipcp->ops);
        ASSERT(channel.user_ipcp->ops->sdu_enqueue);
        ret = channel.user_ipcp->ops->sdu_enqueue(channel.user_ipcp->data,
                                                  port_id,
                                                  vb);
        if (unlikely(ret)) {
                LOG_ERR("%s: sdu_enqueue() failed", __func__);
                return;
        }
        LOG_DBGF("SDU received");
}

static void
shim_hv_write_restart_cb(void *opaque)
{
}

/* Register an application to this IPC process. */
static int
shim_hv_application_register(struct ipcp_instance_data * priv,
                             const struct name         * application_name,
			     const struct name         * daf_name)
{
        struct name_list_element *cur;
        char *tmpstr = name_tostring(application_name);
        int ret = -1;

        mutex_lock(&priv->reg_lock);

        /* Is this application already registered? */
        list_for_each_entry(cur, &priv->registered_applications, node) {
                if (name_is_equal(application_name, &cur->application_name)) {
                        LOG_ERR("%s: Application %s already registered",
                                __func__, tmpstr);
                        goto out;
                }
        }

        /* Add the application to the list of registered applications. */
        cur = rkzalloc(sizeof(*cur), GFP_KERNEL);
        if (!cur) {
                LOG_ERR("%s: Allocating list element", __func__);
                goto out;
        }

        if (name_cpy(application_name, &cur->application_name)) {
                LOG_ERR("%s: name_cpy() failed", __func__);
                goto name_alloc;
        }

        list_add(&cur->node, &priv->registered_applications);

        ret = 0;
        LOG_DBGF("Application %s registered", tmpstr);

 name_alloc:
        if (ret) {
                rkfree(cur);
        }
 out:
        mutex_unlock(&priv->reg_lock);
        if (tmpstr)
                rkfree(tmpstr);

        return ret;
}

/* Unregister an application from this IPC process. */
static int
shim_hv_application_unregister(struct ipcp_instance_data *priv,
                               const struct name *application_name)
{
        struct name_list_element *cur;
        struct name_list_element *found = NULL;
        char *tmpstr = name_tostring(application_name);
        unsigned int ch;

        mutex_lock(&priv->reg_lock);

        /* Is this application registered? */
        list_for_each_entry(cur, &priv->registered_applications, node) {
                if (name_is_equal(application_name, &cur->application_name)) {
                        found = cur;
                        break;
                }
        }

        if (!found) {
                LOG_WARN("%s: Application %s not registered",
                         __func__, tmpstr);
                /*
                 * FIXME:
                 *   For now don't return error in this case, since these
                 *   spurious unregistrations seem to happen in the stack. Need
                 *   some more investigation.
                 */
                goto out;
        }

        /* Remove the application from the list of registered applications. */
        name_fini(&found->application_name);
        list_del(&found->node);
        rkfree(found);

        LOG_DBGF("Application %s unregistered", tmpstr);

 out:
        mutex_unlock(&priv->reg_lock);
        if (tmpstr)
                rkfree(tmpstr);

        mutex_lock(&priv->vc_lock);
        for (ch = 0; ch < vmpi_num_channels; ch++) {
                if (name_is_equal(application_name,
                                  &priv->vmpi.channels[ch].application_name)) {
                        shim_hv_flow_deallocate_common(priv, ch, 0);
                }
        }
        mutex_unlock(&priv->vc_lock);

        return 0;
}

/* Callback invoked when a shim IPC process is assigned to a DIF. */
static int
shim_hv_assign_to_dif(struct ipcp_instance_data *priv,
                      const struct name * dif_name,
		      const string_t * type,
		      struct dif_config * config)
{
        struct ipcp_config *elem;
        bool vmpi_id_found = false;
        int ret;

        ASSERT(priv);
        ASSERT(dif_information);

        if (priv->assigned) {
                ASSERT(priv->dif_name.process_name);
                LOG_ERR("%s: IPC process already assigned to the DIF %s",
                        __func__,  priv->dif_name.process_name);
                return -1;
        }

        if (name_cpy(dif_name, &priv->dif_name)) {
                LOG_ERR("%s: name_cpy() failed", __func__);
                return -1;
        }

        list_for_each_entry(elem, &(config->ipcp_config_entries), next) {
                const struct ipcp_config_entry *entry = elem->entry;

                if (!strcmp(entry->name, "vmpi-id")) {
                        ASSERT(entry->value);
                        ret = kstrtouint(entry->value, 10, &priv->vmpi.id);
                        if (ret) {
                                LOG_ERR("%s: Invalid vmpi-id", __func__);
                                return -1;
                        }
                        vmpi_id_found = true;
                } else {
                        LOG_WARN("%s: Unknown config param", __func__);
                }
        }

        if (!vmpi_id_found) {
                LOG_ERR("%s: Missing vmpi-id configuration parameter",
                        __func__);
                return -1;
        }

        /*
         * Try to get the VMPI instance specified by the "vmpi-id"
         * configuration parameter.
         */
        ret = vmpi_provider_find_instance(VMPI_PROVIDER_AUTO, priv->vmpi.id,
                                          &priv->vmpi.ops);
        if (ret) {
                LOG_ERR("%s: mpi instance %u not found", __func__, 0);
                return -1;
        }

        ret = priv->vmpi.ops.register_cbs(&priv->vmpi.ops,
                                          shim_hv_recv_cb,
                                          shim_hv_write_restart_cb,
                                          priv);
        if (ret) {
                LOG_ERR("%s: vmpi_register_read_callback() failed", __func__);
                return -1;
        }

        /* Now the IPC process is ready to be used. */
        priv->assigned = 1;

        LOG_DBGF("ipcp %d assigned to DIF %s, VMPI instance %u",
                 priv->id, priv->dif_name.process_name, priv->vmpi.id);

        return 0;
}

/* Invoked from the RINA stack when an application sends an
 * SDU to a flow managed by this shim IPC process.
 */
static int
shim_hv_sdu_write(struct ipcp_instance_data *   priv,
		  port_id_t 		        port_id,
                  struct vmpi_buf *	        vb,
                  bool                          blocking)
{
        unsigned int ch = port_id_to_channel(priv, port_id);
        int ret;

        if (unlikely(!priv->assigned)) {
                LOG_ERR("%s: IPC process not ready", __func__);
                return -1;
        }

        if (unlikely(!ch)) {
                LOG_ERR("%s: unknown port-id %d", __func__, port_id);
                return -1;
        }

        rmb();

        if (unlikely(priv->vmpi.channels[ch].state !=
                     CHANNEL_STATE_ALLOCATED)) {
                LOG_ERR("%s: channel %u in invalid state %d", __func__,
                        ch, priv->vmpi.channels[ch].state);
                return -1;
        }

        ret = priv->vmpi.ops.write(&priv->vmpi.ops, ch, vb);
        if (likely(ret > 0)) {
                LOG_DBGF("vmpi_write_kernel(%u) --> %d", ch, ret);
                ret = 0;
        }

        return ret;
}

static const struct name *
shim_hv_ipcp_name(struct ipcp_instance_data *priv)
{
        ASSERT(priv);
        ASSERT(name_is_ok(&priv->name));

        return &priv->name;
}

static const struct name *
shim_hv_dif_name(struct ipcp_instance_data *priv)
{
        ASSERT(priv);
        ASSERT(name_is_ok(&priv->dif_name));

        return &priv->dif_name;
}

static size_t
shim_hv_max_sdu_size(struct ipcp_instance_data *priv)
{
        ASSERT(priv);

        /* FIXME: return something that makes more sense*/
        return 200000;
}

ipc_process_id_t shim_hv_ipcp_id(struct ipcp_instance_data * data)
{
	ASSERT(data);
	return data->id;
}

static int shim_hv_query_rib(struct ipcp_instance_data * data,
                             struct list_head *          entries,
                             const string_t *            object_class,
                             const string_t *            object_name,
                             uint64_t                    object_instance,
                             uint32_t                    scope,
                             const string_t *            filter) {
	LOG_MISSING;
	return -1;
}

/* Interface exported by a shim IPC process. */
static struct ipcp_instance_ops shim_hv_ipcp_ops = {
        .flow_allocate_request     = shim_hv_flow_allocate_request,
        .flow_allocate_response    = shim_hv_flow_allocate_response,
        .flow_deallocate           = shim_hv_flow_deallocate,
        .flow_prebind              = NULL,
        .flow_binding_ipcp         = NULL,
        .flow_unbinding_ipcp       = NULL,
        .flow_unbinding_user_ipcp  = shim_hv_unbind_user_ipcp,
	.nm1_flow_state_change	   = NULL,

        .application_register      = shim_hv_application_register,
        .application_unregister    = shim_hv_application_unregister,

        .assign_to_dif             = shim_hv_assign_to_dif,
        .update_dif_config         = NULL,

        .connection_create         = NULL,
        .connection_update         = NULL,
        .connection_destroy        = NULL,
        .connection_create_arrived = NULL,
	.connection_modify 	   = NULL,

        .sdu_enqueue               = NULL,
        .sdu_write                 = shim_hv_sdu_write,

        .mgmt_sdu_write            = NULL,
        .mgmt_sdu_post             = NULL,

        .pff_add                   = NULL,
        .pff_remove                = NULL,
        .pff_dump                  = NULL,
        .pff_flush                 = NULL,
	.pff_modify		   = NULL,

        .query_rib		   = shim_hv_query_rib,

        .ipcp_name                 = shim_hv_ipcp_name,

        .set_policy_set_param      = NULL,
        .select_policy_set         = NULL,
        .update_crypto_state	   = NULL,
	.address_change		   = NULL,
        .dif_name		   = shim_hv_dif_name,
	.ipcp_id		   = shim_hv_ipcp_id,
	.max_sdu_size		   = shim_hv_max_sdu_size
};

/* Initialize the IPC process factory. */
static int shim_hv_factory_init(struct ipcp_factory_data * factory_data)
{
        ASSERT(factory_data);

        INIT_LIST_HEAD(&factory_data->instances);

        LOG_DBG("Initialized");

        return 0;
}

/* Uninitialize the IPC process factory. */
static int shim_hv_factory_fini(struct ipcp_factory_data * data)
{
        ASSERT(data);

        ASSERT(list_empty(&data->instances));

        LOG_DBG("Uninitialized");

        return 0;
}

/* Create a new shim IPC process. */
static struct ipcp_instance *
shim_hv_factory_ipcp_create(struct ipcp_factory_data * factory_data,
                            const struct name *        name,
                            ipc_process_id_t           id,
			    uint_t		       us_nl_port)
{
        struct ipcp_instance *      ipcp;
        struct ipcp_instance_data * priv;
        int i;

        /* Check if there already is an instance with the specified id. */
        if (find_instance(factory_data, id)) {
                LOG_ERR("%s: id %d already created", __func__, id);
                goto alloc_ipcp;
        }

        /* Allocate a new ipcp instance. */
        ipcp = rkzalloc(sizeof(*ipcp), GFP_KERNEL);
        if (!ipcp)
                goto alloc_ipcp;

        ipcp->ops = &shim_hv_ipcp_ops;

	if (robject_rset_init_and_add(&ipcp->robj,
				      &shim_hv_ipcp_rtype,
				      kipcm_rset(default_kipcm),
				      "%u",
				      id))
		goto alloc_data;

        /* Allocate private data for the new shim IPC process. */
        ipcp->data = priv = rkzalloc(sizeof(struct ipcp_instance_data),
                                     GFP_KERNEL);
        if (!priv)
                goto alloc_data;

        priv->id = id;
        priv->assigned = 0;
        bzero(&priv->fspec, sizeof(priv->fspec));
        priv->fspec.max_sdu_size      = vmpi_get_max_payload_size();
        priv->fspec.max_allowable_gap = -1;

        if (name_cpy(name, &priv->name)) {
                LOG_ERR("%s: name_cpy() failed", __func__);
                goto alloc_name;
        }

        INIT_LIST_HEAD(&priv->registered_applications);
        mutex_init(&priv->reg_lock);
        mutex_init(&priv->vc_lock);

        priv->kfa = kipcm_kfa(default_kipcm);
        ASSERT(priv->kfa);

        /* Initialize the VMPI-related data structure. */
        bzero(&priv->vmpi, sizeof(priv->vmpi));
        priv->vmpi.channels = rkzalloc(
                                       sizeof(priv->vmpi.channels[0]) * vmpi_num_channels,
                                       GFP_KERNEL);
        if (priv->vmpi.channels == NULL) {
                LOG_ERR("%s: channels allocation failed", __func__);
                goto alloc_channels;
        }
        for (i = 0; i < vmpi_num_channels; i++) {
                priv->vmpi.channels[i].state = CHANNEL_STATE_NULL;
                priv->vmpi.channels[i].port_id = port_id_bad();
                name_init_with(&priv->vmpi.channels[i].application_name,
                               NULL, NULL, NULL, NULL);
        }
        priv->vmpi.id = ~0U;

        /* Add this IPC process to the factory global list. */
        list_add(&priv->node, &factory_data->instances);

        LOG_DBGF("ipcp created (id = %d)", id);

        return ipcp;

 alloc_channels:
        name_fini(&priv->name);
 alloc_name:
        rkfree(priv);
 alloc_data:
        rkfree(ipcp);
 alloc_ipcp:
        LOG_ERR("%s: failed", __func__);

        return NULL;
}

/* Destroy a shim IPC process. */
static int
shim_hv_factory_ipcp_destroy(struct ipcp_factory_data * factory_data,
                             struct ipcp_instance *     ipcp)
{
        struct ipcp_instance_data *cur, *next, *found;

        ASSERT(factory_data);
        ASSERT(ipcp);

        found = NULL;
        list_for_each_entry_safe(cur, next, &factory_data->instances, node) {
                if (cur == ipcp->data) {
                        list_del(&cur->node);
                        found = cur;
                        break;
                }
        }

        if (!found) {
                LOG_ERR("%s: entry not found", __func__);
        }

        name_fini(&ipcp->data->name);
        rkfree(ipcp->data->vmpi.channels);
        name_fini(&ipcp->data->dif_name);
        robject_del(&ipcp->robj);
        LOG_DBGF("ipcp destroyed (id = %d)", ipcp->data->id);
        rkfree(ipcp->data);
        rkfree(ipcp);

        return 0;
}

static struct ipcp_factory_ops shim_hv_factory_ops = {
        .init    = shim_hv_factory_init,
        .fini    = shim_hv_factory_fini,
        .create  = shim_hv_factory_ipcp_create,
        .destroy = shim_hv_factory_ipcp_destroy,
};

/* A factory for shim IPC processes for hypervisors. */
static struct ipcp_factory *shim_hv_ipcp_factory = NULL;

static int __init shim_hv_init(void)
{
        vmpi_num_channels = 64;

        shim_hv_ipcp_factory =
                kipcm_ipcp_factory_register(default_kipcm, SHIM_NAME,
                                            &shim_hv_factory_data,
                                            &shim_hv_factory_ops);

        if (shim_hv_ipcp_factory == NULL) {
                LOG_ERR("%s: factory registration failed", __func__);
                return -1;
        }

        LOG_INFO("Shim for Hypervisors support loaded");

        return 0;
}

static void __exit shim_hv_fini(void)
{
        ASSERT(shim_hv_ipcp_factory);

        kipcm_ipcp_factory_unregister(default_kipcm, shim_hv_ipcp_factory);

        LOG_INFO("Shim for Hypervisors support unloaded");
}

module_init(shim_hv_init);
module_exit(shim_hv_fini);

MODULE_DESCRIPTION("RINA Shim IPC for Hypervisors");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vincenzo Maffione <v.maffione@nextworks.it>");
