/*
 *  Normal IPC Process
 *
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#define IPCP_NAME   "normal-ipc"

#define RINA_PREFIX IPCP_NAME

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
#include "efcp.h"
#include "rmt.h"
#include "efcp-utils.h"

/*  FIXME: To be removed ABSOLUTELY */
extern struct kipcm * default_kipcm;

struct normal_info {
        struct name * name;
        struct name * dif_name;
};

struct ipcp_instance_data {
        /* FIXME add missing needed attributes */
        ipc_process_id_t        id;
        struct list_head        flows;
        struct list_head        list;
        struct normal_info *    info;
        /*  FIXME: Remove it as soon as the kipcm_kfa gets removed*/
        struct kfa *            kfa;
        struct efcp_container * efcpc;
        struct rmt *            rmt;
};

enum normal_flow_state {
        PORT_STATE_NULL = 1,
        PORT_STATE_RECIPIENT_ALLOCATE_PENDING,
        PORT_STATE_INITIATOR_ALLOCATE_PENDING,
        PORT_STATE_ALLOCATED
};

struct normal_flow {
        port_id_t               port_id;
        port_id_t               dst_port_id;
        struct name *           source;
        struct name *           dest;
        struct list_head        list;
        enum normal_flow_state  state;
        flow_id_t               dst_fid;
        flow_id_t               src_fid; /* Required to notify back to the */
        ipc_process_id_t        dst_id;  /* IPC Manager the result of the */
        struct flow_spec *      fspec;   /* allocation */
        struct efcp_container * efcps;
        struct rmt *            rmt;

};

struct ipcp_factory_data {
        struct list_head instances;
};

static struct ipcp_factory_data normal_data;

static int normal_init(struct ipcp_factory_data * data)
{
        ASSERT(data);

        bzero(&normal_data, sizeof(normal_data));
        INIT_LIST_HEAD(&data->instances);

        return 0;
}

static int normal_fini(struct ipcp_factory_data * data)
{
        ASSERT(data);
        ASSERT(list_empty(&data->instances));
        return 0;
}

static int normal_sdu_write(struct ipcp_instance_data * data,
                            port_id_t                   id,
                            struct sdu *                sdu)
{
        LOG_MISSING;
        return 0;
}

struct ipcp_factory * normal = NULL;

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

static cep_id_t connection_create_request(struct ipcp_instance_data * data,
                                          port_id_t                   port_id,
                                          address_t                   source,
                                          address_t                   dest,
                                          qos_id_t                    qos_id,
                                          int                         policies)
{
        cep_id_t cep_id;
        struct connection * conn;

        conn = rkzalloc(sizeof(*conn), GFP_KERNEL);
        if (!conn) {
                LOG_ERR("Failed connection creation");
                return -1;
        }
        conn->destination_address = dest;
        conn->source_address      = source;
        conn->port_id             = port_id;
        conn->qos_id              = qos_id;

        cep_id = efcp_connection_create(data->efcpc, conn);
        if (!is_cep_id_ok(cep_id)) {
                LOG_ERR("Failed EFCP connection creation");
                rkfree(conn);
                return cep_id_bad();
        }

        return cep_id;
}

static int connection_update_request(struct ipcp_instance_data * data,
                                     port_id_t                   port_id,
                                     cep_id_t                    src_cep_id,
                                     cep_id_t                    dst_cep_id)
{
        if (efcp_connection_update(data->efcpc, src_cep_id, dst_cep_id))
                return -1;

        return 0;
}

static int connection_destroy_request(struct ipcp_instance_data * data,
                                      port_id_t                   port_id,
                                      cep_id_t                    src_cep_id)
{
        if (efcp_connection_destroy(data->efcpc, src_cep_id))
                return -1;

        return 0;
}

static cep_id_t
connection_create_arrived(struct ipcp_instance_data * data,
                          port_id_t                   port_id,
                          cep_id_t                    src_cep_id)
{
        LOG_MISSING;
        return -1;
}

/*  FIXME: register ops */
static struct ipcp_instance_ops normal_instance_ops = {
        .flow_allocate_request     = NULL,
        .flow_allocate_response    = NULL,
        .flow_deallocate           = NULL,
        .application_register      = NULL,
        .application_unregister    = NULL,
        .sdu_write                 = normal_sdu_write,
        .assign_to_dif             = NULL,
        .update_dif_config         = NULL,
        .connection_create         = connection_create_request,
        .connection_update         = connection_update_request,
        .connection_destroy        = connection_destroy_request,
        .connection_create_arrived = connection_create_arrived,
};

static struct ipcp_instance * normal_create(struct ipcp_factory_data * data,
                                            const struct name *        name,
                                            ipc_process_id_t           id)
{
        struct ipcp_instance * instance;

        ASSERT(data);

        if (find_instance(data, id)) {
                LOG_ERR("There is already a normal ipcp instance with id %d",
                        id);
                return NULL;
        }

        LOG_DBG("Creating normal IPC process...");
        instance = rkzalloc(sizeof(*instance), GFP_KERNEL);
        if (!instance) {
                LOG_ERR("Could not allocate memory for normal ipc process "
                        "with id %d", id);
                return NULL;
        }

        instance->ops  = &normal_instance_ops;
        instance->data = rkzalloc(sizeof(struct ipcp_instance_data),
                                  GFP_KERNEL);
        if (!instance->data) {
                LOG_ERR("Could not allocate memory for normal ipcp "
                        "internal data");
                rkfree(instance);
                return NULL;
        }

        instance->data->id = id;
        instance->data->info = rkzalloc(sizeof(struct normal_info *),
                                        GFP_KERNEL);
        if (!instance->data->info) {
                LOG_ERR("Could not allocate momory for normal ipcp info");
                rkfree(instance->data);
                rkfree(instance);
                return NULL;
        }

        instance->data->info->name = name_dup(name);
        if (!instance->data->info->name) {
                LOG_ERR("Failed creation of ipc name");
                rkfree(instance->data->info);
                rkfree(instance->data);
                rkfree(instance);
                return NULL;
        }

        instance->data->efcpc = efcp_container_create();
        if (!instance->data->efcpc) {
                LOG_ERR("Failed creation of EFCP container");
                rkfree(instance->data->info->name);
                rkfree(instance->data->info);
                rkfree(instance->data);
                rkfree(instance);
                return NULL;
        }

        instance->data->rmt = rmt_create();
        if (!instance->data->rmt) {
                LOG_ERR("Failed creation of EFCP container");
                rkfree(instance->data->info->name);
                rkfree(instance->data->info);
                rkfree(instance->data);
                rkfree(instance);
                return NULL;
        }

        /*  FIXME: Remove as soon as the kipcm_kfa gets removed */
        instance->data->kfa = kipcm_kfa(default_kipcm);

        /* FIXME: Probably missing normal flow structures creation */
        INIT_LIST_HEAD(&instance->data->flows);
        INIT_LIST_HEAD(&instance->data->list);
        list_add(&(instance->data->list), &(data->instances));
        LOG_DBG("Normal IPC process instance created and added to the list");

        return instance;
}

static int normal_deallocate_all(struct ipcp_instance_data * data)
{
        LOG_MISSING;
        return 0;
}

static int normal_destroy(struct ipcp_factory_data * data,
                          struct ipcp_instance *     instance)
{

        struct ipcp_instance_data * tmp;

        ASSERT(data);
        ASSERT(instance);

        tmp = find_instance(data, instance->data->id);
        if (!tmp) {
                LOG_ERR("Could not find normal ipcp instance to destroy");
                return -1;
        }

        list_del(&tmp->list);

        /* FIXME: flow deallocation not implemented */
        if (normal_deallocate_all(tmp)) {
                LOG_ERR("Could not deallocate normal ipcp flows");
                return -1;
        }

        if (tmp->info->dif_name)
                name_destroy(tmp->info->dif_name);

        if (tmp->info->name)
                name_destroy(tmp->info->name);

        rkfree(tmp->info);
        rkfree(tmp);
        rkfree(instance);

        return 0;
}

static struct ipcp_factory_ops normal_ops = {
        .init    = normal_init,
        .fini    = normal_fini,
        .create  = normal_create,
        .destroy = normal_destroy,
};

static int __init mod_init(void)
{
        LOG_DBG("RINA IPCP loading");

        if (normal) {
                LOG_ERR("RINA normal IPCP already initialized, bailing out");
                return -1;
        }

        normal = kipcm_ipcp_factory_register(default_kipcm,
                                             IPCP_NAME,
                                             &normal_data,
                                             &normal_ops);
        ASSERT(normal != NULL);

        if (!normal) {
                LOG_CRIT("Could not register %s factory, bailing out",
                         IPCP_NAME);
                return -1;
        }

        LOG_DBG("RINA normal IPCP loaded successfully");

        return 0;
}

static void __exit mod_exit(void)
{

        ASSERT(normal);

        if (kipcm_ipcp_factory_unregister(default_kipcm, normal)) {
                LOG_CRIT("Could not unregister %s factory, bailing out",
                         IPCP_NAME);
                return;
        }

        LOG_DBG("RINA normal IPCP unloaded successfully");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RINA normal IPC Process");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
MODULE_AUTHOR("Miquel Tarzan <miquel.tarzan@i2cat.net>");
MODULE_AUTHOR("Sander Vrijders <sander.vrijders@intec.ugent.be>");
MODULE_AUTHOR("Leonardo Bergesio <leonardo.bergesio@i2cat.net>");
