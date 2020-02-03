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
#include <linux/compat.h>

#define RINA_PREFIX "ctrldev"

#include "logs.h"
#include "debug.h"
#include "utils.h"
#include "ctrldev.h"
#include "kipcm.h"
#include "rds/rfifo.h"
#include "irati/kernel-msg.h"
#include "irati/serdes-utils.h"

#define IRATI_CTRL_MSG_MAX_SIZE 5000

extern struct kipcm *default_kipcm;

/* Private data to an ctrldev file instance. */
struct ctrldev_priv {
	irati_msg_port_t   port_id;
	int		   flushed;
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
};

static struct irati_ctrl_dm irati_ctrl_dm;

struct msg_queue_entry {
	char   * sermsg;
	uint32_t   serlen;
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

        if (msg_type <= IRATI_RINA_C_MIN &&
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
        if (!ctrl_dev->pending_msgs) {
        	LOG_ERR("Control device has been closed");
        	retval = -1;
        } else if (rfifo_push_ni(ctrl_dev->pending_msgs, entry)) {
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

void msg_queue_entry_destroy(struct msg_queue_entry * entry)
{
	if (!entry)
		return;
 	if (entry->sermsg)
		rkfree(entry->sermsg);
 	rkfree(entry);
}

int irati_ctrl_dev_snd_resp_msg(irati_msg_port_t port,
				struct irati_msg_base *bmsg)
{
	struct msg_queue_entry * entry;
	int ret = 0;

	//Serialize message
	entry = rkzalloc(sizeof(*entry), GFP_KERNEL);
	if (!entry) {
		LOG_ERR("Could not create entry");
		return -1;
	}

	entry->serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX, bmsg);
	entry->sermsg = rkzalloc(entry->serlen * sizeof (char), GFP_KERNEL);
	if (!entry->sermsg) {
		LOG_ERR("Could not create entry->sermsg");
		rkfree(entry);
		return -1;
	}

	ret = serialize_irati_msg(irati_ker_numtables, IRATI_RINA_C_MAX,
                        	  entry->sermsg, bmsg);
        if (ret <= 0) {
        	LOG_ERR("Problems serializing msg: %d", entry->serlen);
        	rkfree(entry->sermsg);
        	rkfree(entry);
        	return -1;
        }
        entry->serlen = ret;

        if (ctrl_dev_data_post(entry, port)) {
        	msg_queue_entry_destroy(entry);
		return -1;
        }

        return 0;
}
EXPORT_SYMBOL(irati_ctrl_dev_snd_resp_msg);

static ssize_t
ctrldev_write(struct file *f, const char __user *ubuf, size_t len, loff_t *ppos)
{
        struct ctrldev_priv    * priv = (struct ctrldev_priv *) f->private_data;
        struct irati_msg_base  * bmsg;
        struct msg_queue_entry * entry;
        char 		       * kbuf;
        ssize_t 		 ret = 0;
        bool 			 destroy_kbuf = true;

        LOG_DBG("Syscall write SDU (size = %zd, port-id = %d)",
                        len, priv->port_id);

        if (!priv) {
        	LOG_ERR("Device has been closed");
        	return -1;
        }

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

        bmsg = IRATI_MB(kbuf);
        /* Check if message is for the kernel, otherwise, put in right queue */
        if (bmsg->dest_port != 0) {
        	entry = rkzalloc(sizeof(*entry), GFP_KERNEL);
        	if (!entry) {
        		rkfree(kbuf);
        		return -ENOMEM;
        	}

        	entry->sermsg = kbuf;
        	entry->serlen = len;
        	destroy_kbuf = false;

        	if (ctrl_dev_data_post(entry, bmsg->dest_port)) {
        		rkfree(kbuf);
        		rkfree(entry);
			return -EFAULT;
        	}
        } else {
        	if (bmsg->msg_type >= IRATI_RINA_C_MAX ||
        			!irati_ctrl_dm.handlers[bmsg->msg_type].cb) {
        		rkfree(kbuf);
        		return -EINVAL;
        	}
        	/* TODO check permissions */

                /* Deserialize message */
                bmsg = (struct irati_msg_base *) deserialize_irati_msg(irati_ker_numtables, IRATI_RINA_C_MAX,
                						       kbuf, len);
                if (!bmsg) {
                	rkfree(kbuf);
                	return -EINVAL;
                }

        	/* Invoke the message handler */
        	ret = irati_ctrl_dm.handlers[bmsg->msg_type].cb(priv->port_id, bmsg,
        			 irati_ctrl_dm.handlers[bmsg->msg_type].data);

        	irati_msg_free(irati_ker_numtables, IRATI_RINA_C_MAX, bmsg);
        	rkfree(bmsg);
        }

        if (destroy_kbuf)
        	rkfree(kbuf);

	if (ret) {
		return ret;
	}

        *ppos += len;
        return len;
}

bool queue_ready(struct ctrldev_priv * priv)
{
	LOG_DBG("Evaluating queue_reqdy");

	if (!priv || !priv->pending_msgs) {
		LOG_DBG("File descriptor has closed");
		return true;
	}

	return !rfifo_is_empty(priv->pending_msgs);
}

static ssize_t
ctrldev_read(struct file *f, char __user *buffer, size_t size, loff_t *ppos)
{
        struct ctrldev_priv * priv = f->private_data;
        struct msg_queue_entry * entry = NULL;
        bool blocking = !(f->f_flags & O_NONBLOCK);
        uint32_t msg_size;
        ssize_t ret;

        LOG_DBG("Syscall read SDU (size = %zd, port-id = %d)",
                size, priv->port_id);

        if (!priv) {
        	LOG_ERR("Device private data is null");
        	return -1;
        }

        spin_lock(&priv->pending_msgs_lock);
        if (!priv->pending_msgs) {
        	spin_unlock(&priv->pending_msgs_lock);
        	LOG_INFO("Control device has been closed");
        	return -1;
        }

	if (blocking) { /* blocking I/O */
		while (rfifo_is_empty(priv->pending_msgs)) {
			spin_unlock(&priv->pending_msgs_lock);

			LOG_DBG("Going to sleep on wait queue %pK (reading)",
					&priv->read_wqueue);
			ret = wait_event_interruptible(priv->read_wqueue,
						       queue_ready(f->private_data));
			LOG_DBG("Read woken up (%zd)", ret);

			if (ret < 0) {
				goto finish;
			}

			priv = f->private_data;

			if (!priv) {
				LOG_INFO("Control device has been closed");
				ret = -1;
				goto finish;
			}

			spin_lock(&priv->pending_msgs_lock);
		}

	        if (!priv->pending_msgs) {
	        	spin_unlock(&priv->pending_msgs_lock);
	        	LOG_INFO("Control device has been closed");
	        	ret = -1;
	        	goto finish;
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
	if (!entry) {
		spin_unlock(&priv->pending_msgs_lock);
		ret = -EFAULT;
		goto finish;
	}

	if (size == 0) {
		msg_size = entry->serlen;
		spin_unlock(&priv->pending_msgs_lock);
		if (unlikely(copy_to_user(buffer, &msg_size, sizeof(uint32_t)))) {
			ret = -EFAULT;
		} else {
			ret = sizeof(uint32_t);
		}
		goto finish;
	}

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

	LOG_DBG("Read on port %u finishing, read %zd bytes",
		priv->port_id, ret);
	msg_queue_entry_destroy(entry);

finish:
        return ret;
}

static unsigned int
ctrldev_poll(struct file *f, poll_table *wait)
{
	struct ctrldev_priv *priv = (struct ctrldev_priv *) f->private_data;
	unsigned int mask = 0;

	if (!priv) {
		LOG_ERR("Device has been closed");
		return mask;
	}

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
        priv->flushed = 0;
        priv->file = f;
        f->private_data = priv;

        mutex_lock(&irati_ctrl_dm.general_lock);
        list_add_tail(&priv->node, &irati_ctrl_dm.ctrl_devs);
        mutex_unlock(&irati_ctrl_dm.general_lock);

        return 0;
}

static int ctrldev_release(struct inode *inode, struct file *f)
{
        struct ctrldev_priv *priv = (struct ctrldev_priv *) f->private_data;
        struct rfifo * pmsgs;

        mutex_lock(&irati_ctrl_dm.general_lock);
        LOG_DBG("Releasing ctrl file descriptor associated to port-id %d", priv->port_id);
        list_del_init(&priv->node);
        mutex_unlock(&irati_ctrl_dm.general_lock);

        spin_lock(&priv->pending_msgs_lock);
        pmsgs = priv->pending_msgs;
        priv->pending_msgs = 0;
        spin_unlock(&priv->pending_msgs_lock);

        /* Drain queue of pending messages */
        if (rfifo_destroy(pmsgs, (void (*) (void *)) msg_queue_entry_destroy)) {
        	LOG_ERR("Ctrl-dev %u FIFO has not been destroyed",
        		priv->port_id);
        }

        //TODO If IPC Manager has died, destroy all control devices and IPCPs
        if (priv->port_id == 1) {
        	LOG_WARN("IPC Manager process has been destroyed");
        }

        LOG_DBG("Instance of control device bound to port %d released",
                priv->port_id);

        /* Notify reader of the control device */
        wake_up_interruptible_poll(&priv->read_wqueue,
        			   POLLIN | POLLRDNORM | POLLRDBAND);

        rkfree(priv);

        return 0;
}

static int ctrldev_flush(struct file * f, fl_owner_t id) {
	struct ctrldev_priv *priv = (struct ctrldev_priv *) f->private_data;

	mutex_lock(&irati_ctrl_dm.general_lock);

	LOG_DBG("Ctrl dev flush called for port-id %d", priv->port_id);
	priv->flushed = 1;

	mutex_unlock(&irati_ctrl_dm.general_lock);


	return 0;
}

static bool ctrl_port_in_use(irati_msg_port_t port_id)
{
	struct ctrldev_priv * pos;

        mutex_lock(&irati_ctrl_dm.general_lock);
        list_for_each_entry(pos, &(irati_ctrl_dm.ctrl_devs), node) {
                if (port_id == pos->port_id && !pos->flushed) {
                	mutex_unlock(&irati_ctrl_dm.general_lock);
                	return true;
                }
        }
        mutex_unlock(&irati_ctrl_dm.general_lock);

        return false;
}

static long ctrldev_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
        struct ctrldev_priv *priv = (struct ctrldev_priv *) f->private_data;
        void __user *p = (void __user *)arg;
        struct irati_ctrldev_ctldata data;

        if (cmd != IRATI_CTRL_FLOW_BIND) {
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

        if (ctrl_port_in_use(data.port_id)) {
        	LOG_ERR("Control port is already in use, %d", data.port_id);
        	return -EINVAL;
        }

        if (is_port_id_ok(priv->port_id)) {
                LOG_ERR("Cannot bind to port %d, "
                        "already bound to port id %d",
                        data.port_id, priv->port_id);
                return -EBUSY;
        }

        priv->port_id = data.port_id;

        LOG_DBG("Control device instance bound to port id %d", data.port_id);

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
	.flush		= ctrldev_flush,
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
