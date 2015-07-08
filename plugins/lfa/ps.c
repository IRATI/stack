/*
 * Loop-Free Alternates policy set for PFF
 *
 *    Sander Vrijders <sander.vrijders@intec.ugent.be>
 *    Vincenzo Maffione <v.maffione@nextworks.it>
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

#include <linux/export.h>
#include <linux/module.h>
#include <linux/string.h>

#define RINA_PREFIX "pff-lfa"

#include "logs.h"
#include "rds/rmem.h"
#include "rds/rtimer.h"
#include "pff-ps.h"
#include "debug.h"

/* FIXME: Will hold the ports that are down */
struct pft_ports_down {
        struct list_head ports;
};

static struct pft_ports_down * ports_down;

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

/* FIXME: This thing is bogus and has to be fixed properly */
#ifdef CONFIG_RINA_ASSERTIONS
static bool pft_pe_is_ok(struct pft_port_entry * pe)
{ return pe ? true : false;  }
#endif

static void pft_pe_destroy(struct pft_port_entry * pe)
{
        ASSERT(pft_pe_is_ok(pe));

        list_del(&pe->next);
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
        int              nhop;
        int              port;
        struct list_head alt_ports;
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
        tmp->nhop        = 0;
        tmp->port        = 0;
        INIT_LIST_HEAD(&tmp->alt_ports);
        INIT_LIST_HEAD(&tmp->next);

        return tmp;
}

static struct pft_entry * pfte_create_ni(address_t destination,
                                         qos_id_t  qos_id)
{ return pfte_create_gfp(GFP_ATOMIC, destination, qos_id); }

/* FIXME: This thing is bogus and has to be fixed properly */
#ifdef CONFIG_RINA_ASSERTIONS
static bool pfte_is_ok(struct pft_entry * entry)
{ return entry ? true : false; }
#endif

static void pfte_destroy(struct pft_entry * entry)
{
        struct pft_port_entry * pos, * next;

        ASSERT(pfte_is_ok(entry));

        list_for_each_entry_safe(pos, next, &entry->alt_ports, next) {
                pft_pe_destroy(pos);
        }

        list_del(&entry->next);
        rkfree(entry);
}

static struct pft_port_entry * pfte_port_find(struct pft_entry * entry,
                                              port_id_t          id)
{
        struct pft_port_entry * pos;

        ASSERT(pfte_is_ok(entry));

        list_for_each_entry(pos, &entry->alt_ports, next) {
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

        list_add(&pe->next, &entry->alt_ports);

        return 0;
}

static int pfte_ports_copy(struct pft_entry * entry,
                           port_id_t **       port_ids,
                           size_t *           entries)
{
        ASSERT(pfte_is_ok(entry));
        ASSERT(entries);

        if (*entries != 1) {
                if (*entries > 0)
                        rkfree(*port_ids);
                *port_ids = rkmalloc(sizeof(**port_ids),
                                     GFP_ATOMIC);
                if (!*port_ids) {
                        *entries = 0;
                        return -1;
                }
        }
        *entries = 1;

        (*port_ids)[0] = entry->nhop;

        return 0;
}

struct pff_ps_priv {
        spinlock_t       lock;
        struct list_head entries;
};

static bool priv_is_ok(struct pff_ps_priv * priv)
{ return priv != NULL; }

static struct pft_entry * pft_find(struct pff_ps_priv * priv,
                                   address_t            destination,
                                   qos_id_t             qos_id)
{
        struct pft_entry * pos;

        ASSERT(priv_is_ok(priv));
        ASSERT(is_address_ok(destination));

        list_for_each_entry(pos, &priv->entries, next) {
                if ((pos->destination == destination) &&
                    (pos->qos_id      == qos_id)) {
                        return pos;
                }
        }

        return NULL;
}

static int lfa_add(struct pff_ps *        ps,
                   struct mod_pff_entry * entry)
{
        struct pff_ps_priv *     priv;
        struct pft_entry *       tmp;
	struct port_id_altlist * alts;
        int                      i;

        priv = (struct pff_ps_priv *) ps->priv;
        if (!priv_is_ok(priv))
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

        spin_lock(&priv->lock);

        tmp = pft_find(priv, entry->fwd_info, entry->qos_id);
        if (!tmp) {
                tmp = pfte_create_ni(entry->fwd_info, entry->qos_id);
                if (!tmp) {
                        spin_unlock(&priv->lock);
                        return -1;
                }

                list_add(&tmp->next, &priv->entries);
        } else {
                LOG_ERR("Entry already exists");
                spin_unlock(&priv->lock);
                return -1;
        }

        alts = list_first_entry(&entry->port_id_altlists,
                                typeof(*alts),
                                next);

        if (alts->num_ports < 1) {
                LOG_INFO("Port id set is empty");
                pfte_destroy(tmp);
                spin_unlock(&priv->lock);
                return -1;
        }

        tmp->port = alts->ports[0];
        tmp->nhop = tmp->port;

        for (i = 1; i < alts->num_ports-1; i++) {
                if (pfte_port_add(tmp, alts->ports[i])) {
                        pfte_destroy(tmp);
                        spin_unlock(&priv->lock);
                        return -1;
                }
        }

        spin_unlock(&priv->lock);

        return 0;
}

static int lfa_remove(struct pff_ps *        ps,
                      struct mod_pff_entry * entry)
{
        struct pff_ps_priv *       priv;
        struct pft_entry *         tmp;
        struct port_id_altlist *   alt;

        priv = (struct pff_ps_priv *) ps->priv;
        if (!priv_is_ok(priv))
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

        spin_lock(&priv->lock);

        tmp = pft_find(priv, entry->fwd_info, entry->qos_id);
        if (!tmp) {
                spin_unlock(&priv->lock);
                LOG_ERR("No such entry");
                return -1;
        }

        alt = list_first_entry(&entry->port_id_altlists,
                               typeof(*alt),
                               next);

        if (!alt->ports || tmp->port != alt->ports[0]) {
                spin_unlock(&priv->lock);
                LOG_ERR("No or wrong port in mod_pff message");
                return -1;
        }

        pfte_destroy(tmp);

        spin_unlock(&priv->lock);

        return 0;
}

static bool lfa_is_empty(struct pff_ps * ps)
{
        struct pff_ps_priv * priv;
        bool                 empty;

        priv = (struct pff_ps_priv *) ps->priv;
        if (!priv_is_ok(priv))
                return false;

        spin_lock(&priv->lock);
        empty = list_empty(&priv->entries);
        spin_unlock(&priv->lock);

        return empty;
}

static void __pft_flush(struct pff_ps_priv * priv)
{
        struct pft_entry * pos, * next;

        ASSERT(priv_is_ok(priv));

        list_for_each_entry_safe(pos, next, &priv->entries, next) {
                pfte_destroy(pos);
        }
}

static int lfa_flush(struct pff_ps * ps)
{
        struct pff_ps_priv * priv;

        priv = (struct pff_ps_priv *) ps->priv;
        if (!priv_is_ok(priv))
                return -1;

        spin_lock(&priv->lock);

        __pft_flush(priv);

        spin_unlock(&priv->lock);

        return 0;
}

static int lfa_nhop(struct pff_ps * ps,
                    struct pci *    pci,
                    port_id_t **    ports,
                    size_t *        count)
{
        struct pff_ps_priv * priv;
        address_t            destination;
        qos_id_t             qos_id;
        struct pft_entry *   tmp;
        unsigned long        flags;

        priv = (struct pff_ps_priv *) ps->priv;
        if (!priv_is_ok(priv)) {
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
         * Taking the lock here since otherwise priv might be deleted when
         * copying the ports
         */
        spin_lock_irqsave(&priv->lock, flags);

        tmp = pft_find(priv, destination, qos_id);
        if (!tmp) {
                LOG_ERR("Could not find any entry for dest address: %u and "
                        "qos_id %d", destination, qos_id);
                spin_unlock_irqrestore(&priv->lock, flags);
                return -1;
        }

        if (pfte_ports_copy(tmp, ports, count)) {
                spin_unlock_irqrestore(&priv->lock, flags);
                return -1;
        }

        spin_unlock_irqrestore(&priv->lock, flags);

        return 0;
}

static int pfte_port_id_altlists_copy(struct pft_entry * entry,
                                      struct list_head * port_id_altlists)
{
        struct pft_port_entry *  pos;
        struct port_id_altlist * alt;
        int                      i;

        ASSERT(pfte_is_ok(entry));

        alt = rkmalloc(sizeof(*alt), GFP_ATOMIC);
        if (!alt) {
                return -1;
        }

        /* Count the number of alt ports */
        i = 1;
        list_for_each_entry(pos, &entry->alt_ports, next) {
                i++;
        }
        alt->num_ports = i;

        alt->ports = rkmalloc(i * sizeof(*(alt->ports)), GFP_ATOMIC);
        if (!alt->ports) {
                return -1;
        }

        /* Fill in the ports */
        i = 0;
        alt->ports[i] = entry->port;
        i++;
        list_for_each_entry(pos, &entry->alt_ports, next) {
                alt->ports[i] = pft_pe_port(pos);
                i++;
        }

	list_add_tail(&alt->next, port_id_altlists);

        return 0;
}

static int lfa_dump(struct pff_ps *    ps,
                    struct list_head * entries)
{
        struct pff_ps_priv *   priv;
        struct pft_entry *     pos;
        struct mod_pff_entry * entry;

        priv = (struct pff_ps_priv *) ps->priv;
        if (!priv_is_ok(priv))
                return -1;

        spin_lock(&priv->lock);
        list_for_each_entry(pos, &priv->entries, next) {
                entry = rkmalloc(sizeof(*entry), GFP_ATOMIC);
                if (!entry) {
                        spin_unlock(&priv->lock);
                        return -1;
                }

                entry->fwd_info = pos->destination;
                entry->qos_id = pos->qos_id;
		INIT_LIST_HEAD(&entry->port_id_altlists);
                if (pfte_port_id_altlists_copy(pos, &entry->port_id_altlists)) {
                        rkfree(entry);
                        spin_unlock(&priv->lock);
                        return -1;
                }

                list_add(&entry->next, entries);
        }
        spin_unlock(&priv->lock);

        return 0;
}

/* NOTE: This is skeleton code that was directly copy pasted */
static int pff_ps_set_policy_set_param(struct ps_base * bps,
                                       const char *     name,
                                       const char *     value)
{
        struct pff_ps * ps = container_of(bps, struct pff_ps, base);

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
pff_ps_lfa_create(struct rina_component * component)
{
        struct pff_ps * ps;
        struct pff_ps_priv * priv;
        struct pff * pff = pff_from_component(component);

        priv = rkzalloc(sizeof(*priv), GFP_KERNEL);
        if (!priv) {
                return NULL;
        }

        spin_lock_init(&priv->lock);

        INIT_LIST_HEAD(&priv->entries);

        ps = rkzalloc(sizeof(*ps), GFP_KERNEL);
        if (!ps) {
                return NULL;
        }

        ps->base.set_policy_set_param = pff_ps_set_policy_set_param;
        ps->dm = pff;
        ps->priv = (void *) priv;

        ps->pff_add = lfa_add;
        ps->pff_remove = lfa_remove;
        ps->pff_is_empty = lfa_is_empty;
        ps->pff_flush = lfa_flush;
        ps->pff_nhop = lfa_nhop;
        ps->pff_dump = lfa_dump;

        return &ps->base;
}

static void pff_ps_lfa_destroy(struct ps_base * bps)
{
        struct pff_ps * ps = container_of(bps, struct pff_ps, base);

        if (bps) {
                struct pff_ps_priv * priv;

                priv = (struct pff_ps_priv *) ps->priv;
                if(!priv_is_ok(priv)) {
                        return;
                }

                spin_lock(&priv->lock);

                __pft_flush(priv);

                spin_unlock(&priv->lock);

                rkfree(priv);
                rkfree(ps);
        }
}

struct ps_factory pff_factory = {
        .owner   = THIS_MODULE,
        .create  = pff_ps_lfa_create,
        .destroy = pff_ps_lfa_destroy,
};

#define RINA_PFF_LFA_NAME "lfa"

static int __init mod_init(void)
{
        int ret;

        ports_down = rkmalloc(sizeof(*ports_down),
                              GFP_KERNEL);

        strcpy(pff_factory.name, RINA_PFF_LFA_NAME);

        ret = pff_ps_publish(&pff_factory);
        if (ret) {
                LOG_ERR("Failed to publish policy set factory");
                return -1;
        }

        LOG_INFO("PFF LFA policy set loaded successfully");

        return 0;
}

static void __exit mod_exit(void)
{
        int ret;

        rkfree(ports_down);

        ret = pff_ps_unpublish(RINA_PFF_LFA_NAME);

        if (ret) {
                LOG_ERR("Failed to unpublish policy set factory");
                return;
        }

        LOG_INFO("PFF LFA policy set unloaded successfully");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("PFF LFA policy set");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Sander Vrijders <sander.vrijders@intec.ugent.be>");
