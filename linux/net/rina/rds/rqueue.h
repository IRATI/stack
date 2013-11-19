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

#ifndef RINA_RQUEUE_H
#define RINA_RQUEUE_H

struct rqueue;

struct rqueue * rqueue_create(void);
int             rqueue_destroy(struct rqueue * q,
                               void         (* dtor)(void * e));

int             rqueue_enqueue(struct rqueue * q, void * e);
void *          rqueue_dequeue(struct rqueue * q);
bool            rqueue_is_empty(struct rqueue * q);

#endif
