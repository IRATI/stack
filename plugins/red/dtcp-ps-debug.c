/*
 * TCP-like RED Congestion avoidance policy set
 * Debug operations for DTCP PS
 *
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
 *
 * This program is free software; you can casistribute it and/or modify
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
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>
#include "dtcp-ps-debug.h"
#include "rds/rmem.h"

#define PROC_FILE_NAME "dtcp_credits"

static struct list_head debug_instances = LIST_HEAD_INIT(debug_instances);

static void *
credit_seq_start(struct seq_file *s, loff_t *pos)
{ return seq_list_start(&debug_instances, *pos); }

static int
credit_seq_show(struct seq_file *s, void *v)
{
	unsigned int i;
	struct red_dtcp_debug * debug = list_entry(v, struct red_dtcp_debug, list);

	seq_printf(s, "RED Credit for DTCP %p\n", debug);
	for (i=0; i < debug->ws_index; i++)
		seq_printf(s, "%lu\n", debug->ws_log[i]);
	seq_printf(s, "----------------------------------\n\n");
	return 0;
}

static void *
credit_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	return seq_list_next(v, &debug_instances, pos);
}

static void
credit_seq_stop(struct seq_file *s, void *v)
{ return; }

static struct seq_operations credit_seq_ops = {
    .start = credit_seq_start,
    .next  = credit_seq_next,
    .stop  = credit_seq_stop,
    .show  = credit_seq_show
};

static int
credit_open(struct inode *inode, struct file *file)
{ return seq_open(file, &credit_seq_ops); }

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,5,0)
static struct file_operations credit_file_ops = {
	.owner   = THIS_MODULE,
	.open    = credit_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release
};
#else
static struct proc_ops credit_file_ops = {
	.proc_open = credit_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = seq_release
};
#endif

struct red_dtcp_debug *
red_dtcp_debug_create(void)
{
	struct red_dtcp_debug * debug;

	debug = rkmalloc(sizeof(*debug), GFP_KERNEL);
	if (!debug)
		return NULL;

	INIT_LIST_HEAD(&debug->list);
	debug->ws_index = 0;

	list_add(&debug->list, &debug_instances);

	return debug;
}

int
red_dtcp_debug_proc_init(void)
{
	proc_create(PROC_FILE_NAME, 0, NULL, &credit_file_ops);
	INIT_LIST_HEAD(&debug_instances);
	return 0;
}

void
red_dtcp_debug_proc_exit(void)
{
	struct red_dtcp_debug * entry;
	list_for_each_entry(entry, &debug_instances, list) {
		rkfree(entry);
	}
	remove_proc_entry(PROC_FILE_NAME, NULL);
}
