/* A VMPI implemented using the vmpi-impl hypervisor interface
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

#include <linux/compat.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/socket.h>

#include "vmpi-host-impl.h"
#include "vmpi-host-test.h"


struct vmpi_info {
    struct vmpi_impl_info *vi;

    struct vmpi_ring write;
    struct vmpi_ring read;
};

struct vmpi_ring *vmpi_get_write_ring(struct vmpi_info *mpi)
{
    return &mpi->write;
}

struct vmpi_ring *vmpi_get_read_ring(struct vmpi_info *mpi)
{
    return &mpi->read;
}

struct vmpi_info *vmpi_init(struct vmpi_impl_info *vi, int *err)
{
    struct vmpi_info *mpi;

    mpi = kmalloc(sizeof(struct vmpi_info), GFP_KERNEL);
    if (mpi == NULL) {
        *err = -ENOMEM;
        return NULL;
    }

    mpi->vi = vi;

    *err = vmpi_ring_init(&mpi->write);
    if (*err) {
        goto init_write;
    }

    *err = vmpi_ring_init(&mpi->read);
    if (*err) {
        goto init_read;
    }

    return mpi;

init_read:
    vmpi_ring_fini(&mpi->write);
init_write:
    kfree(mpi);

    return NULL;    
}

void vmpi_fini(struct vmpi_info *mpi)
{
    mpi->vi = NULL;
    vmpi_ring_fini(&mpi->write);
    vmpi_ring_fini(&mpi->read);
    kfree(mpi);
}

ssize_t vmpi_write(struct vmpi_info *mpi, const struct iovec *iv,
                   unsigned long iovcnt)
{
    //struct vhost_mpi_virtqueue *nvq = &mpi->vqs[VHOST_NET_VQ_RX];
    struct vmpi_ring *ring = &mpi->write;
    size_t len = iov_length(iv, iovcnt);
    DECLARE_WAITQUEUE(wait, current);
    ssize_t ret = 0;

    if (!mpi)
        return -EBADFD;

    //IFV(printk("vmpi_write %ld %ld\n", iovcnt, len));
    /* XXX file->f_flags & O_NONBLOCK */

    add_wait_queue(&ring->wqh, &wait);
    while (len) {
        struct vmpi_buffer *buf;
        size_t copylen;

        current->state = TASK_INTERRUPTIBLE;

        mutex_lock(&ring->lock);
        if (vmpi_ring_unused(ring) == 0) {
            mutex_unlock(&ring->lock);
            if (signal_pending(current)) {
                ret = -ERESTARTSYS;
                break;
            }

            /* No space to write to, let's sleep */
            schedule();
            continue;
        }

        buf = &ring->bufs[ring->nu];
        copylen = len;
        if (copylen > buf->size) {
            copylen = buf->size;
        }
        ret = copylen;
        if (memcpy_fromiovecend(buf->p, iv, 0, copylen))
            ret = -EFAULT;
        buf->len = copylen;
        VMPI_RING_INC(ring->nu);
        mutex_unlock(&ring->lock);

        vmpi_impl_write_buf(mpi->vi, buf);

        break;
    }

    current->state = TASK_RUNNING;
    remove_wait_queue(&ring->wqh, &wait);

    return ret;
}

ssize_t vmpi_read(struct vmpi_info *mpi, const struct iovec *iv,
                         unsigned long iovcnt)
{
    //struct vhost_mpi_virtqueue *nvq = &mpi->vqs[VHOST_NET_VQ_TX];
    struct vmpi_ring *ring = &mpi->read;
    ssize_t len = iov_length(iv, iovcnt);
    DECLARE_WAITQUEUE(wait, current);
    ssize_t ret = 0;

    if (!mpi) {
        return -EBADFD;
    }

    if (len < 0) {
        return -EINVAL;
    }

    add_wait_queue(&ring->wqh, &wait);
    while (len) {
        struct vmpi_buffer *buf;
        size_t copylen;

        current->state = TASK_INTERRUPTIBLE;

        mutex_lock(&ring->lock);
        if (vmpi_ring_ready(ring) == 0) {
            mutex_unlock(&ring->lock);
            if (signal_pending(current)) {
                ret = -ERESTARTSYS;
                break;
            }

            /* Nothing to read, let's sleep */
            schedule();
            continue;
        }

        buf = &ring->bufs[ring->nr];
        copylen = buf->len;
        if (len < copylen) {
            copylen = len;
        }
        ret = copylen;
        if (memcpy_toiovecend(iv, buf->p, 0, copylen))
            ret = -EFAULT;
        buf->len = 0;
        VMPI_RING_INC(ring->nr);
        VMPI_RING_INC(ring->nu);
        mutex_unlock(&ring->lock);

        vmpi_impl_give_rx_buf(mpi->vi, buf);

        break;
    }

    current->state = TASK_RUNNING;
    remove_wait_queue(&ring->wqh, &wait);

    return ret;
}

#ifdef VMPI_HOST_TEST
/* To use only in vmpi-host-test. */
struct vmpi_info *vmpi_info_from_file_private_data(void *opaque)
{   
    vmpi_impl_info_t *vi = opaque;

    return vmpi_info_from_vmpi_impl_info(vi);
}
#endif  /* VMPI_HOST_TEST */
