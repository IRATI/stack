/*
 *  Shim IPC Processes layer
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

#define RINA_PREFIX "shims"

#include "logs.h"
#include "utils.h"
#include "shim.h"
#include "kipcm.h"
#include "debug.h"

static ssize_t shim_show(struct kobject *   kobj,
                         struct attribute * attr,
                         char *             buf)
{ return sprintf(buf, "%s", kobject_name(kobj)); }

static const struct sysfs_ops shim_sysfs_ops = {
        .show = shim_show
};

static struct kobj_type shim_ktype = {
        .sysfs_ops     = &shim_sysfs_ops,
        .default_attrs = NULL,
        .release       = NULL,
};

struct shims * shims_init(struct kobject * parent)
{
        struct shims * temp;

        LOG_DBG("Initializing shims layer");

        temp = rkzalloc(sizeof(*temp), GFP_KERNEL);
        if (!temp)
                return NULL;

        temp->set = kset_create_and_add("shims", NULL, parent);
        if (!temp->set) {
                LOG_ERR("Cannot initialize shims layer support");
                return NULL;
        }

        ASSERT(temp->set != NULL);

        LOG_DBG("Shims layer initialized successfully");

        return temp;
}

int shims_fini(struct shims * shims)
{
        LOG_DBG("Finalizing shims layer");

        if (!shims) {
                LOG_ERR("Bogus shims, cannot finalize");
                return -1;
        }

        /* All the shims have to be unregistered from now on */
        ASSERT(list_empty(&shims->set->list));

        kset_unregister(shims->set);

        rkfree(shims);

        LOG_DBG("Shims layer finalized successfully");

        return 0;
}

/*
 * NOTE:
 *
 *   The shims API might change regardless the core version. The core API is
 *   the northboud API which separates RINA from the user-space. The shim API
 *   is the southbound one which separate the RINA stack from the shims.
 *
 *     Francesco
 */
static uint32_t version = MK_RINA_VERSION(0, 0, 4);

uint32_t shims_version(void)
{ return version; }

static int is_name_ok(const char * name)
{
        LOG_DBG("Checking name");

        if (!name) {
                LOG_DBG("Name is empty");
                return 0;
        }
        if (strlen(name) == 0) {
                LOG_DBG("Name has 0 length");
                return 0;
        }

        LOG_DBG("Name is ok");

        return 1;
}

static int are_ops_ok(const struct shim_ops * ops)
{
        LOG_DBG("Checking ops %pK", ops);

        if (!ops) {
                LOG_ERR("Ops are empty");
                return 0;
        }

        if (!(ops->create    &&
              ops->configure &&
              ops->destroy)) {
                LOG_DBG("Ops are bogus");
                return 0;
        }

        LOG_DBG("Ops are ok");

        return 1;
}

#define to_shim(O) container_of(O, struct shim, kobj)

static struct shim * shim_find(struct shims * parent,
                               const char *   name)
{
        struct kobject * k;

        ASSERT(parent);
        ASSERT(parent->set);
        ASSERT(name);

        k = kset_find_obj(parent->set, name);
        if (k) {
                kobject_put(k);
                return to_shim(k);
        }

        return NULL;
}

struct shim * shim_register(struct shims *          parent,
                            const char *            name,
                            struct shim_data *      data,
                            const struct shim_ops * ops)
{
        struct shim * tmp;
        struct shim * shim;

        if (!is_name_ok(name)) {
                LOG_ERR("Name is bogus, cannot register shim");
                return NULL;
        }

        if (!are_ops_ok(ops)) {
                LOG_ERR("Cannot register shim '%s', ops are bogus", name);
                return NULL;
        }

        if (!parent) {
                LOG_ERR("Bogus parent, cannot register shim '%s", name);
                return NULL;
        }

        LOG_DBG("Registering shim '%s'", name);

        tmp = shim_find(parent, name);
        if (tmp) {
                LOG_ERR("Shim '%s' already registered", name);
                return NULL;
        }

        shim = rkzalloc(sizeof(*shim), GFP_KERNEL);
        if (!shim)
                return NULL;

        shim->data = data;
        shim->ops  = ops;

        shim->kobj.kset = parent->set;
        if (kobject_init_and_add(&shim->kobj, &shim_ktype, NULL,
                                 "%s", name)) {
                LOG_ERR("Cannot add shim '%s' to the set", name);
                kobject_put(&shim->kobj);
                /*
                 * FIXME: To be removed once shim_ktype.release
                 * gets implemented
                 */
                rkfree(shim);
                return NULL;
        }

        if (shim->ops->init(shim->data)) {
        	LOG_ERR("Cannot initialize shim '%s'", name);
        	kobject_put(&shim->kobj);
		/*
		 * FIXME: To be removed once shim_ktype.release
		 * gets implemented
		 */
		rkfree(shim);
        	return NULL;
        }

        /* Double checking for bugs */
        name = kobject_name(&shim->kobj);

        LOG_INFO("Shim '%s' registered successfully", name);

        return shim;
}

int shim_unregister(struct shims * parent,
                    struct shim *  shim)
{
        struct shim * tmp;
        const char *  name;

        if (!parent) {
                LOG_ERR("Bogus parent, cannot unregister shim %pK", shim);
                return -1;
        }

        if (!shim) {
                LOG_ERR("Bogus shim, cannot unregister");
                return -1;
        }

        name = kobject_name(&shim->kobj);

        ASSERT(name);
        ASSERT(is_name_ok(name));

        LOG_DBG("Unregistering shim '%s'", name);

        ASSERT(parent);

        tmp = shim_find(parent, name);
        if (!tmp) {
                LOG_ERR("Shim '%s' not registered", name);
                return -1;
        }

        kobject_put(&shim->kobj);

        if (shim->ops->fini(shim->data)) {
		LOG_ERR("Cannot finalize shim '%s'", name);
	}

        rkfree(shim); /* FIXME: To be removed */

        LOG_DBG("Shim unregistered successfully");

        return 0;
}
