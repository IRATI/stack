/*
 * Delimiting
 *
 *    Eduard Grasa    <eduard.grasa@i2cat.net>
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

#include <linux/types.h>
#include <linux/list.h>
#include <linux/string.h>

#define RINA_PREFIX "delim"

#include "logs.h"
#include "delim.h"
#include "debug.h"
#include "policies.h"
#include "sdup.h"
#include "delim-ps.h"
#include "irati/kucommon.h"
#include "rds/rmem.h"

static struct policy_set_list delim_policy_sets = {
	.head = LIST_HEAD_INIT(delim_policy_sets.head)
};

struct delim * delim_from_component(struct rina_component *component)
{ return container_of(component, struct delim, base); }
EXPORT_SYMBOL(delim_from_component);

static ssize_t delim_attr_show(struct robject *        robj,
                               struct robj_attribute * attr,
                               char *                  buf)
{
	struct delim * delim;

	delim = container_of(robj, struct delim, robj);
	if (!delim || !delim->base.ps)
		return 0;

	if (strcmp(robject_attr_name(attr), "ps_name") == 0) {
		return sprintf(buf, "%s\n", delim->base.ps_factory->name);
	}

	if (strcmp(robject_attr_name(attr), "max_fragment") == 0) {
		return sprintf(buf, "%u\n", delim->max_fragment_size);
	}

	return 0;
}

RINA_SYSFS_OPS(delim);
RINA_ATTRS(delim, ps_name, max_fragment);
RINA_KTYPE(delim);

struct delim * delim_create(struct efcp * efcp, struct robject * parent)
{
	struct delim * delim;

	if (!parent || !efcp) {
		LOG_ERR("Bogus input parameters");
		return NULL;
	}

	delim = rkzalloc(sizeof(*delim), GFP_KERNEL);
	if (!delim)
		return NULL;

	delim->efcp = efcp;
	delim->max_fragment_size = 0;
	rina_component_init(&delim->base);
	delim->tx_dus = NULL;
	delim->rx_dus = NULL;

	if (robject_init_and_add(&delim->robj, &delim_rtype, parent, "delim")) {
                LOG_ERR("Failed to create Delimiting sysfs entry");
                delim_destroy(delim);
                return NULL;
	}

	delim->tx_dus = du_list_create();
	if (!delim->tx_dus) {
		LOG_ERR("Failed to create list of DUs to tx");
		delim_destroy(delim);
		return NULL;
	}

	delim->rx_dus = du_list_create();
	if (!delim->rx_dus) {
		LOG_ERR("Failed to create list of DUs to rx");
		delim_destroy(delim);
		return NULL;
	}

	return delim;
}
EXPORT_SYMBOL(delim_create);

int delim_destroy(struct delim * delim)
{
	if (!delim) {
		LOG_ERR("Bogus instance passed, bailing out");
		return -1;
	}

	robject_del(&delim->robj);

	rina_component_fini(&delim->base);

	if (delim->tx_dus) {
		du_list_destroy(delim->tx_dus, true);
	}

	if (delim->rx_dus) {
		du_list_destroy(delim->rx_dus, true);
	}

	rkfree(delim);

	LOG_DBG("Instance %pK finalized successfully", delim);

	return 0;
}
EXPORT_SYMBOL(delim_destroy);

int delim_select_policy_set(struct delim * delim,
                            const string_t *  path,
                            const string_t *  name)
{
        struct ps_select_transaction trans;

        BUG_ON(!path);

        if (strcmp(path, "")) {
                LOG_ERR("This component has no selectable subcomponents");
                return -1;
        }

        base_select_policy_set_start(&delim->base,
        			     &trans,
        			     &delim_policy_sets,
        			     name);

        if (trans.state == PS_SEL_TRANS_PENDING) {
                struct delim_ps *ps;

                ps = container_of(trans.candidate_ps, struct delim_ps, base);
                if (!ps->delim_fragment  || !ps->delim_process_udf) {
                        LOG_ERR("Delimiting policy set is invalid, policies are "
                                "missing:\n"
                                "       fragment=%p\n"
                                "       process_udf=%p\n",
                                ps->delim_fragment,
                                ps->delim_process_udf);
                        trans.state = PS_SEL_TRANS_ABORTED;
                }
        }

        base_select_policy_set_finish(&delim->base, &trans);

        return trans.state = PS_SEL_TRANS_COMMITTED ? 0 : -1;
}
EXPORT_SYMBOL(delim_select_policy_set);

int delim_set_policy_set_param(struct delim * delim,
			       const char * path,
			       const char * name,
			       const char * value)
{
        struct delim_ps * ps;
        int ret = -1;

        if (!delim || !path || !name || !value) {
                LOG_ERRF("NULL arguments %p %p %p %p", delim, path, name, value);
                return -1;
        }

        LOG_DBG("set-policy-set-param '%s' '%s' '%s'", path, name, value);

        if (strcmp(path, "") == 0) {
                /* The request addresses this PFF instance. */
                rcu_read_lock();

                ps = container_of(rcu_dereference(delim->base.ps),
                                  struct delim_ps, base);
                if (!ps) {
                        LOG_ERR("No policy-set selected for this Delimiting component");
                } else {
                        LOG_ERR("Unknown Delimiting parameter policy '%s'", name);
                }

                rcu_read_unlock();
        } else {
                ret = base_set_policy_set_param(&delim->base, path, name, value);
        }

        return ret;
}
EXPORT_SYMBOL(delim_set_policy_set_param);

int delim_ps_publish(struct ps_factory * factory)
{
	if (factory == NULL) {
		LOG_ERR("%s: NULL factory", __func__);
		return -1;
	}

	return ps_publish(&delim_policy_sets, factory);
}
EXPORT_SYMBOL(delim_ps_publish);

int delim_ps_unpublish(const char * name)
{ return ps_unpublish(&delim_policy_sets, name); }
EXPORT_SYMBOL(delim_ps_unpublish);
