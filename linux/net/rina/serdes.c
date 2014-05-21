/*
 * PDU Serialization/Deserialization 
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

#define RINA_PREFIX "serdes"

#include "serdes.h"

struct pdu_ser {
        struct buffer * buf;
};

static bool serdes_pdu_is_ok(const struct pdu_ser * s)
{ return (s && buffer_is_ok(s->buffer)) ? true : false; }

static struct pdu_ser * serdes_pdu_ser_gfp(gfp_t flags,
                                           struct pdu * pdu)
{
        struct pdu_ser * tmp;
        
        if (!pdu_is_ok(pdu))
                return NULL;

        tmp = rkzalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->buf = buffer_dup(pdu_buffer_get_ro(pdu));
        if (!tmp->buf) {
                rkfree(tmp);
                return NULL;
        } 

        pdu_destroy(pdu);

        return tmp;
}

struct pdu_ser * serdes_pdu_ser_gfp(struct pdu * pdu)
{ return serdes_pdu_ser_gfp(GFP_KERNEL, pdu); }
EXPORT_SYMBOL(serdes_pdu_ser);

struct buffer * serdes_pdu_buffer(struct pdu_ser * pdu)
{
        if (!serdes_pdu_is_ok(pdu))
                return NULL;
        
        return pdu->buf;
}
EXPORT_SYMBOL(serdes_pdu_buffer);

int serdes_pdu_destroy(struct pdu_ser * pdu) 
{
        if (!pdu) return -1;

        if (pdu->buf)
                buffer_destroy(pdu->buf);

        rkfree(pdu);

        return 0;
}
EXPORT_SYMBOL(serdes_pdu_destroy);



struct pdu * serdes_pdu_deser(struct pdu_ser * pdu) 
{
        LOG_MISSING;
        
        return NULL;
}
EXPORT_SYMBOL(serdes_pdu_deser);
