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
#include "fidm.h"

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
 * FMAPs
 */

#define FMAP_HASH_BITS 7
#define SNVALUE_WRONG  0xFFFFFF

rnl_sn_t seq_num_bad(void)
{ return SNVALUE_WRONG; }
EXPORT_SYMBOL(seq_num_bad);

/* FIXME: We need to change this */
int is_seq_num_ok(rnl_sn_t sn)
{ return (sn >= 0 && sn < SNVALUE_WRONG) ? 1 : 0; }
EXPORT_SYMBOL(is_seq_num_ok);

struct kipcm_fmap {
        DECLARE_HASHTABLE(table, FMAP_HASH_BITS);
};

struct kipcm_fmap_entry {
        flow_id_t         key;
        rnl_sn_t          value;
        struct hlist_node hlist;
};

struct kipcm_fmap * kipcm_fmap_create(void)
{
        struct kipcm_fmap * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        hash_init(tmp->table);

        return tmp;
}

int kipcm_fmap_destroy(struct kipcm_fmap * map)
{
        struct kipcm_fmap_entry * entry;
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

int kipcm_fmap_empty(struct kipcm_fmap * map)
{
        ASSERT(map);
        return hash_empty(map->table);
}

#define fmap_hash(T, K) hash_min(K, HASH_BITS(T))

static struct kipcm_fmap_entry * fmap_entry_find(struct kipcm_fmap * map,
                                                 flow_id_t           key)
{
        struct kipcm_fmap_entry * entry;
        struct hlist_head *      head;

        ASSERT(map);

        head = &map->table[fmap_hash(map->table, key)];
        hlist_for_each_entry(entry, head, hlist) {
                if (entry->key == key)
                        return entry;
        }

        return NULL;
}

rnl_sn_t kipcm_fmap_find(struct kipcm_fmap * map,
                         flow_id_t           key)
{
        struct kipcm_fmap_entry * entry;

        ASSERT(map);

        entry = fmap_entry_find(map, key);
        if (!entry)
                return SNVALUE_WRONG;

        return entry->value;
}

int kipcm_fmap_update(struct kipcm_fmap * map,
                      flow_id_t           key,
                      rnl_sn_t            value)
{
        struct kipcm_fmap_entry * cur;

        ASSERT(map);

        cur = fmap_entry_find(map, key);
        if (!cur)
                return -1;

        cur->value = value;

        return 0;
}

int kipcm_fmap_add(struct kipcm_fmap * map,
                   flow_id_t           key,
                   rnl_sn_t            value)
{
        struct kipcm_fmap_entry * tmp;

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

int kipcm_fmap_remove(struct kipcm_fmap * map,
                      flow_id_t           key)
{
        struct kipcm_fmap_entry * cur;

        ASSERT(map);

        cur = fmap_entry_find(map, key);
        if (!cur)
                return -1;

        hash_del(&cur->hlist);
        rkfree(cur);

        return 0;
}

/*
 * SMAP returning fids
 */

#define SMAP_SEQN_HASH_BITS 7

struct kipcm_smap {
        DECLARE_HASHTABLE(table, SMAP_SEQN_HASH_BITS);
};

struct kipcm_smap_entry {
        rnl_sn_t          key;
        flow_id_t         value;
        struct hlist_node hlist;
};

struct kipcm_smap * kipcm_smap_create(void)
{
        struct kipcm_smap * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        hash_init(tmp->table);

        return tmp;
}

int kipcm_smap_destroy(struct kipcm_smap * map)
{
        struct kipcm_smap_entry * entry;
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

int kipcm_smap_empty(struct kipcm_smap * map)
{
        ASSERT(map);
        return hash_empty(map->table);
}

#define smap_hash(T, K) hash_min(K, HASH_BITS(T))

static struct kipcm_smap_entry * smap_entry_find(struct kipcm_smap * map,
                                                 rnl_sn_t            key)
{
        struct kipcm_smap_entry * entry;
        struct hlist_head *       head;

        ASSERT(map);

        head = &map->table[smap_hash(map->table, key)];
        hlist_for_each_entry(entry, head, hlist) {
                if (entry->key == key)
                        return entry;
        }

        return NULL;
}

flow_id_t kipcm_smap_find(struct kipcm_smap * map,
                          rnl_sn_t            key)
{
        struct kipcm_smap_entry * entry;

        ASSERT(map);

        entry = smap_entry_find(map, key);
        if (!entry)
                return flow_id_bad();

        return entry->value;
}

int kipcm_smap_update(struct kipcm_smap * map,
                      rnl_sn_t            key,
                      flow_id_t           value)
{
        struct kipcm_smap_entry * cur;

        ASSERT(map);

        cur = smap_entry_find(map, key);
        if (!cur)
                return -1;

        cur->value = value;

        return 0;
}

int kipcm_smap_add(struct kipcm_smap * map,
                   rnl_sn_t             key,
                   flow_id_t            value)
{
        struct kipcm_smap_entry * tmp;

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

int kipcm_smap_remove(struct kipcm_smap * map,
                      rnl_sn_t            key)
{
        struct kipcm_smap_entry * cur;

        ASSERT(map);

        cur = smap_entry_find(map, key);
        if (!cur)
                return -1;

        hash_del(&cur->hlist);
        rkfree(cur);

        return 0;
}
