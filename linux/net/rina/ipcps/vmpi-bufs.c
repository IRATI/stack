/*
 * VMPI buffer adapter
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

#include <linux/gfp.h>
#include "vmpi-bufs.h"


struct vmpi_buf *
vmpi_buf_alloc(size_t size, size_t unused, gfp_t gfp)
{
        struct sdu *sdu;

        if (gfp == GFP_ATOMIC) {
                sdu = sdu_create_ni(size);
        } else {
                sdu = sdu_create(size);
        }

        if (!sdu) {
                return NULL;
        }

        return sdu;
}
EXPORT_SYMBOL_GPL(vmpi_buf_alloc);

void
vmpi_buf_free(struct vmpi_buf *vb)
{
        sdu_destroy(vb);
}
EXPORT_SYMBOL_GPL(vmpi_buf_free);

uint8_t *
vmpi_buf_data(struct vmpi_buf *vb)
{
        return sdu_buffer(vb);
}
EXPORT_SYMBOL_GPL(vmpi_buf_data);

size_t
vmpi_buf_size(struct vmpi_buf *vb)
{ return (size_t)sdu_len(vb); }
EXPORT_SYMBOL_GPL(vmpi_buf_size);

size_t
vmpi_buf_len(struct vmpi_buf *vb)
{ return (size_t)sdu_len(vb); }
EXPORT_SYMBOL_GPL(vmpi_buf_len);

//void
//vmpi_buf_set_len(struct vmpi_buf *vb, size_t len)
//{
//        buffer_set_length(sdu_buffer_rw(vb), len);
//}
//EXPORT_SYMBOL_GPL(vmpi_buf_set_len);
