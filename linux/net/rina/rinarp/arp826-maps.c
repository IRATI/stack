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

/*
 * FIXME: This is the ugliest and shittier implementation we could find, but
 *        ... we had no time to do any better. This code will be replaced
 *        completely with a better implementation once we'be able to find
 *        time to breathe again ...
 */

struct tmap {
        struct list_head head;
};

struct tmap_entry {
        struct {
                struct net_device * device;
                uint16_t            ptype;
        } key;
        struct table *   value;

        struct list_head next;
};

static struct tmap * tmap_create_gfp(gfp_t flags)
{
        struct tmap * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        INIT_LIST_HEAD(&tmp->head);

        return tmp;
}

struct tmap * tmap_create_ni(void)
{ return tmap_create_gfp(GFP_ATOMIC); }

struct tmap * tmap_create(void)
{ return tmap_create_gfp(GFP_KERNEL); }

static bool tmap_is_ok(const struct tmap * map)
{ return map ? true : false; }

bool tmap_is_empty(struct tmap * map)
{
        ASSERT(tmap_is_ok(map));

        return list_empty(&map->head);
}

int tmap_destroy(struct tmap * map)
{
        struct tmap_entry * pos, * nxt;

        if (!tmap_is_ok(map))
                return -1;

        ASSERT(tmap_is_empty(map)); /* To prevent loosing memory */

        list_for_each_entry_safe(pos, nxt, &map->head, next) {
                list_del(&pos->next);
                rkfree(pos);
        }

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

        tmp->key.device = key_device;
        tmp->key.ptype  = key_ptype;
        tmp->value      = value;
        INIT_LIST_HEAD(&tmp->next);

        list_add(&tmp->next, &map->head);

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
        struct tmap_entry * tmp;

        if (!tmap_is_ok(map))
                return NULL;

        list_for_each_entry(tmp, &map->head, next) {
                if ((tmp->key.device == key_device) &&
                    (tmp->key.ptype  == key_ptype))
                        return tmp;
        }

        return NULL;
}

static bool tmap_entry_is_ok(const struct tmap_entry * entry)
{ return entry ? true : false; }

int tmap_entry_remove(struct tmap_entry * entry)
{
        if (!tmap_entry_is_ok(entry))
                return -1;

        list_del(&entry->next);

        return 0;
}

struct table * tmap_entry_value(struct tmap_entry * entry)
{
        if (!tmap_entry_is_ok(entry))
                return NULL;

        return entry->value;
}

int tmap_entry_destroy(struct tmap_entry * entry)
{
        if (!tmap_entry_is_ok(entry))
                return -1;

        rkfree(entry);

        return 0;
}
