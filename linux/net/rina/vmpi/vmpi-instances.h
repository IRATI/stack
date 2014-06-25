/*
 * Support for lists of VMPI instances
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

#include "vmpi-ops.h"

static ssize_t
vmpi_ops_write(struct vmpi_ops *ops, unsigned int channel,
               const struct iovec *iv, unsigned long iovcnt)
{
        return vmpi_write_common(ops->priv, channel, iv, iovcnt, 0);
}

static int
vmpi_ops_register_read_callback(struct vmpi_ops *ops, vmpi_read_cb_t cb,
                                void *opaque)
{
        return vmpi_register_read_callback(ops->priv, cb, opaque);
}


