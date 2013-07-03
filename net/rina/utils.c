/*
 * Utilities
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

#include <linux/slab.h>
#include <linux/kobject.h>
#include <linux/export.h>

/* FIXME: We should stick the caller in the prefix (rk[mz]alloc oriented) */
#define RINA_PREFIX "utils"

#include "logs.h"

void * rkmalloc(size_t size, int type)
{
        void * tmp = kmalloc(size, type);
        if (!tmp) {
                LOG_ERR("Cannot allocate %zd bytes of memory", size);
                return NULL;
        }

        return tmp;
}
EXPORT_SYMBOL(rkmalloc);

void * rkzalloc(size_t size, int type)
{
        void * tmp = kzalloc(size, type);
        if (!tmp) {
                LOG_ERR("Cannot allocate %zd bytes of memory", size);
                return NULL;
        }

        return tmp;
}
EXPORT_SYMBOL(rkzalloc);
