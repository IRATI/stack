/*
 *  Empty Shim IPC Process (Shim template that should be used as a reference)
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

#include <linux/module.h>

#include <linux/slab.h>
#include <linux/list.h>

#define RINA_PREFIX "shim-empty"

#include "logs.h"
#include "common.h"
#include "utils.h"
#include "kipcm.h"
#include "shim.h"

static int empty_flow_allocate_request(void *                     data,
                                       const struct name_t *      source,
                                       const struct name_t *      dest,
                                       const struct flow_spec_t * flow_spec,
                                       port_id_t *                id)
{
        LOG_FBEGN;
        LOG_FEXIT;

        return 0;
}

static int empty_flow_allocate_response(void *              data,
                                        port_id_t           id,
                                        response_reason_t * response)
{
        LOG_FBEGN;
        LOG_FEXIT;

        return 0;
}

static int empty_flow_deallocate(void *    data,
                                 port_id_t id)
{
        LOG_FBEGN;
        LOG_FEXIT;

        return 0;
}

static int empty_application_register(void *                data,
                                      const struct name_t * name)
{
        LOG_FBEGN;
        LOG_FEXIT;

        return 0;
}

static int empty_application_unregister(void *                data,
                                        const struct name_t * name)
{
        LOG_FBEGN;
        LOG_FEXIT;

        return 0;
}

static int empty_sdu_write(void *               data,
                           port_id_t            id,
                           const struct sdu_t * sdu)
{
        LOG_FBEGN;
        LOG_FEXIT;

        return 0;
}

static int empty_sdu_read(void *         data,
                          port_id_t      id,
                          struct sdu_t * sdu)
{
        LOG_FBEGN;
        LOG_FEXIT;

        return 0;
}

struct shim_instance_data {
        int this_is_another_fake_and_should_avoid_compiler_barfs;
};

static struct shim_instance_ops empty_instance_ops = {
        .flow_allocate_request  = empty_flow_allocate_request,
        .flow_allocate_response = empty_flow_allocate_response,
        .flow_deallocate        = empty_flow_deallocate,
        .application_register   = empty_application_register,
        .application_unregister = empty_application_unregister,
        .sdu_write              = empty_sdu_write,
        .sdu_read               = empty_sdu_read,
};

struct shim_data {
        int this_is_fake_and_should_avoid_compiler_barfs;
};

static struct shim_data empty_data;

static struct shim *    empty_shim = NULL;

static int empty_init(void * data)
{
        LOG_FBEGN;
        LOG_FEXIT;

        return 0;
}

static int empty_fini(void * data)
{
        LOG_FBEGN;
        LOG_FEXIT;

        return 0;
}

static struct shim_instance * empty_create(void *           data,
                                           ipc_process_id_t ipc_process_id)
{
        struct shim_instance * inst;

        LOG_FBEGN;

        inst = kzalloc(sizeof(*inst), GFP_KERNEL);
        if (!inst) {
                LOG_ERR("Cannot allocate %zd bytes of memory", sizeof(*inst));
                return NULL;
        }

        inst->ops  = &empty_instance_ops;
        inst->data = kzalloc(sizeof(struct shim_instance_data), GFP_KERNEL);
        if (!inst->data) {
                LOG_ERR("Cannot allocate %zd bytes of memory",
                        sizeof(*inst->data));
                return NULL;
        }

        LOG_FEXIT;

        return inst;
}

static struct shim_instance * empty_configure(void *                     data,
                                              struct shim_instance *     inst,
                                              const struct shim_config * cfg)
{
        LOG_FBEGN;
        LOG_FEXIT;

        return NULL;
}

static int empty_destroy(void *                   data,
                         struct shim_instance *   inst)
{
        LOG_FBEGN;
        LOG_FEXIT;

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
        LOG_FBEGN;

        bzero(&empty_data, sizeof(empty_data));

        empty_shim = kipcm_shim_register(default_kipcm,
                                         "shim-empty",
                                         &empty_data,
                                         &empty_ops);
        if (!empty_shim) {
                LOG_CRIT("Initialization failed");

                LOG_FEXIT;
                return -1;
        }

        LOG_FEXIT;

        return 0;
}

static void __exit mod_exit(void)
{
        LOG_FBEGN;

        if (kipcm_shim_unregister(default_kipcm, empty_shim)) {
                LOG_CRIT("Cannot unregister");
                return;
        }

        LOG_FEXIT;
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RINA Empty Shim IPC");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
