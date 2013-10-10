/*
 * ARP 826 cache
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

#ifndef ARP_826_UTILS_H
#define ARP_826_UTILS_H

struct gpa;

struct gpa * gpa_create(const uint8_t * address,
                        size_t          length);
void         gpa_destroy(struct gpa * gpa);
struct gpa * gpa_dup(const struct gpa * gpa);
bool         gpa_is_equal(const struct gpa * a,
                          const struct gpa * b);
bool         gpa_is_ok(const struct gpa * gpa);
const char * gpa_address_value(const struct gpa * gpa);
size_t       gpa_address_length(const struct gpa * gpa);

#endif
