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

#define RINA_PREFIX "ipcp-factories"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "ipcp-factories.h"

struct ipcp_factories {
        struct rset * set;
};

#define to_ipcp(O) container_of(O, struct ipcp_factory, robj)

static bool ops_are_ok(const struct ipcp_factory_ops * ops)
{
        return (ops && ops->create && ops->destroy && ops->init && ops->fini) ?
                true : false;
}

/* NOTE: this seems not to be needed since factories name are already presented
 * in sysfs as the dir names
 * static ssize_t ipcp_factory_show(struct robject *   robj,
 *                                  struct attribute * attr,
 *                                  char *             buf)
 * { return sprintf(buf, "%s", robject_name(robj)); }
 */
RINA_SYSFS_OPS(ipcp_factory);
RINA_EMPTY_KTYPE(ipcp_factory);

struct ipcp_factories * ipcpf_init(struct robject * parent)
{
        struct ipcp_factories * temp;

        LOG_DBG("Initializing layer");

        temp = rkzalloc(sizeof(*temp), GFP_KERNEL);
        if (!temp)
                return NULL;

        temp->set = rset_create_and_add("ipcp-factories", parent);
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

        rset_unregister(factories->set);

        rkfree(factories);

        LOG_DBG("Layer finalized successfully");

        return 0;
}

static bool string_is_ok(const char * name)
{ return (name && strlen(name) != 0) ? true : false; }

struct ipcp_factory * ipcpf_find(struct ipcp_factories * factories,
                                 const char *            name)
{
        struct robject * k;

        if (!factories || !factories->set || !string_is_ok(name))
                return NULL;

        k = rset_find_obj(factories->set, name);
        if (k) {
		/* kobject_put is not needed since the struct factory is the one to be
		 * freed
                robject_put(k);
		*/
                return to_ipcp(k);
        }

        return NULL;
}

struct ipcp_factory * ipcpf_register(struct ipcp_factories *         factories,
                                     const char *                    name,
                                     struct ipcp_factory_data *      data,
                                     const struct ipcp_factory_ops * ops)
{
        struct ipcp_factory * factory;

        LOG_DBG("Registering new factory");

        if (!string_is_ok(name)) {
                LOG_ERR("Name is bogus, cannot register factory");
                return NULL;
        }

        if (!ops_are_ok(ops)) {
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
        if (robject_rset_init_and_add(&factory->robj, &ipcp_factory_rtype,
		factories->set, "%s", name)) {
                LOG_ERR("Cannot add factory '%s' to the set", name);
		/* kobject_put is not needed since the struct factory is the one to be
		 * freed
                robject_put(&factory->robj);
		*/

                rkfree(factory);
                return NULL;
        }

        if (factory->ops->init(factory->data)) {
                LOG_ERR("Cannot initialize factory '%s'", name);
		/* kobject_put is not needed since the struct factory is the one to be
		 * freed
                robject_put(&factory->robj);
		*/
                rkfree(factory);
                return NULL;
        }

        /* Double checking for bugs */
        LOG_INFO("Factory '%s' registered successfully",
                 robject_name(&factory->robj));

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

        name = robject_name(&factory->robj);
        ASSERT(string_is_ok(name));

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

        robject_del(&factory->robj);
	/* kobject_put is not needed since the struct factory is the one to be
	 * freed
        robject_put(&factory->robj);
	*/

        rkfree(factory);

        LOG_DBG("IPCP unregistered successfully");

        return 0;
}
