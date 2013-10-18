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

/* For RWQ */
#include <linux/workqueue.h>

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

static void mb_filler_init(uint8_t * f, size_t length)
{
        size_t i;

        LOG_DBG("Applying filler at %pK, size %zd", f, length);

        for (i = 0; i < length; i++) {
                *f = i % 0xff;
                f++;
        }
}

static int mb_is_filler_ok(const uint8_t * f, size_t length)
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

static void mb_header_filler_init(struct memblock_header * m)
{ mb_filler_init(m->filler, ARRAY_SIZE(m->filler)); }

static int mb_is_header_filler_ok(const struct memblock_header * m)
{ return mb_is_filler_ok(m->filler, ARRAY_SIZE(m->filler)); }

static void mb_footer_filler_init(struct memblock_footer * m)
{ mb_filler_init(m->filler, ARRAY_SIZE(m->filler)); }

static int mb_is_footer_filler_ok(const struct memblock_footer * m)
{ return mb_is_filler_ok(m->filler, ARRAY_SIZE(m->filler)); }
#endif

#ifdef CONFIG_RINA_MEMORY_POISONING
static void mb_poison(void * ptr, size_t size)
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

        LOG_DBG("Poisoning memory %pK-%pK (%zd bytes)", p, p + size - 1, size);

        for (i = 0; i < size; i++) {
                *p = (uint8_t) i;
                p++;
        }
}
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

#ifdef CONFIG_RINA_MEMORY_TAMPERING
        LOG_DBG("The requested block is at %pK, size %zd", ptr, real_size);
        if (!ptr) {
                LOG_ERR("Cannot tamper a NULL memory block");
                return ptr;
        }

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
{
        void * ptr = generic_alloc(kmalloc, size, flags);
#ifdef CONFIG_RINA_MEMORY_POISONING
        mb_poison(ptr, size);
#endif
        return ptr;
}
EXPORT_SYMBOL(rkmalloc);

void * rkzalloc(size_t size, gfp_t flags)
{ return generic_alloc(kzalloc, size, flags); }
EXPORT_SYMBOL(rkzalloc);

void rkfree(void * ptr)
{
#ifdef CONFIG_RINA_MEMORY_TAMPERING
        struct memblock_header * header;
        struct memblock_footer * footer;
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

        if (!mb_is_header_filler_ok(header)) {
                LOG_CRIT("Memory block %pK has been corrupted (header)", ptr);
                BUG();
        }
        if (!mb_is_footer_filler_ok(footer)) {
                LOG_CRIT("Memory block %pK has been corrupted (footer)", ptr);
                BUG();
        }

#ifdef CONFIG_RINA_MEMORY_POISONING
        mb_poison(ptr, header->inner_length);
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

struct rwq_work_item {
        struct work_struct work; /* KEEP AT TOP AND DO NOT MOVE ! */
        int                (* worker)(void * data);
        void *             data;
};

static void rwq_worker(struct work_struct * work)
{
        struct rwq_work_item * item = (struct rwq_work_item *) work;

        if (!item) {
                LOG_ERR("No item to work on, bailing out");
                return;
        }
        ASSERT(item->worker); /* Ensured by post */

        /* We're the owner of the data, let's free it */
        if (item->worker(item->data)) {
                LOG_ERR("The worker could not process its data!");
                return;
        }

        return;
}

struct workqueue_struct * rwq_create(const char * name)
{
        struct workqueue_struct * wq;

        if (!name) {
                LOG_ERR("No workqueue name passed, cannot create it");
                return NULL;
        }

        wq = create_workqueue(name);
        if (!wq) {
                LOG_ERR("Cannot create workqueue '%s'", name);
                return NULL;
        }

        LOG_DBG("Workqueue '%s' (%pK) created successfully", name, wq);

        return wq;
}
EXPORT_SYMBOL(rwq_create);

int rwq_post(struct workqueue_struct * wq,
             int                       (* worker)(void * data),
             void *                    data)
{
        struct rwq_work_item * tmp;

        if (!wq) {
                LOG_ERR("No workqueue passed, cannot post work");
                return -1;
        }
        if (!worker) {
                LOG_ERR("No worker passed, "
                        "cannot post work on workqueue %pK", wq);
                return -1;
        }

        tmp = rkzalloc(sizeof(struct rwq_work_item), GFP_KERNEL);
        if (!tmp) {
                LOG_ERR("Cannot post work on workqueue %pK", wq);
                return -1;
        }

        /* Filling the workqueue item */
        INIT_WORK((struct work_struct *) tmp, rwq_worker);
        tmp->worker = worker;
        tmp->data   = data;

        /* Finally posting the work to do */
        if (queue_work(wq, (struct work_struct *) tmp)) {
                /* FIXME: Add workqueue name in the log */
                LOG_ERR("Cannot post work on workqueue %pK", wq);
                return -1;
        }

        LOG_DBG("Work posted on workqueue %pK, please wait ...", wq);

        return 0;
}
EXPORT_SYMBOL(rwq_post);

int rwq_destroy(struct workqueue_struct * wq)
{
        if (!wq) {
                LOG_ERR("The passed workqueue is NULL, cannot destroy");
                return -1;
        }

        LOG_DBG("Destroying workqueue %pK", wq);

        flush_workqueue(wq);
        destroy_workqueue(wq);

        LOG_DBG("Workqueue %pK destroyed successfully", wq);

        return 0;
}
EXPORT_SYMBOL(rwq_destroy);
