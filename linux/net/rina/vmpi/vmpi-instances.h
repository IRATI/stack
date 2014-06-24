/*
 * Support for lists of VMPI instances
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

#include <linux/list.h>
#include <linux/wait.h>

#include "vmpi-ops.h"

static inline void
vmpi_add_instance(struct list_head *head, wait_queue_head_t *wqh,
                  struct vmpi_info *mpi)
{
        unsigned int next_id = 0;

        if (!list_empty(head)) {
                struct vmpi_info *last = list_last_entry(head,
                                                         struct vmpi_info,
                                                         node);

                next_id = last->id + 1;
        }

        mpi->id = next_id;
        list_add_tail(&mpi->node, head);
        wake_up_interruptible(wqh);
}

static inline void
vmpi_remove_instance(struct list_head *head, struct vmpi_info *mpi)
{
        list_del(&mpi->node);
}

static ssize_t
vmpi_ops_write(struct vmpi_ops *ops, unsigned int channel,
               const struct iovec *iv, unsigned long iovcnt)
{
        return vmpi_write_common(ops->priv, channel, iv, iovcnt, 0);
}

static int
vmpi_ops_register_read_callback(struct vmpi_ops *ops, vmpi_read_cb_t cb,
                                void *opaque)
{
        return vmpi_register_read_callback(ops->priv, cb, opaque);
}

static inline struct vmpi_info *
__vmpi_search_instance(struct list_head *head, unsigned int id)
{
        struct vmpi_info *elem;

        list_for_each_entry(elem, head, node) {
                if (elem->id == id) {
                        return elem;
                }
        }

        return NULL;
}

static inline int
__vmpi_find_instance(struct list_head *head, wait_queue_head_t *wqh,
                     unsigned int id, struct vmpi_ops *ops)
{
        int ret;

        ret = wait_event_interruptible(*wqh,
                        __vmpi_search_instance(head, id) != NULL);
        if (ret) {
                return ret;
        }

        /* Fill in the vmpi_ops structure. */
        ops->priv = __vmpi_search_instance(head, id);
        ops->write = vmpi_ops_write;
        ops->register_read_callback = vmpi_ops_register_read_callback;

        return 0;
}

