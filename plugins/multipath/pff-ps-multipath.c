/*
 * ECMP PS
 *
 *    Javier Garcia <javier.garcial@atos.net>
 *
 * This program is free software; you can dummyistribute it and/or modify
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
#include <linux/crc16.h>
#include <linux/random.h>
#include <linux/list.h>
#include <linux/types.h>
#include <linux/slab.h>

#define RINA_PREFIX "pff-multipath"

#include "logs.h"
#include "rds/rmem.h"
#include "pff-ps.h"
#include "pff.h"
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

        list_del(&entry->next);
        rkfree(entry);
}

static struct pft_port_entry * pfte_port_find(struct pft_entry * entry,
                                              port_id_t          id)
{
        struct pft_port_entry * pos;

        ASSERT(pfte_is_ok(entry));

        list_for_each_entry(pos, &entry->ports, next) {
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

        list_add(&pe->next, &entry->ports);

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

static int pfte_ports_copy(struct  pft_port_entry * port,
                           port_id_t **       port_ids,
                           size_t *           entries)
{
        size_t	count;
        count = 1;

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

        (*port_ids)[0] = pft_pe_port(port);

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
		    ((pos->qos_id == 0) || (pos->qos_id == qos_id))) {
			return pos;
                }
        }
	
        return NULL;
}

static int __pff_add(struct pff_ps *        ps,
		     struct pff_ps_priv * priv,
		     struct mod_pff_entry * entry)
{
        struct pft_entry *       tmp;
	struct port_id_altlist * alts;

        tmp = pft_find(priv, entry->fwd_info, entry->qos_id);
        if (!tmp) {
                tmp = pfte_create_ni(entry->fwd_info, entry->qos_id);
                if (!tmp) {
                        return -1;
                }

                list_add(&tmp->next, &priv->entries);
        }

	list_for_each_entry(alts, &entry->port_id_altlists, next) {
		if (alts->num_ports < 1) {
			LOG_INFO("Port id alternative set is empty");
			continue;
		}

                if (pfte_port_add(tmp, alts->ports[0])) {
                        pfte_destroy(tmp);
                        return -1;
                }
	}

	return 0;
}

static int mp_add(struct pff_ps *        ps,
                       struct mod_pff_entry * entry)
{
        struct pff_ps_priv *     priv;
        int result;

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

        spin_lock_bh(&priv->lock);

        result = __pff_add(ps, priv, entry);

        spin_unlock_bh(&priv->lock);

        return result;
}

static int mp_remove(struct pff_ps *        ps,
                          struct mod_pff_entry * entry)
{
	
        struct pff_ps_priv *       priv;
        struct port_id_altlist *   alts;
        struct pft_entry *         tmp;

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

        spin_lock_bh(&priv->lock);

        tmp = pft_find(priv, entry->fwd_info, entry->qos_id);
        if (!tmp) {
                spin_unlock_bh(&priv->lock);
                return -1;
        }

	list_for_each_entry(alts, &entry->port_id_altlists, next) {
		if (alts->num_ports < 1) {
			LOG_INFO("Port id alternative set is empty");
			continue;
		}

	// Just remove the first alternative and ignore the others.
		pfte_port_remove(tmp, alts->ports[0]);
	}

        // If the list of port-ids is empty, remove the entry 
	
        if (list_empty(&tmp->ports)) {
                pfte_destroy(tmp);
        }
	

        spin_unlock_bh(&priv->lock);

        return 0;
}

static bool mp_is_empty(struct pff_ps * ps)
{
        struct pff_ps_priv * priv;
        bool                 empty;

        priv = (struct pff_ps_priv *) ps->priv;
        if (!priv_is_ok(priv))
                return false;

        spin_lock_bh(&priv->lock);
        empty = list_empty(&priv->entries);
        spin_unlock_bh(&priv->lock);

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

static int mp_flush(struct pff_ps * ps)
{
        struct pff_ps_priv * priv;

        priv = (struct pff_ps_priv *) ps->priv;
        if (!priv_is_ok(priv))
                return -1;

        spin_lock_bh(&priv->lock);

        __pft_flush(priv);

        spin_unlock_bh(&priv->lock);

        return 0;
}



/**
 * @brief Selects the next hop pft_entry from a list based
 * on hash-threshold algorithm
 * @param entry_list: list of possible next hop pft_entries
 * @param pci: PCI to extract the source & dest address and QoS
 * of the PDU
 * @return pointer to selected pft_entry
 */
struct pft_port_entry * select_entry(struct pft_entry * entry,
                                struct pci * pci)
{
        int num_entries;
        struct pft_port_entry * pos;
        int i;

        unsigned short hash_key;
        const int KEYSPACE = 0xFFFF;
        int region_size;
        unsigned short region;
        
	typedef struct {
		address_t pci_source;
		address_t pci_destination;
		address_t pci_qos_id;
		cep_id_t  pci_cep_source;
		cep_id_t  pci_cep_destination;
	} connection_id;
	connection_id c_id;

	c_id.pci_source = pci_source(pci);
	c_id.pci_destination = pci_destination(pci);
	c_id.pci_qos_id = pci_qos_id(pci);
	c_id.pci_cep_source = pci_cep_source(pci);
	c_id.pci_cep_destination = pci_cep_destination(pci);

        num_entries = 0;
        list_for_each_entry(pos, &entry->ports, next) {
                num_entries++;
        }
        
        hash_key = crc16(0, (const u8 *)&c_id, sizeof(c_id));
        
        region_size = KEYSPACE / num_entries;
        region = hash_key / region_size;
        
        i = 0;
        list_for_each_entry(pos, &entry->ports, next) {
                if (region == i++) {
                        return pos;
                }
        }
        
        return pos;
}

static int mp_next_hop(struct pff_ps * ps,
                            struct pci *    pci,
                            port_id_t **    ports,
                            size_t *        count)
{
	struct pff_ps_priv *    priv;
        address_t               destination;
        qos_id_t                qos_id;
        struct pft_entry *      tmp;
	struct pft_port_entry * port;

        priv = (struct pff_ps_priv *) ps->priv;
        if (priv == NULL) {
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
        spin_lock_bh(&priv->lock);

        tmp = pft_find(priv, destination, qos_id);
        if (!tmp) {
                LOG_ERR("Could not find any entry for dest address: %u and "
                        "qos_id %d", destination, qos_id);
                spin_unlock_bh(&priv->lock);
                return -1;
        }

        /* 
         * Hash-threshold algorithm based on
         * CRC16 Linux kernel implementation
        */
        port = select_entry(tmp, pci);
	if (!port) {
                LOG_ERR("Could not select destination port for dest "
                         "address %u and qos_id %d", destination, qos_id);
                spin_unlock_bh(&priv->lock);
                return -1;
         }

        if (pfte_ports_copy(port, ports, count)) {
                spin_unlock_bh(&priv->lock);
                return -1;
        }

        spin_unlock_bh(&priv->lock);
	
	return 0;
}

static int pfte_port_id_altlists_copy(struct pft_entry * entry,
                                      struct list_head * port_id_altlists)
{
        struct pft_port_entry * pos;

        ASSERT(pfte_is_ok(entry));

        list_for_each_entry(pos, &entry->ports, next) {
		struct port_id_altlist * alt;
		int cnt = 1;

		alt = rkmalloc(sizeof(*alt), GFP_ATOMIC);
		if (!alt) {
			return -1;
		}

		alt->ports = rkmalloc(cnt * sizeof(*(alt->ports)), GFP_ATOMIC);
		if (!alt->ports) {
			rkfree(alt);
			return -1;
		}

		alt->ports[0] = pft_pe_port(pos);
		alt->num_ports = cnt;

		list_add_tail(&alt->next, port_id_altlists);
        }

        return 0;
}

static int mp_dump(struct pff_ps *    ps,
                        struct list_head * entries)
{
        struct pff_ps_priv *   priv;
        struct pft_entry *     pos;
        struct mod_pff_entry * entry;

        priv = (struct pff_ps_priv *) ps->priv;
        if (!priv_is_ok(priv))
                return -1;

        spin_lock_bh(&priv->lock);
        list_for_each_entry(pos, &priv->entries, next) {
                entry = rkmalloc(sizeof(*entry), GFP_ATOMIC);
                if (!entry) {
                        spin_unlock_bh(&priv->lock);
                        return -1;
                }

                entry->fwd_info = pos->destination;
                entry->qos_id = pos->qos_id;
		INIT_LIST_HEAD(&entry->port_id_altlists);
                if (pfte_port_id_altlists_copy(pos, &entry->port_id_altlists)) {
                        rkfree(entry);
                        spin_unlock_bh(&priv->lock);
                        return -1;
                }

                list_add(&entry->next, entries);
        }
        spin_unlock_bh(&priv->lock);

        return 0;
}

int mp_modify(struct pff_ps *    ps,
              struct list_head * entries)
{
        struct pff_ps_priv *   priv;
        struct mod_pff_entry * entry;

        priv = (struct pff_ps_priv *) ps->priv;
        if (!priv_is_ok(priv))
                return -1;

        spin_lock_bh(&priv->lock);

        __pft_flush(priv);

        list_for_each_entry(entry, entries, next) {
        	if (!entry)
        		continue;

        	if (!is_address_ok(entry->fwd_info))
        		continue;

        	if (!is_qos_id_ok(entry->qos_id))
        		continue;

                __pff_add(ps, priv, entry);
        }

        spin_unlock_bh(&priv->lock);

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
pff_ps_multipath_create(struct rina_component * component)
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
        	rkfree(priv);
                return NULL;
        }

        ps->base.set_policy_set_param = pff_ps_set_policy_set_param;
        ps->dm = pff;
        ps->priv = (void *) priv;
        ps->pff_add = mp_add;
        ps->pff_remove = mp_remove;
	ps->pff_port_state_change = NULL;
        ps->pff_is_empty = mp_is_empty;
        ps->pff_flush = mp_flush;
        ps->pff_nhop = mp_next_hop;
        ps->pff_dump = mp_dump;
        ps->pff_modify = mp_modify;

        return &ps->base;
}

static void pff_ps_multipath_destroy(struct ps_base * bps)
{
	struct pff_ps * ps = container_of(bps, struct pff_ps, base);

        if (bps) {
                struct pff_ps_priv * priv;

                priv = (struct pff_ps_priv *) ps->priv;
                if(!priv_is_ok(priv)) {
                        return;
                }

                spin_lock_bh(&priv->lock);

                __pft_flush(priv);

                spin_unlock_bh(&priv->lock);

                rkfree(priv);
                rkfree(ps);
        }

}
struct ps_factory pff_factory = {
        .owner          = THIS_MODULE,
        .create  = pff_ps_multipath_create,
        .destroy = pff_ps_multipath_destroy,
};
