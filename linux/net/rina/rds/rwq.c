/*
 * RINA Work Queues
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

#define RINA_PREFIX "rwq"

#include "logs.h"
#include "debug.h"
#include "rmem.h"

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
        }

        rkfree(item);

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

struct rwq_work_item * rwq_work_create(gfp_t     flags,
                                       int    (* worker)(void * data),
                                       void *    data)
{
        struct rwq_work_item * tmp;

        if (!worker) {
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
        tmp->worker = worker;
        tmp->data   = data;

        return tmp;
}
EXPORT_SYMBOL(rwq_work_create);

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

        if (!queue_work(wq, (struct work_struct *) item)) {
                /* FIXME: Add workqueue name in the log */
                LOG_ERR("Cannot post work on workqueue %pK", wq);
                return -1;
        }

        LOG_DBG("Work posted on workqueue %pK, please wait ...", wq);

        return 0;
}
EXPORT_SYMBOL(rwq_work_post);

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
