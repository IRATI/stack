/*
 * Protocol Data Unit
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
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

#ifndef RINA_PDU_H
#define RINA_PDU_H

#include <linux/types.h>

#include "common.h"
#include "buffer.h"
#include "sdu.h"
#include "pci.h"

struct pdu;

struct pdu *          pdu_create(void);
struct pdu *          pdu_create_ni(void);
struct pdu *          pdu_create_with(struct sdu * sdu);
struct pdu *          pdu_create_with_ni(struct sdu * sdu);
/* FIXME: To be removed after ser/des */
struct pdu *          pdu_create_from(const struct sdu * sdu);
struct pdu *          pdu_create_from_ni(const struct sdu * sdu);

struct pdu *          pdu_dup(const struct pdu * pdu);
struct pdu *          pdu_dup_ni(const struct pdu * pdu);

/* NOTE: PCI is ok and has a buffer */
bool                  pdu_is_ok(const struct pdu * pdu);
const struct buffer * pdu_buffer_get_ro(const struct pdu * pdu);
struct buffer *       pdu_buffer_get_rw(struct pdu * pdu);

/* NOTE: Takes ownership of the buffer passed */
int                   pdu_buffer_set(struct pdu *    pdu,
                                     struct buffer * buffer);

/* NOTE: Please use this "method" instead of pdu_pci_get_*(), for checks */
bool                  pdu_pci_present(const struct pdu * pdu);

const struct pci *    pdu_pci_get_ro(const struct pdu * pdu);
struct pci *          pdu_pci_get_rw(struct pdu * pdu);
/* NOTE: Takes ownership of the PCI passed */
int                   pdu_buffer_disown(struct pdu * pdu);
int                   pdu_pci_set(struct pdu * pdu, struct pci * pci);

int                   pdu_destroy(struct pdu * pdu);

#endif
