/*
 * Buffers
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

#ifndef RINA_BUFFER_H
#define RINA_BUFFER_H

/* NOTE: Creates an uninitialized buffer (data might be garbage) */
struct buffer * buffer_create(uint32_t size);
struct buffer * buffer_create_ni(uint32_t size);

int             buffer_destroy(struct buffer * b);

/* NOTE: The following function may return -1 */
ssize_t         buffer_length(const struct buffer * b);

/* NOTE: Returns the raw buffer memory, watch-out ... */
const void *    buffer_data_ro(const struct buffer * b); /* Read only */

#endif
