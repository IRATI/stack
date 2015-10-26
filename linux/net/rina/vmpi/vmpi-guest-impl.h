/*
 * A guest-side vmpi-impl interface
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

#ifndef __VMPI_GUEST_IMPL_H__
#define __VMPI_GUEST_IMPL_H__

#include "vmpi-bufs.h"

typedef struct vmpi_impl_info vmpi_impl_info_t;
typedef struct vmpi_info vmpi_info_t;

typedef void (*vmpi_impl_callback_t)(vmpi_impl_info_t *);

int vmpi_impl_write_buf(vmpi_impl_info_t *vi, struct vmpi_buf *buf,
                        unsigned int channel);
void vmpi_impl_txkick(vmpi_impl_info_t *vi);
bool vmpi_impl_tx_should_stop(vmpi_impl_info_t *vi);
struct vmpi_buf *vmpi_impl_get_written_buffer(vmpi_impl_info_t *vi);
struct vmpi_buf *vmpi_impl_read_buffer(vmpi_impl_info_t *vi,
                                       unsigned int *channel);
bool vmpi_impl_send_cb(vmpi_impl_info_t *vi, int enable);
bool vmpi_impl_receive_cb(vmpi_impl_info_t *vi, int enable);
void vmpi_impl_callbacks_register(vmpi_impl_info_t *vi,
                                  vmpi_impl_callback_t xmit,
                                  vmpi_impl_callback_t recv);
void vmpi_impl_callbacks_unregister(vmpi_impl_info_t *vi);

vmpi_info_t *vmpi_info_from_vmpi_impl_info(vmpi_impl_info_t *vi);
vmpi_info_t *vmpi_init(vmpi_impl_info_t *vi, int *err);
void vmpi_fini(vmpi_info_t *mpi);


#endif  /* __VMPI_GUEST_IMPL_H__ */
