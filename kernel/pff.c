/*
 * PFF (PDU Forwarding Function)
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

#define RINA_PREFIX "pff"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "pff.h"
#include "pff-ps.h"

static struct policy_set_list policy_sets = {
        .head = LIST_HEAD_INIT(policy_sets.head)
};

struct pff {
        struct rina_component base;
        struct rset * rset;
        struct ipcp_instance * ipcp;
};

/*
static ssize_t pff_attr_show(struct robject *        robj,
                             struct robj_attribute * attr,
                             char *                  buf)
{
	struct pff * pff;

	 = container_of(robj, struct rmt, robj);
	if (!rmt || !rmt->rmt_cfg || !rmt->rmt_cfg->policy_set)
		return 0;

	if (strcmp(robject_attr_name(attr), "ps_name") == 0) {
		return sprintf(buf, "%s\n",
			policy_name(rmt->rmt_cfg->policy_set));
	}
	if (strcmp(robject_attr_name(attr), "ps_name") == 0) {
		return sprintf(buf, "%s\n",
			policy_version(rmt->rmt_cfg->policy_set));
	}
	return 0;
}
RINA_SYSFS_OPS(pff);
RINA_ATTRS(pff, ps_name, ps_version);
RINA_KTYPE(pff);
*/

static bool __pff_is_ok(struct pff * instance)
{ return instance ? true : false; }

static struct pff * pff_create_gfp(struct robject * parent,
				   struct ipcp_instance * ipcp, gfp_t flags)
{
        struct pff * tmp;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->rset = NULL;
        rina_component_init(&tmp->base);

	tmp->rset = rset_create_and_add("pff", parent);
	if (!tmp->rset) {
                LOG_ERR("Failed to create PFF sysfs entry");
                pff_destroy(tmp);
                return NULL;
	}

	tmp->ipcp = ipcp;

        /* Try to select the default policy-set. */
        if (pff_select_policy_set(tmp, "", RINA_PS_DEFAULT_NAME)) {
                pff_destroy(tmp);
                return NULL;
        }

        return tmp;
}

#if 0
/* NOTE: Unused at the moment */
struct pff * pff_create_ni(void)
{ return pff_create_gfp(GFP_ATOMIC); }
#endif

struct pff * pff_create(struct robject * parent,
			struct ipcp_instance * ipcp)
{ return pff_create_gfp(parent, ipcp, GFP_KERNEL); }

int pff_destroy(struct pff * instance)
{
        if (!__pff_is_ok(instance))
                return -1;

	if (instance->rset)
		rset_unregister(instance->rset);

        rina_component_fini(&instance->base);

        rkfree(instance);

        return 0;
}

/*
 * NOTE: This could break if we do more checks on the instance.
 *       A lock will have to be be taken in that case ...
 */
bool pff_is_ok(struct pff * instance)
{ return __pff_is_ok(instance); }
EXPORT_SYMBOL(pff_is_ok);

bool pff_is_empty(struct pff * instance)
{
        struct pff_ps * ps;

        if (!__pff_is_ok(instance))
                return -1;

        rcu_read_lock();

        ps = container_of(rcu_dereference(instance->base.ps),
                          struct pff_ps, base);

        ASSERT(ps->pff_is_empty);
        if (ps->pff_is_empty(ps)) {
                rcu_read_unlock();
                return -1;
        }

        rcu_read_unlock();

        return 0;
}

int pff_flush(struct pff * instance)
{
        struct pff_ps * ps;

        if (!__pff_is_ok(instance))
                return -1;

        rcu_read_lock();

        ps = container_of(rcu_dereference(instance->base.ps),
                          struct pff_ps, base);

        ASSERT(ps->pff_flush);
        if (ps->pff_flush(ps)) {
                rcu_read_unlock();
                return -1;
        }

        rcu_read_unlock();

        return 0;
}

int pff_add(struct pff *           instance,
	    struct mod_pff_entry * mpe)
{
        struct pff_ps * ps;

        if (!__pff_is_ok(instance))
                return -1;

        if (!mpe) {
                LOG_ERR("Bogus output parameters, won't remove");
                return -1;
        }

        rcu_read_lock();

        ps = container_of(rcu_dereference(instance->base.ps),
                          struct pff_ps, base);

        ASSERT(ps->pff_add);
        if (ps->pff_add(ps, mpe)) {
                rcu_read_unlock();
                return -1;
        }

        rcu_read_unlock();

        return 0;
}

int pff_remove(struct pff *           instance,
	       struct mod_pff_entry * mpe)
{
        struct pff_ps * ps;

        if (!__pff_is_ok(instance))
                return -1;

        if (!mpe) {
                LOG_ERR("Bogus output parameters, won't remove");
                return -1;
        }

        rcu_read_lock();

        ps = container_of(rcu_dereference(instance->base.ps),
                          struct pff_ps, base);

        ASSERT(ps->pff_remove);
        if (ps->pff_remove(ps, mpe)) {
                rcu_read_unlock();
                return -1;
        }

        rcu_read_unlock();

        return 0;
}

int pff_port_state_change(struct pff *	pff,
			  port_id_t	port_id,
			  bool		up)
{
        struct pff_ps * ps;

        if (!__pff_is_ok(pff))
                return -1;

        rcu_read_lock();

        ps = container_of(rcu_dereference(pff->base.ps),
                          struct pff_ps, base);

        if (ps->pff_port_state_change &&
			ps->pff_port_state_change(ps, port_id, up)) {
                rcu_read_unlock();
                return -1;
        }

        rcu_read_unlock();

        return 0;
}

int pff_nhop(struct pff * instance,
             struct pci * pci,
             port_id_t ** ports,
             size_t *     count)
{
        struct pff_ps *ps;

        if (!__pff_is_ok(instance))
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
                          struct pff_ps, base);

        ASSERT(ps->pff_nhop);
        if (ps->pff_nhop(ps, pci, ports, count)) {
                rcu_read_unlock();
                return -1;
        }
        rcu_read_unlock();

        return 0;
}

int pff_dump(struct pff *       instance,
             struct list_head * entries)
{
        struct pff_ps * ps;

        if (!__pff_is_ok(instance))
                return -1;

        rcu_read_lock();

        ps = container_of(rcu_dereference(instance->base.ps),
                          struct pff_ps, base);

        ASSERT(ps->pff_dump);
        if (ps->pff_dump(ps, entries)) {
                rcu_read_unlock();
                return -1;
        }

        rcu_read_unlock();

        return 0;
}

int pff_modify(struct pff *       instance,
               struct list_head * entries)
{
        struct pff_ps * ps;

        if (!__pff_is_ok(instance))
                return -1;

        rcu_read_lock();

        ps = container_of(rcu_dereference(instance->base.ps),
                          struct pff_ps, base);

        ASSERT(ps->pff_modify);
        if (ps->pff_modify(ps, entries)) {
                rcu_read_unlock();
                return -1;
        }

        rcu_read_unlock();

        return 0;
}

int pff_select_policy_set(struct pff *     pff,
                          const string_t * path,
                          const string_t * name)
{
        struct ps_select_transaction trans;

        BUG_ON(!path);

        if (strcmp(path, "")) {
                LOG_ERR("This component has no selectable subcomponents");
                return -1;
        }

        base_select_policy_set_start(&pff->base, &trans, &policy_sets, name);

        if (trans.state == PS_SEL_TRANS_PENDING) {
                struct pff_ps *ps;

                ps = container_of(trans.candidate_ps, struct pff_ps, base);
                if (!ps->pff_add || !ps->pff_remove || !ps->pff_is_empty ||
                        !ps->pff_flush || !ps->pff_nhop || !ps->pff_dump) {
                        LOG_ERR("PFF policy set is invalid, policies are "
                                "missing:\n"
                                "       pff_add=%p\n"
                                "       pff_remove=%p\n"
                                "       pff_is_empty=%p\n"
                                "       pff_flush=%p\n"
                                "       pff_nhop=%p\n"
                                "       pff_dump=%p\n",
                                ps->pff_add, ps->pff_remove, ps->pff_is_empty,
                                ps->pff_flush, ps->pff_nhop, ps->pff_dump);
                        trans.state = PS_SEL_TRANS_ABORTED;
                }
        }

        base_select_policy_set_finish(&pff->base, &trans);

        return trans.state = PS_SEL_TRANS_COMMITTED ? 0 : -1;
}
EXPORT_SYMBOL(pff_select_policy_set);

/* NOTE: Skeleton code, PFF currently does not take params */
int pff_set_policy_set_param(struct pff * pff,
                             const char * path,
                             const char * name,
                             const char * value)
{
        struct pff_ps * ps;
        int ret = -1;

        if (!pff || !path || !name || !value) {
                LOG_ERRF("NULL arguments %p %p %p %p", pff, path, name, value);
                return -1;
        }

        LOG_DBG("set-policy-set-param '%s' '%s' '%s'", path, name, value);

        if (strcmp(path, "") == 0) {
                /* The request addresses this PFF instance. */
                rcu_read_lock();

                ps = container_of(rcu_dereference(pff->base.ps),
                                  struct pff_ps, base);
                if (!ps) {
                        LOG_ERR("No policy-set selected for this PFF");
                } else {
                        LOG_ERR("Unknown PFF parameter policy '%s'", name);
		}

                rcu_read_unlock();
        } else {
                ret = base_set_policy_set_param(&pff->base, path, name, value);
	}

        return ret;
}
EXPORT_SYMBOL(pff_set_policy_set_param);

/* Must be called under RCU read lock. */
struct pff_ps * pff_ps_get(struct pff * pff)
{
        return container_of(rcu_dereference(pff->base.ps),
                            struct pff_ps, base);
}

struct rset * pff_rset(struct pff * pff)
{ return pff->rset; }
EXPORT_SYMBOL(pff_rset);

struct ipcp_instance * pff_ipcp_get(struct pff * pff)
{
	return pff->ipcp;
}
EXPORT_SYMBOL(pff_ipcp_get);

struct pff * pff_from_component(struct rina_component * component)
{ return container_of(component, struct pff, base); }
EXPORT_SYMBOL(pff_from_component);

int pff_ps_publish(struct ps_factory * factory)
{
        if (factory == NULL) {
                LOG_ERR("%s: NULL factory", __func__);
                return -1;
        }

        return ps_publish(&policy_sets, factory);
}
EXPORT_SYMBOL(pff_ps_publish);

int pff_ps_unpublish(const char * name)
{ return ps_unpublish(&policy_sets, name); }
EXPORT_SYMBOL(pff_ps_unpublish);
