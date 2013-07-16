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

/* FIXME: To be removed ABSOLUTELY */
extern struct kipcm * default_kipcm;

/* Holds all configuration related to a shim instance */
struct dummy_info {
	/* IPC Instance name */
	struct name * name;
	/* DIF name */
	struct name * dif_name;
};

struct shim_instance_data {
        ipc_process_id_t    id;

        /* FIXME: Stores the state of flows indexed by port_id */
        struct list_head    flows;

        /* Used to keep a list of all the dummy shims */
        struct list_head    list;

	struct dummy_info * info;
};

enum dummy_flow_state {
	NULL_STATE = 1,
	CONNECT_PENDING,
	AUTHENTICATED,
	ESTABLISHED,
	RELEASING,
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
        struct dummy_flow * flow;

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
        struct dummy_flow * flow;

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
        if (!flow->dest) {
        	rkfree(flow);
		LOG_ERR("Name copy failed");
		return -1;
	}
        flow->source = name_dup(source);
	if (!flow->source) {
		rkfree(flow->dest);
		rkfree(flow);
		LOG_ERR("Name copy failed");
		return -1;
	}
        /* FIXME: Now we should ask the destination application for a flow */

        flow->port_id = id;

        INIT_LIST_HEAD(&flow->list);
        list_add(&flow->list, &data->flows);

        if (kipcm_flow_add(default_kipcm, data->id, id))
        	return -1;

        return 0;
}

static int dummy_flow_allocate_response(struct shim_instance_data * data,
                                        port_id_t                   id,
                                        response_reason_t *         response)
{         
	ASSERT(data);
        ASSERT(response);

	/* On positive response, flow should transition to allocated state */
	if (response == 0) {
		
	}

	/* 
	 * NOTE:
	 *   Other shims may implement other behavior here,
	 *   such as contacting the apposite shim IPC process 
	 */

        return 0;
}

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

	/* FIXME: Notify the destination application, maybe? */

	list_del(&flow->list);
	name_destroy(flow->dest);
	name_destroy(flow->source);
	rkfree(flow);

	if (kipcm_flow_remove(default_kipcm, id))
		return -1;

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
                LOG_ERR("There is not a flow allocated for port-id %d", id);
                return -1;
        }

        return -1;
}

static int dummy_deallocate_all(struct shim_instance_data * data)
{
	struct dummy_flow *pos, *next;

	list_for_each_entry_safe(pos, next, &data->flows, list) {
		if (dummy_flow_deallocate(data, pos->port_id)) {
			/* FIXME: Maybe more should be done here in case
			 * the flow couldn't be destroyed
			 */
			LOG_ERR("Flow %d could not be removed", pos->port_id);
		}
	}
	return 0;
}

struct shim_data {
        struct list_head instances;
};

static struct shim_data dummy_data;
static struct shim *    dummy_shim = NULL;

static int dummy_init(struct shim_data * data)
{
	ASSERT(data);

	bzero(&dummy_data, sizeof(dummy_data));
	INIT_LIST_HEAD(&data->instances);

        return 0;
}

static int dummy_fini(struct shim_data * data)
{
        ASSERT(data);

        ASSERT(list_empty(&data->instances));

        return 0;
}

static struct shim_instance_ops dummy_instance_ops = {
	.flow_allocate_request  = dummy_flow_allocate_request,
	.flow_allocate_response = dummy_flow_allocate_response,
	.flow_deallocate        = dummy_flow_deallocate,
	.application_register   = dummy_application_register,
	.application_unregister = dummy_application_unregister,
	.sdu_write              = dummy_sdu_write,
	.sdu_read               = dummy_sdu_read,
};

static struct shim_instance_data * find_instance(struct shim_data * data,
                                                 ipc_process_id_t   id)
{

        struct shim_instance_data * pos;

        list_for_each_entry(pos, &(data->instances), list) {
                if (pos->id == id) {
                        return pos;
                }
        }

        return NULL;
}

static struct shim_instance * dummy_create(struct shim_data * data,
                                           ipc_process_id_t   id)
{
	struct shim_instance * inst;

        ASSERT(data);
	
        /* Check if there already is an instance with that id */
        if (find_instance(data,id)) {
                LOG_ERR("There's a shim instance with id %d already", id);
                return NULL;
        } 

        /* Create an instance */
	LOG_DBG("Create an instance");
        inst = rkzalloc(sizeof(*inst), GFP_KERNEL);
        if (!inst)
                return NULL;

        /* fill it properly */
	LOG_DBG("Fill it properly");
        inst->ops  = &dummy_instance_ops;
        inst->data = rkzalloc(sizeof(struct shim_instance_data), GFP_KERNEL);
        if (!inst->data) {
		LOG_DBG("Fill it properly failed");
                rkfree(inst);
                return NULL;
        }

        inst->data->id = id;
        INIT_LIST_HEAD(&inst->data->flows);
	inst->data->info = rkzalloc(sizeof(*inst->data->info), GFP_KERNEL);
	if (!inst->data->info) {
		LOG_DBG("Failed creation of inst->data->info");
		rkfree(inst->data);
		rkfree(inst);
		return NULL;
	}

        inst->data->info->dif_name = name_create();
	if (!inst->data->info->dif_name) {
		LOG_DBG("Failed creation of dif_name");
		rkfree(inst->data->info);
		rkfree(inst->data);
		rkfree(inst);
		return NULL;
	}

	inst->data->info->name = name_create();
	if (!inst->data->info->name) {
		LOG_DBG("Failed creation of ipc name");
		name_destroy(inst->data->info->dif_name);
		rkfree(inst->data->info);
		rkfree(inst->data);
		rkfree(inst);
		return NULL;
	}


        /*
         * Bind the shim-instance to the shims set, to keep all our data
         * structures linked (somewhat) together
         */
	
	LOG_DBG("Adding dummy instance to the list of shim dummy instances");

	INIT_LIST_HEAD(&(inst->data->list));
	LOG_DBG("Initialization of instance list: %pK", &inst->data->list);
        list_add(&(inst->data->list), &(data->instances));
	LOG_DBG("Inst %pK added to the dummy instances", inst);

        return inst;
}

/* FIXME: It doesn't allow reconfiguration */
static struct shim_instance * dummy_configure(struct shim_data *         data,
                                              struct shim_instance *     inst,
                                              const struct shim_config * conf)
{
	struct shim_instance_data * instance;
        struct shim_config *        tmp;

        ASSERT(data);
        ASSERT(inst);
        ASSERT(conf);

        instance = find_instance(data, inst->data->id);
        if (!instance) {
                LOG_ERR("There's no instance with id %d", inst->data->id);
                return inst;
        }

        /* Use configuration values on that instance */
        list_for_each_entry(tmp, &(conf->list), list) {
                if (!strcmp(tmp->entry->name, "dif-name") &&
                    tmp->entry->value->type == SHIM_CONFIG_STRING) {
                        if (name_cpy(instance->info->dif_name,
                                     (struct name *)
                                     tmp->entry->value->data)) {
                                LOG_ERR("Failed to copy DIF name");
                                return inst;
                        }
                }
		else if (!strcmp(tmp->entry->name, "name") &&
                         tmp->entry->value->type == SHIM_CONFIG_STRING) {
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

static int dummy_destroy(struct shim_data *     data,
                         struct shim_instance * instance)
{
	struct shim_instance_data * pos, * next;

        ASSERT(data);
        ASSERT(instance);

        /* Retrieve the instance */
        list_for_each_entry_safe(pos, next, &data->instances, list) {
                if (pos->id == instance->data->id) {
                        /* Unbind from the instances set */
                        list_del(&pos->list);
			dummy_deallocate_all(pos);
                        /* Destroy it */
                        name_destroy(pos->info->dif_name);
			name_destroy(pos->info->name);
			rkfree(pos->info);
                        rkfree(pos);
                        rkfree(instance);

                        return 0;
                }
        }

        return -1;
}

static struct shim_ops dummy_ops = {
        .init      = dummy_init,
        .fini      = dummy_fini,
        .create    = dummy_create,
        .destroy   = dummy_destroy,
        .configure = dummy_configure,
};

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
