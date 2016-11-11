/*
 * Protocol Data Unit
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
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
#include "pci.h"

struct sdu;
struct pdu;

struct pdu *pdu_create(pdu_type_t type,
		       struct efcp_config *cfg);
struct pdu *pdu_create_ni(pdu_type_t type,
			  struct efcp_config *cfg);

struct pdu *pdu_from_sdu(struct sdu *sdu);
int pdu_encap(struct pdu *pdu, pdu_type_t type);
int pdu_decap(struct pdu *pdu);

struct pdu *pdu_dup(const struct pdu *pdu);
struct pdu *pdu_dup_ni(const struct pdu *pdu);

bool pdu_is_ok(const struct pdu *pdu);

const struct pci *pdu_pci_get_ro(const struct pdu *pdu);
struct pci	 *pdu_pci_get_rw(struct pdu *pdu);
ssize_t		 pdu_data_len(const struct pdu *pdu);
ssize_t		 pdu_len(const struct pdu *pdu);

int pdu_destroy(struct pdu *pdu);

/* added for SDU protection */
void *pdu_sdup_head(struct pdu *pdu);
void *pdu_sdup_tail(struct pdu *pdu);
int pdu_sdup_head_set(struct pdu *pdu, void *header);
int pdu_sdup_tail_set(struct pdu *pdu, void *tail);
unsigned char *pdu_buffer(const struct pdu *pdu);
int pdu_tail_grow(struct pdu *pdu, size_t bytes);
int pdu_tail_shrink(struct pdu *pdu, size_t bytes);
int pdu_head_grow(struct pdu *pdu, size_t bytes);
int pdu_head_shrink(struct pdu *pdu, size_t bytes);
#endif
