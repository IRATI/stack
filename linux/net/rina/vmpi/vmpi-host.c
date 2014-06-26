/*
 * An hypervisor-side VMPI implemented using the vmpi-impl hypervisor interface
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

#include <linux/compat.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/socket.h>
#include <linux/list.h>
#include <linux/wait.h>

#include "vmpi.h"
#include "vmpi-stats.h"
#include "vmpi-iovec.h"
#include "vmpi-host-impl.h"
#include "vmpi-ops.h"
#include "vmpi-test.h"
#include "vmpi-provider.h"


unsigned int vmpi_max_channels = VMPI_MAX_CHANNELS_DEFAULT;
module_param(vmpi_max_channels, uint, 0444);

struct vmpi_info {
        struct vmpi_impl_info *vi;

        struct vmpi_ring write;
        struct vmpi_queue *read;

        struct vmpi_stats stats;
        unsigned int id;
};

int
vmpi_register_read_callback(struct vmpi_info *mpi, vmpi_read_cb_t cb,
                            void *opaque)
{
        return vmpi_impl_register_read_callback(mpi->vi, cb, opaque);
}

struct vmpi_ring *
vmpi_get_write_ring(struct vmpi_info *mpi)
{
        return &mpi->write;
}

struct vmpi_queue *
vmpi_get_read_queue(struct vmpi_info *mpi)
{
        return mpi->read;
}

struct vmpi_stats *
vmpi_get_stats(struct vmpi_info *mpi)
{
        return &mpi->stats;
}

static unsigned int vmpi_id_counter = 0;

/*
 * This is a "template" to be included after the definition of struct
 * vmpi_info.
 */
#include "vmpi-instances.h"

struct vmpi_info *
vmpi_init(struct vmpi_impl_info *vi, int *err, bool deferred_test_init)
{
        struct vmpi_info *mpi;
        struct vmpi_ops ops;
        int i;

        mpi = kmalloc(sizeof(struct vmpi_info), GFP_KERNEL);
        if (mpi == NULL) {
                *err = -ENOMEM;
                return NULL;
        }
        memset(mpi, 0, sizeof(*mpi));
        vmpi_stats_init(&mpi->stats);

        mpi->vi = vi;

        *err = vmpi_ring_init(&mpi->write, VMPI_BUF_SIZE);
        if (*err) {
                goto init_write;
        }

        mpi->read = kmalloc(sizeof(mpi->read[0]) * vmpi_max_channels,
                            GFP_KERNEL);
        if (mpi->read == NULL) {
                *err = -ENOMEM;
                goto alloc_read_queues;
        }

        for (i = 0; i < vmpi_max_channels; i++) {
                *err = vmpi_queue_init(&mpi->read[i], 0, VMPI_BUF_SIZE);
                if (*err) {
                        goto init_read;
                }
        }

#ifdef VMPI_TEST
        *err = vmpi_test_init(mpi, deferred_test_init);
        if (*err) {
                printk("vmpi_test_init() failed\n");
                goto test_ini;
        }
#endif  /* VMPI_TEST */

        mpi->id = vmpi_id_counter++;

        ops.priv = mpi;
        ops.write = vmpi_ops_write;
        ops.register_read_callback = vmpi_ops_register_read_callback;
        vmpi_provider_register(VMPI_PROVIDER_HOST, mpi->id, &ops);

        return mpi;

#ifdef VMPI_TEST
 test_ini:
#endif  /* VMPI_TEST */
 init_read:
        for (--i; i >= 0; i--) {
                vmpi_queue_fini(&mpi->read[i]);
        }
        kfree(mpi->read);
 alloc_read_queues:
        vmpi_ring_fini(&mpi->write);
 init_write:
        vmpi_stats_fini(&mpi->stats);
        kfree(mpi);

        return NULL;
}

void
vmpi_fini(struct vmpi_info *mpi, bool deferred_test_fini)
{
        unsigned int i;

        vmpi_provider_unregister(VMPI_PROVIDER_HOST, mpi->id);

#ifdef VMPI_TEST
        vmpi_test_fini(mpi, deferred_test_fini);
#endif  /* VMPI_TEST */

        mpi->vi = NULL;
        vmpi_ring_fini(&mpi->write);
        for (i = 0; i < vmpi_max_channels; i++) {
                vmpi_queue_fini(&mpi->read[i]);
        }
        kfree(mpi->read);
        vmpi_stats_fini(&mpi->stats);
        kfree(mpi);
}

ssize_t
vmpi_write_common(struct vmpi_info *mpi, unsigned int channel,
                  const struct iovec *iv, unsigned long iovcnt, bool user)
{
        //struct vhost_mpi_virtqueue *nvq = &mpi->vqs[VHOST_MPI_VQ_RX];
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
                        if (memcpy_fromiovecend(vmpi_buffer_data(buf),
                                                iv, 0, copylen))
                                ret = -EFAULT;
                } else {
                        iovec_to_buf(iv, iovcnt, vmpi_buffer_data(buf),
                                     copylen);
                }
                buf->len = sizeof(struct vmpi_hdr) + copylen;
                VMPI_RING_INC(ring->nu);
                mpi->stats.txreq++;
                mutex_unlock(&ring->lock);

                vmpi_impl_txkick(mpi->vi);

                break;
        }

        current->state = TASK_RUNNING;
        remove_wait_queue(&ring->wqh, &wait);

        return ret;
}

ssize_t
vmpi_read(struct vmpi_info *mpi, unsigned int channel,
          const struct iovec *iv, unsigned long iovcnt)
{
        //struct vhost_mpi_virtqueue *nvq = &mpi->vqs[VHOST_MPI_VQ_TX];
        struct vmpi_queue *queue;
        ssize_t len = iov_length(iv, iovcnt);
        DECLARE_WAITQUEUE(wait, current);
        ssize_t ret = 0;

        if (unlikely(!mpi)) {
                return -EBADFD;
        }

        if (unlikely(channel >= vmpi_max_channels || len < 0)) {
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

                buf = vmpi_queue_pop_front(queue);
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

