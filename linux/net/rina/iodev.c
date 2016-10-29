/*
 * IRATI IO misc device
 *
 *    Vincenzo Maffionee <v.maffione@nextworks.it>
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

#include <linux/types.h>
#include <linux/module.h>
#include <linux/aio.h>
#include <linux/miscdevice.h>
#include <linux/poll.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/spinlock.h>

#define RINA_PREFIX "iodev"

#include "logs.h"
#include "debug.h"
#include "utils.h"
#include "common.h"

/* Private data to an iodev file instance. */
struct iodev_priv {
        port_id_t       port_id;
};

static ssize_t
iodev_write(struct file *f, const char __user *ubuf, size_t len, loff_t *ppos)
{
        return -ENXIO;
}

static ssize_t
iodev_read(struct file *f, char __user *buf, size_t len, loff_t *ppos)
{
        return -ENXIO;
}

static unsigned int
iodev_poll(struct file *f, poll_table *wait)
{
        unsigned int mask = 0;
        return mask;
}

static int
iodev_open(struct inode *inode, struct file *f)
{
        struct iodev_priv *priv;

        priv = rkzalloc(sizeof(*priv), GFP_KERNEL);
        if (!priv) {
                return -ENOMEM;
        }

        priv->port_id = port_id_bad();
        f->private_data = priv;

        return 0;
}

static int
iodev_release(struct inode *inode, struct file *f)
{
        struct iodev_priv *priv = f->private_data;

        rkfree(priv);

        return 0;
}

static long
iodev_ioctl(struct file *f, unsigned int cmd, unsigned long flags)
{
        return -EINVAL;
}

static const struct file_operations irati_fops = {
        .owner          = THIS_MODULE,
        .release        = iodev_release,
        .open           = iodev_open,
        .write          = iodev_write,
        .read           = iodev_read,
        .poll           = iodev_poll,
        .unlocked_ioctl = iodev_ioctl,
        .llseek         = noop_llseek,
};

static struct miscdevice irati_misc = {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "irati",
        .fops = &irati_fops,
};

int
iodev_init(void)
{
        int ret;

        ret = misc_register(&irati_misc);
        if (ret) {
                LOG_ERR("Failed to register rlite-io misc device");
                return ret;
        }

        return ret;
}

void
iodev_fini(void)
{
        misc_deregister(&irati_misc);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vincenzo Maffione <v.maffione@nextworks.it>");
