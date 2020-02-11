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
#include <linux/compat.h>

#define RINA_PREFIX "iodev"

#include "logs.h"
#include "debug.h"
#include "utils.h"
#include "common.h"
#include "kipcm.h"
#include "kfa.h"
#include "kfa-utils.h"
#include "ctrldev.h"
#include "irati/kernel-msg.h"

extern struct kipcm *default_kipcm;

/* Private data to an iodev file instance. */
struct iodev_priv {
        port_id_t       port_id;
        struct iowaitqs * wqs;
        spinlock_t 	flow_dealloc_lock;
        int		flow_dealloc;
};

static ssize_t iodev_write(struct file *f, const char __user *buffer, 
			   size_t size, loff_t *ppos)
{
        struct iodev_priv *priv = f->private_data;
        bool blocking = !(f->f_flags & O_NONBLOCK);
        ssize_t retval;

        LOG_DBG("Syscall write SDU (size = %zd, port-id = %d)",
                        size, priv->port_id);

        if (!buffer || !size) {
                return -EINVAL;
        }

        ASSERT(default_kipcm);
        retval = kipcm_du_write(default_kipcm, priv->port_id, buffer,
        			NULL, size, blocking);
        LOG_DBG("SDU write returned %zd", retval);

        return retval;
}

static ssize_t iodev_write_iter(struct kiocb * kcb, struct iov_iter * iov) 
{
	struct file * f = kcb->ki_filp;
	struct iodev_priv * priv = f->private_data;
	bool blocking = !(f->f_flags & O_NONBLOCK);
	size_t size = iov_iter_count(iov);
	ssize_t retval;

	LOG_DBG("Syscall writev SDU, port-id = %d", priv->port_id);

	if (!kcb || !iov) {
		return -EINVAL;
	}

	ASSERT(default_kipcm);
	retval = kipcm_du_write(default_kipcm, priv->port_id, NULL,
				iov, size, blocking);

	LOG_DBG("SDU write returned %zd", retval);

	return retval;
}

static ssize_t common_read(size_t size, bool blocking, port_id_t port_id, 
			   char __user * buffer, struct iov_iter * iov)
{
	struct du * tmp;
	ssize_t retval;
	bool partial_read;
	size_t retsize;
	unsigned char * data;

	tmp = NULL;

	ASSERT(default_kipcm);
	retval = kipcm_du_read(default_kipcm, 
			       port_id,
			       &tmp,
			       size,
			       blocking);

	/* Taking ownership from the internal layers */

	LOG_DBG("SDU read returned %zd", retval);

	if (retval <= 0) {
		return retval;
	}

	if (!is_du_ok(tmp)) {
		return -EIO;
	}

	retsize = retval;
	partial_read = retsize > size;
	data = du_buffer(tmp);
	if (partial_read) {
		retsize = size;
	}

	if (buffer) {
		if (copy_to_user(buffer, data, retsize)) {
			LOG_ERR("Error copying data to user space");
			du_destroy(tmp);
			return -EIO;
		}
	} else {
		if (copy_to_iter(data, retsize, iov) != retsize) {
			LOG_ERR("Error copying data to user space");
			du_destroy(tmp);
			return -EIO;
		}
	}

	if (partial_read) {
		du_consume_data(tmp, size);
	} else {
		du_destroy(tmp);
	}

	return retsize;
}

static ssize_t iodev_read(struct file *f, char __user *buffer, 
			  size_t size, loff_t *ppos)
{
        struct iodev_priv *priv = f->private_data;
        bool blocking = !(f->f_flags & O_NONBLOCK);

        LOG_DBG("Syscall read SDU (size = %zd, port-id = %d)",
                size, priv->port_id);
	
	return common_read(size, blocking, priv->port_id, buffer, NULL);
}

static ssize_t iodev_read_iter(struct kiocb * kio, struct iov_iter * iov)
{
	struct file * f = kio->ki_filp;
	struct iodev_priv * priv = f->private_data;
	size_t size = iov_iter_count(iov);
	bool blocking = !(f->f_flags & O_NONBLOCK);

	LOG_DBG("iov_read_iter called: %p, %p", kio, iov);

	return common_read(size, blocking, priv->port_id, NULL, iov);
}	

/* Conservative implementation: we always pretend to be ready.
 * This needs to be implemented properly once it is possible to
 * ask lower layers for the status of receive/send queues. */
static unsigned int iodev_poll(struct file *f, poll_table *wait)
{
        struct kfa *kfa = kipcm_kfa(default_kipcm);
        struct iodev_priv *priv = f->private_data;
        unsigned int mask = 0;
        int res;

        if (!is_port_id_ok(priv->port_id)) {
                return -ENXIO;
        }

        /* Set POLLIN if the receive queue is not empty or
         * the flow has been deallocated. Also call poll_wait(),
         * as required by the caller. */
        res = kfa_flow_readable(kfa, priv->port_id, &mask, f, wait);
        if (res == 0) {
        	/* If the flow is there, allow writes on the port-id */
        	mask |= POLLOUT | POLLWRNORM;
        }

        return mask;
}

static int iodev_open(struct inode *inode, struct file *f)
{
        struct iodev_priv *priv;

        priv = rkzalloc(sizeof(*priv), GFP_KERNEL);
        if (!priv) {
                return -ENOMEM;
        }

        priv->port_id = port_id_bad();
        priv->flow_dealloc = 0;
        spin_lock_init(&priv->flow_dealloc_lock);
        priv->wqs = rkzalloc(sizeof(struct iowaitqs), GFP_KERNEL);
        if (!priv->wqs) {
        	rkfree(priv);
        	return -ENOMEM;
        }

	init_waitqueue_head(&priv->wqs->read_wqueue);
	init_waitqueue_head(&priv->wqs->write_wqueue);

        f->private_data = priv;

        return 0;
}

static void deallocate_flow(struct iodev_priv * priv)
{
	struct kfa *kfa = kipcm_kfa(default_kipcm);
	struct irati_msg_app_dealloc_flow msg;

	spin_lock(&priv->flow_dealloc_lock);
	if (priv->flow_dealloc) {
		spin_unlock(&priv->flow_dealloc_lock);
		return;
	}

	priv->flow_dealloc = 1;
	spin_unlock(&priv->flow_dealloc_lock);

	/* Unbind waitqueues from flow */
	if (kfa_flow_cancel_iowqs(kfa, priv->port_id) == 0) {
		/* Request flow deallocation */
		msg.msg_type = RINA_C_APP_DEALLOCATE_FLOW_REQUEST;
		msg.port_id = priv->port_id;
		irati_ctrl_dev_snd_resp_msg(IPCM_CTRLDEV_PORT, IRATI_MB(&msg));

		LOG_DBG("Requested deallocation of flow with port-id %d", priv->port_id);
	}
}

static int iodev_release(struct inode *inode, struct file *f)
{
        struct iodev_priv *priv = f->private_data;

        LOG_DBG("I/O dev release called for port-id %d", priv->port_id);

        deallocate_flow(priv);

        rkfree(priv->wqs);
        rkfree(priv);

        return 0;
}

static int iodev_flush(struct file * f, fl_owner_t id) {
	struct iodev_priv *priv = f->private_data;

	LOG_DBG("I/O dev flush called for port-id %d", priv->port_id);

	/*deallocate_flow(priv);*/

	return 0;
}

static long iodev_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
        struct kfa *kfa = kipcm_kfa(default_kipcm);
        struct iodev_priv *priv = f->private_data;
        void __user *p = (void __user *)arg;
        struct irati_iodev_ctldata data;
        size_t max_sdu_size;

        switch(cmd) {

        case IRATI_FLOW_BIND: {
        	if (copy_from_user(&data, p, sizeof(data))) {
        		return -EFAULT;
        	}

        	if (!is_port_id_ok(data.port_id)) {
        		LOG_ERR("Bad port id %d", data.port_id);
        		return -EINVAL;
        	}

        	if (is_port_id_ok(priv->port_id)) {
        		LOG_ERR("Cannot bind to port %d, "
        				"already bound to port id %d",
					data.port_id, priv->port_id);
        		return -EBUSY;
        	}

        	if (kfa_flow_set_iowqs(kfa, priv->wqs, data.port_id) != 0) {
        		LOG_ERR("Error binding to port-id %d", data.port_id);
        		return -ENXIO;
        	}

        	priv->port_id = data.port_id;

        	LOG_DBG("Bound to port id %d", data.port_id);
        	break;
        }

        case IRATI_IOCTL_MSS_GET: {
        	struct irati_iodev_ctldata __user * mss_data =
        			(struct irati_iodev_ctldata __user *) p;

                max_sdu_size = kfa_flow_max_sdu_size(kfa, priv->port_id);
                if (max_sdu_size == 0) {
                	return -EFAULT;
                }

                data.port_id = max_sdu_size;
                if (put_user(data, mss_data)) {
                    return -EFAULT;
                }

        	break;
        }

        default:
        	LOG_ERR("Invalid cmd %u", cmd);
        	return -EINVAL;
        }

        return 0;
}

#ifdef CONFIG_COMPAT
static long iodev_compat_ioctl(struct file *f, unsigned int cmd, 
			       unsigned long arg)
{
	return iodev_ioctl(f, cmd, (unsigned long) compat_ptr(arg));
}
#endif

static const struct file_operations irati_fops = {
        .owner          = THIS_MODULE,
        .release        = iodev_release,
        .open           = iodev_open,
        .write          = iodev_write,
        .read           = iodev_read,
	.write_iter	= iodev_write_iter,
	.read_iter	= iodev_read_iter,
        .poll           = iodev_poll,
        .unlocked_ioctl = iodev_ioctl,
	.flush		= iodev_flush,
#ifdef CONFIG_COMPAT
	.compat_ioctl   = iodev_compat_ioctl,
#endif
        .llseek         = noop_llseek,
};

static struct miscdevice irati_misc = {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "irati",
        .fops = &irati_fops,
};

int iodev_init(void)
{
        int ret;

        ret = misc_register(&irati_misc);
        if (ret) {
                LOG_ERR("Failed to register i/o misc device");
                return ret;
        }

        return ret;
}

void iodev_fini(void)
{
        misc_deregister(&irati_misc);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vincenzo Maffione <v.maffione@nextworks.it>");
MODULE_AUTHOR("Eduard Grasa <eduard.grasa@i2cat.net>");
