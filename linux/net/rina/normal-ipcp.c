/*
 *  IPC Processes layer
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

#define IPCP_NAME "normal-ipc"
#define RINA_PREFIX IPCP_NAME

#include "logs.h"
#include "normal-ipcp.h"
#include "common.h"
#include "debug.h"
#include "utils.h"
#include "kipcm.h"
#include "ipcp-utils.h"
#include "ipcp-factories.h"
#include "du.h"
#include "kfa.h"
#include "rnl-utils.h"

/*  FIXME: To be removed ABSOLUTELY */
extern struct kipcm * default_kipcm;

struct normal_info {
        struct name * name;
        struct name * dif_name;
};

struct ipcp_instance_data {
        /* FIXME add missing needed attributes */
        ipc_process_id_t     id;
        struct list_head     list;
        struct normal_info * info;
        struct list_head     apps_registered;
        /*  FIXME: Remove it as soon as the kipcm_kfa gets removed*/
        struct kfa *         kfa;
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


static struct ipcp_instance_ops normal_instance_ops = {
        .flow_allocate_request  = NULL,
        .flow_allocate_response = NULL,
        .flow_deallocate        = NULL,
        .application_register   = NULL,
        .application_unregister = NULL,
        .sdu_write              = NULL,
        .assign_to_dif          = NULL,
        .update_dif_config      = NULL,
};

static struct ipcp_instance * normal_create(struct ipcp_factory_data * data,
                                            const struct name *        name,
                                            ipc_process_id_t          id)
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
                LOG_ERR("Could not allocate memory for normal ipc process " \
                        "with id %d", id);
                return NULL;
        }

        instance->ops  = &normal_instance_ops;
        instance->data = rkzalloc(sizeof(struct ipcp_instance_data), GFP_KERNEL);
        if (!instance->data) {
                LOG_ERR("Could not allocate memory for normal ipcp " \
                        "internal data");
                rkfree(instance);
                return NULL;
        }

        instance->data->id = id;
        instance->data->info = rkzalloc(sizeof(struct normal_info *), GFP_KERNEL);
        if (!instance->data->info) {
                LOG_ERR("Could not allocate momory for normal ipcp info");
                rkfree(instance->data);
                rkfree(instance);
                return NULL;
        }

        /*  FIXME: Remove as soon as the kipcm_kfa gets removed */
        instance->data->kfa = kipcm_kfa(default_kipcm);
        
        INIT_LIST_HEAD(&instance->data->apps_registered);
        INIT_LIST_HEAD(&instance->data->list);
        list_add(&(instance->data->list), &(data->instances));
        LOG_DBG("Normal IPC process instance created and added to the list");

        return instance;
}

static int normal_destroy(struct ipcp_factory_data * data,
                          struct ipcp_instance *     instance)
{
        LOG_MISSING;
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
