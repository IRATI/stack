/*
 *  Shim IPC Process
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
#include <linux/string.h>
#include <linux/kobject.h>
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
        LOG_DBG("Initializing shim layer");

        if (shims) {
                LOG_ERR("Shim layer already initialized");
                return -1;
        }

#if CONFIG_RINA_SYSFS
        /* FIXME: Move the set path from kernel to rina */
        shims = kset_create_and_add("shims", NULL, kernel_kobj);
        if (!shims) {
                LOG_ERR("Cannot intialize shim layer sysfs support");
                return -1;
        }
#endif

        ASSERT(shims != NULL);

        LOG_DBG("Shim layer initialized successfully");

        return 0;
}

void shim_exit(void)
{
        LOG_DBG("Finalizing shim layer");

        ASSERT(shims != NULL);
#if CONFIG_RINA_SYSFS
        kset_unregister(shims);
#endif
        shims = NULL;

        LOG_DBG("Shim layer finalized successfully");
}

static int is_label_ok(const char * label)
{
        LOG_DBG("Checking label");

        if (!label) {
                LOG_DBG("Label is empty");
                return 0;
        }
        if (strlen(label) == 0) {
                LOG_DBG("Label has 0 length");
                return 0;
        }

        LOG_DBG("Label is ok");

        return 1;
}

static int is_ok(const struct shim_t * shim)
{
        LOG_DBG("Checking shim %pK consistence", shim);

        if (!shim) {
                LOG_ERR("Shim is NULL");
                return 0;
        }

        if (!is_label_ok(shim->label)) {
                LOG_ERR("Shim %pK has bogus label", shim);
                return 0;
        }

        if (shim->create             &&
            shim->configure          &&
            shim->destroy) {
                LOG_DBG("Shim '%s' is ok", shim->label);
                return 1;
        }

        LOG_ERR("Shim '%s' has bogus hooks", shim->label);

        return 0;
}

int shim_register(struct shim_t * shim)
{
        int err;

        LOG_DBG("Registering shim %pK", shim);

        if (!is_ok(shim)) {
                LOG_ERR("Cannot register shim %pK, it's bogus", shim);
                return -1;
        }

        err = kipcm_shim_register(shim);
        if (err)
                return err;

        LOG_INFO("Shim '%s' registered successfully", shim->label);

        return 0;
}
EXPORT_SYMBOL(shim_register);

int shim_unregister(struct shim_t * shim)
{
        int err;

        LOG_DBG("Un-registering shim %pK", shim);

        if (!shim) {
                LOG_ERR("Cannot unregister shim %pK, it's bogus", shim);
                return -1;
        }

        err = kipcm_shim_unregister(shim);
                return err;

        LOG_INFO("Shim '%s' unregistered successfully", shim->label);

        return 0;
}
EXPORT_SYMBOL(shim_unregister);
