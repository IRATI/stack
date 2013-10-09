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

#ifndef ARP_826_CACHE_H
#define ARP_826_CACHE_H

struct cache_line;

struct cache_line * cl_create(void);
void                cl_destroy(struct cache_line * instance);

int                 cl_add(struct cache_line *   instance,
                           size_t                protocol_address_length,
                           const unsigned char * source_protocol_address,
                           const unsigned char * target_protocol_address,
                           size_t                hardware_address_length,
                           const unsigned char * source_hardware_address,
                           const unsigned char * target_hardware_address);

#endif
