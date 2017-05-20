/*
 * Test interface for VMPI
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
#include <linux/miscdevice.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/moduleparam.h>
#include <linux/uio.h>
#include <linux/slab.h>

#include "vmpi.h"
#include "vmpi-bufs.h"


#define VMPI_MAX_CHANNELS       64

static unsigned int vmpi_id = 0;
module_param(vmpi_id, uint, 0644);

static unsigned int write_channel = 0;
module_param(write_channel, uint, 0644);

static unsigned int read_channel = 0;
module_param(read_channel, uint, 0644);

struct test_queue {
        struct list_head        entries;
        unsigned int            len;
        spinlock_t              lock;
};

struct vmpi_test {
        unsigned int vmpi_id;
        struct vmpi_ops vmpi_ops;
        struct test_queue readqueues[VMPI_MAX_CHANNELS];
        wait_queue_head_t read_wqh;
        wait_queue_head_t write_wqh;
};

static ssize_t
vmpi_test_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
        struct file *file = iocb->ki_filp;
        struct vmpi_test *vt = file->private_data;
        size_t len = iov_iter_count(from);
        DECLARE_WAITQUEUE(wait, current);
        int ret;

        struct vmpi_buf *vb;

        IFV(printk("%s(%d)\n", __func__, (int)len));

        if (unlikely(!len)) {
                return 0;
        }

        vb = vmpi_buf_alloc(len, 0, GFP_KERNEL);
        if (!vb) {
                printk("%s: Out of memory\n", __func__);
                return -ENOMEM;
        }
        copy_from_iter(vmpi_buf_data(vb), vmpi_buf_size(vb), from);

        add_wait_queue(&vt->write_wqh, &wait);

        for (;;) {
                current->state = TASK_INTERRUPTIBLE;

                ret = vt->vmpi_ops.write(&vt->vmpi_ops, write_channel, vb);
                if (unlikely(ret == -EAGAIN)) {
                        if (signal_pending(current)) {
                                ret = -ERESTARTSYS;
                                break;
                        }

                        /* No room to write, let's sleep. */
                        schedule();
                        continue;
                }

                break;
        }

        current->state = TASK_RUNNING;
        remove_wait_queue(&vt->write_wqh, &wait);

        IFV(printk("%s() --> %d\n", __func__, (int)len));

        return len;
}

static void
test_write_restart_callback(void *opaque)
{
        struct vmpi_test *vt = opaque;

        wake_up_interruptible_poll(&vt->write_wqh, POLLOUT |
                                   POLLWRNORM | POLLWRBAND);
}

static void
test_read_callback(void *opaque, unsigned int channel,
                   struct vmpi_buf *vb)
{
        struct vmpi_test *vt = opaque;
        struct test_queue *queue;
	struct vmpi_buf_node *vbn;

        if (unlikely(channel >= VMPI_MAX_CHANNELS)) {
                printk("WARNING: bogus channel index %u\n", channel);
                channel = 0;
        }

        queue = &vt->readqueues[channel];
        spin_lock(&queue->lock);
        if (likely(queue->len < 512)) {
		vbn = vmpi_buf_node_alloc(vb, GFP_ATOMIC);
		if (!vbn) {
			spin_unlock(&queue->lock);
			printk("ERROR: unable to allocate vmpi_buf_node\n");
			return;
		}
                list_add_tail(&vbn->node, &queue->entries);
                queue->len++;
        } else {
                vmpi_buf_free(vb);
        }
        spin_unlock(&queue->lock);

        wake_up_interruptible_poll(&vt->read_wqh, POLLIN |
                                   POLLRDNORM | POLLRDBAND);
}

static ssize_t
vmpi_test_read_iter(struct kiocb *iocb,
                    struct iov_iter *to)
{
        struct file *file = iocb->ki_filp;
        struct vmpi_test *vt = file->private_data;
        ssize_t size = iov_iter_count(to);
        DECLARE_WAITQUEUE(wait, current);
        struct test_queue *queue;
        ssize_t ret = 0;

        IFV(printk("%s(%d)\n", __func__, (int)size));

        if (unlikely(read_channel >= VMPI_MAX_CHANNELS)) {
                return -EINVAL;
        }
        queue = &vt->readqueues[read_channel];

        add_wait_queue(&vt->read_wqh, &wait);

        while (size) {
                ssize_t copylen;
                struct vmpi_buf *vb;
                struct vmpi_buf_node *vbn;

                current->state = TASK_INTERRUPTIBLE;

                spin_lock(&queue->lock);
                if (queue->len == 0) {
                        spin_unlock(&queue->lock);
                        if (signal_pending(current)) {
                                ret = -ERESTARTSYS;
                                break;
                        }

                        /* Nothing to read, let's sleep */
                        schedule();
                        continue;
                }

                vbn = list_first_entry(&queue->entries,
                                      struct vmpi_buf, node);
                list_del(&vbn->node);
                queue->len--;
		vb = vbn->vb;
		vmpi_buf_node_free(vbn);
                spin_unlock(&queue->lock);

                copylen = vmpi_buf_len(vb);
                if (size < copylen) {
                        copylen = size;
                }
                copylen = copy_to_iter(vmpi_buf_data(vb), copylen, to);
                ret = copylen;

                vmpi_buf_free(vb);

                break;
        }

        current->state = TASK_RUNNING;
        remove_wait_queue(&vt->read_wqh, &wait);

        IFV(printk("%s() --> %d\n", __func__, (int)ret));

        if (ret > 0)
                iocb->ki_pos = ret;

        return ret;
}

static int
vmpi_test_open(struct inode *inode, struct file *f)
{
        unsigned int provider = VMPI_PROVIDER_AUTO;
        struct vmpi_test *vt;
        int ret;
        int i;

        vt = kzalloc(sizeof(*vt), GFP_KERNEL);
        if (!vt) {
                return -ENOMEM;
        }

        /* Sample the vmpi_id */
        vt->vmpi_id = vmpi_id;

        /* Find the VMPI provider. */
        ret = vmpi_provider_find_instance(provider, vt->vmpi_id,
                                          &vt->vmpi_ops);
        if (ret) {
                printk("vmpi_provider_find(%u, %u) failed\n", provider,
                       vt->vmpi_id);
                goto fail;
        }

        /* Initialize receive and send side. */
        ret = vt->vmpi_ops.register_cbs(&vt->vmpi_ops, test_read_callback,
                                        test_write_restart_callback, vt);
        if (ret) {
                printk("register_cbs failed [%d]\n", ret);
                goto fail;
        }

        for (i = 0; i < VMPI_MAX_CHANNELS; i++) {
                INIT_LIST_HEAD(&vt->readqueues[i].entries);
                vt->readqueues[i].len = 0;
                spin_lock_init(&vt->readqueues[i].lock);
        }
        init_waitqueue_head(&vt->read_wqh);
        init_waitqueue_head(&vt->write_wqh);

        BUG_ON(!vt->vmpi_ops.write);

        f->private_data = vt;

        printk("vmpi_test_open completed\n");

        return 0;
fail:
        kfree(vt);

        return ret;
}

static int
vmpi_test_release(struct inode *inode, struct file *f)
{
        struct vmpi_test *vt = f->private_data;
        int i;

        vt->vmpi_ops.unregister_cbs(&vt->vmpi_ops);

        /* Clean up queues. */
        for (i = 0; i < VMPI_MAX_CHANNELS; i++) {
                struct vmpi_buf *vb, *tmp;

                list_for_each_entry_safe(vb, tmp, &vt->readqueues[i].entries,
                                         node) {
                        list_del(&vb->node);
                        vmpi_buf_free(vb);
                }
        }

        f->private_data = NULL;

        kfree(vt);

        printk("vmpi_test_close completed\n");

        return 0;
}

static const struct file_operations vmpi_test_fops = {
        .owner          = THIS_MODULE,
        .release        = vmpi_test_release,
        .open           = vmpi_test_open,
        .write_iter     = vmpi_test_write_iter,
        .read_iter      = vmpi_test_read_iter,
        .llseek         = noop_llseek,
};

#define VMPI_TEST_MINOR     246

static struct miscdevice vmpi_test_misc = {
        .minor = VMPI_TEST_MINOR,
        .name = "vmpi-test",
        .fops = &vmpi_test_fops,
};

static int __init
vmpi_test_init(void)
{
        int ret;

        /* Register the misc device. */
        ret = misc_register(&vmpi_test_misc);
        if (ret) {
                printk("Failed to register vmpi-test misc device\n");
                return ret;
        }

        printk("vmpi_test_init completed\n");

        return 0;
}

static void __exit
vmpi_test_fini(void)
{
        misc_deregister(&vmpi_test_misc);

        printk("vmpi_test_fini completed\n");
}

module_init(vmpi_test_init);
module_exit(vmpi_test_fini);
MODULE_LICENSE("GPL");
MODULE_ALIAS("devname: vmpi-test");
