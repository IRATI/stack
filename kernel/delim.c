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

static struct policy_set_list delim_policy_sets = {
	.head = LIST_HEAD_INIT(delim_policy_sets.head)
};

struct delim * delim_from_component(struct rina_component *component)
{ return container_of(component, struct delim, base); }
EXPORT_SYMBOL(delim_from_component);

struct rmt *tmp;

struct delim * delim_create(struct efcp * efcp, struct robject * parent)
{
	struct delim * delim;

	if (!parent || !efcp) {
		LOG_ERR("Bogus input parameters");
		return NULL;
	}

	delim = rkzalloc(sizeof(*tmp), GFP_KERNEL);
	if (!delim)
		return NULL;

	delim->efcp = efcp;
	rina_component_init(&delim->base);

	if (robject_init_and_add(&tmp->robj, &rmt_rtype, parent, "delim")) {
                LOG_ERR("Failed to create Delimiting sysfs entry");
                rmt_destroy(tmp);
                return NULL;
	}
}

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
                if (!ps->delim_fragment) {
                        LOG_ERR("Delimiting policy set is invalid, policies are "
                                "missing:\n"
                                "       fragment=%p\n"
                                ps->delim_fragment);
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
        struct sdup_crypto_ps * ps;
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
                                  struct sdup_crypto_ps, base);
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
