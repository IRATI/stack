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

struct buffer * buffer_create_gfp(gfp_t  flags,
				  uint32_t size)
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

struct buffer * buffer_create(uint32_t size)
{ return buffer_create_gfp(GFP_KERNEL, size); }
EXPORT_SYMBOL(buffer_create);

struct buffer * buffer_create_ni(uint32_t size)
{ return buffer_create_gfp(GFP_ATOMIC, size); }
EXPORT_SYMBOL(buffer_create_ni);

ssize_t buffer_length(const struct buffer * b)
{
        if (!b)
                return -1;

        return b->size;
}
EXPORT_SYMBOL(buffer_length);

const void * buffer_data_ro(const struct buffer * b)
{
        if (!b)
                return NULL;

        return b->data;
}
EXPORT_SYMBOL(buffer_data_ro);
