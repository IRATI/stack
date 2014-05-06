/* A ring of buffers for VMPI
 *
 * Copyright 2014 Vincenzo Maffione <v.maffione@nextworks.it> Nextworks
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __VMPI_RING_H__
#define __VMPI_RING_H__

#include <linux/types.h>
#include <linux/slab.h>
#include <linux/mutex.h>


struct vmpi_buffer {
    void *p;
    size_t len;
    size_t size;
};

int vmpi_buffer_alloc(struct vmpi_buffer *buf, size_t size);
void vmpi_buffer_free(struct vmpi_buffer *buf);

#define RING_SIZE   256
#define BUF_SIZE    2048

struct vmpi_ring {
    unsigned int nu;    /* Next unused. */
    unsigned int np;    /* Next pending. */
    unsigned int nr;    /* Next ready. */
    struct vmpi_buffer *bufs;
    wait_queue_head_t wqh;
    struct mutex lock;
};

static inline unsigned int
vmpi_ring_unused(struct vmpi_ring *ring)
{
    int space = (int)ring->nr - (int)ring->nu - 1;

    if (space < 0) {
        space += RING_SIZE;
    }

    return space;
}

static inline unsigned int
vmpi_ring_ready(struct vmpi_ring *ring)
{
    int space = (int)ring->np - (int)ring->nr;

    if (space < 0) {
        space += RING_SIZE;
    }

    return space;
}

static inline unsigned int
vmpi_ring_pending(struct vmpi_ring *ring)
{
    int space = (int)ring->nu - (int)ring->np;

    if (space < 0) {
        space += RING_SIZE;
    }

    return space;
}

#define VMPI_RING_INC(x)   do { \
                            if (++(x) == RING_SIZE) \
                                x = 0; \
                        } while (0)

int vmpi_ring_init(struct vmpi_ring *ring);

void vmpi_ring_fini(struct vmpi_ring *ring);

#endif  /*  __VMPI_RING_H__ */
