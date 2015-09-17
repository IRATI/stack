/*
 * TCP-like RED Congestion avoidance policy set
 * Debug operations for RMT PS
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
#include "rmt-ps-debug.h"
#include "rds/rmem.h"

#define PROC_FILE_NAME "rmt_q_lengths"

static struct list_head debug_instances = LIST_HEAD_INIT(debug_instances);

static void *
q_len_seq_start(struct seq_file *s, loff_t *pos)
{ return seq_list_start(&debug_instances, *pos); }

static int
q_len_seq_show(struct seq_file *s, void *v)
{
	unsigned int i;
	struct red_rmt_debug * debug = list_entry(v, struct red_rmt_debug, list);

	seq_printf(s, "RED Stats Queue %u\n", debug->port);
	seq_printf(s, "Prob drops:\t%u\n"	\
		      "Prob mark:\t%u\n"	\
		      "Forced drop:\t%u\n"	\
		      "Forced mark:\t%u\n"	\
		      "Limit drop:\t%u\n"		\
		      "Explicit drop:\t%u\n",
		      debug->stats.prob_drop, debug->stats.prob_mark,
		      debug->stats.forced_drop, debug->stats.forced_mark,
		      debug->stats.pdrop, debug->stats.other);
	seq_printf(s, "Queue occupation:\n");
	for (i=0; i < debug->q_index; i++)
		seq_printf(s, "%5u\t%5u\n", debug->q_log[i][0], debug->q_log[i][1]);
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

static struct file_operations q_len_file_ops = {
	.owner   = THIS_MODULE,
	.open    = q_len_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release
};

struct red_rmt_debug *
red_rmt_debug_create(port_id_t port)
{
	struct red_rmt_debug * debug;

	debug = rkmalloc(sizeof(*debug), GFP_KERNEL);
	if (!debug)
		return NULL;

	INIT_LIST_HEAD(&debug->list);
	debug->q_index = 0;
	debug->port = port;

	list_add(&debug->list, &debug_instances);

	return debug;
}

int
red_rmt_debug_proc_init(void)
{
	proc_create(PROC_FILE_NAME, 0, NULL, &q_len_file_ops);
	INIT_LIST_HEAD(&debug_instances);
	return 0;
}

void
red_rmt_debug_proc_exit(void)
{
	struct red_rmt_debug * entry;
	list_for_each_entry(entry, &debug_instances, list) {
		rkfree(entry);
	}
	remove_proc_entry(PROC_FILE_NAME, NULL);
}
