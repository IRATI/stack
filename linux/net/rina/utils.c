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

#ifdef CONFIG_RINA_MEMORY_TAMPERING
struct memblock_header {
        size_t  inner_length;
        uint8_t filler[BITS_PER_LONG / 8];
};

struct memblock_footer {
        uint8_t filler[BITS_PER_LONG / 8];
};

static void filler_init(uint8_t * f, size_t length)
{
        size_t i;

        LOG_DBG("Applying filler at %pK, size %zd", f, length);

        for (i = 0; i < length; i++) {
                *f = i % 0xff;
                f++;
        }
}

static int is_filler_ok(const uint8_t * f, size_t length)
{
        size_t i;

        LOG_DBG("Checking filler at %pK, size %zd", f, length);

        for (i = 0; i < length; i++) {
                if (*f != i % 0xff) {
                        LOG_ERR("Filler corrupted at %pK", f);
                        return 0;
                }
                f++;
        }

        return 1;
}

static void header_filler_init(struct memblock_header * m)
{ filler_init(m->filler, ARRAY_SIZE(m->filler)); }

static int is_header_filler_ok(const struct memblock_header * m)
{ return is_filler_ok(m->filler, ARRAY_SIZE(m->filler)); }

static void footer_filler_init(struct memblock_footer * m)
{ filler_init(m->filler, ARRAY_SIZE(m->filler)); }

static int is_footer_filler_ok(const struct memblock_footer * m)
{ return is_filler_ok(m->filler, ARRAY_SIZE(m->filler)); }
#endif

static void * generic_alloc(void * (* alloc_func)(size_t size, gfp_t flags),
                            size_t size,
                            gfp_t flags)
{
        size_t                   real_size;
        void *                   ptr;
#ifdef CONFIG_RINA_MEMORY_TAMPERING
        struct memblock_header * header;
        struct memblock_footer * footer;
#endif

        ASSERT(alloc_func);

#ifdef CONFIG_RINA_MEMORY_CHECKS
        /* Should this be transformed into an assertion ? */
        if (!size) {
                /* We will consider 0 bytes allocations as meaningless */
                LOG_ERR("Allocating 0 bytes is meaningless");
                return NULL;
        }
#endif

        real_size = size;
#ifdef CONFIG_RINA_MEMORY_TAMPERING
        real_size += sizeof(*header) + sizeof(*footer);
#endif
        
        ptr = alloc_func(real_size, flags);
        if (!ptr) {
                LOG_ERR("Cannot allocate %zd bytes", size);
                return NULL;
        }
        LOG_DBG("The requested block is at %pK, size %zd", ptr, real_size);

#ifdef CONFIG_RINA_MEMORY_TAMPERING
        if (!ptr) {
                LOG_ERR("Cannot tamper a NULL memory block");
                return ptr;
        }

        header               =
                (struct memblock_header *) ptr;
        header_filler_init(header);
        header->inner_length = size;
        ptr                  =
                (void *)((uint8_t *) header + sizeof(*header));
        footer               =
                (struct memblock_footer *)((uint8_t *) ptr + size);
        footer_filler_init(footer);

        ASSERT((uint8_t *) header <= (uint8_t *) ptr);
        ASSERT((uint8_t *) ptr    <= (uint8_t *) footer);

        LOG_DBG("Memblock header at %pK/%zd", header, sizeof(*header));
        LOG_DBG("Memblock footer at %pK/%zd", footer, sizeof(*footer));

        LOG_DBG("Returning tampered memory block %pK/%zd", ptr, real_size);
#endif

#ifdef CONFIG_RINA_MEMORY_PTRS_DUMP
        LOG_DBG("rkmalloc(%zd) = %pK %s", size, ptr);
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
#ifdef CONFIG_RINA_MEMORY_TAMPERING
        struct memblock_header * header;
        struct memblock_footer * footer;
#ifdef CONFIG_RINA_MEMORY_POISONING
        uint8_t *                p;
        size_t                   i;
        size_t                   size;
#endif
#endif

#ifdef CONFIG_RINA_MEMORY_PTRS_DUMP
        LOG_DBG("rkfree(%pK)", ptr);
#endif

        ASSERT(ptr);

#ifdef CONFIG_RINA_MEMORY_TAMPERING
        header = (struct memblock_header *)
                ((uint8_t *) ptr - sizeof(*header));
        footer = (struct memblock_footer *)
                ((uint8_t *) ptr + header->inner_length);

        if (!is_header_filler_ok(header)) {
                LOG_CRIT("Memory block %pK has been corrupted (header)", ptr);
                BUG();
        }
        if (!is_footer_filler_ok(footer)) {
                LOG_CRIT("Memory block %pK has been corrupted (footer)", ptr);
                BUG();
        }

#ifdef CONFIG_RINA_MEMORY_POISONING
        p    = (uint8_t *) ptr;
        size = header->inner_length;

        LOG_DBG("Now poisoning the memory block (%pK-%pK)", p, p + size - 1);

        LOG_DBG("Poisoning memory block at %pK, length %zd", p, size);

        for (i = 0; i < size; i++) {
                *p = (uint8_t) i;
                p++;
        }
#endif

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
