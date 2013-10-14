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

struct cache_entry;

/* FIXME: There's no need to have this "method" public ... */
struct cache_entry *       ce_create(const struct gpa * gpa,
                                     const struct gha * gha);
/* FIXME: There's no need to have this "method" public ... */
void                       ce_destroy(struct cache_entry * entry);

const struct gpa *         ce_pa(struct cache_entry * entry);
const struct gha *         ce_ha(struct cache_entry * entry);

struct cache_line;

/*
 * NOTE:
 *   Takes the ownership of the passed gpa. Hardware address length is
 *   implicitly obtained from the cache-line (cl_create) so there are no
 *   needs to pass the length here
 */
int                        cl_entry_add(struct cache_line * instance,
                                        struct gpa *        pa,
                                        struct gha *        ha);
void                       cl_entry_remove(struct cache_line *        instance,
                                           const struct cache_entry * entry);

const struct cache_entry * cl_entry_find(struct cache_line * instance,
                                         const struct gpa *  pa,
                                         const struct gha *  ha);
const struct cache_entry * cl_entry_find_by_gha(struct cache_line * instance,
                                                const struct gha *  address);
const struct cache_entry * cl_entry_find_by_gpa(struct cache_line * instance,
                                                const struct gpa *  address);

int                        tbls_create(uint16_t ptype, size_t hwl);
int                        tbls_destroy(uint16_t ptype);
struct cache_line *        tbls_find(uint16_t ptype);

#endif
