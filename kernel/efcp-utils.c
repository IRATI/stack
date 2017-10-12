/*
 * EFCP related utilities
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

#define RINA_PREFIX "efcp-utils"

#include "logs.h"
#include "debug.h"
#include "utils.h"
#include "common.h"
#include "efcp.h"
#include "efcp-str.h"

struct efcp_imap * efcp_imap_create(void)
{
        struct efcp_imap * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        hash_init(tmp->table);

        return tmp;
}

int efcp_imap_destroy(struct efcp_imap * map,
                      int (* destructor)(struct efcp * instance))
{
        struct efcp_imap_entry * entry;
        struct hlist_node *      tmp;
        int                      bucket;

        ASSERT(map);

        hash_for_each_safe(map->table, bucket, tmp, entry, hlist) {
                hash_del(&entry->hlist);
                if (destructor)
                        destructor(entry->value);
                rkfree(entry);
        }

        rkfree(map);

        return 0;
}

int efcp_imap_empty(struct efcp_imap * map)
{
        ASSERT(map);
        return hash_empty(map->table);
}

#define imap_hash(T, K) hash_min(K, HASH_BITS(T))

static struct efcp_imap_entry * imap_entry_find(struct efcp_imap * map,
                                                cep_id_t           key)
{
        struct efcp_imap_entry * entry;
        struct hlist_head *      head;

        ASSERT(map);

        head = &map->table[imap_hash(map->table, key)];
        hlist_for_each_entry(entry, head, hlist) {
                if (entry->key == key)
                        return entry;
        }

        return NULL;
}

struct efcp * efcp_imap_find(struct efcp_imap * map,
                             cep_id_t           key)
{
        struct efcp_imap_entry * entry;

        ASSERT(map);

        entry = imap_entry_find(map, key);
        if (!entry)
                return NULL;

        return entry->value;
}
EXPORT_SYMBOL(efcp_imap_find);

int efcp_imap_address_change(struct efcp_imap *  map,
			     address_t address)
{
        struct efcp_imap_entry * entry;
        struct hlist_node *      tmp;
        int                      bucket;

        ASSERT(map);

        hash_for_each_safe(map->table, bucket, tmp, entry, hlist) {
        	entry->value->connection->source_address = address;
        }

        return 0;

}
EXPORT_SYMBOL(efcp_imap_address_change);

int efcp_imap_update(struct efcp_imap * map,
                     cep_id_t           key,
                     struct efcp *      value)
{
        struct efcp_imap_entry * cur;

        ASSERT(map);

        cur = imap_entry_find(map, key);
        if (!cur)
                return -1;

        cur->value = value;

        return 0;
}

int efcp_imap_add(struct efcp_imap * map,
                  cep_id_t           key,
                  struct efcp *      value)
{
        struct efcp_imap_entry * tmp;

        ASSERT(map);

        tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
        if (!tmp)
                return -1;

        tmp->key   = key;
        tmp->value = value;
        INIT_HLIST_NODE(&tmp->hlist);

        hash_add(map->table, &tmp->hlist, key);

        return 0;
}

int efcp_imap_remove(struct efcp_imap * map,
                     cep_id_t           key)
{
        struct efcp_imap_entry * cur;

        ASSERT(map);

        cur = imap_entry_find(map, key);
        if (!cur)
                return -1;

        hash_del(&cur->hlist);
        rkfree(cur);

        return 0;
}
