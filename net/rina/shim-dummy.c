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

#include <linux/slab.h>
#include <linux/list.h>

#define RINA_PREFIX "shim-dummy"

#include "logs.h"
#include "common.h"
#include "utils.h"
#include "shim.h"

static struct shim_t * shim;

struct dummy_instance_t {
        ipc_process_id_t ipc_process_id;
        struct name_t *  name;

	/* FIXME: Stores the state of flows indexed by port_id */
	struct list_head * flows;

	/* Used to keep a list of all the dummy shims */
	struct list_head list;
};

struct dummy_flow_t {
        port_id_t             port_id;
        const struct name_t * source;
        const struct name_t * dest;
        struct list_head      list;
};

static int dummy_flow_allocate_request(void *                     opaque,
                                       const struct name_t *      source,
                                       const struct name_t *      dest,
                                       const struct flow_spec_t * flow_spec,
                                       port_id_t *                id)
{
        struct dummy_instance_t * dummy;
        struct dummy_flow_t *     flow;

        LOG_FBEGN;

        /* FIXME: We should verify that the port_id has not got a flow yet */

        flow = kzalloc(sizeof(*flow), GFP_KERNEL);
        if (!flow) {
                LOG_ERR("Cannot allocate %zu bytes of memory", sizeof(*flow));
                LOG_FEXIT;
                return -1;
        }

	/* FIXME: Now we should ask the destination application for a flow */

	dummy = (struct dummy_instance_t *) opaque;
	flow->dest = dest;
	flow->source = source;
	flow->port_id = *id;

        INIT_LIST_HEAD(&flow->list);
        list_add(&flow->list, dummy->flows);

        LOG_FEXIT;

        return 0;
}

static int dummy_flow_allocate_response(void *              opaque,
                                        port_id_t           id,
                                        response_reason_t * response)
{
        LOG_FBEGN;
        LOG_FEXIT;

        return 0;
}

static int dummy_flow_deallocate(void *    opaque,
                                 port_id_t id)
{
        LOG_FBEGN;
        LOG_FEXIT;

        return 0;
}

static int dummy_application_register(void *                opaque,
                                      const struct name_t * name)
{
        LOG_FBEGN;
        LOG_FEXIT;

        return 0;
}

static int dummy_application_unregister(void *                opaque,
                                        const struct name_t * name)
{
        LOG_FBEGN;
        LOG_FEXIT;

        return 0;
}

static struct dummy_flow_t * dummy_find_flow(void * opaque, port_id_t id) {
	struct dummy_instance_t * dummy;
	struct dummy_flow_t *     flow;
	dummy = (struct dummy_instance_t *) opaque;
	list_for_each_entry(flow, dummy->flows, list) {
		if (flow->port_id == id) {
			return flow;
		}
	}
	return NULL;
}

static int dummy_sdu_write(void *               opaque,
                           port_id_t            id,
                           const struct sdu_t * sdu)
{
	struct dummy_flow_t * flow;
	LOG_FBEGN;

	flow = dummy_find_flow(opaque, id);
	if (!flow) {
		LOG_ERR("There is not a flow allocated with port-id %d", id);
		return -1;
	}

	/* FIXME: Add code here to send the sdu */

	LOG_FEXIT;

        return 0;
}

static int dummy_sdu_read(void *         opaque,
                          port_id_t      id,
                          struct sdu_t * sdu)
{
	struct dummy_flow_t * flow;

	LOG_FBEGN;

	flow = dummy_find_flow(opaque, id);
	if (!flow) {
		LOG_ERR("There is not a flow allocated with port-id %d", id);
		return -1;
	}

	LOG_FEXIT;

        return 0;
}

static struct shim_instance_t * dummy_create(void *           opaque,
                                             ipc_process_id_t ipc_process_id)
{
	struct shim_instance_t *  instance;
	struct dummy_instance_t * dummy_inst;
	struct list_head *        port_flow;

	LOG_FBEGN;

	instance = kzalloc(sizeof(*instance), GFP_KERNEL);
	if (!instance) {
		LOG_ERR("Cannot allocate %zu bytes of memory",
				sizeof(*instance));
		LOG_FEXIT;
		return NULL;
	}
	dummy_inst = kzalloc(sizeof(*dummy_inst), GFP_KERNEL);
	if (!dummy_inst) {
		LOG_ERR("Cannot allocate %zu bytes of memory",
				sizeof(*dummy_inst));
		kfree(instance);
		LOG_FEXIT;
		return NULL;
	}
	port_flow = kzalloc(sizeof(*port_flow), GFP_KERNEL);
	if (!port_flow) {
		LOG_ERR("Cannot allocate %zu bytes of memory",
				sizeof(*port_flow));
		kfree(instance);
		kfree(dummy_inst);
		LOG_FEXIT;
		return NULL;
	}
	port_flow->prev = port_flow;
	port_flow->next = port_flow;
	dummy_inst->ipc_process_id = ipc_process_id;
	dummy_inst->flows          = port_flow;

	instance->opaque                 = dummy_inst;
	instance->flow_allocate_request  = dummy_flow_allocate_request;
	instance->flow_allocate_response = dummy_flow_allocate_response;
	instance->flow_deallocate        = dummy_flow_deallocate;
	instance->application_register   = dummy_application_register;
	instance->application_unregister = dummy_application_unregister;
	instance->sdu_write              = dummy_sdu_write;
	instance->sdu_read               = dummy_sdu_read;

	INIT_LIST_HEAD(&dummy_inst->list);
	list_add(&dummy_inst->list, (struct list_head *) shim->opaque);

        LOG_FEXIT;

        return instance;
}

static int dummy_destroy(void *                   opaque,
                         struct shim_instance_t * inst)
{
        LOG_FBEGN;
        LOG_FEXIT;

        return 0;
}

static struct shim_instance_t *
dummy_configure(void *                     opaque,
                struct shim_instance_t *   instance,
                const struct shim_conf_t * configuration)
{
        struct shim_conf_t * current_entry;
        struct dummy_instance_t * dummy;

        LOG_FBEGN;

        ASSERT(instance);
        ASSERT(configuration);
        dummy = (struct dummy_instance_t *) instance->opaque;
        if (!dummy) {
                LOG_ERR("There is not a dummy instance in this shim instance");
                LOG_FEXIT;

                return instance;
        }
        list_for_each_entry(current_entry, &(configuration->list), list) {
                if (strcmp(current_entry->entry->name, "name"))
                        dummy->name = (struct name_t *)
                                current_entry->entry->value->data;
                else {
                        LOG_ERR("Cannot identify this parameter");
                        LOG_FEXIT;

                        return instance;
                }
        }

        LOG_FEXIT;

        return NULL;
}

static int __init mod_init(void)
{
	struct list_head * dummy_shim_list;
	LOG_FBEGN;

        shim = kmalloc(sizeof(*shim), GFP_KERNEL);
        if (!shim) {
                LOG_ERR("Cannot allocate %zu bytes of memory", sizeof(*shim));
                LOG_FEXIT;
                return -1;
        }
        dummy_shim_list = kmalloc(sizeof(*dummy_shim_list), GFP_KERNEL);
	if (!dummy_shim_list) {
		LOG_CRIT("Cannot allocate %zu bytes of memory",
			 sizeof(*dummy_shim_list));

		kfree(shim);
		LOG_FEXIT;
		return -1;
	}
	dummy_shim_list->next = dummy_shim_list;
	dummy_shim_list->prev = dummy_shim_list;
        shim->label     = "shim-dummy";
        shim->create    = dummy_create;
        shim->destroy   = dummy_destroy;
        shim->configure = dummy_configure;
        shim->opaque    = dummy_shim_list;

        if (shim_register(shim)) {
                LOG_ERR("Initialization of module shim-dummy failed");
                kfree(shim);

                LOG_FEXIT;
                return -1;
        }

        LOG_FEXIT;

        return 0;
}

static void __exit mod_exit(void)
{
        struct dummy_instance_t *pos, *next;
        struct dummy_flow_t     *pos_flow, *next_flow;

	LOG_FBEGN;

	ASSERT(shim);
	ASSERT(shim->opaque);
	if (shim_unregister(shim)) {
        	/* FIXME: Should we do something here? */
        }
        list_for_each_entry_safe(pos, next, (struct list_head *) shim->opaque,
        		list) {
        	list_del(&pos->list);
        	list_for_each_entry_safe(pos_flow,
        				next_flow, pos->flows, list) {
        		list_del(&pos_flow->list);
        		kfree(pos_flow);
        	}
        	kfree(pos);
        }
        kfree(shim->opaque);
        kfree(shim);

        /* FIXME: We must unroll all the things done in mod_init */

        LOG_FEXIT;
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RINA Dummy Shim IPC");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
MODULE_AUTHOR("Miquel Tarzan <miquel.tarzan@i2cat.net>");
MODULE_AUTHOR("Sander Vrijders <sander.vrijders@intec.ugent.be>");
