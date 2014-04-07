/* Test interface for VMPI on the hypervisor
 *
 * Copyright 2014 Vincenzo Maffione <v.maffione@nextworks.it> Nextworks
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/uio.h>
#include <linux/aio.h>
#include <linux/compat.h>
#include <linux/fs.h>

#include "vmpi.h"
#include "vmpi-host-test.h"


#ifdef VMPI_HOST_TEST
struct vmpi_info *vmpi_info_from_file_private_data(void *);

ssize_t vhost_mpi_aio_write(struct kiocb *iocb, const struct iovec *iv,
                            unsigned long iovcnt, loff_t pos)
{
    struct file *file = iocb->ki_filp;
    vmpi_info_t *mpi = vmpi_info_from_file_private_data(file->private_data);

    return vmpi_write(mpi, iv, iovcnt);
}

ssize_t vhost_mpi_aio_read(struct kiocb *iocb, const struct iovec *iv,
                           unsigned long iovcnt, loff_t pos)
{
    struct file *file = iocb->ki_filp;
    vmpi_info_t *mpi = vmpi_info_from_file_private_data(file->private_data);
    ssize_t ret;

    ret = vmpi_read(mpi, iv, iovcnt);

    if (ret > 0)
        iocb->ki_pos = ret;

    return ret;
}
#endif  /* VMPI_HOST_TEST */
