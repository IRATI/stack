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
#include <linux/timer.h>

#define RINA_PREFIX "rtimer"

#include "logs.h"
#include "debug.h"
#include "rmem.h"
#include "rtimer.h"

struct rtimer {
        int bogus;
};

static struct rtimer * rtimer_create_gfp(gfp_t flags)
{
        struct rtimer * tmp;

        tmp = rkmalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        return tmp;
}

struct rtimer * rtimer_create(void)
{ return rtimer_create_gfp(GFP_KERNEL); }
EXPORT_SYMBOL(rtimer_create);

struct rtimer * rtimer_create_ni(void)
{ return rtimer_create_gfp(GFP_ATOMIC); }
EXPORT_SYMBOL(rtimer_create_ni);

int rtimer_destroy(struct rtimer * timer)
{
        if (!timer)
                return -1;

        rkfree(timer);

        return 0;
}
EXPORT_SYMBOL(rtimer_destroy);
