/*
 * ARP 826 (wonnabe) related utilities
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

#include <linux/types.h>
#include <linux/hashtable.h>

/* FIXME: The following dependencies have to be removed */
#define RINA_PREFIX "arp826-maps"

#include "logs.h"
#include "debug.h"
#include "utils.h"
/* FIXME: End of dependencies ... */

#include "arp826-maps.h"

#define TMAP_HASH_BITS 7

struct tmap {
        DECLARE_HASHTABLE(table, TMAP_HASH_BITS);
};

struct tmap_entry {
        uint16_t          key;
        struct table *    value;
        struct hlist_node hlist;
};

struct tmap * tmap_create(void)
{
        struct tmap * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        hash_init(tmp->table);

        return tmp;
}

int tmap_destroy(struct tmap * map)
{
#if 0
        struct tmap_entry * entry;
        struct hlist_node * tmp;
        int                 bucket;
#endif

        ASSERT(map);
        ASSERT(hash_empty(map->table));

#if 0
        hash_for_each_safe(map->table, bucket, tmp, entry, hlist) {
                hash_del(&entry->hlist);
                rkfree(entry);
        }
#endif

        rkfree(map);

        return 0;
}

int tmap_empty(struct tmap * map)
{
        ASSERT(map);

        return hash_empty(map->table);
}

#define tmap_hash(T, K) hash_min(K, HASH_BITS(T))

#if 0
struct table * tmap_find(struct tmap * map,
                         uint16_t      key)
{
        struct tmap_entry * entry;

        ASSERT(map);

        entry = tmap_entry_find(map, key);
        if (!entry)
                return NULL;

        return entry->value;
}

int tmap_update(struct tmap *   map,
                uint16_t        key,
                struct table *  value)
{
        struct tmap_entry * cur;

        ASSERT(map);

        cur = tmap_entry_find(map, key);
        if (!cur)
                return -1;

        cur->value = value;

        return 0;
}
#endif

struct tmap_entry * tmap_entry_create(struct net_device * key_device,
                                      uint16_t            key_ptype,
                                      struct table *      value)
{
        struct tmap_entry * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->key   = key_ptype;
        tmp->value = value;
        INIT_HLIST_NODE(&tmp->hlist);

        return tmp;
}

int tmap_entry_insert(struct tmap *       map,
                      struct net_device * key_device,
                      uint16_t            key_ptype,
                      struct tmap_entry * entry)
{
        ASSERT(map);
        ASSERT(entry);

        hash_add(map->table, &entry->hlist, key_ptype);

        return 0;
}

struct tmap_entry * tmap_entry_find(struct tmap *       map,
                                    struct net_device * key_device,
                                    uint16_t            key_ptype)
{
        struct tmap_entry * entry;
        struct hlist_head * head;

        ASSERT(map);

        head = &map->table[tmap_hash(map->table, key_ptype)];
        hlist_for_each_entry(entry, head, hlist) {
                if (entry->key == key_ptype)
                        return entry;
        }

        return NULL;
}

int tmap_entry_remove(struct tmap_entry * entry)
{
        ASSERT(entry);

        hash_del(&entry->hlist);

        return 0;
}

struct table * tmap_entry_value(struct tmap_entry * entry)
{
        ASSERT(entry);

        return entry->value;
}

int tmap_entry_destroy(struct tmap_entry * entry)
{
        ASSERT(entry);

        rkfree(entry);

        return 0;
}
