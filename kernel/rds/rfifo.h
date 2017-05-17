/*
 * RINA FIFOs
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

#ifndef RINA_RFIFO_H
#define RINA_RFIFO_H

struct rfifo;

extern struct rfifo * rfifo_create(void);
extern struct rfifo * rfifo_create_ni(void);

/* NOTE: dtor has the ownership of freeing the passed element */
extern int            rfifo_destroy(struct rfifo * f,
                             	    void        (* dtor)(void * e));

/*
 * NOTE: We allow pushing NULL entries in the fifo but the dtor passed to
 *       rfifo_destroy() has to handle them opportunely
 */
int            rfifo_push(struct rfifo * f, void * e);
int            rfifo_push_ni(struct rfifo * f, void * e);
int            rfifo_head_push_ni(struct rfifo * f, void * e);
void *         rfifo_pop(struct rfifo * f);
void *         rfifo_peek(struct rfifo * f);
bool           rfifo_is_empty(struct rfifo * f);
ssize_t        rfifo_length(struct rfifo * f);

#endif
