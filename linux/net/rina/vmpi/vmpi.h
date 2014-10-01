/*
 * A Virtual MPI interface
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

#ifndef __VMPI_H__
#define __VMPI_H__


typedef struct vmpi_info vmpi_info_t;

/* Do not call this function directly, use the wrappers instead. */
ssize_t vmpi_write_common(vmpi_info_t *mpi, unsigned int channel,
                          const struct iovec *iv, unsigned long iovlen,
                          bool user);

/* Use vmpi_write() when writing an userspace buffer. */
static inline ssize_t
vmpi_write(vmpi_info_t *mpi, unsigned int channel,
           const struct iovec *iv, unsigned long iovlen)
{
        return vmpi_write_common(mpi, channel, iv, iovlen, 1);
}

/* Use vmpi_write_kernel() when writing a kernelspace buffer. */
static inline ssize_t
vmpi_write_kernel(vmpi_info_t *mpi, unsigned int channel,
                  const struct iovec *iv, unsigned long iovlen)
{
        return vmpi_write_common(mpi, channel, iv, iovlen, 0);
}

/* Use vmpi_read() when reading into an userspace buffer. */
ssize_t vmpi_read(vmpi_info_t *mpi, unsigned int channel,
                  const struct iovec *iv, unsigned long iovcnt);

typedef void (*vmpi_read_cb_t)(void *opaque, unsigned int channel,
                               const char *buffer, int len);

int vmpi_register_read_callback(vmpi_info_t *mpi, vmpi_read_cb_t rcb,
                                void *opaque);

#include "vmpi-limits.h"

#endif  /* __VMPI_H__ */
