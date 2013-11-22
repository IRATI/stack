/*
 * RINA Maps
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

#define RINA_PREFIX "rmap"

#include "logs.h"
#include "debug.h"
#include "rmem.h"
#include "rmap.h"

#define RMAP_HASH_BITS 7

struct rmap {
        DECLARE_HASHTABLE(table, RMAP_HASH_BITS);
};

struct rmap_entry {
        uint16_t          key;
        void *            value;
        struct hlist_node hlist;
};

#define rmap_hash(T, K) hash_min(K, HASH_BITS(T))

static struct rmap * rmap_create_gfp(gfp_t flags)
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
EXPORT_SYMBOL(rmap_create);

struct rmap * rmap_create_ni(void)
{ return rmap_create_gfp(GFP_ATOMIC); }
EXPORT_SYMBOL(rmap_create_ni);

int rmap_destroy(struct rmap * map,
                 void       (* dtor)(struct rmap_entry * e))
{
        struct rmap_entry * entry;
        struct hlist_node * tmp;
        int                 bucket;

        if (!map) {
                LOG_ERR("Cannot destroy a NULL map");
                return -1;
        }

        if (!dtor) {
                LOG_ERR("No destructor passed, cannod destroy map %pK", map);
                return -1;
        }

        hash_for_each_safe(map->table, bucket, tmp, entry, hlist) {
                LOG_DBG("Calling dtor for entry %pK", entry);
                dtor(entry);
        }
        rkfree(map);

        LOG_DBG("Map %pK destroyed successfully", map);

        return 0;
}
EXPORT_SYMBOL(rmap_destroy);

bool rmap_is_empty(struct rmap * map)
{
        if (!map) {
                LOG_ERR("Map %pK is NULL, returning false", map);
                return false;
        }

        return hash_empty(map->table) ? true : false;
}
EXPORT_SYMBOL(rmap_is_empty);

int rmap_insert(struct rmap *       map,
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
EXPORT_SYMBOL(rmap_insert);

struct rmap_entry * rmap_find(const struct rmap * map,
                              uint16_t            key)
{
        struct rmap_entry *       entry;
        const struct hlist_head * head;

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
EXPORT_SYMBOL(rmap_find);

int rmap_remove(struct rmap *       map,
                struct rmap_entry * entry)
{
        if (!map) {
                LOG_ERR("Cannot remove an entry from a NULL map");
                return -1;
        }

        if (!entry) {
                return -1;
        }

        hash_del(&entry->hlist);

        return 0;
}
EXPORT_SYMBOL(rmap_remove);

void rmap_for_each(struct rmap * map,
                   int        (* f)(struct rmap_entry * entry))
{
        struct rmap_entry * entry;
        struct hlist_node * tmp;
        int                 bucket;

        if (!map) {
                LOG_ERR("Input map is NULL, cannot iterate");
                return;
        }
        if (!f) {
                LOG_ERR("Iterator function is NULL");
                return;
        }

        hash_for_each_safe(map->table, bucket, tmp, entry, hlist)
                f(entry);
}
EXPORT_SYMBOL(rmap_for_each);

int rmap_entry_update(struct rmap_entry * entry,
                      void *              value)
{
        if (!entry) {
                LOG_ERR("Cannot update a NULL entry");
                return -1;
        }

        entry->value = value;

        return 0;
}
EXPORT_SYMBOL(rmap_entry_update);

static struct rmap_entry * rmap_entry_create_gfp(gfp_t    flags,
                                                 uint16_t key,
                                                 void *   value)
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

struct rmap_entry * rmap_entry_create(uint16_t key,
                                      void *   value)
{ return rmap_entry_create_gfp(GFP_KERNEL, key, value); }
EXPORT_SYMBOL(rmap_entry_create);

struct rmap_entry * rmap_entry_create_ni(uint16_t key,
                                         void *   value)
{ return rmap_entry_create_gfp(GFP_ATOMIC, key, value); }
EXPORT_SYMBOL(rmap_entry_create_ni);

void * rmap_entry_value(const struct rmap_entry * entry)
{
        if (!entry) {
                LOG_ERR("Entry is NULL, cannot return its value");
                return NULL;
        }

        return entry->value;
}
EXPORT_SYMBOL(rmap_entry_value);

int rmap_entry_destroy(struct rmap_entry * entry)
{
        if (!entry) {
                LOG_ERR("The entry is NULL, cannot destroy the entry");
                return -1;
        }

        rkfree(entry);

        return 0;
}
EXPORT_SYMBOL(rmap_entry_destroy);
