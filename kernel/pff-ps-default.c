/*
 * Default policy set for PFF
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

#include <linux/export.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/random.h>

#define RINA_PREFIX "pff-ps-default"

#include "logs.h"
#include "rds/rmem.h"
#include "rds/rtimer.h"
#include "rds/rwq.h"
#include "pff-ps-default.h"
#include "debug.h"
#include "rds/robjects.h"
#include "ipcp-instances.h"

/* FIXME: This representation is crappy and MUST be changed */
struct pft_port_entry {
        port_id_t        port_id;
        struct list_head next;
};

struct pff_sysfs_work_data {
	struct pft_entry * entry;
	struct rset * rset;
	bool add;
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
	struct robject   robj;
};

static ssize_t pft_entry_attr_show(struct robject *        robj,
                        	   struct robj_attribute * attr,
                                   char *                  buf)
{
	struct pft_entry * entry;

	entry = container_of(robj, struct pft_entry, robj);
	if (!entry)
		return 0;

	if (strcmp(robject_attr_name(attr), "dest_addr") == 0) {
		return sprintf(buf, "%u\n", entry->destination);
	}
	if (strcmp(robject_attr_name(attr), "qos_id") == 0) {
		return sprintf(buf, "%u\n", entry->qos_id);
	}
	if (strcmp(robject_attr_name(attr), "ports") == 0) {
		int offset = 0;
		struct pft_port_entry * pos;
        	list_for_each_entry(pos, &entry->ports, next) {
			offset += sprintf(buf + offset, "%u ", pos->port_id);
        	}
		if (offset > 1)
			sprintf(buf + offset -1, "\n");
		return offset;
	}
	return 0;
}
RINA_SYSFS_OPS(pft_entry);
RINA_ATTRS(pft_entry, dest_addr, qos_id, ports);
RINA_KTYPE(pft_entry);

static struct pft_entry * pfte_create_gfp(gfp_t     flags,
                                          address_t destination,
                                          qos_id_t  qos_id)
{
        struct pft_entry * tmp;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->destination = destination;
        tmp->qos_id      = qos_id;
        INIT_LIST_HEAD(&tmp->ports);
        INIT_LIST_HEAD(&tmp->next);

	robject_init(&tmp->robj, &pft_entry_rtype);

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

static int pff_sysfs_worker(void * o)
{
        struct pff_sysfs_work_data * data;

	ASSERT(o);

        data = (struct pff_sysfs_work_data *) o;

        if (data->add) {
        	robject_rset_add(&data->entry->robj, data->rset,
        			 "%u-%d", data->entry->destination,
				 data->entry->qos_id);
        } else {
        	robject_del(&data->entry->robj);
                rkfree(&data->entry);
        }

        return 0;
}

struct pff_ps_priv {
        spinlock_t       lock;
        struct list_head entries;
        struct workqueue_struct * sysfs_wq;
};

static void pfte_destroy(struct pft_entry * entry, struct pff_ps_priv * priv)
{
        struct pft_port_entry * pos, * next;
        struct pff_sysfs_work_data * wdata;
        struct rwq_work_item       * item;

        ASSERT(pfte_is_ok(entry));

        list_for_each_entry_safe(pos, next, &entry->ports, next) {
                pft_pe_destroy(pos);
        }

        list_del(&entry->next);

	/* Defer sysfs entry deletion to workqueue, since it may sleep */
        wdata = rkzalloc(sizeof(* wdata), GFP_ATOMIC);
        wdata->entry = entry;
        wdata->rset = 0;
        wdata->add = false;
        item  = rwq_work_create_ni(pff_sysfs_worker, wdata);

        rwq_work_post(priv->sysfs_wq, item);
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

static int pfte_ports_copy(struct pft_entry * entry,
                           port_id_t **       port_ids,
                           size_t *           entries)
{
        struct pft_port_entry * pos;
        size_t                  count;
        int                     i;

        ASSERT(pfte_is_ok(entry));

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
                (*port_ids)[i++] = pft_pe_port(pos);
        }

        return 0;
}

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
        struct pff_sysfs_work_data * wdata;
        struct rwq_work_item       * item;

	tmp = pft_find(priv, entry->fwd_info, entry->qos_id);
	if (!tmp) {
		tmp = pfte_create_ni(entry->fwd_info, entry->qos_id);
		if (!tmp) {
			return -1;
		}
		list_add(&tmp->next, &priv->entries);

		/* Defer sysfs entry creation to workqueue, since it may sleep */
	        wdata = rkzalloc(sizeof(* wdata), GFP_ATOMIC);
	        wdata->entry = tmp;
	        wdata->rset = pff_rset(ps->dm);
	        wdata->add = true;
	        item  = rwq_work_create_ni(pff_sysfs_worker, wdata);

	        rwq_work_post(priv->sysfs_wq, item);
	}

	list_for_each_entry(alts, &entry->port_id_altlists, next) {
		if (alts->num_ports < 1) {
			LOG_INFO("Port id alternative set is empty");
			continue;
		}

		/* Just add the first alternative and ignore the others. */
		if (pfte_port_add(tmp, alts->ports[0])) {
			pfte_destroy(tmp, priv);
			return -1;
		}
	}

	return 0;
}

int default_add(struct pff_ps *        ps,
                struct mod_pff_entry * entry)
{
        struct pff_ps_priv *     priv;
        int result = 0;

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

int default_remove(struct pff_ps *        ps,
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

		/* Just remove the first alternative and ignore the others. */
                pfte_port_remove(tmp, alts->ports[0]);
	}

        /* If the list of port-ids is empty, remove the entry */
        if (list_empty(&tmp->ports)) {
                pfte_destroy(tmp, priv);
        }

        spin_unlock_bh(&priv->lock);

        return 0;
}

bool default_is_empty(struct pff_ps * ps)
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

static void __pff_flush(struct pff_ps_priv * priv)
{
        struct pft_entry * pos, * next;

        ASSERT(priv_is_ok(priv));

        list_for_each_entry_safe(pos, next, &priv->entries, next) {
                pfte_destroy(pos, priv);
        }
}

int default_flush(struct pff_ps * ps)
{
        struct pff_ps_priv * priv;

        priv = (struct pff_ps_priv *) ps->priv;
        if (!priv_is_ok(priv))
                return -1;

        spin_lock_bh(&priv->lock);

        __pff_flush(priv);

        spin_unlock_bh(&priv->lock);

        return 0;
}

int default_modify(struct pff_ps *    ps,
                   struct list_head * entries)
{
        struct pff_ps_priv *   priv;
        struct mod_pff_entry * entry;

        priv = (struct pff_ps_priv *) ps->priv;
        if (!priv_is_ok(priv))
                return -1;

        spin_lock_bh(&priv->lock);

        __pff_flush(priv);

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

int default_nhop(struct pff_ps * ps,
                 struct pci *    pci,
                 port_id_t **    ports,
                 size_t *        count)
{
        struct pff_ps_priv * priv;
        address_t            destination;
        qos_id_t             qos_id;
        struct pft_entry *   tmp;

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
        spin_lock_bh(&priv->lock);

        tmp = pft_find(priv, destination, qos_id);
        if (!tmp) {
                LOG_ERR("Could not find any entry for dest address: %u and "
                        "qos_id %d", destination, qos_id);
                spin_unlock_bh(&priv->lock);
                return -1;
        }

        if (pfte_ports_copy(tmp, ports, count)) {
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
			return -1;
		}

		alt->ports[0] = pft_pe_port(pos);
		alt->num_ports = cnt;

		list_add_tail(&alt->next, port_id_altlists);
        }

        return 0;
}

int default_dump(struct pff_ps *    ps,
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

static string_t * create_pff_wq_name(ipc_process_id_t id)
{
        char       string_ipcp_id[5];
        char 	   prefix[4] = "pff";
        string_t * wq_name;
        size_t     length;

        prefix[3] = 0;
        bzero(string_ipcp_id, sizeof(string_ipcp_id)); /* Be safe */
        snprintf(string_ipcp_id, sizeof(string_ipcp_id), "%d", id);

        length = strlen(prefix) +
                sizeof(char)            +
                strlen(string_ipcp_id)  +
                1 /* Terminator */;

        wq_name = rkmalloc(length, GFP_KERNEL);
        if (!wq_name)
                return NULL;

        wq_name[0] = 0;
        strcat(wq_name, prefix);
        strcat(wq_name, "-");
        strcat(wq_name, string_ipcp_id);
        wq_name[length - 1] = 0; /* Final terminator */

        return wq_name;
}

struct ps_base *
pff_ps_default_create(struct rina_component * component)
{
        struct pff_ps * ps;
        struct pff_ps_priv * priv;
        struct pff * pff = pff_from_component(component);
        ipc_process_id_t ipc_process_id;
        struct ipcp_instance * ipcp;
        string_t * wq_name;

        priv = rkzalloc(sizeof(*priv), GFP_KERNEL);
        if (!priv) {
                return NULL;
        }

        spin_lock_init(&priv->lock);

        INIT_LIST_HEAD(&priv->entries);

        ipcp = pff_ipcp_get(pff);
        ipc_process_id = ipcp->ops->ipcp_id(ipcp->data);
        wq_name = create_pff_wq_name(ipc_process_id);
        priv->sysfs_wq = alloc_workqueue(wq_name,
        		WQ_MEM_RECLAIM | WQ_HIGHPRI | WQ_UNBOUND, 1);
        rkfree(wq_name);

        ps = rkzalloc(sizeof(*ps), GFP_KERNEL);
        if (!ps) {
                return NULL;
        }

        ps->base.set_policy_set_param = NULL; /* default */
        ps->dm = pff;
        ps->priv = (void *) priv;

        ps->pff_add = default_add;
        ps->pff_remove = default_remove;
	ps->pff_port_state_change = NULL;
        ps->pff_is_empty = default_is_empty;
        ps->pff_flush = default_flush;
        ps->pff_nhop = default_nhop;
        ps->pff_dump = default_dump;
        ps->pff_modify = default_modify;

        return &ps->base;
}
EXPORT_SYMBOL(pff_ps_default_create);

void pff_ps_default_destroy(struct ps_base * bps)
{
        struct pff_ps * ps = container_of(bps, struct pff_ps, base);

        if (bps) {
                struct pff_ps_priv * priv;

                priv = (struct pff_ps_priv *) ps->priv;
                if(!priv_is_ok(priv)) {
                        return;
                }

                spin_lock_bh(&priv->lock);

                __pff_flush(priv);

                spin_unlock_bh(&priv->lock);

                flush_workqueue(priv->sysfs_wq);
                destroy_workqueue(priv->sysfs_wq);

                rkfree(priv);
                rkfree(ps);
        }
}
EXPORT_SYMBOL(pff_ps_default_destroy);
