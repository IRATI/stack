/*
 * Port-2-IPCP instance mapping
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
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
#include <linux/hashtable.h>

#define RINA_PREFIX "pim"

#include "logs.h"
#include "debug.h"
#include "utils.h"
#include "pim.h"

#define PMAP_HASH_BITS 7

struct pmap {
        DECLARE_HASHTABLE(table, PMAP_HASH_BITS);
};

struct pmap_entry {
        port_id_t                   key;
        struct ipcp_instance_data * value;
        struct hlist_node           hlist;
};

struct pmap * pmap_create(void)
{
        struct pmap * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        hash_init(tmp->table);

        return tmp;
}

int pmap_destroy(struct pmap * map)
{
        struct pmap_entry * entry;
        struct hlist_node * tmp;
        int                 bucket;

        ASSERT(map);

        hash_for_each_safe(map->table, bucket, tmp, entry, hlist) {
                hash_del(&entry->hlist);
                rkfree(entry);
        }

        rkfree(map);

        return 0;
}

int pmap_empty(struct pmap * map)
{
        ASSERT(map);
        return hash_empty(map->table);
}

#define pmap_hash(T, K) hash_min(K, HASH_BITS(T))

static struct pmap_entry * pmap_entry_find(struct pmap * map,
                                           port_id_t     key)
{
        struct pmap_entry * entry;
        struct hlist_head *     head;

        ASSERT(map);

        head = &map->table[pmap_hash(map->table, key)];
        hlist_for_each_entry(entry, head, hlist) {
                if (entry->key == key)
                        return entry;
        }

        return NULL;
}

struct ipcp_instance_data * pmap_find(struct pmap * map,
                                      port_id_t     key)
{
        struct pmap_entry * entry;

        ASSERT(map);

        entry = pmap_entry_find(map, key);
        if (!entry)
                return NULL;

        return entry->value;
}

int pmap_update(struct pmap *               map,
                port_id_t                   key,
                struct ipcp_instance_data * value)
{
        struct pmap_entry * cur;

        ASSERT(map);

        cur = pmap_entry_find(map, key);
        if (!cur)
                return -1;

        cur->value = value;

        return 0;
}

static int pmap_add_gfp(gfp_t                       flags,
                        struct pmap *               map,
                        port_id_t                   key,
                        struct ipcp_instance_data * value)
{
        struct pmap_entry * tmp;

        ASSERT(map);

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return -1;

        tmp->key   = key;
        tmp->value = value;
        INIT_HLIST_NODE(&tmp->hlist);

        hash_add(map->table, &tmp->hlist, key);

        return 0;
}

int pmap_add(struct pmap *               map,
             port_id_t                   key,
             struct ipcp_instance_data * value)
{ return pmap_add_gfp(GFP_KERNEL, map, key, value); }

int pmap_add_ni(struct pmap *               map,
                port_id_t                   key,
                struct ipcp_instance_data * value)
{ return pmap_add_gfp(GFP_ATOMIC, map, key, value); }

int pmap_remove(struct pmap * map,
                port_id_t     key)
{
        struct pmap_entry * cur;

        ASSERT(map);

        cur = pmap_entry_find(map, key);
        if (!cur)
                return -1;

        hash_del(&cur->hlist);
        rkfree(cur);

        return 0;
}

struct pim {
        struct pmap * map;
};

struct pim * pim_create(void)
{
        struct pim * tmp;

        tmp = rkmalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->map = pmap_create();
        if (!tmp->map) {
                rkfree(tmp);
                return NULL;
        }

        return tmp;
}

int pim_destroy(struct pim * pim)
{
        if (!pim)
                return -1;

        ASSERT(pim->map);

        pmap_destroy(pim->map);
        rkfree(pim);

        return 0;
}

struct ipcp_instance_data * pim_ingress_get(struct pim * pim,
                                            port_id_t    id)
{
        if (!pim)
                return NULL;
        if (!is_port_id_ok(id))
                return NULL;

        return pmap_find(pim->map, id);
}

int pim_ingress_set(struct pim *                pim,
                    port_id_t                   id,
                    struct ipcp_instance_data * ipcp)
{
        struct ipcp_instance_data * tmp;

        if (!pim)
                return -1;
        if (!is_port_id_ok(id))
                return -1;

        tmp = pmap_find(pim->map, id);
        if (tmp)
                return pmap_update(pim->map, id, ipcp);

        return pmap_add_gfp(GFP_KERNEL, pim->map, id, ipcp);
}
