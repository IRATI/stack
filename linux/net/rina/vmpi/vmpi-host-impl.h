/*
 * An hypervisor-side vmpi-impl interface
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

#ifndef __VMPI_HOST_IMPL_H__
#define __VMPI_HOST_IMPL_H__

#include <linux/compat.h>
#include <linux/eventfd.h>
#include <linux/vhost.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/file.h>
#include <linux/slab.h>
#include <linux/aio.h>

#include "vmpi.h"
#include "vmpi-structs.h"
#include "vmpi-stats.h"


typedef struct vmpi_impl_info vmpi_impl_info_t;
typedef struct vmpi_info vmpi_info_t;

int vmpi_impl_write_buf(vmpi_impl_info_t *vi, struct vmpi_buffer *buf);
int vmpi_impl_register_read_callback(vmpi_impl_info_t *vi, vmpi_read_cb_t,
                                     void* opaque);

struct vmpi_ring *vmpi_get_write_ring(vmpi_info_t *mpi);
struct vmpi_queue *vmpi_get_read_queue(vmpi_info_t *mpi);
struct vmpi_stats *vmpi_get_stats(vmpi_info_t *mpi);
struct vmpi_info *vmpi_init(vmpi_impl_info_t *vi, int *err,
                            bool deferred_test_init);
void vmpi_fini(vmpi_info_t *mpi, bool deferred_test_fini);
vmpi_info_t *vmpi_info_from_vmpi_impl_info(vmpi_impl_info_t *vi);

#endif  /* __VMPI_HOST_IMPL_H__ */
