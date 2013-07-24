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
#include "utils.h"
#include "debug.h"

int is_value_in_range(int value,
                      int min_value,
                      int max_value)
{ return ((value >= min_value || value <= max_value) ? 1 : 0); }

/*
 * NOTE:
 *
 * These functions will contain wrappers to check for memory corruptions etc.
 * These wrappers/checks will be enabled/disabled at compilation time (via a
 * Kconfig rule)
 *
 * Francesco
 */

void * rkmalloc(size_t size, gfp_t flags)
{
        void * tmp;

#ifdef CONFIG_RINA_MEMORY_CHECKS
        if (!size) {
                /* We wont use 0 bytes allocation */
                LOG_ERR("Allocating 0 bytes is meaningless");
                return NULL;
        }
#endif

        tmp = kmalloc(size, flags);
        if (!tmp) {
                LOG_ERR("Cannot allocate %zd bytes of memory", size);
                return NULL;
        }

#ifdef CONFIG_RINA_MEMORY_PTRS_DUMP
        LOG_DBG("rkmalloc(%zd) = %pK", size, tmp);
#endif

        return tmp;
}
EXPORT_SYMBOL(rkmalloc);

void * rkzalloc(size_t size, gfp_t flags)
{
        void * tmp;

#ifdef CONFIG_RINA_MEMORY_CHECKS
        if (!size) {
                /* We wont use 0 bytes allocation */
                LOG_ERR("Allocating 0 bytes is meaningless");
                return NULL;
        }
#endif

        tmp = kzalloc(size, flags);
        if (!tmp) {
                LOG_ERR("Cannot allocate %zd bytes of memory", size);
                return NULL;
        }

#ifdef CONFIG_RINA_MEMORY_PTRS_DUMP
        LOG_DBG("rkzalloc(%zd) = %pK", size, tmp);
#endif

        return tmp;
}
EXPORT_SYMBOL(rkzalloc);

void rkfree(void * ptr)
{
#ifdef CONFIG_RINA_MEMORY_PTRS_DUMP
        LOG_DBG("rkfree(%pK)", ptr);
#endif

#ifdef CONFIG_RINA_MEMORY_CHECKS
        if (!ptr) {
                LOG_ERR("Passed pointer is NULL, cannot free");
                return;
        }
#endif

        ASSERT(ptr);

        kfree(ptr);
}
EXPORT_SYMBOL(rkfree);
