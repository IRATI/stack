/*
 * An hypervisor-side VMPI implemented using the vmpi-impl hypervisor interface
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

#include <linux/compat.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/socket.h>
#include <linux/list.h>
#include <linux/wait.h>

#include "vmpi-iovec.h"
#include "vmpi-host-impl.h"
#include "vmpi.h"


struct vmpi_info {
        struct vmpi_impl_info *vi;
        unsigned int id;
};

static int
vmpi_host_register_cbs(struct vmpi_ops *ops, vmpi_read_cb_t rcb,
                       vmpi_write_restart_cb_t wcb, void *opaque)
{
        struct vmpi_info *mpi = ops->priv;

        return vmpi_impl_register_cbs(mpi->vi, rcb, wcb, opaque);
}

static int
vmpi_host_unregister_cbs(struct vmpi_ops *ops)
{
        struct vmpi_info *mpi = ops->priv;

        return vmpi_impl_unregister_cbs(mpi->vi);
}

static ssize_t
vmpi_host_write(struct vmpi_ops *ops, unsigned int channel,
                struct vmpi_buf *vb)
{
        //struct vhost_mpi_virtqueue *nvq = &mpi->vqs[VHOST_MPI_VQ_RX];
        struct vmpi_info *mpi = ops->priv;
        size_t len = vmpi_buf_len(vb);
        ssize_t ret;

        if (!mpi)
                return -EBADFD;

        ret = vmpi_impl_write_buf(mpi->vi, vb, channel);
        if (likely(ret == 0)) {
                ret = len;
        }

        vmpi_impl_txkick(mpi->vi);

        return ret;
}

struct vmpi_info *
vmpi_init(struct vmpi_impl_info *vi, int *err)
{
        struct vmpi_info *mpi;
        struct vmpi_ops ops;

        mpi = kmalloc(sizeof(struct vmpi_info), GFP_KERNEL);
        if (mpi == NULL) {
                *err = -ENOMEM;
                return NULL;
        }
        memset(mpi, 0, sizeof(*mpi));

        mpi->vi = vi;

        ops.priv = mpi;
        ops.write = vmpi_host_write;
        ops.register_cbs = vmpi_host_register_cbs;
        ops.unregister_cbs = vmpi_host_unregister_cbs;
        vmpi_provider_register(VMPI_PROVIDER_HOST, &ops, &mpi->id);

        return mpi;
}

void
vmpi_fini(struct vmpi_info *mpi)
{
        vmpi_provider_unregister(VMPI_PROVIDER_HOST, mpi->id);

        mpi->vi = NULL;
        kfree(mpi);
}
