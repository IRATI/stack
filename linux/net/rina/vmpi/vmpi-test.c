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
#include <linux/aio.h>
#include <linux/miscdevice.h>
#include <linux/poll.h>
#include <linux/moduleparam.h>

#include "vmpi.h"


static unsigned int write_channel = 0;
module_param(write_channel, uint, 0644);
static unsigned int read_channel = 0;
module_param(read_channel, uint, 0644);

static ssize_t
vmpi_test_aio_write(struct kiocb *iocb, const struct iovec *iv,
                    unsigned long iovlen, loff_t pos)
{
        struct file *file = iocb->ki_filp;
        vmpi_info_t *mpi = file->private_data;

        /* XXX file->f_flags & O_NONBLOCK */

        return vmpi_write(mpi, write_channel, iv, iovlen);
}

static ssize_t
vmpi_test_aio_read(struct kiocb *iocb, const struct iovec *iv,
                   unsigned long iovcnt, loff_t pos)
{
        struct file *file = iocb->ki_filp;
        vmpi_info_t *mpi = file->private_data;
        ssize_t ret;

        ret = vmpi_read(mpi, read_channel, iv, iovcnt);

        if (ret > 0)
                iocb->ki_pos = ret;

        return ret;
}

static vmpi_info_t *instance = NULL;

static int
vmpi_test_open(struct inode *inode, struct file *f)
{
        if (instance == NULL) {
                /* Not yet ready.. this should not happen! */
                printk("vmpi_test_open: not ready\n");
                BUG_ON(1);
                return -ENXIO;
        }

        f->private_data = instance;

        printk("vmpi_test_open completed\n");

        return 0;
}

static int
vmpi_test_release(struct inode *inode, struct file *f)
{
        f->private_data = NULL;

        printk("vmpi_test_close completed\n");

        return 0;
}

static const struct file_operations vmpi_test_fops = {
        .owner          = THIS_MODULE,
        .release        = vmpi_test_release,
        .open           = vmpi_test_open,
        .write          = do_sync_write,
        .aio_write      = vmpi_test_aio_write,
        .read           = do_sync_read,
        .aio_read       = vmpi_test_aio_read,
        .llseek         = noop_llseek,
};

#define VMPI_TEST_MINOR     246

static struct miscdevice vmpi_test_misc = {
        .minor = VMPI_TEST_MINOR,
        .name = "vmpi-test",
        .fops = &vmpi_test_fops,
};

static int
__vmpi_test_init(void)
{
        int ret;

        ret = misc_register(&vmpi_test_misc);
        if (ret) {
                printk("Failed to register vmpi-test misc device\n");
                return ret;
        }

        printk("vmpi_test_init completed\n");

        return 0;
}

static void
vmpi_test_init_work(struct work_struct *unused)
{
        __vmpi_test_init();
}

static void
vmpi_test_fini_work(struct work_struct *unused)
{
        misc_deregister(&vmpi_test_misc);
        instance = NULL;

        printk("vmpi_test_fini completed\n");
}

static DECLARE_WORK(init_work, vmpi_test_init_work);
static DECLARE_WORK(fini_work, vmpi_test_fini_work);

int
vmpi_test_init(void *opaque, bool deferred)
{
        instance = opaque;

        if (!deferred) {
                return __vmpi_test_init();
        }

        /* Call __vmpi_test_init() in another process context (kworker). */
        schedule_work(&init_work);

        return 0;
}

void
vmpi_test_fini(bool deferred)
{
        if (deferred) {
                return vmpi_test_fini_work(&fini_work);
        }

        /* Call vmpi_test_fini_work() in another process context (kworker). */
        schedule_work(&fini_work);
}
