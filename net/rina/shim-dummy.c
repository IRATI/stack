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

#include "logs.h"
#include "common.h"
#include "shim.h"

int dummy_create(ipc_process_id_t      ipc_process_id,
		 const struct name_t * name)
{
	LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}

int dummy_destroy(ipc_process_id_t      ipc_process_id,
		 const struct name_t * name)
{
	LOG_FBEGN;
        LOG_FEXIT;

	return 0;
}

int dummy_configure(ipc_process_id_t          ipc_process_id,
                      const struct shim_conf_t * configuration)
{
	LOG_FBEGN;
	LOG_FEXIT;

	return 0;
}

static int __init mod_init(void)
{
        LOG_FBEGN;
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
