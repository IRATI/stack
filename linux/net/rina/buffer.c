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

#include <linux/export.h>
#include <linux/types.h>

#define RINA_PREFIX "buffer"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "buffer.h"

struct buffer {
        char * data;
        size_t size;
};

bool buffer_is_ok(const struct buffer * b)
{ return (b && b->data && b->size) ? true : false; }
EXPORT_SYMBOL(buffer_is_ok);

static int __buffer_destroy(struct buffer * b)
{
        ASSERT(b);

        if (b->data) rkfree(b->data);
        rkfree(b);

        return 0;
}

int buffer_destroy(struct buffer * b)
{
        if (!b)
                return -1;

        return __buffer_destroy(b);
}
EXPORT_SYMBOL(buffer_destroy);

struct buffer * buffer_create_with_gfp(gfp_t  flags,
                                       void * data,
                                       size_t size)
{
        struct buffer * tmp;

        if (!data) {
                LOG_ERR("Cannot create buffer, data is NULL");
                return NULL;
        }
        if (!size) {
                LOG_ERR("Cannot create buffer, data is 0 size");
                return NULL;
        }

        tmp = rkmalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->data = data;
        tmp->size = size;

        return tmp;
}

struct buffer * buffer_create_with(void * data,
                                   size_t size)
{ return buffer_create_with_gfp(GFP_KERNEL, data, size); }
EXPORT_SYMBOL(buffer_create_with);

struct buffer * buffer_create_with_ni(void * data,
                                      size_t size)
{ return buffer_create_with_gfp(GFP_ATOMIC, data, size); }
EXPORT_SYMBOL(buffer_create_with_ni);

struct buffer * buffer_create_from_gfp(gfp_t        flags,
                                       const void * data,
                                       size_t       size)
{
        struct buffer * tmp;

        if (!data) {
                LOG_ERR("Cannot create buffer, data is NULL");
                return NULL;
        }
        if (!size) {
                LOG_ERR("Cannot create buffer, data is 0 size");
                return NULL;
        }

        tmp = rkmalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->data = rkmalloc(size, flags);
        if (!tmp->data) {
                __buffer_destroy(tmp);
                return NULL;
        }
        if (!memcpy(tmp->data, data, size)) {
                __buffer_destroy(tmp);
                return NULL;
        }

        tmp->size = size;

        return tmp;
}

struct buffer * buffer_create_from(const void * data, size_t size)
{ return buffer_create_from_gfp(GFP_KERNEL, data, size); }
EXPORT_SYMBOL(buffer_create_from);

struct buffer * buffer_create_from_ni(const void * data, size_t size)
{ return buffer_create_from_gfp(GFP_ATOMIC, data, size); }
EXPORT_SYMBOL(buffer_create_from_ni);

struct buffer * buffer_create_gfp(gfp_t  flags,
                                  size_t size)
{
        struct buffer * tmp;

        if (!size) {
                LOG_ERR("Cannot create buffer, data is 0 size");
                return NULL;
        }

        tmp = rkmalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->data = rkmalloc(size, flags);
        if (!tmp->data) {
                rkfree(tmp);
                return NULL;
        }

        tmp->size = size;

        return tmp;
}

struct buffer * buffer_create(size_t size)
{ return buffer_create_gfp(GFP_KERNEL, size); }
EXPORT_SYMBOL(buffer_create);

struct buffer * buffer_create_ni(size_t size)
{ return buffer_create_gfp(GFP_ATOMIC, size); }
EXPORT_SYMBOL(buffer_create_ni);

/* FIXME: This function must disappear from the API */
struct buffer * buffer_dup_gfp(gfp_t                 flags,
                               const struct buffer * b)
{
        struct buffer * tmp;
        void *          m;

        if (!buffer_is_ok(b))
                return NULL;

        m = rkmalloc(b->size, flags);
        if (!m)
                return NULL;

        if (!memcpy(m, b->data, b->size)) {
                rkfree(m);
                return NULL;
        }

        tmp = buffer_create_with_gfp(flags, m, b->size);
        if (!tmp) {
                rkfree(m);
                return NULL;
        }

        return tmp;
}
EXPORT_SYMBOL(buffer_dup_gfp);

struct buffer * buffer_dup(const struct buffer * b)
{ return buffer_dup_gfp(GFP_KERNEL, b); }
EXPORT_SYMBOL(buffer_dup);

struct buffer * buffer_dup_ni(const struct buffer * b)
{ return buffer_dup_gfp(GFP_ATOMIC, b); }
EXPORT_SYMBOL(buffer_dup_ni);

ssize_t buffer_length(const struct buffer * b)
{
        if (!buffer_is_ok(b))
                return -1;

        return b->size;
}
EXPORT_SYMBOL(buffer_length);

int buffer_set_length(struct buffer * b, size_t len)
{
        if (len > b->size) {
                return -1;
        }

        b->size = len;

        return 0;
}
EXPORT_SYMBOL(buffer_set_length);

const void * buffer_data_ro(const struct buffer * b)
{
        if (!buffer_is_ok(b))
                return NULL;

        return b->data;
}
EXPORT_SYMBOL(buffer_data_ro);

void * buffer_data_rw(struct buffer * b)
{
        if (!buffer_is_ok(b))
                return NULL;

        return b->data;
}
EXPORT_SYMBOL(buffer_data_rw);

/* FIXME: To be heavily hammered, it is temporary (lastin' forever, sigh) */
static void buffer_assign(struct buffer * buffer,
                          char *          new_data,
                          size_t          new_len)
{
        ASSERT(buffer);
        ASSERT(new_data);
        ASSERT(new_len);

        if (buffer->data)
                rkfree(buffer->data);

        buffer->data = new_data;
        buffer->size = new_len;
}

/* FIXME: To be heavily hammered, it is temporary (lastin' forever, sigh) */
int buffer_head_grow(gfp_t           flags,
                     struct buffer * buffer,
                     size_t          bytes)
{
        char * new_data;

        if (!buffer_is_ok(buffer))
                return -1;
        if (!bytes)
                return 0; /* This is a NO-OP */

        new_data = rkmalloc(buffer->size + bytes, flags);
        if (!new_data)
                return -1;

        memcpy(new_data + bytes, buffer->data, buffer->size);

        buffer_assign(buffer, new_data, buffer->size + bytes);

        return 0;
}
EXPORT_SYMBOL(buffer_head_grow);

/* FIXME: To be heavily hammered, it is temporary (lastin' forever, sigh) */
int buffer_head_shrink(gfp_t           flags,
                       struct buffer * buffer,
                       size_t          bytes)
{
        char * new_data;

        if (!buffer_is_ok(buffer))
                return -1;
        if (!bytes)
                return 0; /* This is a NO-OP */

        new_data = rkmalloc(buffer->size - bytes, flags);
        if (!new_data)
                return -1;

        memcpy(new_data, buffer->data + bytes, buffer->size - bytes);

        buffer_assign(buffer, new_data, buffer->size - bytes);

        return 0;
}
EXPORT_SYMBOL(buffer_head_shrink);

/* FIXME: To be heavily hammered, it is temporary (lastin' forever, sigh) */
int buffer_tail_grow(struct buffer * buffer,
                     size_t          bytes)
{
        char * new_data;

        if (!buffer_is_ok(buffer))
                return -1;
        if (!bytes)
                return 0; /* This is a NO-OP */

        new_data = rkmalloc(buffer->size + bytes, GFP_KERNEL);
        if (!new_data)
                return -1;

        memcpy(new_data, buffer->data, buffer->size);

        buffer_assign(buffer, new_data, buffer->size + bytes);

        return 0;
}
EXPORT_SYMBOL(buffer_tail_grow);

/* FIXME: To be heavily hammered, it is temporary (lastin' forever, sigh) */
int buffer_tail_shrink(struct buffer * buffer,
                       size_t          bytes)
{
        char * new_data;

        if (!buffer_is_ok(buffer))
                return -1;
        if (!bytes)
                return 0; /* This is a NO-OP */

        new_data = rkmalloc(buffer->size - bytes, GFP_KERNEL);
        if (!new_data)
                return -1;

        memcpy(new_data, buffer->data, buffer->size - bytes);

        buffer_assign(buffer, new_data, buffer->size - bytes);

        return 0;
}
EXPORT_SYMBOL(buffer_tail_shrink);
