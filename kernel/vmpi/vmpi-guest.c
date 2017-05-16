/*
 * A guest-side VMPI implemented using the vmpi-impl guest interface
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
#include <linux/poll.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/socket.h>
#include <linux/slab.h>

#include "vmpi-stats.h"
#include "vmpi-iovec.h"
#include "vmpi-guest-impl.h"
#include "vmpi.h"


#define VMPI_GUEST_BUDGET  64

struct vmpi_info {
        vmpi_impl_info_t *vi;

        spinlock_t write_lock;
        struct work_struct recv_worker;
        struct mutex recv_worker_lock;

        vmpi_read_cb_t read_cb;
        vmpi_write_restart_cb_t write_restart_cb;
        void *cb_data;

        struct vmpi_stats stats;
        unsigned int id;
};

static int
vmpi_guest_register_cbs(struct vmpi_ops *ops, vmpi_read_cb_t rcb,
                        vmpi_write_restart_cb_t wcb, void *opaque)
{
        struct vmpi_info *mpi = ops->priv;
        int ret = 0;

        if (!rcb || !wcb) {
                return -EINVAL;
        }

        /* We could use a separate lock for this, but we'll just
         * reuse the write lock. */
        spin_lock_bh(&mpi->write_lock);
        if (mpi->read_cb) {
                ret = -EBUSY;

        } else {
                mpi->read_cb = rcb;
                mpi->write_restart_cb = wcb;
                mpi->cb_data = opaque;
        }
        spin_unlock_bh(&mpi->write_lock);

        return ret;
}

static int
vmpi_guest_unregister_cbs(struct vmpi_ops *ops)
{
        struct vmpi_info *mpi = ops->priv;

        spin_lock_bh(&mpi->write_lock);
        mpi->read_cb = NULL;
        mpi->write_restart_cb = NULL;
        mpi->cb_data = NULL;
        spin_unlock_bh(&mpi->write_lock);

        return 0;
}

static void
vmpi_clean_tx(struct vmpi_info *mpi)
{
        struct vmpi_buf *buf;

        vmpi_impl_send_cb(mpi->vi, 0);

        while ((buf = vmpi_impl_get_written_buffer(mpi->vi)) != NULL) {
                IFV(printk("xmit done %p\n", buf));
                vmpi_buf_free(buf);
                mpi->stats.txres++;
        }
}

static ssize_t
vmpi_guest_write(struct vmpi_ops *ops, unsigned int channel,
           struct vmpi_buf *vb)

{
        struct vmpi_info *mpi = ops->priv;
        vmpi_impl_info_t *vi = mpi->vi;
        size_t len = vmpi_buf_len(vb);
        ssize_t ret = 0;

        if (!vi)
                return -EBADFD;

        spin_lock_bh(&mpi->write_lock);

        vmpi_clean_tx(mpi);

        if (vmpi_impl_tx_should_stop(vi)) {
                /* We have to stop, let's enable notifications and
                 * doublecheck. */
                if (vmpi_impl_send_cb(vi, 1)) {
                        spin_unlock_bh(&mpi->write_lock);
                        return -EAGAIN;
                }

                /* Wow, that's uncommon, we may go ahead. */
                vmpi_clean_tx(mpi);
                if (vmpi_impl_tx_should_stop(vi)) {
                        /* No, false alarm. */
                        spin_unlock_bh(&mpi->write_lock);
                        return -EAGAIN;
                }
                vmpi_impl_send_cb(vi, 0);
        }

        ret = vmpi_impl_write_buf(vi, vb, channel);
        if (likely(ret == 0)) {
                ret = len;
                mpi->stats.txreq++;
        }

        vmpi_impl_txkick(vi);

        spin_unlock_bh(&mpi->write_lock);

        return ret;
}

static void
xmit_callback(vmpi_impl_info_t *vi)
{
        struct vmpi_info *mpi = vmpi_info_from_vmpi_impl_info(vi);

        mpi->stats.txint++;

        /* XXX can we disable xmit callbacks here, to avoid an
               useless burst of TX interrupts? */
        if (likely(mpi->write_restart_cb)) {
                mpi->write_restart_cb(mpi->cb_data);
        }
}

static void
recv_worker_function(struct work_struct *work)
{
        struct vmpi_info *mpi =
                container_of(work, struct vmpi_info, recv_worker);
        struct vmpi_impl_info *vi = mpi->vi;
        unsigned int budget = VMPI_GUEST_BUDGET;
        struct vmpi_buf *vb;
        unsigned int channel;

        mutex_lock(&mpi->recv_worker_lock);
 again:
        while (budget && (vb = vmpi_impl_read_buffer(vi, &channel)) != NULL) {
                IFV(printk("received %d bytes\n", (int)vmpi_buf_len(vb)));

                /* XXX don't drop the lock here */
                mutex_unlock(&mpi->recv_worker_lock);

                if (unlikely(!mpi->read_cb)) {
                        vmpi_buf_free(vb);
                } else {
                        mpi->read_cb(mpi->cb_data, channel, vb);
                }

                mutex_lock(&mpi->recv_worker_lock);

                mpi->stats.rxres++;
                budget--;
        }

        if (!budget) {
                schedule_work(&mpi->recv_worker); /* HZ/20 */
        } else if (!vmpi_impl_receive_cb(vi, 1)) {
                vmpi_impl_receive_cb(vi, 0);
                goto again;
        }

        mutex_unlock(&mpi->recv_worker_lock);
}

static void
recv_callback(vmpi_impl_info_t *vi)
{
        struct vmpi_info *mpi = vmpi_info_from_vmpi_impl_info(vi);

        mpi->stats.rxint++;

        vmpi_impl_receive_cb(vi, 0);
        schedule_work(&mpi->recv_worker);
}

struct vmpi_info *
vmpi_init(vmpi_impl_info_t *vi, int *ret)
{
        struct vmpi_info *mpi;
        struct vmpi_ops ops;

        *ret = -ENOMEM;

        if (vi == NULL) {
                printk("Invalid vmpi_init() null implementation\n");
                *ret = -ENXIO;

                return NULL;
        }

        /* Allocate the vmpi_info instance. */
        mpi = kmalloc(sizeof(*mpi), GFP_KERNEL);
        if (mpi == NULL) {
                return NULL;
        }
        memset(mpi, 0, sizeof(*mpi));
        vmpi_stats_init(&mpi->stats);

        mpi->vi = vi;

        spin_lock_init(&mpi->write_lock);
        INIT_WORK(&mpi->recv_worker, recv_worker_function);
        mutex_init(&mpi->recv_worker_lock);

        vmpi_impl_callbacks_register(mpi->vi, xmit_callback, recv_callback);

        ops.priv = mpi;
        ops.write = vmpi_guest_write;
        ops.register_cbs = vmpi_guest_register_cbs;
        ops.unregister_cbs = vmpi_guest_unregister_cbs;
        vmpi_provider_register(VMPI_PROVIDER_GUEST, &ops, &mpi->id);

        printk("vmpi_init completed\n");

        *ret = 0;

        return mpi;
}

void
vmpi_fini(struct vmpi_info *mpi)
{
        vmpi_provider_unregister(VMPI_PROVIDER_GUEST, mpi->id);

        if (mpi == NULL) {
                printk("vmpi_info_fini: NULL pointer\n");
                BUG_ON(1);
                return;
        }
        /*
         * Deregister the callbacks, so that vmpi-impl will stop
         * scheduling the receive worker.
         */
        vmpi_impl_callbacks_unregister(mpi->vi);

        /*
         * Cancel and flush any pending receive work, so that we
         * can free the RX resources.
         */
        cancel_work_sync(&mpi->recv_worker);

        vmpi_stats_fini(&mpi->stats);
        kfree(mpi);

        printk("vmpi_fini completed\n");
}
