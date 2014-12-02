/*
 * Data Unit Protection
 *
 *    Dimitri Staessens     <dimitri.staessens@intec.ugent.be>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#ifndef RINA_DU_PROTECTION_H
#define RINA_DU_PROTECTION_H

#include "pdu-ser.h"

struct dup_chksum;

struct dup_chksum * dup_chksum_create(size_t (* size)(void),
                                      bool   (* compute)(void * buffer,
                                                         size_t size,
                                                         void * value),
                                      bool   (* update)(void * buffer,
                                                        size_t size,
                                                        void * old,
                                                        void * new));
int                 dup_chksum_destroy(struct dup_chksum * inst);

int                 dup_chksum_add(struct dup_chksum * inst,
                                   struct pdu_ser *     pdu);
int                 dup_chksum_remove(struct dup_chksum * inst,
                                      struct pdu_ser *     pdu);
int                 dup_chksum_is_ok(struct dup_chksum * inst,
                                     struct pdu_ser *     pdu);
int                 dup_chksum_set(struct dup_chksum * inst,
                                   struct pdu_ser *     pdu);
int                 dup_chksum_update(struct dup_chksum * inst,
                                      struct pdu_ser *     pdu);

#if 0
struct dup_ttl;

struct dup_ttl * dup_ttl_create();
int              dup_ttl_destroy(struct dup_ttl * inst);

int              dup_ttl_add(struct dup_ttl * inst,
                             struct pdu_ser * pdu);
int              dup_ttl_decrement(struct dup_ttl * inst,
                                   struct pdu_ser * pdu);
int              dup_ttl_is_zero(struct dup_ttl * inst,
                                 struct pdu_ser * pdu);
int              dup_ttl_remove(struct dup_ttl * inst,
                                struct pdu_ser * pdu);
#endif

#endif
