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
#include "ipcp-utils.h"
#include "ipcp-factories.h"
#include "du.h"
#include "kfa.h"
#include "rnl-utils.h"

/* FIXME: To be removed ABSOLUTELY */
extern struct kipcm * default_kipcm;

/* Holds all configuration related to a shim instance */
struct dummy_info {
        /* IPC Instance name */
        struct name * name;
        /* DIF name */
        struct name * dif_name;
};

struct ipcp_instance_data {
        ipc_process_id_t    id;

        /* FIXME: Stores the state of flows indexed by port_id */
        struct list_head    flows;

        /* Used to keep a list of all the dummy shims */
        struct list_head    list;

        struct dummy_info * info;

        struct list_head    apps_registered;

        /* FIXME: Remove it as soon as the kipcm_kfa gets removed*/
        struct kfa *        kfa;
};

enum dummy_flow_state {
        PORT_STATE_NULL = 1,
        PORT_STATE_RECIPIENT_ALLOCATE_PENDING,
        PORT_STATE_INITIATOR_ALLOCATE_PENDING,
        PORT_STATE_ALLOCATED
};

struct dummy_flow {
        port_id_t             port_id;
        port_id_t             dst_port_id;
        struct name *         source;
        struct name *         dest;
        struct list_head      list;
        enum dummy_flow_state state;
        flow_id_t             dst_fid;
        flow_id_t             src_fid; /* Required to notify back to the */
        ipc_process_id_t      dst_id;  /* IPC Manager the result of the */
        struct flow_spec *    fspec;   /* allocation */
};

struct app_register {
        struct name *    app_name;
        struct list_head list;
};

static int is_app_registered(struct ipcp_instance_data * data,
                             const struct name *         name)
{
        struct app_register * app;

        list_for_each_entry(app, &data->apps_registered, list) {
                if (!name_cmp(app->app_name, name)) {
                        return 1;
                }
        }
        return 0;
}

static struct app_register * find_app(struct ipcp_instance_data * data,
                                      const struct name *         name)
{
        struct app_register * app;

        list_for_each_entry(app, &data->apps_registered, list) {
                if (!name_cmp(app->app_name, name)) {
                        return app;
                }
        }

        return NULL;
}

static struct dummy_flow * find_flow(struct ipcp_instance_data * data,
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

static int dummy_flow_allocate_request(struct ipcp_instance_data * data,
                                       const struct name *         source,
                                       const struct name *         dest,
                                       const struct flow_spec *    fspec,
                                       port_id_t                   id,
                                       flow_id_t                   fid)
{
        struct dummy_flow * flow;

        ASSERT(data);
        ASSERT(source);
        ASSERT(dest);

        if (!data->info) {
                LOG_ERR("There is not info in this IPC Process");
                return -1;
        }

        if (!data->info->dif_name) {
                LOG_ERR("This IPC Process doesn't belong to a DIF");
                return -1;
        }

        if (!is_app_registered(data, dest)) {
                char * tmp = name_tostring(dest);
                LOG_ERR("Application %s is not registered in IPC process %d",
                        tmp, data->id);
                return -1;
        }

        if (find_flow(data, id)) {
                LOG_ERR("A flow already exists on port %d", id);
                return -1;
        }

        flow = rkzalloc(sizeof(*flow), GFP_KERNEL);
        if (!flow)
                return -1;

        flow->dest    = name_dup(dest);
        if (!flow->dest) {
                rkfree(flow);
                return -1;
        }
        flow->source = name_dup(source);
        if (!flow->source) {
                name_destroy(flow->dest);
                rkfree(flow);
                return -1;
        }

        /* Only for readability reasons */
        ASSERT(flow->dest);
        ASSERT(flow->source);

        flow->state   = PORT_STATE_INITIATOR_ALLOCATE_PENDING;
        flow->port_id = id;
        flow->src_fid = fid;
        flow->fspec   = flow_spec_dup(fspec);
        flow->dst_fid = kfa_flow_create(data->kfa);
        ASSERT(is_flow_id_ok(flow->dst_fid));

        INIT_LIST_HEAD(&flow->list);
        list_add(&flow->list, &data->flows);

        if (kipcm_flow_arrived(default_kipcm,
                               data->id,
                               flow->dst_fid,
                               data->info->dif_name,
                               flow->source,
                               flow->dest,
                               flow->fspec)) {
                return -1;
        }

        return 0;
}

static struct dummy_flow *
find_flow_by_fid(struct ipcp_instance_data * data,
                 uint_t                      fid)
{
        struct dummy_flow * flow;

        list_for_each_entry(flow, &data->flows, list) {
                if (flow->dst_fid == fid) {
                        return flow;
                }
        }

        return NULL;
}

static int dummy_flow_allocate_response(struct ipcp_instance_data * data,
                                        flow_id_t                   flow_id,
                                        port_id_t                   port_id,
                                        int                         result)
{
        struct dummy_flow * flow;

        ASSERT(data);

        if (!data->info) {
                LOG_ERR("There is not info in this IPC Process");
                return -1;
        }

        if (!data->info->dif_name) {
                LOG_ERR("This IPC Process doesn't belong to a DIF");
                return -1;
        }

        flow = find_flow_by_fid(data, flow_id);
        if (!flow) {
                LOG_ERR("Flow does not exist, cannot allocate");
                return -1;
        }

        if (flow->state != PORT_STATE_INITIATOR_ALLOCATE_PENDING) {
                LOG_ERR("Wrong flow state");
                return -1;
        }

        /* On positive response, flow should transition to allocated state */
        if (result == 0) {
                flow->dst_port_id = port_id;
                flow->state = PORT_STATE_ALLOCATED;
                if (kipcm_flow_add(default_kipcm,
                                   data->id, flow->port_id, flow->src_fid)) {
                        list_del(&flow->list);
                        name_destroy(flow->source);
                        name_destroy(flow->dest);
                        rkfree(flow);
                        return -1;
                }
                if (kipcm_flow_add(default_kipcm,
                                   data->id, port_id, flow_id)) {
                        /* FIXME: change this with kfa_flow_destroy
                        kipcm_flow_remove(default_kipcm, port_id); */
                        kfa_flow_unbind_and_destroy(data->kfa, port_id);
                        list_del(&flow->list);
                        name_destroy(flow->source);
                        name_destroy(flow->dest);
                        rkfree(flow);
                        return -1;
                }
                if (kipcm_flow_res(default_kipcm, data->id, flow->src_fid, 0)) {
                        /* FIXME: change this with kfa_flow_destroy
                        kipcm_flow_remove(default_kipcm, flow->port_id);
                        kipcm_flow_remove(default_kipcm, port_id); */
                        kfa_flow_unbind_and_destroy(data->kfa, flow->port_id);

                        kfa_flow_unbind_and_destroy(data->kfa, port_id);
                        list_del(&flow->list);
                        name_destroy(flow->source);
                        name_destroy(flow->dest);
                        rkfree(flow);
                        return -1;
                }
        } else {
                list_del(&flow->list);
                name_destroy(flow->source);
                name_destroy(flow->dest);
                rkfree(flow);
                return -1;
        }



        /*
         * NOTE:
         *   Other shims may implement other behavior here,
         *   such as contacting the apposite shim IPC process
         */

        return 0;
}

static int dummy_flow_deallocate(struct ipcp_instance_data * data,
                                 port_id_t                   id)
{
        struct dummy_flow * flow;
        port_id_t dest_port_id;

        ASSERT(data);
        flow = find_flow(data, id);
        if (!flow) {
                LOG_ERR("Flow does not exist, cannot remove");
                return -1;
        }

        if (id == flow->port_id)
                dest_port_id = flow->dst_port_id;
        else
                dest_port_id = flow->port_id;

        /* FIXME: dummy_flow is not updated after unbinding cause it is going
         * to be deleted. Is it really needed to unbind+destroy?
         */

	if (kfa_flow_unbind_and_destroy(data->kfa, id) ||
	    kfa_flow_unbind_and_destroy(data->kfa, id)) {
		return -1;
	}
	/*  
	rm_fid = kfa_flow_unbind(data->kfa, id);
        if (!is_flow_id_ok(rm_fid)){
                LOG_ERR("Could not unbind flow at port %d", id);
                return -1;
        }
        rm_dst_fid = kfa_flow_unbind(data->kfa, dest_port_id);
        if (!is_flow_id_ok(rm_dst_fid)){
                LOG_ERR("Could not unbind flow at port %d", dest_port_id);
                return -1;
        }

        if (kfa_flow_destroy(data->kfa, rm_fid)) {
                LOG_ERR("Could not destroy flow with fid: %d", rm_fid);
                return -1;
        }

        if (kfa_flow_destroy(data->kfa, rm_dst_fid)) {
                LOG_ERR("Could not destroy flow with fid: %d", rm_dst_fid);
                return -1;
        }
	*/

        /* Notify the destination application */
        kipcm_notify_flow_dealloc(data->id, 0, dest_port_id, 1);

        list_del(&flow->list);
        name_destroy(flow->dest);
        name_destroy(flow->source);
        rkfree(flow);

        return 0;
}

static int dummy_application_register(struct ipcp_instance_data * data,
                                      const struct name *         source)
{
        struct app_register * app_reg;
        char * tmp;

        ASSERT(source);

        if (!data->info) {
                LOG_ERR("There is not info in this IPC Process");
                return -1;
        }
        if (!data->info->dif_name) {
                LOG_ERR("IPC Process doesn't belong to any DIF");
                return -1;
        }

        if (is_app_registered(data, source)) {
                char * tmp = name_tostring(source);

                LOG_ERR("Application %s has been already registered", tmp);

                rkfree(tmp);

                return -1;
        }

        app_reg = rkzalloc(sizeof(*app_reg), GFP_KERNEL);
        if (!app_reg)
                return -1;

        app_reg->app_name = rkzalloc(sizeof(struct name), GFP_KERNEL);
        if (!app_reg->app_name) {
                rkfree(app_reg);

                return -1;
        }
        if (name_cpy(source, app_reg->app_name)) {
                char * tmp = name_tostring(source);

                name_destroy(app_reg->app_name);
                rkfree(app_reg);

                LOG_ERR("Application %s registration has failed", tmp);

                rkfree(tmp);

                return -1;
        }

        INIT_LIST_HEAD(&app_reg->list);
        list_add(&app_reg->list, &data->apps_registered);
        tmp = name_tostring(source);
        LOG_DBG("Application %s registered successfully", tmp);
        rkfree(tmp);

        return 0;
}

static int dummy_application_unregister(struct ipcp_instance_data * data,
                                        const struct name *         source)
{
        struct app_register * app;

        ASSERT(source);

        if (!is_app_registered(data, source)) {
                char * tmp = name_tostring(source);

                LOG_ERR("Application %s is not registered", tmp);

                rkfree(tmp);

                return -1;
        }
        app = find_app(data, source);
        list_del(&app->list);
        name_destroy(app->app_name);
        rkfree(app);

        return 0;
}

static int dummy_unregister_all(struct ipcp_instance_data * data)
{
        struct app_register * cur, *next;

        list_for_each_entry_safe(cur, next, &data->apps_registered, list) {
                list_del(&cur->list);
                name_destroy(cur->app_name);
                rkfree(cur);
        }

        return 0;
}

static int dummy_sdu_write(struct ipcp_instance_data * data,
                           port_id_t                   id,
                           struct sdu *                sdu)
{
        struct dummy_flow * flow;

        LOG_DBG("Dummy SDU write invoked.");

        list_for_each_entry(flow, &data->flows, list) {
                if (flow->port_id == id) {
                        kfa_sdu_post(data->kfa,
                                     flow->dst_port_id, sdu);
                        return 0;
                }
                if (flow->dst_port_id == id) {
                        kfa_sdu_post(data->kfa,
                                     flow->port_id, sdu);
                        return 0;
                }
        }
        LOG_ERR("There is no flow allocated for port-id %d", id);

        return -1;
}

static int dummy_deallocate_all(struct ipcp_instance_data * data)
{
        struct dummy_flow *pos, *next;

        list_for_each_entry_safe(pos, next, &data->flows, list) {
                list_del(&pos->list);
                name_destroy(pos->dest);
                name_destroy(pos->source);
                rkfree(pos);
        }
        return 0;
}

struct ipcp_factory_data {
        struct list_head instances;
};

static struct ipcp_factory_data dummy_data;

static int dummy_init(struct ipcp_factory_data * data)
{
        ASSERT(data);

        bzero(&dummy_data, sizeof(dummy_data));
        INIT_LIST_HEAD(&data->instances);

        return 0;
}

static int dummy_fini(struct ipcp_factory_data * data)
{
        ASSERT(data);

        ASSERT(list_empty(&data->instances));

        return 0;
}

static int dummy_assign_to_dif(struct ipcp_instance_data * data,
                               const struct dif_info *  dif_information)
{
        struct ipcp_config * pos;

        ASSERT(data);
        ASSERT(dif_information);

        if (!data->info) {
                LOG_ERR("There is no info for this IPC process");
                return -1;
        }

        data->info->dif_name = name_dup(dif_information->dif_name);
        if (!data->info->dif_name) {
                char * tmp = name_tostring(dif_information->dif_name);
                LOG_ERR("Assingment of IPC Process to DIF %s failed", tmp);
                rkfree(tmp);
                rkfree(data->info);

                return -1;
        }

        if (dif_information->configuration)
                list_for_each_entry(
                                    pos,
                                    &(dif_information->configuration->ipcp_config_entries),
                                    next)
                        LOG_DBG("Configuration entry name: %s; value: %s",
                                pos->entry->name,
                                pos->entry->value);

        LOG_DBG("Assigned IPC Process to DIF %s",
                data->info->dif_name->process_name);

        return 0;
}

static int dummy_update_dif_config(struct ipcp_instance_data * data,
                                   const struct dif_config *   new_config)
{
        /* Nothing to be reconfigured */

        return -1;
}

static struct ipcp_instance_ops dummy_instance_ops = {
        .flow_allocate_request  = dummy_flow_allocate_request,
        .flow_allocate_response = dummy_flow_allocate_response,
        .flow_deallocate        = dummy_flow_deallocate,
        .application_register   = dummy_application_register,
        .application_unregister = dummy_application_unregister,
        .sdu_write              = dummy_sdu_write,
        .assign_to_dif          = dummy_assign_to_dif,
        .update_dif_config      = dummy_update_dif_config,
};

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

static struct ipcp_instance * dummy_create(struct ipcp_factory_data * data,
                                           const struct name *        name,
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
        LOG_DBG("Create an instance");
        inst = rkzalloc(sizeof(*inst), GFP_KERNEL);
        if (!inst)
                return NULL;

        /* fill it properly */
        LOG_DBG("Fill it properly");
        inst->ops  = &dummy_instance_ops;
        inst->data = rkzalloc(sizeof(struct ipcp_instance_data), GFP_KERNEL);
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

        inst->data->info->name = name_dup(name);
        if (!inst->data->info->name) {
                LOG_DBG("Failed creation of ipc name");
                rkfree(inst->data->info);
                rkfree(inst->data);
                rkfree(inst);
                return NULL;
        }

        /* FIXME: Remove as soon as the kipcm_kfa gets removed*/
        inst->data->kfa = kipcm_kfa(default_kipcm);
        LOG_DBG("KFA instance %pK bound to the shim dummy", inst->data->kfa);

        /*
         * Bind the shim-instance to the shims set, to keep all our data
         * structures linked (somewhat) together
         */

        INIT_LIST_HEAD(&inst->data->apps_registered);

        LOG_DBG("Adding dummy instance to the list of shim dummy instances");

        INIT_LIST_HEAD(&(inst->data->list));
        LOG_DBG("Initialization of instance list: %pK", &inst->data->list);
        list_add(&(inst->data->list), &(data->instances));
        LOG_DBG("Inst %pK added to the dummy instances", inst);

        return inst;
}

static int dummy_destroy(struct ipcp_factory_data * data,
                         struct ipcp_instance *     instance)
{
        struct ipcp_instance_data * pos, * next;

        ASSERT(data);
        ASSERT(instance);

        /* Retrieve the instance */
        list_for_each_entry_safe(pos, next, &data->instances, list) {
                if (pos->id == instance->data->id) {
                        /* Unbind from the instances set */
                        list_del(&pos->list);
                        dummy_deallocate_all(pos);
                        dummy_unregister_all(pos);

                        /* Destroy it */
                        if (pos->info->dif_name)
                                name_destroy(pos->info->dif_name);

                        if (pos->info->name)
                                name_destroy(pos->info->name);

                        rkfree(pos->info);
                        rkfree(pos);
                        rkfree(instance);

                        return 0;
                }
        }

        return -1;
}

static struct ipcp_factory_ops dummy_ops = {
        .init      = dummy_init,
        .fini      = dummy_fini,
        .create    = dummy_create,
        .destroy   = dummy_destroy,
};

struct ipcp_factory * shim = NULL;

static int __init mod_init(void)
{
        shim = kipcm_ipcp_factory_register(default_kipcm,
                                           SHIM_NAME,
                                           &dummy_data,
                                           &dummy_ops);
        if (!shim) {
                LOG_CRIT("Cannot register %s factory", SHIM_NAME);
                return -1;
        }

        return 0;
}

static void __exit mod_exit(void)
{
        ASSERT(shim);

        if (kipcm_ipcp_factory_unregister(default_kipcm, shim)) {
                LOG_CRIT("Cannot unregister %s factory", SHIM_NAME);
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
