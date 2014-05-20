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

struct pdu_ser * serdes_pdu_ser(struct pdu * pdu)
{
        LOG_MISSING;

        return NULL;
}

struct buffer *  serdes_pdu_buffer(struct pdu_ser * pdu)
{
        LOG_MISSING;
        
        return NULL;
}

int serdes_pdu_destroy(struct pdu_ser * pdu) 
{
        LOG_MISSING;

        return -1;
}

struct pdu * serdes_pdu_deser(struct pdu_ser * pdu) 
{
        LOG_MISSING;
        
        return NULL;
}
