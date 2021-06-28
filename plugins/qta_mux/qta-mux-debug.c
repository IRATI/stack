/*
 * QTA-Mux policy set
 * Debug operations for RMT PS
 *
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
 *    Eduard Grasa	<eduard.grasa@i2cat.net>
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
#include "rds/rmem.h"
#include "qta-mux-debug.h"

#define PROC_FILE_NAME "qta_mux_uq_lengths"

static struct list_head debug_instances = LIST_HEAD_INIT(debug_instances);

static void *
q_len_seq_start(struct seq_file *s, loff_t *pos)
{ return seq_list_start(&debug_instances, *pos); }

static int
q_len_seq_show(struct seq_file *s, void *v)
{
	unsigned int i;
	struct urgency_queue_debug_info * debug =
			list_entry(v, struct urgency_queue_debug_info, list);

	seq_printf(s, "C/U Mux urgency queue for N-1 port_id %u\n",
			debug->port);
	seq_printf(s, "Urgency level:\t%u\n", debug->urgency_level);
	seq_printf(s, "Queue occupation (length, time):\n");
	for (i=0; i < debug->q_index; i++)
		seq_printf(s, "%5lld\t%5lld\n", debug->q_log[i][0], debug->q_log[i][1]);
	seq_printf(s, "----------------------------------\n\n");
	return 0;
}

static void *
q_len_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	return seq_list_next(v, &debug_instances, pos);
}

static void
q_len_seq_stop(struct seq_file *s, void *v)
{ return; }

static struct seq_operations q_len_seq_ops = {
    .start = q_len_seq_start,
    .next  = q_len_seq_next,
    .stop  = q_len_seq_stop,
    .show  = q_len_seq_show
};

static int
q_len_open(struct inode *inode, struct file *file)
{ return seq_open(file, &q_len_seq_ops); }

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,5,0)
static struct file_operations q_len_file_ops = {
	.owner = THIS_MODULE,
	.open = q_len_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release
};
#else
static struct proc_ops q_len_file_ops = {
	.proc_open    = q_len_open,
	.proc_read    = seq_read,
	.proc_lseek  = seq_lseek,
	.proc_release = seq_release
};
#endif

struct urgency_queue_debug_info * uqueue_debug_info_create(port_id_t port,
							   uint_t urgency_lev)
{
	struct urgency_queue_debug_info * debug;

	debug = rkmalloc(sizeof(*debug), GFP_KERNEL);
	if (!debug)
		return NULL;

	INIT_LIST_HEAD(&debug->list);
	debug->q_index = 0;
	debug->port = port;
	debug->urgency_level = urgency_lev;

	list_add(&debug->list, &debug_instances);

	return debug;
}

void udebug_info_destroy(struct urgency_queue_debug_info * info)
{
	if (!info)
		return;

	list_del(&info->list);

	rkfree(info);
}

int
qta_mux_debug_proc_init(void)
{
	proc_create(PROC_FILE_NAME, 0, NULL, &q_len_file_ops);
	INIT_LIST_HEAD(&debug_instances);
	return 0;
}

void
qta_mux_debug_proc_exit(void)
{
	struct urgency_queue_debug_info  * entry;
	list_for_each_entry(entry, &debug_instances, list) {
		rkfree(entry);
	}
	remove_proc_entry(PROC_FILE_NAME, NULL);
}
