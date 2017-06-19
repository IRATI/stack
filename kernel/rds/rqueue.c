/*
 * RINA Queues
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

#define RINA_PREFIX "rqueue"

#include "logs.h"
#include "debug.h"
#include "rmem.h"
#include "rqueue.h"

struct rqueue_entry {
        struct list_head next;
        void *           data;
};

struct rqueue {
        struct list_head head;
        size_t           length;
};

struct rqueue * rqueue_create_gfp(gfp_t flags)
{
        struct rqueue * q;

        q = rkmalloc(sizeof(*q), flags);
        if (!q)
                return NULL;

        INIT_LIST_HEAD(&q->head);
        q->length = 0;

        return q;
}

struct rqueue * rqueue_create(void)
{ return rqueue_create_gfp(GFP_KERNEL); }
EXPORT_SYMBOL(rqueue_create);

struct rqueue * rqueue_create_ni(void)
{ return rqueue_create_gfp(GFP_ATOMIC); }
EXPORT_SYMBOL(rqueue_create_ni);

ssize_t rqueue_length(struct rqueue * q)
{
        if (!q)
                return -1;

        return q->length;
}
EXPORT_SYMBOL(rqueue_length);

static struct rqueue_entry * entry_create(gfp_t flags, void * data)
{
        struct rqueue_entry * entry;

        entry = rkmalloc(sizeof(*entry), flags);
        if (!entry)
                return NULL;

        entry->data = data;
        INIT_LIST_HEAD(&entry->next);

        return entry;
}

static int entry_destroy(struct rqueue_entry * entry)
{
        if (!entry)
                return -1;

        rkfree(entry);

        return 0;
}

static int __rqueue_flush(struct rqueue * q,
                          void         (* dtor)(void * data))
{
        struct rqueue_entry * pos, * nxt;

        ASSERT(q);
        ASSERT(dtor);

        list_for_each_entry_safe(pos, nxt, &q->head, next) {
                ASSERT(pos);

                list_del(&pos->next);
                dtor(pos->data);
                entry_destroy(pos);

                q->length--;
        }

        ASSERT(q->length == 0);

        return 0;
}

static int __rqueue_destroy(struct rqueue * q,
                            void         (* dtor)(void * data))
{
        ASSERT(q);

        if (__rqueue_flush(q, dtor)) {
                LOG_ERR("Cannot flush queue %pK", q);
                return -1;
        }

        rkfree(q);

        return 0;
}

int rqueue_destroy(struct rqueue * q,
                   void         (* dtor)(void * data))
{
        if (!q || !dtor) {
                LOG_ERR("Bogus input parameters, can't destroy rqueue %pK", q);
                return -1;
        }

        return __rqueue_destroy(q, dtor);
}
EXPORT_SYMBOL(rqueue_destroy);

static int rqueue_head_push_gfp(gfp_t           flags,
                                struct rqueue * q,
                                void *          data)
{
        struct rqueue_entry * entry;

        if (!q) {
                LOG_ERR("Cannot head-push on a NULL queue");
                return -1;
        }

        entry = entry_create(flags, data);
        if (!entry)
                return -1;

        list_add(&entry->next, &q->head);
        q->length++;

        LOG_DBG("Entry %pK head-pushed into queue %pK (length = %zd)",
                entry, q, q->length);

        return 0;
}

int rqueue_head_push(struct rqueue * q, void * e)
{ return rqueue_head_push_gfp(GFP_KERNEL, q, e); }
EXPORT_SYMBOL(rqueue_head_push);

int rqueue_head_push_ni(struct rqueue * q, void * e)
{ return rqueue_head_push_gfp(GFP_ATOMIC, q, e); }
EXPORT_SYMBOL(rqueue_head_push_ni);

void * rqueue_head_pop(struct rqueue * q)
{
        struct rqueue_entry * entry;
        void *                data;

        if (!q) {
                LOG_ERR("Cannot head-pop from a NULL queue");
                return NULL;
        }

        if (list_empty(&q->head)) {
                LOG_WARN("queue %pK is empty, can't head-pop", q);
                return NULL;
        }
        ASSERT(q->length > 0);

        entry = list_first_entry(&q->head, struct rqueue_entry, next);
        ASSERT(entry);

        data = entry->data;

        list_del(&entry->next);
        q->length--;

        LOG_DBG("Entry %pK head-popped from queue %pK (length = %zd)",
                entry, q, q->length);

        entry_destroy(entry);

        return data;
}
EXPORT_SYMBOL(rqueue_head_pop);

void * rqueue_head_peek(struct rqueue * q)
{
        struct rqueue_entry * entry;
        void *                data;

        if (!q) {
                LOG_ERR("Cannot head-pop from a NULL queue");
                return NULL;
        }

        if (list_empty(&q->head)) {
                LOG_WARN("queue %pK is empty, can't head-pop", q);
                return NULL;
        }
        ASSERT(q->length > 0);

        entry = list_first_entry(&q->head, struct rqueue_entry, next);
        ASSERT(entry);

        data = entry->data;

        LOG_DBG("Entry %pK head-peeked from queue %pK (length = %zd)",
                entry, q, q->length);

        return data;
}
EXPORT_SYMBOL(rqueue_head_peek);

static int rqueue_tail_push_gfp(gfp_t flags, struct rqueue * q, void * data)
{
        struct rqueue_entry * entry;

        if (!q) {
                LOG_ERR("Cannot tail-push on a NULL queue");
                return -1;
        }

        entry = entry_create(flags, data);
        if (!entry)
                return -1;

        list_add_tail(&entry->next, &q->head);
        q->length++;

        LOG_DBG("Entry %pK tail-pushed into queue %pK (length = %zd)",
                entry, q, q->length);

        return 0;
}

int rqueue_tail_push(struct rqueue * q, void * entry)
{ return rqueue_tail_push_gfp(GFP_KERNEL, q, entry); }
EXPORT_SYMBOL(rqueue_tail_push);

int rqueue_tail_push_ni(struct rqueue * q, void * entry)
{ return rqueue_tail_push_gfp(GFP_ATOMIC, q, entry); }
EXPORT_SYMBOL(rqueue_tail_push_ni);

void * rqueue_tail_pop(struct rqueue * q)
{
        struct rqueue_entry * entry;
        void *                data;
        struct list_head *    h = NULL;

        if (!q) {
                LOG_ERR("Cannot tail-pop from a NULL queue");
                return NULL;
        }

        if (list_empty(&q->head)) {
                LOG_WARN("queue %pK is empty, can't tail-pop", q);
                return NULL;
        }
        ASSERT(q->length > 0);

        list_move_tail(&q->head, h);
        q->length--;

        entry = ((struct rqueue_entry *) h);
        ASSERT(entry);

        LOG_DBG("Entry %pK tail-popped from queue %pK (length = %zd)",
                entry, q, q->length);

        data = entry->data;

        entry_destroy(entry);

        return data;
}
EXPORT_SYMBOL(rqueue_tail_pop);

bool rqueue_is_empty(struct rqueue * q)
{
        if (!q) {
                LOG_ERR("Can't chek the emptiness of a NULL queue");
                return false;
        }

        return list_empty(&q->head) ? true : false;
}
EXPORT_SYMBOL(rqueue_is_empty);

#ifdef CONFIG_RINA_RQUEUE_REGRESSION_TESTS
bool regression_tests_rqueue(void)
{ return true; }
#endif
