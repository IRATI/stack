/*
 * Dummy PFT PS
 *
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
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

#define RINA_PREFIX "pff-multipath"

#include "logs.h"
#include "rds/rmem.h"
#include "pff-ps.h"
#include "pff.h"
#include "debug.h"

struct pft_entry {
        address_t        destination;
        qos_id_t         qos_id;
        struct list_head ports;
        struct list_head next;
};

struct pft_port_entry {
        port_id_t        port_id;
        struct list_head next;
};

struct pff_ps_priv {
        spinlock_t       lock;
        struct list_head entries;
};

static struct pft_entry * pft_find(struct pff_ps_priv * priv,
                                   address_t            destination,
                                   qos_id_t             qos_id)
{
        struct pft_entry * res = NULL;
        struct pft_entry * it;
        struct pft_entry * previous;

        ASSERT(priv != NULL);
        ASSERT(is_address_ok(destination));

        list_for_each_entry(it, &priv->entries, next) {
                if ((it->destination == destination) &&
                    (it->qos_id == qos_id)) {

			LOG_DBG("Match found: %s", it->destination);

                        struct pft_entry * tmp = rkmalloc(sizeof(*tmp), GFP_ATOMIC);
                        if (!tmp) {
                                return res;
                        }
                        *tmp = *it;
                        // tmp->next = NULL;
			INIT_LIST_HEAD(&tmp->next);
                        
                        if (res == NULL) {
                                res = tmp;
                                previous = tmp;
                        } else {
				list_add(&tmp->next, 
					&previous->next);
                                // previous->next = tmp;
                                previous = tmp;
                        }
                }
        }

        return res;
}

static int pfte_ports_copy(struct pft_entry * entry,
                           port_id_t **       port_ids,
                           size_t *           entries)
{
        struct pft_port_entry * pos;
        size_t                  count;
        int                     i;

        ASSERT(entry != NULL);

        count = 0;
        list_for_each_entry(pos, &entry->ports, next) {
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
        list_for_each_entry(pos, &entry->ports, next) {
                (*port_ids)[i++] = pos->port_id;
        }

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
struct pft_entry * select_entry(struct pft_entry * entry_list,
                                struct pci * pci)
{
        int num_entries;
        struct pft_entry * pos;
        int i;
        
        unsigned short hash_key;
        const int KEYSPACE = 0xFFFF;
        int region_size;
        unsigned short region;
        
	typedef struct {
		address_t pci_source;
		address_t pci_destination;
		address_t pci_qos_id;
	} connection_id;
	connection_id c_id;

	c_id.pci_source = pci_source(pci);
	c_id.pci_destination = pci_destination(pci);
	c_id.pci_qos_id = pci_qos_id(pci);

        num_entries = 0;
        list_for_each_entry(pos, &entry_list->next, next) {
                num_entries++;
        }
        
        hash_key = crc16(0, (const u8 *)&c_id, sizeof(c_id));
        
        region_size = KEYSPACE / num_entries;
        region = hash_key / region_size;
        
        i = 0;
        list_for_each_entry(pos, &entry_list->next, next) {
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
	struct pff_ps_priv * priv;
        address_t            destination;
        qos_id_t             qos_id;
        struct pft_entry *   tmp;
        unsigned long        flags;

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
        spin_lock_irqsave(&priv->lock, flags);

        tmp = pft_find(priv, destination, qos_id);
        if (!tmp) {
                LOG_ERR("Could not find any entry for dest address: %u and "
                        "qos_id %d", destination, qos_id);
                spin_unlock_irqrestore(&priv->lock, flags);
                return -1;
        }

        /* 
         * Hash-threshold algorithm based on
         * CRC16 Linux kernel implementation
        */
        tmp = select_entry(tmp, pci);

        if (pfte_ports_copy(tmp, ports, count)) {
                spin_unlock_irqrestore(&priv->lock, flags);
                return -1;
        }

        spin_unlock_irqrestore(&priv->lock, flags);  
	
	return 0;
}

static int pff_ps_set_policy_set_param(struct ps_base * bps,
                                       const char *     name,
                                       const char *     value)
{
        printk("%s: called()\n", __func__);
        return 0;
}

static struct ps_base *
pff_ps_multipath_create(struct rina_component * component)
{
        struct pff * dtp = pff_from_component(component);
        struct pff_ps * ps = rkzalloc(sizeof(*ps), GFP_KERNEL);

        if (!ps) {
                return NULL;
        }

        ps->base.set_policy_set_param = pff_ps_set_policy_set_param;
        ps->dm              = dtp;
        ps->priv            = NULL;

	LOG_INFO("Multipath create called");

        ps->pff_nhop = mp_next_hop;

        return &ps->base;
}

static void pff_ps_multipath_destroy(struct ps_base * bps)
{
        struct pff_ps *ps = container_of(bps, struct pff_ps, base);

        if (bps) {
                rkfree(ps);
        }
}
struct ps_factory pff_factory = {
        .owner          = THIS_MODULE,
        .create  = pff_ps_multipath_create,
        .destroy = pff_ps_multipath_destroy,
};
