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
#include "shim.h"

/* Holds all configuration related to a shim instance */
struct empty_info {
	struct name_t * dif_name;
};

/* This structure will contains per-instance data */
struct shim_instance_data {
	struct list_head    list;
	struct list_head *  flows;
        ipc_process_id_t    id;
	struct empty_info * info;
};

struct empty_flow {
	struct list_head      list;
	port_id_t 	      port_id;
	const struct name_t * source;
	const struct name_t * dest;
};

/*
 * NOTE:
 *   The functions that have to be exported for shim-instance. The caller
 *   ensures the parameters are not empty, therefore only assertions should be
 *   added
 */

static struct empty_flow *
find_flow(struct shim_instance_data * data, port_id_t id)
{
	struct empty_flow * cur;

	list_for_each_entry(cur, data->flows, list) {
		if (cur->port_id == id) {
			return cur;
		}
	}

	return NULL;
}

static int empty_flow_allocate_request(struct shim_instance_data * data,
                                       const struct name_t *       source,
                                       const struct name_t *       dest,
                                       const struct flow_spec_t *  flow_spec,
                                       port_id_t                   id)
{
	struct empty_flow * flow;

        ASSERT(data);
        ASSERT(source);
        ASSERT(dest);

        /* It must be ensured that this request has not been awarded before */
        if (find_flow(data, id)) {
        	LOG_ERR("This flow already exists");
        	return -1;
        }
        flow = kzalloc(sizeof(*flow), GFP_KERNEL);
	if (!flow) {
		LOG_ERR("Cannot allocate %zu bytes of memory", sizeof(*flow));
		return -1;
	}

	flow->dest = dest;
	flow->source = source;
	flow->port_id = *id;

	INIT_LIST_HEAD(&flow->list);
	list_add(&flow->list, data->flows);

        return -1;
}

static int empty_flow_allocate_response(struct shim_instance_data * data,
                                        port_id_t                   id,
                                        response_reason_t *         response)
{
        ASSERT(data);
        ASSERT(response);

        return -1;
}

static int empty_flow_deallocate(struct shim_instance_data * data,
                                 port_id_t                   id)
{
	struct empty_flow * flow;

        ASSERT(data);
        flow = find_flow(data, id);
        if (!flow) {
		LOG_ERR("Flow does not exist, cannot remove");

		return -1;
	}


        return -1;
}

static int empty_application_register(struct shim_instance_data * data,
                                      const struct name_t *       name)
{
        ASSERT(data);
        ASSERT(name);

        return -1;
}

static int empty_application_unregister(struct shim_instance_data * data,
                                        const struct name_t *       name)
{
        ASSERT(data);
        ASSERT(name);

        return -1;
}

static int empty_sdu_write(struct shim_instance_data * data,
                           port_id_t                   id,
                           const struct sdu_t *        sdu)
{
        ASSERT(data);
        ASSERT(sdu);

        return -1;
}

static int empty_sdu_read(struct shim_instance_data * data,
                          port_id_t                   id,
                          struct sdu_t *              sdu)
{
        ASSERT(data);
        ASSERT(sdu);

        return -1;
}

/*
 * The shim_instance ops are common to all the shim instances therefore
 * there's no real need to take a dynamically allocated buffer. Let's use a
 * static struct containing all the pointers and share it among all the
 * instances.
 */
static struct shim_instance_ops empty_instance_ops = {
        .flow_allocate_request  = empty_flow_allocate_request,
        .flow_allocate_response = empty_flow_allocate_response,
        .flow_deallocate        = empty_flow_deallocate,
        .application_register   = empty_application_register,
        .application_unregister = empty_application_unregister,
        .sdu_write              = empty_sdu_write,
        .sdu_read               = empty_sdu_read,
};

/*
 * This structure (static again, there's no need to dynamically allocate a
 * buffer) will hold all the shim data
 */
static struct shim_data {
        struct list_head * instances;
} empty_data;

/*
 * We maintain a shim pointer since the module is loaded once (and we need it
 * during module unloading)
*/
static struct shim * empty_shim = NULL;

/*
 * NOTE:
 *   The functions that have to be exported to the shim-layer.
 */

/* Initializes the shim_data structure */
static int empty_init(struct shim_data * data)
{
        ASSERT(data);

        bzero(&empty_data, sizeof(empty_data));
        INIT_LIST_HEAD(data->instances);

        return 0;
}

/*
 * Finalizes (destroys) the shim_data structure, releasing resources allocated
 * by empty_init
 */
static int empty_fini(struct shim_data * data)
{
        ASSERT(data);

        /*
         * NOTE:
         *   All the instances should be removed by the shims layer, no work is
         *   needed here. In theory, a check for empty list should be added
         *   here
         */

        /* FIXME: Add code here */

        return -1;
}

static struct shim_instance_data * find_instance(struct shim_data * data,
						 ipc_process_id_t   id)
{

	struct shim_instance_data * pos;

	list_for_each_entry(pos, data->instances, list) {
		if (pos->id == id) {
			return pos;
		}
	}

	return NULL;
}

static struct shim_instance * empty_create(struct shim_data * data,
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
        inst = rkzalloc(sizeof(*inst), GFP_KERNEL);
        if (!inst)
                return NULL;

        /* fill it properly */
        inst->ops  = &empty_instance_ops;
        inst->data = rkzalloc(sizeof(struct shim_instance_data), GFP_KERNEL);
        if (!inst->data) {
                kfree(inst);
                return NULL;
        }

        inst->data->id = id;

        /*
         * Bind the shim-instance to the shims set, to keep all our data
         * structures linked (somewhat) together
         */
        list_add(data->instances, &(inst->data->list));

        return inst;
}

/* FIXME: Might need to move this to a global file for all shims */
static int name_cpy(struct name_t ** dst, const struct name_t * src)
{
        *dst = rkzalloc(sizeof(**dst), GFP_KERNEL);
        if (!*dst)
                return -1;

        if(!strcpy((*dst)->process_name,     src->process_name)     ||
           !strcpy((*dst)->process_instance, src->process_instance) ||
           !strcpy((*dst)->entity_name,      src->entity_name)      ||
           !strcpy((*dst)->entity_instance,  src->entity_instance)) {
		LOG_ERR("Cannot perform strcpy");
                return -1;
	}

	return 0;
}

static struct shim_instance * empty_configure(struct shim_data *         data,
                                              struct shim_instance *     inst,
                                              const struct shim_config * cfg)
{
	struct shim_instance_data * instance;
	struct shim_config *        tmp;

        ASSERT(data);
        ASSERT(inst);
        ASSERT(cfg);

	instance = find_instance(data, inst->data->id);
	if (!instance) {
		LOG_ERR("There's no instance with id %d", inst->data->id);
		return inst;
	}

	/* Get configuration struct pertaining to this shim instance */ 
	if (!instance->info) {
		instance->info = rkzalloc(sizeof(*instance->info), GFP_KERNEL);
                if (!instance->info)
                        return NULL;
	}

        /* Use configuration values on that instance */
	list_for_each_entry(tmp, &(cfg->list), list) {
		if (!strcmp(tmp->entry->name, "dif-name") &&
                    tmp->entry->value->type == SHIM_CONFIG_STRING) {
                        if (name_cpy(&(instance->info->dif_name),
                                     (struct name_t *)
                                     tmp->entry->value->data)) {
				LOG_ERR("Failed to copy DIF name");
                                return inst;
                        }
		}
	}
	
        /*
         * Instance might change (reallocation), return the updated pointer
         * if needed. We don't re-allocate our instance so we'll be returning
         * the same pointer.
         */
	
        return inst;
}

static int empty_destroy(struct shim_data *     data,
                         struct shim_instance * instance)
{
	struct shim_instance_data * inst;
	struct list_head          * pos, * q;

        ASSERT(data);
        ASSERT(instance);

        /* Retrieve the instance */
	list_for_each_safe(pos, q, data->instances) {
		 inst = list_entry(pos, struct shim_instance_data, list);

		 if(inst->id == instance->data->id) {
			 /* Unbind from the instances set */
			 list_del(pos);
			 /* Destroy it */
			 kfree(inst->info->dif_name);
			 kfree(inst->info);
			 kfree(inst);
		 }
	}

        return 0;
}

static struct shim_ops empty_ops = {
        .init      = empty_init,
        .fini      = empty_fini,
        .create    = empty_create,
        .destroy   = empty_destroy,
        .configure = empty_configure,
};

/* FIXME: To be removed ABSOLUTELY */
extern struct kipcm * default_kipcm;

static int __init mod_init(void)
{
        /*
         * Pass data and ops to the upper layer.
         *
         * If the init function is not empty (our case, points to empty_fini)
         * it will get called and empty_data will be initialized.
         *
         * If the init function is not present (empty_ops.init == NULL)
         *
         */

        empty_shim = kipcm_shim_register(default_kipcm,
                                         SHIM_NAME,
                                         &empty_data,
                                         &empty_ops);
        if (!empty_shim) {
                LOG_CRIT("Initialization failed");
                return -1;
        }

        return 0;
}

static void __exit mod_exit(void)
{
        /*
         * Upon unregistration empty_ops.fini will be called (if present), that
         * function will be in charge to release all the resources allocated
         * during the shim lifetime.
         *
         * The upper layers will call empty_ops.create and empty_ops.destroy so
         * empty_ops.fini should release the resources allocated by
         * empty_ops.init
         */
        if (kipcm_shim_unregister(default_kipcm, empty_shim)) {
                LOG_CRIT("Cannot unregister");
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
