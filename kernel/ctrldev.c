/*
 * IRATI control device
 *
 *    Eduard Grasa <eduard.grasa@i2cat.net>
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
#include <asm/compat.h>

#define RINA_PREFIX "ctrldev"

#include "logs.h"
#include "debug.h"
#include "utils.h"
#include "ctrldev.h"
#include "kipcm.h"
#include "rds/rfifo.h"
#include "irati/kernel-msg.h"
#include "irati/serdes-utils.h"

extern struct kipcm *default_kipcm;

/* Private data to an ctrldev file instance. */
struct ctrldev_priv {
	char 		   msgbuf[8192];
	irati_msg_port_t   port_id;
	struct rfifo      *pending_msgs;
	spinlock_t 	   pending_msgs_lock;
	struct list_head   node;        /* queue of ctrl device file descriptors */
	struct file 	  *file; 	/* backpointer */
	wait_queue_head_t  read_wqueue;
};

struct message_handler {
        void *             data;
        irati_msg_handler_t cb;
};

struct irati_ctrl_dm {
	/* List that contains all the IRATI ctrl devices that
	 * are currently opened. */
	struct list_head ctrl_devs;

	/* Array with message handlers */
	struct message_handler handlers[IRATI_RINA_C_MAX];

	/* Lock for ctrl_devs list */
	struct mutex general_lock;

	/* Sequence number counter */
	uint32_t sn_counter;

	/* Pointer to IPCM control device */
	struct ctrldev_priv * ipcm_ctrl_dev;
};

static struct irati_ctrl_dm irati_ctrl_dm;

struct msg_queue_entry {
	void   * sermsg;
	size_t   serlen;
};

int irati_handler_register(irati_msg_t msg_type,
		           irati_msg_handler_t handler,
			   void * data)
{
        LOG_DBG("Registering handler callback %pK "
                "for message type %d", handler, msg_type);

        if (!handler) {
                LOG_ERR("Handler for message type %d is empty, "
                        "use unregister to remove it", msg_type);
                return -1;
        }

        if (msg_type <= IRATI_RINA_C_MIN ||
        		msg_type >= IRATI_RINA_C_MAX) {
                LOG_ERR("Message type %d is out-of-range, "
                        "cannot register", msg_type);
                return -1;
        }

        mutex_lock(&irati_ctrl_dm.general_lock);
        if (irati_ctrl_dm.handlers[msg_type].cb) {
        	mutex_unlock(&irati_ctrl_dm.general_lock);
                LOG_ERR("There is a handler for message type %d still "
                        "registered, unregister it first",
                        msg_type);
                return -1;
        }

        irati_ctrl_dm.handlers[msg_type].cb = handler;
        irati_ctrl_dm.handlers[msg_type].data = data;

        mutex_unlock(&irati_ctrl_dm.general_lock);

        LOG_DBG("Handler %pK registered for message type %d",
                handler, msg_type);

        return 0;
}
EXPORT_SYMBOL(irati_handler_register);

int irati_handler_unregister(irati_msg_t msg_type)
{
        LOG_DBG("Unregistering handler for message type %d", msg_type);

        if (msg_type <= IRATI_RINA_C_MIN ||
        		msg_type >= IRATI_RINA_C_MAX) {
                LOG_ERR("Message type %d is out-of-range, "
                        "cannot register", msg_type);
                return -1;
        }

        mutex_lock(&irati_ctrl_dm.general_lock);
        bzero(&irati_ctrl_dm.handlers[msg_type],
              sizeof(irati_ctrl_dm.handlers[msg_type]));
        mutex_unlock(&irati_ctrl_dm.general_lock);

        LOG_DBG("Handler for message type %d unregistered successfully",
                msg_type);

        return 0;
}
EXPORT_SYMBOL(irati_handler_unregister);

void irati_ctrl_dev_msg_free(struct irati_msg_base *bmsg)
{
	return irati_msg_free(irati_ker_numtables, IRATI_RINA_C_MAX, bmsg);
}
EXPORT_SYMBOL(irati_ctrl_dev_msg_free);

int irati_ctrl_dev_snd_resp_msg(struct ctrldev_priv *ctrl_dev,
				struct irati_msg_base *bmsg)
{
	struct msg_queue_entry * entry;
	int retval = 0;

	//Serialize message
	entry = rkzalloc(sizeof(*entry), GFP_KERNEL);
	if (!entry) {
		LOG_ERR("Could not create entry");
		return -1;
	}

	retval = serialize_irati_msg(irati_ker_numtables, IRATI_RINA_C_MAX,
                        	     entry->sermsg, bmsg);
        if (retval < 0) {
        	LOG_ERR("Problems serializing msg: %d", retval);
        	rkfree(entry);
        }
        entry->serlen = retval;

        /* TODO implement maximum queue size */
        spin_lock(&ctrl_dev->pending_msgs_lock);
        if (rfifo_push_ni(ctrl_dev->pending_msgs, entry)) {
        	LOG_ERR("Could not write %zd bytes into port-id %u fifo",
        			sizeof(*entry), ctrl_dev->port_id);
        	retval = -1;
        }
        spin_unlock(&ctrl_dev->pending_msgs_lock);

        if (retval == 0) {
		/* set_tsk_need_resched(current); */
		wake_up_interruptible_poll(&ctrl_dev->read_wqueue,
					   POLLIN | POLLRDNORM | POLLRDBAND);
        }

        return retval;
}
EXPORT_SYMBOL(irati_ctrl_dev_snd_resp_msg);

uint32_t ctrl_dev_get_next_seqn()
{
        uint32_t tmp;

        mutex_lock(&irati_ctrl_dm.general_lock);

        tmp = irati_ctrl_dm.sn_counter++;
        if (irati_ctrl_dm.sn_counter == 0) {
                LOG_WARN("RNL Sequence number rolled-over");
                /* FIXME: What to do about roll-over? */
        }

        mutex_unlock(&irati_ctrl_dm.general_lock);

        return tmp;
}
EXPORT_SYMBOL(ctrl_dev_get_next_seqn);

void set_ipcm_ctrl_dev(struct ctrldev_priv *ctrl_dev)
{
	 mutex_lock(&irati_ctrl_dm.general_lock);
	 irati_ctrl_dm.ipcm_ctrl_dev = ctrl_dev;
	 mutex_unlock(&irati_ctrl_dm.general_lock);
}
EXPORT_SYMBOL(set_ipcm_ctrl_dev);

struct ctrldev_priv * get_ipcm_ctrl_dev(void)
{
	struct ctrldev_priv * ret;

	mutex_lock(&irati_ctrl_dm.general_lock);
	ret = irati_ctrl_dm.ipcm_ctrl_dev;
	mutex_unlock(&irati_ctrl_dm.general_lock);

	return ret;
}
EXPORT_SYMBOL(get_ipcm_ctrl_dev);

static struct ctrldev_priv * get_ctrl_dev(irati_msg_port_t port_id)
{
	struct ctrldev_priv * pos;

        list_for_each_entry(pos, &(irati_ctrl_dm.ctrl_devs), node) {
                if (pos->port_id == port_id) {
                        return pos;
                }
        }

        return NULL;
}

struct ctrldev_priv * get_ctrl_dev_from_port_id(irati_msg_port_t port)
{
	struct ctrldev_priv * ret = 0;

	mutex_lock(&irati_ctrl_dm.general_lock);
	ret = get_ctrl_dev(port);
	mutex_unlock(&irati_ctrl_dm.general_lock);

	return ret;
}
EXPORT_SYMBOL(get_ctrl_dev_from_port_id);

static int ctrl_dev_data_post(struct msg_queue_entry * entry, irati_msg_port_t port_id)
{
	struct ctrldev_priv * ctrl_dev;
	int retval = 0;

        mutex_lock(&irati_ctrl_dm.general_lock);

        ctrl_dev = get_ctrl_dev(port_id);
        if (!ctrl_dev) {
        	mutex_unlock(&irati_ctrl_dm.general_lock);
        	LOG_ERR("Could not get IRATI ctrl dev for port-id %u",
        		port_id);
        	return -1;
        }

        /* TODO implement maximum queue size */
        spin_lock(&ctrl_dev->pending_msgs_lock);
        if (rfifo_push_ni(ctrl_dev->pending_msgs, entry)) {
        	LOG_ERR("Could not write %zd bytes into port-id %u fifo",
        			sizeof(*entry), port_id);
        	retval = -1;
        }
        spin_unlock(&ctrl_dev->pending_msgs_lock);

        if (retval == 0) {
		/* set_tsk_need_resched(current); */
		wake_up_interruptible_poll(&ctrl_dev->read_wqueue,
					   POLLIN | POLLRDNORM | POLLRDBAND);
        }

        mutex_unlock(&irati_ctrl_dm.general_lock);

        return retval;
}

static ssize_t
ctrldev_write(struct file *f, const char __user *ubuf, size_t len, loff_t *ppos)
{
        struct ctrldev_priv    * priv = (struct ctrldev_priv *) f->private_data;
        struct irati_msg_base  * bmsg;
        struct msg_queue_entry * entry;
        char 		       * kbuf;
        ssize_t 		 ret;

        LOG_DBG("Syscall write SDU (size = %zd, port-id = %d)",
                        len, priv->port_id);

        if (len < sizeof(irati_msg_t)) {
        	/* This message doesn't even contain a message type. */
        	return -EINVAL;
        }

        kbuf = rkzalloc(len, GFP_KERNEL);
        if (!kbuf) {
        	return -ENOMEM;
        }

        /* Copy the userspace serialized message into a temporary kernelspace
         * buffer. */
        if (unlikely(copy_from_user(kbuf, ubuf, len))) {
        	rkfree(kbuf);
        	return -EFAULT;
        }

        /* TODO: deserialize message */
        ret = deserialize_irati_msg(irati_ker_numtables, IRATI_RINA_C_MAX,
                        kbuf, len, priv->msgbuf, sizeof(priv->msgbuf));
        if (ret) {
        	rkfree(kbuf);
        	return -EINVAL;
        }

        bmsg = IRATI_MB(priv->msgbuf);

        /* Check if message is for the kernel, otherwise, put in right queue */
        if (bmsg->dest_port != 0) {
        	entry = rkzalloc(sizeof(*entry), GFP_KERNEL);
        	if (!entry) {
        		irati_msg_free(irati_ker_numtables, IRATI_RINA_C_MAX,
        			       bmsg);
        		rkfree(kbuf);
        		return -ENOMEM;
        	}

        	entry->sermsg = kbuf;
        	entry->serlen = len;

        	if (ctrl_dev_data_post(entry, bmsg->dest_port)) {
        		irati_msg_free(irati_ker_numtables, IRATI_RINA_C_MAX,
        			       bmsg);
        		rkfree(kbuf);
        		rkfree(entry);
			return -EFAULT;
        	}
        } else {
        	if (bmsg->msg_type >= IRATI_RINA_C_MAX ||
        			!irati_ctrl_dm.handlers[bmsg->msg_type].cb) {
        		irati_msg_free(irati_ker_numtables, IRATI_RINA_C_MAX,
        			       bmsg);
        		rkfree(kbuf);
        		return -EINVAL;
        	}
        	/* TODO check permissions */

        	/* Invoke the message handler */
        	ret = irati_ctrl_dm.handlers[bmsg->msg_type].cb(priv, bmsg,
        			 irati_ctrl_dm.handlers[bmsg->msg_type].data);
        }

        irati_msg_free(irati_ker_numtables, IRATI_RINA_C_MAX, bmsg);
	rkfree(kbuf);

	if (ret) {
		return ret;
	}

        *ppos += len;
        return len;
}

static ssize_t
ctrldev_read(struct file *f, char __user *buffer, size_t size, loff_t *ppos)
{
        struct ctrldev_priv * priv = f->private_data;
        struct msg_queue_entry * entry = NULL;
        bool blocking = !(f->f_flags & O_NONBLOCK);
        ssize_t ret;

        LOG_DBG("Syscall read SDU (size = %zd, port-id = %d)",
                size, priv->port_id);

        spin_lock(&priv->pending_msgs_lock);
	if (blocking) { /* blocking I/O */
		while (rfifo_is_empty(priv->pending_msgs)) {
			spin_unlock(&priv->pending_msgs_lock);

			LOG_DBG("Going to sleep on wait queue %pK (reading)",
					&priv->read_wqueue);
			ret = wait_event_interruptible(priv->read_wqueue,
						       rfifo_is_empty(priv->pending_msgs));
			LOG_DBG("Read woken up (%zd)", ret);

			if (ret < 0)
				goto finish;

			spin_lock(&priv->pending_msgs_lock);
		}

		if (rfifo_is_empty(priv->pending_msgs)) {
			spin_unlock(&priv->pending_msgs_lock);
			ret = -EIO;
			goto finish;
		}
	} else {  /* non-blocking I/O */
		if (rfifo_is_empty(priv->pending_msgs)) {
			spin_unlock(&priv->pending_msgs_lock);
			LOG_DBG("No data available in port-id %u", priv->port_id);
			ret = -EAGAIN;
			goto finish;
		}
	}

	entry = rfifo_peek(priv->pending_msgs);
	if (entry->serlen > size) {
		spin_unlock(&priv->pending_msgs_lock);
		ret = -ENOBUFS;
		goto finish;
	}

	rfifo_pop(priv->pending_msgs);
	spin_unlock(&priv->pending_msgs_lock);

	if (unlikely(copy_to_user(buffer,
			entry->sermsg, entry->serlen))) {
		ret = -EFAULT;
	} else {
		ret = entry->serlen;
		*ppos += ret;
	}

	rkfree(entry->sermsg);
	rkfree(entry);

finish:
        return ret;
}

static unsigned int
ctrldev_poll(struct file *f, poll_table *wait)
{
	struct ctrldev_priv *priv = (struct ctrldev_priv *) f->private_data;
	unsigned int mask = 0;

	poll_wait(f, &priv->read_wqueue, wait);

	spin_lock(&priv->pending_msgs_lock);
	if (!rfifo_is_empty(priv->pending_msgs)) {
		mask |= POLLIN | POLLRDNORM;
	}
	spin_unlock(&priv->pending_msgs_lock);

	mask |= POLLOUT | POLLWRNORM;

	return mask;
}

static int
ctrldev_open(struct inode *inode, struct file *f)
{
        struct ctrldev_priv *priv;

        priv = rkzalloc(sizeof(*priv), GFP_KERNEL);
        if (!priv) {
                return -ENOMEM;
        }

	priv->pending_msgs = rfifo_create_ni();
	if (!priv->pending_msgs) {
		rkfree(priv);
		return -ENOMEM;
	}

	spin_lock_init(&priv->pending_msgs_lock);
	init_waitqueue_head(&priv->read_wqueue);

        priv->port_id = port_id_bad();
        priv->file = f;
        f->private_data = priv;

        mutex_lock(&irati_ctrl_dm.general_lock);
        list_add_tail(&priv->node, &irati_ctrl_dm.ctrl_devs);
        mutex_unlock(&irati_ctrl_dm.general_lock);

        return 0;
}

static int
ctrldev_release(struct inode *inode, struct file *f)
{
        struct ctrldev_priv *priv = (struct ctrldev_priv *) f->private_data;
        struct ctrldev_priv * ipcm_ctrl_dev;
        struct irati_msg_base msg;

        mutex_lock(&irati_ctrl_dm.general_lock);
        list_del_init(&priv->node);
        mutex_unlock(&irati_ctrl_dm.general_lock);

        /* Drain queue of pending messages */
        if (rfifo_destroy(priv->pending_msgs,
        		  (void (*) (void *)) rkfree)) {
        	LOG_ERR("Ctrl-dev %u FIFO has not been destroyed",
        		priv->port_id);
        }

        /* TODO inform IPCM Daemon that a OS process has died */
        mutex_lock(&irati_ctrl_dm.general_lock);
        if (priv == irati_ctrl_dm.ipcm_ctrl_dev) {
        	LOG_WARN("IPC Manager process has been destroyed");
        	irati_ctrl_dm.ipcm_ctrl_dev = 0;
        	mutex_unlock(&irati_ctrl_dm.general_lock);
        } else {
        	//Inform IPCM Daemon that a OS process has died
        	ipcm_ctrl_dev = irati_ctrl_dm.ipcm_ctrl_dev;
        	mutex_unlock(&irati_ctrl_dm.general_lock);
        	msg.msg_type = RINA_C_IPCM_SOCKET_CLOSED_NOTIFICATION;
        	msg.src_ipcp_id = 0;
        	msg.src_port = priv->port_id;
        	msg.event_id = 0;

        	if (irati_ctrl_dev_snd_resp_msg(ipcm_ctrl_dev, &msg)) {
        		LOG_ERR("Could not send flow_result_msg");
        	}

        }

        rkfree(priv);
        f->private_data = NULL;

        return 0;
}

/* Data structure passed along with ioctl */
struct irati_ctrldev_ctldata {
	irati_msg_port_t port_id;
};

#define IRATI_FLOW_BIND _IOW(0xAF, 0x00, struct irati_ctrldev_ctldata)

static long
ctrldev_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
        struct ctrldev_priv *priv = (struct ctrldev_priv *) f->private_data;
        void __user *p = (void __user *)arg;
        struct irati_ctrldev_ctldata data;

        if (cmd != IRATI_FLOW_BIND) {
                LOG_ERR("Invalid cmd %u", cmd);
                return -EINVAL;
        }

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

        priv->port_id = data.port_id;

        LOG_DBG("Bound to port id %d", data.port_id);

        return 0;
}

#ifdef CONFIG_COMPAT
static long
ctrldev_compat_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	return ctrldev_ioctl(f, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static const struct file_operations irati_ctrl_fops = {
        .owner          = THIS_MODULE,
        .release        = ctrldev_release,
        .open           = ctrldev_open,
        .write          = ctrldev_write,
        .read           = ctrldev_read,
        .poll           = ctrldev_poll,
        .unlocked_ioctl = ctrldev_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl   = ctrldev_compat_ioctl,
#endif
        .llseek         = noop_llseek,
};

static struct miscdevice irati_ctrl_misc = {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "irati-ctrl",
        .fops = &irati_ctrl_fops,
};

int
ctrldev_init(void)
{
        int ret;

        mutex_init(&irati_ctrl_dm.general_lock);
        INIT_LIST_HEAD(&irati_ctrl_dm.ctrl_devs);

        irati_ctrl_dm.sn_counter = 0;
        irati_ctrl_dm.ipcm_ctrl_dev = 0;

        ret = misc_register(&irati_ctrl_misc);
        if (ret) {
                LOG_ERR("Failed to register ctrl misc device");
                return ret;
        }

        return ret;
}

void
ctrldev_fini(void)
{
        misc_deregister(&irati_ctrl_misc);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eduard Grasa <eduard.grasa@i2cat.net>");
MODULE_AUTHOR("Vincenzo Maffione <v.maffione@nextworks.it>");
