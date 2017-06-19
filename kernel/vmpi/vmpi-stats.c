#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "vmpi-stats.h"


static struct vmpi_stats *stats_instance = NULL;

static int
vmpi_stats_show(struct seq_file *m, void *v)
{
        if (!stats_instance) {
                seq_printf(m, "(NULL)\n");
                return 0;
        }

        seq_printf(m, "rxres[%u]\n", stats_instance->rxres);
        seq_printf(m, "rxint[%u]\n", stats_instance->rxint);
        seq_printf(m, "txreq[%u]\n", stats_instance->txreq);
        seq_printf(m, "txres[%u]\n", stats_instance->txres);
        seq_printf(m, "txint[%u]\n", stats_instance->txint);

        return 0;
}

static int
vmpi_stats_open(struct inode *inode, struct file *file)
{
        return single_open(file, vmpi_stats_show, NULL);
}

static const struct file_operations vmpi_stats_fops = {
        .owner     = THIS_MODULE,
        .open      = vmpi_stats_open,
        .read      = seq_read,
        .llseek    = seq_lseek,
        .release   = single_release,
};

int
vmpi_stats_init(struct vmpi_stats *st)
{
        if (stats_instance) {
                /* Oooops, we've lost! */
                return 0;
        }

        /* Store the provided pointer in a static global variable. */
        stats_instance = st;

        proc_create("vmpi-stats", 0, NULL, &vmpi_stats_fops);
        return 0;
}

void
vmpi_stats_fini(struct vmpi_stats *st)
{
        if (!stats_instance || st != stats_instance) {
                return;
        }

        remove_proc_entry("vmpi-stats", NULL);
        stats_instance = NULL;
}

