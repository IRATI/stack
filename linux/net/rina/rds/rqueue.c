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

#include <linux/export.h>

#define RINA_PREFIX "rqueue"

#include "logs.h"
#include "debug.h"

struct rqueue {
        int empty;
};

int rqueue_create(void)
{ return -1; }
EXPORT_SYMBOL(rqueue_create);

int rqueue_destroy(struct rqueue * q)
{ return -1; }
EXPORT_SYMBOL(rqueue_destroy);

int rqueue_enqueue(struct rqueue * q, void * e)
{ return -1; }
EXPORT_SYMBOL(rqueue_enqueue);

void * rqueue_dequeue(struct rqueue * q)
{ return NULL; }
EXPORT_SYMBOL(rqueue_dequeue);

bool rqueue_is_empty(struct rqueue * q)
{ return true; }
EXPORT_SYMBOL(rqueue_is_empty);

