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
#include "kipcm-utils.h"

/* FIXME: Replace with a map as soon as possible */
struct ipcp_map {
        struct list_head root;
};

struct ipcp_map_entry {
        ipc_process_id_t       id;   /* Key */
        struct ipcp_instance * data; /* Value */
        struct list_head       list;
};

struct ipcp_map * ipcp_map_create(void)
{
        struct ipcp_map * tmp;
        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;
        
        INIT_LIST_HEAD(&tmp->root);
        return tmp;
}

int ipcp_map_empty(struct ipcp_map * map)
{
        ASSERT(map);
        return list_empty(&map->root);
}

int ipcp_map_destroy(struct ipcp_map * map)
{
        ASSERT(map);

        /* FIXME: Destroy all the entries */
        LOG_MISSING;

        rkfree(map);

        return 0;
}

int ipcp_map_add(struct ipcp_map *      map,
                 ipc_process_id_t       id,
                 struct ipcp_instance * data)
{
        struct ipcp_map_entry * tmp;

        ASSERT(map);

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return -1;

        tmp->id   = id;
        tmp->data = data;
        INIT_LIST_HEAD(&tmp->list);

        list_add(&tmp->list, &map->root);

        return 0;
}

static struct ipcp_map_entry * map_entry_find(struct ipcp_map * map,
                                              ipc_process_id_t  id)
{
        struct ipcp_map_entry * cur;

        ASSERT(map);

        list_for_each_entry(cur, &map->root, list) {
                if (cur->id == id) {
                        return cur;
                }
        }

        return NULL;
}

struct ipcp_instance * ipcp_map_find(struct ipcp_map * map,
				     ipc_process_id_t  id)
{
        struct ipcp_map_entry * cur;

        ASSERT(map);

        cur = map_entry_find(map, id);
        if (!cur)
                return NULL;
        return cur->data;
}

int ipcp_map_update(struct ipcp_map * 	   map,
                    ipc_process_id_t  	   id,
                    struct ipcp_instance * data)
{
        struct ipcp_map_entry * cur;

        ASSERT(map);
        
        cur = map_entry_find(map, id);
        if (!cur)
                return -1;

        cur->data = data;

        return 0;
}

int ipcp_map_remove(struct ipcp_map * map,
                    ipc_process_id_t  id)
{
        struct ipcp_map_entry * cur;

        ASSERT(map);

        cur = map_entry_find(map, id);
        if (!cur)
                return -1;

        list_del(&cur->list);
        rkfree(cur);
        return 0;
}
