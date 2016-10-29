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
#include "kipcm.h"
#include "kfa.h"
#include "kfa-utils.h"

extern struct kipcm *default_kipcm;

/* Private data to an iodev file instance. */
struct iodev_priv {
        port_id_t       port_id;
};

/* Data structure passed along with ioctl */
struct irati_iodev_ctldata {
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

        /* TODO possibly deallocate the flow */
        rkfree(priv);

        return 0;
}

static long
iodev_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
        struct kfa *kfa = kipcm_kfa(default_kipcm);
        struct iodev_priv *priv = f->private_data;
        void __user *p = (void __user *)arg;
        struct irati_iodev_ctldata data;

        if (copy_from_user(&data, p, sizeof(data))) {
                return -EFAULT;
        }

        if (!is_port_id_ok(data.port_id)) {
                LOG_ERR("Bad port id %d", data.port_id);
                return -EINVAL;
        }

        if (!kfa_flow_exists(kfa, data.port_id)) {
                LOG_ERR("Cannot find flow for port id %d", data.port_id);
                return -ENXIO;
        }

        if (is_port_id_ok(priv->port_id)) {
                LOG_ERR("Cannot bind to port %d, "
                        "already bound to port id %d",
                        data.port_id, priv->port_id);
                return -EBUSY;
        }

        priv->port_id = data.port_id;

        LOG_DBG("Going to bind to port id %d", data.port_id);

        return 0;
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
