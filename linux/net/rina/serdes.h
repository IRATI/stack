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

#ifndef RINA_SERDES_H
#define RINA_SERDES_H

#include "pdu.h"
#include "ipcp-instances.h"

struct pdu_ser;

struct pdu_ser * serdes_pdu_ser(const struct dt_cons * dt_cons,
                                struct pdu *           pdu);
/* FIXME: Change to read-write and/or read-only */
struct buffer *  serdes_pdu_buffer(struct pdu_ser * pdu);
int              serdes_pdu_destroy(struct pdu_ser * pdu);

struct pdu *     serdes_pdu_deser(const struct dt_cons * dt_cons,
                                  struct pdu_ser * pdu);

#endif
