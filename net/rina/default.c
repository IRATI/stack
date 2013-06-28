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
        void * kipcm;
        void * efcp;
        void * rmt;
        void * shims;
};

#define to_pd(X) ((struct personality_data *) X)

static int default_ipc_create(void *                data,
                              const struct name_t * name,
                              ipc_process_id_t      id,
                              dif_type_t            type)
{
        if (!data) return -1;

        LOG_DBG("Calling wrapped function");

        return kipcm_ipc_process_create(to_pd(data)->kipcm, name, id, type);
}

static int default_ipc_configure(void *                            data,
                                 ipc_process_id_t                  id,
                                 const struct ipc_process_conf_t * conf)
{
        if (!data) return -1;

        LOG_DBG("Calling wrapped function");

        return kipcm_ipc_process_configure(to_pd(data)->kipcm, id, conf);
}

static int default_ipc_destroy(void *           data,
                               ipc_process_id_t id)
{
        if (!data) return -1;

        LOG_DBG("Calling wrapped function");

        return kipcm_ipc_process_destroy(to_pd(data)->kipcm, id);
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

int default_sdu_write(void *               data,
                      port_id_t            id,
                      const struct sdu_t * sdu)
{
        if (!data) return -1;

        LOG_DBG("Calling wrapped function");

        return kipcm_sdu_write(to_pd(data)->kipcm, id, sdu);
}

int default_sdu_read(void *         data,
                     port_id_t      id,
                     struct sdu_t * sdu)
{
        if (!data) return -1;

        LOG_DBG("Calling wrapped function");

        return kipcm_sdu_read(to_pd(data)->kipcm, id, sdu);
}

struct personality_ops ops = {
	.init               = NULL,
	.fini               = NULL,
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

        LOG_DBG("Rina default personality initializing");

        if (personality) {
                LOG_ERR("Rina default personality already initialized, "
                        "bailing out");
                return -1;
        }

        LOG_DBG("Initializing shims layer");
        data.shims = shims_init(NULL); /* FIXME: Wrong root node */
        if (!data.shims)
                goto CLEANUP_NONE;

        LOG_DBG("Initializing kipcm component");
        data.kipcm = kipcm_init();
        if (!data.kipcm)
                goto CLEANUP_SHIM;

        LOG_DBG("Initializing efcp component");
        data.efcp = efcp_init();
        if (!data.efcp)
                goto CLEANUP_KIPCM;

        LOG_DBG("Initializing rmt component");
        data.rmt = rmt_init();
        if (!data.rmt)
                goto CLEANUP_EFCP;

        LOG_DBG("Finally registering personality");
        personality = rina_personality_register("default", &data, &ops);
        if (!personality) {
                goto CLEANUP_RMT;
        }

        LOG_DBG("Rina default personality loaded successfully");

        LOG_FEXIT;
        return 0;

 CLEANUP_RMT:
        rmt_fini(data.rmt);
 CLEANUP_EFCP:
        efcp_fini(data.efcp);
 CLEANUP_KIPCM:
        kipcm_fini(data.kipcm);
 CLEANUP_SHIM:
        shims_fini(data.shims);
 CLEANUP_NONE:

        return -1;
}

static void __exit mod_exit(void)
{
        ASSERT(personality);

        if (rina_personality_unregister(personality)) {
                LOG_CRIT("Got problems while unregistering personality, "
                         "bailing out");

                LOG_FEXIT;
                return;
        }

        rmt_fini(data.rmt);
        efcp_fini(data.efcp);
        kipcm_fini(data.kipcm);
        shims_fini(data.shims);

        personality = NULL;

        LOG_DBG("Rina default personality unloaded successfully");

        LOG_FEXIT;
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RINA default personality");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
