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

#include "pdu.h"

struct sdup_chksum;

struct sdup_chksum * sdup_chksum_create(size_t (* size)(void),
                                        bool   (* compute)(void * buffer,
                                                           size_t size,
                                                           void * value),
                                        bool   (* update)(void * buffer,
                                                          size_t size,
                                                          void * old,
                                                          void * new));
int                  sdup_chksum_destroy(struct sdup_chksum * inst);

int                  sdup_chksum_add(struct sdup_chksum * inst,
                                     struct pdu_ser *     pdu);
int                  sdup_chksum_remove(struct sdup_chksum * inst,
                                        struct pdu_ser *     pdu);
int                  sdup_chksum_is_ok(struct sdup_chksum * inst,
                                       struct pdu_ser *     pdu);
int                  sdup_chksum_set(struct sdup_chksum * inst,
                                     struct pdu_ser *     pdu);
int                  sdup_chksum_update(struct sdup_chksum * inst,
                                        struct pdu_ser *     pdu);

#if 0
struct sdup_ttl;

struct sdup_ttl * sdup_ttl_create();
int               sdup_ttl_destroy(struct sdup_ttl * inst);

int               sdup_ttl_add(struct sdup_ttl * inst,
                               struct pdu_ser * pdu);
int               sdup_ttl_decrement(struct sdup_ttl * inst,
                                     struct pdu_ser * pdu);
int               sdup_ttl_is_zero(struct sdup_ttl * inst,
                                   struct pdu_ser * pdu);
int               sdup_ttl_remove(struct sdup_ttl * inst,
                                  struct pdu_ser * pdu);
#endif

#endif
