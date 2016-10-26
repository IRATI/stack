/*
 * Support for multiple VMPI providers
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
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/mutex.h>

#include "vmpi.h"


struct vmpi_provider {
        struct vmpi_ops ops;
        unsigned int id;
        unsigned int provider;
        struct list_head node;
};

static LIST_HEAD(providers);
static struct mutex lock;

/* To be called under lock. */
static struct vmpi_provider*
vmpi_search_instance_by_id(struct list_head *head, unsigned int provider,
                     unsigned int id)
{
        struct vmpi_provider *elem;

        list_for_each_entry(elem, head, node) {
                if (elem->id == id && (elem->provider == provider ||
                                       provider == VMPI_PROVIDER_AUTO)) {
                        return elem;
                }
        }

        return NULL;
}

/* To be called under lock.
 * This function looks for the lowest unused id. In future we could use a
 * bitmap, but for now we stick to a simpler approach. */
static unsigned int
vmpi_alloc_id(void)
{
	struct vmpi_provider *elem;
	unsigned int vid = 0;

retry:
	list_for_each_entry(elem, &providers, node) {
		if (elem->id == vid) {
			vid ++;
			goto retry;
		}
	}

	return vid;
}

int vmpi_provider_find_instance(unsigned int provider, int id,
                                struct vmpi_ops *ops)
{
        struct vmpi_provider *elem;

        if (provider > VMPI_PROVIDER_AUTO) {
                printk("%s: Invalid provider %u\n", __func__, provider);
                return -EINVAL;
        }

        mutex_lock(&lock);
        elem = vmpi_search_instance_by_id(&providers, provider, id);
        if (elem == NULL) {
                mutex_unlock(&lock);
                return -EAGAIN;
        }

        /* Fill in the vmpi_ops structure. */
        *ops = elem->ops;
        mutex_unlock(&lock);

        return 0;
}
EXPORT_SYMBOL_GPL(vmpi_provider_find_instance);

int vmpi_provider_register(unsigned int provider, const struct vmpi_ops *ops,
                           unsigned int *vid)
{
        struct vmpi_provider *elem;

        if (provider >= VMPI_PROVIDER_AUTO) {
                printk("%s: Invalid provider %u\n", __func__, provider);
                return -EINVAL;
        }

        elem = kmalloc(sizeof(*elem), GFP_KERNEL);
        if (!elem) {
                printk("%s: Provider allocation failed\n", __func__);
                return -ENOMEM;
        }

        memset(elem, 0, sizeof(*elem));
        elem->provider = provider;
        elem->ops = *ops;

        mutex_lock(&lock);
        *vid = elem->id = vmpi_alloc_id();
        list_add_tail(&elem->node, &providers);
        mutex_unlock(&lock);

        printk("%s: Provider %s:%u registered\n", __func__,
               (provider == VMPI_PROVIDER_HOST) ? "HOST" : "GUEST", *vid);

        return 0;
}
EXPORT_SYMBOL_GPL(vmpi_provider_register);

int vmpi_provider_unregister(unsigned int provider, unsigned int id)
{
        struct vmpi_provider *elem;

        if (provider >= VMPI_PROVIDER_AUTO) {
                printk("%s: Invalid provider %u\n", __func__, provider);
                return -EINVAL;
        }

        mutex_lock(&lock);
        elem = vmpi_search_instance_by_id(&providers, provider, id);
        if (!elem) {
                mutex_unlock(&lock);
                printk("%s: No such provider %u:%u\n", __func__, provider, id);
                return -ENOENT;
        }

        list_del(&elem->node);
        mutex_unlock(&lock);

        kfree(elem);

        printk("%s: Provider %s:%u unregistered\n", __func__,
               (provider == VMPI_PROVIDER_HOST) ? "HOST" : "GUEST", id);

        return 0;
}
EXPORT_SYMBOL_GPL(vmpi_provider_unregister);

unsigned int vmpi_get_max_payload_size(void)
{
        return VMPI_BUF_SIZE;
}
EXPORT_SYMBOL_GPL(vmpi_get_max_payload_size);

static int __init vmpi_provider_init(void)
{
        mutex_init(&lock);
        return 0;
}

static void __exit vmpi_provider_fini(void)
{
        mutex_destroy(&lock);
}

module_init(vmpi_provider_init);
module_exit(vmpi_provider_fini);

MODULE_AUTHOR("Vincenzo Maffione");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("VMPI providers support");
