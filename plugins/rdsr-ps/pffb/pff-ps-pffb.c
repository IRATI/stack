/* This program is free software; you can redistribute it and/or modify
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
 *
 * Author: Kewin Rausch <kewin.rausch@create-net.org>
 *
 * This file is 80 character wide.
 * Use monospace font for better visualization.
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *
 */

#include <linux/kobject.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/crc16.h>
#include <linux/random.h>
#include <linux/list.h>
#include <linux/types.h>
#include <linux/slab.h>

#define RINA_PREFIX 	"pff-ps-pffb"

#include "logs.h"
#include "rds/rmem.h"
#include "pff-ps.h"
#include "pff.h"
#include "debug.h"

#define PFFB_NAME 		"pffb"

/* How much pass before considering a flow timed out, for us? (in ms) */
#define PFFB_TIMEOUT 		60000

/* Distribution of new flow takes in account how many flows a port is serving.
 */
#define PFFB_NOF_DIST 		0

/* A random port is picked for the distribution.
 */
#define PFFB_RAND_DIST 		1

/*
 * Data structure:
 */

struct connection_id {
	address_t pci_source;
	address_t pci_destination;
	address_t pci_qos_id;
	cep_id_t  pci_cep_source;
	cep_id_t  pci_cep_destination;
};

/* Flow info. */
struct flow_info {
	struct list_head next;
	struct timespec la;
	unsigned short key;
	unsigned int rate;
	port_id_t port_id;
};

struct pft_port_entry {
	struct list_head next;
        port_id_t        port_id;
};

/* Master private data structure for the policy set. */
struct pff_ps_priv {
        spinlock_t       lock;
        struct list_head entries;
};

/* Definition for one particular destination. */
struct pft_entry {
        address_t destination;
        qos_id_t qos_id;
        /* Last port used; for water distribution. */
        unsigned int lpu;
        struct list_head ports;
        /* Num. of ports. */
        int nop;
        struct list_head next;
};

/*
 * Globals:
 */

/* How much ms pass before considering it as timed out? */
static unsigned int pffb_flow_timeout = PFFB_TIMEOUT;

/* Full renew when a flow expires?
static unsigned int pffb_full_renew = 0;
 */

/* Flow distribution strategy? */
static int pffb_distr = PFFB_RAND_DIST;

static struct timespec pffb_load = {0};

#define PFFB_NOP 64

static unsigned int pffb_flows[PFFB_NOP] = {0};
static LIST_HEAD(pffb_flows_info);

/*
 * PFFB sysfs:
 */

/* Kernel object for the policy. */
struct kobject pffb_obj;

/* Attribute used in PFFB policy. */
struct pffb_attribute {
	struct attribute attr;
	ssize_t (* show)(
		struct kobject * kobj,
		struct attribute * attr,
		char * buf);
	ssize_t (* store)(
		struct kobject * foo,
		struct attribute * attr,
		const char * buf,
		size_t count);
};

/* Show procedure! */
static ssize_t pffb_attr_show(
	struct kobject * kobj,
	struct attribute * attr,
	char * buf) {

	if(strcmp(attr->name, "fwd_type") == 0) {
		return snprintf(buf, PAGE_SIZE, "%u\n", pffb_distr);
	}

	return 0;
}

/* Store procedure! */
static ssize_t pffb_attr_store(
	struct kobject * kobj,
	struct attribute * attr,
	const char * buf,
	size_t count) {

	int ft = 0;
	int op = 0;

        /* Cleanup must happens before using a different strategy!
        pffb_cleanup = 1;
         */

	if(strcmp(attr->name, "fwd_type") == 0) {
		op = kstrtoint(buf, 10, &ft);

		if(op < 0) {
			LOG_ERR("Failed to set the reset rate.");
			return op;
		}

		/* Single flow rate is now changed! */
		pffb_distr = ft;
	}

	/* Renew the infos here...
	if(pffb_distr == PFFB_NOF_DIST) {
		memset(pffb_flows, 0, sizeof(unsigned int) * PFFB_NOP);
	}
	 */

	return count;
}

/* Sysfs operations. */
static const struct sysfs_ops pffb_sysfs_ops = {
	.show  = pffb_attr_show,
	.store = pffb_attr_store,
};

/*
 * Attributes visible in PFFB sysfs directory.
 */

static struct pffb_attribute pffb_ft_attr =
	__ATTR(fwd_type, 0664, pffb_attr_show, pffb_attr_store);

static struct attribute * pffb_attrs[] = {
	&pffb_ft_attr.attr,
	NULL,
};

/* Operations associated with the release of the kobj. */
static void pffb_release(struct kobject *kobj) {
	/* Nothing... */
}

/* Master PFFB ktype, different for the type of shown attributes. */
static struct kobj_type pffb_ktype = {
	.sysfs_ops = &pffb_sysfs_ops,
	.release = pffb_release,
	.default_attrs = pffb_attrs,
};
/*
 * Port-status related:
 */

/* Timespec to ms. */
static inline unsigned long pffb_to_ms(struct timespec * t) {
	return (t->tv_sec * 1000) + (t->tv_nsec / 1000000);
}

/* Just remove the entry... */
static void __pffb_rem_flow(struct flow_info * f) {
	list_del(&f->next);
	rkfree(f);
}

/* Remove a flow entry with a little logic. */
static void pffb_rem_flow(
	struct pft_port_entry * port_e,
	struct flow_info * flow) {

	if(pffb_distr == PFFB_NOF_DIST) {
		pffb_flows[flow->port_id] -= flow->rate;
	}

	LOG_INFO("PFFBI > %x expired on port %d, remains %d",
		flow->key, flow->port_id, pffb_flows[flow->port_id]);

	__pffb_rem_flow(flow);

	return;
}

/* Find and update the flow entry. On expiration the entry is removed.
 * Returns -1 if the flow has not found (and so is new).
 */
static struct pft_port_entry * pffb_find_key(
	unsigned short key,
	struct pft_entry * entry) {

	struct pft_port_entry * p = 0;
	struct flow_info * f = 0;
	struct flow_info * t = 0;
	struct timespec now;

	port_id_t pid = -1;

	switch(pffb_distr) {
	default:
		getnstimeofday(&now); /* Only once! */

		list_for_each_entry_safe(f, t, &pffb_flows_info, next) {
			if(pffb_to_ms(&now) -
				pffb_to_ms(&f->la) >
				pffb_flow_timeout) {

				LOG_INFO("PFFBI > %x expired!", f->key);
				pffb_rem_flow(p, f);
			}

			if(f->key == key) {
				f->la.tv_sec  = now.tv_sec;
				f->la.tv_nsec = now.tv_nsec;

				pid = f->port_id;
				break;
			}
		}

		if(pid == -1) {
			return 0;
		}

		list_for_each_entry(p, &entry->ports, next) {
			if(p->port_id == pid) {
				return p;
			}
		}
		break;
	}

	return 0;
}

static struct pft_port_entry * pffb_add_key(
	unsigned short key,
	unsigned int rate,
	struct pft_entry * entry,
	struct flow_info ** fi) {

	struct flow_info * f = 0;
	struct pft_port_entry * p = 0;
	struct pft_port_entry * r = 0;
	struct timespec now;

	unsigned int rnd = 0;
	int t = INT_MAX;

	switch(pffb_distr) {
	/* "Pick a random port" distribution. */
	case PFFB_RAND_DIST:
		f = rkzalloc(sizeof(struct flow_info), GFP_KERNEL);

		if(!f) {
			LOG_ERR("No more memory!");
			return 0;
		}

		getnstimeofday(&now);

		INIT_LIST_HEAD(&f->next);
		f->key = key;
		f->la.tv_sec = now.tv_sec;
		f->la.tv_nsec = now.tv_nsec;
		f->rate = rate;

		get_random_bytes(&rnd, sizeof(unsigned int));

		t = 0;
		list_for_each_entry(p, &entry->ports, next) {
			t++;
		}

		LOG_INFO("PFFBI Rnd=%u, ports=%u, index=%u", rnd, t, rnd % t);

		/* Stay within the range. */
		rnd = rnd % t;

		/* Select the port. */
		t = 0;
		list_for_each_entry(p, &entry->ports, next) {
			if(t == rnd) {
				r = p;
			}

			t++;
		}

		if(!r) {
			LOG_ERR("Next port NOT selected, %u", rnd);
			rkfree(f);
			return 0;
		}

		f->port_id = r->port_id;
		list_add(&f->next, &pffb_flows_info);
		*fi = f;

		return r;

	/* Default is the NOF distribution. */
	default:
		/* Select the less 'used' one, whatever used means to you... */
		list_for_each_entry(p, &entry->ports, next) {
			if(p->port_id < PFFB_NOP && p->port_id > 0) {
				if(pffb_flows[p->port_id] < t) {
					t = pffb_flows[p->port_id];
					r = p;
				}
			}
		}

		f = rkzalloc(sizeof(struct flow_info), GFP_KERNEL);

		if(!f) {
			LOG_ERR("No more memory!");
			return 0;
		}

		getnstimeofday(&now);

		INIT_LIST_HEAD(&f->next);
		f->key = key;
		f->la.tv_sec = now.tv_sec;
		f->la.tv_nsec = now.tv_nsec;
		f->rate = rate;

		if(!r) {
			rkfree(f);
			return 0;
		}

		f->port_id = r->port_id;
		list_add(&f->next, &pffb_flows_info);
		pffb_flows[f->port_id] += rate;
		*fi = f;

		return r;
	}

	return 0;
}

/*
 * Procedures:
 */

/* Create a new port entry... with flags... */
static struct pft_port_entry * pft_pe_create_gfp(
	gfp_t flags, port_id_t port_id) {

	struct pft_port_entry * tmp = 0;

        tmp = rkmalloc(sizeof(*tmp), flags);

        if (!tmp) {
                return NULL;
        }

        tmp->port_id = port_id;
        INIT_LIST_HEAD(&tmp->next);

        return tmp;
}

/* Create a new port entry... */
static struct pft_port_entry * pft_pe_create_ni(port_id_t port_id) {
	return pft_pe_create_gfp(GFP_ATOMIC, port_id);
}

/* Destroy the port entry. */
static void pft_pe_destroy(struct pft_port_entry * pe) {
        list_del(&pe->next);
        rkfree(pe);
}

/* Returns brand new, initialized, entry... with flags... */
static struct pft_entry * pfte_create_gfp(
	gfp_t flags, address_t destination, qos_id_t qos_id) {

	struct pft_entry * tmp = 0;

        tmp = rkmalloc(sizeof(*tmp), flags);

        if (!tmp) {
                return NULL;
        }

        tmp->destination = destination;
        tmp->qos_id = qos_id;
        tmp->nop = -1;

        INIT_LIST_HEAD(&tmp->ports);
        INIT_LIST_HEAD(&tmp->next);

        return tmp;
}

/* Returns brand new, initialized, entry. */
static struct pft_entry * pfte_create_ni(
	address_t destination, qos_id_t qos_id) {

	return pfte_create_gfp(GFP_ATOMIC, destination, qos_id);
}

/* Destroy da entry... */
static void pfte_destroy(struct pft_entry * entry) {
        struct pft_port_entry * pos  = 0;
        struct pft_port_entry * next = 0;

        if(!entry) {
        	LOG_WARN("NULL entry to remove.");
        	return;
        }

        list_for_each_entry_safe(pos, next, &entry->ports, next) {
                pft_pe_destroy(pos);
        }

        list_del(&entry->next);
        rkfree(entry);
}

/* Find da port. */
static struct pft_port_entry * pfte_port_find(
	struct pft_entry * entry, port_id_t id) {

	struct pft_port_entry * pos = 0;

        list_for_each_entry(pos, &entry->ports, next) {
                if (pos->port_id == id) {
                        return pos;
                }
        }

        return NULL;
}

/* Adds da port. */
static int pfte_port_add(struct pft_entry * entry, port_id_t id) {
        struct pft_port_entry * pe = 0;

        pe = pfte_port_find(entry, id);

        if (pe) {
                return 0;
        }

        pe = pft_pe_create_ni(id);

        if (!pe) {
                return -1;
        }

        list_add(&pe->next, &entry->ports);

        return 0;
}

/* Removes da port. */
static void pfte_port_remove(struct pft_entry * entry, port_id_t id) {
        struct pft_port_entry * pos  = 0;
        struct pft_port_entry * next = 0;

        if(!entry) {
        	LOG_WARN("NULL entry to remove port from...");
        	return;
        }

        list_for_each_entry_safe(pos, next, &entry->ports, next) {
                if (pos->port_id == id) {
                        pft_pe_destroy(pos);
                        return;
                }
        }

}

/* Aka: actually copying only the first port, even if allocates space for every
 * one.
 */
static int pfte_ports_copy(
	struct  pft_port_entry * port,
	port_id_t ** port_ids,
	size_t * entries) {

        size_t	count = 1;

        if(!entries) {
        	LOG_ERR("Passing null? Where do I supposed to copy this?!?");
        	return -1;
        }

        if (*entries != count) {
                if (*entries > 0) {
                        rkfree(*port_ids);
                }

                if (count > 0) {
                        *port_ids = rkmalloc(
				sizeof(port_id_t) * count, GFP_ATOMIC);

                        if (!*port_ids) {
                                *entries = 0;
                                return -1;
                        }
                }

                *entries = count;
        }

        (*port_ids)[0] = port->port_id;

        return 0;
}

/* Finds an entry starting from its address and qos... */
static struct pft_entry * pft_find(
	struct pff_ps_priv * priv,
	address_t destination,
	qos_id_t qos_id) {

        struct pft_entry * pos = 0;

        if(!priv) {
        	LOG_WARN("Could not get the private data! :S");
        	return 0;
        }

        list_for_each_entry(pos, &priv->entries, next) {
		if ((pos->destination == destination) &&
                    (pos->qos_id == qos_id)) {

			LOG_DBG("Match found!");
			return pos;
                }
        }
	
        return NULL;
}

/* Do the dirty flush job, but must be protected... "Lock me Onii-chan!" */
static void pft_do_flush(struct pff_ps_priv * priv) {
        struct pft_entry * pos  = 0;
        struct pft_entry * next = 0;

        struct flow_info * f = 0;
        struct flow_info * t = 0;

        list_for_each_entry_safe(f, t, &pffb_flows_info, next) {
		list_del(&f->next);
		rkfree(f);
	}

        list_for_each_entry_safe(pos, next, &priv->entries, next) {
                pfte_destroy(pos);
        }
}

/* Adds an entry... */
static int pffb_add(struct pff_ps * ps, struct mod_pff_entry * entry) {
        struct pff_ps_priv * priv = 0;
        struct pft_entry * tmp = 0;
	struct port_id_altlist * alts = 0;

        priv = (struct pff_ps_priv *) ps->priv;

        if (!priv) {
        	LOG_WARN("Could not get the private data!");
                return -1;
        }

        if (!entry) {
                LOG_ERR("Null entry, won't add...");
                return -1;
        }

        if (!is_address_ok(entry->fwd_info) || !is_qos_id_ok(entry->qos_id)) {
                LOG_ERR("Wrong entry keys, addr:%d, qos:%d",
			entry->fwd_info, entry->qos_id);

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
        }

	list_for_each_entry(alts, &entry->port_id_altlists, next) {
		if (alts->num_ports < 1) {
			LOG_INFO("Port id alternative set is empty");
			continue;
		}

                if (pfte_port_add(tmp, alts->ports[0])) {
                        pfte_destroy(tmp);
                        spin_unlock(&priv->lock);
                        return -1;
                }
	}

        spin_unlock(&priv->lock);

        return 0;
}

/* Removes an entry... */
static int pffb_remove(struct pff_ps * ps, struct mod_pff_entry * entry) {
        struct pff_ps_priv * priv = 0;
        struct port_id_altlist * alts = 0;
        struct pft_entry * tmp = 0;

        priv = (struct pff_ps_priv *) ps->priv;

        if (!priv) {
        	LOG_WARN("Could not get the private data! :S");
                return -1;
        }

        if (!entry) {
                LOG_ERR("NULL entry!");
                return -1;
        }

        if (!is_address_ok(entry->fwd_info) || !is_qos_id_ok(entry->qos_id)) {
                LOG_ERR("Wrong keys for entry, addr:%d, qos:%d",
                	entry->fwd_info,
                	entry->qos_id);

                return -1;
        }

        spin_lock(&priv->lock);

        tmp = pft_find(priv, entry->fwd_info, entry->qos_id);

        if (!tmp) {
                spin_unlock(&priv->lock);
                return -1;
        }

	list_for_each_entry(alts, &entry->port_id_altlists, next) {
		if (alts->num_ports < 1) {
			LOG_INFO("Port id alternative set is empty");
			continue;
		}

		/* Just the first is used by now... */
		pfte_port_remove(tmp, alts->ports[0]);
	}

        if (list_empty(&tmp->ports)) {
                pfte_destroy(tmp);
        }
	
        spin_unlock(&priv->lock);

        return 0;
}

/* The title say everything... */
static bool pffb_is_empty(struct pff_ps * ps) {
        struct pff_ps_priv * priv = 0;
        bool empty = false;

        priv = (struct pff_ps_priv *) ps->priv;

        if (!priv) {
                return false;
        }

        spin_lock(&priv->lock);
        empty = list_empty(&priv->entries);
        spin_unlock(&priv->lock);

        return empty;
}

/* Flush out fwd information. */
static int pffb_flush(struct pff_ps * ps) {
        struct pff_ps_priv * priv;

        priv = (struct pff_ps_priv *) ps->priv;
        if (!priv) {
                return -1;
        }

        spin_lock(&priv->lock);
        pft_do_flush(priv);
        spin_unlock(&priv->lock);

        return 0;
}

/* Select the right entry for the next hop alternatives. */
struct pft_port_entry * pffb_select_entry(
	struct pft_entry * entry, struct pci * pci, struct pff_ps_priv * priv) {

	struct pft_port_entry * ret = 0;
	unsigned short key = 0;
	struct connection_id c_id = {0};
	struct flow_info * fi = 0;
	unsigned int rt = 0;
	int m = 0;

	c_id.pci_source = pci_source(pci);
	c_id.pci_cep_source = pci_cep_source(pci);
	c_id.pci_destination = pci_destination(pci);
	c_id.pci_cep_destination = pci_cep_destination(pci);
	c_id.pci_qos_id = pci_qos_id(pci);

#if 0
	if(pci_type(pci) == PDU_TYPE_MGMT) {
		m = 1;
		rt = 0;
	} else {
		/* Rate of the flow (it's weight). */
		rt = 1;
	}
#endif

	rt = 1; /* Weight of the flow. */
	key = crc16(0, (const u8 *)&c_id, sizeof(struct connection_id));
	ret = pffb_find_key(key, entry);

	/* New entry...
	 * NOTE: some fwd type will never enter here, for example
	 * 	- Water distribution.
	 */
	if(!ret) {
		ret = pffb_add_key(key, rt, entry, &fi);

		if(!ret) {
			LOG_ALERT("Could not select next HOP for %x!", key);
		} else {
			if(!fi) {
				goto out;
			}

			LOG_INFO("PFFBI > (%d-%u <--> %d-%u, qos:%d), "
				"hash=%x, "
				"isM?=%d, "
				"on=%lu.%lu, "
				"port=%d, "
				"flows=%d",
				c_id.pci_source,
				c_id.pci_cep_source,
				c_id.pci_destination,
				c_id.pci_cep_destination,
				entry->qos_id,
				key,
				m,
				fi->la.tv_sec - pffb_load.tv_sec,
				fi->la.tv_nsec / 1000000,
				ret->port_id,
				pffb_flows[ret->port_id]);
		}
	}

out:
        return ret;
}

/* Select the next hop for a certain PDU. */
static int pffb_next_hop(
	struct pff_ps * ps,
	struct pci * pci,
	port_id_t ** ports,
	size_t * count) {

        address_t destination = -1;
        qos_id_t qos_id = -1;
        unsigned long flags = 0;

        struct pff_ps_priv * priv = 0;
        struct pft_entry * tmp = 0;
	struct pft_port_entry * port = 0;

        priv = (struct pff_ps_priv *) ps->priv;

        if (!priv) {
        	LOG_WARN("Could not get private data... :S");
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
                LOG_ERR("No entry for addr: %u, qos: %d", destination, qos_id);

                spin_unlock_irqrestore(&priv->lock, flags);
                return -1;
        }

        port = pffb_select_entry(tmp, pci, priv);

        if(!port) {
        	LOG_ERR("Could not select next port!");
        	spin_unlock_irqrestore(&priv->lock, flags);
        	return -1;
        }

        /* Copy in the given pointer array the port id. */
        if (pfte_ports_copy(port, ports, count)) {
                spin_unlock_irqrestore(&priv->lock, flags);
                return -1;
        }

        spin_unlock_irqrestore(&priv->lock, flags);  
	
	return 0;
}

/* Copies the port alternative list. */
static int pfte_copy_alts(
	struct pft_entry * entry, struct list_head * port_id_altlists) {

	struct pft_port_entry * pos = 0;
	struct port_id_altlist * alt = 0;
	int cnt = 1;

        list_for_each_entry(pos, &entry->ports, next) {
		alt = rkmalloc(sizeof(*alt), GFP_ATOMIC);

		if (!alt) {
			LOG_ERR("No more memory!");
			return -1;
		}

		alt->ports = rkmalloc(cnt * sizeof(*(alt->ports)), GFP_ATOMIC);

		if (!alt->ports) {
			LOG_ERR("No more memory!");
			rkfree(alt);
			return -1;
		}

		alt->ports[0]  = pos->port_id;
		alt->num_ports = cnt;

		list_add_tail(&alt->next, port_id_altlists);
        }

        return 0;
}

/* Clone the status of the policy in the given list. */
static int pffb_dump(struct pff_ps * ps, struct list_head * entries) {
        struct pff_ps_priv * priv = 0;
        struct pft_entry * pos = 0;
        struct mod_pff_entry * entry = 0;

        priv = (struct pff_ps_priv *) ps->priv;

        if (!priv) {
        	LOG_ERR("No private data found!");
                return -1;
        }

        spin_lock(&priv->lock);
        list_for_each_entry(pos, &priv->entries, next) {
                entry = rkmalloc(sizeof(*entry), GFP_ATOMIC);

                if (!entry) {
                	LOG_ERR("No more memory!");
                        spin_unlock(&priv->lock);
                        return -1;
                }

                entry->fwd_info = pos->destination;
                entry->qos_id = pos->qos_id;
		INIT_LIST_HEAD(&entry->port_id_altlists);

                if (pfte_copy_alts(pos, &entry->port_id_altlists)) {
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
static int pffb_set_param(
	struct ps_base * bps, const char * name, const char * value) {

	/*
        struct pff_ps * ps = container_of(bps, struct pff_ps, base);
        struct pff_ps_priv * priv = ps->priv;
	 */

        if (!name || !value) {
                LOG_ERR("Some parameters are null, name=0x%pK, value=0x%pK",
			name, value);
                return -1;
        }

        LOG_ERR("%s not recognized as parametric policy...", name);

        return -1;
}

static struct ps_base * pffb_create(struct rina_component * component) {
        struct pff_ps * ps;
        struct pff_ps_priv * priv;
        struct pff * pff = pff_from_component(component);
        struct timespec n;

        priv = rkzalloc(sizeof(*priv), GFP_KERNEL);

        if (!priv) {
        	LOG_ERR("No more memory!");
                return NULL;
        }

        spin_lock_init(&priv->lock);
        INIT_LIST_HEAD(&priv->entries);

        ps = rkzalloc(sizeof(*ps), GFP_KERNEL);

        if (!ps) {
        	LOG_ERR("No more memory!");
        	rkfree(priv);
                return NULL;
        }

        getnstimeofday(&n);
        pffb_load.tv_sec  = n.tv_sec;
        pffb_load.tv_nsec = n.tv_nsec;

        ps->base.set_policy_set_param = pffb_set_param;
        ps->dm = pff;
        ps->priv = (void *) priv;
        ps->pff_add = pffb_add;
        ps->pff_remove = pffb_remove;
	ps->pff_port_state_change = NULL;
        ps->pff_is_empty = pffb_is_empty;
        ps->pff_flush = pffb_flush;
        ps->pff_nhop = pffb_next_hop;
        ps->pff_dump = pffb_dump;

	LOG_DBG("PFFB instance created...");

        return &ps->base;
}

static void pffb_destroy(struct ps_base * bps) {
	struct pff_ps * ps = container_of(bps, struct pff_ps, base);
	struct pff_ps_priv * priv;

	priv = (struct pff_ps_priv *) ps->priv;

	if(!priv) {
		LOG_ERR("Could not get the private data!");
		return;
	}

	spin_lock(&priv->lock);
	pft_do_flush(priv);
	spin_unlock(&priv->lock);

	rkfree(priv);
	rkfree(ps);
}

struct ps_factory pffb_factory = {
        .owner		= THIS_MODULE,
        .create		= pffb_create,
        .destroy	= pffb_destroy,
};

static int __init pffb_init(void) {
        int ret = 0;

	strcpy(pffb_factory.name, PFFB_NAME);
        ret = pff_ps_publish(&pffb_factory);

        if (ret) {
                LOG_ERR("Failed to publish PFFB policy set factory :(");
                return -1;
        }

	if(kobject_init_and_add(&pffb_obj, &pffb_ktype, 0, "pffb")) {
		LOG_ERR("Sysfs failed!");
		return 0;
	}

        LOG_INFO("PFFB policy set loaded successfully!");

        return 0;
}

static void __exit pffb_exit(void) {
        int ret = 0;

        ret = pff_ps_unpublish(PFFB_NAME);

        if (ret) {
                LOG_ERR("Failed to unpublish PFFB policy set factory :(");
                return;
        }

        kobject_put(&pffb_obj);

        LOG_INFO("PFFB policy set unloaded successfully!");
}

module_init(pffb_init);
module_exit(pffb_exit);

MODULE_AUTHOR("Kewin Rausch <kewin.rausch@create-net.org>");
MODULE_LICENSE ("GPL");
MODULE_DESCRIPTION("Multipath routing with Flow Balancer.");
MODULE_VERSION("1");
