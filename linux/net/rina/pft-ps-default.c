/*
 * Default policy set for PFT
 *
 *    Vincenzo Maffione <v.maffione@nextworks.it>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Sander Vrijders <sander.vrijders@intec.ugent.be>
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
#include <linux/export.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/random.h>

#define RINA_PREFIX "pft-ps-default"

#include "logs.h"
#include "rds/rmem.h"
#include "rds/rtimer.h"
#include "pft-ps.h"
#include "debug.h"

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
/* NOTE: Unused at the moment */
 ps = rkzalloc(sizeof(*ps), GFP_KERNEL);
        if (!ps) {
                return NULL;
        }
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

/* FIXME: This representation is crappy and MUST be changed */
struct pft_entry {
        address_t        destination;
        qos_id_t         qos_id;
        struct list_head ports;
        struct list_head next;
};

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
/* NOTE: Unused at the moment */
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

static int pfte_ports_copy(struct pft_entry * entry,
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

struct pft_ps_dm {
        struct mutex     write_lock;
        struct list_head entries;
};

static bool dm_is_ok(struct pft_ps_dm * dm)
{ return dm != NULL; }

static struct pft_entry * pft_find(struct pft_ps_dm * dm,
                                   address_t          destination,
                                   qos_id_t           qos_id)
{
        struct pft_entry * pos;

        ASSERT(dm_is_ok(dm));
        ASSERT(is_address_ok(destination));

        list_for_each_entry_rcu(pos, &dm->entries, next) {
                if ((pos->destination == destination) &&
                    (pos->qos_id      == qos_id)) {
                        return pos;
                }
        }

        return NULL;
}

static int default_add(struct pft_ps *          ps,
                       struct modpdufwd_entry * entry)
{
        struct pft_ps_dm *       dm;
        struct pft_entry *       tmp;
	struct port_id_altlist * alts;

        dm = (struct pft_ps_dm *) ps->dm;
        if (!dm_is_ok(dm))
                return -1;

        if (!entry) {
                LOG_ERR("Bogus output parameters, won't add");
                return -1;
        }

        if (!is_address_ok(entry->fwd_info)) {
                LOG_ERR("Bogus destination address passed, cannot add");
                return -1;
        }
        if (!is_qos_id_ok(entry->qos_id)) {
                LOG_ERR("Bogus qos-id passed, cannot add");
                return -1;
        }

        mutex_lock(&dm->write_lock);

        tmp = pft_find(dm, entry->fwd_info, entry->qos_id);
        if (!tmp) {
                tmp = pfte_create_ni(entry->fwd_info, entry->qos_id);
                if (!tmp) {
                        mutex_unlock(&dm->write_lock);
                        return -1;
                }

                list_add_rcu(&tmp->next, &dm->entries);
        }

	list_for_each_entry(alts, &entry->port_id_altlists, next) {
		if (alts->num_ports < 1) {
			LOG_INFO("Port id alternative set is empty");
			continue;
		}

		/* Just add the first alternative and ignore the others. */
                if (pfte_port_add(tmp, alts->ports[0])) {
                        pfte_destroy(tmp);
                        mutex_unlock(&dm->write_lock);
                        return -1;
                }
	}

        mutex_unlock(&dm->write_lock);

        return 0;
}

static int default_remove(struct pft_ps *          ps,
                          struct modpdufwd_entry * entry)
{
        struct pft_ps_dm *       dm;
        struct port_id_altlist * alts;
        struct pft_entry *       tmp;

        dm = (struct pft_ps_dm *) ps->dm;
        if (!dm_is_ok(dm))
                return -1;

        if (!entry) {
                LOG_ERR("Bogus output parameters, won't remove");
                return -1;
        }

        if (!is_address_ok(entry->fwd_info)) {
                LOG_ERR("Bogus destination address passed, cannot remove");
                return -1;
        }
        if (!is_qos_id_ok(entry->qos_id)) {
                LOG_ERR("Bogus qos-id passed, cannot remove");
                return -1;
        }

        mutex_lock(&dm->write_lock);

        tmp = pft_find(dm, entry->fwd_info, entry->qos_id);
        if (!tmp) {
                mutex_unlock(&dm->write_lock);
                return -1;
        }

	list_for_each_entry(alts, &entry->port_id_altlists, next) {
		if (alts->num_ports < 1) {
			LOG_INFO("Port id alternative set is empty");
			continue;
		}

		/* Just remove the first alternative and ignore the others. */
                pfte_port_remove(tmp, alts->ports[0]);
	}

        /* If the list of port-ids is empty, remove the entry */
        if (list_empty(&tmp->ports)) {
                pfte_destroy(tmp);
        }

        mutex_unlock(&dm->write_lock);

        return 0;
}

static bool default_is_empty(struct pft_ps * ps)
{
        struct pft_ps_dm * dm;
        bool               empty;

        dm = (struct pft_ps_dm *) ps->dm;
        if (!dm_is_ok(dm))
                return false;

        rcu_read_lock();
        empty = list_empty(&dm->entries);
        rcu_read_unlock();

        return empty;
}

static void __pft_flush(struct pft_ps_dm * dm)
{
        struct pft_entry * pos, * next;

        ASSERT(dm_is_ok(dm));

        list_for_each_entry_safe(pos, next, &dm->entries, next) {
                pfte_destroy(pos);
        }
}

static int default_flush(struct pft_ps * ps)
{
        struct pft_ps_dm * dm;

        dm = (struct pft_ps_dm *) ps->dm;
        if (!dm_is_ok(dm))
                return -1;

        mutex_lock(&dm->write_lock);

        __pft_flush(dm);

        mutex_unlock(&dm->write_lock);

        return 0;
}

static int default_nhop(struct pft_ps * ps,
                        struct pci *    pci,
                        port_id_t **    ports,
                        size_t *        count)
{
        struct pft_ps_dm * dm;
        address_t          destination;
        qos_id_t           qos_id;
        struct pft_entry * tmp;

        dm = (struct pft_ps_dm *) ps->dm;
        if (!dm_is_ok(dm)) {
                return -1;
        }

        destination = pci_destination(pci);
        if (!is_address_ok(destination)) {
                LOG_ERR("Bogus destination address, cannot get NHOP");
                return -1;
        }

        qos_id = pci_qos_id(pci);
        if (!is_qos_id_ok(qos_id)) {
                LOG_ERR("Bogus qos-id, cannot get NHOP");
                return -1;
        }

        if (!ports || !count) {
                LOG_ERR("Bogus output parameters, won't get NHOP");
                return -1;
        }

        /*
         * Taking the lock here since otherwise dm might be deleted when
         * copying the ports
         */
        rcu_read_lock();

        tmp = pft_find(dm, destination, qos_id);
        if (!tmp) {
                LOG_ERR("Could not find any entry for dest address: %u and "
                        "qos_id %d", destination, qos_id);
                rcu_read_unlock();
                return -1;
        }

        if (pfte_ports_copy(tmp, ports, count)) {
                rcu_read_unlock();
                return -1;
        }

        rcu_read_unlock();

        return 0;
}

static int pfte_port_id_altlists_copy(struct pft_entry * entry,
                                      struct list_head * port_id_altlists)
{
        struct pft_port_entry * pos;

        ASSERT(pfte_is_ok(entry));

        list_for_each_entry_rcu(pos, &entry->ports, next) {
		struct port_id_altlist * alt;
		int cnt = 1;

		alt = rkmalloc(sizeof(*alt), GFP_ATOMIC);
		if (!alt) {
			return -1;
		}

		alt->ports = rkmalloc(cnt * sizeof(*(alt->ports)), GFP_ATOMIC);
		if (!alt->ports) {
			return -1;
		}

		alt->ports[0] = pft_pe_port(pos);
		alt->num_ports = cnt;

		list_add_tail(&alt->next, port_id_altlists);
        }

        return 0;
}

static int default_dump(struct pft_ps *    ps,
                        struct list_head * entries)
{
        struct pft_ps_dm *       dm;
        struct pft_entry *       pos;
        struct modpdufwd_entry * entry;

        dm = (struct pft_ps_dm *) ps->dm;
        if (!dm_is_ok(dm))
                return -1;

        rcu_read_lock();
        list_for_each_entry_rcu(pos, &dm->entries, next) {
                entry = rkmalloc(sizeof(*entry), GFP_ATOMIC);
                if (!entry) {
                        rcu_read_unlock();
                        return -1;
                }

                entry->fwd_info = pos->destination;
                entry->qos_id = pos->qos_id;
		INIT_LIST_HEAD(&entry->port_id_altlists);
                if (pfte_port_id_altlists_copy(pos, &entry->port_id_altlists)) {
                        rkfree(entry);
                        rcu_read_unlock();
                        return -1;
                }

                list_add(&entry->next, entries);
        }
        rcu_read_unlock();

        return 0;
}

/* NOTE: This is skeleton code that was directly copy pasted */
static int pft_ps_set_policy_set_param(struct ps_base * bps,
                                       const char *     name,
                                       const char *     value)
{
        struct pft_ps *ps = container_of(bps, struct pft_ps, base);

        (void) ps;

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

static struct ps_base *
pft_ps_default_create(struct rina_component * component)
{
        struct pft_ps * ps;
        struct pft_ps_dm * dm;
        struct pft * pft = pft_from_component(component);

        dm = rkzalloc(sizeof(*dm), GFP_KERNEL);
        if (!dm) {
                return NULL;
        }

        mutex_init(&dm->write_lock);

        INIT_LIST_HEAD(&dm->entries);

        ps = rkzalloc(sizeof(*ps), GFP_KERNEL);
        if (!ps) {
                return NULL;
        }

        ps->base.set_policy_set_param = pft_ps_set_policy_set_param;
        ps->pft = pft;
        ps->dm = (void *) dm;

        ps->pft_add = default_add;
        ps->pft_remove = default_remove;
        ps->pft_is_empty = default_is_empty;
        ps->pft_flush = default_flush;
        ps->pft_nhop = default_nhop;
        ps->pft_dump = default_dump;

        return &ps->base;
}

static void pft_ps_default_destroy(struct ps_base * bps)
{
        struct pft_ps * ps = container_of(bps, struct pft_ps, base);

        if (bps) {
                struct pft_ps_dm * dm;

                dm = (struct pft_ps_dm *) ps->dm;
                if(!dm_is_ok(dm)) {
                        return;
                }

                mutex_lock(&dm->write_lock);

                __pft_flush(dm);

                mutex_unlock(&dm->write_lock);
                rkfree(ps);
        }
}

struct ps_factory pft_factory = {
        .owner   = THIS_MODULE,
        .create  = pft_ps_default_create,
        .destroy = pft_ps_default_destroy,
};
