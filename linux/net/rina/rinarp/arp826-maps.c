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

static bool tmap_is_ok(struct tmap * map)
{ return (map ? true : false); }

bool tmap_is_empty(struct tmap * map)
{
        ASSERT(tmap_is_ok(map));

        return hash_empty(map->table) ? true : false;
}

int tmap_destroy(struct tmap * map)
{
#if 0
        struct tmap_entry * entry;
        struct hlist_node * tmp;
        int                 bucket;
#endif

        if (!tmap_is_ok(map))
                return -1;

        if (!tmap_is_empty(map))
                return -1;

#if 0
        hash_for_each_safe(map->table, bucket, tmp, entry, hlist) {
                hash_del(&entry->hlist);
                rkfree(entry);
        }
#endif

        rkfree(map);

        return 0;
}

#define tmap_hash(T, K) hash_min(K, HASH_BITS(T))

static int tmap_entry_add_gfp(gfp_t               flags,
                              struct tmap *       map,
                              struct net_device * key_device,
                              uint16_t            key_ptype,
                              struct table *      value)
{
        struct tmap_entry * tmp;

        if (!tmap_is_ok(map))
                return -1;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return -1;

        tmp->key   = key_ptype;
        tmp->value = value;
        INIT_HLIST_NODE(&tmp->hlist);

        hash_add(map->table, &tmp->hlist, key_ptype);

        return 0;
}

int tmap_entry_add(struct tmap *       map,
                   struct net_device * key_device,
                   uint16_t            key_ptype,
                   struct table *      value)
{ return tmap_entry_add_gfp(GFP_KERNEL, map, key_device, key_ptype, value); }

int tmap_entry_add_ni(struct tmap *       map,
                      struct net_device * key_device,
                      uint16_t            key_ptype,
                      struct table *      value)
{ return tmap_entry_add_gfp(GFP_ATOMIC, map, key_device, key_ptype, value); }

struct tmap_entry * tmap_entry_find(struct tmap *       map,
                                    struct net_device * key_device,
                                    uint16_t            key_ptype)
{
        struct tmap_entry * entry;
        struct hlist_head * head;

        if (!tmap_is_ok(map))
                return NULL;

        head = &map->table[tmap_hash(map->table, key_ptype)];
        hlist_for_each_entry(entry, head, hlist) {
                if (entry->key == key_ptype)
                        return entry;
        }

        return NULL;
}

int tmap_entry_remove(struct tmap_entry * entry)
{
        if (!entry)
                return -1;

        hash_del(&entry->hlist);

        return 0;
}

struct table * tmap_entry_value(struct tmap_entry * entry)
{
        if (!entry)
                return NULL;

        return entry->value;
}

int tmap_entry_destroy(struct tmap_entry * entry)
{
        if (!entry)
                return -1;

        rkfree(entry);

        return 0;
}
