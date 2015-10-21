/*
 * Policy set publication/unpublication functions
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
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

#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/string.h>

#define RINA_PREFIX "ps-factory"

#include "logs.h"
#include "ps-factory.h"
#include "debug.h"

static int
default_set_policy_set_param(struct ps_base * bps,
			     const char    * name,
			     const char    * value)
{
	(void) bps;

        if (!name) {
                LOG_ERR("Null parameter name");
                return -1;
        }

        if (!value) {
                LOG_ERR("Null parameter value");
                return -1;
        }

        LOG_ERR("No such parameter to set");

        return -1;
}

int ps_publish(struct policy_set_list * list,
               struct ps_factory * factory)
{
        struct ps_factory * cur;

        if (!factory || !factory->name || !factory->owner ||
                !factory->create || !factory->destroy) {
                LOG_ERR("Wrong factory");
                return -1;
        }

        if (!list) {
                LOG_ERR("Null policy set list");
                return -1;
        }

        list_for_each_entry(cur, &list->head, node) {
                if (strcmp(factory->name, cur->name) == 0) {
                        LOG_ERR("%s: policy set '%s' already published",
                                __func__, factory->name);
                        return -1;
                }
        }

        list_add(&factory->node, &list->head);

        LOG_INFO("policy-set '%s' published successfully", factory->name);

        return 0;
}

struct ps_factory * ps_lookup(struct policy_set_list * list,
                              const char *             name)
{
        struct ps_factory * cur;

        if (name == NULL) {
                LOG_ERRF("NULL name");
                return NULL;
        }

        if (!list) {
                LOG_ERRF("Null policy set list");
                return NULL;
        }

        list_for_each_entry(cur, &list->head, node) {
                if (strcmp(name, cur->name) == 0)
                        return cur;
        }

        LOG_ERR("policy set '%s' has not been published", name);

        return NULL;
}

int ps_unpublish(struct policy_set_list * list, const char * name)
{
        struct ps_factory * factory;

        factory = ps_lookup(list, name);
        if (factory == NULL)
                return -1;

        /* Don't free here, the plug-in has the ownership. */
        list_del(&factory->node);

        LOG_INFO("policy-set '%s' unpublished successfully",
                 factory->name);

        return 0;
}

void rina_component_init(struct rina_component * comp)
{
        mutex_init(&comp->ps_lock);
        comp->ps = NULL;
        comp->ps_factory = NULL;
}

void base_select_policy_set_start(struct rina_component * comp,
                                  struct ps_select_transaction *trans,
                                  struct policy_set_list *list,
                                  const string_t * name)
{
        struct ps_factory *candidate_ps_factory;

        trans->state = PS_SEL_TRANS_ABORTED;
        trans->candidate_ps = NULL;
        trans->candidate_ps_factory = NULL;

        if (!name) {
                LOG_ERR("NULL name");
                return;
        }

        candidate_ps_factory = ps_lookup(list, name);
        if (!candidate_ps_factory) {
                LOG_ERR("No policy-set '%s' published", name);
                return;
        }

        /* Hold the mutex until transaction completes. */
        mutex_lock(&comp->ps_lock);

        if (candidate_ps_factory == comp->ps_factory) {
                trans->state = PS_SEL_TRANS_COMMITTED;
                LOG_INFO("Policy-set '%s' already selected", name);

                return;
        }

        /* Take a reference to the plugin module, to prevent
         * rmmodding. */
        if (!try_module_get(candidate_ps_factory->owner)) {
                LOG_ERR("Module %p is not alive as it should",
                        candidate_ps_factory->owner);
                return;
        }

        /* Instantiate the new policy set. */
        trans->candidate_ps = candidate_ps_factory->create(comp);
        if (!trans->candidate_ps) {
                module_put(candidate_ps_factory->owner);
                LOG_ERR("Policy-set instantiation failed");

                return;
        }

	/* Fill in default set_policy_set_param, if missing. */
	if (!trans->candidate_ps->set_policy_set_param) {
		trans->candidate_ps->set_policy_set_param =
					default_set_policy_set_param;
	}

        trans->state = PS_SEL_TRANS_PENDING;
        trans->candidate_ps_factory = candidate_ps_factory;
}

void base_select_policy_set_finish(struct rina_component * comp,
                                   struct ps_select_transaction * trans)
{
        struct ps_base *old_ps;

        if (trans->state != PS_SEL_TRANS_PENDING) {
                mutex_unlock(&comp->ps_lock);

                if (trans->state == PS_SEL_TRANS_COMMITTED) {
                        /* Transaction already committed during the first
                         * phase. */
                        BUG_ON(trans->candidate_ps_factory ||
                               trans->candidate_ps);
                        return;
                }

                /* Transaction aborted during the first phase or was aborted
                 * by the caller. Clean up if needed. */
                if (trans->candidate_ps) {
                        BUG_ON(!trans->candidate_ps_factory);
                        trans->candidate_ps_factory->
                                destroy(trans->candidate_ps);
                        trans->candidate_ps = NULL;
                        trans->candidate_ps_factory = NULL;
                }

                return;
        }

        BUG_ON(!trans->candidate_ps || !trans->candidate_ps_factory);

        /* Save old RCU-protected pointer. */
        old_ps = comp->ps;

        /* Removal old pointer and replace it with a new one. */
        rcu_assign_pointer(comp->ps, trans->candidate_ps);
        if (old_ps) {
                /* Wait for a grace period. */
                synchronize_rcu();
                ASSERT(comp->ps_factory);
                /* Reclaim the content pointed by the old pointer. */
                comp->ps_factory->destroy(old_ps);
                module_put(comp->ps_factory->owner);
        }

        comp->ps_factory = trans->candidate_ps_factory;

        mutex_unlock(&comp->ps_lock);

        trans->state = PS_SEL_TRANS_COMMITTED;

        return;
}

int base_set_policy_set_param(struct rina_component *comp,
                              const char * path,
                              const char * name,
                              const char * value)
{
        struct ps_base *ps;
        int ret = -1;

        mutex_lock(&comp->ps_lock);
        rcu_read_lock();
        ps = rcu_dereference(comp->ps);

        if (ps && ps->set_policy_set_param) {
                if (strcmp(path, comp->ps_factory->name) == 0) {
                        /* The request addresses the RMT policy set. */
                        ret = ps->set_policy_set_param(ps, name, value);
                } else {
                        LOG_ERR("Policy set %s not selected for this "
                                 "component", path);
                }
        } else {
                LOG_ERRF("No policy set selected");
        }

        rcu_read_unlock();
        mutex_unlock(&comp->ps_lock);

        return ret;
}

void rina_component_fini(struct rina_component * comp)
{
        struct ps_base *ps;

        mutex_lock(&comp->ps_lock);
        rcu_read_lock();
        ps = rcu_dereference(comp->ps);
        if (ps) {
                comp->ps_factory->destroy(ps);
                module_put(comp->ps_factory->owner);
        }
        rcu_read_unlock();
        mutex_unlock(&comp->ps_lock);
}

void ps_factory_parse_component_id(const string_t *path, size_t *cmplen,
                                   size_t *offset)
{
        const string_t *dot = strchr(path, '.');

        if (!dot) {
                /* Emulate strchrnul(). */
                dot = path + strlen(path);
        }

        *offset = *cmplen = dot - path;
        if (path[*offset] == '.') {
                (*offset)++;
        }
}
EXPORT_SYMBOL(ps_factory_parse_component_id);

void
ps_factory_nop_policy(void)
{
}
EXPORT_SYMBOL(ps_factory_nop_policy);

