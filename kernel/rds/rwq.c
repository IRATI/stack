/*
 * RINA Work Queues
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
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

#include <linux/types.h>

#define RINA_PREFIX "rwq"

#include "logs.h"
#include "debug.h"
#include "rmem.h"
#include "rwq.h"

/*
 * RWQ
 */

struct rwq_work_item {
        struct work_struct        work; /* KEEP AT TOP AND DO NOT MOVE ! */
        int                       (* worker)(void * data);
        void *                    data;
        bool                      single;
        struct workqueue_struct * wq;
};

static void rwq_worker(struct work_struct * work)
{
        struct rwq_work_item * item = (struct rwq_work_item *) work;
        int    ret;

        if (!item) {
                LOG_ERR("No item to work on, bailing out");
                return;
        }
        ASSERT(item->worker); /* Ensured by post */

        ret = item->worker(item->data);
        if (ret == -RWQ_WORKERROR )
                LOG_ERR("The worker could not process its data!");


        if (item->single) {
                clear_bit(WORK_STRUCT_PENDING_BIT,
                          work_data_bits((struct work_struct *)item));
                if (item->wq && ret == RWQ_RESCHEDULE)
                        queue_work(item->wq, (struct work_struct *) item);
        } else
                /* We're the owner of the data, let's free it */
                rkfree(item);

        return;
}

#define RWQ_MAX_ACTIVE 1 /* Only temporary */

static struct workqueue_struct * rwq_create_priority(const char * name,
                                                     const int    priority)
{
        struct workqueue_struct * wq;

        if (!name) {
                LOG_ERR("No workqueue name passed, cannot create it");
                return NULL;
        }

#if 1
	wq = create_singlethread_workqueue(name);
#else
        wq = alloc_workqueue(name,
                             WQ_UNBOUND      |
                             WQ_MEM_RECLAIM  |
                             priority        |
                             WQ_CPU_INTENSIVE,
                             RWQ_MAX_ACTIVE);
        wq = create_workqueue(name);
#endif
        if (!wq) {
                LOG_ERR("Cannot create workqueue '%s'", name);
                return NULL;
        }

        LOG_DBG("Workqueue '%s' (%pK) created successfully", name, wq);

        return wq;
}

struct workqueue_struct * rwq_create(const char * name)
{ return rwq_create_priority(name, 0); }
EXPORT_SYMBOL(rwq_create);

struct workqueue_struct * rwq_create_hp(const char * name)
{ return rwq_create_priority(name, WQ_HIGHPRI); }
EXPORT_SYMBOL(rwq_create_hp);

static struct rwq_work_item * rwq_work_create_gfp(gfp_t    flags,
                                                  int   (* work)(void * data),
                                                  void *   data,
                                                  bool     single)
{
        struct rwq_work_item * tmp;

        if (!work) {
                LOG_ERR("No worker passed, cannot create work");
                return NULL;
        }
        /* The data parameter can be empty ... */

        tmp = rkzalloc(sizeof(struct rwq_work_item), flags);
        if (!tmp) {
                LOG_ERR("Cannot create work item");
                return NULL;
        }

        /* Filling the workqueue item */
        INIT_WORK((struct work_struct *) tmp, rwq_worker);
        tmp->worker = work;
        tmp->data   = data;
        tmp->single = single;
        tmp->wq     = NULL;

        return tmp;
}

struct rwq_work_item * rwq_work_create(int   (* work)(void * data),
                                       void *   data)
{ return rwq_work_create_gfp(GFP_KERNEL, work, data, false); }
EXPORT_SYMBOL(rwq_work_create);

struct rwq_work_item * rwq_work_create_ni(int   (* work)(void * data),
                                          void *   data)
{ return rwq_work_create_gfp(GFP_ATOMIC, work, data, false); }
EXPORT_SYMBOL(rwq_work_create_ni);

struct rwq_work_item * rwq_work_create_single(int  (* work)(void * data),
                                              void *   data)
{ return rwq_work_create_gfp(GFP_KERNEL, work, data, true); }
EXPORT_SYMBOL(rwq_work_create_single);

struct rwq_work_item * rwq_work_create_single_ni(int  (* work)(void * data),
                                                 void *   data)
{ return rwq_work_create_gfp(GFP_ATOMIC, work, data, true); }
EXPORT_SYMBOL(rwq_work_create_single_ni);

void rwq_work_destroy(struct rwq_work_item * item)
{ rkfree(item); }
EXPORT_SYMBOL(rwq_work_destroy);

int rwq_work_post(struct workqueue_struct * wq,
                  struct rwq_work_item *    item)
{
        if (!wq) {
                LOG_ERR("No workqueue passed, cannot post work");
                return -1;
        }
        if (!item) {
                LOG_ERR("No item passed, cannot post work");
                return -1;
        }

        item->wq = wq;

        if (!queue_work(wq, (struct work_struct *) item)) {
                /* FIXME: Add workqueue name in the log */
                LOG_ERR("Cannot post work on workqueue %pK", wq);
                return -1;
        }

        LOG_DBG("Work posted on workqueue %pK, please wait ...", wq);

        return 0;
}
EXPORT_SYMBOL(rwq_work_post);

int rwq_flush(struct workqueue_struct * wq)
{
        if (!wq) {
                LOG_ERR("The passed workqueue is NULL, cannot flush");
                return -1;
        }

        LOG_DBG("Flushing workqueue %pK", wq);

        flush_workqueue(wq);

        LOG_DBG("Workqueue %pK flushed successfully", wq);

        return 0;
}
EXPORT_SYMBOL(rwq_flush);

int rwq_destroy(struct workqueue_struct * wq)
{

        if (rwq_flush(wq)) {
                LOG_ERR("The passed wq could not be destroyed");
                return -1;
        }

        destroy_workqueue(wq);

        LOG_DBG("Workqueue %pK destroyed successfully", wq);

        return 0;
}
EXPORT_SYMBOL(rwq_destroy);

#if 0
/*
 * RWQO
 */

struct workqueue_struct * rwqo_create(const char * name)
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
EXPORT_SYMBOL(rwqo_create);

int rwqo_work_post(struct workqueue_struct * wq,
                   int                    (* worker)(void * data),
                   void *                    data)
{
        if (!wq) {
                LOG_ERR("No workqueue passed, cannot post work");
                return -1;
        }
        if (!work) {
                LOG_ERR("No work passed, cannot post it");
                return -1;
        }

        if (!queue_work(wq, (struct work_struct *) item)) {
                /* FIXME: Add workqueue name in the log */
                LOG_ERR("Cannot post work on workqueue %pK", wq);
                return -1;
        }

        LOG_DBG("Work posted on workqueue %pK, please wait ...", wq);

        return 0;
}
EXPORT_SYMBOL(rwqo_work_post);

int rwqo_destroy(struct workqueue_struct * wq)
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
EXPORT_SYMBOL(rwqo_destroy);
#endif
