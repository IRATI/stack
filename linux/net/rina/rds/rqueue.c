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
};

static struct rqueue * rqueue_create_gfp(gfp_t flags)
{
        struct rqueue * tmp;

        tmp = rkmalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        INIT_LIST_HEAD(&tmp->head);

        return tmp;
}

struct rqueue * rqueue_create(void)
{ return rqueue_create_gfp(GFP_KERNEL); }
EXPORT_SYMBOL(rqueue_create);

struct rqueue * rqueue_create_ni(void)
{ return rqueue_create_gfp(GFP_ATOMIC); }
EXPORT_SYMBOL(rqueue_create_ni);

static int __rqueue_destroy(struct rqueue * q,
                            void         (* dtor)(void * e))
{
        struct rqueue_entry * pos, * nxt;

        list_for_each_entry_safe(pos, nxt, &q->head, next) {
                ASSERT(pos);

                list_del(&pos->next);
                dtor(pos->data);
        }

        rkfree(q);

        return 0;
}

int rqueue_destroy(struct rqueue * q,
                   void         (* dtor)(void * e))
{
        if (!q || !dtor) {
                LOG_ERR("Bogus input parameters, can't destroy rqueue %pK", q);
                return -1;
        }

        return __rqueue_destroy(q, dtor);
}
EXPORT_SYMBOL(rqueue_destroy);

static struct rqueue_entry * entry_create(gfp_t flags, void * e)
{
        struct rqueue_entry * tmp;

        tmp = rkmalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->data = e;
        INIT_LIST_HEAD(&tmp->next);

        return tmp;
}

static int entry_destroy(struct rqueue_entry * e)
{
        if (!e)
                return -1;

        rkfree(e);
        return 0;
}

static int rqueue_head_push_gfp(gfp_t flags, struct rqueue * q, void * e)
{
        struct rqueue_entry * tmp;

        if (!q) {
                LOG_ERR("Cannot head-push on a NULL queue");
                return -1;
        }

        tmp = entry_create(flags, e);
        if (!tmp)
                return -1;

        list_add(&tmp->next, &q->head);

        LOG_DBG("Element %pK head-pushed into queue %pK", e, q);

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
        struct rqueue_entry * tmp;
        void *                ret;

        if (!q) {
                LOG_ERR("Cannot head-pop from a NULL queue");
                return NULL;
        }

        if (list_empty(&q->head)) {
                LOG_ERR("Cannot head-pop from an empty queue");
                return NULL;
        }

        tmp = list_first_entry(&q->head, struct rqueue_entry, next);
        ASSERT(tmp);
        ret = tmp->data;

        list_del(&tmp->next);

        entry_destroy(tmp);

        return ret;
}
EXPORT_SYMBOL(rqueue_head_pop);

static int rqueue_tail_push_gfp(gfp_t flags, struct rqueue * q, void * e)
{
        struct rqueue_entry * tmp;

        if (!q) {
                LOG_ERR("Cannot tail-push on a NULL queue");
                return -1;
        }

        tmp = entry_create(flags, e);
        if (!tmp)
                return -1;

        list_add_tail(&tmp->next, &q->head);

        LOG_DBG("Element %pK tail-pushed into queue %pK", e, q);

        return 0;
}

int rqueue_tail_push(struct rqueue * q, void * e)
{ return rqueue_tail_push_gfp(GFP_KERNEL, q, e); }
EXPORT_SYMBOL(rqueue_tail_push);

int rqueue_tail_push_ni(struct rqueue * q, void * e)
{ return rqueue_tail_push_gfp(GFP_ATOMIC, q, e); }
EXPORT_SYMBOL(rqueue_tail_push_ni);

void * rqueue_tail_pop(struct rqueue * q)
{
        struct rqueue_entry * tmp;
        void *                ret;
        struct list_head *    h = NULL;

        if (!q) {
                LOG_ERR("Cannot tail-pop from a NULL queue");
                return NULL;
        }

        if (list_empty(&q->head)) {
                LOG_ERR("Cannot tail-pop from an empty queue");
                return NULL;
        }

        list_move_tail(&q->head, h);

        tmp = ((struct rqueue_entry *) h);
        ASSERT(tmp);

        ret = tmp->data;

        entry_destroy(tmp);

        return ret;
}
EXPORT_SYMBOL(rqueue_tail_pop);

bool rqueue_is_empty(struct rqueue * q)
{
        if (!1) {
                LOG_ERR("Can't chek the emptiness of a NULL queue");
                return false;
        }

        return list_empty(&q->head) ? true : false;
}
EXPORT_SYMBOL(rqueue_is_empty);
