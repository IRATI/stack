/* A Virtual MPI interface
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

#ifndef __VMPI_OPS_H__
#define __VMPI_OPS_H__

typedef void (*vmpi_read_cb_t)(void *opaque, unsigned int channel,
                               const char *buffer, int len);

struct vmpi_ops {
        /* Write a kernelspace buffer. */
        ssize_t (*write)(struct vmpi_ops *ops, unsigned int channel,
                         const struct iovec *iv, unsigned long iovlen);
        int (*register_read_callback)(struct vmpi_ops *ops, vmpi_read_cb_t rcb,
                                      void *opaque);

        /* Private: do not use. */
        void *priv;
};

#include "vmpi-limits.h"

#endif  /* __VMPI_OPS_H__ */
