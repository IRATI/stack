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

#include "vmpi.h"
#include "vmpi-iovec.h"
#include "vmpi-host-impl.h"
#include "vmpi-host-test.h"


struct vmpi_info {
    struct vmpi_impl_info *vi;

    struct vmpi_ring write;
    struct vmpi_queue read[VMPI_MAX_CHANNELS];
};

int vmpi_register_read_callback(struct vmpi_info *mpi, vmpi_read_cb_t cb,
                                void *opaque)
{
    return vmpi_impl_register_read_callback(mpi->vi, cb, opaque);
}

struct vmpi_ring *vmpi_get_write_ring(struct vmpi_info *mpi)
{
    return &mpi->write;
}

struct vmpi_queue *vmpi_get_read_queue(struct vmpi_info *mpi)
{
    return mpi->read;
}

#define SHIM_HV
#ifdef SHIM_HV
int shim_hv_init(vmpi_info_t *mpi);
void shim_hv_fini(void);
#endif  /* SHIM_HV */

struct vmpi_info *vmpi_init(struct vmpi_impl_info *vi, int *err)
{
    struct vmpi_info *mpi;
    int i;

    mpi = kmalloc(sizeof(struct vmpi_info), GFP_KERNEL);
    if (mpi == NULL) {
        *err = -ENOMEM;
        return NULL;
    }

    mpi->vi = vi;

    *err = vmpi_ring_init(&mpi->write, VMPI_BUF_SIZE);
    if (*err) {
        goto init_write;
    }

    for (i = 0; i < VMPI_MAX_CHANNELS; i++) {
        *err = vmpi_queue_init(&mpi->read[i], 0, VMPI_BUF_SIZE);
        if (*err) {
            goto init_read;
        }
    }

#ifdef SHIM_HV
    *err = shim_hv_init(mpi);
    if (*err) {
        goto init_read;
    }
#endif  /* SHIM_HV */

    return mpi;

init_read:
    for (--i; i >= 0; i--) {
        vmpi_queue_fini(&mpi->read[i]);
    }
    vmpi_ring_fini(&mpi->write);
init_write:
    kfree(mpi);

    return NULL;    
}

void vmpi_fini(struct vmpi_info *mpi)
{
    unsigned int i;

#ifdef SHIM_HV
    shim_hv_fini();
#endif  /* SHIM_HV */
    mpi->vi = NULL;
    vmpi_ring_fini(&mpi->write);
    for (i = 0; i < VMPI_MAX_CHANNELS; i++) {
        vmpi_queue_fini(&mpi->read[i]);
    }
    kfree(mpi);
}

ssize_t vmpi_write_common(struct vmpi_info *mpi, unsigned int channel,
                          const struct iovec *iv, unsigned long iovcnt,
                          int user)
{
    //struct vhost_mpi_virtqueue *nvq = &mpi->vqs[VHOST_NET_VQ_RX];
    struct vmpi_ring *ring = &mpi->write;
    size_t len = iov_length(iv, iovcnt);
    DECLARE_WAITQUEUE(wait, current);
    size_t buf_data_size = ring->buf_size - sizeof(struct vmpi_hdr);
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
        if (copylen > buf_data_size) {
            copylen = buf_data_size;
        }
        ret = copylen;
        vmpi_buffer_hdr(buf)->channel = channel;
        if (user) {
            if (memcpy_fromiovecend(vmpi_buffer_data(buf), iv, 0, copylen))
                ret = -EFAULT;
        } else {
            iovec_to_buf(iv, iovcnt, vmpi_buffer_data(buf), copylen);
        }
        buf->len = sizeof(struct vmpi_hdr) + copylen;
        VMPI_RING_INC(ring->nu);
        mutex_unlock(&ring->lock);

        vmpi_impl_write_buf(mpi->vi, buf);

        break;
    }

    current->state = TASK_RUNNING;
    remove_wait_queue(&ring->wqh, &wait);

    return ret;
}

ssize_t vmpi_read(struct vmpi_info *mpi, unsigned int channel,
                  const struct iovec *iv, unsigned long iovcnt)
{
    //struct vhost_mpi_virtqueue *nvq = &mpi->vqs[VHOST_NET_VQ_TX];
    struct vmpi_queue *queue;
    ssize_t len = iov_length(iv, iovcnt);
    DECLARE_WAITQUEUE(wait, current);
    ssize_t ret = 0;

    if (unlikely(!mpi)) {
        return -EBADFD;
    }

    if (unlikely(channel >= VMPI_MAX_CHANNELS || len < 0)) {
        return -EINVAL;
    }

    queue = &mpi->read[channel];

    add_wait_queue(&queue->wqh, &wait);
    while (len) {
        struct vmpi_buffer *buf;
        size_t copylen;

        current->state = TASK_INTERRUPTIBLE;

        mutex_lock(&queue->lock);
        if (vmpi_queue_len(queue) == 0) {
            mutex_unlock(&queue->lock);
            if (signal_pending(current)) {
                ret = -ERESTARTSYS;
                break;
            }

            /* Nothing to read, let's sleep */
            schedule();
            continue;
        }

        buf = vmpi_queue_pop(queue);
        mutex_unlock(&queue->lock);

        copylen = buf->len - sizeof(struct vmpi_hdr);
        if (len < copylen) {
            copylen = len;
        }
        ret = copylen;
        if (memcpy_toiovecend(iv, vmpi_buffer_data(buf), 0, copylen))
            ret = -EFAULT;
        buf->len = 0;

        vmpi_buffer_destroy(buf);

        break;
    }

    current->state = TASK_RUNNING;
    remove_wait_queue(&queue->wqh, &wait);

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
