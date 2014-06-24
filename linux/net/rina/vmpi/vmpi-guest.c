/*
 * A guest-side VMPI implemented using the vmpi-impl guest interface
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

#include <linux/module.h>
#include <linux/poll.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/socket.h>

#include "vmpi-stats.h"
#include "vmpi-iovec.h"
#include "vmpi-guest-impl.h"
#include "vmpi-structs.h"
#include "vmpi.h"
#include "vmpi-ops.h"
#include "shim-hv.h"
#include "vmpi-test.h"


#ifdef VERBOSE
#define IFV(x) x
#else  /* !VERBOSE */
#define IFV(x)
#endif /* !VERBOSE */

unsigned int vmpi_max_channels = VMPI_MAX_CHANNELS_DEFAULT;
module_param(vmpi_max_channels, uint, 0444);

#define VMPI_GUEST_BUDGET  64

LIST_HEAD(vmpi_instances);
DECLARE_WAIT_QUEUE_HEAD(vmpi_instances_wqh);

struct vmpi_info {
        vmpi_impl_info_t *vi;

        struct vmpi_ring write;
        struct vmpi_queue *read;
        wait_queue_head_t read_global_wqh;

        struct work_struct recv_worker;
        struct mutex recv_worker_lock;

        vmpi_read_cb_t read_cb;
        void *read_cb_data;

        struct vmpi_ops ops;
        struct vmpi_stats stats;
        unsigned int id;
        struct list_head node;
};

int
vmpi_register_read_callback(struct vmpi_info *mpi, vmpi_read_cb_t cb,
                            void *opaque)
{
        mpi->read_cb = cb;
        mpi->read_cb_data = opaque;

        return 0;
}

static void
vmpi_clean_tx(struct vmpi_info *mpi)
{
        struct vmpi_buffer *buf;

        vmpi_impl_send_cb(mpi->vi, 0);

        while ((buf = vmpi_impl_get_written_buffer(mpi->vi)) != NULL) {
                IFV(printk("xmit done %p\n", buf));
                buf->len = 0;
                VMPI_RING_INC(mpi->write.np);
                VMPI_RING_INC(mpi->write.nr);
                mpi->stats.txres++;
        }
}

ssize_t
vmpi_write_common(struct vmpi_info *mpi, unsigned int channel,
                  const struct iovec *iv, unsigned long iovcnt, bool user)
{
        vmpi_impl_info_t *vi = mpi->vi;
        size_t len = iov_length(iv, iovcnt);
        DECLARE_WAITQUEUE(wait, current);
        size_t buf_data_size = mpi->write.buf_size - sizeof(struct vmpi_hdr);
        ssize_t ret = 0;

        if (!vi)
                return -EBADFD;

        IFV(printk("vmpi_info_aio_write user-buf (%lu,%d)\n",
                   iovcnt, (int)len));

        add_wait_queue(&mpi->write.wqh, &wait);
        while (len) {
                size_t copylen;
                struct vmpi_buffer *buf;

                current->state = TASK_INTERRUPTIBLE;

                mutex_lock(&mpi->write.lock);
                vmpi_clean_tx(mpi);
                if (vmpi_ring_unused(&mpi->write) == 0) {
                        if (vmpi_impl_send_cb(vi, 1)) {
                                mutex_unlock(&mpi->write.lock);
                                if (signal_pending(current)) {
                                        ret = -ERESTARTSYS;
                                        break;
                                }

                                /* Nothing to read, let's sleep */
                                schedule();
                                continue;
                        }
                        vmpi_clean_tx(mpi);
                }

                buf = &mpi->write.bufs[mpi->write.nu];

                copylen = len;
                if (copylen > buf_data_size) {
                        copylen = buf_data_size;
                }
                vmpi_buffer_hdr(buf)->channel = channel;
                if (user) {
                        if (memcpy_fromiovecend(vmpi_buffer_data(buf),
                                                iv, 0, copylen)) {
                                ret = -EFAULT;
                                break;
                        }
                } else {
                        iovec_to_buf(iv, iovcnt, vmpi_buffer_data(buf),
                                     copylen);
                }
                buf->len = sizeof(struct vmpi_hdr) + copylen;
                VMPI_RING_INC(mpi->write.nu);

                ret = vmpi_impl_write_buf(vi, buf);
                if (ret == 0) {
                        ret = copylen;
                }
                mpi->stats.txreq++;
                vmpi_impl_txkick(vi);
                mutex_unlock(&mpi->write.lock);
                break;
        }

        current->state = TASK_RUNNING;
        remove_wait_queue(&mpi->write.wqh, &wait);

        IFV(printk("vmpi_info_aio_write completed --> %d\n", (int)ret));

        return ret;
}

ssize_t
vmpi_read(struct vmpi_info *mpi, unsigned int channel,
          const struct iovec *iv, unsigned long iovcnt)
{
        ssize_t len = iov_length(iv, iovcnt);
        DECLARE_WAITQUEUE(wait, current);
        ssize_t ret = 0;

        if (unlikely(!mpi)) {
                return -EBADFD;
        }

        if (unlikely(channel >= vmpi_max_channels || len < 0)) {
                return -EINVAL;
        }

        IFV(printk("vmpi_info_aio-read user-buf (%lu, %d)\n",
                   iovcnt, (int)len));

        add_wait_queue(&mpi->read_global_wqh, &wait);
        while (len) {
                ssize_t copylen;
                struct vmpi_buffer *buf;

                current->state = TASK_INTERRUPTIBLE;

                mutex_lock(&mpi->read[channel].lock);
                if (vmpi_queue_len(&mpi->read[channel]) == 0) {
                        mutex_unlock(&mpi->read[channel].lock);
                        if (signal_pending(current)) {
                                ret = -ERESTARTSYS;
                                break;
                        }

                        /* Nothing to read, let's sleep */
                        schedule();
                        continue;
                }

                buf = vmpi_queue_pop_front(&mpi->read[channel]);
                mutex_unlock(&mpi->read[channel].lock);

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
        remove_wait_queue(&mpi->read_global_wqh, &wait);

        IFV(printk("vmpi_info_aio-read completed --> %d\n", (int)ret));

        return ret;
}

static void
xmit_callback(vmpi_impl_info_t *vi)
{
        struct vmpi_info *mpi = vmpi_info_from_vmpi_impl_info(vi);

        /* XXX can we disable xmit callbacks here, to avoid an
               useless burst of TX interrupts? */
        wake_up_interruptible_poll(&mpi->write.wqh, POLLOUT |
                                   POLLWRNORM | POLLWRBAND);
}

static void
recv_worker_function(struct work_struct *work)
{
        struct vmpi_info *mpi =
                container_of(work, struct vmpi_info, recv_worker);
        struct vmpi_impl_info *vi = mpi->vi;
        unsigned int budget = VMPI_GUEST_BUDGET;
        struct vmpi_buffer *buf;
        unsigned int channel;
        struct vmpi_queue *queue;

        mutex_lock(&mpi->recv_worker_lock);
 again:
        while (budget && (buf = vmpi_impl_read_buffer(vi)) != NULL) {
                IFV(printk("received %d bytes\n", (int)buf->len));
                channel = vmpi_buffer_hdr(buf)->channel;

                if (!mpi->read_cb) {
                        if (unlikely(channel >= vmpi_max_channels)) {
                                printk("WARNING: bogus channel index %u\n", channel);
                                channel = 0;
                        }

                        queue = &mpi->read[channel];
                        mutex_lock(&queue->lock);
                        if (unlikely(vmpi_queue_len(queue) >= VMPI_RING_SIZE)) {
                                vmpi_buffer_destroy(buf);
                        } else {
                                vmpi_queue_push_back(queue, buf);
                        }
                        mutex_unlock(&queue->lock);
                } else {
                        /* XXX don't drop the lock here */
                        mutex_unlock(&mpi->recv_worker_lock);
                        mpi->read_cb(mpi->read_cb_data, channel,
                                     vmpi_buffer_data(buf),
                                     buf->len - sizeof(struct vmpi_hdr));
                        mutex_lock(&mpi->recv_worker_lock);
                        vmpi_buffer_destroy(buf);
                }
                mpi->stats.rxres++;
                budget--;
        }

        if (!budget) {
                schedule_work(&mpi->recv_worker); /* HZ/20 */
        } else if (!vmpi_impl_receive_cb(vi, 1)) {
                vmpi_impl_receive_cb(vi, 0);
                goto again;
        }

        mutex_unlock(&mpi->recv_worker_lock);

        wake_up_interruptible_poll(&mpi->read_global_wqh, POLLIN |
                                   POLLRDNORM | POLLRDBAND);
}

static void
recv_callback(vmpi_impl_info_t *vi)
{
        struct vmpi_info *mpi = vmpi_info_from_vmpi_impl_info(vi);

        vmpi_impl_receive_cb(vi, 0);
        schedule_work(&mpi->recv_worker);
}

static ssize_t
vmpi_guest_ops_write(struct vmpi_ops *ops, unsigned int channel,
                     const struct iovec *iv, unsigned long iovcnt)
{
        return vmpi_write_common(ops->priv, channel, iv, iovcnt, 0);
}

static int
vmpi_guest_ops_register_read_callback(struct vmpi_ops *ops, vmpi_read_cb_t cb,
                                      void *opaque)
{
        return vmpi_register_read_callback(ops->priv, cb, opaque);
}

/*
 * This is a "template" to be included after the definition of struct
 * vmpi_info.
 */
#include "vmpi-instances.h"

struct vmpi_info *
vmpi_find_instance(unsigned int id)
{
        return __vmpi_find_instance(&vmpi_instances, &vmpi_instances_wqh, id);
}

struct vmpi_info *
vmpi_init(vmpi_impl_info_t *vi, int *ret, bool deferred_test_init)
{
        struct vmpi_info *mpi;
        int i;

        *ret = -ENOMEM;

        if (vi == NULL) {
                printk("Invalid vmpi_init() null implementation\n");
                *ret = -ENXIO;

                return NULL;
        }

        /* Allocate the vmpi_info instance. */
        mpi = kmalloc(sizeof(*mpi), GFP_KERNEL);
        if (mpi == NULL) {
                goto alloc_test;
        }
        memset(mpi, 0, sizeof(*mpi));
        vmpi_stats_init(&mpi->stats);

        mpi->vi = vi;

        *ret = vmpi_ring_init(&mpi->write, VMPI_BUF_SIZE);
        if (*ret) {
                goto alloc_write_buf;
        }

        mpi->read = kmalloc(sizeof(mpi->read[0]) * vmpi_max_channels,
                            GFP_KERNEL);
        if (mpi->read == NULL) {
                goto alloc_read_queues;
        }

        for (i = 0; i < vmpi_max_channels; i++) {
                *ret = vmpi_queue_init(&mpi->read[i], 0, VMPI_BUF_SIZE);
                if (*ret) {
                        goto alloc_read_buf;
                }
        }
        init_waitqueue_head(&mpi->read_global_wqh);

        INIT_WORK(&mpi->recv_worker, recv_worker_function);
        mutex_init(&mpi->recv_worker_lock);

        vmpi_impl_callbacks_register(mpi->vi, xmit_callback, recv_callback);
        mpi->read_cb = NULL;

        mpi->ops.priv = mpi;
        mpi->ops.write = vmpi_guest_ops_write;
        mpi->ops.register_read_callback = vmpi_guest_ops_register_read_callback;
        *ret = shim_hv_init(&mpi->ops);
        if (*ret) {
                goto alloc_read_buf;
        }

#ifdef VMPI_TEST
        *ret = vmpi_test_init(mpi, deferred_test_init);
        if (*ret) {
                printk("vmpi_test_init() failed\n");
                goto vmpi_test_ini;
        }
#endif  /* VMPI_TEST */


        vmpi_add_instance(&vmpi_instances, &vmpi_instances_wqh, mpi);

        printk("vmpi_init completed\n");

        *ret = 0;

        return mpi;

#ifdef VMPI_TEST
 vmpi_test_ini:
        shim_hv_fini();
        vmpi_impl_callbacks_unregister(mpi->vi);
#endif  /* VMPI_TEST */
 alloc_read_buf:
        for (--i; i >= 0; i--) {
                vmpi_queue_fini(&mpi->read[i]);
        }
        kfree(mpi->read);
 alloc_read_queues:
        vmpi_ring_fini(&mpi->write);
 alloc_write_buf:
        vmpi_stats_fini(&mpi->stats);
        kfree(mpi);
 alloc_test:

        return NULL;
}

void
vmpi_fini(struct vmpi_info *mpi, bool deferred_test_fini)
{
        unsigned int i;

        vmpi_remove_instance(&vmpi_instances, mpi);

#ifdef VMPI_TEST
        vmpi_test_fini(mpi, deferred_test_fini);
#endif  /* VMPI_TEST */

        if (mpi == NULL) {
                printk("vmpi_info_fini: NULL pointer\n");
                BUG_ON(1);
                return;
        }

        shim_hv_fini();

        /*
         * Deregister the callbacks, so that vmpi-impl will stop
         * scheduling the receive worker.
         */
        vmpi_impl_callbacks_unregister(mpi->vi);

        /*
         * Cancel and flush any pending receive work, so that we
         * can free the RX resources.
         */
        cancel_work_sync(&mpi->recv_worker);

        for (i = 0; i < vmpi_max_channels; i++) {
                vmpi_queue_fini(&mpi->read[i]);
        }
        kfree(mpi->read);
        vmpi_ring_fini(&mpi->write);
        vmpi_stats_fini(&mpi->stats);
        kfree(mpi);

        printk("vmpi_fini completed\n");
}
