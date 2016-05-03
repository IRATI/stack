/*
 * RINA RING QUEUES
 *
 *    Miquel Tarzan <miquel.tarzan@i2cat.net>
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

#ifndef RINA_RINGQ_H
#define RINA_RINGQ_H

struct ringq;

struct ringq * ringq_create(unsigned int size);
struct ringq * ringq_create_ni(unsigned int size);

/* NOTE: dtor has the ownership of freeing the passed element */
int            ringq_destroy(struct ringq * q,
                               void      (* dtor)(void * e));

/*
 * NOTE: We allow pushing NULL entries in the queue but the dtor passed to
 *       ringq_destroy() has to handle them opportunely
 */
int            ringq_push(struct ringq * q, void * e);
int            ringq_push_ni(struct ringq * q, void * e);
int            ringq_head_push_ni(struct ringq * q, void * e);
void *         ringq_pop(struct ringq * q);
bool           ringq_is_empty(struct ringq * q);
ssize_t        ringq_length(struct ringq * q);
ssize_t        ringq_occupation(struct ringq * q);
struct ringq * ringq_order_create(unsigned int length,
				int (* comp)(void*, void*));
int 		ringq_order_push(struct ringq * q, void * e);
struct ringq * ringq_order_entry_create(unsigned int length,
					unsigned int size,
					int (* comp)(void*, void*),
					void (* add)(void*, void*));
int ringq_push_entry(struct ringq * q, void * entry);
void * ringq_pop_entry(struct ringq * q);

#endif
