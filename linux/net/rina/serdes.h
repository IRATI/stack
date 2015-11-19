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
#include "pdu-ser.h"
#include <linux/crypto.h>

struct serdes;

struct serdes *        serdes_create(struct dt_cons * dt_cons);
const struct dt_cons * serdes_dt_cons(const struct serdes * instance);
int                    serdes_destroy(struct serdes * instance);

struct pdu_ser *       pdu_serialize(const struct serdes * instance,
                                     struct pdu *          pdu);
struct pdu_ser *       pdu_serialize_ni(const struct serdes * instance,
                                        struct pdu *          pdu);
struct pdu *           pdu_deserialize(const struct serdes * instance,
                                       struct pdu_ser *      pdu);
struct pdu *           pdu_deserialize_ni(const struct serdes * instance,
                                          struct pdu_ser *      pdu);
#endif
