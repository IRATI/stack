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
#include <linux/uaccess.h>

/* FIXME: We should stick the caller in the prefix (rk[mz]alloc oriented) */
#define RINA_PREFIX "utils"

#include "logs.h"
#include "utils.h"
#include "debug.h"

int is_value_in_range(int value, int min_value, int max_value)
{ return ((value >= min_value || value <= max_value) ? 1 : 0); }

#ifdef RINA_MEMORY_TAMPERING
struct memblock_header {
        uint8_t filler[BITS_PER_LONG / sizeof(uint8_t)];
        size_t  inner_length;
};

struct memblock_footer {
        uint8_t filler[BITS_PER_LONG / sizeof(uint8_t)];
};

static void filler_init(uint8_t * f, size_t len)
{
        for (size_t i = 0; i != len; i++) {
                *f = i;
        }
}

static void is_filler_ok(uint8_t * f, size_t len)
{
        for (size_t i = 0; i != len; i++)
                if (*f != i)
                        return 0;
        return 1;
}

static void header_filler_init(memblock_header * m)
{ filler_init(m->filler, sizeof(m->filler)); }

static int is_header_filler_ok(memblock_footer * m)
{
        LOG_MISSING;

        return 1;
}

static void footer_filler_init(memblock_footer * h)
{ filler_init(m->filler, sizeof(m->filler)); }

static int is_footer_filler_ok(memblock_footer * h)
{
        LOG_MISSING;

        return 1;
}
#endif

static void * generic_alloc(void * (* alloc_func)(size_t size, gfp_t flags),
                            size_t size,
                            gfp_t flags)
{
        size_t                   real_size;
        void *                   ptr;
#ifdef RINA_MEMORY_TAMPERING
        struct memblock_header * header;
        struct memblock_footer * footer;
#endif

#ifdef CONFIG_RINA_MEMORY_CHECKS
        /* Should this be transformed into an assertion ? */
        if (!size) {
                /* We will consider 0 bytes allocations as meaningless */
                LOG_ERR("Allocating 0 bytes is meaningless");
                return NULL;
        }
#endif

        real_size = size;
#ifdef RINA_MEMORY_TAMPERING
        real_size += sizeof(*header) + sizeof(*footer);
#endif
        
        ptr = alloc_func(real_size, flags);
        if (!ptr) {
                LOG_ERR("Cannot allocate %zd bytes", size);
                return NULL;
        }

#ifdef RINA_MEMORY_TAMPERING
        header               = ptr;
        header_filler_init(header);
        header->inner_length = size;
        ptr                  = header + sizeof(*header) / sizeof(void *);
        footer               = ptr    + sizeof(*footer) / sizeof(void *);
        footer_filler_init(footer);
#endif

#ifdef CONFIG_RINA_MEMORY_PTRS_DUMP
        LOG_DBG("rkmalloc(%zd) = %pK", size, ptr);
#endif

        return ptr;
}

void * rkmalloc(size_t size, gfp_t flags)
{ return generic_alloc(kmalloc, size, flags); }
EXPORT_SYMBOL(rkmalloc);

void * rkzalloc(size_t size, gfp_t flags)
{ return generic_alloc(kzalloc, size, flags); }
EXPORT_SYMBOL(rkzalloc);

void rkfree(void * ptr)
{
#ifdef RINA_MEMORY_TAMPERING
        struct memblock_header * header;
        struct memblock_header * footer;
        size_t                   size;
#endif
#ifdef RINA_MEMORY_POISONING
        uint8_t *                p;
#endif

#ifdef CONFIG_RINA_MEMORY_PTRS_DUMP
        LOG_DBG("rkfree(%pK)", ptr);
#endif

        ASSERT(ptr);

#ifdef RINA_MEMORY_TAMPERING
        header = ptr - sizeof(*header) / sizeof(void *);
        footer = ptr + sizeof(*header) + header->length / sizeof(void *);
        if (is_header_filler_ok(header)) {
                LOG_CRIT("Memory block %pK has been corrupted (header)", ptr);
                BUG();
        }
        if (is_footer_filler_ok(footer)) {
                LOG_CRIT("Memory block %pK has been corrupted (footer)", ptr);
                BUG();
        }
#endif
#ifdef RINA_MEMORY_POISONING
        p      = ptr + sizeof(*header);
        size   = header->inner_length;
        for (i = 0; i < size; i++) {
                *p = (uint8_t) i;
                p++;
        }
#endif

#ifdef RINA_MEMORY_TAMPERING
        ptr = header;
#endif

        kfree(ptr);
}
EXPORT_SYMBOL(rkfree);

char * strdup_from_user(const char __user * src)
{
        size_t size;
        char * tmp;

        if (!src)
        	return NULL;

        size = strlen_user(src); /* Includes the terminating NUL */
        if (!size)
            return NULL;

        tmp = rkmalloc(size, GFP_KERNEL);
        if (!tmp)
                return NULL;

        /*
         * strncpy_from_user() copies the terminating NUL. On success, returns
         * the length of the string (not including the trailing NUL). We care
         * on having the complete-copy, parts of it are a no-go
         */
        if (strncpy_from_user(tmp, src, size) != (size - 1)) {
                rkfree(tmp);
                return NULL;
        }

        return tmp;
}
