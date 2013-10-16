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

/*
 * IMAPs
 */

#define IMAP_HASH_BITS 7

struct efcp_imap {
        DECLARE_HASHTABLE(table, IMAP_HASH_BITS);
};

struct efcp_imap_entry {
        cep_id_t          key;
        struct efcp *     value;
        struct hlist_node hlist;
};

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

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
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

int is_cep_id_ok(cep_id_t id)
{ return 1; /* FIXME: Bummer, add it */ }
EXPORT_SYMBOL(is_cep_id_ok);

#define BITS_IN_BITMAP ((2 << BITS_PER_BYTE) * sizeof(cep_id_t))

#define CEP_ID_WRONG -1

struct cidm {
        DECLARE_BITMAP(bitmap, BITS_IN_BITMAP);
};

struct cidm * cidm_create(void)
{
        struct cidm * instance;

        instance = rkmalloc(sizeof(*instance), GFP_KERNEL);
        if (!instance)
                return NULL;

        bitmap_zero(instance->bitmap, BITS_IN_BITMAP);

        LOG_DBG("Instance initialized successfully (%zd bits)",
                BITS_IN_BITMAP);

        return instance;
}

int cidm_destroy(struct cidm * instance)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        rkfree(instance);

        return 0;
}

cep_id_t cidm_allocate(struct cidm * instance)
{
        cep_id_t id;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return CEP_ID_WRONG;
        }

        id = (cep_id_t) bitmap_find_next_zero_area(instance->bitmap,
                                                   BITS_IN_BITMAP,
                                                   0, 1, 0);
        LOG_DBG("The cidm bitmap find returned id %d (bad = %d)",
                id, CEP_ID_WRONG);

        LOG_DBG("Bits in bitmap %zd", BITS_IN_BITMAP);

        if (!is_cep_id_ok(id)) {
                LOG_WARN("Got an out-of-range cep-id (%d) from "
                         "the bitmap allocator, the bitmap is full ...", id);
                return CEP_ID_WRONG;
        }

        bitmap_set(instance->bitmap, id, 1);

        LOG_DBG("Bitmap allocation completed successfully (id = %d)", id);

        return id;
}

int cidm_release(struct cidm * instance,
                 cep_id_t      id)
{
        if (!is_cep_id_ok(id)) {
                LOG_ERR("Bad cep-id passed, bailing out");
                return -1;
        }
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        bitmap_clear(instance->bitmap, id, 1);

        LOG_DBG("Bitmap release completed successfully");

        return 0;
}
