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

#include <linux/slab.h>
#include <linux/mm.h>

#include "vmpi-structs.h"


static inline int
vmpi_buffer_init(struct vmpi_buffer *buf, size_t size)
{
        buf->len = 0;
        buf->next = NULL;

        if (size) {
                buf->page = NULL;
                buf->p = kmalloc(size, GFP_ATOMIC);
                if (unlikely(buf->p == NULL))
                        return -ENOMEM;
                buf->size = size;
        } else {
                buf->page = alloc_page(GFP_ATOMIC);
                if (unlikely(buf->page == NULL))
                        return -ENOMEM;
                buf->p = page_address(buf->page);
                buf->size = PAGE_SIZE;
        }

        return 0;
}

static inline void
vmpi_buffer_fini(struct vmpi_buffer *buf)
{
        buf->len = 0;
        if (buf->page) {
                put_page(buf->page);
        } else {
                kfree(buf->p);
        }
}

struct vmpi_buffer *
vmpi_buffer_create(size_t size)
{
        struct vmpi_buffer *buf;
        int r;

        buf = kmalloc(sizeof(struct vmpi_buffer), GFP_ATOMIC);
        if (buf == NULL) {
                return NULL;
        }

        r = vmpi_buffer_init(buf, size);
        if (r) {
                kfree(buf);
                buf = NULL;
        }

        return buf;
}

void
vmpi_buffer_destroy(struct vmpi_buffer *buf)
{
        vmpi_buffer_fini(buf);
        kfree(buf);
}

int
vmpi_ring_init(struct vmpi_ring *ring, unsigned int buf_size)
{
        int ret = -ENOMEM;
        int i;

        init_waitqueue_head(&ring->wqh);
        mutex_init(&ring->lock);

        ring->nu = ring->np = ring->nr = 0;

        ring->buf_size = buf_size;

        ring->bufs = kmalloc(sizeof(struct vmpi_buffer) * VMPI_RING_SIZE, GFP_KERNEL);
        if (ring->bufs == NULL) {
                return ret;
        }

        for (i = 0; i < VMPI_RING_SIZE; i++) {
                ret = vmpi_buffer_init(&ring->bufs[i], ring->buf_size);
                if (ret) {
                        goto err;
                }
        }

        return 0;

 err:
        for (i--; i >= 0; i--) {
                vmpi_buffer_fini(&ring->bufs[i]);
        }
        kfree(ring->bufs);

        return ret;
}

void
vmpi_ring_fini(struct vmpi_ring *ring)
{
        int i;

        for (i = 0; i < VMPI_RING_SIZE; i++) {
                vmpi_buffer_fini(&ring->bufs[i]);
        }

        kfree(ring->bufs);
}

void
vmpi_queue_push_back(struct vmpi_queue *queue, struct vmpi_buffer *buf)
{
        buf->next = NULL;
        if (queue->tail) {
                queue->tail->next = buf;
                queue->tail = buf;
        } else {
                queue->head = queue->tail = buf;
        }

        queue->len++;
}

struct vmpi_buffer *
vmpi_queue_pop_front(struct vmpi_queue *queue)
{
        struct vmpi_buffer *ret = queue->head;

        if (ret) {
                queue->head = ret->next;
                if (queue->head == NULL) {
                        queue->tail = NULL;
                }
                ret->next = NULL;  /* Not necessary. */
                queue->len--;
        }

        return ret;
}

void
vmpi_queue_purge(struct vmpi_queue *queue)
{
        struct vmpi_buffer *qbuf;

        while ((qbuf = vmpi_queue_pop_front(queue))) {
                vmpi_buffer_destroy(qbuf);
        }
}

int
vmpi_queue_init(struct vmpi_queue *queue, unsigned int initial_length,
                unsigned int buf_size)
{
        queue->head = queue->tail = NULL;
        queue->len = 0;
        queue->buf_size = buf_size;

        init_waitqueue_head(&queue->wqh);
        mutex_init(&queue->lock);

        if (initial_length) {
                unsigned int i;

                for (i = 0; i < initial_length; i++) {
                        struct vmpi_buffer *buf;

                        buf = vmpi_buffer_create(queue->buf_size);
                        if (buf == NULL) {
                                vmpi_queue_purge(queue);
                                return -ENOMEM;
                        }
                        vmpi_queue_push_back(queue, buf);
                }
        }

        return 0;
}

void
vmpi_queue_fini(struct vmpi_queue *queue)
{
        vmpi_queue_purge(queue);
}
