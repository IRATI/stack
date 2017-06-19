/*
 * RINA Maps
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

#ifndef RINA_RMAP_H
#define RINA_RMAP_H

/* Map entries */
struct rmap_entry;

struct rmap_entry * rmap_entry_create(uint16_t key,
                                      void *   value);
struct rmap_entry * rmap_entry_create_ni(uint16_t key,
                                         void *   value);
int                 rmap_entry_destroy(struct rmap_entry * entry);

void *              rmap_entry_value(const struct rmap_entry * entry);
int                 rmap_entry_update(struct rmap_entry * entry,
                                      void *              value);

/* Maps */
struct rmap;

struct rmap *       rmap_create(void);
struct rmap *       rmap_create_ni(void);
int                 rmap_destroy(struct rmap * map,
                                 void       (* dtor)(struct rmap_entry * e));

/* FIXME: rmap_is_empty has to take a const parameter */
bool                rmap_is_empty(struct rmap * map);
int                 rmap_insert(struct rmap *       map,
                                uint16_t            key,
                                struct rmap_entry * entry);
struct rmap_entry * rmap_find(const struct rmap * map,
                              uint16_t            key);
int                 rmap_remove(struct rmap *       map,
                                struct rmap_entry * entry);
void                rmap_for_each(struct rmap * map,
                                  int        (* f)(struct rmap_entry * entry));
#endif
