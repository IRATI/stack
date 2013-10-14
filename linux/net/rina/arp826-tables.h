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

const struct gpa *         tble_pa(struct table_entry * entry);
const struct gha *         tble_ha(struct table_entry * entry);

struct table;

/*
 * NOTE:
 *   Takes the ownership of the passed gpa. Hardware address length is
 *   implicitly obtained so there are no needs to pass the length here
 */
int                        tbl_add(struct table * instance,
                                   struct gpa *   pa,
                                   struct gha *   ha);
void                       tbl_remove(struct table *             instance,
                                      const struct table_entry * entry);

const struct table_entry * tbl_find(struct table *     instance,
                                    const struct gpa * pa,
                                    const struct gha * ha);
const struct table_entry * tbl_find_by_gha(struct table *     instance,
                                           const struct gha * address);
const struct table_entry * tbl_find_by_gpa(struct table *     instance,
                                           const struct gpa * address);

int                        tbls_create(uint16_t ptype, size_t hwl);
int                        tbls_destroy(uint16_t ptype);
struct table *             tbls_find(uint16_t ptype);

#endif
