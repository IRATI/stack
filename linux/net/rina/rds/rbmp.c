/*
 * RINA Bitmaps
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
#include <linux/bitmap.h>

#define RINA_PREFIX "rbmp"

#include "logs.h"
#include "debug.h"
#include "rmem.h"
#include "rbmp.h"

#define BITS_IN_BITMAP ((2 << BITS_PER_BYTE) * sizeof(size_t))

struct rbmp {
        ssize_t offset;
        size_t  size;

        DECLARE_BITMAP(bitmap, BITS_IN_BITMAP);
};

static struct rbmp * rbmp_create_gfp(gfp_t flags, size_t bits, ssize_t offset)
{
        struct rbmp * tmp;

        if (bits == 0)
                return NULL;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->size   = bits;
        tmp->offset = offset;
        bitmap_zero(tmp->bitmap, BITS_IN_BITMAP);

        return tmp;
}

struct rbmp * rbmp_create(size_t bits, ssize_t offset)
{ return rbmp_create_gfp(GFP_KERNEL, bits, offset); }
EXPORT_SYMBOL(rbmp_create);

struct rbmp * rbmp_create_ni(size_t bits, ssize_t offset)
{ return rbmp_create_gfp(GFP_ATOMIC, bits, offset); }
EXPORT_SYMBOL(rbmp_create_ni);

int rbmp_destroy(struct rbmp * b)
{
        if (!b)
                return -1;

        rkfree(b);

        return 0;
}
EXPORT_SYMBOL(rbmp_destroy);

static ssize_t bad_id(struct rbmp * b)
{
        ASSERT(b);

        return b->offset - 1;
}

ssize_t rbmp_allocate(struct rbmp * b)
{
        ssize_t id;

        if (!b)
                return bad_id(b);

        id = (ssize_t) bitmap_find_next_zero_area(b->bitmap,
                                                  BITS_IN_BITMAP,
                                                  0, 1, 0);
        if (id < 0)
                return bad_id(b);

        bitmap_set(b->bitmap, id, 1);

        return id + b->offset;
}
EXPORT_SYMBOL(rbmp_allocate);

static bool is_id_ok(struct rbmp * b, ssize_t id)
{
        ASSERT(b);

        if ((id < b->offset) || (id > (b->offset + b->size)))
                return false;

        return true;
}

bool rbmp_is_id_ok(struct rbmp * b, ssize_t id)
{
        if (!b)
                return false;

        return is_id_ok(b, id);
}
EXPORT_SYMBOL(rbmp_is_id_ok);

int rbmp_release(struct rbmp * b,
                 ssize_t       id)
{
        ssize_t rid;

        if (!b)
                return -1;

        if (!is_id_ok(b, id))
                return -1;

        rid = id - b->offset;

        bitmap_clear(b->bitmap, id, 1);

        return 0;
}
EXPORT_SYMBOL(rbmp_release);
