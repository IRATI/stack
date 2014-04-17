/*
 * Shim IPC process for hypervisors
 *
 *      Author: Vincenzo Maffione <v.maffione@nextworks.it>
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

#define SHIM_HV_NAME    "shim-hv-virtio"
#define RINA_PREFIX SHIM_HV_NAME

#include "logs.h"
#include "common.h"
#include "kipcm.h"
#include "debug.h"
#include "utils.h"
#include "du.h"
#include "ipcp-utils.h"
#include "ipcp-factories.h"


/* FIXME: To be removed ABSOLUTELY */
extern struct kipcm *default_kipcm;

/* Private data associated to a shim IPC process. */
struct ipcp_instance_data {
        struct list_head        list;
        ipc_process_id_t        id;
        struct name             *name;
        struct kfa              *kfa;
};

/* Private data associated to shim IPC process factory. */
static struct ipcp_factory_data {
        struct list_head instances;
} shim_hv_factory_data;

/* Interface exported by a shim IPC process. */
static struct ipcp_instance_ops shim_hv_ipcp_ops = {
        .flow_allocate_request     = NULL,
        .flow_allocate_response    = NULL,
        .flow_deallocate           = NULL,
        .flow_binding_ipcp         = NULL,
        .flow_destroy              = NULL,

        .application_register      = NULL,
        .application_unregister    = NULL,

        .assign_to_dif             = NULL,
        .update_dif_config         = NULL,

        .connection_create         = NULL,
        .connection_update         = NULL,
        .connection_destroy        = NULL,
        .connection_create_arrived = NULL,

        .sdu_enqueue               = NULL,
        .sdu_write                 = NULL,

        .mgmt_sdu_read             = NULL,
        .mgmt_sdu_write            = NULL,
        .mgmt_sdu_post             = NULL,

        .pft_add                   = NULL,
        .pft_remove                = NULL,
        .pft_dump                  = NULL,
        .pft_flush                 = NULL,

        .ipcp_name                 = NULL,
};

/* Initialize the IPC process factory. */
static int
shim_hv_factory_init(struct ipcp_factory_data *factory_data)
{
        ASSERT(factory_data);

        bzero(factory_data, sizeof(*factory_data));
        INIT_LIST_HEAD(&(factory_data->instances));

        LOG_INFO("%s initialized", SHIM_HV_NAME);

        return 0;
}

/* Uninitialize the IPC process factory. */
static int
shim_hv_factory_fini(struct ipcp_factory_data *data)
{
        ASSERT(data);

        ASSERT(list_empty(&data->instances));

        LOG_INFO("%s uninitialized", SHIM_HV_NAME);

        return 0;
}

/* TODO reuse this function across all the ipcps. */
static struct ipcp_instance_data *
find_instance(struct ipcp_factory_data *factory_data, ipc_process_id_t id)
{

        struct ipcp_instance_data *cur;

        list_for_each_entry(cur, &(factory_data->instances), list) {
                if (cur->id == id) {
                        return cur;
                }
        }

        return NULL;

}

/* Create a new shim IPC process. */
static struct ipcp_instance *
shim_hv_factory_ipcp_create(struct ipcp_factory_data *factory_data,
                            const struct name *name, ipc_process_id_t id)
{
        struct ipcp_instance *ipcp;
        struct ipcp_instance_data *priv;

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

        /* Allocate private data for the new shim IPC process. */
        ipcp->data = priv = rkzalloc(sizeof(struct ipcp_instance_data), GFP_KERNEL);
        if (!priv)
                goto alloc_data;

        priv->id = id;
        priv->name = name_dup(name);
        if (!priv->name) {
                LOG_ERR("name_dup() failed\n");
                goto alloc_name;
        }

        priv->kfa = kipcm_kfa(default_kipcm);
        ASSERT(priv->kfa);

        /* Add this IPC process to the factory global list. */
        list_add(&priv->list, &factory_data->instances);

        return ipcp;

alloc_name:
        rkfree(priv);
alloc_data:
        rkfree(ipcp);
alloc_ipcp:
        LOG_ERR("%s: failed\n", __func__);

        return NULL;
}

/* Destroy a shim IPC process. */
static int
shim_hv_factory_ipcp_destroy(struct ipcp_factory_data *factory_data,
                             struct ipcp_instance *ipcp)
{
        struct ipcp_instance_data *cur, *next, *found;

        ASSERT(factory_data);
        ASSERT(ipcp);

        found = NULL;
        list_for_each_entry_safe(cur, next, &factory_data->instances, list) {
                if (cur == ipcp->data) {
                        list_del(&cur->list);
                        found = cur;
                        break;
                }
        }

        if (!found) {
                LOG_ERR("%s: entry not found", __func__);
        }

        name_destroy(ipcp->data->name);
        rkfree(ipcp->data);
        rkfree(ipcp);

        return 0;
}

static struct ipcp_factory_ops shim_hv_factory_ops = {
        .init      = shim_hv_factory_init,
        .fini      = shim_hv_factory_fini,
        .create    = shim_hv_factory_ipcp_create,
        .destroy   = shim_hv_factory_ipcp_destroy,
};

/* A factory for shim IPC processes for hypervisors. */
static struct ipcp_factory *shim_hv_ipcp_factory = NULL;

int
shim_hv_init(void)
{
        shim_hv_ipcp_factory = kipcm_ipcp_factory_register(
                        default_kipcm, SHIM_HV_NAME,
                        &shim_hv_factory_data,
                        &shim_hv_factory_ops);

        if (shim_hv_ipcp_factory == NULL)
                return -1;

        return 0;
}

void
shim_hv_fini(void)
{
        ASSERT(shim_hv_ipcp_factory);

        kipcm_ipcp_factory_unregister(default_kipcm, shim_hv_ipcp_factory);
}

