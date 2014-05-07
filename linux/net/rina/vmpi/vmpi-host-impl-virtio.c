/* A vmpi-impl hypervisor implementation for virtio
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

#include "vhost.h"

#include "vmpi-ring.h"
#include "vmpi-host-impl.h"
#include "vmpi-host-test.h"


#ifdef VERBOSE
#define IFV(x) x
#else  /* VERBOSE */
#define IFV(x)
#endif /* VERBOSE */

/* Max number of bytes transferred before requeueing the job.
 * Using this limit prevents one virtqueue from starving others. */
#define VHOST_NET_WEIGHT 0x80000

enum {
	VHOST_MPI_FEATURES = VHOST_FEATURES,
};

enum {
	VHOST_NET_VQ_RX = 0,
	VHOST_NET_VQ_TX = 1,
	VHOST_NET_VQ_MAX = 2,
};

struct vmpi_impl_queue {
        struct vhost_virtqueue vq;
};

struct vmpi_impl_info {
	struct vhost_dev dev;
	struct vmpi_impl_queue vqs[VHOST_NET_VQ_MAX];
	struct vhost_poll poll[VHOST_NET_VQ_MAX];

        struct file *file;
        wait_queue_head_t wqh_poll;

        struct vmpi_info *mpi;
        struct vmpi_ring *write;
        struct vmpi_ring *read;
};

static void vhost_mpi_vq_reset(struct vmpi_impl_info *n)
{
}

static size_t iovec_to_buf(struct iovec *iov, unsigned int iovcnt,
                           void *to, size_t len)
{
    size_t copylen;
    size_t tot  = 0;

    while (iovcnt && len) {
        copylen = iov->iov_len;
        if (len < copylen) {
            copylen = len;
        }
        memcpy(to, iov->iov_base, copylen);
        tot += copylen;
        len -= copylen;
        to += copylen;
        iovcnt--;
        iov++;
    }

    return tot;
}

static size_t iovec_from_buf(struct iovec *iov, unsigned int iovcnt,
                             void *from, size_t len)
{
    size_t copylen;
    size_t tot  = 0;

    while (iovcnt && len) {
        copylen = iov->iov_len;
        if (len < copylen) {
            copylen = len;
        }
        memcpy(iov->iov_base, from, copylen);
        tot += copylen;
        len -= copylen;
        from += copylen;
        iovcnt--;
        iov++;
    }

    return tot;
}

/* Expects to be always run from workqueue - which acts as
 * read-size critical section for our kind of RCU. */
static void handle_tx(struct vmpi_impl_info *vi)
{
	struct vmpi_impl_queue *nvq = &vi->vqs[VHOST_NET_VQ_TX];
	struct vhost_virtqueue *vq = &nvq->vq;
        struct vmpi_ring *ring = vi->read;
	unsigned out, in;
	int head;
	size_t len, total_len = 0;
        void *opaque;
        struct vmpi_buffer *buf = NULL;

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

                buf = &ring->bufs[ring->np];
                len = iov_length(vq->iov, out);
                IFV(printk("transmit (%u, %d)\n", out, (int)len));
                len = iovec_to_buf(vq->iov, out, buf->p, buf->size);
                buf->len = len;
                IFV(printk("popped %d bytes from the TX ring\n", (int)len));
                VMPI_RING_INC(ring->np);

                wake_up_interruptible_poll(&ring->wqh, POLLIN |
                                            POLLRDNORM | POLLRDBAND);

		vhost_add_used_and_signal(&vi->dev, vq, head, 0);
		total_len += len;
		if (unlikely(total_len >= VHOST_NET_WEIGHT)) {
			vhost_poll_queue(&vq->poll);
			break;
		}
	}
out:
	mutex_unlock(&vq->mutex);
}

static int peek_head_len(struct vmpi_ring *ring)
{
        int ret = 0;

        if (vmpi_ring_pending(ring)) {
	    ret = ring->bufs[ring->np].len;
        }

        return ret;
}

/* This is a multi-buffer version of vhost_get_desc, that works if
 *	vq has read descriptors only.
 * XXX probably useless for us (for now)
 * @vq		- the relevant virtqueue
 * @datalen	- data length we'll be reading
 * @iovcount	- returned count of io vectors we fill
 * @log		- vhost log
 * @log_num	- log offset
 * @quota       - headcount quota, 1 for big buffer
 *	returns number of buffer heads allocated, negative on error
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
static void handle_rx(struct vmpi_impl_info *vi)
{
	struct vmpi_impl_queue *nvq = &vi->vqs[VHOST_NET_VQ_RX];
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

		vhost_add_used_and_signal_n(&vi->dev, vq, vq->heads,
					    headcount);
		if (unlikely(vq_log))
			vhost_log_write(vq, vq_log, log, len);
		total_len += len;
		if (unlikely(total_len >= VHOST_NET_WEIGHT)) {
			vhost_poll_queue(&vq->poll);
			break;
		}
	}
out:
	mutex_unlock(&vq->mutex);
}

static void handle_tx_kick(struct vhost_work *work)
{
	struct vhost_virtqueue *vq = container_of(work, struct vhost_virtqueue,
						  poll.work);
	struct vmpi_impl_info *vi = container_of(vq->dev, struct vmpi_impl_info, dev);

	handle_tx(vi);
}

static void handle_rx_kick(struct vhost_work *work)
{
	struct vhost_virtqueue *vq = container_of(work, struct vhost_virtqueue,
						  poll.work);
	struct vmpi_impl_info *vi = container_of(vq->dev, struct vmpi_impl_info, dev);

	handle_rx(vi);
}

static void handle_tx_mpi(struct vhost_work *work)
{
	struct vmpi_impl_info *vi = container_of(work, struct vmpi_impl_info,
					     poll[VHOST_NET_VQ_TX].work);
	handle_tx(vi);
}

static void handle_rx_mpi(struct vhost_work *work)
{
	struct vmpi_impl_info *vi = container_of(work, struct vmpi_impl_info,
					     poll[VHOST_NET_VQ_RX].work);
	handle_rx(vi);
}

static int vhost_mpi_open(struct inode *inode, struct file *f)
{
	struct vmpi_impl_info *n = kmalloc(sizeof *n, GFP_KERNEL);
	struct vhost_dev *dev;
	struct vhost_virtqueue **vqs;
	int r;

	if (!n)
		return -ENOMEM;
	vqs = kmalloc(VHOST_NET_VQ_MAX * sizeof(*vqs), GFP_KERNEL);
	if (!vqs) {
		kfree(n);
		return -ENOMEM;
	}

	dev = &n->dev;
	vqs[VHOST_NET_VQ_TX] = &n->vqs[VHOST_NET_VQ_TX].vq;
	vqs[VHOST_NET_VQ_RX] = &n->vqs[VHOST_NET_VQ_RX].vq;
	n->vqs[VHOST_NET_VQ_TX].vq.handle_kick = handle_tx_kick;
	n->vqs[VHOST_NET_VQ_RX].vq.handle_kick = handle_rx_kick;
        n->mpi = vmpi_init(n, &r);
        if (n->mpi == NULL) {
            goto buf_alloc_fail;
        }
        n->write = vmpi_get_write_ring(n->mpi);
        n->read = vmpi_get_read_ring(n->mpi);

	r = vhost_dev_init(dev, vqs, VHOST_NET_VQ_MAX);
	if (r < 0) {
                vmpi_fini(n->mpi);
		kfree(n);
		kfree(vqs);
		return r;
	}

	vhost_poll_init(n->poll + VHOST_NET_VQ_TX, handle_tx_mpi, POLLOUT, dev);
	vhost_poll_init(n->poll + VHOST_NET_VQ_RX, handle_rx_mpi, POLLIN, dev);

	f->private_data = n;
        n->file = f;

        init_waitqueue_head(&n->wqh_poll);
        /* Mark the read vmpi ring (corresponding to the virtio TX ring)
           as completely available. */
        n->read->nu = RING_SIZE - 1;

        printk("vhost_mpi_open completed\n");

	return 0;

buf_alloc_fail:
    kfree(vqs);
    kfree(n);

    return -ENOMEM;
}

static void vhost_mpi_disable_vq(struct vmpi_impl_info *n,
				 struct vhost_virtqueue *vq)
{
	struct vmpi_impl_queue *nvq =
		container_of(vq, struct vmpi_impl_queue, vq);
	struct vhost_poll *poll = n->poll + (nvq - n->vqs);
	if (!vq->private_data)
		return;
	vhost_poll_stop(poll);
}

static int vhost_mpi_enable_vq(struct vmpi_impl_info *n,
				struct vhost_virtqueue *vq)
{
	struct vmpi_impl_queue *nvq =
		container_of(vq, struct vmpi_impl_queue, vq);
	struct vhost_poll *poll = n->poll + (nvq - n->vqs);

	return vhost_poll_start(poll, n->file);
}

static void vhost_mpi_stop_vq(struct vmpi_impl_info *n,
					struct vhost_virtqueue *vq)
{
	mutex_lock(&vq->mutex);
	vhost_mpi_disable_vq(n, vq);
	vq->private_data = NULL;
	mutex_unlock(&vq->mutex);
}

static void vhost_mpi_stop(struct vmpi_impl_info *n)
{
	vhost_mpi_stop_vq(n, &n->vqs[VHOST_NET_VQ_TX].vq);
	vhost_mpi_stop_vq(n, &n->vqs[VHOST_NET_VQ_RX].vq);
}

static void vhost_mpi_flush_vq(struct vmpi_impl_info *n, int index)
{
	vhost_poll_flush(n->poll + index);
	vhost_poll_flush(&n->vqs[index].vq.poll);
}

static void vhost_mpi_flush(struct vmpi_impl_info *n)
{
	vhost_mpi_flush_vq(n, VHOST_NET_VQ_TX);
	vhost_mpi_flush_vq(n, VHOST_NET_VQ_RX);
}

static int vhost_mpi_release(struct inode *inode, struct file *f)
{
	struct vmpi_impl_info *n = f->private_data;

	vhost_mpi_stop(n);
	vhost_mpi_flush(n);
	vhost_dev_stop(&n->dev);
	vhost_dev_cleanup(&n->dev, false);
	vhost_mpi_vq_reset(n);
	/* We do an extra flush before freeing memory,
	 * since jobs can re-queue themselves. */
	vhost_mpi_flush(n);
        vmpi_fini(n->mpi);
	kfree(n->dev.vqs);
	kfree(n);

        printk("vhost_mpi_release completed\n");

	return 0;
}

static long vhost_mpi_startstop(struct vmpi_impl_info *n, unsigned index, unsigned enable)
{
	struct vhost_virtqueue *vq;
	struct vmpi_impl_queue *nvq;
        struct file *file, *oldfile;
	int r;

        printk("vhost_mpi_startstop idx = %d enable = %d\n", index, enable);

	mutex_lock(&n->dev.mutex);
	r = vhost_dev_check_owner(&n->dev);
	if (r)
		goto err;

	if (index >= VHOST_NET_VQ_MAX) {
		r = -ENOBUFS;
		goto err;
	}
	vq = &n->vqs[index].vq;
	nvq = &n->vqs[index];
	mutex_lock(&vq->mutex);

	/* Verify that ring has been setup correctly. */
	if (!vhost_vq_access_ok(vq)) {
		r = -EFAULT;
		goto err_vq;
	}

        if (enable) {
	    file = n->file;
        } else {
            file = NULL;
        }

	/* start polling new socket */
	oldfile = vq->private_data;
	if (file != oldfile) {
		vhost_mpi_disable_vq(n, vq);
		vq->private_data = file;
		r = vhost_init_used(vq);
		if (r)
			goto err_used;
		r = vhost_mpi_enable_vq(n, vq);
		if (r)
			goto err_used;
	}

	mutex_unlock(&vq->mutex);

	if (oldfile) {
		vhost_mpi_flush_vq(n, index);
	}

	mutex_unlock(&n->dev.mutex);
	return 0;

err_used:
	vq->private_data = oldfile;
	vhost_mpi_enable_vq(n, vq);
err_vq:
	mutex_unlock(&vq->mutex);
err:
	mutex_unlock(&n->dev.mutex);
	return r;
}

static long vhost_mpi_reset_owner(struct vmpi_impl_info *n)
{
	long err;
	struct vhost_memory *memory;

	mutex_lock(&n->dev.mutex);
	err = vhost_dev_check_owner(&n->dev);
	if (err)
		goto done;
	memory = vhost_dev_reset_owner_prepare();
	if (!memory) {
		err = -ENOMEM;
		goto done;
	}
	vhost_mpi_stop(n);
	vhost_mpi_flush(n);
	vhost_dev_reset_owner(&n->dev, memory);
	vhost_mpi_vq_reset(n);
done:
	mutex_unlock(&n->dev.mutex);
	return err;
}

static int vhost_mpi_set_features(struct vmpi_impl_info *n, u64 features)
{
	mutex_lock(&n->dev.mutex);
	if ((features & (1 << VHOST_F_LOG_ALL)) &&
	    !vhost_log_access_ok(&n->dev)) {
		mutex_unlock(&n->dev.mutex);
		return -EFAULT;
	}
	n->dev.acked_features = features;
	smp_wmb();
	vhost_mpi_flush(n);
	mutex_unlock(&n->dev.mutex);
	return 0;
}

static long vhost_mpi_set_owner(struct vmpi_impl_info *n)
{
	int r;

	mutex_lock(&n->dev.mutex);
	if (vhost_dev_has_owner(&n->dev)) {
		r = -EBUSY;
		goto out;
	}
	r = vhost_dev_set_owner(&n->dev);
	vhost_mpi_flush(n);
out:
	mutex_unlock(&n->dev.mutex);
	return r;
}

struct vhost_mpi_command {
    unsigned int index;
    unsigned int enable;
};

/* Name overloading. Should be a temporary solution. */
#define VHOST_MPI_STARTSTOP VHOST_NET_SET_BACKEND

static long vhost_mpi_ioctl(struct file *f, unsigned int ioctl,
			    unsigned long arg)
{
	struct vmpi_impl_info *n = f->private_data;
	void __user *argp = (void __user *)arg;
	u64 __user *featurep = argp;
	struct vhost_mpi_command mpi_cmd;
	u64 features;
	int r;

	switch (ioctl) {
	case VHOST_MPI_STARTSTOP:
		if (copy_from_user(&mpi_cmd, argp, sizeof mpi_cmd))
			return -EFAULT;
		return vhost_mpi_startstop(n, mpi_cmd.index, mpi_cmd.enable);
	case VHOST_GET_FEATURES:
		features = VHOST_MPI_FEATURES;
		if (copy_to_user(featurep, &features, sizeof features))
			return -EFAULT;
		return 0;
	case VHOST_SET_FEATURES:
		if (copy_from_user(&features, featurep, sizeof features))
			return -EFAULT;
		if (features & ~VHOST_MPI_FEATURES)
			return -EOPNOTSUPP;
		return vhost_mpi_set_features(n, features);
	case VHOST_RESET_OWNER:
		return vhost_mpi_reset_owner(n);
	case VHOST_SET_OWNER:
		return vhost_mpi_set_owner(n);
	default:
		mutex_lock(&n->dev.mutex);
		r = vhost_dev_ioctl(&n->dev, ioctl, argp);
		if (r == -ENOIOCTLCMD)
			r = vhost_vring_ioctl(&n->dev, ioctl, argp);
		else
			vhost_mpi_flush(n);
		mutex_unlock(&n->dev.mutex);
		return r;
	}
}

#ifdef CONFIG_COMPAT
static long vhost_mpi_compat_ioctl(struct file *f, unsigned int ioctl,
				   unsigned long arg)
{
	return vhost_mpi_ioctl(f, ioctl, (unsigned long)compat_ptr(arg));
}
#endif

static unsigned int vhost_mpi_poll(struct file *file, poll_table *wait)
{
    struct vmpi_impl_info *n = file->private_data;
    struct vmpi_ring *write = n->write;
    struct vmpi_ring *read = n->read;
    unsigned int mask = 0;

    if (!n)
        return POLLERR;

    poll_wait(file, &n->wqh_poll, wait);

    mutex_lock(&write->lock);
    if (vmpi_ring_pending(write))
        mask |= POLLIN | POLLRDNORM;
    mutex_unlock(&write->lock);

    mutex_lock(&read->lock);
    if (vmpi_ring_pending(read))
        mask |= POLLOUT | POLLWRNORM;
    mutex_unlock(&read->lock);

    return mask;
}

int vmpi_impl_write_buf(struct vmpi_impl_info *vi,
                        struct vmpi_buffer *buf)
{
    wake_up_interruptible_poll(&vi->wqh_poll, POLLIN |
                               POLLRDNORM | POLLRDBAND);

    return 0;

}

int vmpi_impl_give_rx_buf(struct vmpi_impl_info *vi, struct vmpi_buffer *buf)
{
    wake_up_interruptible_poll(&vi->wqh_poll, POLLOUT |
                               POLLWRNORM | POLLWRBAND);

    return 0;
}

#ifdef VMPI_HOST_TEST
vmpi_info_t *vmpi_info_from_vmpi_impl_info(struct vmpi_impl_info *vi)
{
    return vi->mpi;
}
#endif  /* VMPI_HOST_TEST */

static const struct file_operations vhost_mpi_fops = {
	.owner          = THIS_MODULE,
	.release        = vhost_mpi_release,
	.unlocked_ioctl = vhost_mpi_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl   = vhost_mpi_compat_ioctl,
#endif
        .poll           = vhost_mpi_poll,
	.open           = vhost_mpi_open,
#ifdef VMPI_HOST_TEST
        .write          = do_sync_write,
        .aio_write      = vhost_mpi_aio_write,
        .read           = do_sync_read,
        .aio_read       = vhost_mpi_aio_read,
#endif  /* VMPI_HOST_TEST */
	.llseek		= noop_llseek,
};

#define VHOST_MPI_MINOR     245

static struct miscdevice vhost_mpi_misc = {
	.minor = VHOST_MPI_MINOR,
	.name = "vhost-mpi",
	.fops = &vhost_mpi_fops,
};

static int vhost_mpi_init(void)
{
	return misc_register(&vhost_mpi_misc);
}
module_init(vhost_mpi_init);

static void vhost_mpi_exit(void)
{
	misc_deregister(&vhost_mpi_misc);
}
module_exit(vhost_mpi_exit);

MODULE_VERSION("0.0.1");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Vincenzo Maffione");
MODULE_DESCRIPTION("Host kernel server for virtio mpi");
MODULE_ALIAS_MISCDEV(VHOST_NET_MINOR);
MODULE_ALIAS("devname:vhost-mpi");
