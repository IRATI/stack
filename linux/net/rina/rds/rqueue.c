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

#define RINA_PREFIX "rqueue"

#include "logs.h"
#include "debug.h"
#include "rmem.h"

struct rqueue_entry {
        struct list_head next;
        void *           data;
};

struct rqueue {
        struct list_head head;
};

struct rqueue * rqueue_create_gfp(gfp_t flags);
{
        struct rqueue * tmp;

        tmp = rkmalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        INIT_LIST_HEAD(&tmp->head);

        return tmp;
}
EXPORT_SYMBOL(rqueue_create_gfp);

struct rqueue * rqueue_create(void)
{ return rqueue_create_gfp(GFP_KERNEL); }
EXPORT_SYMBOL(rqueue_create);

int rqueue_destroy(struct rqueue * q,
                   void         (* dtor)(void * e))
{
        struct rqueue_entry * pos, * nxt;

        if (!q) {
                LOG_ERR("Cannot destroy a NULL queue");
                return -1;
        }
        if (!dtor) {
                LOG_ERR("Cannot destroy with a NULL dtor");
                return -1;
        }

        list_for_each_entry_safe(pos, nxt, &q->head, next) {
                list_del(&pos->next);
                dtor(pos->data);
        }

        rkfree(q);

        return 0;
}
EXPORT_SYMBOL(rqueue_destroy);

static struct rqueue_entry * entry_create(void * e)
{
        struct rqueue_entry * tmp;

        tmp = rkmalloc(sizeof(*tmp), GFP_KERNEL);
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

int rqueue_head_push(struct rqueue * q, void * e)
{
        struct rqueue_entry * tmp;

        if (!q) {
                LOG_ERR("Cannot head-push on a NULL queue");
                return -1;
        }

        tmp = entry_create(e);
        if (!tmp)
                return -1;

        list_add(&tmp->next, &q->head);

        LOG_DBG("Element %pK head-pushed into queue %pK", e, q);

        return 0;
}
EXPORT_SYMBOL(rqueue_head_push);

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
        list_del(&q->head);

        ret = tmp->data;
        entry_destroy(tmp);

        return ret;
}
EXPORT_SYMBOL(rqueue_head_pop);

int rqueue_tail_push(struct rqueue * q, void * e)
{
        struct rqueue_entry * tmp;

        if (!q) {
                LOG_ERR("Cannot tail-push on a NULL queue");
                return -1;
        }

        tmp = entry_create(e);
        if (!tmp)
                return -1;

        list_add_tail(&tmp->next, &q->head);

        LOG_DBG("Element %pK tail-pushed into queue %pK", e, q);

        return 0;
}
EXPORT_SYMBOL(rqueue_tail_push);

void * rqueue_tail_pop(struct rqueue * q)
{
        struct rqueue_entry * tmp;
        struct list_head *    h;
        void *                ret;

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
        ret = tmp->data;

        entry_destroy(tmp);

        return ret;
}
EXPORT_SYMBOL(rqueue_tail_pop);

bool rqueue_is_empty(struct rqueue * q)
{ return list_empty(&q->head); }
EXPORT_SYMBOL(rqueue_is_empty);
