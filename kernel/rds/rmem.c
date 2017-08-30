/*
 * RINA Memory
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

#define RINA_PREFIX "rmem"

#include "logs.h"
#include "debug.h"
#include "rmem.h"

#ifdef CONFIG_RINA_MEMORY_TAMPERING
struct memblock_header {
        size_t  inner_length;
        uint8_t filler[BITS_PER_LONG / 8]; /* Mah ... */
};

struct memblock_footer {
        uint8_t filler[BITS_PER_LONG / 8]; /* Mah ... */
};

static void * inner2outer(void * ptr)
{
        ASSERT(ptr);

#ifdef CONFIG_RINA_MEMORY_TAMPERING
        return (struct memblock_header *)
                ((uint8_t *) ptr - sizeof(struct memblock_header));
#else
        return ptr;
#endif
}

#if 0
static void * outer2inner(void * ptr)
{
        ASSERT(ptr);

#ifdef CONFIG_RINA_MEMORY_TAMPERING
        return (struct memblock_header *)
                ((uint8_t *) ptr + sizeof(struct memblock_header));
#else
        return ptr;
#endif
}
#endif

static void mb_filler_init(uint8_t * f, size_t length)
{
        size_t i;

#ifdef CONFIG_RINA_MEMORY_TAMPERING_VERBOSE
        LOG_DBG("Applying filler at %pK, size %zd", f, length);
#endif
        for (i = 0; i < length; i++) {
                *f = i % 0xff;
                f++;
        }
}

static bool mb_is_filler_ok(const uint8_t * f, size_t length)
{
        size_t          i;
        const uint8_t * g;

        ASSERT(f);

        g = f;

#ifdef CONFIG_RINA_MEMORY_TAMPERING_VERBOSE
        LOG_DBG("Checking filler at %pK, size %zd", g, length);
#endif
        for (i = 0; i < length; i++) {
                if (*g != (uint8_t) i % 0xff) {
                        size_t          j;
                        const uint8_t * h;

                        LOG_ERR("Filler corrupted at %pK "
                                "(pos = %zd, obt = 0x%02X, exp = 0x%02X)",
                                g, i, *g, (uint8_t) i % 0xff);

                        LOG_ERR("Filler dump begin");

                        h = f;
                        for (j = 0; j < length; j++) {
                                LOG_ERR("  0x%pK = 0x%02X", h, *h);
                                h++;
                        }

                        LOG_ERR("Filler dump end");

                        return false;
                }
                g++;
        }

        return true;
}

static void mb_header_filler_init(struct memblock_header * m)
{ mb_filler_init(m->filler, ARRAY_SIZE(m->filler)); }

static bool mb_is_header_filler_ok(const struct memblock_header * m)
{ return mb_is_filler_ok(m->filler, ARRAY_SIZE(m->filler)); }

static void mb_footer_filler_init(struct memblock_footer * m)
{ mb_filler_init(m->filler, ARRAY_SIZE(m->filler)); }

static bool mb_is_footer_filler_ok(const struct memblock_footer * m)
{ return mb_is_filler_ok(m->filler, ARRAY_SIZE(m->filler)); }
#endif

#ifdef CONFIG_RINA_MEMORY_POISONING
static void poison(void * ptr, size_t size)
{
        size_t    i;
        uint8_t * p;

        if (!ptr) {
                LOG_ERR("Cannot poison NULL memory block");
                return;
        }
        if (size == 0) {
                LOG_ERR("Cannot poison a zero size memory block");
                return;
        }

        ASSERT(ptr  != NULL);
        ASSERT(size != 0);

        p = (uint8_t *) ptr;

#ifdef CONFIG_RINA_MEMORY_POISONING_VERBOSE
        LOG_DBG("Poisoning memory %pK-%pK (%zd bytes)", p, p + size - 1, size);
#endif
        for (i = 0; i < size; i++) {
                *p = (uint8_t) i;
                p++;
        }
}
#endif

#ifdef CONFIG_RINA_MEMORY_STATS
static atomic_t mem_stats[] = {
        ATOMIC_INIT(0) /* 2 ^  0 */,
        ATOMIC_INIT(0) /* 2 ^  1 */,
        ATOMIC_INIT(0) /* 2 ^  2 */,
        ATOMIC_INIT(0) /* 2 ^  3 */,
        ATOMIC_INIT(0) /* 2 ^  4 */,
        ATOMIC_INIT(0) /* 2 ^  5 */,
        ATOMIC_INIT(0) /* 2 ^  6 */,
        ATOMIC_INIT(0) /* 2 ^  7 */,
        ATOMIC_INIT(0) /* 2 ^  8 */,
        ATOMIC_INIT(0) /* 2 ^  9 */,
        ATOMIC_INIT(0) /* 2 ^ 10 */,
        ATOMIC_INIT(0) /* 2 ^ 11 */,
        ATOMIC_INIT(0) /* 2 ^ 12 */,
        ATOMIC_INIT(0) /* 2 ^ 13 */,
        ATOMIC_INIT(0) /* 2 ^ 14 */,
        ATOMIC_INIT(0) /* 2 ^ 15 */,
        ATOMIC_INIT(0) /* 2 ^ 16 */,
        ATOMIC_INIT(0) /* 2 ^ 17 */,
        ATOMIC_INIT(0) /* 2 ^ 18 */,
        ATOMIC_INIT(0) /* 2 ^ 19 */,
        ATOMIC_INIT(0) /* 2 ^ 20 */,
};

#define BLOCKS_COUNT ARRAY_SIZE(mem_stats)

static DEFINE_SPINLOCK(mem_stats_lock);
static unsigned long mem_stats_j = 0;
#define MEM_STATS_INTERVAL msecs_to_jiffies(CONFIG_RINA_MEMORY_STATS_INTERVAL)
#define MEM_STATS_BANNER   "MEMSTAT "

static void mem_stats_dump(void)
{
        size_t        s;
        unsigned long flags;
        unsigned long now = jiffies;

        spin_lock_irqsave(&mem_stats_lock, flags);
        if (mem_stats_j && time_before(now,
                                       mem_stats_j + MEM_STATS_INTERVAL)) {
                spin_unlock_irqrestore(&mem_stats_lock, flags);
                return;
        }
        mem_stats_j = now;
        spin_unlock_irqrestore(&mem_stats_lock, flags);

        LOG_INFO(MEM_STATS_BANNER "BEG %u", jiffies_to_msecs(now));
        for (s = 0; s < BLOCKS_COUNT; s++)
                LOG_INFO(MEM_STATS_BANNER "%d %u",
                         (int) s, atomic_read(&mem_stats[s]));
        LOG_INFO(MEM_STATS_BANNER "END");
}

static size_t size2bin(size_t size)
{
        size_t bin = 0;

        if (!size)
                return 0;

        size--;
        while (size) {
                size >>= 1;
                bin++;
        }

        if (bin >= BLOCKS_COUNT)
                bin = BLOCKS_COUNT - 1;

        return bin;
}

static void mem_stats_inc(size_t size)
{ atomic_inc(&mem_stats[size2bin(size)]); }

static void mem_stats_dec(size_t size)
{ atomic_dec(&mem_stats[size2bin(size)]); }
#endif

void rms_dump()
{
#ifdef CONFIG_RINA_MEMORY_STATS
        mem_stats_dump();
#endif
}

static void * generic_alloc(void * (* alloc_func)(size_t size, gfp_t flags),
                            size_t    size,
                            gfp_t     flags)
{
        size_t                   real_size;
        void *                   ptr;
#ifdef CONFIG_RINA_MEMORY_TAMPERING
        struct memblock_header * header;
        struct memblock_footer * footer;
#endif

        if (!size) {
                /* We will consider 0 bytes allocations as meaningless */
                LOG_ERR("Allocating 0 bytes is meaningless");
                return NULL;
        }

        real_size = size;
#ifdef CONFIG_RINA_MEMORY_TAMPERING
        real_size += sizeof(*header) + sizeof(*footer);
#endif

        ASSERT(alloc_func);
        ptr = alloc_func(real_size, flags);
        if (!ptr) {
                LOG_ERR("Cannot allocate %zd bytes", size);
                return NULL;
        }

#ifdef CONFIG_RINA_MEMORY_TAMPERING
#ifdef CONFIG_RINA_MEMORY_TAMPERING_VERBOSE
        LOG_DBG("Tampering block at %pK, size %zd, real-size %zd",
                ptr, size, real_size);
#endif

        header               =
                (struct memblock_header *) ptr;
        mb_header_filler_init(header);
        header->inner_length = size;
        ptr                  =
                (void *)((uint8_t *) header + sizeof(*header));
        footer               =
                (struct memblock_footer *)((uint8_t *) ptr + size);
        mb_footer_filler_init(footer);

        ASSERT((uint8_t *) header <= (uint8_t *) ptr);
        ASSERT((uint8_t *) ptr    <= (uint8_t *) footer);

#ifdef CONFIG_RINA_MEMORY_TAMPERING_VERBOSE
        LOG_DBG("Memblock header at %pK/%zd", header, sizeof(*header));
        LOG_DBG("Memblock footer at %pK/%zd", footer, sizeof(*footer));

        LOG_DBG("Returning tampered memory block %pK/%zd", ptr, real_size);
#endif
#endif

#ifdef CONFIG_RINA_MEMORY_PTRS_DUMP
        LOG_DBG("generic_alloc(%zd) = %pK", size, ptr);
#endif

#ifdef CONFIG_RINA_MEMORY_STATS
        mem_stats_inc(size);
        mem_stats_dump();
#endif

        return ptr;
}

void * rkmalloc(size_t size, gfp_t flags)
{
        void * ptr;

        ptr = generic_alloc(kmalloc, size, flags);
#ifdef CONFIG_RINA_MEMORY_POISONING
        if (ptr)
                poison(ptr, size);
#endif
        return ptr;
}
EXPORT_SYMBOL(rkmalloc);

void * rkzalloc(size_t size, gfp_t flags)
{ 	return generic_alloc(kzalloc, size, flags); }
EXPORT_SYMBOL(rkzalloc);

#ifdef CONFIG_RINA_MEMORY_TAMPERING
static bool tamper_check(void * ptr)
{
        struct memblock_header * header;
        struct memblock_footer * footer;

        ASSERT(ptr);

        header = (struct memblock_header *) ptr;
        footer = (struct memblock_footer *)
                ((uint8_t *) ptr + sizeof(*header) + header->inner_length);

        if (!mb_is_header_filler_ok(header)) {
                LOG_CRIT("Memory block %pK has been corrupted (header)", ptr);
                return false;
        }
        if (!mb_is_footer_filler_ok(footer)) {
                LOG_CRIT("Memory block %pK has been corrupted (footer)", ptr);
                return false;
        }

        return true;
}
#endif

static bool generic_free(void * ptr)
{
#ifdef CONFIG_RINA_MEMORY_TAMPERING
        struct memblock_header * header;
#endif

        ASSERT(ptr);

#ifdef CONFIG_RINA_MEMORY_TAMPERING
        header = inner2outer(ptr);
        if (!tamper_check(header))
                return false;
#ifdef CONFIG_RINA_MEMORY_POISONING
        poison(ptr, header->inner_length);
#endif
        ptr = header;
#endif

#ifdef CONFIG_RINA_MEMORY_STATS
        mem_stats_dec(ksize(ptr));
        mem_stats_dump();
#endif

        kfree(ptr);

#ifdef CONFIG_RINA_MEMORY_PTRS_DUMP
        LOG_DBG("generic_free(%pK)", ptr);
#endif
        return true;
}

static bool __rkfree(void * ptr)
{
        ASSERT(ptr);

        return generic_free(ptr);
}

void rkfree(void * ptr)
{
        if (!__rkfree(ptr))
                BUG();
}
EXPORT_SYMBOL(rkfree);

#ifdef CONFIG_RINA_RMEM_REGRESSION_TESTS
bool regression_tests_rmem(void)
{
        void * tmp;

        LOG_DBG("RMem regression tests");

        LOG_DBG("Regression test #1.1");

        tmp = rkmalloc(100, GFP_KERNEL);
        if (!tmp)
                return false;

        if (!__rkfree(tmp))
                return false;

        return true;
}
#endif
