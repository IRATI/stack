/*
 *  Dummy Shim IPC Process
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

#define RINA_PREFIX "shim-dummy"

#include <linux/slab.h>
#include "logs.h"
#include "common.h"
#include "shim.h"

struct dummy_info_t {
	uint16_t        dummy_id;
	char *      	name;
};

struct dummy_instance_t {
	ipc_process_id_t      ipc_process_id;
	struct name_t *       name;
	struct dummy_info_t * info;
	/* FIXME: Stores the state of flows indexed by port_id */
};

struct shim_instance_t * dummy_create(ipc_process_id_t ipc_process_id)
{
	struct shim_instance_t * shim_dummy;

	LOG_FBEGN;

	shim_dummy = kmalloc(sizeof(*shim_dummy), GFP_KERNEL);
	if (!shim_dummy) {
		LOG_ERR("Cannot allocate memory");
		LOG_FEXIT;
		return NULL;
	}

        LOG_FEXIT;

	return shim_dummy;
}

int dummy_destroy(struct shim_instance_t * inst)
{
	LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}

struct shim_instance_t * dummy_configure(struct shim_instance_t *   instance,
					 const struct shim_conf_t *
					 configuration)
{
	LOG_FBEGN;
	LOG_FEXIT;

	return NULL;
}

static int __init mod_init(void)
{
        struct shim_t * shim;

	LOG_FBEGN;

	shim = kmalloc(sizeof(*shim), GFP_KERNEL);
	if (!shim) {
		LOG_ERR("Cannot allocate %zu bytes of memory", sizeof(*shim));
		LOG_FEXIT;
		return -1;
	}
	shim->label = "shim-dummy";
	shim->create = dummy_create;
	shim->destroy = dummy_destroy;
	shim->configure = dummy_configure;
	if(shim_register(shim)){
		LOG_ERR("Initialization of module shim-dummy failed");
		LOG_FEXIT;
		return -1;
	}

        LOG_FEXIT;

        return 0;
}

static void __exit mod_exit(void)
{
        LOG_FBEGN;
        LOG_FEXIT;
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RINA Dummy Shim IPC");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
MODULE_AUTHOR("Miquel Tarzan <miquel.tarzan@i2cat.net>");
MODULE_AUTHOR("Sander Vrijders <sander.vrijders@intec.ugent.be>");
