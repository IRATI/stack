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

#define USE_WQ 1

#include <linux/module.h>
#include <linux/list.h>
#if USE_WQ
#include <linux/workqueue.h>
#endif

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
#include "netlink.h"
#include "netlink-utils.h"

#if USE_WQ
/* DO NOT REMOVE THIS CODE, IT WILL BE MOVED TO utils.[ch] */
/* DO NOT REMOVE THIS CODE, IT WILL BE MOVED TO utils.[ch] */
/* DO NOT REMOVE THIS CODE, IT WILL BE MOVED TO utils.[ch] */

/* It's global BUT should be placed into a shim-dummy handle */
static struct workqueue_struct * wq;

struct work_item {
        struct work_struct work; /* Must be at the beginning */

        struct sdu *       sdu;
};

/* FIXME: Statics commented out to prevent compiler from barfing */

/* static */ void wq_worker(struct work_struct * work)
{
        struct work_item * data = (struct work_item *) work;

        LOG_DBG("Working on a new SDU (%pK)", data->sdu);

        /* We're the owner of the data, let's free it */
        rkfree(data->sdu);
        rkfree(data);

        return;
}

/* static */ int wq_init(void)
{
        ASSERT(!wq); /* Already initialized */

        wq = create_workqueue("shim-dummy-workqueue");

        return 0;
}

/* static */ int wq_post(struct sdu * sdu)
{
        struct work_item * work;

        if (!sdu)
                return -1;

        work = (struct work_item *) rkzalloc(sizeof(struct work_item),
                                             GFP_KERNEL);
        if (!work)
                return -1;

        /* Filling workqueue item */
        INIT_WORK((struct work_struct *) work, wq_worker);
        work->sdu = sdu;

        /* Finally posting the work to do */
        if (queue_work(wq, (struct work_struct *) work)) {
                LOG_ERR("Cannot post work on workqueue");
                return -1;
        }

        LOG_DBG("Work posted on the workqueue, please wait ...");

        return 0;
}

/* static */ void wq_fini(void)
{
        flush_workqueue(wq);
        destroy_workqueue(wq);
}
#endif

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
        uint_t                dst_seq_num;
        uint_t                seq_num; /* Required to notify back to the */
        ipc_process_id_t      dst_id;  /* IPC Manager the result of the
                                        * allocation */
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
                uint_t                     seq_num)
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
                LOG_ERR("Application is not registered in this IPC Process");
                return -1;
        }

        if (find_flow(data, id)) {
                LOG_ERR("A flow already exists on port %d", id);
                return -1;
        }

        flow = rkzalloc(sizeof(*flow), GFP_KERNEL);
        if (!flow)
                return -1;

        flow->seq_num = seq_num;
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

        flow->state = PORT_STATE_INITIATOR_ALLOCATE_PENDING;
        flow->port_id = id;
        flow->dst_seq_num = 666; /*FIXME!!!*/
        INIT_LIST_HEAD(&flow->list);
        list_add(&flow->list, &data->flows);

        if (rnl_app_alloc_flow_req_arrived_msg(data->id,
                                               data->info->dif_name,
                                               source,
                                               dest,
                                               fspec,
                                               flow->dst_seq_num,
                                               1)) {
                list_del(&flow->list);
                name_destroy(flow->source);
                name_destroy(flow->dest);
                rkfree(flow);
                return -1;
        }

        return 0;
}

static struct dummy_flow * find_flow_by_seq_num(struct ipcp_instance_data * data,
                                                uint_t                      seq_num)
{
        struct dummy_flow * flow;

        list_for_each_entry(flow, &data->flows, list) {
                if (flow->dst_seq_num == seq_num) {
                        return flow;
                }
        }

        return NULL;
}

static int dummy_flow_allocate_response(struct ipcp_instance_data * data,
                                        port_id_t                   id,
                                        uint_t                      seq_num,
                                        response_reason_t *         response)
{
        struct dummy_flow * flow;

        ASSERT(data);
        ASSERT(response);

        if (!data->info) {
                LOG_ERR("There is not info in this IPC Process");
                return -1;
        }

        if (!data->info->dif_name) {
                LOG_ERR("This IPC Process doesn't belong to a DIF");
                return -1;
        }

        flow = find_flow_by_seq_num(data, seq_num);
        if (!flow) {
                LOG_ERR("Flow does not exist, cannot allocate");
                return -1;
        }

        if (flow->state != PORT_STATE_INITIATOR_ALLOCATE_PENDING)
                return -1;

        /* On positive response, flow should transition to allocated state */
        if (*response == 0) {
                flow->dst_port_id = id;
                flow->state = PORT_STATE_ALLOCATED;
                if (kipcm_flow_add(default_kipcm, data->id, flow->port_id)) {
                        list_del(&flow->list);
                        name_destroy(flow->source);
                        name_destroy(flow->dest);
                        rkfree(flow);
                        return -1;
                }
                if (kipcm_flow_add(default_kipcm, data->id, id)) {
                        kipcm_flow_remove(default_kipcm, id);
                        list_del(&flow->list);
                        name_destroy(flow->source);
                        name_destroy(flow->dest);
                        rkfree(flow);
                        return -1;
                }
                if (rnl_app_alloc_flow_result_msg(data->id,
                                0,
                                flow->seq_num,
                                1)) {
                        kipcm_flow_remove(default_kipcm, flow->port_id);
                        kipcm_flow_remove(default_kipcm, flow->dst_port_id);
                        list_del(&flow->list);
                        name_destroy(flow->source);
                        name_destroy(flow->dest);
                        rkfree(flow);
                        return -1;
                }
        } else {
                if (rnl_app_alloc_flow_result_msg(data->id,
                                -1,
                                flow->seq_num,
                                1))
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

static int dummy_application_register(struct ipcp_instance_data * data,
                                      const struct name *         source)
{
        struct app_register * app_reg;

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

        list_for_each_entry(flow, &data->flows, list) {
                if (flow->port_id == id) {
                        kipcm_sdu_post(default_kipcm, flow->dst_port_id, sdu);
                        return 0;
                }
                if (flow->dst_port_id == id) {
                        kipcm_sdu_post(default_kipcm, flow->port_id, sdu);
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
                const struct name *         dif_name)
{
        ASSERT(data);

        if (!data->info) {
                LOG_ERR("There is no info for this IPC process");
                return -1;
        }

        data->info->dif_name = name_dup(dif_name);
        if (!data->info->dif_name) {
                char * tmp = name_tostring(dif_name);
                LOG_ERR("Assingment of IPC Process to DIF %s failed", tmp);
                rkfree(tmp);
                rkfree(data->info);

                return -1;
        }

        LOG_DBG("Assigned IPC Process to DIF %s",
                        data->info->dif_name->process_name);

        return 0;
}

static struct ipcp_instance_ops dummy_instance_ops = {
        .flow_allocate_request  = dummy_flow_allocate_request,
        .flow_allocate_response = dummy_flow_allocate_response,
        .flow_deallocate        = dummy_flow_deallocate,
        .application_register   = dummy_application_register,
        .application_unregister = dummy_application_unregister,
        .sdu_write              = dummy_sdu_write,
        .assign_to_dif          = dummy_assign_to_dif,
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

        INIT_LIST_HEAD(&inst->data->apps_registered);

        LOG_DBG("Adding dummy instance to the list of shim dummy instances");

        INIT_LIST_HEAD(&(inst->data->list));
        LOG_DBG("Initialization of instance list: %pK", &inst->data->list);
        list_add(&(inst->data->list), &(data->instances));
        LOG_DBG("Inst %pK added to the dummy instances", inst);

        return inst;
}

/* FIXME: It doesn't allow reconfiguration */
static struct ipcp_instance * dummy_configure(struct ipcp_factory_data * data,
                                              struct ipcp_instance *     inst,
                                              const struct ipcp_config * conf)
{
        struct ipcp_instance_data * instance;
        struct ipcp_config *        tmp;

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

static struct ipcp_factory_ops dummy_ops = {
        .init      = dummy_init,
        .fini      = dummy_fini,
        .create    = dummy_create,
        .destroy   = dummy_destroy,
        .configure = dummy_configure,
};

struct ipcp_factory * shim;

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
