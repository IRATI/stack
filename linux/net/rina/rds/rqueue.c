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

struct rqueue * rqueue_create(void)
{
        struct rqueue * tmp;

        tmp = rkmalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        INIT_LIST_HEAD(&tmp->head);

        return tmp;
}
EXPORT_SYMBOL(rqueue_create);

int rqueue_destroy(struct rqueue * q,
                   void         (* dtor)(void * e))
{
        struct rqueue_entry * pos, * nxt;

        if (!q)
                return -1;
        if (!dtor)
                return -1;

        list_for_each_entry_safe(pos, nxt, &q->head, next) {
                list_del(&pos->next);
                dtor(pos->data);
                rkfree(pos);
        }
        rkfree(q);

        return 0;
}
EXPORT_SYMBOL(rqueue_destroy);

int rqueue_head_push(struct rqueue * q, void * e)
{
        struct rqueue_entry * tmp;

        if (!q)
                return -1;
        if (!e)
                return -1;

        tmp = rkmalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return -1;

        tmp->data = e;
        INIT_LIST_HEAD(&tmp->next);
        list_add(&q->head, &tmp->next);

        return 0;
}
EXPORT_SYMBOL(rqueue_head_push);

void * rqueue_head_pop(struct rqueue * q)
{
        if (!q)
                return NULL;

        LOG_MISSING;

        return NULL;
}
EXPORT_SYMBOL(rqueue_head_pop);

int rqueue_tail_push(struct rqueue * q, void * e)
{
        struct rqueue_entry * tmp;

        if (!q)
                return -1;
        if (!e)
                return -1;

        tmp = rkmalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return -1;

        tmp->data = e;
        INIT_LIST_HEAD(&tmp->next);
        list_add(&q->head, &tmp->next);

        LOG_MISSING;

        return 0;
}
EXPORT_SYMBOL(rqueue_tail_push);

void * rqueue_tail_pop(struct rqueue * q)
{
        if (!q)
                return NULL;

        LOG_MISSING;

        return NULL;
}
EXPORT_SYMBOL(rqueue_tail_pop);

bool rqueue_is_empty(struct rqueue * q)
{ return true; }
EXPORT_SYMBOL(rqueue_is_empty);
