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
        struct name             name;
        int                     assigned;
        struct name             dif_name;
        struct kfa              *kfa;
        struct list_head        registered_applications;
};

/* Private data associated to shim IPC process factory. */
static struct ipcp_factory_data {
        struct list_head instances;
} shim_hv_factory_data;

struct name_list_element {
        struct list_head list;
        struct name application_name;
};

static int
shim_hv_application_register(struct ipcp_instance_data *priv,
                             const struct name *application_name)
{
        struct name_list_element *cur;
        char *tmpstr = name_tostring(application_name);
        int ret = -1;

        /* Is this application already registered? */
        list_for_each_entry(cur, &priv->registered_applications, list) {
                if (name_is_equal(application_name, &cur->application_name)) {
                        LOG_ERR("%s: Application %s already registered",
                                __func__, tmpstr);
                        goto out;
                }
        }

        /* Add the application to the list of registered applications. */
        cur = rkzalloc(sizeof(*cur), GFP_KERNEL);
        if (!cur) {
                LOG_ERR("%s: Allocating list element\n", __func__);
                goto out;
        }

        if (name_cpy(application_name, &cur->application_name)) {
                LOG_ERR("%s: name_cpy() failed\n", __func__);
                goto name_alloc;
        }

        list_add(&priv->registered_applications, &cur->list);

        ret = 0;
        LOG_INFO("%s: Application %s registered", __func__, tmpstr);

name_alloc:
        if (ret) {
                rkfree(cur);
        }
out:
        if (tmpstr)
                rkfree(tmpstr);

        return ret;
}

static int
shim_hv_application_unregister(struct ipcp_instance_data *priv,
                               const struct name *application_name)
{
        struct name_list_element *cur;
        struct name_list_element *found = NULL;
        char *tmpstr = name_tostring(application_name);

        /* Is this application registered? */
        list_for_each_entry(cur, &priv->registered_applications, list) {
                if (name_is_equal(application_name, &cur->application_name)) {
                        found = cur;
                        break;
                }
        }

        if (!found) {
                LOG_ERR("%s: Application %s not registered", __func__, tmpstr);
                goto out;
        }

        /* Remove the application from the list of registered applications. */
        name_fini(&found->application_name);
        list_del(&found->list);
        rkfree(found);

        LOG_INFO("%s: Application %s unregistered", __func__, tmpstr);

out:
        if (tmpstr)
                rkfree(tmpstr);

        return (found != NULL);
}

static int
shim_hv_assign_to_dif(struct ipcp_instance_data *priv,
                      const struct dif_info *dif_information)
{
        ASSERT(priv);
        ASSERT(dif_information);

        if (priv->assigned) {
                ASSERT(priv->dif_name.process_name);
                LOG_ERR("%s: IPC process already assigned to the DIF %s",
                        __func__,  priv->dif_name.process_name);
                return -1;
        }

        priv->assigned = 1;
        if (name_cpy(dif_information->dif_name, &priv->dif_name)) {
                LOG_ERR("%s: name_cpy() failed\n", __func__);
                return -1;
        }

        return 0;
}

static const struct name *
shim_hv_ipcp_name(struct ipcp_instance_data *priv)
{
        ASSERT(priv);
        ASSERT(name_is_ok(&priv->name));

        return &priv->name;
}

/* Interface exported by a shim IPC process. */
static struct ipcp_instance_ops shim_hv_ipcp_ops = {
        .flow_allocate_request     = NULL,
        .flow_allocate_response    = NULL,
        .flow_deallocate           = NULL,
        .flow_binding_ipcp         = NULL,
        .flow_destroy              = NULL,

        .application_register      = shim_hv_application_register,
        .application_unregister    = shim_hv_application_unregister,

        .assign_to_dif             = shim_hv_assign_to_dif,
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

        .ipcp_name                 = shim_hv_ipcp_name,
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
        priv->assigned = 0;

        if (name_cpy(name, &priv->name)) {
                LOG_ERR("%s: name_cpy() failed\n", __func__);
                goto alloc_name;
        }

        INIT_LIST_HEAD(&priv->registered_applications);

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

        name_fini(&ipcp->data->name);
        name_fini(&ipcp->data->dif_name);
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

