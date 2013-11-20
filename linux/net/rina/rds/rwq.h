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

#ifndef RINA_RWQ_H
#define RINA_RWQ_H

#include <linux/types.h>
#include <linux/workqueue.h>

struct workqueue_struct * rwq_create(const char * name);
int                       rwq_destroy(struct workqueue_struct * rwq);

/*
 * NOTE: The worker is the owner of the data passed (and must dispose it). It
 *       must return 0 if its work completed successfully.
 */
struct rwq_work_item *    rwq_work_create(gfp_t  flags,
                                          int    (* worker)(void * data),
                                          void * data);
/*
 * NOTE: This function will dispose the rwq_work_item on failure. The item
 *       will be disposed automatically upon work completion.
 */
int                       rwq_work_post(struct workqueue_struct * rwq,
                                        struct rwq_work_item *    item);

#endif
