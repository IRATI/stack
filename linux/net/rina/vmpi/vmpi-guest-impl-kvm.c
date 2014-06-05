/* A vmpi-impl implementation for virtio.
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

//#define DEBUG
#include <linux/netdevice.h>
#include <linux/module.h>
#include <linux/virtio.h>
#include <linux/virtio_net.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/aio.h>
#include <linux/poll.h>

#include "vmpi-guest-impl.h"
#include "vmpi-structs.h"
#include "vmpi.h"



struct vmpi_impl_queue {
        struct virtqueue *vq;
        vmpi_impl_callback_t cb;
        struct scatterlist sg[MAX_SKB_FRAGS + 2];
        char name[40];
};

struct vmpi_impl_info {
        struct virtio_device *vdev;
        struct vmpi_impl_queue *sq;
        struct vmpi_impl_queue *rq;
        unsigned int status;
        void *private;
};

vmpi_info_t *
vmpi_info_from_vmpi_impl_info(vmpi_impl_info_t *vi)
{
        return vi->private;
}

static void
virtio_mpi_xmit_callback(struct virtqueue *vq)
{
        struct vmpi_impl_info *vi = vq->vdev->priv;

        /*
         * Callback is NULL in the interval between vmpi-guest deregistering
         * callbacks and the virtiqueue being removed by this driver.
         */
        if (likely(vi->sq->cb)) {
                vi->sq->cb(vi);
        }
}

static void
virtio_mpi_recv_callback(struct virtqueue *vq)
{
        struct vmpi_impl_info *vi = vq->vdev->priv;

        /*
         * Callback is NULL in the interval between vmpi-guest deregistering
         * callbacks and the virtiqueue being removed by this driver.
         */
        if (likely(vi->rq->cb)) {
                vi->rq->cb(vi);
        }
}

int
vmpi_impl_write_buf(struct vmpi_impl_info *vi, struct vmpi_buffer *buf)
{
        ssize_t ret = 0;
        struct vmpi_impl_queue *q = vi->sq;

        sg_set_buf(q->sg, buf->p, buf->len);
        ret = virtqueue_add_outbuf(q->vq, q->sg, 1, buf, GFP_KERNEL);

        return ret;
}

void
vmpi_impl_txkick(struct vmpi_impl_info *vi)
{
        virtqueue_kick(vi->sq->vq);
}

void
vmpi_impl_rxkick(struct vmpi_impl_info *vi)
{
        virtqueue_kick(vi->rq->vq);
}

struct vmpi_buffer *
vmpi_impl_get_written_buffer(struct vmpi_impl_info *vi)
{
        unsigned int len;
        struct vmpi_buffer *buf = virtqueue_get_buf(vi->sq->vq, &len);

        if (buf) {
                buf->len = len;
        }

        return buf;
}

struct vmpi_buffer *
vmpi_impl_read_buffer(struct vmpi_impl_info *vi)
{
        unsigned int len;
        struct vmpi_impl_queue *q = vi->rq;
        struct vmpi_buffer *buf = virtqueue_get_buf(q->vq, &len);
        struct vmpi_buffer *newbuf;
        int err;

        if (buf) {
                buf->len = len;
                newbuf = vmpi_buffer_create(VMPI_BUF_SIZE);
                if (unlikely(newbuf == NULL)) {
                        printk("Error: vmpi_buffer_create(%u) failed\n",
                               VMPI_BUF_SIZE);
                } else {
                        sg_set_buf(q->sg, newbuf->p, newbuf->size);
                        err = virtqueue_add_inbuf(q->vq, q->sg, 1, newbuf,
                                                  GFP_ATOMIC);
                        if (unlikely(err)) {
                                printk("Error: virtqueue_add_inbuf() failed\n");
                        }
                        virtqueue_kick(q->vq);
                }
        }

        return buf;
}

static inline bool
vmpi_impl_xx_cb(struct vmpi_impl_queue *q, int enable)
{
        if (enable) {
                return virtqueue_enable_cb(q->vq);
        }
        virtqueue_disable_cb(q->vq);

        return true;
}

bool
vmpi_impl_send_cb(struct vmpi_impl_info *vi, int enable)
{
        return vmpi_impl_xx_cb(vi->sq, enable);
}

bool
vmpi_impl_receive_cb(struct vmpi_impl_info *vi, int enable)
{
        return vmpi_impl_xx_cb(vi->rq, enable);
}

void
vmpi_impl_callbacks_register(struct vmpi_impl_info *vi,
                             vmpi_impl_callback_t xmit,
                             vmpi_impl_callback_t recv)
{
        vi->sq->cb = xmit;
        vi->rq->cb = recv;
}

void
vmpi_impl_callbacks_unregister(struct vmpi_impl_info *vi)
{
        vi->sq->cb = NULL;
        vi->rq->cb = NULL;
}

static int
virtio_mpi_find_vqs(struct vmpi_impl_info *vi)
{
        vq_callback_t **callbacks;
        struct virtqueue **vqs;
        int ret = -ENOMEM;
        int total_vqs;
        const char **names;

        /* We expect 1 RX virtqueue followed by 1 TX virtqueue, followed by
         * possible N-1 RX/TX queue pairs used in multiqueue mode, followed by
         * possible control vq.
         */
        total_vqs = 2;

        /* Allocate space for find_vqs parameters */
        vqs = kzalloc(total_vqs * sizeof(*vqs), GFP_KERNEL);
        if (!vqs)
                goto err_vq;
        callbacks = kmalloc(total_vqs * sizeof(*callbacks), GFP_KERNEL);
        if (!callbacks)
                goto err_callback;
        names = kmalloc(total_vqs * sizeof(*names), GFP_KERNEL);
        if (!names)
                goto err_names;

        /* Allocate/initialize parameters for send/receive virtqueues */
        callbacks[0] = virtio_mpi_recv_callback;
        callbacks[1] = virtio_mpi_xmit_callback;
        sprintf(vi->rq->name, "input.%d", 0);
        sprintf(vi->sq->name, "output.%d", 0);
        names[0] = vi->rq->name;
        names[1] = vi->sq->name;

        ret = vi->vdev->config->find_vqs(vi->vdev, total_vqs, vqs, callbacks,
                                         names);
        if (ret)
                goto err_find;

        vi->rq->vq = vqs[0];
        vi->sq->vq = vqs[1];

        kfree(names);
        kfree(callbacks);
        kfree(vqs);

        return 0;

 err_find:
        kfree(names);
 err_names:
        kfree(callbacks);
 err_callback:
        kfree(vqs);
 err_vq:
        return ret;
}

static void
virtio_mpi_config_changed(struct virtio_device *vdev)
{
        struct vmpi_impl_info *vi = vdev->priv;

        printk("virtio_mpi_config_changed %p\n", vi);
}


static int
virtio_mpi_alloc_queues(struct vmpi_impl_info *vi)
{
        vi->sq = kzalloc(sizeof(*vi->sq) * 1, GFP_KERNEL);
        if (!vi->sq)
                goto err_sq;

        vi->rq = kzalloc(sizeof(*vi->rq) * 1, GFP_KERNEL);
        if (!vi->rq)
                goto err_rq;

        sg_init_table(vi->rq->sg, ARRAY_SIZE(vi->rq->sg));
        sg_init_table(vi->sq->sg, ARRAY_SIZE(vi->sq->sg));

        return 0;

 err_rq:
        kfree(vi->sq);
 err_sq:
        return -ENOMEM;
}

static void
virtio_mpi_free_queues(struct vmpi_impl_info *vi)
{
        kfree(vi->rq);
        kfree(vi->sq);
}

static void
vmpi_impl_free_unused_bufs(struct vmpi_impl_info *vi)
{
        void *buf;
        struct virtqueue *vq;
        int cnt;

        cnt = 0;
        vq = vi->sq->vq;
        while ((buf = virtqueue_detach_unused_buf(vq)) != NULL) {
                (void)buf;
                cnt++;
        }

        cnt = 0;
        vq = vi->rq->vq;
        while ((buf = virtqueue_detach_unused_buf(vq)) != NULL) {
                vmpi_buffer_destroy(buf);
                cnt++;
                //--vi->rq[0].num;
        }
        //BUG_ON(vi->rq[0].num != 0);
}

static int
init_vqs(struct vmpi_impl_info *vi)
{
        int ret;

        /* Allocate send & receive queues */
        ret = virtio_mpi_alloc_queues(vi);
        if (ret)
                goto err;

        ret = virtio_mpi_find_vqs(vi);
        if (ret)
                goto err_free;

        return 0;

 err_free:
        virtio_mpi_free_queues(vi);
 err:
        return ret;
}

static void
virtio_mpi_del_vqs(struct vmpi_impl_info *vi)
{
        struct virtio_device *vdev = vi->vdev;

        vdev->config->del_vqs(vdev);

        virtio_mpi_free_queues(vi);
}

static void
remove_vq_common(struct vmpi_impl_info *vi)
{
        vi->vdev->config->reset(vi->vdev);

        /* Free unused buffers in both send and recv, if any. */
        vmpi_impl_free_unused_bufs(vi);

        virtio_mpi_del_vqs(vi);
}

#ifdef CONFIG_PM_SLEEP
static int
virtio_mpi_freeze(struct virtio_device *vdev)
{
        struct vmpi_impl_info *vi = vdev->priv;

        remove_vq_common(vi);

        return 0;
}

static int
virtio_mpi_restore(struct virtio_device *vdev)
{
        struct vmpi_impl_info *vi = vdev->priv;
        int err;

        err = init_vqs(vi);
        if (err)
                return err;

        return 0;
}
#endif

static int
virtio_mpi_probe(struct virtio_device *vdev)
{
        int err;
        struct vmpi_impl_info *vi;
        unsigned int i;

        err = -ENOMEM;

        vi = kmalloc(sizeof(*vi), GFP_KERNEL);
        if (!vi) {
                goto alloc;
        }

        vi->vdev = vdev;
        vdev->priv = vi;

        /* Use single tx/rx queue pair as default */

        /* Allocate/initialize the rx/tx queues, and invoke find_vqs */
        err = init_vqs(vi);
        if (err)
                goto init_vqs;

        vi->status = 1;

        vi->private = vmpi_init(vi, &err, false);
        if (vi->private == NULL) {
                printk("vmpi_init() failed\n");
                goto vmpi_ini;
        }

        /* Setup some receive buffers. */
        for (i = 0; i < VMPI_RING_SIZE; i++) {
                struct vmpi_impl_queue *q = vi->rq;
                struct vmpi_buffer *buf;
                int err;

                buf = vmpi_buffer_create(VMPI_BUF_SIZE);
                if (buf == NULL) {
                        goto setup_rxbufs;
                }
                sg_set_buf(q->sg, buf->p, buf->size);
                err = virtqueue_add_inbuf(q->vq, q->sg, 1, buf, GFP_ATOMIC);
                if (err) {
                        vmpi_buffer_destroy(buf);
                        goto setup_rxbufs;
                }
        }
        virtqueue_kick(vi->rq->vq);

        printk("virtio_mpi_probe completed\n");

        return 0;

 setup_rxbufs:
        vmpi_impl_free_unused_bufs(vi);
        vmpi_fini(false);
 vmpi_ini:
        virtio_mpi_del_vqs(vi);
 init_vqs:
        kfree(vi);
 alloc:
        return err;
}

static void
virtio_mpi_remove(struct virtio_device *vdev)
{
        struct vmpi_impl_info *vi = vdev->priv;

        vmpi_fini(false);

        remove_vq_common(vi);
        kfree(vi);

        printk("virtio_mpi_remove completed\n");
}

#define VIRTIO_ID_MPI   15

static struct virtio_device_id id_table[] = {
        { VIRTIO_ID_MPI, VIRTIO_DEV_ANY_ID },
        { 0 },
};

static unsigned int features[] = {
        VIRTIO_F_ANY_LAYOUT,
};

static struct virtio_driver virtio_mpi_driver = {
        .feature_table = features,
        .feature_table_size = ARRAY_SIZE(features),
        .driver.name =  KBUILD_MODNAME,
        .driver.owner = THIS_MODULE,
        .id_table =     id_table,
        .probe =        virtio_mpi_probe,
        .remove =       virtio_mpi_remove,
        .config_changed = virtio_mpi_config_changed,
#ifdef CONFIG_PM_SLEEP
        .freeze =       virtio_mpi_freeze,
        .restore =      virtio_mpi_restore,
#endif
};

module_virtio_driver(virtio_mpi_driver);

MODULE_DEVICE_TABLE(virtio, id_table);
MODULE_DESCRIPTION("Virtio MPI driver");
MODULE_ALIAS_MISCDEV(VIRTIO_MPI_MINOR);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vincenzo Maffione");
