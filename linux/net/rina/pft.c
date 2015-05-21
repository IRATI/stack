/*
 * PFT (PDU Forwarding Table)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
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

/* FIXME: This representation is crappy and MUST be changed */
struct pft_port_entry {
        port_id_t        port_id;

        struct list_head next;
};

static struct pft_port_entry * pft_pe_create_gfp(gfp_t     flags,
                                                 port_id_t port_id)
{
        struct pft_port_entry * tmp;

        ASSERT(is_port_id_ok(port_id));

        tmp = rkmalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->port_id = port_id;
        INIT_LIST_HEAD(&tmp->next);

        return tmp;
}

static struct pft_port_entry * pft_pe_create_ni(port_id_t port_id)
{ return pft_pe_create_gfp(GFP_ATOMIC, port_id); }

#if 0
static struct pft_port_entry * pft_pe_create(port_id_t port_id)
{ return pft_pe_create_gfp(GFP_KERNEL, port_id); }
#endif

/* FIXME: This thing is bogus and has to be fixed properly */
#ifdef CONFIG_RINA_ASSERTIONS
static bool pft_pe_is_ok(struct pft_port_entry * pe)
{ return pe ? true : false;  }
#endif

static void pft_pe_destroy(struct pft_port_entry * pe)
{
        ASSERT(pft_pe_is_ok(pe));

        list_del_rcu(&pe->next);
        synchronize_rcu();
        rkfree(pe);
}

static port_id_t pft_pe_port(struct pft_port_entry * pe)
{
        ASSERT(pft_pe_is_ok(pe));

        return pe->port_id;
}

static struct pft_entry * pfte_create_gfp(gfp_t     flags,
                                          address_t destination,
                                          qos_id_t  qos_id)
{
        struct pft_entry * tmp;

        tmp = rkmalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->destination = destination;
        tmp->qos_id      = qos_id;
        INIT_LIST_HEAD(&tmp->ports);
        INIT_LIST_HEAD(&tmp->next);

        return tmp;
}

static struct pft_entry * pfte_create_ni(address_t destination,
                                         qos_id_t  qos_id)
{ return pfte_create_gfp(GFP_ATOMIC, destination, qos_id); }

#if 0
static struct pft_entry * pfte_create(address_t destination,
                                      qos_id_t  qos_id)
{ return pfte_create_gfp(GFP_KERNEL, destination, qos_id); }
#endif

/* FIXME: This thing is bogus and has to be fixed properly */
#ifdef CONFIG_RINA_ASSERTIONS
static bool pfte_is_ok(struct pft_entry * entry)
{ return entry ? true : false; }
#endif

static void pfte_destroy(struct pft_entry * entry)
{
        struct pft_port_entry * pos, * next;

        ASSERT(pfte_is_ok(entry));

        list_for_each_entry_safe(pos, next, &entry->ports, next) {
                pft_pe_destroy(pos);
        }

        list_del_rcu(&entry->next);
        synchronize_rcu();
        rkfree(entry);
}

static struct pft_port_entry * pfte_port_find(struct pft_entry * entry,
                                              port_id_t          id)
{
        struct pft_port_entry * pos;

        ASSERT(pfte_is_ok(entry));

        list_for_each_entry_rcu(pos, &entry->ports, next) {
                if (pos->port_id == id)
                        return pos;
        }

        return NULL;
}

static int pfte_port_add(struct pft_entry * entry,
                         port_id_t          id)
{
        struct pft_port_entry * pe;

        ASSERT(pfte_is_ok(entry));

        pe = pfte_port_find(entry, id);
        if (pe)
                return 0;

        pe = pft_pe_create_ni(id);
        if (!pe)
                return -1;

        list_add_rcu(&pe->next, &entry->ports);

        return 0;
}

static void pfte_port_remove(struct pft_entry * entry,
                             port_id_t          id)
{
        struct pft_port_entry * pos, * next;

        ASSERT(pfte_is_ok(entry));
        ASSERT(is_port_id_ok(id));

        list_for_each_entry_safe(pos, next, &entry->ports, next) {
                if (pft_pe_port(pos) == id) {
                        pft_pe_destroy(pos);
                        return;
                }
        }

}

int pfte_ports_copy(struct pft_entry * entry,
                    port_id_t **       port_ids,
                    size_t *           entries)
{
        struct pft_port_entry * pos;
        size_t                  count;
        int                     i;

        ASSERT(pfte_is_ok(entry));

        count = 0;
        list_for_each_entry_rcu(pos, &entry->ports, next) {
                count++;
        }

        ASSERT(entries);

        if (*entries != count) {
                if (*entries > 0)
                        rkfree(*port_ids);
                if (count > 0) {
                        *port_ids = rkmalloc(count * sizeof(**port_ids),
                                             GFP_ATOMIC);
                        if (!*port_ids) {
                                *entries = 0;
                                return -1;
                        }
                }
                *entries = count;
        }

        /* Get the first port, and so on, fill in the port_ids */
        i = 0;
        list_for_each_entry_rcu(pos, &entry->ports, next) {
                (*port_ids)[i++] = pft_pe_port(pos);
        }

        return 0;
}
EXPORT_SYMBOL(pfte_ports_copy);

/* FIXME: This representation is crappy and MUST be changed */
struct pft {
        struct mutex          write_lock;
        struct list_head      entries;
        struct rina_component base;
};


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

/* Must be called under RCU read lock. */
struct pft_ps *
pft_ps_get(struct pft * pft)
{
        return container_of(rcu_dereference(pft->base.ps),
                            struct pft_ps, base);
}

struct pft *
pft_from_component(struct rina_component * component)
{
        return container_of(component, struct pft, base);
}
EXPORT_SYMBOL(pft_from_component);

static struct pft * pft_create_gfp(gfp_t flags)
{
        struct pft * tmp;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        mutex_init(&tmp->write_lock);

        INIT_LIST_HEAD(&tmp->entries);

        rina_component_init(&tmp->base);

        /* Try to select the default policy-set. */
        if (pft_select_policy_set(tmp, "", RINA_PS_DEFAULT_NAME)) {
                pft_destroy(tmp);
                return NULL;
        }

        return tmp;
}

#if 0
struct pft * pft_create_ni(void)
{ return pft_create_gfp(GFP_ATOMIC); }
#endif

struct pft * pft_create(void)
{ return pft_create_gfp(GFP_KERNEL); }

static bool __pft_is_ok(struct pft * instance)
{ return instance ? true : false; }

/*
 * NOTE: This could break if we do more checks on the instance.
 *       A lock will have to be be taken in that case ...
 */
bool pft_is_ok(struct pft * instance)
{ return __pft_is_ok(instance); }
EXPORT_SYMBOL(pft_is_ok);

bool pft_is_empty(struct pft * instance)
{
        bool empty;

        if (!__pft_is_ok(instance))
                return false;

        rcu_read_lock();
        empty = list_empty(&instance->entries);
        rcu_read_unlock();

        return empty;
}

static void __pft_flush(struct pft * instance)
{
        struct pft_entry * pos, * next;

        ASSERT(__pft_is_ok(instance));

        list_for_each_entry_safe(pos, next, &instance->entries, next) {
                pfte_destroy(pos);
        }
}

int pft_flush(struct pft * instance)
{
        if (!__pft_is_ok(instance))
                return -1;

        mutex_lock(&instance->write_lock);

        __pft_flush(instance);

        mutex_unlock(&instance->write_lock);

        return 0;
}

int pft_destroy(struct pft * instance)
{
        if (!__pft_is_ok(instance))
                return -1;

        mutex_lock(&instance->write_lock);

        __pft_flush(instance);

        mutex_unlock(&instance->write_lock);

        rina_component_fini(&instance->base);

        rkfree(instance);

        return 0;
}

struct pft_entry * pft_find(struct pft * instance,
                            address_t    destination,
                            qos_id_t     qos_id)
{
        struct pft_entry * pos;

        ASSERT(__pft_is_ok(instance));
        ASSERT(is_address_ok(destination));

        list_for_each_entry_rcu(pos, &instance->entries, next) {
                if ((pos->destination == destination) &&
                    (pos->qos_id      == qos_id)) {
                        return pos;
                }
        }

        return NULL;
}
EXPORT_SYMBOL(pft_find);

int pft_add(struct pft *      instance,
            address_t         destination,
            qos_id_t          qos_id,
            const port_id_t * ports,
            size_t            count)
{
        struct pft_entry * tmp;
        int                i;

        if (!__pft_is_ok(instance))
                return -1;

        if (!is_address_ok(destination)) {
                LOG_ERR("Bogus destination address passed, cannot add");
                return -1;
        }
        if (!is_qos_id_ok(qos_id)) {
                LOG_ERR("Bogus qos-id passed, cannot add");
                return -1;
        }
        if (!ports || !count) {
                LOG_ERR("Bogus output parameters, won't add");
                return -1;
        }

        mutex_lock(&instance->write_lock);

        tmp = pft_find(instance, destination, qos_id);
        if (!tmp) {
                tmp = pfte_create_ni(destination, qos_id);
                if (!tmp) {
                        mutex_unlock(&instance->write_lock);
                        return -1;
                }

                list_add_rcu(&tmp->next, &instance->entries);
        }

        for (i = 0; i < count; i++) {
                if (pfte_port_add(tmp, ports[i])) {
                        pfte_destroy(tmp);
                        mutex_unlock(&instance->write_lock);
                        return -1;
                }
        }

        mutex_unlock(&instance->write_lock);

        return 0;
}

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
                ps = container_of(rcu_dereference(pft->base.ps), struct pft_ps, base);
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

int pft_remove(struct pft *      instance,
               address_t         destination,
               qos_id_t          qos_id,
               const port_id_t * ports,
               size_t            count)
{
        struct pft_entry * tmp;
        int                i;

        if (!__pft_is_ok(instance))
                return -1;

        if (!is_address_ok(destination)) {
                LOG_ERR("Bogus destination address, cannot remove");
                return -1;
        }
        if (!is_qos_id_ok(qos_id)) {
                LOG_ERR("Bogus qos-id, cannot remove");
                return -1;
        }
        if (!ports || !count) {
                LOG_ERR("Bogus output parameters, won't remove");
                return -1;
        }

        mutex_lock(&instance->write_lock);

        tmp = pft_find(instance, destination, qos_id);
        if (!tmp) {
                mutex_unlock(&instance->write_lock);
                return -1;
        }

        for (i = 0; i < count; i++)
                pfte_port_remove(tmp, ports[i]);

        /* If the list of port-ids is empty, remove the entry */
        if (list_empty(&tmp->ports)) {
                pfte_destroy(tmp);
        }

        mutex_unlock(&instance->write_lock);

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

        ASSERT(ps->next_hop);
        if (ps->next_hop(ps, pci, ports, count)) {
                rcu_read_unlock();
                return -1;
        }
        rcu_read_unlock();

        return 0;
}

int pft_dump(struct pft *       instance,
             struct list_head * entries)
{
        struct pft_entry *    pos;
        struct pdu_ft_entry * entry;

        if (!__pft_is_ok(instance))
                return -1;

        rcu_read_lock();
        list_for_each_entry_rcu(pos, &instance->entries, next) {
                entry = rkmalloc(sizeof(*entry), GFP_ATOMIC);
                if (!entry) {
                        rcu_read_unlock();
                        return -1;
                }

                entry->destination = pos->destination;
                entry->qos_id      = pos->qos_id;
                entry->ports_size  = 0;
                entry->ports       = NULL;
                if (pfte_ports_copy(pos, &entry->ports, &entry->ports_size)) {
                        rkfree(entry);
                        rcu_read_unlock();
                        return -1;
                }

                list_add(&entry->next, entries);
        }
        rcu_read_unlock();

        return 0;
}

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
