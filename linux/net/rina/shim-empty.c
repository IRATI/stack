/*
 *  Empty Shim IPC Process (Shim template that should be used as a reference)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
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
#include <linux/list.h>
#include <linux/string.h>

#define SHIM_NAME   "shim-empty"

#define RINA_PREFIX SHIM_NAME

#include "logs.h"
#include "common.h"
#include "utils.h"
#include "kipcm.h"
#include "debug.h"
#include "ipcp-utils.h"
#include "ipcp-factories.h"

/* FIXME: To be removed ABSOLUTELY */
extern struct kipcm * default_kipcm;

/* Holds all configuration related to a shim instance */
struct empty_info {
        /* IPC Instance name */
        struct name * name;
        /* DIF name */
        struct name * dif_name;
};

/* This structure will contains per-instance data */
struct ipcp_instance_data {
        struct list_head    list;
        struct list_head    flows;
        ipc_process_id_t    id;
        struct empty_info * info;
};

/*
 * NOTE:
 *   The functions that have to be exported for shim-instance. The caller
 *   ensures the parameters are not empty, therefore only assertions should be
 *   added
 */

static int empty_flow_allocate_request(struct ipcp_instance_data * data,
                                       const struct name *         source,
                                       const struct name *         dest,
                                       const struct flow_spec *    fspec,
                                       port_id_t                   id,
                                       uint_t 			   seq_num,
                                       ipc_process_id_t		   dst_id)
{
        ASSERT(data);
        ASSERT(source);
        ASSERT(dest);
        ASSERT(fspec);
        return -1;
}

static int empty_flow_allocate_response(struct ipcp_instance_data * data,
                                        port_id_t                   id,
                                        response_reason_t *         response)
{
        ASSERT(data);
        ASSERT(response);

        return -1;
}

static int empty_flow_deallocate(struct ipcp_instance_data * data,
                                 port_id_t                   id)
{
        ASSERT(data);

        return -1;
}

static int empty_application_register(struct ipcp_instance_data * data,
                                      const struct name *         name)
{
        ASSERT(data);
        ASSERT(name);

        return -1;
}

static int empty_application_unregister(struct ipcp_instance_data * data,
                                        const struct name *         name)
{
        ASSERT(data);
        ASSERT(name);

        return -1;
}

static int empty_sdu_write(struct ipcp_instance_data * data,
                           port_id_t                   id,
                           struct sdu *                sdu)
{
        ASSERT(data);
        ASSERT(sdu);

        return -1;
}

static int empty_assign_to_dif(struct ipcp_instance_data * data,
                               const struct name *         dif_name)
{
        ASSERT(data);
        ASSERT(dif_name);

        return -1;
}

/*
 * The shim_instance ops are common to all the shim instances therefore
 * there's no real need to take a dynamically allocated buffer. Let's use a
 * static struct containing all the pointers and share it among all the
 * instances.
 */
static struct ipcp_instance_ops empty_instance_ops = {
        .flow_allocate_request  = empty_flow_allocate_request,
        .flow_allocate_response = empty_flow_allocate_response,
        .flow_deallocate        = empty_flow_deallocate,
        .application_register   = empty_application_register,
        .application_unregister = empty_application_unregister,
        .sdu_write              = empty_sdu_write,
        .assign_to_dif          = empty_assign_to_dif,
};

/*
 * This structure (static again, there's no need to dynamically allocate a
 * buffer) will hold all the shim data
 */
static struct ipcp_factory_data {
        struct list_head instances;
} empty_data;

static int empty_init(struct ipcp_factory_data * data)
{
        ASSERT(data);

        bzero(&empty_data, sizeof(empty_data));
        INIT_LIST_HEAD(&(data->instances));

        return 0;
}

static int empty_fini(struct ipcp_factory_data * data)
{

        ASSERT(data);

        /*
         * NOTE:
         *   All the instances will be removed by the shims layer, no work
         *   should be needed here (since the caller should have destroyed all
         *   the instances before)
         *
         *   This function has to unroll things created during init() or things
         *   that cannot be unrolled by empty_destroy.
         *
         *   In theory, a check for an empty list should be added here
         *
         *     Francesco
         */

        ASSERT(list_empty(&(data->instances)));

        return 0;
}

static struct ipcp_instance_data *
find_instance(struct ipcp_factory_data * data,
              ipc_process_id_t           id)
{

        struct ipcp_instance_data * pos;

        list_for_each_entry(pos, &(data->instances), list) {
                if (pos->id == id) {
                        return pos;
                }
        }

        return NULL;
}

static struct ipcp_instance * empty_create(struct ipcp_factory_data * data,
                                           ipc_process_id_t           id)
{
        struct ipcp_instance * inst;

        ASSERT(data);

        /* Check if there already is an instance with that id */
        if (find_instance(data,id)) {
                LOG_ERR("There's a shim instance with id %d already", id);
                return NULL;
        }

        /* Create an instance */
        inst = rkzalloc(sizeof(*inst), GFP_KERNEL);
        if (!inst)
                return NULL;

        /* fill it properly */
        inst->ops  = &empty_instance_ops;
        inst->data = rkzalloc(sizeof(struct ipcp_instance_data), GFP_KERNEL);
        if (!inst->data) {
                rkfree(inst);
                return NULL;
        }

        inst->data->id = id;
        inst->data->info = rkzalloc(sizeof(*inst->data->info), GFP_KERNEL);
        if (!inst->data->info) {
                rkfree(inst->data);
                rkfree(inst);
                return NULL;
        }

        inst->data->info->dif_name = name_create();
        if (!inst->data->info->dif_name) {
                rkfree(inst->data->info);
                rkfree(inst->data);
                rkfree(inst);
                return NULL;
        }

        inst->data->info->name = name_create();
        if (!inst->data->info->name) {
                rkfree(inst->data->info->dif_name);
                rkfree(inst->data->info);
                rkfree(inst->data);
                rkfree(inst);
                return NULL;
        }

        /*
         * Bind the shim-instance to the shims set, to keep all our data
         * structures linked (somewhat) together
         */
        INIT_LIST_HEAD(&(inst->data->list));
        list_add(&(data->instances), &(inst->data->list));

        return inst;
}

static struct ipcp_instance *
empty_configure(struct ipcp_factory_data * data,
                struct ipcp_instance *     inst,
                const struct ipcp_config * cfg)
{
        struct ipcp_instance_data * instance;
        struct ipcp_config *        tmp;

        ASSERT(data);
        ASSERT(inst);
        ASSERT(cfg);

        instance = find_instance(data, inst->data->id);
        if (!instance) {
                LOG_ERR("There's no instance with id %d", inst->data->id);
                return inst;
        }

        /* Use configuration values on that instance */
        list_for_each_entry(tmp, &(cfg->list), list) {
                if (!strcmp(tmp->entry->name, "dif-name") &&
                    tmp->entry->value->type == IPCP_CONFIG_STRING) {
                        if (name_cpy(instance->info->dif_name,
                                     (struct name *)
                                     tmp->entry->value->data)) {
                                LOG_ERR("Failed to copy DIF name");
                                return inst;
                        }
                }
                else if (!strcmp(tmp->entry->name, "name") &&
                         tmp->entry->value->type == IPCP_CONFIG_STRING) {
                        if (name_cpy(instance->info->name,
                                     (struct name *)
                                     tmp->entry->value->data)) {
                                LOG_ERR("Failed to copy name");
                                return inst;
                        }
                }
                else {
                        LOG_ERR("Cannot identify parameter '%s'",
                                tmp->entry->name);
                        return NULL;
                }
        }

        /*
         * Instance might change (reallocation), return the updated pointer
         * if needed. We don't re-allocate our instance so we'll be returning
         * the same pointer.
         */

        return inst;
}

static int empty_destroy(struct ipcp_factory_data * data,
                         struct ipcp_instance *     instance)
{
        struct ipcp_instance_data * inst;
        struct list_head          * pos, * q;

        ASSERT(data);
        ASSERT(instance);

        /* Retrieve the instance */
        list_for_each_safe(pos, q, &(data->instances)) {
                inst = list_entry(pos, struct ipcp_instance_data, list);

                if (inst->id == instance->data->id) {
                        /* Unbind from the instances set */
                        list_del(pos);

                        /* Destroy it */
                        name_destroy(inst->info->dif_name);
                        name_destroy(inst->info->name);
                        rkfree(inst->info);
                        rkfree(inst);
                }
        }

        return 0;
}

static struct ipcp_factory_ops empty_ops = {
        .init      = empty_init,
        .fini      = empty_fini,
        .create    = empty_create,
        .destroy   = empty_destroy,
        .configure = empty_configure,
};

struct ipcp_factory * shim;

static int __init mod_init(void)
{
        shim = kipcm_ipcp_factory_register(default_kipcm,
                                           SHIM_NAME,
                                           &empty_data,
                                           &empty_ops);
        if (!shim) {
                LOG_CRIT("Cannot register %s factory", SHIM_NAME);
                return -1;
        }

        return 0;
}

static void __exit mod_exit(void)
{
        ASSERT(shim);

        /*
         * Upon unregistration empty_ops.fini will be called (if present).
         * That function will be in charge to release all the resources
         * allocated during the shim lifetime.
         *
         * The upper layers will be calling .create. The .destroy must release
         * all the resources allocated by .create. The .fini must release the
         * resources allocated by .init and any resources that cannot be
         * released by .destroy, if any
         */
        if (kipcm_ipcp_factory_unregister(default_kipcm, shim)) {
                LOG_CRIT("Cannot unregister %s factory", SHIM_NAME);
                return;
        }
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RINA Empty Shim IPC");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
MODULE_AUTHOR("Miquel Tarzan <miquel.tarzan@i2cat.net>");
MODULE_AUTHOR("Sander Vrijders <sander.vrijders@intec.ugent.be>");
