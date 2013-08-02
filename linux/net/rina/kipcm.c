/*
 * K-IPCM (Kernel-IPC Manager)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#include <linux/kobject.h>
#include <linux/export.h>

#define RINA_PREFIX "kipcm"

#include "logs.h"
#include "utils.h"
#include "kipcm.h"
#include "debug.h"
#include "ipcp-factories.h"
#include "ipcp-utils.h"
#include "kipcm-utils.h"

#define DEFAULT_FACTORY "normal-ipc"

struct kipcm {
        struct ipcp_factories * factories;
        struct ipcp_imap *      instances;
        struct ipcp_fmap *      flows;
};

struct ipcp_flow {
        /* The port-id identifying the flow */
        port_id_t              port_id;

        /*
         * The components of the IPC Process that will handle the
         * write calls to this flow
         */
        struct ipcp_instance * ipc_process;

        /*
         * True if this flow is serving a user-space application, false
         * if it is being used by an RMT
         */
        bool_t                 application_owned;

        /*
         * In case this flow is being used by an RMT, this is a pointer
         * to the RMT instance.
         */
        struct rmt_instance *  rmt_instance;

        //FIXME: Define QUEUE
        //QUEUE(segmentation_queue, pdu *);
        //QUEUE(reassembly_queue,       pdu *);
        //QUEUE(sdu_ready, sdu *);
#if 0
        struct kfifo *         sdu_ready;
#endif
};

struct kipcm * kipcm_init(struct kobject * parent)
{
        struct kipcm * tmp;

        LOG_DBG("Initializing");

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->factories = ipcpf_init(parent);
        if (!tmp->factories) {
                rkfree(tmp);
                return NULL;
        }

        tmp->instances = ipcp_imap_create();
        if (!tmp->instances) {
                if (ipcpf_fini(tmp->factories)) {
                        /* FIXME: What could we do here ? */
                }
                rkfree(tmp);
                return NULL;
        }

        tmp->flows = ipcp_fmap_create();
        if (!tmp->flows) {
                if (ipcp_imap_destroy(tmp->instances)) {
                        /* FIXME: What could we do here ? */
                }
                if (ipcpf_fini(tmp->factories)) {
                        /* FIXME: What could we do here ? */
                }
                rkfree(tmp);
                return NULL;
        }

        LOG_DBG("Initialized successfully");

        return tmp;
}

int kipcm_fini(struct kipcm * kipcm)
{
        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        LOG_DBG("Finalizing");

        /* FIXME: Destroy all the flows */
        ASSERT(ipcp_fmap_empty(kipcm->flows));
        if (ipcp_fmap_destroy(kipcm->flows))
                return -1;
        
        /* FIXME: Destroy all the instances */
        ASSERT(ipcp_imap_empty(kipcm->instances));
        if (ipcp_imap_destroy(kipcm->instances))
                return -1;

        if (ipcpf_fini(kipcm->factories))
                return -1;

        rkfree(kipcm);

        LOG_DBG("Finalized successfully");

        return 0;
}

struct ipcp_factory *
kipcm_ipcp_factory_register(struct kipcm *             kipcm,
                            const char *               name,
                            struct ipcp_factory_data * data,
                            struct ipcp_factory_ops *  ops)
{
        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return NULL;
        }

        return ipcpf_register(kipcm->factories, name, data, ops);
}
EXPORT_SYMBOL(kipcm_ipcp_factory_register);

int kipcm_ipcp_factory_unregister(struct kipcm *        kipcm,
                                  struct ipcp_factory * factory)
{
        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        /* FIXME:
         *
         *   We have to do the body of kipcm_ipcp_destroy() on all the
         *   instances remaining (and not explicitly destroyed), previously
         *   created with the factory being unregisterd ...
         *
         *     Francesco
         */
        return ipcpf_unregister(kipcm->factories, factory);
}
EXPORT_SYMBOL(kipcm_ipcp_factory_unregister);

int kipcm_ipcp_create(struct kipcm *      kipcm,
                      const struct name * ipcp_name,
                      ipc_process_id_t    id,
                      const char *        factory_name)
{
        char *                 name;
        struct ipcp_factory *  factory;
        struct ipcp_instance * instance;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        if (ipcp_imap_find(kipcm->instances, id)) {
                LOG_ERR("Process id %d already exists", id);

                return -1;
        }

        if (!factory_name) {
                LOG_ERR("Name is missing, cannot create ipc");
                return -1;
        }

        if (!factory_name)
                factory_name = DEFAULT_FACTORY;

        name = name_tostring(ipcp_name);
        if (!name)
                return -1;
        LOG_DBG("Creating IPC process:");
        LOG_DBG("  name:      %s", name);
        LOG_DBG("  id:        %d", id);
        LOG_DBG("  factory:   %s", factory_name);
        rkfree(name);

        if (!factory_name)
                factory_name = DEFAULT_FACTORY;

        factory = ipcpf_find(kipcm->factories, factory_name);
        if (!factory) {
                LOG_ERR("Cannot find the requested factory '%s'",
                        factory_name);
                return -1;
        }

        instance = factory->ops->create(factory->data, id);
        if (!instance)
                return -1;

        /* FIXME: Ugly as hell */
        instance->factory = factory;

        if (ipcp_imap_add(kipcm->instances, id, instance)) {
                factory->ops->destroy(factory->data, instance);
                return -1;
        }

        return 0;
}

int kipcm_ipcp_destroy(struct kipcm *  kipcm,
                      ipc_process_id_t id)
{
        struct ipcp_instance * instance;
        struct ipcp_factory *  factory;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        instance = ipcp_imap_find(kipcm->instances, id);
        if (!instance) {
                LOG_ERR("IPC process %d instance does not exist", id);
                return -1;
        }

        factory = instance->factory;
        ASSERT(factory);

        if (factory->ops->destroy(factory->data, instance))
                return -1;
        
        if (ipcp_imap_remove(kipcm->instances, id))
                return -1;

        return 0;
}

int kipcm_ipcp_configure(struct kipcm *            kipcm,
                        ipc_process_id_t           id,
                        const struct ipcp_config * configuration)
{
        struct ipcp_instance * instance_old;
        struct ipcp_factory *  factory;
        struct ipcp_instance * instance_new;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        instance_old = ipcp_imap_find(kipcm->instances, id);
        if (instance_old == NULL)
                return -1;

        factory = instance_old->factory;
        ASSERT(factory);

        instance_new = factory->ops->configure(factory->data,
                                               instance_old,
                                               configuration);
        if (!instance_new)
                return -1;

        if (instance_new != instance_old)
                if (ipcp_imap_update(kipcm->instances, id, instance_new))
                        return -1;
   
        return 0;
}

int kipcm_flow_add(struct kipcm *   kipcm,
                   ipc_process_id_t ipc_id,
                   port_id_t        id)
{
        struct ipcp_flow * flow;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        flow = rkzalloc(sizeof(*flow), GFP_KERNEL);
        if (!flow) {
                return -1;
        }
        
        flow->port_id     = id;
        flow->ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!flow->ipc_process) {
                LOG_ERR("Couldn't find ipc_process %d", ipc_id);
                rkfree(flow);
                return -1;
        }

#if 0
        /* FIXME */
        switch (flow->ipc_process->type) {
        case DIF_TYPE_SHIM:
                flow->application_owned = 1;
                flow->rmt_instance      = NULL;
                break;
        case DIF_TYPE_NORMAL:
                break;
        default:
                BUG();
        }

        /* FIXME: Use kfifo functions! */ 
        flow->sdu_ready = rkzalloc(sizeof(struct kfifo), GFP_KERNEL);
        if (!flow->sdu_ready) {
                rkfree(flow);
                rkfree(port_flow);

                return -1;
        }

        port_flow->id   = id;
        port_flow->flow = flow;
        INIT_LIST_HEAD(&port_flow->list);
        list_add(&port_flow->list, &kipcm->port_id_to_flow);
#else
        return -1;
#endif

        return 0;
}
EXPORT_SYMBOL(kipcm_flow_add);

int kipcm_flow_remove(struct kipcm * kipcm,
                      port_id_t      id)
{
#if 0
        struct port_id_to_flow * port_flow;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        port_flow = retrieve_port_flow_node(kipcm, id);
        if (!port_flow) {
                LOG_ERR("Couldn't retrieve the flow %d", id);

                return -1;
        }

        rkfree(port_flow->flow);
        rkfree(port_flow);
#endif

        return 0;
}
EXPORT_SYMBOL(kipcm_flow_remove);

int kipcm_sdu_write(struct kipcm *     kipcm,
                    port_id_t          id,
                    const struct sdu * sdu)
{
        int                    retval = -1;
#if 0
        struct ipcp_flow *     flow;
        struct ipc_process_t * ipc_process;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        flow = retrieve_flow_by_port_id(kipcm, id);
        if (!flow) {
                LOG_ERR("There is no flow bound to port-id %d", id);

                return -1;
        }

        retval      = -1;
        ipc_process = flow->ipc_process;

        switch (ipc_process->type) {
        case DIF_TYPE_SHIM: {
                struct ipcp_instance * shim_instance;

                shim_instance = ipc_process->data.shim_instance;
                retval = shim_instance->ops->sdu_write(
                                                       shim_instance->data, id, sdu);
        }
                break;
        case DIF_TYPE_NORMAL:
                LOG_MISSING;

                break;
        default:
                BUG();
        }

        if (retval)
                LOG_ERR("Couldn't write on port_id %d", id);
#endif

        return retval;
}

int kipcm_sdu_read(struct kipcm * kipcm,
                   port_id_t      id,
                   struct sdu *   sdu)
{
#if 0
        struct ipcp_flow * flow;
        size_t             size;
        char *             data;


        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }
        if (!sdu || !sdu->buffer) {
                LOG_ERR("Bogus parameters passed, bailing out");
                return -1;
        }

        flow = retrieve_flow_by_port_id(kipcm, id);
        if (!flow) {
                LOG_ERR("There is no flow bound to port-id %d", id);

                return -1;
        }

        if (kfifo_out(flow->sdu_ready, &size, sizeof(size_t)) <
            sizeof(size_t)) {
                LOG_ERR("There is not enough data for port-id %d", id);

                return -1;
        }

        /* FIXME: Is it possible to have 0 bytes sdus ??? */
        if (size == 0) {
                LOG_ERR("Zero-size SDU detected");
                return -1;
        }

        data = rkmalloc(size, GFP_KERNEL);
        if (!data)
                return -1;

        if (kfifo_out(flow->sdu_ready, data, size) != size) {
                LOG_ERR("Could not get %zd bytes from fifo", size);

                return -1;
        }

        sdu->buffer->data = data;
        sdu->buffer->size = size;
#endif

        return 0;
}

int kipcm_sdu_post(struct kipcm * kipcm,
                   port_id_t      id,
                   struct sdu *   sdu)
{
        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, cannot post SDU");
                return -1;
        }
        if (!sdu) {
                LOG_ERR("Bogus parameters passed, bailing out");
                return -1;
        }

        LOG_MISSING;

        return -1;
}
