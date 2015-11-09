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

struct buffer;

/* NOTE: Creates a buffer from raw data (takes ownership) */
struct buffer * buffer_create_with(void * data, size_t size);
struct buffer * buffer_create_with_ni(void * data, size_t size);

struct buffer * buffer_create_from(const void * data, size_t size);
struct buffer * buffer_create_from_ni(const void * data, size_t size);

/* NOTE: Creates an uninitialized buffer (data might be garbage) */
struct buffer * buffer_create(size_t size);
struct buffer * buffer_create_ni(size_t size);

int             buffer_destroy(struct buffer * b);

/* NOTE: The following function may return -1 */
ssize_t         buffer_length(const struct buffer * b);
int buffer_set_length(struct buffer * b, size_t len);

/* NOTE: Returns the raw buffer memory, watch-out ... */
const void *    buffer_data_ro(const struct buffer * b); /* Read only */
void *          buffer_data_rw(struct buffer * b);       /* Read/Write */

struct buffer * buffer_dup(const struct buffer * b);
struct buffer * buffer_dup_ni(const struct buffer * b);
bool            buffer_is_ok(const struct buffer * b);

int             buffer_head_grow(gfp_t flags, struct buffer * b, size_t bytes);
int             buffer_head_shrink(gfp_t           flags,
                                   struct buffer * b,
                                   size_t          bytes);
int             buffer_tail_grow(struct buffer * b, size_t bytes);
int             buffer_tail_shrink(struct buffer * b, size_t bytes);

#endif
