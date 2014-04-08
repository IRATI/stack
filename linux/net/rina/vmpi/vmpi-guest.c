/* A VMPI implemented using the vmpi-impl interface
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

#include <linux/module.h>
#include <linux/poll.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/socket.h>

#include "vmpi-guest-impl.h"
#include "vmpi-ring.h"
#include "vmpi.h"


#ifdef VERBOSE
#define IFV(x) x
#else  /* !VERBOSE */
#define IFV(x)
#endif /* !VERBOSE */

#define VIRTIO_MPI_TEST_BUDGET  64

struct vmpi_info {
    vmpi_impl_info_t *vi;

    struct vmpi_ring write;
    struct vmpi_ring read;

    struct work_struct recv_worker;

    /* Number of input buffers. */
    //unsigned int num;
};

static struct vmpi_info *vmpi_info_instance = NULL;

struct vmpi_info *vmpi_get_instance(void)
{
    return vmpi_info_instance;
}

static void vmpi_impl_clean_tx(struct vmpi_info *mpi)
{
    struct vmpi_buffer *buf;

    vmpi_impl_send_cb(mpi->vi, 0);

    while ((buf = vmpi_impl_get_written_buffer(mpi->vi)) != NULL) {
        IFV(printk("xmit done %p\n", buf));
        buf->len = 0;
        VMPI_RING_INC(mpi->write.np);
        VMPI_RING_INC(mpi->write.nr);
    }
}

ssize_t vmpi_write(struct vmpi_info *mpi, const struct iovec *iv,
                          unsigned long iovlen)
{
    vmpi_impl_info_t *vi = mpi->vi;
    size_t len = iov_length(iv, iovlen);
    DECLARE_WAITQUEUE(wait, current);
    ssize_t ret = 0;

    if (!vi)
        return -EBADFD;

    IFV(printk("vmpi_info_aio_write user-buf (%lu,%d)\n", iovlen, (int)len));

    add_wait_queue(&mpi->write.wqh, &wait);
    while (len) {
        size_t copylen;
        struct vmpi_buffer *buf;

        current->state = TASK_INTERRUPTIBLE;

        mutex_lock(&mpi->write.lock);
        vmpi_impl_clean_tx(mpi);
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
            vmpi_impl_clean_tx(mpi);
        }

        buf = &mpi->write.bufs[mpi->write.nu];

        copylen = len;
        if (copylen > buf->size) {
            copylen = buf->size;
        }
        if (memcpy_fromiovecend(buf->p, iv, 0, copylen)) {
            ret = -EFAULT;
            break;
        }
        buf->len = copylen;
        VMPI_RING_INC(mpi->write.nu);
        mutex_unlock(&mpi->write.lock);

        ret = vmpi_impl_write_buf(vi, buf);
        if (ret == 0) {
            ret = copylen;
        }
        vmpi_impl_txkick(vi);
        break;
    }

    current->state = TASK_RUNNING;
    remove_wait_queue(&mpi->write.wqh, &wait);

    IFV(printk("vmpi_info_aio_write completed --> %d\n", (int)ret));

    return ret;
}

ssize_t vmpi_read(struct vmpi_info *mpi, const struct iovec *iv,
                         unsigned long iovcnt)
{
    vmpi_impl_info_t *vi = mpi->vi;
    ssize_t len = iov_length(iv, iovcnt);
    DECLARE_WAITQUEUE(wait, current);
    ssize_t ret = 0;

    if (!mpi) {
        return -EBADFD;
    }

    if (len < 0) {
        return -EINVAL;
    }

    IFV(printk("vmpi_info_aio-read user-buf (%lu, %d)\n", iovcnt, (int)len));

    add_wait_queue(&mpi->read.wqh, &wait);
    while (len) {
        ssize_t copylen;
        struct vmpi_buffer *buf;

        current->state = TASK_INTERRUPTIBLE;

        mutex_lock(&mpi->read.lock);
        if (vmpi_ring_ready(&mpi->read) == 0) {
            mutex_unlock(&mpi->read.lock);
            if (signal_pending(current)) {
                ret = -ERESTARTSYS;
                break;
            }

            /* Nothing to read, let's sleep */
            schedule();
            continue;
        }

        buf = &mpi->read.bufs[mpi->read.nr];

        copylen = buf->len;
        if (len < copylen) {
            copylen = len;
        }
        ret = copylen;
        if (memcpy_toiovecend(iv, buf->p, 0, copylen))
            ret = -EFAULT;
        buf->len = 0;
        VMPI_RING_INC(mpi->read.nr);

        vmpi_impl_give_rx_buf(vi, &mpi->read.bufs[mpi->read.nu]);
        VMPI_RING_INC(mpi->read.nu);
        //++mpi->num;
        mutex_unlock(&mpi->read.lock);
        vmpi_impl_rxkick(vi);
        break;
    }

    current->state = TASK_RUNNING;
    remove_wait_queue(&mpi->read.wqh, &wait);

    IFV(printk("vmpi_info_aio-read completed --> %d\n", (int)ret));

    return ret;
}

static void xmit_callback(vmpi_impl_info_t *vi)
{
        //struct vmpi_info *mpi = vi->private;
        struct vmpi_info *mpi = vmpi_info_from_vmpi_impl_info(vi);

        wake_up_interruptible_poll(&mpi->write.wqh, POLLOUT |
                                    POLLWRNORM | POLLWRBAND);
}

static void recv_worker_function(struct work_struct *work)
{
    struct vmpi_info *mpi =
            container_of(work, struct vmpi_info, recv_worker);
    struct vmpi_impl_info *vi = mpi->vi;
    unsigned int budget = VIRTIO_MPI_TEST_BUDGET;
    struct vmpi_buffer *buf;

    mutex_lock(&mpi->read.lock);
again:
    while (budget && (buf = vmpi_impl_read_buffer(vi)) != NULL) {
        IFV(printk("received %d bytes\n", (int)buf->len));
        VMPI_RING_INC(mpi->read.np);
        //--mpi->num;
        budget--;
    }

    if (!budget) {
        schedule_work(&mpi->recv_worker); /* HZ/20 */
    } else if (!vmpi_impl_receive_cb(vi, 1)) {
        vmpi_impl_receive_cb(vi, 0);
        goto again;
    }
    mutex_unlock(&mpi->read.lock);

    wake_up_interruptible_poll(&mpi->read.wqh, POLLIN |
            POLLRDNORM | POLLRDBAND);
}

static void recv_callback(vmpi_impl_info_t *vi)
{
        //struct vmpi_info *mpi = vi->private;
        struct vmpi_info *mpi = vmpi_info_from_vmpi_impl_info(vi);

        vmpi_impl_receive_cb(vi, 0);
        schedule_work(&mpi->recv_worker);
}

struct vmpi_info *vmpi_init(int *ret)
{
    struct vmpi_info *mpi;

    *ret = -ENOMEM;

    if (vmpi_impl_get_instance() == NULL) {
        printk("virtio-mpi module is not loaded\n");
        *ret = -ENXIO;

        return NULL;
    }

    /* Allocate the vmpi_info instance. */
    mpi = kmalloc(sizeof(*mpi), GFP_KERNEL);
    if (mpi == NULL) {
        goto alloc_test;
    }

    /* Install the vmpi_info instance. */
    vmpi_info_instance = mpi;

    mpi->vi = vmpi_impl_get_instance();

    *ret = vmpi_ring_init(&mpi->write);
    if (*ret) {
        goto alloc_write_buf;
    }

    *ret = vmpi_ring_init(&mpi->read);
    if (*ret) {
        goto alloc_read_buf;
    }

    INIT_WORK(&mpi->recv_worker, recv_worker_function);

    vmpi_impl_callbacks_register(mpi->vi, xmit_callback, recv_callback);

    /* Last of all, set up some receive buffers. */
    while (vmpi_ring_unused(&mpi->read) > 0) {
        struct vmpi_buffer *buf = &mpi->read.bufs[mpi->read.nu];
        *ret = vmpi_impl_give_rx_buf(mpi->vi, buf);
        if (*ret) {
            goto give_rx_buf;
        }
        VMPI_RING_INC(mpi->read.nu);
        //++vi->rq[0].num;
    }
    vmpi_impl_rxkick(mpi->vi);

    printk("vmpi_init completed\n");

    *ret = 0;

    return mpi;

give_rx_buf:
    vmpi_impl_free_unused_bufs(mpi->vi);
    vmpi_impl_callbacks_unregister(mpi->vi);
    vmpi_ring_fini(&mpi->read);
alloc_read_buf:
    vmpi_ring_fini(&mpi->write);
alloc_write_buf:
    kfree(mpi);
alloc_test:
    vmpi_info_instance = NULL;

    return NULL;
}

void vmpi_fini(void)
{
    struct vmpi_info *mpi = vmpi_info_instance;

    if (mpi == NULL) {
        printk("vmpi_info_fini: NULL pointer\n");
        BUG_ON(1);
        return;
    }
    vmpi_impl_free_unused_bufs(mpi->vi);

    vmpi_impl_callbacks_unregister(mpi->vi);

    /* Disinstall the vmpi_info instance. */
    vmpi_info_instance = NULL;

    vmpi_ring_fini(&mpi->read);
    vmpi_ring_fini(&mpi->write);
    kfree(mpi);

    printk("vmpi_fini completed\n");
}
