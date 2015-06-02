/*
 * PFT (PDU Forwarding Table)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#include <linux/rculist.h>
#include <linux/slab.h>

#define RINA_PREFIX "pft"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "pft.h"
#include "pft-ps.h"

static struct policy_set_list policy_sets = {
        .head = LIST_HEAD_INIT(policy_sets.head)
};

struct pft {
        struct rina_component base;
};

static bool __pft_is_ok(struct pft * instance)
{ return instance ? true : false; }

static struct pft * pft_create_gfp(gfp_t flags)
{
        struct pft * tmp;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        rina_component_init(&tmp->base);

        /* Try to select the default policy-set. */
        if (pft_select_policy_set(tmp, "", RINA_PS_DEFAULT_NAME)) {
                pft_destroy(tmp);
                return NULL;
        }

        return tmp;
}

#if 0
/* NOTE: Unused at the moment */
struct pft * pft_create_ni(void)
{ return pft_create_gfp(GFP_ATOMIC); }
#endif

struct pft * pft_create(void)
{ return pft_create_gfp(GFP_KERNEL); }

int pft_destroy(struct pft * instance)
{
        if (!__pft_is_ok(instance))
                return -1;

        rina_component_fini(&instance->base);

        rkfree(instance);

        return 0;
}

/*
 * NOTE: This could break if we do more checks on the instance.
 *       A lock will have to be be taken in that case ...
 */
bool pft_is_ok(struct pft * instance)
{ return __pft_is_ok(instance); }
EXPORT_SYMBOL(pft_is_ok);

bool pft_is_empty(struct pft * instance)
{
        struct pft_ps * ps;

        if (!__pft_is_ok(instance))
                return -1;

        rcu_read_lock();

        ps = container_of(rcu_dereference(instance->base.ps),
                          struct pft_ps, base);

        ASSERT(ps->pft_is_empty);
        if (ps->pft_is_empty(ps)) {
                rcu_read_unlock();
                return -1;
        }

        rcu_read_unlock();

        return 0;
}

int pft_flush(struct pft * instance)
{
        struct pft_ps * ps;

        if (!__pft_is_ok(instance))
                return -1;

        rcu_read_lock();

        ps = container_of(rcu_dereference(instance->base.ps),
                          struct pft_ps, base);

        ASSERT(ps->pft_flush);
        if (ps->pft_flush(ps)) {
                rcu_read_unlock();
                return -1;
        }

        rcu_read_unlock();

        return 0;
}

int pft_add(struct pft * instance,
	    struct modpdufwd_entry * mpfe)
{
        struct pft_ps * ps;

        if (!__pft_is_ok(instance))
                return -1;

        if (!mpfe) {
                LOG_ERR("Bogus output parameters, won't remove");
                return -1;
        }

        rcu_read_lock();

        ps = container_of(rcu_dereference(instance->base.ps),
                          struct pft_ps, base);

        ASSERT(ps->pft_add);
        if (ps->pft_add(ps, mpfe)) {
                rcu_read_unlock();
                return -1;
        }

        rcu_read_unlock();

        return 0;
}

int pft_remove(struct pft * instance,
	       struct modpdufwd_entry * mpfe)
{
        struct pft_ps * ps;

        if (!__pft_is_ok(instance))
                return -1;

        if (!mpfe) {
                LOG_ERR("Bogus output parameters, won't remove");
                return -1;
        }

        rcu_read_lock();

        ps = container_of(rcu_dereference(instance->base.ps),
                          struct pft_ps, base);

        ASSERT(ps->pft_remove);
        if (ps->pft_remove(ps, mpfe)) {
                rcu_read_unlock();
                return -1;
        }

        rcu_read_unlock();

        return 0;
}

int pft_nhop(struct pft * instance,
             struct pci * pci,
             port_id_t ** ports,
             size_t *     count)
{
        struct pft_ps *ps;

        if (!__pft_is_ok(instance))
                return -1;

        if (!pci_is_ok(pci)) {
                LOG_ERR("Bogus PCI, cannot get NHOP");
                return -1;
        }
        if (!ports || !count) {
                LOG_ERR("Bogus output parameters, won't get NHOP");
                return -1;
        }

        rcu_read_lock();

        ps = container_of(rcu_dereference(instance->base.ps),
                          struct pft_ps, base);

        ASSERT(ps->pft_nhop);
        if (ps->pft_nhop(ps, pci, ports, count)) {
                rcu_read_unlock();
                return -1;
        }
        rcu_read_unlock();

        return 0;
}

int pft_dump(struct pft *       instance,
             struct list_head * entries)
{
        struct pft_ps * ps;

        if (!__pft_is_ok(instance))
                return -1;

        rcu_read_lock();

        ps = container_of(rcu_dereference(instance->base.ps),
                          struct pft_ps, base);

        ASSERT(ps->pft_dump);
        if (ps->pft_dump(ps, entries)) {
                rcu_read_unlock();
                return -1;
        }

        rcu_read_unlock();

        return 0;
}

/* NOTE: Skeleton code, PFT currently has no subcomponents*/
int pft_select_policy_set(struct pft * pft,
                          const string_t * path,
                          const string_t * name)
{
        int ret;

        if (path && strcmp(path, "")) {
                LOG_ERR("This component has no selectable subcomponents");
                return -1;
        }

        ret = base_select_policy_set(&pft->base, &policy_sets, name);

        return ret;
}
EXPORT_SYMBOL(pft_select_policy_set);

/* NOTE: Skeleton code, PFT currently does not take params */
int pft_set_policy_set_param(struct pft * pft,
                             const char * path,
                             const char * name,
                             const char * value)
{
        struct pft_ps * ps;
        int ret = -1;

        if (!pft || !path || !name || !value) {
                LOG_ERRF("NULL arguments %p %p %p %p", pft, path, name, value);
                return -1;
        }

        LOG_DBG("set-policy-set-param '%s' '%s' '%s'", path, name, value);

        if (strcmp(path, "") == 0) {
                /* The request addresses this PFT instance. */
                rcu_read_lock();

                ps = container_of(rcu_dereference(pft->base.ps),
                                  struct pft_ps, base);
                if (!ps) {
                        LOG_ERR("No policy-set selected for this PFT");
                } else {
                        LOG_ERR("Unknown PFT parameter policy '%s'", name);
                }

                rcu_read_unlock();
        } else {
                ret = base_set_policy_set_param(&pft->base, path, name, value);
        }

        return ret;
}
EXPORT_SYMBOL(pft_set_policy_set_param);

/* Must be called under RCU read lock. */
struct pft_ps * pft_ps_get(struct pft * pft)
{
        return container_of(rcu_dereference(pft->base.ps),
                            struct pft_ps, base);
}

struct pft * pft_from_component(struct rina_component * component)
{ return container_of(component, struct pft, base); }
EXPORT_SYMBOL(pft_from_component);

int pft_ps_publish(struct ps_factory * factory)
{
        if (factory == NULL) {
                LOG_ERR("%s: NULL factory", __func__);
                return -1;
        }

        return ps_publish(&policy_sets, factory);
}
EXPORT_SYMBOL(pft_ps_publish);

int pft_ps_unpublish(const char * name)
{ return ps_unpublish(&policy_sets, name); }
EXPORT_SYMBOL(pft_ps_unpublish);
