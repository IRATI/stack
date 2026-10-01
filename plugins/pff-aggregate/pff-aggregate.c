/*
 * Aggregate PDU forwarding policy set: longest-prefix lookup over a fixed
 * address hierarchy.
 *
 * The default PFF matches destination addresses exactly, so a router needs one
 * entry per IPC process it can reach. With topological addresses, everything
 * behind a gNB shares a prefix, and a core router should hold one entry per
 * prefix instead. Routing installs an entry under the prefix's base address
 * (the address with every bit below the prefix cleared); a lookup masks the
 * destination with each configured prefix length, longest first, and takes
 * the first entry it finds.
 *
 * Parameter
 *   prefixLengths  comma-separated prefix lengths in bits, e.g. "32,20,8".
 *                  32 is an exact match. Default "32", which behaves like the
 *                  default policy set.
 *
 * Address 0 is not a valid IRATI address, so a prefix whose base is 0 (a
 * default route) cannot be installed; give the top level of the hierarchy
 * non-zero values instead.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/hashtable.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/workqueue.h>

#define RINA_PREFIX "pff-aggregate"

#include "logs.h"
#include "rds/rmem.h"
#include "rds/robjects.h"
#include "debug.h"
#include "pff-ps.h"
#include "pci.h"

#define PFF_AGG_NAME      "aggregate"
#define PFF_AGG_MAX_LEVELS 8
#define PFF_AGG_HASH_BITS  8

struct agg_port {
	port_id_t        port_id;
	struct list_head next;
};

struct agg_entry {
	address_t         base;      /* destination, bits below the prefix cleared */
	qos_id_t          qos_id;    /* 0 matches any qos-id, as in the default */
	struct list_head  ports;
	struct hlist_node hnode;
	struct robject    robj;
};

struct agg_priv {
	spinlock_t                lock;
	DECLARE_HASHTABLE(table, PFF_AGG_HASH_BITS);
	unsigned int              nentries;
	/* Prefix lengths, longest first, and the matching masks. */
	unsigned int              nlevels;
	unsigned int              lengths[PFF_AGG_MAX_LEVELS];
	address_t                 masks[PFF_AGG_MAX_LEVELS];
	/* sysfs objects may sleep: add and remove them from an ordered queue, so
	 * an entry's removal never runs before its addition. */
	struct workqueue_struct * sysfs_wq;
	struct rset *             rset;
};

struct agg_sysfs_work {
	struct work_struct work;
	struct agg_entry * entry;
	struct rset *      rset;
	bool               add;
};

static address_t len_to_mask(unsigned int len)
{
	if (len == 0)
		return 0;
	if (len >= 32)
		return (address_t) ~0U;
	return (address_t) (~0U << (32 - len));
}

static u32 agg_key(address_t base, qos_id_t qos_id)
{ return (u32) base ^ ((u32) qos_id << 24); }

/* sysfs: dest_addr shows the base, prefix_len the length it was found at. */
static ssize_t agg_entry_attr_show(struct robject *        robj,
				   struct robj_attribute * attr,
				   char *                  buf)
{
	struct agg_entry * entry = container_of(robj, struct agg_entry, robj);
	struct agg_port *  pos;
	int                off = 0;

	if (strcmp(robject_attr_name(attr), "dest_addr") == 0)
		return sprintf(buf, "%u\n", entry->base);
	if (strcmp(robject_attr_name(attr), "qos_id") == 0)
		return sprintf(buf, "%u\n", entry->qos_id);
	if (strcmp(robject_attr_name(attr), "ports") == 0) {
		list_for_each_entry(pos, &entry->ports, next)
			off += sprintf(buf + off, "%u ", pos->port_id);
		if (off > 0)
			buf[off - 1] = '\n';
		return off;
	}
	return 0;
}
RINA_SYSFS_OPS(agg_entry);
RINA_ATTRS(agg_entry, dest_addr, qos_id, ports);
RINA_KTYPE(agg_entry);

static void agg_sysfs_worker(struct work_struct * w)
{
	struct agg_sysfs_work * sw = container_of(w, struct agg_sysfs_work, work);
	struct agg_port *       pos, * next;

	if (sw->add) {
		if (robject_rset_add(&sw->entry->robj, sw->rset, "%u-%u",
				     sw->entry->base, sw->entry->qos_id))
			LOG_WARN("Could not add sysfs object for %u-%u",
				 sw->entry->base, sw->entry->qos_id);
	} else {
		/* The entry left the table under the lock; only sysfs and
		 * this worker still see it. */
		if (sw->entry->robj.kobj.state_in_sysfs)
			robject_del(&sw->entry->robj);
		list_for_each_entry_safe(pos, next, &sw->entry->ports, next) {
			list_del(&pos->next);
			rkfree(pos);
		}
		rkfree(sw->entry);
	}
	rkfree(sw);
}

static void agg_sysfs_post(struct agg_priv * priv, struct agg_entry * entry,
			   bool add)
{
	struct agg_sysfs_work * sw;

	sw = rkzalloc(sizeof(*sw), GFP_ATOMIC);
	if (!sw) {
		/* Without sysfs bookkeeping a removed entry would leak; an
		 * added one just stays invisible. */
		LOG_ERR("Out of memory for sysfs work");
		return;
	}
	INIT_WORK(&sw->work, agg_sysfs_worker);
	sw->entry = entry;
	sw->rset  = priv->rset;
	sw->add   = add;
	queue_work(priv->sysfs_wq, &sw->work);
}

static struct agg_entry * agg_find_exact(struct agg_priv * priv,
					 address_t base, qos_id_t qos_id)
{
	struct agg_entry * pos;

	hash_for_each_possible(priv->table, pos, hnode, agg_key(base, qos_id)) {
		if (pos->base == base && pos->qos_id == qos_id)
			return pos;
	}
	return NULL;
}

/* Longest prefix first; at each level the exact qos-id, then the wildcard. */
static struct agg_entry * agg_lookup(struct agg_priv * priv,
				     address_t dst, qos_id_t qos_id)
{
	struct agg_entry * e;
	address_t          base;
	unsigned int       i;

	for (i = 0; i < priv->nlevels; i++) {
		base = dst & priv->masks[i];
		e = agg_find_exact(priv, base, qos_id);
		if (!e && qos_id != 0)
			e = agg_find_exact(priv, base, 0);
		if (e)
			return e;
	}
	return NULL;
}

static void agg_entry_unlink(struct agg_priv * priv, struct agg_entry * e)
{
	hash_del(&e->hnode);
	priv->nentries--;
	agg_sysfs_post(priv, e, false);
}

static int agg_port_add(struct agg_entry * e, port_id_t id)
{
	struct agg_port * p;

	list_for_each_entry(p, &e->ports, next) {
		if (p->port_id == id)
			return 0;
	}
	p = rkmalloc(sizeof(*p), GFP_ATOMIC);
	if (!p)
		return -1;
	p->port_id = id;
	list_add_tail(&p->next, &e->ports);
	return 0;
}

static void agg_port_remove(struct agg_entry * e, port_id_t id)
{
	struct agg_port * p, * n;

	list_for_each_entry_safe(p, n, &e->ports, next) {
		if (p->port_id == id) {
			list_del(&p->next);
			rkfree(p);
			return;
		}
	}
}

static int __agg_add(struct agg_priv * priv, struct mod_pff_entry * entry)
{
	struct agg_entry *       e;
	struct port_id_altlist * alts;
	bool                     created = false;

	e = agg_find_exact(priv, entry->fwd_info, entry->qos_id);
	if (!e) {
		e = rkzalloc(sizeof(*e), GFP_ATOMIC);
		if (!e)
			return -1;
		e->base   = entry->fwd_info;
		e->qos_id = entry->qos_id;
		INIT_LIST_HEAD(&e->ports);
		robject_init(&e->robj, &agg_entry_rtype);
		created = true;
	}

	list_for_each_entry(alts, &entry->port_id_altlists, next) {
		if (alts->num_ports < 1)
			continue;
		/* Like the default: first alternative only. */
		if (agg_port_add(e, alts->ports[0])) {
			if (created)
				rkfree(e);
			return -1;
		}
	}

	if (created) {
		hash_add(priv->table, &e->hnode, agg_key(e->base, e->qos_id));
		priv->nentries++;
		agg_sysfs_post(priv, e, true);
	}
	return 0;
}

static int agg_add(struct pff_ps * ps, struct mod_pff_entry * entry)
{
	struct agg_priv * priv = ps->priv;
	int               ret;

	if (!priv || !entry || !is_address_ok(entry->fwd_info) ||
	    !is_qos_id_ok(entry->qos_id))
		return -1;

	spin_lock_bh(&priv->lock);
	ret = __agg_add(priv, entry);
	spin_unlock_bh(&priv->lock);
	return ret;
}

static int agg_remove(struct pff_ps * ps, struct mod_pff_entry * entry)
{
	struct agg_priv *        priv = ps->priv;
	struct agg_entry *       e;
	struct port_id_altlist * alts;

	if (!priv || !entry || !is_address_ok(entry->fwd_info) ||
	    !is_qos_id_ok(entry->qos_id))
		return -1;

	spin_lock_bh(&priv->lock);
	e = agg_find_exact(priv, entry->fwd_info, entry->qos_id);
	if (!e) {
		spin_unlock_bh(&priv->lock);
		return -1;
	}
	list_for_each_entry(alts, &entry->port_id_altlists, next) {
		if (alts->num_ports >= 1)
			agg_port_remove(e, alts->ports[0]);
	}
	if (list_empty(&e->ports))
		agg_entry_unlink(priv, e);
	spin_unlock_bh(&priv->lock);
	return 0;
}

static bool agg_is_empty(struct pff_ps * ps)
{
	struct agg_priv * priv = ps->priv;
	bool              empty;

	if (!priv)
		return false;
	spin_lock_bh(&priv->lock);
	empty = priv->nentries == 0;
	spin_unlock_bh(&priv->lock);
	return empty;
}

static void __agg_flush(struct agg_priv * priv)
{
	struct agg_entry *  e;
	struct hlist_node * tmp;
	int                 bkt;

	hash_for_each_safe(priv->table, bkt, tmp, e, hnode)
		agg_entry_unlink(priv, e);
}

static int agg_flush(struct pff_ps * ps)
{
	struct agg_priv * priv = ps->priv;

	if (!priv)
		return -1;
	spin_lock_bh(&priv->lock);
	__agg_flush(priv);
	spin_unlock_bh(&priv->lock);
	return 0;
}

static int agg_modify(struct pff_ps * ps, struct list_head * entries)
{
	struct agg_priv *      priv = ps->priv;
	struct mod_pff_entry * entry;

	if (!priv)
		return -1;
	spin_lock_bh(&priv->lock);
	__agg_flush(priv);
	list_for_each_entry(entry, entries, next) {
		if (!is_address_ok(entry->fwd_info) ||
		    !is_qos_id_ok(entry->qos_id))
			continue;
		__agg_add(priv, entry);
	}
	spin_unlock_bh(&priv->lock);
	return 0;
}

static int agg_nhop(struct pff_ps * ps, struct pci * pci,
		    port_id_t ** ports, size_t * count)
{
	struct agg_priv *  priv = ps->priv;
	struct agg_entry * e;
	struct agg_port *  p;
	address_t          dst;
	qos_id_t           qos_id;
	size_t             n = 0, i = 0;

	if (!priv || !ports || !count)
		return -1;

	dst    = pci_destination(pci);
	qos_id = pci_qos_id(pci);
	if (!is_address_ok(dst) || !is_qos_id_ok(qos_id))
		return -1;

	spin_lock_bh(&priv->lock);
	e = agg_lookup(priv, dst, qos_id);
	if (!e) {
		spin_unlock_bh(&priv->lock);
		LOG_DBG("No entry covers dest address %u, qos-id %u", dst, qos_id);
		return -1;
	}

	list_for_each_entry(p, &e->ports, next)
		n++;
	/* Same in-out contract as the default: *ports holds *count ports. */
	if (*count != n) {
		if (*count > 0)
			rkfree(*ports);
		*ports = NULL;
		if (n > 0) {
			*ports = rkmalloc(n * sizeof(**ports), GFP_ATOMIC);
			if (!*ports) {
				*count = 0;
				spin_unlock_bh(&priv->lock);
				return -1;
			}
		}
		*count = n;
	}
	list_for_each_entry(p, &e->ports, next)
		(*ports)[i++] = p->port_id;
	spin_unlock_bh(&priv->lock);
	return 0;
}

static int agg_dump(struct pff_ps * ps, struct list_head * entries)
{
	struct agg_priv *        priv = ps->priv;
	struct agg_entry *       e;
	struct agg_port *        p;
	struct mod_pff_entry *   m;
	struct port_id_altlist * alt;
	int                      bkt;

	if (!priv)
		return -1;
	spin_lock_bh(&priv->lock);
	hash_for_each(priv->table, bkt, e, hnode) {
		m = rkmalloc(sizeof(*m), GFP_ATOMIC);
		if (!m)
			goto oom;
		m->fwd_info = e->base;
		m->qos_id   = e->qos_id;
		INIT_LIST_HEAD(&m->port_id_altlists);
		list_add(&m->next, entries);
		list_for_each_entry(p, &e->ports, next) {
			alt = rkmalloc(sizeof(*alt), GFP_ATOMIC);
			if (!alt)
				goto oom;
			alt->ports = rkmalloc(sizeof(*alt->ports), GFP_ATOMIC);
			if (!alt->ports) {
				rkfree(alt);
				goto oom;
			}
			alt->ports[0]  = p->port_id;
			alt->num_ports = 1;
			list_add_tail(&alt->next, &m->port_id_altlists);
		}
	}
	spin_unlock_bh(&priv->lock);
	return 0;
oom:
	/* Entries added so far stay on the caller's list, which owns them. */
	spin_unlock_bh(&priv->lock);
	return -1;
}

/* "32,20,8": lengths in bits, stored longest first. */
static int agg_parse_lengths(struct agg_priv * priv, const char * value)
{
	unsigned int lengths[PFF_AGG_MAX_LEVELS], n = 0, i, j, t;
	char *       copy, * cur, * tok;
	int          ret = 0;

	copy = kstrdup(value, GFP_KERNEL);
	if (!copy)
		return -1;
	cur = copy;
	while ((tok = strsep(&cur, ",")) != NULL) {
		if (*tok == '\0')
			continue;
		if (n == PFF_AGG_MAX_LEVELS || kstrtouint(tok, 10, &lengths[n]) ||
		    lengths[n] > 32) {
			ret = -1;
			goto out;
		}
		n++;
	}
	if (n == 0) {
		ret = -1;
		goto out;
	}
	for (i = 0; i < n; i++)
		for (j = i + 1; j < n; j++)
			if (lengths[j] > lengths[i]) {
				t = lengths[i]; lengths[i] = lengths[j]; lengths[j] = t;
			}

	spin_lock_bh(&priv->lock);
	priv->nlevels = n;
	for (i = 0; i < n; i++) {
		priv->lengths[i] = lengths[i];
		priv->masks[i]   = len_to_mask(lengths[i]);
	}
	spin_unlock_bh(&priv->lock);
out:
	kfree(copy);
	return ret;
}

static int agg_set_param(struct ps_base * bps, const char * name,
			 const char * value)
{
	struct pff_ps *   ps   = container_of(bps, struct pff_ps, base);
	struct agg_priv * priv = ps->priv;

	if (!name || !value)
		return -1;
	if (strcmp(name, "prefixLengths") == 0) {
		if (agg_parse_lengths(priv, value)) {
			LOG_ERR("Bad prefixLengths '%s'", value);
			return -1;
		}
		LOG_INFO("prefixLengths set to %s", value);
		return 0;
	}
	LOG_ERR("No such parameter: %s", name);
	return -1;
}

static struct ps_base * agg_create(struct rina_component * component)
{
	struct pff *      pff = pff_from_component(component);
	struct pff_ps *   ps;
	struct agg_priv * priv;

	priv = rkzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return NULL;
	spin_lock_init(&priv->lock);
	hash_init(priv->table);
	priv->nlevels    = 1;
	priv->lengths[0] = 32;
	priv->masks[0]   = len_to_mask(32);
	priv->rset       = pff_rset(pff);
	priv->sysfs_wq   = alloc_ordered_workqueue("pff-agg", WQ_MEM_RECLAIM);
	if (!priv->sysfs_wq) {
		rkfree(priv);
		return NULL;
	}

	ps = rkzalloc(sizeof(*ps), GFP_KERNEL);
	if (!ps) {
		destroy_workqueue(priv->sysfs_wq);
		rkfree(priv);
		return NULL;
	}
	ps->base.set_policy_set_param = agg_set_param;
	ps->dm                    = pff;
	ps->priv                  = priv;
	ps->pff_add               = agg_add;
	ps->pff_remove            = agg_remove;
	ps->pff_port_state_change = NULL;
	ps->pff_is_empty          = agg_is_empty;
	ps->pff_flush             = agg_flush;
	ps->pff_nhop              = agg_nhop;
	ps->pff_dump              = agg_dump;
	ps->pff_modify            = agg_modify;
	return &ps->base;
}

static void agg_destroy(struct ps_base * bps)
{
	struct pff_ps *   ps;
	struct agg_priv * priv;

	if (!bps)
		return;
	ps   = container_of(bps, struct pff_ps, base);
	priv = ps->priv;
	if (priv) {
		spin_lock_bh(&priv->lock);
		__agg_flush(priv);
		spin_unlock_bh(&priv->lock);
		/* Runs every queued removal, which frees the entries. */
		destroy_workqueue(priv->sysfs_wq);
		rkfree(priv);
	}
	rkfree(ps);
}

static struct ps_factory agg_factory = {
	.owner   = THIS_MODULE,
	.create  = agg_create,
	.destroy = agg_destroy,
};

static int __init mod_init(void)
{
	strscpy(agg_factory.name, PFF_AGG_NAME, sizeof(agg_factory.name));
	if (pff_ps_publish(&agg_factory)) {
		LOG_ERR("Failed to publish the aggregate PFF policy set");
		return -1;
	}
	LOG_INFO("Aggregate PFF policy set loaded");
	return 0;
}

static void __exit mod_exit(void)
{
	if (pff_ps_unpublish(PFF_AGG_NAME))
		LOG_ERR("Failed to unpublish the aggregate PFF policy set");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PFF policy set: longest-prefix lookup over an address hierarchy");
MODULE_AUTHOR("Sergio Giménez <hola@sergiogimenez.com>");
