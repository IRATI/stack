/*
 * RINA Data Structures
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

#ifndef RINA_RDS_H
#define RINA_RDS_H

struct rmap *       rmap_create(void);
struct rmap *       rmap_create_gfp(gfp_t flags);
int                 rmap_destroy(struct rmap * map);
bool                rmap_is_empty(struct rmap * map);
struct table *      rmap_find(struct rmap * map,
                              uint16_t      key);
int                 rmap_update(struct rmap *  map,
                                uint16_t       key,
                                struct table * value);

struct rmap_entry * rmap_entry_create_gfp(gfp_t          gfp,
                                          uint16_t       key,
                                          struct table * value);
struct rmap_entry * rmap_entry_create(uint16_t       key,
                                      struct table * value);
int                 rmap_entry_insert(struct rmap *       map,
                                      uint16_t            key,
                                      struct rmap_entry * entry);
struct rmap_entry * rmap_entry_find(struct rmap * map,
                                    uint16_t      key);
int                 rmap_entry_remove(struct rmap_entry * entry);
struct table *      rmap_entry_value(struct rmap_entry * entry);
int                 rmap_entry_destroy(struct rmap_entry * entry);

#endif
