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
	    struct pdu_ft_entry *entry)
{
        struct pft_entry * tmp;
	struct port_id_altlist * alts;

        if (!__pft_is_ok(instance))
                return -1;

        if (!entry) {
                LOG_ERR("Bogus output parameters, won't add");
                return -1;
        }

        if (!is_address_ok(entry->destination)) {
                LOG_ERR("Bogus destination address passed, cannot add");
                return -1;
        }
        if (!is_qos_id_ok(entry->qos_id)) {
                LOG_ERR("Bogus qos-id passed, cannot add");
                return -1;
        }

        mutex_lock(&instance->write_lock);

        tmp = pft_find(instance, entry->destination, entry->qos_id);
        if (!tmp) {
                tmp = pfte_create_ni(entry->destination, entry->qos_id);
                if (!tmp) {
                        mutex_unlock(&instance->write_lock);
                        return -1;
                }

                list_add_rcu(&tmp->next, &instance->entries);
        }

	list_for_each_entry(alts, &entry->port_id_altlists, next) {
		if (alts->num_ports < 1) {
			LOG_INFO("Port id alternative set is empty");
			continue;
		}

		/* Just add the first alternative and ignore the others. */
                if (pfte_port_add(tmp, alts->ports[0])) {
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
	       struct pdu_ft_entry * entry)
/*
               address_t         destination,
               qos_id_t          qos_id,
               const port_id_t * ports,
               size_t            count)*/
{
	struct port_id_altlist * alts;
        struct pft_entry * tmp;

        if (!__pft_is_ok(instance))
                return -1;

        if (!entry) {
                LOG_ERR("Bogus output parameters, won't add");
                return -1;
        }

        if (!is_address_ok(entry->destination)) {
                LOG_ERR("Bogus destination address passed, cannot add");
                return -1;
        }
        if (!is_qos_id_ok(entry->qos_id)) {
                LOG_ERR("Bogus qos-id passed, cannot add");
                return -1;
        }

        mutex_lock(&instance->write_lock);

        tmp = pft_find(instance, entry->destination, entry->qos_id);
        if (!tmp) {
                mutex_unlock(&instance->write_lock);
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

#ifdef CONFIG_RINA_PFT_REGRESSION_TESTS
static bool regression_tests_nhop(void)
{
        struct pft * tmp;
        port_id_t *  port_ids;
        size_t       nr;
        port_id_t *  ports;
        size_t       entries;
        int          i;

        LOG_DBG("Creating a new instance");
        tmp = pft_create();
        if (!tmp) {
                LOG_DBG("Failed to create instance");
                return false;
        }

        entries = 1;
        ports   = rkmalloc(sizeof(*ports), GFP_KERNEL);
        if (!ports) {
                LOG_DBG("Failed to malloc");
                return false;
        }

        LOG_DBG("Adding a new entry");
        ports[0] = 2;
        LOG_DBG("Adding port-id %d", ports[0]);
        if (pft_add(tmp, 30, 2, ports, entries)) {
                LOG_DBG("Failed to add entry in table");
                rkfree(ports);
                return false;
        }

        LOG_DBG("Adding a new entry");
        ports[0] = 99;
        LOG_DBG("Adding port-id %d", ports[0]);
        if (pft_add(tmp, 30, 2, ports, entries)) {
                LOG_DBG("Failed to add entry");
                rkfree(ports);
                return false;
        }

        LOG_DBG("Trying to retrieve these entries with "
                "a table that is set to NULL "
                "and size 0 as in parameter");
        nr       = 0;
        port_ids = NULL;
        if (pft_nhop(tmp, 30, 2, &port_ids, &nr)) {
                LOG_DBG("Failed to get port-ids");
                rkfree(ports);
                return false;
        }

        LOG_DBG("Checking if we got both port-ids");
        if (nr != 2) {
                LOG_DBG("Wrong number of port-ids returned");
                rkfree(ports);
                return false;
        }

        for (i = 0; i < nr; i++) {
                LOG_DBG("Retrieved port-id %d", port_ids[i]);
        }

        LOG_DBG("Flushing table");
        if (pft_flush(tmp)) {
                LOG_DBG("Failed to flush table");
                rkfree(ports);
                return false;
        }

        /* Port-id table is now 2 in size */

        LOG_DBG("Adding the same entries again");
        ports[0] = 2;
        if (pft_add(tmp, 30, 2, ports, entries)) {
                LOG_DBG("Failed to add entry");
                rkfree(ports);
                return false;
        }

        ports[0] = 99;
        if (pft_add(tmp, 30, 2, ports, entries)) {
                LOG_DBG("Failed to add entry");
                rkfree(ports);
                return false;
        }

        LOG_DBG("Retrieving with a table of size 2 as in param");
        if (pft_nhop(tmp, 30, 2, &port_ids, &nr)) {
                LOG_DBG("Failed to get port-ids");
                rkfree(ports);
                return false;
        }

        LOG_DBG("Checking the number of port-ids returned");
        if (nr != 2) {
                LOG_DBG("Wrong number of port-ids returned");
                rkfree(ports);
                return false;
        }

        LOG_DBG("Flushing the table");
        if (pft_flush(tmp)) {
                LOG_DBG("Failed to flush table");
                rkfree(ports);
                return false;
        }

        /* Trying with 1 port-id */
        LOG_DBG("Trying with just one port-id now");
        LOG_DBG("Adding an entry");
        ports[0] = 2;
        if (pft_add(tmp, 30, 2, ports, entries)) {
                LOG_DBG("Failed to add entry");
                rkfree(ports);
                return false;
        }

        LOG_DBG("Retrieving it");
        if (pft_nhop(tmp, 30, 2, &port_ids, &nr)) {
                LOG_DBG("Failed to get port-ids");
                rkfree(ports);
                return false;
        }

        LOG_DBG("Checking if it is just one port-id");
        if (nr != 1) {
                LOG_DBG("Wrong number of port-ids returned");
                rkfree(ports);
                return false;
        }

        LOG_DBG("Flushing the table");
        if (pft_flush(tmp)) {
                LOG_DBG("Failed to flush table");
                rkfree(ports);
                return false;
        }

        /* Trying with 3 port-ids */
        LOG_DBG("Trying with 3 port-ids");
        LOG_DBG("Adding an entry");
        ports[0] = 2;
        if (pft_add(tmp, 30, 2, ports, entries)) {
                LOG_DBG("Failed to add entry");
                rkfree(ports);
                return false;
        }

        LOG_DBG("Adding an entry");
        ports[0] = 99;
        if (pft_add(tmp, 30, 2,  ports, entries)) {
                LOG_DBG("Failed to add entry");
                rkfree(ports);
                return false;
        }

        LOG_DBG("Adding an entry");
        ports[0] = 9;
        if (pft_add(tmp, 30, 2,  ports, entries)) {
                LOG_DBG("Failed to add entry");
                rkfree(ports);
                return false;
        }

        LOG_DBG("Retrieving it");
        if (pft_nhop(tmp, 30, 2, &port_ids, &nr)) {
                LOG_DBG("Failed to get port-ids");
                rkfree(ports);
                return false;
        }

        LOG_DBG("Checking if we retrieved 3");
        if (nr != 3) {
                LOG_DBG("Wrong number of port-ids returned");
                rkfree(ports);
                return false;
        }

        /* Passing bogus parms here */
        LOG_DBG("Passing bogus parms to nhop");
        LOG_DBG("Passing bogus array");
        if (!pft_nhop(tmp, 30, 2, NULL, 0)) {
                LOG_DBG("Bogus array passed");
                rkfree(ports);
                return false;
        }

        LOG_DBG("Passing bogus instance");
        if (!pft_nhop(NULL, 30, 2, &port_ids, &nr)) {
                LOG_DBG("Bogus instance passed");
                rkfree(ports);
                return false;
        }

        LOG_DBG("Passing bogus port-id");
        if (!pft_nhop(tmp, 30, -99, &port_ids, &nr)) {
                LOG_DBG("Bogus port-id passed");
                rkfree(ports);
                return false;
        }

        LOG_DBG("Passing bogus address");
        if (!pft_nhop(tmp, -59, 9, &port_ids, &nr)) {
                LOG_DBG("Bogus address passed");
                rkfree(ports);
                return false;
        }

        LOG_DBG("Destroying instance");
        if (pft_destroy(tmp)) {
                LOG_DBG("Failed to destroy instance");
                rkfree(ports);
                return false;
        }

        rkfree(port_ids);
        rkfree(ports);
        return true;
}

static bool regression_tests_entries(void)
{
        struct pft * tmp;
        port_id_t *  ports;
        size_t       entries;

        LOG_DBG("Creating a new instance");
        tmp = pft_create();
        if (!tmp) {
                LOG_DBG("Failed to create instance");
                return false;
        }

        entries = 1;
        ports   = rkmalloc(sizeof(*ports), GFP_KERNEL);
        if (!ports) {
                LOG_DBG("Failed to malloc");
                return false;
        }

        LOG_DBG("Adding an entry");
        ports[0] = 1;
        if (pft_add(tmp, 16, 1, ports, entries)) {
                LOG_DBG("Failed to add entry");
                rkfree(ports);
                return false;
        }

        LOG_DBG("Removing an entry");
        if (pft_remove(tmp, 16, 1, ports, entries)) {
                LOG_DBG("Failed to remove entry");
                rkfree(ports);
                return false;
        }

        LOG_DBG("Removing the previous entry again");
        if (!pft_remove(tmp, 16, 1, ports, entries)) {
                LOG_DBG("Entry should have already been removed");
                rkfree(ports);
                return false;
        }

        LOG_DBG("Removing an entry that's not there");
        if (!pft_remove(tmp, 35, 4, ports, entries)) {
                LOG_DBG("No such entry was added");
                rkfree(ports);
                return false;
        }

        LOG_DBG("Adding an entry");
        ports[0] = 2;
        if (pft_add(tmp, 30, 8, ports, entries)) {
                LOG_DBG("Failed to add entry");
                rkfree(ports);
                return false;
        }

        LOG_DBG("Adding an entry");
        ports[0] = 99;
        if (pft_add(tmp, 35, 5, ports, entries)) {
                LOG_DBG("Failed to add entry");
                rkfree(ports);
                return false;
        }

        LOG_DBG("Flushing the whole table");
        if (pft_flush(tmp)) {
                LOG_DBG("Failed to flush table");
                rkfree(ports);
                return false;
        }

        /* Passing bogus parms here */
        LOG_DBG("Trying bogus parameters");

        LOG_DBG("Passing bogus instance");
        if (!pft_add(NULL, 35, 5, ports, entries)) {
                LOG_DBG("Bogus instance used");
                rkfree(ports);
                return false;
        }

        LOG_DBG("Adding an entry with an empty table");
        if (!pft_add(tmp, 35, 5, NULL, 0)) {
                LOG_DBG("Bogus ports passed");
                rkfree(ports);
                return false;
        }

        LOG_DBG("Passing bogus instance");
        if (!pft_remove(NULL, 35, 4, ports, entries)) {
                LOG_DBG("Bogus instance used");
                rkfree(ports);
                return false;
        }

        LOG_DBG("Removing an entry with an empty table");
        if (!pft_remove(tmp, 35, 4, NULL, 0)) {
                LOG_DBG("Bogus ports passed to remove");
                rkfree(ports);
                return false;
        }

        LOG_DBG("Destroying the instance");
        if (pft_destroy(tmp)) {
                LOG_DBG("Failed to destroy instance");
                rkfree(ports);
                return false;
        }

        rkfree(ports);
        return true;
}

static bool regression_tests_instance(void)
{
        struct pft * tmp;

        LOG_DBG("Creating an instance");
        tmp = pft_create();
        if (!tmp) {
                LOG_DBG("Failed to create instance");
                return false;
        }

        LOG_DBG("Destroying an instance");
        if (pft_destroy(tmp)) {
                LOG_DBG("Failed to destroy instance");
                return false;
        }

        return true;
}

bool regression_tests_pft(void)
{
        if (!regression_tests_instance()) {
                LOG_ERR("Creating of an instance test failed, "
                        "bailing out");
                return false;
        }

        if (!regression_tests_entries()) {
                LOG_ERR("Adding/removing entries test failed, "
                        "bailing out");
                return false;
        }

        if (!regression_tests_nhop()) {
                LOG_ERR("NHOP lookup operation is crap, "
                        "bailing out");
                return false;
        }

        return true;
}
#endif
