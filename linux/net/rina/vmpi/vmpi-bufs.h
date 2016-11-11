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

#ifndef __VMPI_BUFS_H__
#define __VMPI_BUFS_H__

#include "../sdu.h"

#define vmpi_buf sdu

struct vmpi_buf_node {
	struct vmpi_buf *vb;
	struct list_head node;
};

struct vmpi_buf * vmpi_buf_alloc(size_t size, size_t unused, gfp_t gfp);

void vmpi_buf_free(struct vmpi_buf *vb);

struct vmpi_buf_node * vmpi_buf_node_alloc(struct vmpi_buf *vb, gfp_t gfp);

void vmpi_buf_node_free(struct vmpi_buf_node *vbn);

uint8_t * vmpi_buf_data(struct vmpi_buf *vb);

size_t vmpi_buf_len(struct vmpi_buf *vb);

void vmpi_buf_set_len(struct vmpi_buf *vb, size_t len);

void vmpi_buf_pop(struct vmpi_buf *vb, size_t len);

void vmpi_buf_push(struct vmpi_buf *vb, size_t len);
#endif /* __VMPI_BUFS_H__ */
