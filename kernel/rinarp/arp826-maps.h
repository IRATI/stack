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

#ifndef ARP826_MAPS_H
#define ARP826_MAPS_H

#include <linux/types.h>
#include <linux/netdevice.h>

struct table;

struct tmap;
struct tmap_entry;

struct tmap *       tmap_create(void);
struct tmap *       tmap_create_ni(void);
int                 tmap_destroy(struct tmap * map);

bool                tmap_is_empty(struct tmap * map);
int                 tmap_entry_add(struct tmap *       map,
                                   struct net_device * key_device,
                                   uint16_t            key_ptype,
                                   struct table *      value);
int                 tmap_entry_add_ni(struct tmap *       map,
                                      struct net_device * key_device,
                                      uint16_t            key_ptype,
                                      struct table *      value);
struct tmap_entry * tmap_entry_find(struct tmap *       map,
                                    struct net_device * key_device,
                                    uint16_t            key_ptype);
int                 tmap_entry_remove(struct tmap_entry * entry);

struct table *      tmap_entry_value(struct tmap_entry * entry);
int                 tmap_entry_destroy(struct tmap_entry * entry);

#endif
