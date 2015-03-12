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

#ifndef __VMPI_OPS_H__
#define __VMPI_OPS_H__

#include <linux/aio.h>

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

unsigned int vmpi_get_num_channels(void);
unsigned int vmpi_get_max_payload_size(void);

/* Use mutexes on the TX datapath (in place of spinlocks). */
// #define VMPI_TX_MUTEX

#endif  /* __VMPI_OPS_H__ */
