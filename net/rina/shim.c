/*
 *  Shim IPC Process
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
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

#define RINA_PREFIX "shim"

#include "logs.h"
#include "utils.h"
#include "shim.h"

int  shim_init(void)
{ return 0; }

void shim_exit(void)
{ }

static int is_ok(const struct shim_t * shim)
{
        ASSERT(shim);

        if (shim->init                    &&
            shim->exit                    &&

            shim->ipc_create              &&
            shim->ipc_configure           &&
	    shim->ipc_reconfigure         &&
            shim->ipc_destroy)
                return 1;

        return 0;
}

#if 0

/* FIXME: Should probably be moved to kipcm.h, to check if the shim instance can be used */

static int instance_is_ok(const struct shim_instance_t * shim)
{
        ASSERT(shim);

        if (shim->flow_allocate_request   &&
            shim->flow_allocate_response  &&
            shim->flow_deallocate         &&

            shim->application_register    &&
            shim->application_unregister  &&

            shim->sdu_read                &&
            shim->sdu_write)
                return 1;

        return 0;
}

#endif


int shim_register(struct shim_t * shim)
{
        LOG_DBG("Registering shim %pK", shim);

        if (!shim || !is_ok(shim)) {
                LOG_ERR("Cannot register shim, it's bogus");
                return -1;
        }

        /* FIXME: Call the KIPCM */

        return 0;
}

int shim_unregister(struct shim_t * shim)
{
        LOG_DBG("Un-registering shim %pK", shim);

        /* FIXME: Call the KIPCM */

        return 0;
}

