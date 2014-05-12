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

#ifndef __VMPI_HOST_TEST_H__
#define __VMPI_HOST_TEST_H__

/* Enable hypervisor-side test interface. */
//#define VMPI_HOST_TEST

#ifdef VMPI_HOST_TEST
ssize_t vhost_mpi_aio_write(struct kiocb *iocb, const struct iovec *iv,
                            unsigned long iovcnt, loff_t pos);
ssize_t vhost_mpi_aio_read(struct kiocb *iocb, const struct iovec *iv,
                           unsigned long iovcnt, loff_t pos);
#endif  /* VMPI_HOST_TEST */

#endif  /* __VMPI_HOST_TEST_H__ */

