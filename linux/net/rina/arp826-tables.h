/*
 * ARP 826 (wonnabe) cache
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

#ifndef ARP_826_TABLES_H
#define ARP_826_TABLES_H

#include "arp826-utils.h"

struct table_entry;

/*
 * NOTE:
 *   Non const parameters indicate ownership takeover in parameters
 */

struct table_entry *       tble_create(struct gpa * gpa,
                                       struct gha * gha);
struct table_entry *       tble_create_gfp(struct gpa * gpa,
                                           struct gha * gha,
                                           gfp_t        flags);
int                        tble_destroy(struct table_entry * entry);

const struct gpa *         tble_pa(const struct table_entry * entry);
const struct gha *         tble_ha(const struct table_entry * entry);

struct table;

int                        tbl_add(struct table * instance,
                                   struct gpa *   pa,
                                   struct gha *   ha);
int                        tbl_remove(struct table *             instance,
                                      const struct table_entry * entry);

/* Replaces the old gha with the new one, takes the ownership */
int                        tbl_update_by_gpa(struct table *     instance,
                                             const struct gpa * gpa,
                                             struct gha *       gha);

const struct table_entry * tbl_find(struct table *     instance,
                                    const struct gpa * pa,
                                    const struct gha * ha);
const struct table_entry * tbl_find_by_gha(struct table *     instance,
                                           const struct gha * address);
const struct table_entry * tbl_find_by_gpa(struct table *     instance,
                                           const struct gpa * address);

int                        tbls_init(void);
void                       tbls_fini(void);
int                        tbls_create(uint16_t ptype, size_t hwl);
int                        tbls_destroy(uint16_t ptype);
struct table *             tbls_find(uint16_t ptype);

#endif
