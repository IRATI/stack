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

#include <linux/export.h>
#include <linux/module.h>

#define RINA_PREFIX "personality-default"

#include "logs.h"
#include "utils.h"
#include "personality.h"
#include "rnl.h"
#include "kipcm.h"
#include "kfa.h"
#include "debug.h"

#define DEFAULT_LABEL "default"

struct personality_data {
        struct kipcm *   kipcm;
        struct rnl_set * nlset;
};

static bool is_personality_ok(const struct personality_data * p)
{
        if (!p)
                return false;
        if (!p->kipcm)
                return false;
        if (!kipcm_kfa(p->kipcm))
                return false;
        if (!p->nlset)
                return false;

        return true;
}

static int default_ipc_create(struct personality_data * data,
                              const struct name *       name,
                              ipc_process_id_t          id,
                              const char *              type)
{
        if (!is_personality_ok(data)) return -1;

        LOG_DBG("Calling wrapped function");

        return kipcm_ipcp_create(data->kipcm, name, id, type);
}

static int default_ipc_destroy(struct personality_data * data,
                               ipc_process_id_t          id)
{
        if (!is_personality_ok(data)) return -1;

        LOG_DBG("Calling wrapped function");

        return kipcm_ipcp_destroy(data->kipcm, id);
}

static int default_sdu_write(struct personality_data * data,
                             port_id_t                 id,
                             struct sdu *              sdu)
{
        if (!is_personality_ok(data)) return -1;

        LOG_DBG("Calling wrapped function");

        return kipcm_sdu_write(data->kipcm, id, sdu);
}

static int default_sdu_read(struct personality_data * data,
                            port_id_t                 id,
                            struct sdu **             sdu)
{
        if (!is_personality_ok(data)) return -1;

        LOG_DBG("Calling wrapped function");

        return kipcm_sdu_read(data->kipcm, id, sdu);
}

static int default_flow_create(struct personality_data * data,
			       ipc_process_id_t          ipc_id,
			       struct name *             name)
{
        if (!is_personality_ok(data)) return -1;

        LOG_DBG("Calling wrapped function");

        return kipcm_flow_create(data->kipcm, ipc_id, name);
}

static int default_flow_opts_set(struct personality_data *data,
				 port_id_t                port_id,
				 flow_opts_t              flow_opts)
{
	  if (!is_personality_ok(data)) return -1;

	  LOG_DBG("Calling wrapped function");

	  return kipcm_flow_opts_set(data->kipcm, port_id, flow_opts);
}

static int default_flow_opts(struct personality_data *data,
			     port_id_t                port_id)
{
	if (!is_personality_ok(data)) return -1;

        LOG_DBG("Calling wrapped function");

        return kipcm_flow_opts(data->kipcm, port_id);
}

static int default_flow_destroy(struct personality_data * data,
				ipc_process_id_t          ipc_id,
				port_id_t                 port_id)
{
        if (!is_personality_ok(data)) return -1;

        LOG_DBG("Calling wrapped function");

        return kipcm_flow_destroy(data->kipcm, ipc_id, port_id);
}

static int default_mgmt_sdu_write(struct personality_data * data,
                                  ipc_process_id_t          id,
                                  struct sdu_wpi *          sdu_wpi)
{
        if (!is_personality_ok(data)) return -1;

        LOG_DBG("Calling wrapped function");

        return kipcm_mgmt_sdu_write(data->kipcm, id, sdu_wpi);
}

static int default_mgmt_sdu_read(struct personality_data * data,
                                 ipc_process_id_t          id,
                                 struct sdu_wpi **         sdu_wpi)
{
        if (!is_personality_ok(data)) return -1;

        LOG_DBG("Calling wrapped function");

        return kipcm_mgmt_sdu_read(data->kipcm, id, sdu_wpi);
}

/* FIXME: To be removed ABSOLUTELY */
struct kipcm * default_kipcm = NULL;
EXPORT_SYMBOL(default_kipcm);

static int default_fini(struct personality_data * data)
{
        struct personality_data * tmp = data;
        int                       err;

        if (!data) {
                LOG_ERR("Personality data is bogus, cannot finalize "
                        "default personality");
                return -1;
        }

        LOG_DBG("Finalizing default personality");

        LOG_DBG("Finalizing kipcm");
        if (tmp->kipcm) {
                err = kipcm_destroy(tmp->kipcm);
                if (err) return err;
        }

        LOG_DBG("Finalizing nlset");
        if (tmp->nlset) {
                err = rnl_set_destroy(tmp->nlset);
                if (err) return err;
        }

        /* FIXME: To be removed */
        default_kipcm = NULL; /* Useless */

        LOG_DBG("Default personality finalized successfully");

        return 0;
}

static int default_init(struct kobject *          parent,
                        personality_id            id,
                        struct personality_data * data)
{
        if (!data) {
                LOG_ERR("Personality data is bogus, cannot initialize "
                        "default personality");
                return -1;
        }

        LOG_DBG("Initializing default personality");

        LOG_DBG("Initializing RNL");
        data->nlset = rnl_set_create(id);
        if (!data->nlset) {
                if (default_fini(data)) {
                        LOG_CRIT("The system might become unstable ...");
                        return -1;
                }
        }

        LOG_DBG("Initializing KIPCM");
        data->kipcm = kipcm_create(parent, data->nlset);
        if (!data->kipcm) {
                if (default_fini(data)) {
                        LOG_CRIT("The system might become unstable ...");
                        return -1;
                }
        }

        /* FIXME: To be removed */
        default_kipcm = data->kipcm;

        LOG_DBG("Default personality initialized successfully");

        return 0;
}

struct personality_ops ops = {
        .init            = default_init,
        .fini            = default_fini,
        .ipc_create      = default_ipc_create,
        .ipc_destroy     = default_ipc_destroy,
        .sdu_read        = default_sdu_read,
        .sdu_write       = default_sdu_write,
        .flow_create     = default_flow_create,
	.flow_opts_set   = default_flow_opts_set,
	.flow_opts       = default_flow_opts,
        .flow_destroy    = default_flow_destroy,
        .mgmt_sdu_read   = default_mgmt_sdu_read,
        .mgmt_sdu_write  = default_mgmt_sdu_write
};

static struct personality_data data;
static struct personality *    personality = NULL;

#ifdef CONFIG_RINA_RDS_REGRESSION_TESTS
extern bool regression_tests_rds(void);
#endif

static int __init mod_init(void)
{
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

        /* FIXME: This is not the right place, please fix */
#ifdef CONFIG_RINA_RDS_REGRESSION_TESTS
        if (!regression_tests_rds()) {
                rina_personality_unregister(personality);
                return -1;
        }
#endif

        LOG_DBG("Rina default personality loaded successfully");

        return 0;
}

static void __exit mod_exit(void)
{
        LOG_DBG("Rina default personality unloading");

        ASSERT(personality != NULL);

        if (rina_personality_unregister(personality)) {
                LOG_CRIT("Got problems while unregistering personality, "
                         "bailing out");
                return;
        }

        personality = NULL;

        LOG_DBG("Rina default personality unloaded successfully");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RINA default personality");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
