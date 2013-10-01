/*
 * K-IPCM related utilities
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
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
#include "rnl-utils.h"

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

int ipcp_imap_update(struct ipcp_imap *     map,
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

int ipcp_imap_add(struct ipcp_imap *     map,
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
                     ipc_process_id_t   key)
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
 * SEQNMAPs
 */

#define SEQNMAP_HASH_BITS 7
#define SEQNMAP_WRONG 0xFF

rnl_sn_t seq_num_bad(void)
{ return SEQNMAP_WRONG; }
EXPORT_SYMBOL(seq_num_bad);

int is_seq_num_ok(rnl_sn_t sn)
{ return (sn >= 0 && sn < SEQNMAP_WRONG) ? 1 : 0; }
EXPORT_SYMBOL(is_seq_num_ok);

struct seqn_fmap {
        DECLARE_HASHTABLE(table, SEQNMAP_HASH_BITS);
};

struct seqn_fmap_entry {
        flow_id_t         key;
        rnl_sn_t          value;
        struct hlist_node hlist;
};

struct seqn_fmap * seqn_fmap_create(void)
{
        struct seqn_fmap * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        hash_init(tmp->table);

        return tmp;
}

int seqn_fmap_destroy(struct seqn_fmap * map)
{
        struct seqn_fmap_entry * entry;
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

int seqn_fmap_empty(struct seqn_fmap * map)
{
        ASSERT(map);
        return hash_empty(map->table);
}

#define snmap_hash(T, K) hash_min(K, HASH_BITS(T))

static struct seqn_fmap_entry * snmap_entry_find(struct seqn_fmap * map,
                                                 flow_id_t          key)
{
        struct seqn_fmap_entry * entry;
        struct hlist_head *      head;

        ASSERT(map);

        head = &map->table[snmap_hash(map->table, key)];
        hlist_for_each_entry(entry, head, hlist) {
                if (entry->key == key)
                        return entry;
        }

        return NULL;
}


rnl_sn_t seqn_fmap_find(struct seqn_fmap * map,
                        flow_id_t          key)
{
        struct seqn_fmap_entry * entry;

        ASSERT(map);

        entry = snmap_entry_find(map, key);
        if (!entry)
                return seq_num_bad();

        return entry->value;
}

int seqn_fmap_update(struct seqn_fmap * map,
                     flow_id_t          key,
                     rnl_sn_t           value)
{
        struct seqn_fmap_entry * cur;

        ASSERT(map);

        cur = snmap_entry_find(map, key);
        if (!cur)
                return -1;

        cur->value = value;

        return 0;
}

int seqn_fmap_add(struct seqn_fmap * map,
                  flow_id_t          key,
                  rnl_sn_t           value)
{
        struct seqn_fmap_entry * tmp;

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

int seqn_fmap_remove(struct seqn_fmap * map,
                     flow_id_t          key)
{
        struct seqn_fmap_entry * cur;

        ASSERT(map);

        cur = snmap_entry_find(map, key);
        if (!cur)
                return -1;

        hash_del(&cur->hlist);
        rkfree(cur);

        return 0;
}
