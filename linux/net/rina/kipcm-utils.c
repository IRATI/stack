/*
 * K-IPCM related utilities
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

#define RINA_PREFIX "kipcm-utils"

#include "logs.h"
#include "debug.h"
#include "utils.h"
#include "common.h"
#include "kipcm-utils.h"

/*
 * IMAPs
 */

#define IMAP_HASH_BITS 7

struct ipcp_imap {
        DECLARE_HASHTABLE(table, IMAP_HASH_BITS);
};

struct ipcp_imap_entry {
        ipc_process_id_t       key;
        struct ipcp_instance * value;
        struct hlist_node      hlist;
};

struct ipcp_imap * ipcp_imap_create(void)
{
        struct ipcp_imap * tmp;
        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        hash_init(tmp->table);
        return tmp;
}

int ipcp_imap_destroy(struct ipcp_imap * map)
{
        struct ipcp_imap_entry * entry;
        struct hlist_node *      tmp;
        int                      bucket;

        ASSERT(map);

        hash_for_each_safe(map->table, bucket, tmp, entry, hlist) {
                hash_del(&entry->hlist);
                rkfree(entry);
        }

        rkfree(map);

        return 0;
}

int ipcp_imap_empty(struct ipcp_imap * map)
{
        ASSERT(map);
        return hash_empty(map->table);
}

#define imap_hash(T, K) hash_min(K, HASH_BITS(T))

static struct ipcp_imap_entry * imap_entry_find(struct ipcp_imap * map,
                                                ipc_process_id_t   key)
{
        struct ipcp_imap_entry * entry;
        struct hlist_head *      head;

        ASSERT(map);

        head = &map->table[imap_hash(map->table, key)];
        hlist_for_each_entry(entry, head, hlist) {
                if (entry->key == key)
                        return entry;
        }

        return NULL;
}

struct ipcp_instance * ipcp_imap_find(struct ipcp_imap * map,
                                      ipc_process_id_t   key)
{
        struct ipcp_imap_entry * entry;

        ASSERT(map);

        entry = imap_entry_find(map, key);
        if (!entry)
                return NULL;

        return entry->value;
}

int ipcp_imap_update(struct ipcp_imap *    map,
                     ipc_process_id_t       key,
                     struct ipcp_instance * value)
{
        struct ipcp_imap_entry * cur;

        ASSERT(map);

        cur = imap_entry_find(map, key);
        if (!cur)
                return -1;

        cur->value = value;

        return 0;
}

int ipcp_imap_add(struct ipcp_imap *    map,
                  ipc_process_id_t       key,
                  struct ipcp_instance * value)
{
        struct ipcp_imap_entry * tmp;

        ASSERT(map);

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return -1;

        tmp->key   = key;
        tmp->value = value;
        INIT_HLIST_NODE(&tmp->hlist);

        hash_add(map->table, &tmp->hlist, key);

        return 0;
}

int ipcp_imap_remove(struct ipcp_imap * map,
                     ipc_process_id_t    key)
{
        struct ipcp_imap_entry * cur;

        ASSERT(map);

        cur = imap_entry_find(map, key);
        if (!cur)
                return -1;

        hash_del(&cur->hlist);
        rkfree(cur);

        return 0;
}

/*
 * FMAPs
 */

#define FMAP_HASH_BITS 7

struct ipcp_fmap {
        DECLARE_HASHTABLE(table, FMAP_HASH_BITS);
};

struct ipcp_fmap_entry {
        flow_id_t          key;
        struct ipcp_flow * value;
        struct hlist_node  hlist;
};

struct ipcp_fmap * ipcp_fmap_create(void)
{
        struct ipcp_fmap * tmp;
        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        hash_init(tmp->table);
        return tmp;
}

int ipcp_fmap_destroy(struct ipcp_fmap * map)
{
        struct ipcp_fmap_entry * entry;
        struct hlist_node *      tmp;
        int                      bucket;

        ASSERT(map);

        hash_for_each_safe(map->table, bucket, tmp, entry, hlist) {
                hash_del(&entry->hlist);
                rkfree(entry);
        }

        rkfree(map);

        return 0;
}

int ipcp_fmap_empty(struct ipcp_fmap * map)
{
        ASSERT(map);
        return hash_empty(map->table);
}

#define fmap_hash(T, K) hash_min(K, HASH_BITS(T))

static struct ipcp_fmap_entry * fmap_entry_find(struct ipcp_fmap * map,
                                                flow_id_t          key)
{
        struct ipcp_fmap_entry * entry;
        struct hlist_head *      head;

        ASSERT(map);

        head = &map->table[fmap_hash(map->table, key)];
        hlist_for_each_entry(entry, head, hlist) {
                if (entry->key == key)
                        return entry;
        }

        return NULL;
}

struct ipcp_flow * ipcp_fmap_find(struct ipcp_fmap * map,
                                  flow_id_t          key)
{
        struct ipcp_fmap_entry * entry;

        ASSERT(map);

        entry = fmap_entry_find(map, key);
        if (!entry)
                return NULL;

        return entry->value;
}

int ipcp_fmap_update(struct ipcp_fmap * map,
                     flow_id_t          key,
                     struct ipcp_flow * value)
{
        struct ipcp_fmap_entry * cur;

        ASSERT(map);

        cur = fmap_entry_find(map, key);
        if (!cur)
                return -1;

        cur->value = value;

        return 0;
}

int ipcp_fmap_add(struct ipcp_fmap * map,
                  flow_id_t          key,
                  struct ipcp_flow * value)
{
        struct ipcp_fmap_entry * tmp;

        ASSERT(map);

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return -1;

        tmp->key   = key;
        tmp->value = value;
        INIT_HLIST_NODE(&tmp->hlist);

        hash_add(map->table, &tmp->hlist, key);

        return 0;
}

int ipcp_fmap_remove(struct ipcp_fmap * map,
                     flow_id_t          key)
{
        struct ipcp_fmap_entry * cur;

        ASSERT(map);

        cur = fmap_entry_find(map, key);
        if (!cur)
                return -1;

        hash_del(&cur->hlist);
        rkfree(cur);

        return 0;
}

/*
 * PMAPs
 */

#define PMAP_HASH_BITS 7

struct ipcp_pmap {
        DECLARE_HASHTABLE(table, PMAP_HASH_BITS);
};

struct ipcp_pmap_entry {
        port_id_t          key;
        struct ipcp_flow * value;
        struct hlist_node  hlist;
        ipc_process_id_t   id;
};

struct ipcp_pmap * ipcp_pmap_create(void)
{
        struct ipcp_pmap * tmp;
        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        hash_init(tmp->table);
        return tmp;
}

int ipcp_pmap_destroy(struct ipcp_pmap * map)
{
        struct ipcp_pmap_entry * entry;
        struct hlist_node *      tmp;
        int                      bucket;

        ASSERT(map);

        hash_for_each_safe(map->table, bucket, tmp, entry, hlist) {
                hash_del(&entry->hlist);
                rkfree(entry);
        }

        rkfree(map);

        return 0;
}

int ipcp_pmap_empty(struct ipcp_pmap * map)
{
        ASSERT(map);
        return hash_empty(map->table);
}

#define pmap_hash(T, K) hash_min(K, HASH_BITS(T))

static struct ipcp_pmap_entry * pmap_entry_find(struct ipcp_pmap * map,
                                                port_id_t          key)
{
        struct ipcp_pmap_entry * entry;
        struct hlist_head *      head;

        ASSERT(map);

        head = &map->table[pmap_hash(map->table, key)];
        hlist_for_each_entry(entry, head, hlist) {
                if (entry->key == key)
                        return entry;
        }

        return NULL;
}

struct ipcp_flow * ipcp_pmap_find(struct ipcp_pmap * map,
                                  port_id_t          key)
{
        struct ipcp_pmap_entry * entry;

        ASSERT(map);

        entry = pmap_entry_find(map, key);
        if (!entry)
                return NULL;

        return entry->value;
}

int ipcp_pmap_update(struct ipcp_pmap *    map,
                     port_id_t             key,
                     struct ipcp_flow *    value)
{
        struct ipcp_pmap_entry * cur;

        ASSERT(map);

        cur = pmap_entry_find(map, key);
        if (!cur)
                return -1;

        cur->value = value;

        return 0;
}

int ipcp_pmap_add(struct ipcp_pmap * map,
                  port_id_t          key,
                  struct ipcp_flow * value,
                  ipc_process_id_t   id)
{
        struct ipcp_pmap_entry * tmp;

        ASSERT(map);

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return -1;

        tmp->key   = key;
        tmp->value = value;
        tmp->id    = id;
        INIT_HLIST_NODE(&tmp->hlist);

        hash_add(map->table, &tmp->hlist, key);

        return 0;
}

int ipcp_pmap_remove(struct ipcp_pmap * map,
                     port_id_t          key)
{
        struct ipcp_pmap_entry * cur;

        ASSERT(map);

        cur = pmap_entry_find(map, key);
        if (!cur)
                return -1;

        hash_del(&cur->hlist);
        rkfree(cur);

        return 0;
}

int ipcp_pmap_remove_all_for_id(struct ipcp_pmap * map,
                                ipc_process_id_t   id)
{
        struct ipcp_pmap_entry * entry;
        struct hlist_node *      tmp;
        int                      bucket;

        ASSERT(map);

        hash_for_each_safe(map->table, bucket, tmp, entry, hlist) {
                if (entry->id == id) {
                        hash_del(&entry->hlist);
                        rkfree(entry);
                }
        }

        return 0;
}
