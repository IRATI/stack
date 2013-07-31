/*
 *  IPC Process Factories
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

#include <linux/kobject.h>

#define RINA_PREFIX "ipcp-factories"

#include "logs.h"
#include "ipcp.h"
#include "ipcp-factories.h"
#include "utils.h"
#include "debug.h"

struct ipcp_factories {
        struct kset * set;
};

#define to_ipcp(O) container_of(O, struct ipcp_factory, kobj)

static int are_ops_ok(const struct ipcp_factory_ops * ops)
{
        LOG_DBG("Checking ops");

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

static ssize_t factory_show(struct kobject *   kobj,
                            struct attribute * attr,
                            char *             buf)
{ return sprintf(buf, "%s", kobject_name(kobj)); }

static const struct sysfs_ops ipcp_factory_sysfs_ops = {
        .show = factory_show
};

static struct kobj_type ipcp_factory_ktype = {
        .sysfs_ops     = &ipcp_factory_sysfs_ops,
        .default_attrs = NULL,
        .release       = NULL,
};

struct ipcp_factories * ipcpf_init(struct kobject * parent)
{
        struct ipcp_factories * temp;

        LOG_DBG("Initializing layer");

        temp = rkzalloc(sizeof(*temp), GFP_KERNEL);
        if (!temp)
                return NULL;

        temp->set = kset_create_and_add("ipcp-factories", NULL, parent);
        if (!temp->set) {
                LOG_ERR("Cannot initialize layer");
                return NULL;
        }

        ASSERT(temp->set != NULL);

        LOG_DBG("Layer initialized successfully");

        return temp;
}

int ipcpf_fini(struct ipcp_factories * factories)
{
        LOG_DBG("Finalizing layer");

        if (!factories) {
                LOG_ERR("Bogus parameter, cannot finalize");
                return -1;
        }

        /* All thetemplates have to be unregistered from now on */
        ASSERT(list_empty(&factories->set->list));

        kset_unregister(factories->set);

        rkfree(factories);

        LOG_DBG("Layer finalized successfully");

        return 0;
}

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

struct ipcp_factory * ipcpf_find(struct ipcp_factories * factories,
                                 const char *            name)
{
        struct kobject * k;

        if (!factories || !factories->set || !name || !is_name_ok(name))
                return NULL;

        k = kset_find_obj(factories->set, name);
        if (k) {
                kobject_put(k);
                return to_ipcp(k);
        }

        return NULL;
}

struct ipcp_factory * ipcpf_register(struct ipcp_factories *          factories,
                                     const char *                     name,
                                     struct ipcp_factory_data *       data,
                                     const struct ipcp_factory_ops *  ops)
{
        struct ipcp_factory * factory;

        LOG_DBG("Registering new factory");

        if (!is_name_ok(name)) {
                LOG_ERR("Name is bogus, cannot register factory");
                return NULL;
        }

        if (!are_ops_ok(ops)) {
                LOG_ERR("Cannot register factory '%s', ops are bogus", name);
                return NULL;
        }

        if (!factories) {
                LOG_ERR("Bogus parent, cannot register factory '%s", name);
                return NULL;
        }

        factory = ipcpf_find(factories, name);
        if (factory) {
                LOG_ERR("Factory '%s' already registered", name);
                return NULL;
        }

        LOG_DBG("Registering factory '%s'", name);

        factory = rkzalloc(sizeof(*factory), GFP_KERNEL);
        if (!factory)
                return NULL;

        factory->data      = data;
        factory->ops       = ops;
        factory->kobj.kset = factories->set;
        if (kobject_init_and_add(&factory->kobj, &ipcp_factory_ktype, NULL,
                                 "%s", name)) {
                LOG_ERR("Cannot add factory '%s' to the set", name);
                kobject_put(&factory->kobj);

                rkfree(factory);
                return NULL;
        }

        if (factory->ops->init(factory->data)) {
                LOG_ERR("Cannot initialize factory '%s'", name);
                kobject_put(&factory->kobj);
                rkfree(factory);
                return NULL;
        }

        /* Double checking for bugs */
        LOG_INFO("Factory '%s' registered successfully",
                 kobject_name(&factory->kobj));

        return factory;
}

int ipcpf_unregister(struct ipcp_factories * factories,
                     struct ipcp_factory *   factory)
{
        struct ipcp_factory * tmp;
        const char *          name;

        if (!factories) {
                LOG_ERR("Bogus parent, cannot unregister factory");
                return -1;
        }

        if (!factory) {
                LOG_ERR("Bogus factory, cannot unregister");
                return -1;
        }

        name = kobject_name(&factory->kobj);

        ASSERT(name);
        ASSERT(is_name_ok(name));

        LOG_DBG("Unregistering factory '%s'", name);

        ASSERT(factories);

        tmp = ipcpf_find(factories, name);
        if (!tmp) {
                LOG_ERR("Cannot find factory '%s'", name);
                return -1;
        }

        ASSERT(tmp == factory);

        if (tmp->ops->fini(factory->data)) {
                LOG_ERR("Cannot finalize factory '%s'", name);
        }

        kobject_del(&factory->kobj);
        kobject_put(&factory->kobj);

        rkfree(factory);

        LOG_DBG("IPCP unregistered successfully");

        return 0;
}
