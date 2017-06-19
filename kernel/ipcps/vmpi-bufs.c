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
#include <linux/slab.h>
#include "vmpi-bufs.h"
#include "../sdu.h"

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

struct vmpi_buf_node *
vmpi_buf_node_alloc(struct vmpi_buf * vb, gfp_t gfp)
{
	struct vmpi_buf_node *vbn;

	vbn = kzalloc(sizeof(*vbn), gfp);
	if (!vbn)
		return NULL;

	vbn->vb = vb;
	INIT_LIST_HEAD(&vbn->node);

        return vbn;
}
EXPORT_SYMBOL_GPL(vmpi_buf_node_alloc);

void
vmpi_buf_node_free(struct vmpi_buf_node *vbn)
{
        kfree(vbn);
}
EXPORT_SYMBOL_GPL(vmpi_buf_node_free);

uint8_t *
vmpi_buf_data(struct vmpi_buf *vb)
{
        return sdu_buffer(vb);
}
EXPORT_SYMBOL_GPL(vmpi_buf_data);

size_t
vmpi_buf_len(struct vmpi_buf *vb)
{ return (size_t)sdu_len(vb); }
EXPORT_SYMBOL_GPL(vmpi_buf_len);

void
vmpi_buf_set_len(struct vmpi_buf *vb, size_t len)
{
	BUG_ON(len > sdu_len(vb));
	sdu_shrink(vb, sdu_len(vb) - len);
	return;
}
EXPORT_SYMBOL_GPL(vmpi_buf_set_len);

void
vmpi_buf_pop(struct vmpi_buf *vb, size_t len)
{ sdu_pop(vb, len); }
EXPORT_SYMBOL_GPL(vmpi_buf_pop);

void
vmpi_buf_push(struct vmpi_buf *vb, size_t len)
{ sdu_push(vb, len); }
EXPORT_SYMBOL_GPL(vmpi_buf_push);
