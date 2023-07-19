/*
 * PIDM (Port-IDs Manager)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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
#include <linux/debugfs.h>
#include <linux/seq_file.h>

#define RINA_PREFIX "pidm"

#include "logs.h"
#include "debug.h"
#include "utils.h"
#include "pidm.h"
#include "common.h"

#define MAX_PORT_ID (((2 << BITS_PER_BYTE) * sizeof(port_id_t)) - 1)

struct pidm {
	struct list_head allocated_ports;
	port_id_t last_allocated;
        spinlock_t lock;

#ifdef CONFIG_DEBUG_FS
        struct dentry* dbg_file;
        int dbg_file_data;
#endif
};

struct alloc_pid {
        struct list_head       list;
        port_id_t pid;
};

#ifdef CONFIG_DEBUG_FS
static int pidm_dbg_show(struct seq_file *s, void *v) {
        struct pidm *instance;
	struct alloc_pid * pos, * next;

        instance = (struct pidm *)s->private;
        seq_printf(s, "Last allocated: %u\n", instance->last_allocated);
        seq_printf(s, "Allocated ports:\n");

        spin_lock(&instance->lock);
        list_for_each_entry_safe(pos, next, &instance->allocated_ports, list) {
                seq_printf(s, "%d ", pos->pid);
        }
        spin_unlock(&instance->lock);

        seq_printf(s, "\n");

        return 0;
}

static int pidm_dbg_open(struct inode *inode, struct file *file)
{
	return single_open(file, pidm_dbg_show, inode->i_private);
}

static const struct file_operations pidm_dbg_fops = {
	.open		= pidm_dbg_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};
#endif

struct pidm * pidm_create(struct dentry *dbg_dir)
{
        struct pidm *instance;

        instance = rkmalloc(sizeof(struct pidm), GFP_KERNEL);
        if (!instance) return NULL;

        INIT_LIST_HEAD(&(instance->allocated_ports));
        instance->last_allocated = 0;

#ifdef CONFIG_DEBUG_FS
        if (dbg_dir)
                instance->dbg_file = debugfs_create_file("pidm", 0400, dbg_dir,
                                                         instance, &pidm_dbg_fops);
#endif

        LOG_INFO("Instance initialized successfully (%zd port-ids)",
        	MAX_PORT_ID);

        return instance;
}

int pidm_destroy(struct pidm * instance)
{
	struct alloc_pid * pos, * next;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

#ifdef CONFIG_DEBUG_FS
        if (instance->dbg_file)
                debugfs_remove(instance->dbg_file);
#endif

        spin_lock(&instance->lock);

        list_for_each_entry_safe(pos, next, &instance->allocated_ports, list) {
                rkfree(pos);
        }

        rkfree(instance);

        return 0;
}

int pidm_allocated(struct pidm * instance, port_id_t port_id)
{
	struct alloc_pid * pos, * next;

        list_for_each_entry_safe(pos, next, &instance->allocated_ports, list) {
                if (pos->pid == port_id) {
                	return 1;
                }
        }

        return 0;
}

port_id_t pidm_allocate(struct pidm * instance)
{
	struct alloc_pid * new_port_id;
        port_id_t pid;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return port_id_bad();
        }

        if (instance->last_allocated == MAX_PORT_ID) {
        	pid = 1;
        } else {
        	pid = instance->last_allocated + 1;
        }

        while (pidm_allocated(instance, pid)) {
        	if (pid == MAX_PORT_ID) {
        		pid = 1;
        	} else {
        		pid ++;
        	}
        }

        new_port_id = rkmalloc(sizeof(*new_port_id), GFP_ATOMIC);
        if (!new_port_id)
        	return port_id_bad();

        INIT_LIST_HEAD(&(new_port_id->list));
        new_port_id->pid = pid;
        list_add(&(new_port_id->list), &(instance->allocated_ports));

        instance->last_allocated = pid;

        LOG_DBG("Port-id allocation completed successfully (id = %d)", pid);

        return pid;
}

int pidm_release(struct pidm * instance,
                 port_id_t     id)
{
	struct alloc_pid * pos, * next;
        int found = 0;

        if (!is_port_id_ok(id)) {
                LOG_ERR("Bad flow-id passed, bailing out");
                return -1;
        }
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        list_for_each_entry_safe(pos, next, &instance->allocated_ports, list) {
                if (pos->pid == id) {
                	list_del(&pos->list);
                	rkfree(pos);
                        found = 1;
                }
        }

        if (!found) {
                LOG_ERR("Didn't find port-id %d, returning error", id);
        } else {
                LOG_DBG("Port-id release completed successfully (port_id: %d)", id);
        }

        return 0;
}

