/*
 * RINA References
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
#include <linux/list.h>
#include <linux/types.h>

#define RINA_PREFIX "rref"

#include "logs.h"
#include "debug.h"
#include "rmem.h"
#include "rref.h"

struct rref {
        ssize_t count;
};

static bool rref_is_ok(struct rref * r)
{ return r ? true : false; }

static struct rref * rref_create_gfp(gfp_t flags, size_t count)
{
        struct rref * tmp;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->count = count;

        return tmp;
}

struct rref * rref_create(ssize_t count)
{ return rref_create_gfp(GFP_KERNEL, count); }
EXPORT_SYMBOL(rref_create);

struct rref * rref_create_ni(ssize_t count)
{ return rref_create_gfp(GFP_ATOMIC, count); }
EXPORT_SYMBOL(rref_create_ni);

int rref_destroy(struct rref * r)
{
        if (!rref_is_ok(r))
                return -1;

        rkfree(r);

        return 0;
}

ssize_t rref_inc(struct rref * r)
{
        if (!rref_is_ok(r))
                return -1;

        return r->count++;
}
EXPORT_SYMBOL(rref_inc);

ssize_t rref_dec(struct rref * r)
{
        if (!rref_is_ok(r))
                return -1;

        return r->count--;
}
EXPORT_SYMBOL(rref_dec);

ssize_t rref_value(struct rref * r)
{
        if (!rref_is_ok(r))
                return -1;

        return r->count;
}
EXPORT_SYMBOL(rref_value);
