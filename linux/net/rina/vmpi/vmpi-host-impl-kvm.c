/*
 * An hypervisor-side vmpi-impl implementation for KVM and virtio
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
#include <linux/eventfd.h>
#include <linux/vhost.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/file.h>
#include <linux/slab.h>
#include <linux/aio.h>
#include <linux/sched.h>

#include "../../../drivers/vhost/vhost.h"

#include "vmpi-iovec.h"
#include "vmpi-structs.h"
#include "vmpi.h"
#include "vmpi-host-impl.h"


#ifdef VERBOSE
#define IFV(x) x
#else  /* VERBOSE */
#define IFV(x)
#endif /* VERBOSE */

/* Max number of bytes transferred before requeueing the job.
 * Using this limit prevents one virtqueue from starving others. */
#define VHOST_MPI_WEIGHT_X  0x80000
#define VHOST_MPI_WEIGHT    150

enum {
        VHOST_MPI_FEATURES = VHOST_FEATURES,
};

enum {
        VHOST_MPI_VQ_RX = 0,
        VHOST_MPI_VQ_TX = 1,
        VHOST_MPI_VQ_MAX = 2,
};

extern unsigned int vmpi_max_channels;

struct vmpi_impl_queue {
        struct vhost_virtqueue vq;
};

struct vmpi_impl_info {
        struct vhost_dev dev;
        struct vmpi_impl_queue vqs[VHOST_MPI_VQ_MAX];
        struct vhost_poll poll[VHOST_MPI_VQ_MAX];

        struct file *file;
        wait_queue_head_t wqh_poll;

        struct vmpi_info *mpi;
        struct vmpi_ring *write;
        struct vmpi_queue *read;
        struct vmpi_stats *stats;
        vmpi_read_cb_t read_cb;
        void *read_cb_data;
        struct vmpi_queue read_cb_queue;
        struct work_struct read_cb_worker;
};

int
vmpi_impl_register_read_callback(vmpi_impl_info_t *vi, vmpi_read_cb_t cb,
                                 void *opaque)
{
        vi->read_cb = cb;
        vi->read_cb_data = opaque;

        return 0;
}

static void
vhost_mpi_vq_reset(struct vmpi_impl_info *vi)
{
}

static void
read_cb_worker_function(struct work_struct *work)
{
        struct vmpi_impl_info *vi =
                container_of(work, struct vmpi_impl_info, read_cb_worker);

        for (;;) {
                struct vmpi_buffer *buf;

                mutex_lock(&vi->read_cb_queue.lock);
                buf = vmpi_queue_pop_front(&vi->read_cb_queue);
                mutex_unlock(&vi->read_cb_queue.lock);
                if (buf == NULL) {
                        break;
                }
                vi->read_cb(vi->read_cb_data,
                            vmpi_buffer_hdr(buf)->channel,
                            vmpi_buffer_data(buf),
                            buf->len - sizeof(struct vmpi_hdr));
                vmpi_buffer_destroy(buf);
        }
}

/* Expects to be always run from workqueue - which acts as
 * read-size critical section for our kind of RCU. */
static void
handle_guest_tx(struct vmpi_impl_info *vi)
{
        struct vmpi_impl_queue *nvq = &vi->vqs[VHOST_MPI_VQ_TX];
        struct vhost_virtqueue *vq = &nvq->vq;
        struct vmpi_queue *read;
        unsigned out, in;
        int head;
        size_t len, total_len = 0;
        void *opaque;
        struct vmpi_buffer *buf = NULL;
        unsigned int channel;

        mutex_lock(&vq->mutex);
        opaque = vq->private_data;
        if (!opaque)
                goto out;

        vhost_disable_notify(&vi->dev, vq);

        for (;;) {
                head = vhost_get_vq_desc(&vi->dev, vq, vq->iov,
                                         ARRAY_SIZE(vq->iov),
                                         &out, &in,
                                         NULL, NULL);
                /* On error, stop handling until the next kick. */
                if (unlikely(head < 0))
                        break;
                /* Nothing new?  Wait for eventfd to tell us they refilled. */
                if (head == vq->num) {
                        if (unlikely(vhost_enable_notify(&vi->dev, vq))) {
                                vhost_disable_notify(&vi->dev, vq);
                                continue;
                        }
                        break;
                }
                if (in) {
                        vq_err(vq, "Unexpected descriptor format for TX: "
                               "out %d, int %d\n", out, in);
                        break;
                }

                len = iov_length(vq->iov, out);
                buf = vmpi_buffer_create(VMPI_BUF_SIZE);
                if (unlikely(buf == NULL)) {
                        printk("vmpi_buffer_create(%u) failed\n",
                               VMPI_BUF_SIZE);
                } else {
                        IFV(printk("transmit (%u, %d)\n", out, (int)len));
                        len = iovec_to_buf(vq->iov, out, buf->p, buf->size);
                        buf->len = len;
                        IFV(printk("popped %d bytes from the TX ring\n",
                                   (int)len));

                        if (!vi->read_cb) {
                                channel = vmpi_buffer_hdr(buf)->channel;
                                if (unlikely(channel >= vmpi_max_channels)) {
                                        printk("bogus channel request: %u\n", channel);
                                        channel = 0;
                                }

                                read = &vi->read[channel];
                                mutex_lock(&read->lock);
                                if (unlikely(vmpi_queue_len(read) >=
                                             VMPI_RING_SIZE)) {
                                        vmpi_buffer_destroy(buf);
                                } else {
                                        vmpi_queue_push_back(read, buf);
                                }
                                mutex_unlock(&read->lock);

                                wake_up_interruptible_poll(&read->wqh,
                                                           POLLIN |
                                                           POLLRDNORM |
                                                           POLLRDBAND);
                        } else {
                                mutex_lock(&vi->read_cb_queue.lock);
                                vmpi_queue_push_back(&vi->read_cb_queue, buf);
                                mutex_unlock(&vi->read_cb_queue.lock);
                                schedule_work(&vi->read_cb_worker);
                        }
                        vi->stats->rxres++;
                }

                vhost_add_used_and_signal(&vi->dev, vq, head, 0);
                total_len += len;
                if (unlikely(total_len >= VHOST_MPI_WEIGHT)) {
                        vhost_poll_queue(&vq->poll);
                        break;
                }
        }
 out:
        mutex_unlock(&vq->mutex);
}

static int
peek_head_len(struct vmpi_ring *ring)
{
        int ret = 0;

        if (vmpi_ring_pending(ring)) {
                ret = ring->bufs[ring->np].len;
        }

        return ret;
}

/* This is a multi-buffer version of vhost_get_desc, that works if
 *      vq has read descriptors only.
 * XXX probably useless for us (for now)
 * @vq          - the relevant virtqueue
 * @datalen     - data length we'll be reading
 * @iovcount    - returned count of io vectors we fill
 * @log         - vhost log
 * @log_num     - log offset
 * @quota       - headcount quota, 1 for big buffer
 *      returns number of buffer heads allocated, negative on error
 */
static int get_rx_bufs(struct vhost_virtqueue *vq,
                       struct vring_used_elem *heads,
                       int datalen,
                       unsigned *iovcount,
                       struct vhost_log *log,
                       unsigned *log_num,
                       unsigned int quota)
{
        unsigned int out, in;
        int seg = 0;
        int headcount = 0;
        unsigned d;
        int r, nlogs = 0;

        while (datalen > 0 && headcount < quota) {
                if (unlikely(seg >= UIO_MAXIOV)) {
                        r = -ENOBUFS;
                        goto err;
                }
                d = vhost_get_vq_desc(vq->dev, vq, vq->iov + seg,
                                      ARRAY_SIZE(vq->iov) - seg, &out,
                                      &in, log, log_num);
                if (d == vq->num) {
                        r = 0;
                        goto err;
                }
                if (unlikely(out || in <= 0)) {
                        vq_err(vq, "unexpected descriptor format for RX: "
                               "out %d, in %d\n", out, in);
                        r = -EINVAL;
                        goto err;
                }
                if (unlikely(log)) {
                        nlogs += *log_num;
                        log += *log_num;
                }
                heads[headcount].id = d;
                heads[headcount].len = iov_length(vq->iov + seg, in);
                datalen -= heads[headcount].len;
                ++headcount;
                seg += in;
        }
        heads[headcount - 1].len += datalen;
        *iovcount = seg;
        if (unlikely(log))
                *log_num = nlogs;
        return headcount;
 err:
        vhost_discard_vq_desc(vq, headcount);
        return r;
}

/* Expects to be always run from workqueue - which acts as
 * read-size critical section for our kind of RCU. */
static void
handle_guest_rx(struct vmpi_impl_info *vi)
{
        struct vmpi_impl_queue *nvq = &vi->vqs[VHOST_MPI_VQ_RX];
        struct vhost_virtqueue *vq = &nvq->vq;
        struct vmpi_ring *ring = vi->write;
        unsigned uninitialized_var(in), log;
        struct vhost_log *vq_log;
        size_t total_len = 0;
        s16 headcount;
        size_t len;
        void *opaque;
        struct vmpi_buffer *buf = NULL;

        mutex_lock(&vq->mutex);
        opaque = vq->private_data;
        if (!opaque)
                goto out;
        vhost_disable_notify(&vi->dev, vq);

        vq_log = unlikely(vhost_has_feature(&vi->dev, VHOST_F_LOG_ALL)) ?
                vq->log : NULL;

        while ((len = peek_head_len(ring))) {
                IFV(printk("peek %d bytes\n", (int)len));
                headcount = get_rx_bufs(vq, vq->heads, len,
                                        &in, vq_log, &log, 1);
                /* On error, stop handling until the next kick. */
                if (unlikely(headcount < 0))
                        break;
                /* OK, now we need to know about added descriptors. */
                if (!headcount) {
                        if (unlikely(vhost_enable_notify(&vi->dev, vq))) {
                                /* They have slipped one in as we were
                                 * doing that: check again. */
                                vhost_disable_notify(&vi->dev, vq);
                                continue;
                        }
                        /* Nothing new?  Wait for eventfd to tell us
                         * they refilled. */
                        break;
                }
                /* We don't need to be notified again. */

                buf = &ring->bufs[ring->np];
                IFV(printk("received %d\n", (int)len));
                len = iovec_from_buf(vq->iov, in, buf->p, len);
                buf->len = 0;
                VMPI_RING_INC(ring->np);
                VMPI_RING_INC(ring->nr);

                wake_up_interruptible_poll(&ring->wqh, POLLOUT |
                                           POLLWRNORM | POLLWRBAND);
                IFV(printk("pushed %d bytes in the RX ring\n", (int)len));
                vi->stats->txres++;

                vhost_add_used_and_signal_n(&vi->dev, vq, vq->heads,
                                            headcount);
                if (unlikely(vq_log))
                        vhost_log_write(vq, vq_log, log, len);
                total_len += len;
                if (unlikely(total_len >= VHOST_MPI_WEIGHT)) {
                        vhost_poll_queue(&vq->poll);
                        break;
                }
        }
 out:
        mutex_unlock(&vq->mutex);
}

static void
handle_tx_kick(struct vhost_work *work)
{
        struct vhost_virtqueue *vq = container_of(work, struct vhost_virtqueue,
                                                  poll.work);
        struct vmpi_impl_info *vi = container_of(vq->dev,
                                                 struct vmpi_impl_info, dev);

        handle_guest_tx(vi);
}

static void
handle_rx_kick(struct vhost_work *work)
{
        struct vhost_virtqueue *vq = container_of(work, struct vhost_virtqueue,
                                                  poll.work);
        struct vmpi_impl_info *vi = container_of(vq->dev,
                                                 struct vmpi_impl_info, dev);

        handle_guest_rx(vi);
}

static void
handle_tx_mpi(struct vhost_work *work)
{
        struct vmpi_impl_info *vi = container_of(work, struct vmpi_impl_info,
                                                 poll[VHOST_MPI_VQ_TX].work);
        handle_guest_tx(vi);
}

static void
handle_rx_mpi(struct vhost_work *work)
{
        struct vmpi_impl_info *vi = container_of(work, struct vmpi_impl_info,
                                                 poll[VHOST_MPI_VQ_RX].work);
        handle_guest_rx(vi);
}

static int
vhost_mpi_open(struct inode *inode, struct file *f)
{
        struct vmpi_impl_info *vi = kmalloc(sizeof *vi, GFP_KERNEL);
        struct vhost_dev *dev;
        struct vhost_virtqueue **vqs;
        int r;

        if (!vi)
                return -ENOMEM;
        vqs = kmalloc(VHOST_MPI_VQ_MAX * sizeof(*vqs), GFP_KERNEL);
        if (!vqs) {
                kfree(vi);
                return -ENOMEM;
        }

        dev = &vi->dev;
        vqs[VHOST_MPI_VQ_TX] = &vi->vqs[VHOST_MPI_VQ_TX].vq;
        vqs[VHOST_MPI_VQ_RX] = &vi->vqs[VHOST_MPI_VQ_RX].vq;
        vi->vqs[VHOST_MPI_VQ_TX].vq.handle_kick = handle_tx_kick;
        vi->vqs[VHOST_MPI_VQ_RX].vq.handle_kick = handle_rx_kick;

        vhost_dev_init(dev, vqs, VHOST_MPI_VQ_MAX);

        vhost_poll_init(vi->poll + VHOST_MPI_VQ_TX,
                        handle_tx_mpi, POLLOUT, dev);
        vhost_poll_init(vi->poll + VHOST_MPI_VQ_RX,
                        handle_rx_mpi, POLLIN, dev);

        f->private_data = vi;
        vi->file = f;

        init_waitqueue_head(&vi->wqh_poll);

        /* We tell vmpi_test_init() to do its work in another
         * process context, in order to avoid a deadlock.
         * The deadlock would show up because in this point
         * we hold the misc_open() mutex, and the same mutex
         * is required by misc_register(), which is necessary
         * to init the test device.
         */
        vi->mpi = vmpi_init(vi, &r, true);
        if (vi->mpi == NULL) {
                goto buf_alloc_fail;
        }
        vi->write = vmpi_get_write_ring(vi->mpi);
        vi->read = vmpi_get_read_queue(vi->mpi);
        vi->stats = vmpi_get_stats(vi->mpi);
        vi->read_cb = NULL;
        INIT_WORK(&vi->read_cb_worker, read_cb_worker_function);
        vmpi_queue_init(&vi->read_cb_queue, 0, VMPI_BUF_SIZE);

        printk("vhost_mpi_open completed\n");

        return 0;

 buf_alloc_fail:
        vhost_dev_cleanup(&vi->dev, false);
        kfree(vqs);
        kfree(vi);

        return r;
}

static void
vhost_mpi_disable_vq(struct vmpi_impl_info *vi, struct vhost_virtqueue *vq)
{
        struct vmpi_impl_queue *nvq =
                container_of(vq, struct vmpi_impl_queue, vq);
        struct vhost_poll *poll = vi->poll + (nvq - vi->vqs);
        if (!vq->private_data)
                return;
        vhost_poll_stop(poll);
}

static int
vhost_mpi_enable_vq(struct vmpi_impl_info *vi, struct vhost_virtqueue *vq)
{
        struct vmpi_impl_queue *nvq =
                container_of(vq, struct vmpi_impl_queue, vq);
        struct vhost_poll *poll = vi->poll + (nvq - vi->vqs);

        return vhost_poll_start(poll, vi->file);
}

static void
vhost_mpi_stop_vq(struct vmpi_impl_info *vi, struct vhost_virtqueue *vq)
{
        mutex_lock(&vq->mutex);
        vhost_mpi_disable_vq(vi, vq);
        vq->private_data = NULL;
        mutex_unlock(&vq->mutex);
}

static void
vhost_mpi_stop(struct vmpi_impl_info *vi)
{
        vhost_mpi_stop_vq(vi, &vi->vqs[VHOST_MPI_VQ_TX].vq);
        vhost_mpi_stop_vq(vi, &vi->vqs[VHOST_MPI_VQ_RX].vq);
}

static void
vhost_mpi_flush_vq(struct vmpi_impl_info *vi, int index)
{
        vhost_poll_flush(vi->poll + index);
        vhost_poll_flush(&vi->vqs[index].vq.poll);
}

static void
vhost_mpi_flush(struct vmpi_impl_info *vi)
{
        vhost_mpi_flush_vq(vi, VHOST_MPI_VQ_TX);
        vhost_mpi_flush_vq(vi, VHOST_MPI_VQ_RX);
}

static int
vhost_mpi_release(struct inode *inode, struct file *f)
{
        struct vmpi_impl_info *vi = f->private_data;

        vhost_mpi_stop(vi);
        vhost_mpi_flush(vi);
        vhost_dev_stop(&vi->dev);
        vhost_dev_cleanup(&vi->dev, false);
        vhost_mpi_vq_reset(vi);
        /* We do an extra flush before freeing memory,
         * since jobs can re-queue themselves. */
        vhost_mpi_flush(vi);
        /* Here the same reasoning explained above (right
         * before the vmpi_init() call) apply.
         */
        vmpi_fini(vi->mpi, true);
        kfree(vi->dev.vqs);
        kfree(vi);

        printk("vhost_mpi_release completed\n");

        return 0;
}

static long
vhost_mpi_startstop(struct vmpi_impl_info *vi, unsigned index, unsigned enable)
{
        struct vhost_virtqueue *vq;
        struct vmpi_impl_queue *nvq;
        struct file *file, *oldfile;
        int r;

        printk("vhost_mpi_startstop idx = %d enable = %d\n", index, enable);

        mutex_lock(&vi->dev.mutex);
        r = vhost_dev_check_owner(&vi->dev);
        if (r)
                goto err;

        if (index >= VHOST_MPI_VQ_MAX) {
                r = -ENOBUFS;
                goto err;
        }
        vq = &vi->vqs[index].vq;
        nvq = &vi->vqs[index];
        mutex_lock(&vq->mutex);

        /* Verify that ring has been setup correctly. */
        if (!vhost_vq_access_ok(vq)) {
                r = -EFAULT;
                goto err_vq;
        }

        if (enable) {
                file = vi->file;
        } else {
                file = NULL;
        }

        /* start polling new socket */
        oldfile = vq->private_data;
        if (file != oldfile) {
                vhost_mpi_disable_vq(vi, vq);
                vq->private_data = file;
                r = vhost_init_used(vq);
                if (r)
                        goto err_used;
                r = vhost_mpi_enable_vq(vi, vq);
                if (r)
                        goto err_used;
        }

        mutex_unlock(&vq->mutex);

        if (oldfile) {
                vhost_mpi_flush_vq(vi, index);
        }

        mutex_unlock(&vi->dev.mutex);
        return 0;

 err_used:
        vq->private_data = oldfile;
        vhost_mpi_enable_vq(vi, vq);
 err_vq:
        mutex_unlock(&vq->mutex);
 err:
        mutex_unlock(&vi->dev.mutex);
        return r;
}

static long
vhost_mpi_reset_owner(struct vmpi_impl_info *vi)
{
        long err;
        struct vhost_memory *memory;

        mutex_lock(&vi->dev.mutex);
        err = vhost_dev_check_owner(&vi->dev);
        if (err)
                goto done;
        memory = vhost_dev_reset_owner_prepare();
        if (!memory) {
                err = -ENOMEM;
                goto done;
        }
        vhost_mpi_stop(vi);
        vhost_mpi_flush(vi);
        vhost_dev_reset_owner(&vi->dev, memory);
        vhost_mpi_vq_reset(vi);
 done:
        mutex_unlock(&vi->dev.mutex);
        return err;
}

static int
vhost_mpi_set_features(struct vmpi_impl_info *vi, u64 features)
{
        mutex_lock(&vi->dev.mutex);
        if ((features & (1 << VHOST_F_LOG_ALL)) &&
            !vhost_log_access_ok(&vi->dev)) {
                mutex_unlock(&vi->dev.mutex);
                return -EFAULT;
        }
        vi->dev.acked_features = features;
        smp_wmb();
        vhost_mpi_flush(vi);
        mutex_unlock(&vi->dev.mutex);
        return 0;
}

static long
vhost_mpi_set_owner(struct vmpi_impl_info *vi)
{
        int r;

        mutex_lock(&vi->dev.mutex);
        if (vhost_dev_has_owner(&vi->dev)) {
                r = -EBUSY;
                goto out;
        }
        r = vhost_dev_set_owner(&vi->dev);
        vhost_mpi_flush(vi);
 out:
        mutex_unlock(&vi->dev.mutex);
        return r;
}

struct vhost_mpi_command {
        unsigned int index;
        unsigned int enable;
};

/* Name overloading. Should be a temporary solution. */
#define VHOST_MPI_STARTSTOP VHOST_NET_SET_BACKEND

static long
vhost_mpi_ioctl(struct file *f, unsigned int ioctl,
                unsigned long arg)
{
        struct vmpi_impl_info *vi = f->private_data;
        void __user *argp = (void __user *)arg;
        u64 __user *featurep = argp;
        struct vhost_mpi_command mpi_cmd;
        u64 features;
        int r;

        switch (ioctl) {
        case VHOST_MPI_STARTSTOP:
                if (copy_from_user(&mpi_cmd, argp, sizeof mpi_cmd))
                        return -EFAULT;
                return vhost_mpi_startstop(vi, mpi_cmd.index,
                                           mpi_cmd.enable);
        case VHOST_GET_FEATURES:
                features = VHOST_MPI_FEATURES;
                if (copy_to_user(featurep, &features, sizeof features))
                        return -EFAULT;
                return 0;
        case VHOST_SET_FEATURES:
                if (copy_from_user(&features, featurep,
                                   sizeof features))
                        return -EFAULT;
                if (features & ~VHOST_MPI_FEATURES)
                        return -EOPNOTSUPP;
                return vhost_mpi_set_features(vi, features);
        case VHOST_RESET_OWNER:
                return vhost_mpi_reset_owner(vi);
        case VHOST_SET_OWNER:
                return vhost_mpi_set_owner(vi);
        default:
                mutex_lock(&vi->dev.mutex);
                r = vhost_dev_ioctl(&vi->dev, ioctl, argp);
                if (r == -ENOIOCTLCMD)
                        r = vhost_vring_ioctl(&vi->dev, ioctl, argp);
                else
                        vhost_mpi_flush(vi);
                mutex_unlock(&vi->dev.mutex);
                return r;
        }
}

#ifdef CONFIG_COMPAT
static long
vhost_mpi_compat_ioctl(struct file *f, unsigned int ioctl,
                       unsigned long arg)
{
        return vhost_mpi_ioctl(f, ioctl, (unsigned long)compat_ptr(arg));
}
#endif

static unsigned int
vhost_mpi_poll(struct file *file, poll_table *wait)
{
        struct vmpi_impl_info *vi = file->private_data;
        struct vmpi_ring *write = vi->write;
        unsigned int mask = 0;

        if (!vi)
                return POLLERR;

        poll_wait(file, &vi->wqh_poll, wait);

        /*
         * Are there host resources for the guest to receive
         * (e.g. pending rx packets)?
         */
        mutex_lock(&write->lock);
        if (vmpi_ring_pending(write))
                mask |= POLLIN | POLLRDNORM;
        mutex_unlock(&write->lock);

        /*
         * Are there host resources for the guest to send
         * (e.g. buffers in the read pool)?
         */
        mask |= POLLOUT | POLLWRNORM;

        return mask;
}

int
vmpi_impl_txkick(struct vmpi_impl_info *vi)
{
        wake_up_interruptible_poll(&vi->wqh_poll, POLLIN |
                                   POLLRDNORM | POLLRDBAND);

        return 0;

}

static const struct file_operations vhost_mpi_fops = {
        .owner          = THIS_MODULE,
        .release        = vhost_mpi_release,
        .unlocked_ioctl = vhost_mpi_ioctl,
#ifdef CONFIG_COMPAT
        .compat_ioctl   = vhost_mpi_compat_ioctl,
#endif
        .poll           = vhost_mpi_poll,
        .open           = vhost_mpi_open,
        .llseek         = noop_llseek,
};

#define VHOST_MPI_MINOR     245

static struct miscdevice vhost_mpi_misc = {
        .minor = VHOST_MPI_MINOR,
        .name = "vhost-mpi",
        .fops = &vhost_mpi_fops,
};

static int
vhost_mpi_init(void)
{
        return misc_register(&vhost_mpi_misc);
}
module_init(vhost_mpi_init);

static void
vhost_mpi_exit(void)
{
        misc_deregister(&vhost_mpi_misc);
}
module_exit(vhost_mpi_exit);

MODULE_VERSION("0.0.1");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Vincenzo Maffione");
MODULE_DESCRIPTION("Host kernel server for virtio mpi");
MODULE_ALIAS_MISCDEV(VHOST_MPI_MINOR);
MODULE_ALIAS("devname:vhost-mpi");
