/*
 * KFA related utilities
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#include <linux/hashtable.h>
#include <linux/list.h>

#define RINA_PREFIX "kfa-utils"

#include "logs.h"
#include "debug.h"
#include "utils.h"
#include "common.h"
#include "kfa-utils.h"

/*
 * PMAPs
 */

#define PMAP_HASH_BITS 7

struct kfa_pmap {
        DECLARE_HASHTABLE(table, PMAP_HASH_BITS);
};

struct kfa_pmap_entry {
        port_id_t          key;
        struct ipcp_flow * value_flow;

        struct hlist_node  hlist;
};

struct kfa_pmap * kfa_pmap_create(void)
{
        struct kfa_pmap * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        hash_init(tmp->table);

        return tmp;
}

int kfa_pmap_destroy(struct kfa_pmap * map)
{
        struct kfa_pmap_entry * entry;
        struct hlist_node *     tmp;
        int                     bucket;

        ASSERT(map);

        hash_for_each_safe(map->table, bucket, tmp, entry, hlist) {
                hash_del(&entry->hlist);
                rkfree(entry);
        }

        rkfree(map);

        return 0;
}

int kfa_pmap_empty(struct kfa_pmap * map)
{
        ASSERT(map);
        return hash_empty(map->table);
}

#define pmap_hash(T, K) hash_min(K, HASH_BITS(T))

static struct kfa_pmap_entry * pmap_entry_find(struct kfa_pmap * map,
                                               port_id_t         key)
{
        struct kfa_pmap_entry * entry;
        struct hlist_head *     head;

        ASSERT(map);

        head = &map->table[pmap_hash(map->table, key)];
        hlist_for_each_entry(entry, head, hlist) {
                if (entry->key == key)
                        return entry;
        }

        return NULL;
}

struct ipcp_flow * kfa_pmap_find(struct kfa_pmap * map,
                                 port_id_t         key)
{
        struct kfa_pmap_entry * entry;

        ASSERT(map);

        entry = pmap_entry_find(map, key);
        if (!entry)
                return NULL;

        return entry->value_flow;
}

int kfa_pmap_update(struct kfa_pmap *   map,
                    port_id_t           key,
                    struct ipcp_flow *  value)
{
        struct kfa_pmap_entry * cur;

        ASSERT(map);

        cur = pmap_entry_find(map, key);
        if (!cur)
                return -1;

        cur->value_flow = value;

        return 0;
}

static int kfa_pmap_add_gfp(gfp_t              flags,
                            struct kfa_pmap *  map,
                            port_id_t          key,
                            struct ipcp_flow * value_flow)
{
        struct kfa_pmap_entry * tmp;

        ASSERT(map);

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return -1;

        tmp->key        = key;
        tmp->value_flow = value_flow;
        INIT_HLIST_NODE(&tmp->hlist);

        hash_add(map->table, &tmp->hlist, key);

        return 0;
}

int kfa_pmap_add(struct kfa_pmap *  map,
                 port_id_t          key,
                 struct ipcp_flow * value_flow)
{ return kfa_pmap_add_gfp(GFP_KERNEL, map, key, value_flow); }

int kfa_pmap_add_ni(struct kfa_pmap *  map,
                    port_id_t          key,
                    struct ipcp_flow * value_flow)
{ return kfa_pmap_add_gfp(GFP_ATOMIC, map, key, value_flow); }

int kfa_pmap_remove(struct kfa_pmap * map,
                    port_id_t         key)
{
        struct kfa_pmap_entry * cur;

        ASSERT(map);

        cur = pmap_entry_find(map, key);
        if (!cur)
                return -1;

        hash_del(&cur->hlist);
        rkfree(cur);

        return 0;
}
