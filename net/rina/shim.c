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
#include "kipcm.h"

int  shim_init(void)
{ return 0; }

void shim_exit(void)
{ }

static int is_shim_ok(const struct shim_t * shim)
{
        LOG_DBG("Checking shim %pK consistence", shim);

        if (shim            &&
            shim->create    &&
            shim->configure &&
	    shim->destroy) {
                LOG_DBG("Shim %pK is consistent", shim);
                return 1;
        }

        LOG_ERR("Shim %pK is inconsistent", shim);
        return 0;
}

int shim_register(struct shim_t * shim)
{
        if (!shim || !is_shim_ok(shim)) {
                LOG_ERR("Cannot register shim, it's bogus");
                return -1;
        }

        LOG_DBG("Registering shim %pK", shim);
        return kipcm_shim_register(shim);
}

int shim_unregister(struct shim_t * shim)
{
        if (!shim) {
                LOG_ERR("Cannot unregister shim, it's bogus");
                return -1;
        }

        LOG_DBG("Un-registering shim %pK", shim);
        return kipcm_shim_unregister(shim);
}
