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
        struct buffer *buf;
        struct sdu *sdu;

        if (gfp == GFP_ATOMIC) {
                buf = buffer_create_ni(size);
                if (!buf) {
                        return NULL;
                }
                sdu = sdu_create_buffer_with_ni(buf);

        } else {
                buf = buffer_create(size);
                if (!buf) {
                        return NULL;
                }

                sdu = sdu_create_buffer_with(buf);
        }

        if (!sdu) {
                buffer_destroy(buf);
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
        return buffer_data_rw(sdu_buffer_rw(vb));
}
EXPORT_SYMBOL_GPL(vmpi_buf_data);

size_t
vmpi_buf_size(struct vmpi_buf *vb)
{
        const struct buffer *buf;

        buf = sdu_buffer_ro(vb);
        BUG_ON(!buf);

        return (size_t)buffer_length(buf);
}
EXPORT_SYMBOL_GPL(vmpi_buf_size);

size_t
vmpi_buf_len(struct vmpi_buf *vb)
{
        const struct buffer *buf;

        buf = sdu_buffer_ro(vb);
        BUG_ON(!buf);

        return (size_t)buffer_length(buf);
}
EXPORT_SYMBOL_GPL(vmpi_buf_len);

void
vmpi_buf_set_len(struct vmpi_buf *vb, size_t len)
{
        buffer_set_length(sdu_buffer_rw(vb), len);
}
EXPORT_SYMBOL_GPL(vmpi_buf_set_len);
