/*
 *  Dummy Shim IPC Process
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
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

#define SHIM_NAME   "shim-dummy"

#define RINA_PREFIX SHIM_NAME

#include "logs.h"
#include "common.h"
#include "utils.h"
#include "kipcm.h"
#include "shim.h"

struct dummy_instance {
        ipc_process_id_t ipc_process_id;
        struct name_t *  name;

        /* FIXME: Stores the state of flows indexed by port_id */
        struct list_head * flows;

        /* Used to keep a list of all the dummy shims */
        struct list_head list;
};

struct dummy_flow {
        port_id_t             port_id;
        const struct name_t * source;
        const struct name_t * dest;
        struct list_head      list;
};

static int dummy_flow_allocate_request(struct shim_instance_data * data,
                                       const struct name_t *       source,
                                       const struct name_t *       dest,
                                       const struct flow_spec_t *  flow_spec,
                                       port_id_t                   id)
{
        struct dummy_instance * dummy;
        struct dummy_flow *     flow;

        /* FIXME: We should verify that the port_id has not got a flow yet */

        flow = rkzalloc(sizeof(*flow), GFP_KERNEL);
        if (!flow)
                return -1;

        /* FIXME: Now we should ask the destination application for a flow */

        dummy = (struct dummy_instance *) data;

        flow->dest = dest;
        flow->source = source;
        flow->port_id = *id;

        INIT_LIST_HEAD(&flow->list);
        list_add(&flow->list, dummy->flows);

        return 0;
}

static int dummy_flow_allocate_response(struct shim_instance_data * data,
                                        port_id_t                   id,
                                        response_reason_t *         response)
{ return -1; }

static int dummy_flow_deallocate(struct shim_instance_data * data,
                                 port_id_t                   id)
{ return -1; }

static int dummy_application_register(struct shim_instance_data * data,
                                      const struct name_t *       name)
{ return -1; }

static int dummy_application_unregister(struct shim_instance_data * data,
                                        const struct name_t *       name)
{ return -1; }

static struct dummy_flow * find_flow(struct shim_instance_data * opaque,
                                     port_id_t                   id)
{
        struct dummy_instance * dummy;
        struct dummy_flow *     flow;

        dummy = (struct dummy_instance *) opaque;
        list_for_each_entry(flow, dummy->flows, list) {
                if (flow->port_id == id) {
                        return flow;
                }
        }

        return NULL;
}

static int dummy_sdu_write(struct shim_instance_data * data,
                           port_id_t                   id,
                           const struct sdu_t *        sdu)
{
        struct dummy_flow * flow;

        flow = find_flow(data, id);
        if (!flow) {
                LOG_ERR("There is no flow allocated for port-id %d", id);
                return -1;
        }

        /* FIXME: Add code here to send the sdu */

        return -1;
}

static int dummy_sdu_read(struct shim_instance_data * data,
                          port_id_t                   id,
                          struct sdu_t *              sdu)
{
        struct dummy_flow * flow;

        flow = find_flow(data, id);
        if (!flow) {
                LOG_ERR("There is not flow allocated for port-id %d", id);
                return -1;
        }

        return -1;
}

struct shim_data {
        struct list_head * shim_list;
};

static struct shim_data dummy_data;

static struct shim *    dummy_shim = NULL;

static int dummy_init(struct shim_data * data)
{
        struct list_head * dummy_shim_list;

        dummy_data.shim_list = rkmalloc(sizeof(*dummy_shim_list), GFP_KERNEL);
        if (!dummy_data.shim_list)
                return -1;

        dummy_data.shim_list->next = dummy_data.shim_list;
        dummy_data.shim_list->prev = dummy_data.shim_list;

        return -1;
}

static int dummy_fini(struct shim_data * data)
{
        struct dummy_instance * pos, * next;
        struct dummy_flow *     pos_flow, * next_flow;

        list_for_each_entry_safe(pos, next,
                                 (struct list_head *) dummy_shim->data,
                                 list) {
                list_del(&pos->list);
                list_for_each_entry_safe(pos_flow,
                                         next_flow, pos->flows, list) {
                        list_del(&pos_flow->list);
                        rkfree(pos_flow);
                }
                rkfree(pos);
        }

        return -1;
}

static struct shim_instance * dummy_create(struct shim_data * data,
                                           ipc_process_id_t   id)
{
        struct shim_instance *   instance;
        struct shim_instance_ops ops;
        struct dummy_instance *  dummy_inst;
        struct list_head *       port_flow;

        instance = rkzalloc(sizeof(*instance), GFP_KERNEL);
        if (!instance)
                return NULL;

        dummy_inst = rkzalloc(sizeof(*dummy_inst), GFP_KERNEL);
        if (!dummy_inst) {
                rkfree(instance);
                return NULL;
        }

        port_flow = rkzalloc(sizeof(*port_flow), GFP_KERNEL);
        if (!port_flow) {
                rkfree(instance);
                rkfree(dummy_inst);
                return NULL;
        }

        port_flow->prev = port_flow;
        port_flow->next = port_flow;

        dummy_inst->ipc_process_id = id;
        dummy_inst->flows          = port_flow;

        instance->data             = dummy_inst;

        ops.flow_allocate_request  = dummy_flow_allocate_request;
        ops.flow_allocate_response = dummy_flow_allocate_response;
        ops.flow_deallocate        = dummy_flow_deallocate;
        ops.application_register   = dummy_application_register;
        ops.application_unregister = dummy_application_unregister;
        ops.sdu_write              = dummy_sdu_write;
        ops.sdu_read               = dummy_sdu_read;

        instance->ops              = &ops;

        INIT_LIST_HEAD(&dummy_inst->list);
        list_add(&dummy_inst->list, (struct list_head *) dummy_shim->data);

        return instance;
}

static int dummy_destroy(struct shim_data *     data,
                         struct shim_instance * inst)
{ return -1; }

/* FIXME: It doesn't allow reconfiguration */
static struct shim_instance * dummy_configure(struct shim_data *         data,
                                              struct shim_instance *     inst,
                                              const struct shim_config * conf)
{
        struct shim_config *    current_entry;
        struct dummy_instance * dummy;

        ASSERT(inst);
        ASSERT(conf);

        dummy = (struct dummy_instance *) inst->data;
        if (!dummy) {
                LOG_ERR("There is not a dummy instance in this shim instance");
                return NULL;
        }

        list_for_each_entry(current_entry, &(conf->list), list) {
                if (strcmp(current_entry->entry->name, "name"))
                        dummy->name = (struct name_t *)
                                current_entry->entry->value->data;
                else {
                        LOG_ERR("Cannot identify parameter '%s'",
                                current_entry->entry->name);
                        return NULL;
                }
        }

        /* FIXME: NULL ??? */

        return NULL;
}

static struct shim_ops dummy_ops = {
        .init      = dummy_init,
        .fini      = dummy_fini,
        .create    = dummy_create,
        .destroy   = dummy_destroy,
        .configure = dummy_configure,
};

/* FIXME: To be removed ABSOLUTELY */
extern struct kipcm * default_kipcm;

static int __init mod_init(void)
{
        dummy_shim = kipcm_shim_register(default_kipcm,
                                         SHIM_NAME,
                                         &dummy_data,
                                         &dummy_ops);
        if (!dummy_shim) {
                LOG_CRIT("Initialization failed");
                return -1;
        }

        return 0;
}

static void __exit mod_exit(void)
{
        if (kipcm_shim_unregister(default_kipcm, dummy_shim)) {
                LOG_CRIT("Cannot unregister");
                return;
        }
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RINA Dummy Shim IPC");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
MODULE_AUTHOR("Miquel Tarzan <miquel.tarzan@i2cat.net>");
MODULE_AUTHOR("Sander Vrijders <sander.vrijders@intec.ugent.be>");
