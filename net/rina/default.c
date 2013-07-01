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

struct personality_data {
        struct kipcm * kipcm;

        /* FIXME: Types to be rearranged */
        void *         efcp;
        void *         rmt;
};

#define to_pd(X) ((struct personality_data *) X)

static int default_ipc_create(void *                data,
                              const struct name_t * name,
                              ipc_process_id_t      id,
                              dif_type_t            type)
{
        if (!data) return -1;

        LOG_DBG("Calling wrapped function");

        return kipcm_ipc_create(to_pd(data)->kipcm, name, id, type);
}

static int default_ipc_configure(void *                          data,
                                 ipc_process_id_t                id,
                                 const struct ipc_process_conf * conf)
{
        if (!data) return -1;

        LOG_DBG("Calling wrapped function");

        return kipcm_ipc_configure(to_pd(data)->kipcm, id, conf);
}

static int default_ipc_destroy(void *           data,
                               ipc_process_id_t id)
{
        if (!data) return -1;

        LOG_DBG("Calling wrapped function");

        return kipcm_ipc_destroy(to_pd(data)->kipcm, id);
}

static int default_connection_create(void *                      data,
                                     const struct connection_t * connection)
{
        if (!data) return -1;

        LOG_DBG("Calling wrapped function");

        return efcp_create(to_pd(data)->efcp, connection);
}

static int default_connection_destroy(void *   data,
                                      cep_id_t cep_id)
{
        if (!data) return -1;

        LOG_DBG("Calling wrapped function");

        return efcp_destroy(to_pd(data)->efcp, cep_id);
}

static int default_connection_update(void *   data,
                                     cep_id_t id_from,
                                     cep_id_t id_to)
{
        if (!data) return -1;

        LOG_DBG("Calling wrapped function");

        return efcp_update(to_pd(data)->efcp, id_from, id_to);
}

static int default_sdu_write(void *               data,
                             port_id_t            id,
                             const struct sdu_t * sdu)
{
        if (!data) return -1;

        LOG_DBG("Calling wrapped function");

        return kipcm_sdu_write(to_pd(data)->kipcm, id, sdu);
}

static int default_sdu_read(void *         data,
                            port_id_t      id,
                            struct sdu_t * sdu)
{
        if (!data) return -1;

        LOG_DBG("Calling wrapped function");

        return kipcm_sdu_read(to_pd(data)->kipcm, id, sdu);
}

static int default_fini(void * data)
{
        struct personality_data * tmp = data;
        int                       err;

        if (!data) return -1;

        LOG_DBG("Finalizing default personality");

        if (tmp->rmt) {
                err = rmt_fini(tmp->rmt);
                if (err) return err;
        }
        if (tmp->efcp) {
                err = efcp_fini(tmp->efcp);
                if (err) return err;
        }
        if (tmp->kipcm) {
                err = kipcm_fini(tmp->kipcm);
                if (err) return err;
        }

        LOG_DBG("Default personality finalized successfully");

        return 0;
}

/* FIXME: To be removed ABSOLUTELY */
struct kipcm * default_kipcm;
EXPORT_SYMBOL(default_kipcm);

static int default_init(struct kobject * parent,
                        void *           data)
{
        struct personality_data * tmp = data;

        if (!tmp) return -1;

        LOG_DBG("Initializing default personality");

        LOG_DBG("Initializing kipcm component");
        tmp->kipcm = kipcm_init(parent);
        if (!tmp->kipcm) {
                if (default_fini(tmp)) {
                        LOG_CRIT("The system might become unstable ...");
                        return -1;
                }
        }

        /* FIXME: To be removed */
        default_kipcm = tmp->kipcm;

        LOG_DBG("Initializing efcp component");
        tmp->efcp = efcp_init(parent);
        if (!tmp->efcp) {
                if (default_fini(tmp)) {
                        LOG_CRIT("The system might become unstable ...");
                        return -1;
                }
        }

        LOG_DBG("Initializing rmt component");
        tmp->rmt = rmt_init(parent);
        if (!tmp->rmt) {
                if (default_fini(tmp)) {
                        LOG_CRIT("The system might become unstable ...");
                        return -1;
                }
        }

        LOG_DBG("Default personality initialized successfully");

        return 0;
}

struct personality_ops ops = {
        .init               = default_init,
        .fini               = default_fini,
        .ipc_create         = default_ipc_create,
        .ipc_configure      = default_ipc_configure,
        .ipc_destroy        = default_ipc_destroy,
        .sdu_read           = default_sdu_read,
        .sdu_write          = default_sdu_write,
        .connection_create  = default_connection_create,
        .connection_destroy = default_connection_destroy,
        .connection_update  = default_connection_update,
};

static struct personality_data data;

static struct personality *    personality = NULL;

static int __init mod_init(void)
{
        LOG_FBEGN;

        LOG_DBG("Rina default personality loading");

        if (personality) {
                LOG_ERR("Rina default personality already initialized, "
                        "bailing out");
                return -1;
        }

        LOG_DBG("Finally registering personality");
        personality = rina_personality_register("default", &data, &ops);
        if (!personality)
                return -1;

        ASSERT(personality != NULL);

        LOG_DBG("Rina default personality loaded successfully");

        LOG_FEXIT;
        return 0;
}

static void __exit mod_exit(void)
{
        LOG_DBG("Rina default personality unloading");

        ASSERT(personality != NULL);

        if (rina_personality_unregister(personality)) {
                LOG_CRIT("Got problems while unregistering personality, "
                         "bailing out");

                LOG_FEXIT;
                return;
        }

        personality = NULL;

        LOG_DBG("Rina default personality unloaded successfully");

        LOG_FEXIT;
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RINA default personality");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
