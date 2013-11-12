/*
 * RINA Data Structures
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
#include <linux/list.h>
#include <linux/kernel.h>

#define RINA_PREFIX "rds"

#include "logs.h"
#include "debug.h"
#include "utils.h"
#include "rds.h"

/*
 * Maps
 */
#define RMAP_HASH_BITS 7

struct rmap {
        DECLARE_HASHTABLE(table, RMAP_HASH_BITS);
};

struct rmap_entry {
        uint16_t          key;
        struct table *    value;
        struct hlist_node hlist;
};

struct rmap * rmap_create_gfp(gfp_t flags)
{
        struct rmap * tmp;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        hash_init(tmp->table);

        LOG_DBG("Map %pK created successfully", tmp);

        return tmp;
}

struct rmap * rmap_create(void)
{ return rmap_create_gfp(GFP_KERNEL); }

int rmap_destroy(struct rmap * map)
{
        if (!map) {
                LOG_ERR("Bogus input parameter, cannot destroy a NULL map");
                return -1;
        }
        
        if (!hash_empty(map->table)) {
                LOG_ERR("Map %pK is not empty and I won't destroy it. "
                        "You would be lossing memory ...", map);
                return -1;
        }

        rkfree(map);

        LOG_DBG("Map %pK destroyed successfully", map);

        return 0;
}

bool rmap_is_empty(struct rmap * map)
{
        if (!map) {
                LOG_ERR("Map %pK is NULL, returning false (check your code!)",
                        map);
                return false;
        }

        return hash_empty(map->table) ? true : false;
}

#define rmap_hash(T, K) hash_min(K, HASH_BITS(T))

struct table * rmap_find(struct rmap * map,
                         uint16_t      key)
{
        struct rmap_entry * entry;

        if (!map) {
                LOG_ERR("Bogus input parameter, "
                        "I cannot do a lookup on a NULL map ...");
                return NULL;
        }

        entry = rmap_entry_find(map, key);
        if (!entry)
                return NULL;

        return entry->value;
}

int rmap_update(struct rmap *   map,
                uint16_t        key,
                struct table *  value)
{
        struct rmap_entry * cur;

        if (!map) {
                LOG_ERR("Bogus input parameter, cannot update a NULL map");
                return -1;
        }

        cur = rmap_entry_find(map, key);
        if (!cur) {
                LOG_ERR("Cannot find entry. I won't be updating it ...");
                return -1;
        }

        cur->value = value;

        return 0;
}

struct rmap_entry * rmap_entry_create_gfp(gfp_t          flags,
                                          uint16_t       key,
                                          struct table * value)
{
        struct rmap_entry * tmp;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->key   = key;
        tmp->value = value;
        INIT_HLIST_NODE(&tmp->hlist);

        LOG_ERR("Entry %pK created successfully", tmp);

        return tmp;
}

struct rmap_entry * rmap_entry_create(uint16_t       key,
                                      struct table * value)
{ return rmap_entry_create_gfp(GFP_KERNEL, key, value); }

int rmap_entry_insert(struct rmap *       map,
                      uint16_t            key,
                      struct rmap_entry * entry)
{
        if (!map) {
                LOG_ERR("Cannot insert an entry into a NULL map");
                return -1;
        }

        if (!entry) {
                LOG_ERR("Cannot insert a NULL entry in map %pK", map);
                return -1;
        }

        hash_add(map->table, &entry->hlist, key);

        return 0;
}

struct rmap_entry * rmap_entry_find(struct rmap * map,
                                    uint16_t      key)
{
        struct rmap_entry * entry;
        struct hlist_head * head;

        if (!map) {
                LOG_ERR("Cannot look-up in a empty map");
                return NULL;
        }

        head = &map->table[rmap_hash(map->table, key)];
        hlist_for_each_entry(entry, head, hlist) {
                if (entry->key == key)
                        return entry;
        }

        return NULL;
}

int rmap_entry_remove(struct rmap_entry * entry)
{
        if (!entry)
                return -1;

        hash_del(&entry->hlist);

        return 0;
}

struct table * rmap_entry_value(struct rmap_entry * entry)
{
        if (!entry) {
                LOG_ERR("Entry is NULL, cannot return its value");
                return NULL;
        }

        return entry->value;
}

int rmap_entry_destroy(struct rmap_entry * entry)
{
        if (!entry) {
                LOG_ERR("The entry is NULL, cannot destroy the entry");
                return -1;
        }

        rkfree(entry);

        return 0;
}
