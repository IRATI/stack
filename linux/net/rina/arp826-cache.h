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

#ifndef ARP_826_CACHE_H
#define ARP_826_CACHE_H

#include "arp826-utils.h"

struct cache_entry;

struct cache_entry * ce_create(const struct gpa * gpa,
                               const uint8_t *    hardware_address,
                               size_t             hardware_address_length);
void                 ce_destroy(struct cache_entry * entry);
const struct gpa *   ce_pa(struct cache_entry * entry);
const uint8_t *      ce_ha(struct cache_entry * entry);

struct cache_line;

struct cache_line *        cl_create(size_t hw_address_length);
void                       cl_destroy(struct cache_line * instance);

size_t                     cl_hwa_length(struct cache_line * instance);

/*
 * NOTE:
 *   Takes the ownership of the passed gpa. Hardware address length is
 *   implicitly obtained from the cache-line (cl_create) so there are no
 *   needs to pass the length here
 */
int                        cl_add(struct cache_line * instance,
                                  struct gpa *        protocol_address,
                                  const uint8_t *     hardware_address);
void                       cl_remove(struct cache_line *        instance,
                                     const struct cache_entry * entry);

const struct cache_entry * cl_find_by_ha(struct cache_line * instance,
                                         const uint8_t *     hardware_addr);
const struct cache_entry * cl_find_by_gpa(struct cache_line * instance,
                                          const struct gpa *  proto_addr);

#endif
