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

#include <linux/export.h>
#include <linux/types.h>

#define RINA_PREFIX "pdu-ser"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "pdu-ser.h"
#include "buffer.h"

struct pdu_ser {
        struct buffer * buf;
};

bool pdu_ser_is_ok(const struct pdu_ser * s)
{ return (s && buffer_is_ok(s->buf)) ? true : false; }
EXPORT_SYMBOL(pdu_ser_is_ok);

struct pdu_ser * pdu_ser_create_buffer_with_gfp(gfp_t           flags,
                                                struct buffer * buffer)
{
        struct pdu_ser * tmp;

        if (!buffer_is_ok(buffer))
                return NULL;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->buf = buffer;

        return tmp;
}

struct pdu_ser * pdu_ser_create_buffer_with(struct buffer * buffer)
{ return pdu_ser_create_buffer_with_gfp(GFP_KERNEL, buffer); }

struct pdu_ser * pdu_ser_create_buffer_with_ni(struct buffer * buffer)
{ return pdu_ser_create_buffer_with_gfp(GFP_ATOMIC, buffer); }
EXPORT_SYMBOL(pdu_ser_create_buffer_with_ni);

int pdu_ser_destroy(struct pdu_ser * pdu)
{
        if (!pdu)
                return -1;

        if (pdu->buf)
                buffer_destroy(pdu->buf);

        rkfree(pdu);

        return 0;
}
EXPORT_SYMBOL(pdu_ser_destroy);

struct buffer * pdu_ser_buffer(struct pdu_ser * pdu)
{
        if (!pdu_ser_is_ok(pdu))
                return NULL;

        return pdu->buf;
}
EXPORT_SYMBOL(pdu_ser_buffer);

int pdu_ser_buffer_disown(struct pdu_ser * pdu)
{
        if (!pdu)
                return -1;

        pdu->buf = NULL;

        return 0;
}
EXPORT_SYMBOL(pdu_ser_buffer_disown);

int pdu_ser_head_grow_gfp(gfp_t flags, struct pdu_ser * pdu, size_t bytes)
{ return buffer_head_grow(flags, pdu->buf, bytes); }
EXPORT_SYMBOL(pdu_ser_head_grow_gfp);

int pdu_ser_head_shrink_gfp(gfp_t flags, struct pdu_ser * pdu, size_t bytes)
{ return buffer_head_shrink(flags, pdu->buf, bytes); }
EXPORT_SYMBOL(pdu_ser_head_shrink_gfp);

int pdu_ser_tail_grow_gfp(struct pdu_ser * pdu, size_t bytes)
{ return buffer_tail_grow(pdu->buf, bytes); }
EXPORT_SYMBOL(pdu_ser_tail_grow_gfp);

int pdu_ser_tail_shrink_gfp(struct pdu_ser * pdu, size_t bytes)
{ return buffer_tail_shrink(pdu->buf, bytes); }
EXPORT_SYMBOL(pdu_ser_tail_shrink_gfp);
