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

#ifndef RINA_RWQ_H
#define RINA_RWQ_H

#include <linux/workqueue.h>

/*
 * RWQ allows to post multiple times the same work-item (worker and data),
 * or create singe work_items that can be posted only once
 */

#define RWQ_WORKERROR    1
#define RWQ_NORESCHEDULE 0
#define RWQ_RESCHEDULE   1


struct workqueue_struct * rwq_create(const char * name);
struct workqueue_struct * rwq_create_hp(const char * name);
int                       rwq_flush(struct workqueue_struct * q);
int                       rwq_destroy(struct workqueue_struct * q);

/*
 * NOTE: The worker is the owner of the data passed (and must dispose it). It
 *       must return 0 if its work completed successfully.
 */
struct rwq_work_item *    rwq_work_create(int (* worker)(void * data),
                                          void * data);
struct rwq_work_item *    rwq_work_create_ni(int (* worker)(void * data),
                                             void * data);
struct rwq_work_item *    rwq_work_create_single(int  (* worker)(void * data),
                                                 void * data);
struct rwq_work_item *    rwq_work_create_single_ni(int  (* worker)(void * data),
                                                    void * data);
void                      rwq_work_destroy(struct rwq_work_item * item);

/*
 * NOTE: This function will dispose the rwq_work_item on failure. The item
 *       will be disposed automatically upon work completion.
 */
int                       rwq_work_post(struct workqueue_struct * q,
                                        struct rwq_work_item *    item);

#endif
