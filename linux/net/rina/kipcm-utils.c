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

#include <linux/list.h>

#define RINA_PREFIX "kipcm-utils"

#include "logs.h"
#include "debug.h"
#include "utils.h"
#include "common.h"
#include "kipcm-utils.h"

/* FIXME: Replace with a map as soon as possible */
struct ipcp_imap {
        struct list_head root;
};

/* FIXME: Replace with a map as soon as possible */
struct ipcp_fmap {
        struct list_head root;
};

struct ipcp_imap_entry {
        ipc_process_id_t       key;
        struct ipcp_instance * value;
        struct list_head       list;
};

struct ipcp_fmap_entry {
	port_id_t        key;
        struct flow *    value;
        struct list_head list;
};

struct ipcp_imap * ipcp_imap_create(void)
{
        struct ipcp_imap * tmp;
        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        INIT_LIST_HEAD(&tmp->root);
        return tmp;
}

struct ipcp_fmap * ipcp_fmap_create(void)
{
        struct ipcp_fmap * tmp;
        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;
        
        INIT_LIST_HEAD(&tmp->root);
        return tmp;
}

int ipcp_imap_empty(struct ipcp_imap * map)
{
        ASSERT(map);
        return list_empty(&map->root);
}

int ipcp_fmap_empty(struct ipcp_fmap * map)
{
        ASSERT(map);
        return list_empty(&map->root);
}

int ipcp_imap_destroy(struct ipcp_imap * map)
{
        ASSERT(map);

        /* FIXME: Destroy all the entries */
        LOG_MISSING;

        rkfree(map);

        return 0;
}

int ipcp_fmap_destroy(struct ipcp_fmap * map)
{
        ASSERT(map);

        /* FIXME: Destroy all the entries */
        LOG_MISSING;

        rkfree(map);

        return 0;
}

int ipcp_imap_add(struct ipcp_imap *        map,
                  ipc_process_id_t          key,
                  struct ipcp_instance *    value)
{
        struct ipcp_imap_entry * tmp;

        ASSERT(map);

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return -1;

        tmp->key   = key;
        tmp->value = value;
        INIT_LIST_HEAD(&tmp->list);

        list_add(&tmp->list, &map->root);

        return 0;
}

int ipcp_fmap_add(struct ipcp_fmap * map,
                  port_id_t          key,
                  struct flow *      value)
{
        struct ipcp_fmap_entry * tmp;

        ASSERT(map);

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return -1;

        tmp->key   = key;
        tmp->value = value;
        INIT_LIST_HEAD(&tmp->list);

        list_add(&tmp->list, &map->root);

        return 0;
}

static struct ipcp_imap_entry * imap_entry_find(struct ipcp_imap * map,
                                                ipc_process_id_t   key)
{
        struct ipcp_map_entry * cur;

        ASSERT(map);

        list_for_each_entry(cur, &map->root, list) {
                if (cur->key == key) {
                        return cur;
                }
        }

        return NULL;
}

static struct ipcp_fmap_entry * fmap_entry_find(struct ipcp_fmap * map,
                                                ipc_process_id_t   key)
{
        struct ipcp_map_entry * cur;

        ASSERT(map);

        list_for_each_entry(cur, &map->root, list) {
                if (cur->key == key) {
                        return cur;
                }
        }

        return NULL;
}

struct ipcp_instance * ipcp_imap_find(struct ipcp_imap * map,
                                      ipc_process_id_t   key)
{
        struct ipcp_imap_entry * cur;

        ASSERT(map);

        cur = imap_entry_find(map, key);
        if (!cur)
                return NULL;
        return cur->value;
}

struct flow * ipcp_fmap_find(struct ipcp_fmap * map,
                             port_id_t          key)
{
        struct ipcp_fmap_entry * cur;

        ASSERT(map);

        cur = fmap_entry_find(map, key);
        if (!cur)
                return NULL;
        return cur->value;
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

int ipcp_fmap_update(struct ipcp_fmap * map,
                     port_id_t          key,
                     struct flow *      value)
{
        struct ipcp_fmap_entry * cur;

        ASSERT(map);
        
        cur = fmap_entry_find(map, key);
        if (!cur)
                return -1;

        cur->value = value;

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

        list_del(&cur->list);
        rkfree(cur);
        return 0;
}

int ipcp_fmap_remove(struct ipcp_fmap * map,
                     port_id_t          key)
{
        struct ipcp_fmap_entry * cur;

        ASSERT(map);

        cur = fmap_entry_find(map, key);
        if (!cur)
                return -1;

        list_del(&cur->list);
        rkfree(cur);
        return 0;
}
