/*
 * Data structures for VMPI
 *
 *    Vincenzo Maffione <v.maffione@nextworks.it>
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

#ifndef __VMPI_STRUCTS_H__
#define __VMPI_STRUCTS_H__

#include <linux/types.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include "vmpi-ops.h"


struct vmpi_hdr {
        unsigned int channel;
};

struct vmpi_buffer {
        void *p;
        size_t len;     /* Includes the vmpi header. */
        size_t size;    /* Includes the vmpi header. */
        struct vmpi_buffer *next;
        struct page *page;
};

/* If size == 0 this allocates a page. */
struct vmpi_buffer *vmpi_buffer_create(size_t size);
void vmpi_buffer_destroy(struct vmpi_buffer *buf);

#define vmpi_buffer_hdr(b) ((struct vmpi_hdr *)b->p)
#define vmpi_buffer_data(b) (b->p + sizeof(struct vmpi_hdr))

#define vmpi_buffer_swap(b1, b2)    do {        \
                struct vmpi_buffer tmp = *b1;   \
                *b1 = *b2;                      \
                *b2 = tmp;                      \
        } while (0)


/* ######################## VMPI-RING ############################## */

#define VMPI_RING_SIZE   256

struct vmpi_ring {
        unsigned int nu;    /* Next unused. */
        unsigned int np;    /* Next pending. */
        unsigned int nr;    /* Next ready. */
        unsigned int buf_size;
        struct vmpi_buffer *bufs;
#ifdef VMPI_TX_MUTEX
        wait_queue_head_t wqh;
        struct mutex lock;
#else
        spinlock_t   lock;
#endif
};

static inline unsigned int
vmpi_ring_unused(struct vmpi_ring *ring)
{
        int space = (int)ring->nr - (int)ring->nu - 1;

        if (space < 0) {
                space += VMPI_RING_SIZE;
        }

        return space;
}

static inline unsigned int
vmpi_ring_ready(struct vmpi_ring *ring)
{
        int space = (int)ring->np - (int)ring->nr;

        if (space < 0) {
                space += VMPI_RING_SIZE;
        }

        return space;
}

static inline unsigned int
vmpi_ring_pending(struct vmpi_ring *ring)
{
        int space = (int)ring->nu - (int)ring->np;

        if (space < 0) {
                space += VMPI_RING_SIZE;
        }

        return space;
}

#define VMPI_RING_INC(x)   do {                 \
                if (++(x) == VMPI_RING_SIZE)    \
                        x = 0;                  \
        } while (0)

int vmpi_ring_init(struct vmpi_ring *ring, unsigned int buf_size);

void vmpi_ring_fini(struct vmpi_ring *ring);


/* ############################# VMPI-QUEUE ########################## */

struct vmpi_queue {
        struct vmpi_buffer *head;
        struct vmpi_buffer *tail;
        unsigned int len;
        unsigned int buf_size;
        wait_queue_head_t wqh;
        struct mutex lock;
};

static inline unsigned int vmpi_queue_len(struct vmpi_queue *queue)
{
        return queue->len;
}

static inline unsigned int vmpi_queue_empty(struct vmpi_queue *queue)
{
        return queue->len == 0;
}

int vmpi_queue_init(struct vmpi_queue *queue, unsigned int initial_length,
                    unsigned int buf_size);
void vmpi_queue_fini(struct vmpi_queue *queue);
void vmpi_queue_purge(struct vmpi_queue *queue);
void vmpi_queue_push_back(struct vmpi_queue *queue, struct vmpi_buffer *buf);
struct vmpi_buffer *vmpi_queue_pop_front(struct vmpi_queue *queue);

#endif  /*  __VMPI_STRUCTS_H__ */
