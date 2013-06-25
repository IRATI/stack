/*
 * RINA default personality
 *
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

#include <linux/slab.h>
#include <linux/module.h>

#define RINA_PREFIX "personality-default"

#include "logs.h"
#include "utils.h"
#include "personality.h"
#include "shim.h"
#include "kipcm.h"
#include "efcp.h"
#include "rmt.h"

#define DEFAULT_LABEL "default"

static struct personality_t * personality = NULL;

struct personality_data {
        void * kipcm;
        void * efcp;
        void * rmt;
};

static int default_ipc_create(struct personality_data * data,
                              const struct name_t *     name,
                              ipc_process_id_t          id,
                              dif_type_t                type)
{
        if (!data)
                return -1;

        return kipcm_ipc_process_create(data->kipcm, name, id, type);
}

static int default_ipc_configure(struct personality_data *         data,
                                 ipc_process_id_t                  id,
                                 const struct ipc_process_conf_t * conf)
{
        if (!data)
                return -1;

        return kipcm_ipc_process_configure(data->kipcm, id, conf);
}

static int default_ipc_destroy(struct personality_data * data,
                               ipc_process_id_t          id)
{
        if (!data)
                return -1;

        return kipcm_ipc_process_destroy(data->kipcm, id);
}

static int default_connection_create(struct personality_data *   data,
                                     const struct connection_t * connection)
{
        if (!data)
                return -1;

        return efcp_create(data->efcp, connection);
}

static int default_connection_destroy(struct personality_data * data,
                                      cep_id_t                  cep_id)
{
        if (!data)
                return -1;

        return efcp_destroy(data->efcp, cep_id);
}

static int default_connection_update(struct personality_data * data,
                                     cep_id_t                  id_from,
                                     cep_id_t                  id_to)
{
        if (!data)
                return -1;

        return efcp_update(data->efcp, id_from, id_to);
}

int default_sdu_write(struct personality_data * data,
                      port_id_t                 id,
                      const struct sdu_t *      sdu)
{
        if (!data)
                return -1;

        return kipcm_sdu_write(data->kipcm, id, sdu);
}

int default_sdu_read(struct personality_data * data,
                     port_id_t                 id,
                     struct sdu_t *            sdu)
{
        if (!data)
                return -1;

        return kipcm_sdu_read(data->kipcm, id, sdu);
}

static struct personality_t * personality_init(void)
{
        struct personality_t * p;

        p = kzalloc(sizeof(*p), GFP_KERNEL);
        if (!p) {
                LOG_ERR("Cannot allocate %zu bytes of memory",
                        sizeof(*p));
                return NULL;
        }
        p->label = DEFAULT_LABEL;
        p->data  = kzalloc(sizeof(struct personality_data), GFP_KERNEL);
        if (!p->data) {
                LOG_ERR("Cannot allocate %zu bytes of memory",
                        sizeof(struct personality_data));
                kfree(personality);
                return NULL;
        }

        return p;
}

static void personality_fini(struct personality_t * pers)
{
        ASSERT(pers);
        ASSERT(pers->data);

        kfree(pers->data);
        kfree(pers);
}

static int __init mod_init(void)
{
        struct personality_data * pd;

        LOG_FBEGN;

        LOG_DBG("Rina personality initializing");

        personality = personality_init();
        if (!personality)
                return -ENOMEM;

        ASSERT(personality);
        ASSERT(personality->data);

        LOG_DBG("Building-up personality data");

        pd = personality->data;

        LOG_DBG("Initializing shim layer");
        if (shim_init())
                return -1;

        LOG_DBG("Initializing kipcm component");
        pd->kipcm = kipcm_init();
        if (!pd->kipcm)
                goto CLEANUP_SHIM;

        LOG_DBG("Initializing efcp component");
        pd->efcp = efcp_init();
        if (!pd->efcp)
                goto CLEANUP_KIPCM;

        LOG_DBG("Initializing rmt component");
        pd->rmt = rmt_init();
        if (!pd->rmt)
                goto CLEANUP_EFCP;

        LOG_DBG("Filling-up personality's hooks");

        personality->init               = 0;
        personality->fini               = 0;
        personality->ipc_create         = default_ipc_create;
        personality->ipc_configure      = default_ipc_configure;
        personality->ipc_destroy        = default_ipc_destroy;
        personality->sdu_read           = default_sdu_read;
        personality->sdu_write          = default_sdu_write;
        personality->connection_create  = default_connection_create;
        personality->connection_destroy = default_connection_destroy;
        personality->connection_update  = default_connection_update;

        LOG_DBG("Finally registering personality");
        if (rina_personality_register(personality)) {
                goto CLEANUP_RMT;
        }

        ASSERT(pd->kipcm);
        ASSERT(pd->efcp);
        ASSERT(pd->rmt);

        LOG_DBG("Rina default personality loaded successfully");

        LOG_FEXIT;
        return 0;

 CLEANUP_RMT:
        rmt_fini(pd->rmt);
 CLEANUP_EFCP:
        efcp_fini(pd->efcp);
 CLEANUP_KIPCM:
        kipcm_fini(pd->kipcm);
 CLEANUP_SHIM:
        shim_exit();

        personality_fini(personality);
        personality = NULL;

        return -1;
}

static void __exit mod_exit(void)
{
        struct personality_data * pd;

        LOG_FBEGN;

        ASSERT(personality);

        if (rina_personality_unregister(personality)) {
                LOG_CRIT("Got problems while unregistering personality, "
                         "bailing out");

                LOG_FEXIT;
                return;
        }
        pd = personality->data;

        rmt_fini(pd->rmt);
        efcp_fini(pd->efcp);
        kipcm_fini(pd->kipcm);

        shim_exit();

        personality_fini(personality);
        personality = NULL;

        LOG_DBG("Rina default personality unloaded successfully");

        LOG_FEXIT;
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RINA default personality");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
