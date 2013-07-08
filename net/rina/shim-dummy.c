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
#include "debug.h"
#include "utils.h"
#include "kipcm.h"
#include "shim.h"
#include "shim-utils.h"

struct shim_instance_data {
        ipc_process_id_t ipc_process_id;
        struct name *    name;

        /* FIXME: Stores the state of flows indexed by port_id */
        struct list_head flows;

        /* Used to keep a list of all the dummy shims */
        struct list_head list;
};

struct dummy_flow {
        port_id_t        port_id;
        struct name *    source;
        struct name *    dest;
        struct list_head list;
};

static struct dummy_flow * find_flow(struct shim_instance_data * data,
                                     port_id_t                   id)
{
        struct dummy_flow *     flow;

        list_for_each_entry(flow, &data->flows, list) {
                if (flow->port_id == id) {
                        return flow;
                }
        }

        return NULL;
}


static int dummy_flow_allocate_request(struct shim_instance_data * data,
                                       const struct name *         source,
                                       const struct name *         dest,
                                       const struct flow_spec *    fspec,
                                       port_id_t                   id)
{
        struct dummy_flow *         flow;

        ASSERT(data);
        ASSERT(source);
        ASSERT(dest);

        if (find_flow(data, id)) {
        	LOG_ERR("Flow already exists");
        	return -1;
        }

        flow = rkzalloc(sizeof(*flow), GFP_KERNEL);
        if (!flow)
                return -1;

        flow->dest = name_dup(dest);
        if(!flow->dest) {
        	rkfree(flow);
		LOG_ERR("Name copy failed");
		return -1;
	}
        flow->source = name_dup(source);
	if(!flow->source) {
		rkfree(flow->dest);
		rkfree(flow);
		LOG_ERR("Name copy failed");
		return -1;
	}
        /* FIXME: Now we should ask the destination application for a flow */

        flow->port_id = id;

        INIT_LIST_HEAD(&flow->list);
        list_add(&flow->list, &data->flows);

        return 0;
}

static int dummy_flow_allocate_response(struct shim_instance_data * data,
                                        port_id_t                   id,
                                        response_reason_t *         response)
{ return -1; }

static int dummy_flow_deallocate(struct shim_instance_data * data,
                                 port_id_t                   id)
{
	struct dummy_flow * flow;

	ASSERT(data);
	flow = find_flow(data, id);
	if (!flow) {
		LOG_ERR("Flow does not exist, cannot remove");
		return -1;
	}

	list_del(&flow->list);
	name_fini(flow->dest);
	name_fini(flow->source);
	rkfree(flow);

	return 0;
}

static int dummy_application_register(struct shim_instance_data * data,
                                      const struct name *         source)
{ return -1; }

static int dummy_application_unregister(struct shim_instance_data * data,
                                        const struct name *         source)
{ return -1; }

static int dummy_sdu_write(struct shim_instance_data * data,
                           port_id_t                   id,
                           const struct sdu *          sdu)
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
                          struct sdu *                sdu)
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
        struct list_head shim_list;
};

static struct shim_data dummy_data;

static struct shim *    dummy_shim = NULL;

static int dummy_init(struct shim_data * data)
{
	ASSERT(data);

	bzero(&dummy_data, sizeof(dummy_data));
	INIT_LIST_HEAD(&data->shim_list);

        return 0;
}

static int dummy_fini(struct shim_data * data)
{
        ASSERT(data);

        ASSERT(list_empty(&data->shim_list));

        return 0;
}

static struct shim_instance_ops instance_ops = {
	.flow_allocate_request  = dummy_flow_allocate_request,
	.flow_allocate_response = dummy_flow_allocate_response,
	.flow_deallocate        = dummy_flow_deallocate,
	.application_register   = dummy_application_register,
	.application_unregister = dummy_application_unregister,
	.sdu_write              = dummy_sdu_write,
	.sdu_read               = dummy_sdu_read,
};

static struct shim_instance * dummy_create(struct shim_data * data,
                                           ipc_process_id_t   id)
{
        struct shim_instance *   inst;

        inst = rkzalloc(sizeof(*inst), GFP_KERNEL);
        if (!inst)
                return NULL;

        inst->data = rkzalloc(sizeof(struct shim_instance_data), GFP_KERNEL);
        if (!inst->data) {
		rkfree(inst);
		return NULL;
	}

        INIT_LIST_HEAD(&inst->data->flows);

        inst->data->ipc_process_id = id;

        inst->ops = &instance_ops;

        INIT_LIST_HEAD(&inst->data->list);
        list_add(&inst->data->list, &data->shim_list);

        return inst;
}

static int dummy_destroy(struct shim_data *     data,
                         struct shim_instance * inst)
{
	struct shim_instance_data * pos, * next;

	ASSERT(data);
	ASSERT(inst);

	list_for_each_entry_safe(pos, next, &data->shim_list, list) {
		if (inst->data->ipc_process_id == pos->ipc_process_id) {
			list_del(&pos->list);
			rkfree(pos);
		}
	}

	return 0;
}

/* FIXME: It doesn't allow reconfiguration */
static struct shim_instance * dummy_configure(struct shim_data *         data,
                                              struct shim_instance *     inst,
                                              const struct shim_config * conf)
{
        struct shim_config *        current_entry;
        struct shim_instance_data * dummy;

        ASSERT(inst);
        ASSERT(conf);

        dummy = inst->data;
        if (!dummy) {
                LOG_ERR("There is not a dummy instance in this shim instance");
                return NULL;
        }

        list_for_each_entry(current_entry, &(conf->list), list) {
                if (strcmp(current_entry->entry->name, "name"))
                        dummy->name = (struct name *)
                                current_entry->entry->value->data;
                else {
                        LOG_ERR("Cannot identify parameter '%s'",
                                current_entry->entry->name);
                        return NULL;
                }
        }

        return inst;
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
