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

#include <asm/page.h>
#include <linux/uio.h>
#include <linux/list.h>

#include "vmpi-bufs.h"


typedef void (*vmpi_read_cb_t)(void *opaque, unsigned int channel,
                               struct vmpi_buf *vb);
typedef void (*vmpi_write_restart_cb_t)(void *opaque);

struct vmpi_ops {
        /* Write a kernelspace buffer. */
        ssize_t (*write)(struct vmpi_ops *ops, unsigned int channel,
                         struct vmpi_buf *vb);
        int (*register_cbs)(struct vmpi_ops *ops, vmpi_read_cb_t rcb,
                            vmpi_write_restart_cb_t wcb, void *opaque);
        int (*unregister_cbs)(struct vmpi_ops *ops);

        /* Private: do not use. */
        void *priv;
};

unsigned int vmpi_get_max_payload_size(void);


#define VMPI_PROVIDER_HOST       0U
#define VMPI_PROVIDER_GUEST      1U
#define VMPI_PROVIDER_AUTO       2U

int vmpi_provider_find_instance(unsigned int provider, int id,
                                struct vmpi_ops *ops);

int vmpi_provider_register(unsigned int provider, const struct vmpi_ops *ops,
                           unsigned int *vid);

int vmpi_provider_unregister(unsigned int provider, unsigned int id);


#define VMPI_RING_SIZE_BITS             8
/* VMPI_RING_SIZE Must match virtio ring size. */
#define VMPI_RING_SIZE                  (1 << VMPI_RING_SIZE_BITS)
#define VMPI_RING_SIZE_MASK             (VMPI_RING_SIZE - 1)
#define VMPI_BUF_SIZE                   PAGE_SIZE

#if (PAGE_SIZE > 4096)
#warning "Page size is greater than 4096: this situation has never been tested"
#endif

struct vmpi_hdr {
        uint32_t channel;
} __attribute__((packed));

#define VMPI_BUF_CAN_PUSH

#define VERBOSE
#ifdef VERBOSE
#define IFV(x) x
#else   /* !VERBOSE */
#define IFV(x)
#endif  /* !VERBOSE */

#endif  /* __VMPI_OPS_H__ */
