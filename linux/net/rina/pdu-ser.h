/*
 * Serialized PDU
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
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

#ifndef RINA_PDU_SER_H
#define RINA_PDU_SER_H

#include <linux/export.h>
#include <linux/types.h>

#include "buffer.h"
#include "common.h"

struct pdu_ser;

struct pdu_ser * pdu_ser_create_buffer_with(struct buffer * buffer);
struct pdu_ser * pdu_ser_create_buffer_with_ni(struct buffer * buffer);
int              pdu_ser_destroy(struct pdu_ser * pdu);

bool             pdu_ser_is_ok(const struct pdu_ser * s);
struct buffer *  pdu_ser_buffer(struct pdu_ser * pdu);
int              pdu_ser_buffer_disown(struct pdu_ser * pdu);

int              pdu_ser_head_grow_gfp(gfp_t flags, struct pdu_ser * pdu, size_t bytes);
int              pdu_ser_head_shrink_gfp(gfp_t flags, struct pdu_ser * pdu, size_t bytes);

int              pdu_ser_tail_grow_gfp(struct pdu_ser * pdu, size_t bytes);
int              pdu_ser_tail_shrink_gfp(struct pdu_ser * pdu, size_t bytes);

#endif
