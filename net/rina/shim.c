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

#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/slab.h>

#define RINA_PREFIX "shim"

#include "logs.h"
#include "utils.h"
#include "shim.h"
#include "kipcm.h"

static struct kset * shims = NULL;

int shim_init(void)
{
        if (shims) {
                LOG_ERR("Shim layer already initialized");
                return -1;
        }

#if CONFIG_RINA_SYSFS
        /* FIXME: Move the set path from kernel to rina */
        shims = kset_create_and_add("shims", NULL, kernel_kobj);
        if (!shims)
                return -1;
#endif

        ASSERT(shims != NULL);
        return 0;
}

void shim_exit(void)
{
        ASSERT(shims != NULL);
#if CONFIG_RINA_SYSFS
        kset_unregister(shims);
#endif
        shims = NULL;
}

static int is_shim_label_ok(const struct shim_t * shim)
{ return (shim ? strlen(shim->label) : 0); }

static int is_shim_ok(const struct shim_t * shim)
{
        LOG_DBG("Checking shim %pK consistence", shim);

        if (shim                   &&
            shim->label            &&
            is_shim_label_ok(shim) &&
            shim->create           &&
            shim->configure        &&
	    shim->destroy) {
                LOG_DBG("Shim %pK is consistent", shim);
                return 1;
        }

        LOG_ERR("Shim %pK is inconsistent", shim);
        return 0;
}

int shim_register(struct shim_t * shim)
{
        struct shim_object * obj;

        LOG_DBG("Registering shim %pK", shim);

        if (!shim || !is_shim_ok(shim)) {
                LOG_ERR("Cannot register shim %pK, it's bogus", shim);
                return -1;
        }

        ASSERT(is_shim_label_ok(shim));

        if (kipcm_shim_register(shim))
                return -1;

#if CONFIG_RINA_SYSFS
#if 0
        obj = kzalloc(sizeof(*obj), GFP_KERNEL);
        if (!obj) {
                LOG_CRIT("Cannot allocate %d bytes of memory", sizeof(*obj));
                return -ENOMEM;
        }

        LDBG("Setting up kobj for label '%s'", shim->label);
        obj->kobj.kset = shims;
        if (!kobject_init_and_add(&obj->kobj, &obj_ktype, NULL, "%s",
                                  shim->label)) {
                LOG_CRIT("Cannot setup sysfs for shim %pK", shim);
                return -1;
        }

        kobject_put(&obj->kobj);
#endif
#endif

        return 0;
}

int shim_unregister(struct shim_t * shim)
{
        LOG_DBG("Un-registering shim %pK", shim);

        if (!shim) {
                LOG_ERR("Cannot unregister shim, it's bogus");
                return -1;
        }

        if (kipcm_shim_unregister(shim))
                return -1;

#if CONFIG_RINA_SYSFS
#endif

        return 0;
}
