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

#include <linux/slab.h>

#include "vmpi-ring.h"


int vmpi_buffer_alloc(struct vmpi_buffer *buf, size_t size)
{
    buf->size = size;
    buf->len = 0;

    buf->p = kmalloc(size, GFP_KERNEL);

    if (buf->p == NULL)
        return -ENOMEM;

    return 0;
}

void vmpi_buffer_free(struct vmpi_buffer *buf)
{
    buf->size = buf->len = 0;
    kfree(buf->p);
}

int vmpi_ring_init(struct vmpi_ring *ring)
{
    int ret = -ENOMEM;
    int i;

    init_waitqueue_head(&ring->wqh);
    mutex_init(&ring->lock);

    ring->nu = ring->np = ring->nr = 0;

    ring->bufs = kmalloc(sizeof(struct vmpi_buffer) * RING_SIZE, GFP_KERNEL);
    if (ring->bufs == NULL) {
        return ret;
    }

    for (i = 0; i < RING_SIZE; i++) {
        ret = vmpi_buffer_alloc(&ring->bufs[i], BUF_SIZE);
        if (ret) {
            goto err;
        }
    }

    return 0;

err:
    for (i--; i >= 0; i--) {
        vmpi_buffer_free(&ring->bufs[i]);
    }
    kfree(ring->bufs);

    return ret;
}

void vmpi_ring_fini(struct vmpi_ring *ring)
{
    int i;

    for (i = 0; i < RING_SIZE; i++) {
        vmpi_buffer_free(&ring->bufs[i]);
    }

    kfree(ring->bufs);
}
